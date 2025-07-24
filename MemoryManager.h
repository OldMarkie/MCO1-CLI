#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <cstdint>
#include <fstream>

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

private:
    int totalMemory;
    int frameSize;
    int totalFrames;

    std::vector<std::string> frameTable;  // maps frame index to process.page
    std::unordered_map<std::string, std::vector<PageTableEntry>> pageTables;
    std::unordered_map<std::string, std::vector<uint16_t>> backingStore;  // optional cache

    std::vector<uint16_t> physicalMemory;
    std::unordered_map<std::string, int> allocatedBytes;

    int pagedIn = 0;
    int pagedOut = 0;

    int findFreeFrame();
    int evictPageLRU();  // basic placeholder
};
