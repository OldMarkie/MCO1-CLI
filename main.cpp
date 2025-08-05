// os_emulator_scheduler.cpp
// Main entry point for the OS Emulator Scheduler CLI

#include "Console.h"
#include "Scheduler.h"
#include <thread>
#include <fstream>

#include "MemoryManager.h"
#include "Config.h"




bool isInitialized = false;
Config globalConfig;
Scheduler scheduler;

MemoryManager* memoryManager;


bool readConfigFile(const std::string& path = "config.txt") {
    std::ifstream file(path);
    if (!file.is_open()) return false;

    std::string line;
    auto readNextValue = [&file, &line](auto& out) -> bool {
        while (std::getline(file, line)) {
            // skip empty or comment lines
            if (line.empty() || line[0] == '#') continue;

            std::istringstream iss(line);
            return static_cast<bool>(iss >> out);
        }
        return false;
        };

    if (!readNextValue(globalConfig.numCPU)) return false;
    if (!readNextValue(globalConfig.scheduler)) return false;
    if (!readNextValue(globalConfig.quantumCycles)) return false;
    if (!readNextValue(globalConfig.batchFreq)) return false;
    if (!readNextValue(globalConfig.minIns)) return false;
    if (!readNextValue(globalConfig.maxIns)) return false;
    if (!readNextValue(globalConfig.delayPerExec)) return false;
    if (!readNextValue(globalConfig.maxOverallMem)) return false;
    if (!readNextValue(globalConfig.memPerFrame)) return false;
    if (!readNextValue(globalConfig.minMemPerProc)) return false;
    if (!readNextValue(globalConfig.maxMemPerProc)) return false;

    memoryManager = new MemoryManager(globalConfig.maxOverallMem, globalConfig.memPerFrame);
    return true;
}


int main() {
    if (!readConfigFile()) {
        std::cerr << "Failed to read config.txt.\n";
        return 1;
    }

    scheduler.initialize(globalConfig);
    isInitialized = false;

    Console::drawMainMenu();
    delete memoryManager;
    memoryManager = nullptr;
    return 0;
}