#pragma once
#include <iostream>
#include <string>
#include "cmdArt.h"
#include <map>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream> 

class menuFunc
{
public:

	struct ScreenSession {
		std::string processName;
		int currentLine;
		int totalLines;
		std::string timestamp;
	};

	static std::map<std::string, ScreenSession> sessions;


	static void mainMenu() {
		std::string uChoice;
		bool isRunning = true;

		cmdArt::showArt();

		while (isRunning) {
			cmdArt::showMenu();
			std::cout << "Enter Command: ";
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
					std::cout << "Screen session '" << screenName << "' started.\n";

					// Immediately enter screen
					bool inScreen = true;
					while (inScreen) {
						system("cls");  // Use "clear" on Linux/macOS
						ScreenSession& s = sessions[screenName];
						std::cout << "\n==== Screen Console Placeholder ====\n";
						std::cout << "Process Name      : " << s.processName << "\n";
						std::cout << "Instruction Line  : " << s.currentLine << " / " << s.totalLines << "\n";
						std::cout << "Created At        : " << s.timestamp << "\n";
						std::cout << "====================================\n";

						std::cout << "\nType 'exit' to return to main menu.\n> ";
						std::string screenInput;
						std::cin >> screenInput;

						if (screenInput == "exit") {
							inScreen = false;
							system("cls");
							cmdArt::showArt();
						}
						else {
							std::cout << "Unknown command in screen context. Type 'exit' to return.\n";
						}
					}
				}

				else if (screenCmd == "-r") {
					auto it = sessions.find(screenName);
					if (it != sessions.end()) {
						bool inScreen = true;
						while (inScreen) {
							// Clear and redraw screen placeholder
							system("cls");  // Use "clear" on Unix
							const ScreenSession& s = it->second;
							std::cout << "\n==== Screen Console Placeholder ====\n";
							std::cout << "Process Name      : " << s.processName << "\n";
							std::cout << "Instruction Line  : " << s.currentLine << " / " << s.totalLines << "\n";
							std::cout << "Created At        : " << s.timestamp << "\n";
							std::cout << "====================================\n";

							std::cout << "\nType 'exit' to return to main menu.\n> ";
							std::string screenInput;
							std::cin >> screenInput;

							if (screenInput == "exit") {
								inScreen = false;
								system("cls");
								cmdArt::showArt();
							}
							else {
								// You can expand this to simulate line advancement, etc.
								std::cout << "Unknown command in screen context. Type 'exit' to return.\n";
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
				system("cls");
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