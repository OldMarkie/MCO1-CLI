// Scheduler.cpp
#include "Scheduler.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>
#include <random>
#include <unordered_set>
#include "MemoryManager.h"

extern MemoryManager* memoryManager;


void Scheduler::initialize(const Config& cfg) {
    config = cfg;
}

void Scheduler::start() {
    running = true;
    acceptingNewProcesses = true;  //  start dummy generation

    // Launch background thread to retry waiting process memory allocation
    

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

            int memSize = config.minMemPerProc;
            if (config.minMemPerProc < config.maxMemPerProc) {
                memSize += rand() % (config.maxMemPerProc - config.minMemPerProc + 1);
            }

            pcb.generateInstructions(numInstructions,0, memSize);

            std::lock_guard<std::recursive_mutex> lock(schedulerMutex);

            
            // Insert into process table first
            auto [it, inserted] = allProcesses.emplace(pcb.name, std::move(pcb));
            ProcessControlBlock& ref = it->second;

            memoryManager->allocateMemory(ref.name, memSize);
            readyQueue.push(&ref);


        }
        });
}

void Scheduler::stopProcessGeneration() {
    acceptingNewProcesses = false;
    if (processGenThread.joinable()) {
        processGenThread.join();  //  cleanly stop dummy generation
    }
    if (retryThread.joinable()) {
        retryThread.join();
    }

}



void Scheduler::stop() {
    if (!running) return;
    running = false;

    if (processGenThread.joinable()) {
        processGenThread.join();  // just in case
    }

    
    for (auto& t : cpuThreads) {
        if (t.joinable()) {
            t.join();
        }
    }
    cpuThreads.clear();
}


void Scheduler::cpuLoop(int coreId) {
    while (running) {
        ProcessControlBlock* process = nullptr;
        {
            std::lock_guard<std::recursive_mutex> lock(schedulerMutex);

            if (!readyQueue.empty()) {
                process = readyQueue.front();
                readyQueue.pop();
            }
        }

        if (process) {
            ++numCPUActiveEstimate;  // Mark this core as active

            if (config.scheduler == "rr") {
                int executed = 0;
                int quantum = config.quantumCycles;

                while (!process->isFinished && executed < quantum) {
                    process->executeNextInstruction(coreId);
                    ++executed;
                }

                std::lock_guard<std::recursive_mutex> lock(schedulerMutex);

                if (process->isFinished) {
                    memoryManager->freeMemory(process->name);
                    finishedProcesses.push_back(process);
                }
                else {
                    readyQueue.push(process);  // Requeue if not done
                }
            }

            else if (config.scheduler == "fcfs") {
                // FCFS: run to completion, no quantum
                while (!process->isFinished) {
                    process->executeNextInstruction(coreId);
                }

                std::lock_guard<std::recursive_mutex> lock(schedulerMutex);

                memoryManager->freeMemory(process->name);
                finishedProcesses.push_back(process);
            }

            --numCPUActiveEstimate;  // Core done
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
    int memSize = config.minMemPerProc;
    if (config.minMemPerProc < config.maxMemPerProc) {
        memSize += rand() % (config.maxMemPerProc - config.minMemPerProc + 1);
    }
    pcb.generateInstructions(numInstructions,0, memSize);
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

    //  Always check if retryThread is started
    if (!retryThread.joinable()) {
        retryThread = std::thread([this]() {
            while (running) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                std::lock_guard<std::recursive_mutex> lock(schedulerMutex);

                if (!fcfsRetryQueue.empty()) {
                    ProcessControlBlock* pcb = fcfsRetryQueue.front();
                    int memSize = allocatedRetrySizes[pcb->name];

                    int used = memoryManager->getUsedBytes();
                    if (used + memSize <= config.maxOverallMem) {
                        fcfsRetryQueue.pop();
                        memoryManager->allocateMemory(pcb->name, memSize);
                        readyQueue.push(pcb);
                        std::cout << "[Scheduler] Retried and loaded: " << pcb->name << "\n";
                    }
                }
            }
            });
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

    int memSize = config.minMemPerProc;
    if (config.minMemPerProc < config.maxMemPerProc) {
        memSize += rand() % (config.maxMemPerProc - config.minMemPerProc + 1);
    }

    pcb.generateInstructions(numInstructions, 0, memSize);

    std::lock_guard<std::recursive_mutex> lock(schedulerMutex);


    int used = memoryManager->getUsedBytes();
    if (used + memSize <= config.maxOverallMem) {
        auto [it, inserted] = allProcesses.emplace(pcb.name, std::move(pcb));
        ProcessControlBlock& procRef = it->second;
        memoryManager->allocateMemory(procRef.name, memSize);
        readyQueue.push(&procRef);
        std::cout << "[Scheduler] Loaded process " << name << " (" << memSize << "B)\n";
    }
    else {
        std::cout << "[Scheduler] Insufficient memory for " << name << ", added to FCFS retry queue.\n";
       
        auto [it, inserted] = allProcesses.emplace(pcb.name, std::move(pcb));
        ProcessControlBlock& procRef = it->second;
        fcfsRetryQueue.push(&procRef);
        allocatedRetrySizes[procRef.name] = memSize;  // keep mem size here

    }
}

void Scheduler::createNamedProcessWithInstructions(const std::string& name, const std::vector<Instruction>& instructions) {
    if (!running) {
        running = true;

        for (int i = 0; i < config.numCPU; ++i) {
            cpuThreads.emplace_back(&Scheduler::cpuLoop, this, i);
        }
    }

    if (!retryThread.joinable()) {
        retryThread = std::thread([this]() {
            while (running) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                std::lock_guard<std::recursive_mutex> lock(schedulerMutex);

                if (!fcfsRetryQueue.empty()) {
                    ProcessControlBlock* pcb = fcfsRetryQueue.front();
                    int memSize = allocatedRetrySizes[pcb->name];
                    int used = memoryManager->getUsedBytes();
                    if (used + memSize <= config.maxOverallMem) {
                        fcfsRetryQueue.pop();
                        memoryManager->allocateMemory(pcb->name, memSize);
                        readyQueue.push(pcb);
                        std::cout << "[Scheduler] Retried and loaded: " << pcb->name << "\n";
                    }
                }
            }
            });
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

    for (const auto& instr : instructions) {
        pcb.addInstruction(instr);
    }

    std::lock_guard<std::recursive_mutex> lock(schedulerMutex);
    auto [it, inserted] = allProcesses.emplace(pcb.name, std::move(pcb));
    ProcessControlBlock& procRef = it->second;

    int memSize = config.minMemPerProc;
    if (config.minMemPerProc < config.maxMemPerProc) {
        memSize += rand() % (config.maxMemPerProc - config.minMemPerProc + 1);
    }

    int used = memoryManager->getUsedBytes();
    if (used + memSize <= config.maxOverallMem) {
        memoryManager->allocateMemory(procRef.name, memSize);
        readyQueue.push(&procRef);
        std::cout << "[Scheduler] Loaded named process " << name << " (" << memSize << "B)\n";
    }
    else {
        fcfsRetryQueue.push(&procRef);
        allocatedRetrySizes[procRef.name] = memSize;
        std::cout << "[Scheduler] Insufficient memory for " << name << ", added to FCFS retry queue.\n";
    }
}







int Scheduler::getCpuTick() const {
    return cpuTick;
}

int Scheduler::getActiveTicks() const {
    return cpuTick * numCPUActiveEstimate;
}

int Scheduler::getIdleTicks() const {
    return getCpuTick() * config.numCPU - getActiveTicks();
}

Scheduler::~Scheduler() {
    stop();  // cleanly stop CPU threads if still running
    stopProcessGeneration();
}








