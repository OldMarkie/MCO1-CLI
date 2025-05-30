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
    Console::showArt();

    while (isRunning) {
        cmdArt::showMenu();
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
                Console::drawScreenSession(screenName);
            }
            else if (screenCmd == "-r") {
                if (sessions.find(screenName) != sessions.end()) {
                    Console::drawScreenSession(screenName);
                }
                else {
                    std::cout << "No session named '" << screenName << "' found.\n\n";
                }
            }
            else {
                std::cout << "Invalid screen command. Use 'screen -s <name>' or 'screen -r <name>'.\n\n";
            }
        }
        else if (uChoice == "scheduler-test") {
            std::cout << "scheduler-test command recognized. Doing something. \n\n";
        }
        else if (uChoice == "scheduler-stop") {
            std::cout << "scheduler-stop command recognized. Doing something. \n\n";
        }
        else if (uChoice == "report-util") {
            std::cout << "report-util command recognized. Doing something.\n\n";
        }
        else if (uChoice == "clear") {
            Console::clear();
            Console::showArt();
        }
        else if (uChoice == "exit") {
            isRunning = false;
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
