// Scheduler.cpp
#include "Scheduler.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>
#include <random>

void Scheduler::initialize(const Config& cfg) {
    config = cfg;
}

void Scheduler::start() {
    running = true;

    // Fixed 10-process test case
    for (int i = 0; i < 10; ++i) {
        ProcessControlBlock pcb;
        std::ostringstream name;
        name << "p" << std::setw(3) << std::setfill('0') << i;
        pcb.name = name.str();


        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
#ifdef _WIN32
        std::tm timeinfo;
        localtime_s(&timeinfo, &now_c);
#else
        std::tm timeinfo;
        localtime_r(&now_c, &timeinfo);
#endif
        std::ostringstream oss;
        oss << std::put_time(&timeinfo, "%m/%d/%Y %I:%M:%S%p");
        pcb.startTime = oss.str(); //  assign here



        for (int j = 0; j < 100; ++j) {
            Instruction instr;
            instr.type = InstructionType::PRINT;
            instr.args = { "Hello world from " + pcb.name };
            pcb.addInstruction(instr);
        }

        std::lock_guard<std::mutex> lock(schedulerMutex);

        // FIX: insert first, then use pointer safely
        auto [it, inserted] = allProcesses.emplace(pcb.name, std::move(pcb));
        readyQueue.push(&(it->second)); // correct and safe pointer
    }

    for (int i = 0; i < config.numCPU; ++i) {
        cpuThreads.emplace_back(&Scheduler::cpuLoop, this, i);
    }

    // Removed batch spawn thread
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
            process->executeNextInstruction(coreId);
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
    report << "CPU Cores     : " << config.numCPU << "\n";
    report << "CPU Tick      : " << cpuTick << "\n";
    report << "Running Procs : " << getRunningProcesses().size() << "\n";
    report << "Finished Procs: " << getFinishedProcesses().size() << "\n";

    if (toFile) {
        std::ofstream out("csopesy-log.txt");
        out << report.str();
        out.close();
    }
    else {
        std::cout << report.str();
    }
}