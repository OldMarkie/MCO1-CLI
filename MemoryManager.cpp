#include "MemoryManager.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <chrono>
#include <mutex>


MemoryManager::MemoryManager(int totalMemBytes, int frameSz)
    : totalMemory(totalMemBytes), frameSize(frameSz) {
    totalFrames = totalMemory / frameSize;
    frameTable.resize(totalFrames, "");
    physicalMemory.resize(totalFrames * (frameSize / 2), 0);  // totalFrames * words per frame
    // each frame holds uint16_t (2 bytes)
}

int MemoryManager::allocateMemory(const std::string& processName, int memSize) {
    int numPages = (memSize + frameSize - 1) / frameSize;

    // Initialize page table entries: all invalid at first
    pageTables[processName] = std::vector<PageTableEntry>(numPages);
    allocatedBytes[processName] = memSize;

    // At end of allocateMemory()
    std::ofstream out("csopesy-backing-store.txt", std::ios::app);
    for (int i = 0; i < numPages; ++i) {
        out << processName << "." << i << ":";
        for (int j = 0; j < frameSize / 2; ++j) {
            out << " 0";
        }
        out << "\n";
    }
    out.close();

    return numPages;  // return success regardless of frame count
}





void MemoryManager::freeMemory(const std::string& processName) {
    if (pageTables.find(processName) == pageTables.end()) return;

    for (int i = 0; i < totalFrames; ++i) {
        if (frameTable[i].find(processName + ".") == 0) {
            frameTable[i] = "";
        }
    }

    pageTables.erase(processName);
    allocatedBytes.erase(processName);
}

bool MemoryManager::handlePageFault(const std::string& processName, uint32_t addr) {
    if (pageTables.find(processName) == pageTables.end()) return false;

    int pageNum = addr / frameSize;
    auto& table = pageTables[processName];

    if (pageNum >= table.size()) return false;

    int frameIdx = findFreeFrame();
    if (frameIdx == -1) {
        frameIdx = evictPageLRU();
    }

    loadPageFromStore(processName, pageNum, frameIdx);

    table[pageNum].frameNumber = frameIdx;
    table[pageNum].valid = true;

    frameTable[frameIdx] = processName + "." + std::to_string(pageNum);

    pagedIn++;
    return true;
}

uint16_t MemoryManager::readMemory(const std::string& processName, uint32_t addr) {
    int pageNum = addr / frameSize;
    int offset = (addr % frameSize) / 2; // because each element is 2 bytes (uint16_t)

    auto& table = pageTables[processName];

    while (pageNum >= table.size() || !table[pageNum].valid) {
        bool success = handlePageFault(processName, addr);
        if (!success) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;  // keep retrying
        }
    }


    int frameNum = table[pageNum].frameNumber;
    int absoluteIdx = (frameNum * frameSize / 2) + offset;

    if (absoluteIdx >= physicalMemory.size()) throw std::runtime_error("Memory index out of range");

    return physicalMemory[absoluteIdx];
}

void MemoryManager::writeMemory(const std::string& processName, uint32_t addr, uint16_t value) {
    int pageNum = addr / frameSize;
    int offset = (addr % frameSize) / 2;

    auto& table = pageTables[processName];

    if (pageNum >= table.size() || !table[pageNum].valid) {
        if (!handlePageFault(processName, addr)) throw std::runtime_error("Access violation");
    }

    int frameNum = table[pageNum].frameNumber;
    int absoluteIdx = (frameNum * frameSize / 2) + offset;

    if (absoluteIdx >= physicalMemory.size()) throw std::runtime_error("Memory index out of range");

    physicalMemory[absoluteIdx] = value;
    table[pageNum].dirty = true;
}

void MemoryManager::loadPageFromStore(const std::string& processName, int pageNum, int frameNum) {
    std::ifstream in("csopesy-backing-store.txt");
    if (!in) return;

    std::string key = processName + "." + std::to_string(pageNum);
    std::string line;
    while (std::getline(in, line)) {
        if (line.find(key) == 0) {
            std::istringstream iss(line.substr(key.size() + 1));
            int index = frameNum * frameSize / 2;
            for (int i = 0; i < frameSize / 2 && iss; ++i) {
                iss >> physicalMemory[index + i];
            }
            break;
        }
    }
}

void MemoryManager::writeBackToStore(const std::string& processName, int pageNum) {
    int frameNum = pageTables[processName][pageNum].frameNumber;
    int startIdx = frameNum * frameSize / 2;

    std::ostringstream oss;
    oss << processName << "." << pageNum << ":";
    for (int i = 0; i < frameSize / 2; ++i) {
        oss << " " << physicalMemory[startIdx + i];
    }
    oss << "\n";

    std::ofstream out("csopesy-backing-store.txt", std::ios::app);
    out << oss.str();
    out.close();

    pagedOut++;
}

int MemoryManager::findFreeFrame() {
    for (int i = 0; i < frameTable.size(); ++i) {
        if (frameTable[i].empty()) return i;
    }
    return -1;
}

int MemoryManager::evictPageLRU() {
    // Simplified: evict the first one we find (can use better LRU later)
    for (int i = 0; i < frameTable.size(); ++i) {
        if (!frameTable[i].empty()) {
            auto token = frameTable[i];
            size_t dot = token.find('.');
            std::string proc = token.substr(0, dot);
            int page = std::stoi(token.substr(dot + 1));

            if (pageTables[proc][page].dirty) {
                writeBackToStore(proc, page);
            }

            pageTables[proc][page].valid = false;
            pageTables[proc][page].frameNumber = -1;
            frameTable[i] = "";

            return i;
        }
    }
    return 0;  // fallback
}

void MemoryManager::debugVMStat() {
    int used = 0;
    for (auto& p : frameTable) if (!p.empty()) ++used;

    std::cout << "\n=== VMSTAT ===\n";
    std::cout << "Total memory     : " << totalMemory << " bytes\n";
    std::cout << "Used memory      : " << used * frameSize << " bytes\n";
    std::cout << "Free memory      : " << (totalFrames - used) * frameSize << " bytes\n";
    std::cout << "Paged In         : " << pagedIn << "\n";
    std::cout << "Paged Out        : " << pagedOut << "\n";
    std::cout << "Total frames     : " << totalFrames << "\n\n";
}

void MemoryManager::debugProcessSMI() {
    std::cout << "\n--- process-smi ---\n";
    std::cout << "Total memory: " << totalMemory << " bytes\n";
    std::cout << "Used frames: " << std::count_if(frameTable.begin(), frameTable.end(), [](const std::string& s) { return !s.empty(); }) << "/" << totalFrames << "\n";

    for (int i = 0; i < frameTable.size(); ++i) {
        if (!frameTable[i].empty()) {
            std::cout << "Frame " << std::setw(3) << i << ": " << frameTable[i] << "\n";
        }
    }
    std::cout << "-------------------\n";
}

int MemoryManager::getUsedBytes() const {
    int used = 0;
    for (const auto& frame : frameTable) {
        if (!frame.empty()) ++used;
    }
    return used * frameSize;
}



