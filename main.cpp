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

    file >> globalConfig.numCPU
        >> globalConfig.scheduler
        >> globalConfig.quantumCycles
        >> globalConfig.batchFreq
        >> globalConfig.minIns
        >> globalConfig.maxIns
        >> globalConfig.delayPerExec
        >> globalConfig.maxOverallMem
        >> globalConfig.memPerFrame
        >> globalConfig.minMemPerProc
        >> globalConfig.maxMemPerProc;

    memoryManager = new MemoryManager(globalConfig.maxOverallMem, globalConfig.memPerFrame);



    file.close();
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