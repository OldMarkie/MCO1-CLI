#include "Console.h"
#include "cmdArt.h"
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <fstream>
#include "Scheduler.h"

std::unordered_map<std::string, ScreenSession> sessions;
extern bool isInitialized;
extern Scheduler scheduler;

void Console::drawMainMenu() {
    std::string uChoice;
    bool isRunning = true;
    Console::showArt();

    while (isRunning) {
        cmdArt::showMenu();
        std::cin >> uChoice;

        if (!isInitialized && uChoice != "initialize" && uChoice != "exit") {
            std::cout << "System not initialized. Use 'initialize' first.\n\n";
            continue;
        }

        if (uChoice == "initialize") {
            std::cout << "Initialization complete.\n\n";
            isInitialized = true;
        }
        else if (uChoice == "screen") {
            std::string screenCmd, screenName;
            std::cin >> screenCmd;

            if (screenCmd == "-s") {
                std::cin >> screenName;
                sessions[screenName] = createNewScreenSession(screenName);
                cmdArt::displayNewSesh(screenName);
                Console::drawScreenSession(screenName);
            }
            else if (screenCmd == "-r") {
                std::cin >> screenName;
                if (sessions.find(screenName) != sessions.end()) {
                    Console::drawScreenSession(screenName);
                }
                else {
                    std::cout << "Process " << screenName << " not found.\n\n";
                }
            }
            else if (screenCmd == "-ls") {
                auto running = scheduler.getRunningProcesses();
                auto finished = scheduler.getFinishedProcesses();

                std::cout << "\nRunning processes:\n";
                for (auto* p : running) {
                    std::cout << p->name << " (" << p->getStartTime() << ")  "
                        << "Core: " << p->lastExecutedCore << " "
                        << p->instructionPointer << "/" << p->totalInstructions() << "\n";
                }

                std::cout << "\nFinished processes:\n";
                for (auto* p : finished) {
                    std::cout << p->name << " (" << p->getStartTime() << ") "
                        << "Finished " << p->instructionPointer << "/" << p->totalInstructions() << "\n";
                }

                std::cout << std::endl;
            }
            else {
                std::cout << "Invalid screen command. Use '-s <name>', '-r <name>', or '-ls'.\n\n";
            }

        }
        else if (uChoice == "scheduler-start") {
            scheduler.start();
            std::cout << "Scheduler started.\n\n";
        }
        else if (uChoice == "scheduler-stop") {
            scheduler.stop();
            std::cout << "Scheduler stopped.\n\n";
        }
        else if (uChoice == "report-util") {
            scheduler.reportUtilization(true);
            std::cout << "CPU utilization saved to csopesy-log.txt\n\n";
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
    cmdArt::visualClear();
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
        else if (screenInput == "process-smi") {
            auto running = scheduler.getRunningProcesses();
            auto finished = scheduler.getFinishedProcesses();
            ProcessControlBlock* target = nullptr;
            for (auto* p : running) if (p->name == screenName) target = p;
            for (auto* p : finished) if (p->name == screenName) target = p;

            if (target) {
                std::cout << "\n--- Process Info ---\n";
                std::cout << "Process: " << target->name << (target->isFinished ? " [Finished]" : " [Running]") << "\n";
                std::cout << target->getLog() << "\n";
            }
            else {
                std::cout << "Process " << screenName << " not found.\n";
            }

            std::cout << "> ";
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

