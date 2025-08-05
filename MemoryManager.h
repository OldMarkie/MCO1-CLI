#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <cstdint>
#include <fstream>
#include "ProcessControlBlock.h"

struct PageTableEntry {
    int frameNumber = -1;     // -1 means not in memory
    bool valid = false;
    bool dirty = false;
};

class MemoryManager {
public:
    MemoryManager(int totalMemBytes, int frameSize);

    int allocateMemory(const std::string& processName, int memSize);
    void freeMemory(const std::string& processName);

    bool handlePageFault(const std::string& processName, uint32_t addr);
    uint16_t readMemory(const std::string& processName, uint32_t addr);
    void writeMemory(const std::string& processName, uint32_t addr, uint16_t value);

    void writeBackToStore(const std::string& processName, int pageNum);
    void loadPageFromStore(const std::string& processName, int pageNum, int frameNum);

    void debugVMStat();  // used by `vmstat` command
    void debugProcessSMI();  // used by `process-smi` command
    std::unordered_map<std::string, int> allocatedBytes;

    int getUsedBytes() const;

    int getAllocatedSize(const std::string& processName) const;
  
    bool isPageLoaded(const std::string& processName, uint32_t address);
    void ensurePagesPresent(
        const std::string& processName,
        const Instruction& instr,
        const std::unordered_map<std::string, uint16_t>& symbolTable);
    uint32_t getSymbolTableAddress(const std::string& processName);
    // MemoryManager.h
    std::unordered_map<std::string, uint32_t> processMaxAddressable;

    uint32_t getMaxAddressable(const std::string& processName) const;



private:
    int totalMemory;
    int frameSize;
    int totalFrames;

    std::vector<std::string> frameTable;  // maps frame index to process.page
    std::unordered_map<std::string, std::vector<PageTableEntry>> pageTables;
    std::unordered_map<std::string, std::vector<uint16_t>> backingStore;  // optional cache

    std::vector<uint16_t> physicalMemory;
   

    int pagedIn = 0;
    int pagedOut = 0;

    int findFreeFrame();
    int evictPageLRU();  // basic placeholder
};
