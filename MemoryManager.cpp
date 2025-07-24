#include "MemoryManager.h"
#include <algorithm>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <sstream>
#include <fstream>
#include <filesystem>
#include "ProcessControlBlock.h"

const std::string BACKING_STORE_FILE = "csopesy-backing-store.txt";

MemoryManager::MemoryManager(int totalMemoryBytes, int memPerProcBytes)
    : totalMemory(totalMemoryBytes), memPerProc(memPerProcBytes) {
}

void MemoryManager::writeToBackingStore(const std::string& processName, int pageNum, const uint8_t* data, size_t size) {
    std::ofstream out(BACKING_STORE_FILE, std::ios::app | std::ios::binary);
    out << processName << ":" << pageNum << ":";
    for (size_t i = 0; i < size; ++i) {
        out << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(data[i]);
    }
    out << "\n";
    out.close();
}

bool MemoryManager::readFromBackingStore(const std::string& processName, int pageNum, uint8_t* buffer, size_t size) {
    std::ifstream in(BACKING_STORE_FILE);
    if (!in.is_open()) return false;

    std::string line;
    std::string targetPrefix = processName + ":" + std::to_string(pageNum) + ":";

    while (std::getline(in, line)) {
        if (line.rfind(targetPrefix, 0) == 0) {
            std::string hexData = line.substr(targetPrefix.length());
            for (size_t i = 0; i < size && i * 2 < hexData.size(); ++i) {
                std::string byteStr = hexData.substr(i * 2, 2);
                buffer[i] = static_cast<uint8_t>(std::stoi(byteStr, nullptr, 16));
            }
            return true;
        }
    }

    return false;
}

bool MemoryManager::allocate(const std::string& processName, int size) {
    auto blocks = getSortedAllocations();

    if (blocks.empty()) {
        if (size <= totalMemory) {
            allocations.push_back({ 0, size, processName });
            return true;
        }
        return false;
    }

    if (blocks[0].start >= size) {
        allocations.push_back({ 0, size, processName });
        return true;
    }

    for (size_t i = 0; i < blocks.size() - 1; ++i) {
        int gapStart = blocks[i].end;
        int gapEnd = blocks[i + 1].start;
        if (gapEnd - gapStart >= size) {
            allocations.push_back({ gapStart, gapStart + size, processName });
            return true;
        }
    }

    int lastEnd = blocks.back().end;
    if (totalMemory - lastEnd >= size) {
        allocations.push_back({ lastEnd, lastEnd + size, processName });
        return true;
    }

    return false;
}


void MemoryManager::deallocate(const std::string& processName) {
    allocations.erase(
        std::remove_if(allocations.begin(), allocations.end(),
            [&](const MemoryBlock& b) {
                return b.processName == processName;
            }),
        allocations.end());
}

std::vector<MemoryBlock> MemoryManager::getSortedAllocations() const {
    auto sorted = allocations;
    std::sort(sorted.begin(), sorted.end(), [](const MemoryBlock& a, const MemoryBlock& b) {
        return a.start < b.start;
        });
    return sorted;
}

int MemoryManager::getExternalFragmentationKB() const {
    auto sorted = getSortedAllocations();
    int totalFragmentation = 0;

    if (sorted.empty()) return totalMemory / 1024;

    // Fragment before first block
    if (sorted[0].start > 0) {
        totalFragmentation += sorted[0].start;
    }

    // Gaps between blocks
    for (size_t i = 0; i < sorted.size() - 1; ++i) {
        int gap = sorted[i + 1].start - sorted[i].end;
        totalFragmentation += gap;
    }

    // Fragment after last block
    int lastEnd = sorted.back().end;
    if (lastEnd < totalMemory) {
        totalFragmentation += (totalMemory - lastEnd);
    }

    return totalFragmentation / 1024;
}

int MemoryManager::getActiveProcessCount() const {
    return static_cast<int>(allocations.size());
}

void MemoryManager::printMemoryLayout(std::ostream& out) const {
    using namespace std::chrono;

    // Print timestamp
    auto now = system_clock::now();
    std::time_t now_time = system_clock::to_time_t(now);
    std::tm timeinfo;
#ifdef _WIN32
    localtime_s(&timeinfo, &now_time);
#else
    localtime_r(&now_time, &timeinfo);
#endif
    std::ostringstream ts;
    ts << std::put_time(&timeinfo, "%m/%d/%Y %I:%M:%S%p");

    out << "Timestamp: (" << ts.str() << ")\n";
    out << "Number of processes in memory: " << getActiveProcessCount() << "\n";
    out << "Total external fragmentation in KB: " << getExternalFragmentationKB() << "\n\n";

    out << "---end--- = " << totalMemory << "\n";

    auto sorted = getSortedAllocations();
    for (auto it = sorted.rbegin(); it != sorted.rend(); ++it) {
        out << it->end << "\n";
        out << it->processName << "\n";
        out << it->start << "\n";
    }

    out << "---start--- = 0\n";
    out << std::endl;
}

bool MemoryManager::findFreeFrame(int& frameIndex) {
    for (int i = 0; i < numFrames; ++i) {
        if (!frameTable[i].occupied) {
            frameIndex = i;
            return true;
        }
    }
    return false;
}

void MemoryManager::swapOut(int frameIndex) {
    Frame& frame = frameTable[frameIndex];

    // Write current data to backing store if dirty
    writeToBackingStore(frame.ownerProcess, frame.pageNumber,
        frameData[frameIndex].data(), memPerFrame);

    // Clear the frame info
    frame.occupied = false;
    frame.ownerProcess = "";
    frame.pageNumber = -1;

    incrementSwappedOut();

}

void MemoryManager::handlePageFault(const std::string& processName, int pageNum,
    std::unordered_map<int, PageTableEntry>& pageTable) {

    int frameIndex = -1;

    if (!findFreeFrame(frameIndex)) {
        // No free frame — pick victim (FIFO strategy: evict 0th found occupied)
        for (int i = 0; i < numFrames; ++i) {
            if (frameTable[i].occupied) {
                swapOut(i);
                frameIndex = i;
                break;
            }
        }
    }

    // Load page data into the selected frame
    bool loaded = readFromBackingStore(processName, pageNum,
        frameData[frameIndex].data(), memPerFrame);

    if (!loaded) {
        // First time use, fill with zero
        std::fill(frameData[frameIndex].begin(), frameData[frameIndex].end(), 0);
    }

    // Update frame table
    frameTable[frameIndex].occupied = true;
    frameTable[frameIndex].ownerProcess = processName;
    frameTable[frameIndex].pageNumber = pageNum;

    // Update page table of process
    PageTableEntry& entry = pageTable[pageNum];
    entry.frameIndex = frameIndex;
    entry.valid = true;
    entry.dirty = false;

    // Stats (optional): pagesPagedIn++
    incrementPageFault();
    if (!loaded) {
        std::fill(frameData[frameIndex].begin(), frameData[frameIndex].end(), 0);
    }
    else {
        incrementSwappedIn();
    }

}


uint8_t* MemoryManager::getFrameData(int frameIndex) {
    return frameData[frameIndex].data();
}

int MemoryManager::getUsedFrames() const {
    return std::count_if(frameTable.begin(), frameTable.end(),
        [](const Frame& f) { return f.occupied; });
}

int MemoryManager::getFreeFrames() const {
    return numFrames - getUsedFrames();
}


