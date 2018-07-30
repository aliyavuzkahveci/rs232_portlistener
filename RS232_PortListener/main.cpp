// main.cpp : Defines the entry point for the console application.
//
#include <csignal>

#include "RS232_Device.h"
#include "INI_Manager.h"

constexpr auto UC_Q = 0x51;
constexpr auto LC_Q = 0x71;

bool terminationReceived = false;

void signalHandler(int sigNum)
{
	if (sigNum == SIGINT)
		std::cout << "Interactive Attention Signal received!" << std::endl;
	else if (sigNum == SIGILL)
		std::cout << "Illegal Instruction Signal received!" << std::endl;
	else if (sigNum == SIGFPE)
		std::cout << "Erroneous Arithmetic Operation Signal received!" << std::endl;
	else if (sigNum == SIGSEGV)
		std::cout << "Invalid Access to Storage Signal received!" << std::endl;
	else if (sigNum == SIGTERM)
		std::cout << "Termination Request Signal received!" << std::endl;
	else if (sigNum == SIGBREAK)
		std::cout << "Ctrl-Break Sequence Signal received!" << std::endl;
	else if (sigNum == SIGABRT)
		std::cout << "Abnormal Termination Signal received!" << std::endl;
	else if (sigNum == SIGABRT_COMPAT)
		std::cout << "Abnormal Termination Signal (compatible with other platforms) received!" << std::endl;
	else
	{
		std::cout << "Unknown signal received!" << std::endl;
		return; //does not enter to termination state!
	}

	terminationReceived = true;
}

int main(int argc, char* argv[])
{
	using namespace RS232;

	/*register termination signals to gracefully shut down*/
	std::cout << "Registering Signals to catch when occured!" << std::endl;
	signal(SIGINT, signalHandler);			// SIGINT -> Receipt of an interactive attention signal.
	signal(SIGILL, signalHandler);			// SIGILL -> Detection of an illegal instruction.
	signal(SIGFPE, signalHandler);			// SIGFPE -> An erroneous arithmetic operation, such as a divide by zero or an operation resulting in overflow.
	signal(SIGSEGV, signalHandler);			// SIGSEGV -> An invalid access to storage.
	signal(SIGTERM, signalHandler);			//SIGTERM -> A termination request sent to the program.
	signal(SIGBREAK, signalHandler);		// SIGBREAK -> Ctrl-Break sequence
	signal(SIGABRT, signalHandler);			// SIGABRT -> Abnormal termination of the program, such as a call to abort.
	signal(SIGABRT_COMPAT, signalHandler);	// SIGABRT_COMPAT -> SIGABRT compatible with other platforms, same as SIGABRT
	/*register termination signals to gracefully shut down*/

	std::vector<std::string> portList;
	if (argc != 2)
	{
		std::cout << "Wrong input format!" << std::endl;
		std::cout << "Correct format is:" << std::endl;
		std::cout << "RS232_PortListener.exe ~iniFilePath~" << std::endl;
	}
	else if(!INI_Manager::getInstance()->initFromXml(std::string(argv[1])))
	{
		std::cout << "Error while loadig RS232 ports from XML file!" << std::endl;
	}
	else if((portList = INI_Manager::getInstance()->getComPortList()).empty())
	{
		std::cout << "No COM port found from the XML file!" << std::endl;
	}
	else
	{
		std::string selectedPort;
		std::cout << "Please select one of the following COM ports:" << std::endl;
		for (auto iter : portList)
			std::cout << iter << std::endl;
		std::cout << "Selection: " << std::endl;
		std::cin >> selectedPort;

		RS232_Device_Ptr device = RS232_Device_Ptr(new RS232_Device(INI_Manager::getInstance()->getPortParams(selectedPort)));

		device->openDevice();

		std::string received;
		std::cout
			<< "Please write \"Q\" to quit application.." << std::endl
			<< "enter file path containing the data to be sent to the device..." << std::endl << "File Path: ";

		while (!terminationReceived && std::cin >> received)
		{
			if (terminationReceived)
				break;
			if (received.length() == 1 && (received.at(0) == UC_Q || received.at(0) == LC_Q))
				break;

			//received string is a file path containing the data to be sent to the device!!!
			std::string dataToBeSent = TransmitDataHandler::prepareTransmitData(received);
			if (dataToBeSent.size())
				device->sendMessageToDevice(dataToBeSent);

			std::cout
				<< "Please write \"Q\" to quit application.." << std::endl
				<< "enter file path containing the data to be sent to the device..." << std::endl << "File Path: ";
		}

		device->closeDevice();
		device.reset();
	}

    return 0;
}

