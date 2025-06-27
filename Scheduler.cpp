// Scheduler.cpp
#include "Scheduler.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>
#include <random>
#include <unordered_set>

void Scheduler::initialize(const Config& cfg) {
    config = cfg;
}

void Scheduler::start() {
    running = true;
    acceptingNewProcesses = true;  //  start dummy generation

    // Launch CPU worker threads
    for (int i = 0; i < config.numCPU; ++i) {
        cpuThreads.emplace_back(&Scheduler::cpuLoop, this, i);
    }

    // Launch dummy process generator in a managed thread
    processGenThread = std::thread([this]() {
        int id = 0;
        while (acceptingNewProcesses) {
            std::this_thread::sleep_for(std::chrono::milliseconds(config.batchFreq * 50));
            if (!acceptingNewProcesses) break;

            ProcessControlBlock pcb;
            std::ostringstream name;
            name << "p" << std::setw(3) << std::setfill('0') << id++;
            pcb.name = name.str();

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

            int numInstructions = config.minIns;
            if (config.minIns < config.maxIns) {
                numInstructions += rand() % (config.maxIns - config.minIns + 1);
            }

            pcb.generateInstructions(numInstructions);

            std::lock_guard<std::mutex> lock(schedulerMutex);
            auto [it, inserted] = allProcesses.emplace(pcb.name, std::move(pcb));
            readyQueue.push(&(it->second));
        }
        });
}

void Scheduler::stopProcessGeneration() {
    acceptingNewProcesses = false;
    if (processGenThread.joinable()) {
        processGenThread.join();  //  cleanly stop dummy generation
    }
}



void Scheduler::stop() {
    running = false;
    for (auto& t : cpuThreads) {
        if (t.joinable()) t.join();
    }
    cpuThreads.clear();
}

void Scheduler::cpuLoop(int coreId) {
    while (running) {
        ProcessControlBlock* process = nullptr;
        {
            std::lock_guard<std::mutex> lock(schedulerMutex);
            if (!readyQueue.empty()) {
                process = readyQueue.front();
                readyQueue.pop();
            }
        }

        if (process) {
            int executed = 0;
            int quantum = (config.scheduler == "rr") ? config.quantumCycles : INT_MAX;

            while (!process->isFinished && executed < quantum) {
                process->executeNextInstruction(coreId);
                ++executed;
            }
            std::lock_guard<std::mutex> lock(schedulerMutex);
            if (process->isFinished) {
                finishedProcesses.push_back(process);
            }
            else {
                readyQueue.push(process);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(config.delayPerExec * 50));
        ++cpuTick;
    }
}


ProcessControlBlock Scheduler::createRandomProcess(int id) {
    ProcessControlBlock pcb;
    std::ostringstream name;
    name << "p" << std::setw(3) << std::setfill('0') << id;
    pcb.name = name.str();
    int numInstructions = config.minIns + rand() % (config.maxIns - config.minIns + 1);
    pcb.generateInstructions(numInstructions);
    return pcb;
}

std::vector<ProcessControlBlock*> Scheduler::getRunningProcesses() {
    std::vector<ProcessControlBlock*> result;
    for (auto& [name, proc] : allProcesses) {
        if (!proc.isFinished) result.push_back(&proc);
    }
    return result;
}

std::vector<ProcessControlBlock*> Scheduler::getFinishedProcesses() {
    return finishedProcesses;
}

void Scheduler::reportUtilization(bool toFile) {
    std::ostringstream report;
    auto running = getRunningProcesses();
    auto finished = getFinishedProcesses();

    //  Accurately count unique cores in use
    std::unordered_set<int> activeCores;
    for (auto* p : running) {
        if (!p->isFinished) {
            activeCores.insert(p->lastExecutedCore);
        }
    }
    int coresInUse = static_cast<int>(activeCores.size());
    int coresAvailable = std::max(0, config.numCPU - coresInUse);

    //  Report with correct core count
    report << "=== CPU UTILIZATION ===\n";
    report << "CPU Cores        : " << config.numCPU << "\n";
    report << "Cores In Use     : " << coresInUse << "\n";
    report << "Cores Available  : " << coresAvailable << "\n";

    report << "\n=== RUNNING PROCESSES ===\n";
    for (auto* p : running) {
        report << p->name << " (" << p->getStartTime() << ")  "
            << "Core: " << p->lastExecutedCore << " "
            << p->instructionPointer << "/" << p->totalInstructions() << "\n";
    }

    report << "\n=== FINISHED PROCESSES ===\n";
    for (auto* p : finished) {
        report << p->name << " (" << p->getStartTime() << ") "
            << "Finished " << p->instructionPointer << "/" << p->totalInstructions() << "\n";
    }

    if (toFile) {
        std::ofstream out("csopesy-log.txt");
        out << report.str();
        out.close();
    }
    else {
        std::cout << report.str();
    }
}

void Scheduler::createNamedProcess(const std::string& name) {

    if (!running) {
        running = true;
        for (int i = 0; i < config.numCPU; ++i) {
            cpuThreads.emplace_back(&Scheduler::cpuLoop, this, i);
        }
    }

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

    int numInstructions = config.minIns;
    if (config.minIns < config.maxIns) {
        numInstructions += rand() % (config.maxIns - config.minIns + 1);
    }

    pcb.generateInstructions(numInstructions);

    std::lock_guard<std::mutex> lock(schedulerMutex);
    auto [it, inserted] = allProcesses.emplace(pcb.name, std::move(pcb));
    readyQueue.push(&(it->second));
}

