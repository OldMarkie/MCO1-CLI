#include "Console.h"
#include "cmdArt.h"
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

std::unordered_map<std::string, ScreenSession> sessions;

void Console::drawMainMenu() {
    std::string uChoice;
    bool isRunning = true;
    bool isInitialized = false;
    Console::showArt();
    cmdArt::showMenu();

    while (isRunning) {
        std::cout << "Enter command: ";
        std::cin >> uChoice;

        if (uChoice == "initialize") {
            if (isInitialized) {
                std::cout << "\033[1;33mSystem is already initialized.\n\n";
            }
            else {
                isInitialized = true;
                Console::clear();
                Console::showArt();
                cmdArt::showMenu();
                std::cout << "System initialized. You may now access other commands.\n\n";
            }
        }
        else if (uChoice == "clear") {
            Console::clear();
            Console::showArt();
            cmdArt::showMenu();
        }
        else if (uChoice == "exit") {
            isRunning = false;
        }
        else if (!isInitialized) {
            std::cout << "\033[1;33mPlease run 'initialize' before using other commands.\033[0m\n\n";
        }
        else if (uChoice.rfind("screen", 0) == 0) {
            std::string fullCommand;
            std::getline(std::cin, fullCommand);

            std::istringstream iss("screen " + fullCommand);
            std::string cmd, flag, screenName;
            iss >> cmd >> flag >> screenName;

            if (flag == "-s" && !screenName.empty()) {
                sessions[screenName] = createNewScreenSession(screenName);
                cmdArt::displayNewSesh(screenName);
                Console::drawScreenSession(screenName);
            }
            else if (flag == "-r" && !screenName.empty()) {
                if (sessions.find(screenName) != sessions.end()) {
                    Console::drawScreenSession(screenName);
                }
                else {
                    std::cout << "No session named '" << screenName << "' found.\n\n";
                }
            }
            else if (flag == "-ls" && screenName.empty()) {
                if (sessions.empty()) {
                    std::cout << "No active screen sessions.\n\n";
                }
                else {
                    std::cout << "Active screen sessions:\n";
                    for (const auto& pair : sessions) {
                        std::cout << "- " << pair.first << "\n";
                    }
                    std::cout << "\n";
                }
            }
            else {
                std::cout << "Invalid screen command. Use 'screen -s <name>', 'screen -r <name>', or 'screen -ls'.\n\n";
            }
        }
        else if (uChoice == "scheduler-test") {
            std::cout << "scheduler-test command recognized. Doing something.\n\n";
        }
        else if (uChoice == "scheduler-stop") {
            std::cout << "scheduler-stop command recognized. Doing something.\n\n";
        }
        else if (uChoice == "report-util") {
            std::cout << "report-util command recognized. Doing something.\n\n";
        }
        else {
            Console::showUnknownCommand();
        }
    }
}

void Console::drawScreenSession(const std::string& screenName) {
    auto it = sessions.find(screenName);
    if (it == sessions.end()) {
        std::cerr << "Screen not found.\n";
        return;
    }

    bool inScreen = true;
    const ScreenSession& session = it->second;

    cmdArt::screenMenu(screenName, session);

    while (inScreen) {
        std::string screenInput;
        std::cin >> screenInput;

        if (screenInput == "exit") {
            inScreen = false;
            Console::clear();
            Console::showArt();
        }
        else {
            Console::clear();
            Console::showUnknownCommand();
            cmdArt::screenMenu(screenName, session);
        }
    }
}   

void Console::showUnknownCommand() {
    std::cout << "\033[1;31mUnknown command.\033[0m\n";
}

void Console::clear() {
    cmdArt::visualClear();
}

void Console::showArt() {
    cmdArt::showArt();
}
