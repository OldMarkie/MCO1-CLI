#pragma once
#include <iostream>
#include <string>
#include <unordered_map>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream> 

#include "ScreenSession.h"
#include "cmdArt.h"
#include "ScreenSessionManager.h"
#include "Console.h"

class menuFunc
{
public:

	static std::unordered_map<std::string, ScreenSession> sessions;


	static void mainMenu() {
		ScreenSessionManager screenMng(sessions);
		std::string uChoice;
		bool isRunning = true;
		Console::showArt();
		while (isRunning) {
			Console::drawMainMenu();
			std::cin >> uChoice;

			if (uChoice == "initialize") {
				std::cout << "initialize command recognized. Doing something. \n\n";
			}
			else if (uChoice == "screen") {
				std::string screenCmd, screenName;
				std::cin >> screenCmd >> screenName;

				if (screenCmd == "-s") {
					sessions[screenName] = createNewScreenSession(screenName);
					cmdArt::displayNewSesh(screenName);
					screenMng.enterScreen(screenName);
				}
				else if (screenCmd == "-r") {
					if (sessions.find(screenName) != sessions.end()) {
						screenMng.enterScreen(screenName);
					}
					else {
						std::cout << "No session named '" << screenName << "' found.\n\n";
					}
				}
				else {
					std::cout << "Invalid screen command. Use 'screen -s <name>' or 'screen -r <name>'.\n\n";
				}
			}
			else if (uChoice == "scheduler-test"){
				std::cout << "scheduler-test command recognized. Doing something. \n\n";
			}
			else if (uChoice == "scheduler-stop"){
				std::cout << "scheduler-stop command recognized. Doing something. \n\n";
			}
			else if (uChoice == "report-util"){
				std::cout << "report-util command recognized.Doing something.\n\n";
			}
			else if (uChoice == "clear"){
				Console::clear();
				Console::showArt();
			}
			else if (uChoice == "exit"){
				isRunning = false;
			}
			else {
				Console::showUnknownCommand();
			}
		}
		
	}
};