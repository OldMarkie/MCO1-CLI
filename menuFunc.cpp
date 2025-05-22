#pragma once
#include <iostream>
#include <string>
#include <map>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream> 

#include "ScreenSession.h"
#include "cmdArt.h"


class menuFunc
{
public:

	static std::map<std::string, ScreenSession> sessions;


	static void mainMenu() {
		std::string uChoice;
		bool isRunning = true;

		cmdArt::showArt();

		while (isRunning) {
			cmdArt::showMenu();
			std::cin >> uChoice;

			if (uChoice == "initialize"){
				std::cout << "initialize command recognized. Doing something. \n\n";
			}
			else if (uChoice == "screen") {
				std::string screenCmd, screenName;
				std::cin >> screenCmd >> screenName;

				if (screenCmd == "-s") {
					// Create a new screen session
					auto now = std::chrono::system_clock::now();
					std::time_t timeNow = std::chrono::system_clock::to_time_t(now);
					std::tm localTime;
#ifdef _WIN32
					localtime_s(&localTime, &timeNow);
#else
					localtime_r(&timeNow, &localTime);
#endif
					std::ostringstream timeStream;
					timeStream << std::put_time(&localTime, "%m/%d/%Y, %I:%M:%S %p");

					ScreenSession newSession{
						screenName,
						1, // current line
						10, // total lines
						timeStream.str()
					};

					sessions[screenName] = newSession;
					cmdArt::displayNewSesh(screenName);
				}

				else if (screenCmd == "-r") {
					auto it = sessions.find(screenName);
					if (it != sessions.end()) {
						bool inScreen = true;
						cmdArt::visualClear();  // Use "clear" on Unix
						const ScreenSession& s = it->second;
						cmdArt::screenMenu(screenName, s);
						while (inScreen) {
							std::string screenInput;
							std::cin >> screenInput;

							if (screenInput == "exit") {
								inScreen = false;
								cmdArt::visualClear();
								cmdArt::showArt();
							}
							else {
								cmdArt::visualClear();
								std::cout << "\033[1;31mUnknown command in screen context. Type 'exit' to return.\033[0m\n";
								cmdArt::screenMenu(screenName, s);
							}
						}
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
				cmdArt::visualClear();
				cmdArt::showArt();
			}
			else if (uChoice == "exit"){
				isRunning = false;
			}
			else {
				std::cout << "Command Not Found\n\n";
			}
		}
		
	}
};