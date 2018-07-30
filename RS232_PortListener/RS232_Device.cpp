#include "RS232_Device.h"
#include "Base64.h"

#include <sstream>

#include <boost/algorithm/string.hpp>

namespace RS232
{
	RS232_Device::RS232_Device(RS232_PortParams_Ptr portParams) :
		m_portParams(portParams),
		m_DLEReceived(false),
		m_firstNonPrintableCharPos(0),
		m_receiveStatus(WaitingForSTX),
		m_tempEOD(0x00),
		m_tempDelims("")
	{
		if (m_portParams->m_dcList.size() != 0)
			m_receiveStatus = WaitingForSOD;
		m_bufferSize = m_portParams->m_txBufferSize;
	}

	RS232_Device::~RS232_Device()
	{
		m_portHandler.reset();
		m_portParams.reset();
	}

	void RS232_Device::openDevice()
	{
		m_portHandler = RS232_PortHandler_Ptr(new RS232_PortHandler(shared_from_this(), m_portParams));
		if (m_portHandler.get())
		{
			if (m_portHandler->is_active())
				m_portHandler->init();
			else
				std::thread(&RS232_PortHandler::waitForPortToBecomeAvailable, m_portHandler).detach();
		}
	}

	void RS232_Device::closeDevice()
	{
		if (m_portHandler.get())
			m_portHandler->close();
	}

	void RS232_Device::sendMessageToDevice(const unsigned char *data, unsigned int len)
	{
		std::lock_guard<std::mutex> lock(m_writeGuard);
		//startStopResponseTimer();

		std::string encapsulatedMsg = encapsulateMessage(std::string(reinterpret_cast<const char *>(data), len));
		//constructHexAndLog(WriteData, encapsulatedMsg);
		this->writeToPort(encapsulatedMsg);
	}

	void RS232_Device::sendMessageToDevice(const std::string& msg)
	{
		std::lock_guard<std::mutex> lock(m_writeGuard);
		//startStopResponseTimer();

		std::string encapsulatedMsg = encapsulateMessage(msg);
		//constructHexAndLog(WriteData, encapsulatedMsg);
		writeToPort(encapsulatedMsg);

	}

	void RS232_Device::on_read(const unsigned char *readData, unsigned int dataLength)
	{
		std::lock_guard<std::mutex> lock(m_readGuard);
		//startStopResponseTimer(false);

		try
		{
			for (unsigned int i = 0; i < dataLength; i++) //for all chars in string
			{
				char ch = readData[i];
				//m_toBeLoggedBuffer.push_back(ch);
				switch (m_receiveStatus)
				{
				case WaitingForSOD:
				{
					m_tempEOD = m_portParams->getEOD(ch);
					if(m_tempEOD != ASCII_NULL)
					{
						std::cout << "RS232_Device::on_read() -> message read started for device type: " << m_portParams->getDataType(ch) << std::endl;
						m_tempDelims = m_portParams->getDelims(ch);
						m_receiveStatus = WaitingForSTX;
					}
				}
				break;
				case WaitingForSTX:
				{
					if (ch == m_portParams->m_STX)
					{
						if (m_portParams->m_dcList.size() == 0)
						{
							std::cout << "RS232_Device::on_read() -> message read started!" << std::endl;
						}
						m_receiveStatus = WaitingForETX;
					}
				}
				break;
				case WaitingForETX:
				{
					if (ch == m_portParams->m_ETX)
					{
						if (m_DLEReceived)
						{
							if (m_firstNonPrintableCharPos == 0 && (ch < ASCII_SP || ch > ASCII_TLDE))
								m_firstNonPrintableCharPos = m_receivedMessageBuffer.size();

							m_receivedMessageBuffer.push_back(ch);
							m_DLEReceived = false;
						}
						else
						{
							if (m_portParams->m_dcList.size() > 0) //we will also wait for End Of Data (EOD)!
							{
								m_receiveStatus = WaitingForEOD;
							}
							else //end of receive!
							{
								//constructHexAndLog(ReadData, std::string(m_toBeLoggedBuffer.begin(), m_toBeLoggedBuffer.end())); //hex dump of received string
								//m_toBeLoggedBuffer.clear();

								if (m_portParams->m_CREnabled && m_receivedMessageBuffer.back() == ASCII_CR)
								{
									m_receivedMessageBuffer.pop_back();
									if (m_firstNonPrintableCharPos == m_receivedMessageBuffer.size())
									{	//the case we receive CR at the end of the data, we should not accidentally set the m_firstNonPrintableCharPos!
										m_firstNonPrintableCharPos = 0;
									}
								}

								std::string receivedMessageStr(m_receivedMessageBuffer.begin(), m_receivedMessageBuffer.end());
								
								printReceivedData(receivedMessageStr, m_firstNonPrintableCharPos);
								
								m_receiveStatus = WaitingForSTX; //NOT WaitingForSOD, because we did not set the SOD & EOD in the RS232.xml!

								m_receivedMessageBuffer.clear();
								m_DLEReceived = false;
								m_firstNonPrintableCharPos = 0;
							}
						}
					}
					else if (m_portParams->m_DLEEnabled && !m_DLEReceived && ch == ASCII_DLE)
					{	//removing ASCII_DLE (0x10) byte from the data!
						m_DLEReceived = true;
					}
					else
					{
						if (m_firstNonPrintableCharPos == 0 && (ch < ASCII_SP || ch > ASCII_TLDE))
							m_firstNonPrintableCharPos = m_receivedMessageBuffer.size();

						m_receivedMessageBuffer.push_back(ch);
						m_DLEReceived = false;
					}
				}
				break;
				case WaitingForEOD:
				{
					if (ch == m_tempEOD)
					{
						//constructHexAndLog(ReadData, std::string(m_toBeLoggedBuffer.begin(), m_toBeLoggedBuffer.end())); //hex dump of received string
						//m_toBeLoggedBuffer.clear();

						if (m_portParams->m_CREnabled && m_receivedMessageBuffer.back() == ASCII_CR)
						{
							m_receivedMessageBuffer.pop_back();
							if (m_firstNonPrintableCharPos == m_receivedMessageBuffer.size())
							{	//the case we receive CR at the end of the data, we should not accidentally set the m_firstNonPrintableCharPos!
								m_firstNonPrintableCharPos = 0;
							}
						}

						std::string receivedMessageStr(m_receivedMessageBuffer.begin(), m_receivedMessageBuffer.end());
						printReceivedData(receivedMessageStr, m_firstNonPrintableCharPos);

						m_receiveStatus = WaitingForSOD; //we have set SOD & EOD in the RS232.xml!
						
						m_receivedMessageBuffer.clear();
						m_DLEReceived = false;
						m_firstNonPrintableCharPos = 0;
					}
				}
				break;
				default:
				{	//DO NOTHING!!! IT IS AN ERROR!
					std::cout << "RS232_Device::on_read() -> read procedure is in an unexpected state!" << std::endl;
				}
				break;
				}
			}//for all received bytes
		}
		catch (...)
		{
			std::cout << "RS232_Device::on_read() -> Unknown exception occurred!!" << std::endl;
		}
	}

	void RS232_Device::on_socket_error(PortError portError)
	{
		if (m_portHandler->is_active())
			m_portHandler->close();

		std::thread(&RS232_PortHandler::waitForPortToBecomeAvailable, m_portHandler).detach();
	}

	void RS232_Device::on_serialstate_changed(RS232_PinStatus pinStatus)
	{
		//std::stringstream o_str;
		std::cout << "!!!serial status changed!!!" << std::endl;
		std::cout << "Current Modem state is >> " << std::endl;
		std::cout
			<< "Modem CTS  Pin " << (pinStatus.m_CTS_on ? "Active" : "Deactive") << std::endl
			<< "Modem DSR  Pin " << (pinStatus.m_DSR_on ? "Active" : "Deactive") << std::endl
			<< "Modem Ring Pin " << (pinStatus.m_RI_on ? "Active" : "Deactive") << std::endl
			<< "Modem RLSD Pin " << (pinStatus.m_RLSD_on ? "Active" : "Deactive") << std::endl;
	}

	std::string RS232_Device::encapsulateMessage(const std::string& message)
	{
		std::string str;
		str += m_portParams->m_STX;
		if (m_portParams->m_DLEEnabled)
		{
			for (unsigned char data : message)
			{
				if (data < ASCII_SP && data >= ASCII_NULL)
					str += ASCII_DLE;
				str += data;
			}
		}
		else
		{
			str += message;
		}
		str += m_portParams->m_ETX;
		return str;
	}

	void RS232_Device::printReceivedData(const std::string& receivedData, unsigned int firstNonPrintableCharPos)
	{
		std::cout << "[Received Data]" << std::endl;
		if (m_tempDelims.size())
		{
			std::vector<std::string> trackDataVector;
			boost::split(trackDataVector, receivedData, [=](char ch) {return m_tempDelims.find(ch) != std::string::npos; });
			trackDataVector.erase(
				std::remove_if(
					trackDataVector.begin(),
					trackDataVector.end(),
					[](std::string iter) {return iter.empty(); }),
				trackDataVector.end());

			for (auto iter : trackDataVector)
				std::cout << iter << std::endl;

			m_tempDelims.clear();

		}
		else if (firstNonPrintableCharPos > 0)
		{
			std::string msgTillNonPrintable = receivedData.substr(0, firstNonPrintableCharPos);
			std::cout << "Printable Part: " << msgTillNonPrintable << std::endl;
			std::string msgAfterNonPrintable = receivedData.substr(firstNonPrintableCharPos);
			std::string encodedBinaryMsg = Base64::Encode((const unsigned char*)msgAfterNonPrintable.c_str(), msgAfterNonPrintable.size());

			std::cout << "Non-Printable Part (Base64-Encoded): " << encodedBinaryMsg << std::endl;
		}
		else
		{
			std::cout << receivedData << std::endl;
		}
	}

}
