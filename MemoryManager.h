#pragma once
#include <vector>
#include <string>
#include <ostream>
#include <cstdint>
#include <unordered_map>

struct MemoryBlock {
    int start;
    int end;
    std::string processName;

    int size() const {
        return end - start;
    }
};

struct Frame {
    std::string ownerProcess;
    int pageNumber;           // virtual page number of owner
    bool occupied = false;
};

class MemoryManager {
public:
    MemoryManager(int totalMemoryBytes, int memPerFrameBytes)
        : totalMemory(totalMemoryBytes),
        memPerProc(4096),
        memPerFrame(memPerFrameBytes),
        numFrames(totalMemoryBytes / memPerFrameBytes) {
        frameTable.resize(numFrames);
        frameData.resize(numFrames, std::vector<uint8_t>(memPerFrame, 0));
    }

    bool allocate(const std::string& processName, int size);
    void deallocate(const std::string& processName);
    int getExternalFragmentationKB() const;
    void printMemoryLayout(std::ostream& out) const;
    int getActiveProcessCount() const;

    void swapOut(int frameIndex);
    void loadPageToFrame(const std::string& processName, int pageNum, int& outFrameIndex);
    void writeToBackingStore(const std::string& processName, int pageNum, const uint8_t* data, size_t size);
    bool readFromBackingStore(const std::string& processName, int pageNum, uint8_t* buffer, size_t size);

    void handlePageFault(const std::string& processName, int pageNum,
        std::unordered_map<int, PageTableEntry>& pageTable);

    uint8_t* getFrameData(int frameIndex);

    // MemoryManager.h (inside public)
    int getTotalFrames() const { return numFrames; }
    int getUsedFrames() const;
    int getFreeFrames() const;

    int getPageFaults() const { return pageFaults; }
    int getPagesSwappedIn() const { return pagesSwappedIn; }
    int getPagesSwappedOut() const { return pagesSwappedOut; }

    void incrementPageFault() { pageFaults++; }
    void incrementSwappedIn() { pagesSwappedIn++; }
    void incrementSwappedOut() { pagesSwappedOut++; }


private:
    int totalMemory;
    int memPerProc;
    int memPerFrame;
    int numFrames;

    int pageFaults = 0;
    int pagesSwappedIn = 0;
    int pagesSwappedOut = 0;

    std::vector<Frame> frameTable;
    std::vector<std::vector<uint8_t>> frameData;
    std::vector<MemoryBlock> allocations;

    std::vector<MemoryBlock> getSortedAllocations() const;

    bool findFreeFrame(int& frameIndex);
};
