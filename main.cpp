// os_emulator_scheduler.cpp
// Main entry point for the OS Emulator Scheduler CLI

#include "Console.h"
#include "Scheduler.h"
#include <thread>
#include <fstream>

bool isInitialized = false;
Config globalConfig;
Scheduler scheduler;

bool readConfigFile(const std::string& path = "config.txt") {
    std::ifstream file(path);
    if (!file.is_open()) return false;

    std::string line;
    int lineNum = 0;

    while (std::getline(file, line)) {
        // Remove inline comments
        size_t commentPos = line.find('#');
        if (commentPos != std::string::npos)
            line = line.substr(0, commentPos);

        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (line.empty()) continue;

        std::istringstream iss(line);

        switch (lineNum++) {
        case 0: iss >> globalConfig.numCPU; break;
        case 1: iss >> globalConfig.scheduler; break;
        case 2: iss >> globalConfig.quantumCycles; break;
        case 3: iss >> globalConfig.batchFreq; break;
        case 4: iss >> globalConfig.minIns; break;
        case 5: iss >> globalConfig.maxIns; break;
        case 6: iss >> globalConfig.delayPerExec; break;
        }
    }

    file.close();
    return (lineNum >= 7); // Ensure all config values were read
}


int main() {
    if (!readConfigFile()) {
        std::cerr << "Failed to read config.txt.\n";
        return 1;
    }

    scheduler.initialize(globalConfig);
    isInitialized = false;

    Console::drawMainMenu();
    return 0;
}