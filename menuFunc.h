#pragma once
#include <iostream>
#include <string>
#include "cmdArt.h"

class menuFunc
{
public:
	static void mainMenu() {
		std::string uChoice;
		bool isRunning = true;

		cmdArt::showArt();

		while (isRunning) {
			cmdArt::showMenu();
			std::cout << "Enter Command: ";
			std::cin >> uChoice;

			if (uChoice == "initialize"){
				std::cout << "Initializing...\n\n";
			}
			else if (uChoice == "screen"){
				std::cout << "Showing Screen...\n\n";
			}
			else if (uChoice == "scheduler-test"){
				std::cout << "Generating Dummies...\n\n";
			}
			else if (uChoice == "scheduler-stop"){
				std::cout << "Stopping Generating Dummies...\n\n";
			}
			else if (uChoice == "report-util"){
				std::cout << "Showing CPU Utilization...\n\n";
			}
			else if (uChoice == "clear"){
				std::cout << "Clearing...\n\n";
				system("cls");
				cmdArt::showArt();
			}
			else if (uChoice == "exit"){
				std::cout << "Exiting...\n\n";
				isRunning = false;
			}
			else {
				std::cout << "Command Not Found\n\n";
			}
		}
		
	}
};