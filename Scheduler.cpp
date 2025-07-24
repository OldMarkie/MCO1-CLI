// Scheduler.cpp
#include "Scheduler.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>
#include <random>
#include <unordered_set>
#include "InstructionParser.h"

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

            pcb.generateInstructions(numInstructions,0);

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

    // Optional: also clear memory on shutdown
    for (auto& [name, pcb] : allProcesses) {
        memoryManager.deallocate(name);
    }
}


void Scheduler::cpuLoop(int coreId) {

    int quantumCycleCounter = 1;

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
            // Allocate memory only if not already allocated
            if (allocatedProcesses.find(process->name) == allocatedProcesses.end()) {
                if (!memoryManager.allocate(process->name)) {
                    //  Not enough memory, requeue and skip
                    std::lock_guard<std::mutex> lock(schedulerMutex);
                    readyQueue.push(process);
                    std::this_thread::sleep_for(std::chrono::milliseconds(config.delayPerExec * 50));
                    continue;
                }
                allocatedProcesses.insert(process->name);
            }

            int executed = 0;
            int quantum = (config.scheduler == "rr") ? config.quantumCycles : INT_MAX;

            while (!process->isFinished && executed < quantum) {
                process->executeNextInstruction(coreId);
                ++executed;
            }

            {
                std::lock_guard<std::mutex> lock(schedulerMutex);
                if (process->isFinished) {
                    finishedProcesses.push_back(process);
                    memoryManager.deallocate(process->name);       //  Free memory
                    allocatedProcesses.erase(process->name);       //  Remove from tracking
                }
                else {
                    readyQueue.push(process);                      //  Continue next cycle
                }
            }

            //  Log memory layout after every quantum
            std::ostringstream filename;
            filename << "memory_stamp_" << quantumCycleCounter++ << ".txt";
            std::ofstream snapshot(filename.str());
            memoryManager.printMemoryLayout(snapshot);
            snapshot.close();
        }

        //  Simulate delay between instructions
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
    pcb.generateInstructions(numInstructions,0);
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

bool Scheduler::createNamedProcess(const std::string& name, int memorySize) {
    std::lock_guard<std::mutex> lock(schedulerMutex);

    if (allProcesses.count(name)) return false;

    if (!memoryManager.allocate(name, memorySize)) return false;

    ProcessControlBlock pcb;
    pcb.name = name;

    auto blocks = memoryManager.getSortedAllocations();
    auto it = std::find_if(blocks.begin(), blocks.end(),
        [&](const MemoryBlock& b) { return b.processName == name; });

    if (it != blocks.end()) {
        pcb.allocatedStart = it->start;
        pcb.allocatedSize = it->size();
    }

    pcb.generateInstructions(rand() % (config.maxIns - config.minIns + 1) + config.minIns, 0);

    allProcesses[name] = std::move(pcb);
    readyQueue.push(&allProcesses[name]);
    allocatedProcesses.insert(name);

    return true;
}


bool Scheduler::createNamedProcess(const std::string& name, int memorySize, const std::vector<std::string>& rawInstructions) {
    std::lock_guard<std::mutex> lock(schedulerMutex);

    if (allProcesses.count(name)) return false;

    if (!memoryManager.allocate(name, memorySize)) return false;

    ProcessControlBlock pcb;
    pcb.name = name;

    auto blocks = memoryManager.getSortedAllocations();
    auto it = std::find_if(blocks.begin(), blocks.end(),
        [&](const MemoryBlock& b) { return b.processName == name; });

    if (it != blocks.end()) {
        pcb.allocatedStart = it->start;
        pcb.allocatedSize = it->size();
    }

    for (const auto& instrStr : rawInstructions) {
        auto parsed = InstructionParser::parse(instrStr, name);
        if (parsed.has_value()) {
            pcb.addInstruction(parsed.value());
        }
        else {
            std::cout << "Skipping invalid instruction: " << instrStr << "\n";
        }
    }

    allProcesses[name] = std::move(pcb);
    readyQueue.push(&allProcesses[name]);
    allocatedProcesses.insert(name);

    return true;
}

int Scheduler::getIdleTicks() const { return idleTicks.load(); }


void Scheduler::printVMStat() const {
    std::cout << "=== VMSTAT ===\n";
    std::cout << "Total Frames     : " << memoryManager.getTotalFrames() << "\n";
    std::cout << "Used Frames      : " << memoryManager.getUsedFrames() << "\n";
    std::cout << "Free Frames      : " << memoryManager.getFreeFrames() << "\n";
    std::cout << "Page Faults      : " << memoryManager.getPageFaults() << "\n";
    std::cout << "Pages Swapped In : " << memoryManager.getPagesSwappedIn() << "\n";
    std::cout << "Pages Swapped Out: " << memoryManager.getPagesSwappedOut() << "\n";
    std::cout << "Idle Ticks       : " << getIdleTicks() << "\n";
}



