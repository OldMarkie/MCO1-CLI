#pragma once
#include <vector>
#include <string>
#include <ostream>

struct MemoryBlock {
    int start;
    int end;
    std::string processName;

    int size() const {
        return end - start;
    }
};

class MemoryManager {
public:
    MemoryManager(int totalMemoryBytes, int memPerProcBytes);

    bool allocate(const std::string& processName);
    void deallocate(const std::string& processName);
    int getExternalFragmentationKB() const;
    void printMemoryLayout(std::ostream& out) const;
    int getActiveProcessCount() const;

private:
    int totalMemory;
    int memPerProc;
    std::vector<MemoryBlock> allocations;

    std::vector<MemoryBlock> getSortedAllocations() const;
};
