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
#include "ProcessControlBlock.h"
#include "MemoryManager.h"

std::unordered_map<std::string, ScreenSession> sessions;
extern bool isInitialized;
extern Scheduler scheduler;
extern Config globalConfig;
extern MemoryManager memoryManager;


bool parseUserInstruction(const std::string& text, Instruction& out);


void Console::drawMainMenu() {
    std::string uChoice;
    bool isRunning = true;
    Console::showArt();
    cmdArt::showMenu();

    while (isRunning) {
        std::cout << "Enter command: ";
        std::cin >> uChoice;

        if (!isInitialized && uChoice != "initialize" && uChoice != "exit" && uChoice != "clear") {
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

                auto it = scheduler.allProcesses.find(screenName);
                if (it == scheduler.allProcesses.end()) {
                    std::cout << "[ERROR] Process " << screenName << " not found.\n\n";
                    continue;  //  Avoid crash/freeze and go back to command prompt
                }

                auto& pcb = it->second;
                sessions[screenName] = createNewScreenSession(pcb.name);
                cmdArt::displayNewSesh(screenName);
                Console::drawScreenSession(screenName);  // enters interactive session
            }

            else if (screenCmd == "-c") {
                std::string name;
                int memSize;
                std::string instructionString;

                std::cin >> name >> memSize;
                std::getline(std::cin, instructionString);  // Get rest of the line
                size_t firstQuote = instructionString.find('"');
                size_t lastQuote = instructionString.rfind('"');

                if (firstQuote == std::string::npos || lastQuote == std::string::npos || firstQuote == lastQuote) {
                    std::cout << "Invalid command. Instructions must be enclosed in quotes.\n\n";
                    return;
                }

                std::string instructionsRaw = instructionString.substr(firstQuote + 1, lastQuote - firstQuote - 1);
                std::vector<std::string> tokens;

                std::istringstream iss(instructionsRaw);
                std::string token;
                while (std::getline(iss, token, ';')) {
                    if (!token.empty()) tokens.push_back(token);
                }

                if (tokens.size() < 1 || tokens.size() > 50) {
                    std::cout << "Invalid command. Instruction count must be between 1 and 50.\n\n";
                    return;
                }

                // Create process and assign instructions
                ProcessControlBlock pcb;
                pcb.name = name;
                auto now = std::chrono::system_clock::now();
                std::time_t now_c = std::chrono::system_clock::to_time_t(now);
                std::tm timeinfo;
#ifdef _WIN32
                localtime_s(&timeinfo, &now_c);
#else
                localtime_r(&now_c, &timeinfo);
#endif
                std::ostringstream oss;
                oss << std::put_time(&timeinfo, "%m/%d/%Y %I:%M:%S%p");
                pcb.startTime = oss.str();

                for (const auto& instr : tokens) {
                    Instruction parsed;
                    if (parseUserInstruction(instr, parsed)) {
                        pcb.addInstruction(parsed);
                    }
                    else {
                        std::cout << "Invalid instruction: " << instr << "\n";
                    }
                }

                std::lock_guard<std::mutex> lock(scheduler.schedulerMutex);
                auto [it, inserted] = scheduler.allProcesses.emplace(name, std::move(pcb));
                scheduler.readyQueue.push(&(it->second));

                memoryManager.allocateMemory(name, memSize);

                sessions[name] = createNewScreenSession(name);
                cmdArt::displayNewSesh(name);
                Console::drawScreenSession(name);
            }

            else if (screenCmd == "-r") {
                std::cin >> screenName;

                //std::lock_guard<std::mutex> lock(scheduler.schedulerMutex);
                auto it = scheduler.allProcesses.find(screenName);
                if (it != scheduler.allProcesses.end()) {
                    if (!it->second.isFinished) {
                        if (sessions.find(screenName) == sessions.end()) {
                            auto& pcb = it->second;
                            sessions[screenName] = createNewScreenSession(pcb.name);
                        }
                        Console::drawScreenSession(screenName);
                    }
                    else if (!it->second.violationTime.empty()) {
                        std::cout << "Process " << screenName
                            << " shut down due to memory access violation error that occurred at "
                            << it->second.violationTime << ". " << it->second.violationAddr << " invalid.\n\n";
                    }
                    else {
                        std::cout << "Process " << screenName << " not found.\n\n";
                    }
                }

            }
            else if (screenCmd == "-ls") {
                auto running = scheduler.getRunningProcesses();
                auto finished = scheduler.getFinishedProcesses();

                std::unordered_set<int> activeCores;
                for (auto* p : running) {
                    if (!p->isFinished && p->lastExecutedCore >= 0) {
                        activeCores.insert(p->lastExecutedCore);
                    }
                }
                int coresInUse = static_cast<int>(activeCores.size());
                int totalCores = globalConfig.numCPU;
                int coresAvailable = std::max(0, totalCores - coresInUse);

                std::cout << "\n=== CPU UTILIZATION ===\n";
                std::cout << "CPU Cores        : " << totalCores << "\n";
                std::cout << "Cores In Use     : " << coresInUse << "\n";
                std::cout << "Cores Available  : " << coresAvailable << "\n";

                std::cout << "\nRunning processes:\n";
                for (auto* p : running) {
                    std::string coreDisplay = (p->lastExecutedCore >= 0)
                        ? std::to_string(p->lastExecutedCore)
                        : "Queued";

                    std::cout << p->name << " (" << p->getStartTime() << ")  "
                        << "Core: " << coreDisplay << " "
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
        else if (uChoice == "vmstat") {
            memoryManager.debugVMStat();

            std::cout << "CPU Ticks (estimates):\n";
            std::cout << "  Total ticks     : " << scheduler.getCpuTick() << "\n";
            std::cout << "  Active ticks    : ~" << scheduler.getActiveTicks() << "\n";
            std::cout << "  Idle ticks      : ~" << scheduler.getIdleTicks() << "\n\n";
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
            cmdArt::showMenu();
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

bool parseUserInstruction(const std::string& text, Instruction& out) {
    std::istringstream ss(text);
    std::string cmd;
    ss >> cmd;

    if (cmd == "DECLARE") {
        std::string var; int val;
        ss >> var >> val;
        out.type = InstructionType::DECLARE;
        out.args = { var, static_cast<uint16_t>(val) };
        return true;
    }
    else if (cmd == "ADD" || cmd == "SUBTRACT") {
        std::string dest, arg1, arg2;
        ss >> dest >> arg1 >> arg2;
        InstructionArg a1 = isdigit(arg1[0]) ? InstructionArg((uint16_t)std::stoi(arg1)) : InstructionArg(arg1);
        InstructionArg a2 = isdigit(arg2[0]) ? InstructionArg((uint16_t)std::stoi(arg2)) : InstructionArg(arg2);
        out.type = (cmd == "ADD") ? InstructionType::ADD : InstructionType::SUBTRACT;
        out.args = { dest, a1, a2 };
        return true;
    }
    else if (cmd == "PRINT") {
        std::string rest;
        getline(ss, rest);
        size_t quote1 = rest.find('"');
        size_t quote2 = rest.rfind('"');
        std::string msg = (quote1 != std::string::npos && quote2 != std::string::npos)
            ? rest.substr(quote1 + 1, quote2 - quote1 - 1) : "[Invalid PRINT]";
        out.type = InstructionType::PRINT;
        out.args = { msg };
        return true;
    }
    else if (cmd == "SLEEP") {
        int ticks; ss >> ticks;
        out.type = InstructionType::SLEEP;
        out.args = { static_cast<uint16_t>(ticks) };
        return true;
    }
    else if (cmd == "READ") {
        std::string var, hexAddr; ss >> var >> hexAddr;
        out.type = InstructionType::READ;
        out.args = { var, hexAddr };
        return true;
    }
    else if (cmd == "WRITE") {
        std::string hexAddr, valOrVar; ss >> hexAddr >> valOrVar;
        InstructionArg val = isdigit(valOrVar[0]) ? InstructionArg((uint16_t)std::stoi(valOrVar)) : InstructionArg(valOrVar);
        out.type = InstructionType::WRITE;
        out.args = { hexAddr, val };
        return true;
    }

    return false;
}


