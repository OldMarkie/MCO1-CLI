// Scheduler.cpp
#include "Scheduler.h"
#include <iostream>

Scheduler::Scheduler(const Config& cfg) : config(cfg), running(false) {}

void Scheduler::start() {
    running = true;
    for (int i = 0; i < config.numCPU; ++i) {
        cpuThreads.emplace_back(&Scheduler::run, this);
    }
}

void Scheduler::stop() {
    running = false;
    cv.notify_all();
    for (auto& t : cpuThreads) {
        if (t.joinable()) t.join();
    }
}

void Scheduler::tick() {
    cv.notify_all();
}

void Scheduler::addProcess(std::shared_ptr<Process> proc) {
    std::lock_guard<std::mutex> lock(queueMutex);
    readyQueue.push(proc);
    cv.notify_one();
}

void Scheduler::run() {
    while (running) {
        std::unique_lock<std::mutex> lock(queueMutex);
        cv.wait(lock, [&] { return !readyQueue.empty() || !running; });
        if (!running) break;

        auto proc = readyQueue.front();
        readyQueue.pop();
        lock.unlock();

        proc->execute();
    }
}
