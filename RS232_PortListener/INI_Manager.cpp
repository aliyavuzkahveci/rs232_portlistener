#include "INI_Manager.h"
#include "Base64.h"

#include <iostream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/algorithm/string/predicate.hpp>

namespace RS232
{
	INI_Manager_Ptr INI_Manager::m_instance = nullptr;

	INI_Manager_Ptr& INI_Manager::getInstance()
	{
		if (m_instance == nullptr)
			m_instance = std::unique_ptr<INI_Manager>(new INI_Manager());
		return m_instance;
	}

	INI_Manager::INI_Manager()
	{

	}

	INI_Manager::~INI_Manager()
	{
		m_portMap.clear();
	}

	bool INI_Manager::initFromXml(const std::string& filePath)
	{
		try
		{
			boost::property_tree::ptree pt;
			boost::property_tree::read_xml(filePath, pt);

			for (auto const& v : pt.get_child(ROOT_ELEMENT))
			{
				if (v.first == PORT_NODE) //RS232Port
				{
					RS232_PortParams* portParam = nullptr;
					std::string comPort = v.second.get<std::string>(PORT_NAME_ATTR);
					for (boost::property_tree::ptree::value_type const& p : v.second.get_child(""))
					{
						if (p.first == DETAILS_NODE) //portDetails
						{
							BaudRate baudRate = (BaudRate)p.second.get<unsigned int>(BAUD_ATTR);
							CharSize charSize = (CharSize)p.second.get<unsigned int>(CHAR_SIZE_ATTR);
							Parity parity = (Parity)p.second.get<char>(PARITY_ATTR);
							StopBits stopBits = (StopBits)p.second.get<unsigned int>(STOP_ATTR);
							FlowControl flow = (FlowControl)p.second.get<char>(FLOW_ATTR);

							portParam = new RS232_PortParams(comPort, baudRate, charSize, parity, stopBits, flow);
						}
						else if (p.first == PROTOCOL_NODE && portParam != nullptr)
						{
							std::istringstream i_str;
							int intVal = -1;
							i_str.str(p.second.get<std::string>(STX_ATTR));
							i_str >> std::hex >> intVal;
							portParam->m_STX = intVal;

							i_str.clear();
							i_str.str(p.second.get<std::string>(ETX_ATTR));
							i_str >> std::hex >> intVal;
							portParam->m_ETX = intVal;

							boost::optional<std::string> dleStr = p.second.get_optional<std::string>(DLE_ATTR);
							if (dleStr.is_initialized())
								portParam->m_DLEEnabled = boost::iequals(dleStr.value(), TRUE_STR);

							boost::optional<std::string> crStr = p.second.get_optional<std::string>(CR_ATTR);
							if (crStr.is_initialized())
								portParam->m_CREnabled = boost::iequals(crStr.value(), TRUE_STR);

							//RS232_PortParams portParam(comPort, baudRate, charSize, parity, stopBits, flow, stx, etx, dle, cr);

							boost::optional<unsigned int> updateTime = p.second.get_optional<unsigned int>(UPDATE_TIME_ATTR);
							if (updateTime.is_initialized())
								portParam->m_statusUpdateTime = updateTime.value();

							boost::optional<unsigned int> rxBufferSize = p.second.get_optional<unsigned int>(RX_SIZE_ATTR);
							if (rxBufferSize.is_initialized())
								portParam->m_rxBufferSize = rxBufferSize.value();

							boost::optional<unsigned int> txBufferSize = p.second.get_optional<unsigned int>(TX_SIZE_ATTR);
							if (txBufferSize.is_initialized())
								portParam->m_txBufferSize = txBufferSize.value();

							for (auto const& r : p.second.get_child(""))
							{
								if (r.first == CONTROL_NODE) //dataControl
								{
									unsigned char sod, eod;

									i_str.clear();
									i_str.str(r.second.get<std::string>(SOD_ATTR));
									i_str >> std::hex >> intVal;
									sod = intVal;

									i_str.clear();
									i_str.str(r.second.get<std::string>(EOD_ATTR));
									i_str >> std::hex >> intVal;
									eod = intVal;

									std::string typeName = r.second.get<std::string>(TYPE_ATTR);

									std::string delims;
									for (auto const& s : r.second.get_child(""))
									{
										if (s.first == DELIM_NODE) //delimeter
										{
											i_str.clear();
											i_str.str(s.second.get_value<std::string>());
											i_str >> std::hex >> intVal;
											delims.push_back(intVal);
										}
									}
									if (delims.size())
										portParam->addDataControl(DataControl(typeName, sod, eod, delims));
									else
										portParam->addDataControl(DataControl(typeName, sod, eod));
								}
							}
							m_portMap.insert(PortMapPair(comPort, RS232_PortParams_Ptr(portParam)));
						}
					}
				}
			}
		}
		catch (boost::property_tree::ptree_error &e)
		{
			std::cout << "ERROR while reading RS232.xml file!" << std::endl;
			std::cout << "INI_Manager::initFromXml() -> ptree_error: " << e.what() << std::endl;
			return false;
		}
		catch (std::exception e) {
			std::cout << "ERROR while reading RS232.xml file!" << std::endl;
			std::cout << "INI_Manager::initFromXml() -> std::exception: " << e.what() << std::endl;
			return false;
		}
		return true;
	}

	RS232_PortParams_Ptr INI_Manager::getPortParams(const std::string& comPort)
	{
		PortMapIter iter = m_portMap.find(comPort);
		if (iter != m_portMap.end())
			return iter->second;
		else
			return RS232_PortParams_Ptr();
	}

	std::vector<std::string> INI_Manager::getComPortList()
	{
		std::vector<std::string> portList;
		for (auto iter : m_portMap)
			portList.push_back(iter.first);
		return portList;
	}

	std::string TransmitDataHandler::prepareTransmitData(const std::string& filePath)
	{
		try
		{
			boost::property_tree::ptree pt;
			boost::property_tree::read_ini(filePath, pt);
			for (auto& section : pt)
			{
				if (section.first == TRANSMIT_SECTION) //TransmitData
				{
					std::string transmitData("");
					std::string binaryData("");
					for (auto& property : section.second)
					{
						if (property.first == TEXT_SECTION) //TextData
						{
							boost::optional<std::string> propertyValue = property.second.get_value_optional<std::string>();
							if (propertyValue.is_initialized())
								transmitData = propertyValue.value();
						}
						else if (property.first == BINARY_SECTION) //BinaryData
						{
							boost::optional<std::string> propertyValue = property.second.get_value_optional<std::string>();
							if (propertyValue.is_initialized())
							{
								binaryData = Base64::Decode(propertyValue.value());
								transmitData += binaryData;
							}
						}
					}
					return transmitData;
				}
			}
		}
		catch (boost::property_tree::ptree_error &e)
		{
			std::cout << "ERROR while reading " << filePath << "file!" << std::endl;
			std::cout << "::init() -> ptree_error: " << e.what() << std::endl;
			return "";
		}
		catch (std::exception e)
		{
			std::cout << "ERROR while reading " << filePath << "file!" << std::endl;
			std::cout << "::init() -> std::exception: " << e.what() << std::endl;
			return "";
		}
	}
}
