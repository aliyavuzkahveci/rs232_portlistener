/*
@author  Ali Yavuz Kahveci aliyavuzkahveci@gmail.com
* @version 1.0
* @since   09-07-2018
* @Purpose: manages the RS232 port (status polling, reading, and writing)
*/

#include <mutex>

#include "RS232_Util.h"

namespace RS232
{
	class RS232_PortSubscriber;
	using RS232_PortSubscriber_Ptr = std::shared_ptr<RS232_PortSubscriber>;

	class RS232_PortHandler
	{
	public:
		RS232_PortHandler(RS232_PortSubscriber_Ptr, RS232_PortParams_Ptr);
		virtual ~RS232_PortHandler();

		void write(const unsigned char*, unsigned int);
		void close();

		bool is_active() const { return m_bOpenSuccess; }

		void init();

		//starts running when the port becomes unavailable until it is available again!
		void waitForPortToBecomeAvailable();

	private:
		void openPortHandler();

		/* Serial Port Read Thread */
		static DWORD WINAPI startReadThread(LPVOID lpV);
		DWORD read();

		/* Get and Display the COM port status (RLSD & RING & DSR & CTS) */
		DWORD updatePinStatus();

		/*returns the available COM ports from the Windows*/
		std::vector<std::string> getComPortNames();

		RS232_PortSubscriber_Ptr m_subscriber;
		RS232_PortParams_Ptr m_portParams;
		RS232_PinStatus m_pinStatus;

		/*serial port handles*/
		HANDLE	m_HSerialPort = NULL; //handle for serial port
		HANDLE	m_HReadDone = NULL; //handle for serial port read finish notification
		HANDLE	m_HReadThread = NULL; //handle for serial port read thread

		/*flags to hold the status of port handling*/
		bool m_bOpenSuccess;
		bool m_ReadTerminated;
		bool m_PortHandlerClosed;

		/*to protect writing/reading processes from multiple access*/
		std::mutex m_writeGuard;
		std::mutex m_readGuard;

		/*to buffer the received data from the serial port*/
		int m_BufferSize;
		char* m_ReadData;
		LPSTR m_LPReadData;

		/*to protect the class from being copied*/
		RS232_PortHandler(const RS232_PortHandler&) = delete;
		RS232_PortHandler& operator=(const RS232_PortHandler&) = delete;
		RS232_PortHandler(RS232_PortHandler&&) = delete;
		RS232_PortHandler& operator=(RS232_PortHandler&) = delete;
		/*to protect the class from being copied*/

	};
	using RS232_PortHandler_Ptr = std::shared_ptr<RS232_PortHandler>;

	class RS232_PortSubscriber
	{
		friend class RS232_PortHandler;
	public:
		RS232_PortSubscriber() {};

		virtual ~RS232_PortSubscriber() {};

	protected:
		//will be called by the RS232_PortHandler object upon receiving data
		virtual void on_read(const unsigned char *readData, unsigned int dataLength) = 0;

		//will be called when there happens an error in the serial port
		virtual void on_socket_error(PortError portError) = 0;

		//will be called when the serial port pin status changes
		virtual void on_serialstate_changed(RS232_PinStatus pinStatus) = 0;

		virtual void writeToPort(const unsigned char* data, unsigned int length) final
		{
			std::lock_guard<std::mutex> lock(m_guard);
			if (m_portHandler.get())
			{
				int numOfSeparateWrites = length / m_bufferSize;
				int leftOver = length % m_bufferSize;
				for (int i = 0; i < numOfSeparateWrites; i++)
				{
					m_portHandler->write((data + (i*m_bufferSize)), m_bufferSize);
				}
				m_portHandler->write((data + (numOfSeparateWrites*m_bufferSize)), leftOver);
			}
		}

		virtual void writeToPort(const std::string& str) final
		{
			this->writeToPort((const unsigned char *)str.c_str(), str.size());
		}

		RS232_PortHandler_Ptr m_portHandler;
		std::mutex m_guard;
		unsigned int m_bufferSize;

	};

}

