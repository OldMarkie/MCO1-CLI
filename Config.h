// Config.h
#pragma once
#include <string>

struct Config {
    int numCPU;             // [1, 128]
    std::string scheduler; // "fcfs" or "rr"
    int quantumCycles;     // [1, ...] only used in rr
    int batchFreq;         // [1, ...] CPU ticks between batch spawns
    int minIns;            // min instructions per process
    int maxIns;            // max instructions per process
    int delayPerExec;      // CPU cycles to delay between each instruction
    
    // NEW for MO2
    int maxOverallMem;
    int memPerFrame;
    int minMemPerProc;
    int maxMemPerProc;
};