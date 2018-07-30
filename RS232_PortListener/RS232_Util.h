#pragma once
/*
@author  Ali Yavuz Kahveci aliyavuzkahveci@gmail.com
* @version 1.0
* @since   09-07-2018
* @Purpose: implements utility structs and constant expressions
*/

#include <string>
#include <vector>
#include <iostream>
#include <memory>

#include <Windows.h>

namespace RS232
{
#define ASCII_NULL	0x00	//Null char
#define ASCII_ETX	0x03	//End of Text
#define ASCII_CR	0x0D	//Carriage Return
#define ASCII_SI	0x0F	//Shift In / X-Off
#define ASCII_DLE	0x10	//Data Line Escape
#define ASCII_DC1	0x11	//Device Control 1 (oft. XON)
#define ASCII_DC3	0x13	//Device Control 3 (oft. XOFF)
#define ASCII_SP	0x20	//Space
#define ASCII_PLS	0x2B	//Plus +
#define ASCII_SLH	0x2F	//Slash | Divide /
#define ASCII_0		0x30	//Zero
#define ASCII_LESS	0x3C	//Less than <
#define ASCII_EQ	0x3D	//Equal =
#define ASCII_QSTN	0x3F	//Question mark ?
#define ASCII_TLDE	0x7E	//Tilde | Equivalency sign ~
#define ASCII_LCAG	0xC0	//Latin capital letter A with grave
#define ASCII_LSET	0xF0	//Latin small letter eth
#define ASCII_LSUD	0xFC	//Latin small letter u with diaeresis

constexpr auto COM_PORT_PREPEND = "\\\\.\\";
#define DEFAULT_BUFFER_SIZE 16384;
#define DEFAULT_STATUS_TIMEOUT 100

#define ROOT_ELEMENT "RS232PortList"
#define PORT_NODE "RS232Port"
#define PORT_NAME_ATTR "<xmlattr>.portName"

#define DETAILS_NODE "portDetails"
#define BAUD_ATTR "<xmlattr>.baudRate"
#define CHAR_SIZE_ATTR "<xmlattr>.charSize"
#define PARITY_ATTR "<xmlattr>.parity"
#define STOP_ATTR "<xmlattr>.stopBits"
#define FLOW_ATTR "<xmlattr>.flowControl"

#define PROTOCOL_NODE "portProtocol"
#define STX_ATTR "<xmlattr>.stx"
#define ETX_ATTR "<xmlattr>.etx"
#define DLE_ATTR "<xmlattr>.dle"
#define CR_ATTR "<xmlattr>.cr"
#define UPDATE_TIME_ATTR "<xmlattr>.statusUpdateTime"
#define RX_SIZE_ATTR "<xmlattr>.rxBufferSize"
#define TX_SIZE_ATTR "<xmlattr>.txBufferSize"

#define CONTROL_NODE "dataControl"
#define SOD_ATTR "<xmlattr>.sod"
#define EOD_ATTR "<xmlattr>.eod"
#define TYPE_ATTR "<xmlattr>.typeName"

#define DELIM_NODE "delimeter"

#define TRUE_STR "true"
#define FALSE_STR "false"

#define TRANSMIT_SECTION "TransmitData"
#define TEXT_SECTION "TextData"
#define BINARY_SECTION "BinaryData"


	enum ReceiveStatus
	{
		WaitingForSOD,
		WaitingForSTX,
		WaitingForETX,
		WaitingForEOD
	};

	enum BaudRate
	{
		BR_50 = 50,
		BR_75 = 75,
		BR_110 = 110,
		BR_134 = 134,
		BR_150 = 150,
		BR_200 = 200,
		BR_300 = 300,
		BR_600 = 600,
		BR_1200 = 1200,
		BR_1800 = 1800,
		BR_2400 = 2400,
		BR_4800 = 4800,
		BR_9600 = 9600,
		BR_19200 = 19200,
		BR_38400 = 38400,
		BR_57600 = 57600,
		BR_115200 = 115200,
		BR_230400 = 230400,
		BR_460800 = 460800,
		BR_UNKNOWN = 0
	};

	enum CharSize
	{
		CS_5 = 5,
		CS_6 = 6,
		CS_7 = 7,
		CS_8 = 8,
		CS_UNKNOWN = 0
	};

	enum Parity
	{
		EVEN = 'E',
		ODD = 'O',
		NONE = 'N',
		UNKNOWN = 'U'
	};

	enum StopBits
	{
		SB_1 = 1,
		SB_1_5 = 11,
		SB_2 = 2,
		SB_UNKNOWN = 'U'
	};

	enum FlowControl
	{
		FC_HARD = 'H',
		FC_SOFT = 'S',
		FC_NONE = 'N',
		FC_UNKNOWN = 'U'
	};

	enum PortError
	{
		PE_PortIsAlreadyOpen,
		PE_PortIsNotOpen,
		PE_InvalidPortParameter,
		PE_CannotOpenPort,
		PE_CannotGetPortSettings,
		PE_CannotAdjustPortSettings,
		PE_ReadError,
		PE_WriteError,
		PE_BufferOverrunError
	};
	
	struct DataControl
	{
		DataControl(const std::string& typeName, char sod, char eod) :
			m_typeName(typeName), m_SOD(sod), m_EOD(eod), m_delims("")
		{}

		DataControl(const std::string& typeName, char sod, char eod, std::string delims) :
			m_typeName(typeName), m_SOD(sod), m_EOD(eod), m_delims(delims)
		{}

		std::string m_typeName;
		char m_SOD; //Start of Data
		char m_EOD; //End of Data
		std::string m_delims; //data delimeter

		bool operator==(const DataControl& rhs) const
		{
			if (m_typeName.compare(rhs.m_typeName) != 0)
				return false;
			else if (m_SOD != rhs.m_SOD)
				return false;
			else if (m_EOD != rhs.m_EOD)
				return false;
			else
				return true;
		}

	};
	using DataControl_Ptr = std::shared_ptr<DataControl>;

	struct RS232_PortParams
	{
		RS232_PortParams(const std::string& comPort) :
			m_comPort(comPort), m_baudRate(BaudRate::BR_9600), m_charSize(CharSize::CS_8), m_parity(Parity::NONE), m_stopBits(StopBits::SB_1), m_flowControl(FlowControl::FC_NONE),
			m_STX(0x02), m_ETX(0x03), m_DLEEnabled(false), m_CREnabled(false)
		{}

		RS232_PortParams(const std::string& comPort, BaudRate baudRate, CharSize charSize, Parity parity, StopBits stopBits, FlowControl flowControl) :
			m_comPort(comPort), m_baudRate(baudRate), m_charSize(charSize), m_parity(parity), m_stopBits(stopBits), m_flowControl(flowControl),
			m_STX(0x02), m_ETX(0x03), m_DLEEnabled(false), m_CREnabled(false)
		{}

		RS232_PortParams(const std::string& comPort, BaudRate baudRate, CharSize charSize, Parity parity, StopBits stopBits, FlowControl flowControl,
			char stx, char etx, bool dleEnabled = false, bool crEnabled = false) :
			m_comPort(comPort), m_baudRate(baudRate), m_charSize(charSize), m_parity(parity), m_stopBits(stopBits), m_flowControl(flowControl),
			m_STX(stx), m_ETX(etx), m_DLEEnabled(dleEnabled), m_CREnabled(crEnabled)
		{}

		std::string m_comPort;
		BaudRate m_baudRate;
		CharSize m_charSize;
		Parity m_parity;
		StopBits m_stopBits;
		FlowControl m_flowControl;

		char m_STX; //Start of Text
		char m_ETX; //End of Text
		bool m_DLEEnabled; //Data Link Escape enabled
		bool m_CREnabled; //Carriage Return enabled

		unsigned int m_statusUpdateTime = DEFAULT_STATUS_TIMEOUT;
		unsigned int m_rxBufferSize = DEFAULT_BUFFER_SIZE;
		unsigned int m_txBufferSize = DEFAULT_BUFFER_SIZE;

		std::vector<DataControl_Ptr> m_dcList;

		void addDataControl(DataControl dc)
		{
			for (auto iter : m_dcList)
				if (*iter == dc)
					return;
			m_dcList.push_back(std::make_shared<DataControl>(dc));
		}

		char getEOD(char sod)
		{
			for (auto iter : m_dcList)
				if (iter->m_SOD == sod)
					return iter->m_EOD;
			return ASCII_NULL; //return NULL char in case no SOD is listed in dataControl list!
		}

		std::string getDataType(char sod)
		{
			for (auto iter : m_dcList)
				if (iter->m_SOD == sod)
					return iter->m_typeName;
			return ""; //return EMPTY string in case no SOD is listed in dataControl list!
		}

		std::string getDelims(char sod)
		{
			for (auto iter : m_dcList)
				if (iter->m_SOD == sod)
					return iter->m_delims;
			return ""; //return EMPTY string in case no SOD is listed in dataControl list!
		}

	};
	using RS232_PortParams_Ptr = std::shared_ptr<RS232_PortParams>;

	struct RS232_PinStatus
	{
		RS232_PinStatus(bool cts_on = false, bool dsr_on = false, bool ri_on = false, bool rlsd_on = false) :
			m_CTS_on(cts_on), m_DSR_on(dsr_on), m_RI_on(ri_on), m_RLSD_on(rlsd_on)
		{}

		/*!!!Information!!!
		*DTE -> Data Terminal Equipment [ex: computer terminal]
		*DCE -> Data Circuit-terminating Equipment (Data Communication Equipment) [ex: modem]
		*/
		bool m_CTS_on; //Clear To Send [DCE is ready to accept data from DTE]
		bool m_DSR_on; //Data Set Ready [DCE is ready to receive and send data]
		bool m_RI_on; //Ring Indicator [DCE has detected an incoming ring signal on the telephone line]
		bool m_RLSD_on; //Receive Line Signal Detect (DCD {Data Carrier Detect}) [DCE is receiving a carrier from a remote DCE]

		bool operator==(const RS232_PinStatus& rhs) const
		{
			if (m_CTS_on != rhs.m_CTS_on)
				return false;
			else if (m_DSR_on != rhs.m_DSR_on)
				return false;
			else if (m_RI_on != rhs.m_RI_on)
				return false;
			else if (m_RLSD_on != rhs.m_RLSD_on)
				return false;
			else
				return true;
		}

		bool operator!=(const RS232_PinStatus& rhs) const
		{
			return !operator==(rhs);
		}
	};

	inline DWORD ConvertBaudRate(BaudRate baudRate)
	{
		switch (baudRate)
		{
		case BR_50:
			return 50;
		case BR_75:
			return 75;
		case BR_110:
			return 110;
		case BR_134:
			return 134;
		case BR_150:
			return 150;
		case BR_200:
			return 200;
		case BR_300:
			return 300;
		case BR_600:
			return 600;
		case BR_1200:
			return 1200;
		case BR_1800:
			return 1800;
		case BR_2400:
			return 2400;
		case BR_4800:
			return 4800;
		case BR_9600:
			return 9600;
		case BR_19200:
			return 19200;
		case BR_38400:
			return 38400;
		case BR_57600:
			return 57600;
		case BR_115200:
			return 115200;
		case BR_230400:
			return 230400;
		case BR_460800:
			return 460800;
		default:
			return 9600;
		}
	}

	inline BYTE ConvertCharSize(CharSize charSize)
	{
		switch (charSize)
		{
		case CS_5:
			return DATABITS_5;
		case CS_6:
			return DATABITS_6;
		case CS_7:
			return DATABITS_7;
		case CS_8:
		default:
			return DATABITS_8;
		}
	}

	inline BYTE ConvertParity(Parity parity)
	{
		switch (parity)
		{
		case EVEN:
			return EVENPARITY;
		case ODD:
			return ODDPARITY;
		case NONE:
		default:
			return NOPARITY;
		}
	}

	inline BYTE ConvertStopBits(StopBits stopBits)
	{
		switch (stopBits)
		{
		case SB_1_5:
			return ONE5STOPBITS;
		case SB_2:
			return TWOSTOPBITS;
		case SB_1:
		default:
			return ONESTOPBIT;
		}
	}

	inline BOOL ConvertFlowControl(FlowControl flowControl)
	{
		switch (flowControl)
		{
		case FC_HARD:
		case FC_SOFT:
			return TRUE;
		case FC_NONE:
		default:
			return FALSE;
		}
	}
}