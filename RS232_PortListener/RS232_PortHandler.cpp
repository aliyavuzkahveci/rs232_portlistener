#include "RS232_PortHandler.h"

#include <chrono>
#include <SetupAPI.h>
#pragma comment(lib, "Setupapi.lib")

namespace RS232
{
	RS232_PortHandler::RS232_PortHandler(RS232_PortSubscriber_Ptr subscriber, RS232_PortParams_Ptr portParams) :
		m_subscriber(subscriber),
		m_portParams(portParams),
		m_bOpenSuccess(false),
		m_ReadTerminated(false),
		m_PortHandlerClosed(false)
	{
		openPortHandler();
	}

	RS232_PortHandler::~RS232_PortHandler()
	{
		free(m_ReadData);
	}

	void RS232_PortHandler::openPortHandler()
	{
		std::string comPort;
		if (m_portParams->m_comPort.length() == 4) /*COM1-9*/
		{
			comPort = m_portParams->m_comPort;
		}
		else /*COM10-99*/
		{
			comPort = COM_PORT_PREPEND + m_portParams->m_comPort;
		}

		m_HSerialPort = CreateFile(
			comPort.c_str(),
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL |
			FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING,
			NULL);

		DWORD errorNumber = GetLastError();

		m_bOpenSuccess = true;

		if (m_HSerialPort == INVALID_HANDLE_VALUE || errorNumber == ERROR_ACCESS_DENIED || errorNumber == ERROR_FILE_NOT_FOUND)
		{	//ERROR_ACCESS_DENIED => COM port is used by another application!
			//ERROR_FILE_NOT_FOUND => COM port does not exist in the system!
			std::cout << "RS232_PortHandler::openPortHandler() -> Unable to open Serial Port" << m_portParams->m_comPort << std::endl;
			m_bOpenSuccess = false;
		}
	}

	void RS232_PortHandler::init()
	{
		m_BufferSize = m_portParams->m_rxBufferSize;
		m_ReadData = (char*)malloc(m_BufferSize * sizeof(char));
		memset(m_ReadData, 0, sizeof(m_ReadData));
		m_LPReadData = m_ReadData;

		DWORD threadID;

		DCB dcb; //Device Control Block
		memset(&dcb, 0, sizeof(DCB));
		dcb.DCBlength = sizeof(DCB);
		dcb.BaudRate = (int)m_portParams->m_baudRate;
		dcb.fBinary = TRUE;
		dcb.fDtrControl = DTR_CONTROL_ENABLE;
		dcb.fRtsControl = RTS_CONTROL_ENABLE;
		dcb.ByteSize = ConvertCharSize(m_portParams->m_charSize);
		dcb.Parity = ConvertParity(m_portParams->m_parity);
		dcb.StopBits = ConvertStopBits(m_portParams->m_stopBits);
		dcb.fOutX = ConvertFlowControl(m_portParams->m_flowControl);
		dcb.fInX = ConvertFlowControl(m_portParams->m_flowControl);
		dcb.XonChar = ASCII_DC1;
		dcb.XoffChar = ASCII_DC3;

		//Hardcoded Configuration Below
		SetCommState(m_HSerialPort, &dcb);

		//SetCommMask will trigger WaitCommEvent
		SetCommMask(m_HSerialPort, EV_RXCHAR | EV_CTS | EV_DSR | EV_RLSD | EV_ERR | EV_RING);

		//Start Thread for Serial Port Handling
		m_HReadDone = CreateEvent(NULL, FALSE, FALSE, NULL);

		//Start Thread for Serial Port Handling
		m_HReadDone = CreateEvent(NULL, FALSE, FALSE, NULL);
		m_HReadThread = CreateThread(NULL, 0, RS232_PortHandler::startReadThread, this, 0, &threadID);
	}

	void RS232_PortHandler::close()
	{
		//Close the Serial Port
		if (m_bOpenSuccess) //close the serial port handles iff serial port is opened successfully!
		{
			CloseHandle(m_HSerialPort);
			CloseHandle(m_HReadDone);
			CloseHandle(m_HReadThread);
		}
		m_ReadTerminated = true;
		m_bOpenSuccess = false;
		m_PortHandlerClosed = true;
	}

	void RS232_PortHandler::write(const unsigned char* data, unsigned int length)
	{
		std::lock_guard<std::mutex> lock(m_writeGuard);

		if (!m_bOpenSuccess)
		{
			std::cout << "RS232_PortHandler::write() -> serial port NOT active!" << std::endl;
			return;
		}

		DWORD numOFWrittenBytes;
		OVERLAPPED ovlWrite;
		ovlWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		// Reset event before writing to COM port
		ResetEvent(ovlWrite.hEvent);
		ovlWrite.OffsetHigh = ovlWrite.Offset = 0;
		if (!WriteFile(m_HSerialPort, data, length, &numOFWrittenBytes, &ovlWrite))
		{
			DWORD errorNumber = GetLastError();
			if (errorNumber == ERROR_IO_PENDING)
			{	// Wait for data to be written
				GetOverlappedResult(m_HSerialPort, &ovlWrite, &numOFWrittenBytes, TRUE);
				if (length != numOFWrittenBytes)
				{
					std::cout << "RS232_PortHandler::write() -> Data could NOT be written to the port!" << std::endl;
				}
			}
			else
			{
				std::cout << "RS232_PortHandler::write() -> Data could NOT be written to the port!" << std::endl;
			}
		}
		CloseHandle(ovlWrite.hEvent);
	}

	DWORD WINAPI RS232_PortHandler::startReadThread(LPVOID lpV)
	{
		return static_cast<RS232_PortHandler*>(lpV)->read();
	}

	DWORD RS232_PortHandler::read()
	{
		std::lock_guard<std::mutex> lock(m_readGuard);

		DWORD dwBytesRead = 0;
		DWORD dwEvent;
		OVERLAPPED ovlRead;
		COMSTAT comStat;
		DWORD dwErrors;

		// Get an initial comm status
		if (updatePinStatus())
		{	// We can NOT get comm status, so exit this thread
			std::cout << "RS232_PortHandler::read() -> Cannot get comm status! Exiting reader thread!" << std::endl;
			m_subscriber->on_socket_error(PE_CannotOpenPort);
			SetEvent(m_HReadDone);
			return 0;
		}

		// Create the event for overlapped reads
		ovlRead.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

		m_ReadTerminated = false; //set false when starting to read!
		while (!m_ReadTerminated)
		{
			// Reset event before waiting for comm event
			ResetEvent(ovlRead.hEvent);
			ovlRead.OffsetHigh = ovlRead.Offset = 0;

			// Wait for comm event
			DWORD errorNumber;
			if (!WaitCommEvent(m_HSerialPort, &dwEvent, &ovlRead))
			{
				// Is data pending?
				if ((errorNumber = GetLastError()) == ERROR_IO_PENDING)
				{
					// Wait for data to be received
					GetOverlappedResult(m_HSerialPort, &ovlRead, &dwBytesRead, TRUE);
				}
				else if ((errorNumber = GetLastError()) == ERROR_ACCESS_DENIED)
				{
					std::cout << "RS232_PortHandler::read() -> Serial Cable is unplugged" << std::endl;
					m_subscriber->on_socket_error(PE_PortIsNotOpen);
					break;
				}
			}

			// Is data available?
			if (!m_ReadTerminated && ((dwEvent & EV_RXCHAR) || (dwEvent == 0)))
			{
				std::cout << "RS232_PortHandler::read() ->  Data is available" << std::endl;
				// Reset event before reading COM data
				ResetEvent(ovlRead.hEvent);
				ovlRead.OffsetHigh = ovlRead.Offset = 0;

				if (m_LPReadData)
				{
					ClearCommError(m_HSerialPort, &dwErrors, &comStat);
					// Read data from COM port
					if (!ReadFile(m_HSerialPort, m_LPReadData, comStat.cbInQue, &dwBytesRead, &ovlRead))
					{
						// Is more data pending?
						if ((errorNumber = GetLastError()) == ERROR_IO_PENDING)
						{
							// Wait for read to complete
							if (!GetOverlappedResult(m_HSerialPort, &ovlRead, &dwBytesRead, TRUE))
							{
								std::cout << "RS232_PortHandler::read() ->  overlapped read FAILED!" << std::endl;
							}
						}
					}

					// Did we receive data?
					if (dwBytesRead)
					{
						m_subscriber->on_read((unsigned char *)m_LPReadData, dwBytesRead);
						// Zero buffer for next data read
						memset(m_ReadData, 0, sizeof(m_ReadData));
					}
				}
			}

			// Did we also receive a CTS or DSR or RLSD or RING line event?
			if (!m_ReadTerminated && ((dwEvent & EV_CTS) || (dwEvent & EV_DSR) || (dwEvent & EV_RLSD) || (dwEvent & EV_RING)))
			{
				//std::cout << "RS232_PortHandler::read() ->  Serial Signal change information is available" << std::endl;
				updatePinStatus();
			}

			if (!m_ReadTerminated && (dwEvent & EV_ERR))
			{
				std::cout << "RS232_PortHandler::read() ->  Error happened in the serial port! Exiting..." << std::endl;
				m_subscriber->on_socket_error(PE_ReadError);
				break;
			}
		}

		CloseHandle(ovlRead.hEvent);
		SetEvent(m_HReadDone);
		return 0;
	}

	DWORD RS232_PortHandler::updatePinStatus()
	{
		DWORD  error = 0;
		DWORD  ModemStat; //MS_CTS_ON=0x0010 MS_DSR_ON=0x0020 MS_RING_ON=0x0040 MS_RLSD_ON=0x0080

		RS232_PinStatus newStatus;

		// Get the current modem status
		if (!GetCommModemStatus(m_HSerialPort, &ModemStat))
		{
			// Get extended error code
			error = GetLastError();
			std::cout << "RS232_PortHandler::updatepinStatus() -> ERRROR - GetCommModemStatus failure" << std::endl;
		}
		else
		{
			// Extract the bits we want to examine
			ModemStat &= (DWORD)(MS_RLSD_ON | MS_RING_ON | MS_DSR_ON | MS_CTS_ON);

			newStatus.m_CTS_on = (ModemStat & MS_CTS_ON) != 0;
			newStatus.m_DSR_on = (ModemStat & MS_DSR_ON) != 0;
			newStatus.m_RI_on = (ModemStat & MS_RING_ON) != 0;
			newStatus.m_RLSD_on = (ModemStat & MS_RLSD_ON) != 0;
		}

		if (m_pinStatus != newStatus)
		{
			m_pinStatus = newStatus;
			m_subscriber->on_serialstate_changed(m_pinStatus);
		}
		return (error);
	}

	void RS232_PortHandler::waitForPortToBecomeAvailable()
	{
		std::cout << "RS232_PortHandler::waitForPortToBecomeAvailable()" << std::endl;
		m_PortHandlerClosed = false;
		while (!m_PortHandlerClosed)
		{
			std::vector<std::string> comPortNames = getComPortNames();
			if (std::find(comPortNames.begin(), comPortNames.end(), m_portParams->m_comPort) != comPortNames.end())
			{	//given comport exists in the list!
				openPortHandler();
				if (is_active())
				{
					init();
					break;
				}
			}
			else
			{	//given comport does NOT exist in the list!
				std::this_thread::sleep_for(std::chrono::seconds(3));
				continue; //continue to retry!
			}
		}
		return;
	}

	std::vector<std::string> RS232_PortHandler::getComPortNames()
	{
		std::vector<std::string> openPortNames;

		BOOL returnVal;
		DWORD size;
		GUID guid[1];
		HDEVINFO hDevInfo;
		DWORD idx = 0;
		SP_DEVINFO_DATA devInfoData;
		devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
		int count = 0;

		returnVal = SetupDiClassGuidsFromName("Ports", (LPGUID)&guid, 1, &size);
		if (!returnVal)
		{
			std::cout << "RS232_PortHandler::getComPortNames() -> error : SetupDiClassGuidsFromName() failed..." << std::endl;
			return openPortNames;
		}

		hDevInfo = SetupDiGetClassDevs(&guid[0], NULL, NULL, DIGCF_PRESENT | DIGCF_PROFILE);
		if (hDevInfo == INVALID_HANDLE_VALUE)
		{
			std::cout << "RS232_PortHandler::getComPortNames() -> error : SetupDiGetClassDevs() failed..." << std::endl;
			return openPortNames;
		}

		while (SetupDiEnumDeviceInfo(hDevInfo, idx++, &devInfoData))
		{
			char friendlyName[MAX_PATH];
			char portName[MAX_PATH];
			DWORD propType;
			DWORD type = REG_SZ;
			HKEY hKey = NULL;

			returnVal = ::SetupDiGetDeviceRegistryProperty(
				hDevInfo, &devInfoData, SPDRP_FRIENDLYNAME, &propType,
				(LPBYTE)friendlyName, sizeof(friendlyName), &size);

			if (!returnVal)
			{
				std::cout << "RS232_PortHandler::getComPortNames() -> error : SetupDiGetDeviceRegistryProperty() failed..." << std::endl;
				continue;
			}

			hKey = ::SetupDiOpenDevRegKey(hDevInfo, &devInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
			if (!hKey)
			{
				std::cout << "RS232_PortHandler::getComPortNames() -> error : SetupDiOpenDevRegKey() failed..." << std::endl;
				continue;
			}

			size = sizeof(portName);
			returnVal = ::RegQueryValueEx(hKey, "PortName", 0, &type, (LPBYTE)&portName, &size);
			::RegCloseKey(hKey);

			openPortNames.push_back(portName);
		}

		SetupDiDestroyDeviceInfoList(hDevInfo);
		return openPortNames;
	}

}
