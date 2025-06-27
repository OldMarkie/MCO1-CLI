// Scheduler.h
#pragma once

#include "ProcessControlBlock.h"
#include "Config.h"
#include <vector>
#include <queue>
#include <unordered_map>
#include <atomic>
#include <thread>
#include <mutex>

class Scheduler {
public:
    void initialize(const Config& config);
    void start();
    void stop();
    void tick();
    void generateProcess();
    std::vector<ProcessControlBlock*> getRunningProcesses();
    std::vector<ProcessControlBlock*> getFinishedProcesses();
    void reportUtilization(bool toFile = false);
    void stopProcessGeneration();  
    std::unordered_map<std::string, ProcessControlBlock> allProcesses;
    std::mutex schedulerMutex;
    void createNamedProcess(const std::string& name);


private:
    Config config;
    std::atomic<bool> running{ false };
    std::atomic<int> cpuTick{ 0 };

    std::vector<std::thread> cpuThreads;
    

    std::queue<ProcessControlBlock*> readyQueue;
    
    std::vector<ProcessControlBlock*> finishedProcesses;


    std::atomic<bool> acceptingNewProcesses{ false };
    std::thread processGenThread;

 
    void cpuLoop(int coreId);
    ProcessControlBlock createRandomProcess(int id);
};