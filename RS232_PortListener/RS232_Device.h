/*
@author  Ali Yavuz Kahveci aliyavuzkahveci@gmail.com
* @version 1.0
* @since   09-07-2018
* @Purpose: manages the RS232 Serial Device read/write/status update
*/

#include "RS232_PortHandler.h"

namespace RS232
{
	class RS232_Device : public RS232_PortSubscriber, public std::enable_shared_from_this<RS232_Device>
	{
	public:
		RS232_Device(RS232_PortParams_Ptr);
		virtual ~RS232_Device();

		void sendMessageToDevice(const unsigned char *data, unsigned int len);

		void sendMessageToDevice(const std::string& msg);

		void openDevice();
		void closeDevice();

	private:
		/*inherited from RS232_PortSubscriber*/
		void on_read(const unsigned char *readData, unsigned int dataLength) override;

		void on_socket_error(PortError portError) override;

		void on_serialstate_changed(RS232_PinStatus pinStatus) override;

		std::string encapsulateMessage(const std::string& message);

		void printReceivedData(const std::string& receivedData, unsigned int firstNonPrintableCharPos);

		RS232_PortParams_Ptr m_portParams;

		/*to protect writing/reading processes from multiple access*/
		std::mutex m_writeGuard;
		std::mutex m_readGuard;

		ReceiveStatus m_receiveStatus;
		std::vector<char> m_receivedMessageBuffer;
		bool m_DLEReceived;
		unsigned int m_firstNonPrintableCharPos;

		char m_tempEOD;
		std::string m_tempDelims;

		RS232_Device(const RS232_Device&) = delete;

	};
	using RS232_Device_Ptr = std::shared_ptr<RS232_Device>;
}

