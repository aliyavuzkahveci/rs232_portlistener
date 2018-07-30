/*
@author  Ali Yavuz Kahveci aliyavuzkahveci@gmail.com
* @version 1.0
* @since   09-07-2018
* @Purpose: parses RS232.ini file to get the configuration of the serial port
*/

#include <string>
#include <iostream>
#include <map>
#include <memory>

#include "RS232_Util.h"

namespace RS232
{

	using PortMap = std::map<std::string, RS232_PortParams_Ptr>;
	using PortMapPair = std::pair<std::string, RS232_PortParams_Ptr>;
	using PortMapIter = PortMap::iterator;

	class INI_Manager;
	using INI_Manager_Ptr = std::unique_ptr<INI_Manager>;

	class INI_Manager final
	{
	public:
		static INI_Manager_Ptr& getInstance();

		virtual ~INI_Manager();

		bool initFromXml(const std::string& filePath);

		RS232_PortParams_Ptr getPortParams(const std::string& comPort);

		std::vector<std::string> getComPortList();

	private:
		INI_Manager();

		/*to protect the Singleton class from being copied*/
		INI_Manager(const INI_Manager&) = delete;
		INI_Manager& operator=(const INI_Manager&) = delete;
		INI_Manager(INI_Manager&&) = delete;
		INI_Manager& operator=(INI_Manager&) = delete;
		/*to protect the Singleton class from being copied*/

		static INI_Manager_Ptr m_instance;
		PortMap m_portMap;
	};

	class TransmitDataHandler final
	{
	public:
		static std::string prepareTransmitData(const std::string& filePath);

		virtual ~TransmitDataHandler();

	private:
		/*to protect the Singleton class from being copied*/
		TransmitDataHandler(const TransmitDataHandler&) = delete;
		TransmitDataHandler& operator=(const TransmitDataHandler&) = delete;
		TransmitDataHandler(TransmitDataHandler&&) = delete;
		TransmitDataHandler& operator=(TransmitDataHandler&) = delete;
		/*to protect the static class from being copied*/
	};

}

