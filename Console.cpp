#include "Console.h"
#include "cmdArt.h"
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <fstream>
#include "Scheduler.h"
#include <unordered_set>

std::unordered_map<std::string, ScreenSession> sessions;
extern bool isInitialized;
extern Scheduler scheduler;
extern Config globalConfig;

void Console::drawMainMenu() {
    std::string uChoice;
    bool isRunning = true;
    Console::showArt();
    cmdArt::showMenu();

    while (isRunning) {
        std::cout << "Enter command: ";
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
                
                scheduler.createNamedProcess(screenName);
                
                auto& pcb = scheduler.allProcesses.at(screenName);
                sessions[screenName] = createNewScreenSession(pcb.name);
                cmdArt::displayNewSesh(screenName);
                Console::drawScreenSession(screenName);  // enters interactive session
            }
            else if (screenCmd == "-r") {
                std::cin >> screenName;

                //std::lock_guard<std::mutex> lock(scheduler.schedulerMutex);
                auto it = scheduler.allProcesses.find(screenName);
                if (it != scheduler.allProcesses.end() && !it->second.isFinished) {
                    if (sessions.find(screenName) == sessions.end()) {
                        auto& pcb = scheduler.allProcesses.at(screenName);
                        sessions[screenName] = createNewScreenSession(pcb.name);
                    }
                    Console::drawScreenSession(screenName);
                }
                else {
                    std::cout << "Process " << screenName << " not found.\n\n";
                }
            }
            else if (screenCmd == "-ls") {
                auto running = scheduler.getRunningProcesses();
                auto finished = scheduler.getFinishedProcesses();

                std::unordered_set<int> activeCores;
                for (auto* p : running) {
                    if (!p->isFinished) {
                        activeCores.insert(p->lastExecutedCore);
                    }
                }
                int coresInUse = static_cast<int>(activeCores.size());

                int totalCores = globalConfig.numCPU;

                int coresAvailable = std::max(0, globalConfig.numCPU - coresInUse);

                std::cout << "\n=== CPU UTILIZATION ===\n";
                std::cout << "CPU Cores        : " << globalConfig.numCPU << "\n";
                std::cout << "Cores In Use     : " << coresInUse << "\n";
                std::cout << "Cores Available  : " << coresAvailable << "\n";

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
            scheduler.stopProcessGeneration();  // ONLY stops dummy process creation
            std::cout << "Dummy process generation stopped.\n";
        }

        else if (uChoice == "report-util") {
            scheduler.reportUtilization(true);
            std::cout << "CPU utilization saved to csopesy-log.txt\n\n";
        }
        else if (uChoice == "clear") {
            Console::clear();
            Console::showArt();
            cmdArt::showMenu();
        }
        else if (uChoice == "exit") {
            isRunning = false;
            scheduler.stop();
        }
        else {
            Console::showUnknownCommand();
        }
    }
}

void Console::drawScreenSession(const std::string& screenName) {
    while (true) {
        Console::clear();

        std::lock_guard<std::mutex> lock(scheduler.schedulerMutex);
        auto it = scheduler.allProcesses.find(screenName);
        if (it == scheduler.allProcesses.end()) {
            std::cout << "Process not found.\n";
            std::this_thread::sleep_for(std::chrono::seconds(2));
            return;
        }

        const auto& pcb = it->second;

        // Header info
        std::cout << "\n==== Process Console ====\n";
        std::cout << "Process Name      : " << pcb.name << "\n";
        std::cout << "Instruction Line  : " << (pcb.instructionPointer + 1)
            << " / " << pcb.totalInstructions() << "\n";
        std::cout << "Created At        : " << pcb.getStartTime() << "\n";
        std::cout << "=========================\n";
        std::cout << "Commands: process-smi | exit\n\n> ";

        // Read user input
        std::string cmd;
        std::cin >> cmd;

        if (cmd == "exit") {
            Console::clear();
            Console::showArt();
            return;
        }
        else if (cmd == "process-smi") {
            std::cout << "\n--- Process Info ---\n";
            std::cout << "Process: " << pcb.name << (pcb.isFinished ? " [Finished]" : " [Running]") << "\n";
            std::cout << pcb.getLog() << "\n";
            std::cout << "---------------------\n";
            std::cout << "Press Enter to continue...";
            std::cin.ignore();
            std::cin.get();
        }
        else {
            std::cout << "Unknown command.\n";
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}




void Console::showUnknownCommand() {
    std::cout << "\033[1;31mUnknown command.\033[0m\n\n";
}

void Console::clear() {
    cmdArt::visualClear();
}

void Console::showArt() {
    cmdArt::showArt();
}

