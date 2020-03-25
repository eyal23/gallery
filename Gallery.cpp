#include <iostream>
#include <string>
#include <ctime>

#include "DatabaseAccess.h"
#include "AlbumManager.h"


int getCommandNumberFromUser()
{
	std::string message("\nPlease enter any command(use number): ");
	std::string numericStr("0123456789");
	
	std::cout << message << std::endl;
	std::string input;
	std::getline(std::cin, input);
	
	while (std::cin.fail() || std::cin.eof() || input.find_first_not_of(numericStr) != std::string::npos) {

		std::cout << "Please enter a number only!" << std::endl;

		if (input.find_first_not_of(numericStr) == std::string::npos) {
			std::cin.clear();
		}

		std::cout << std::endl << message << std::endl;
		std::getline(std::cin, input);
	}
	
	return std::atoi(input.c_str());
}

void printSystemStatistics()
{
	char date[256];
	time_t ttime = time(0);
	strftime(date, sizeof(date), "time: %b/%d/%Y\nhour: %H", localtime(&ttime));
	std::cout << "Developer: Eyal Heinrich" << std::endl << date << std::endl << std::endl;
}

int main(void)
{
	// initialization data access
	DatabaseAccess dataAccess;

	try
	{
		// initialize album manager
		AlbumManager albumManager(dataAccess);
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
		return -1;
	}

	printSystemStatistics();

	std::string albumName;
	std::cout << "Welcome to Gallery!" << std::endl;
	std::cout << "===================" << std::endl;
	std::cout << "Type " << HELP << " to a list of all supported commands" << std::endl;
	
	do {
		int commandNumber = getCommandNumberFromUser();
		
		try	{
			albumManager.executeCommand(static_cast<CommandType>(commandNumber));
		} catch (std::exception& e) {	
			std::cout << e.what() << std::endl;
		}
	} 
	while (true);
}


