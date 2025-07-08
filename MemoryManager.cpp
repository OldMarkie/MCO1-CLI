#include "MemoryManager.h"
#include <algorithm>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <sstream>

MemoryManager::MemoryManager(int totalMemoryBytes, int memPerProcBytes)
    : totalMemory(totalMemoryBytes), memPerProc(memPerProcBytes) {
}

bool MemoryManager::allocate(const std::string& processName) {
    auto blocks = getSortedAllocations();

    // First block case: is there enough space before the first block?
    if (blocks.empty()) {
        if (memPerProc <= totalMemory) {
            allocations.push_back({ 0, memPerProc, processName });
            return true;
        }
        return false;
    }

    if (blocks[0].start >= memPerProc) {
        allocations.push_back({ 0, memPerProc, processName });
        return true;
    }

    // Try to find a gap between blocks
    for (size_t i = 0; i < blocks.size() - 1; ++i) {
        int gapStart = blocks[i].end;
        int gapEnd = blocks[i + 1].start;
        if (gapEnd - gapStart >= memPerProc) {
            allocations.push_back({ gapStart, gapStart + memPerProc, processName });
            return true;
        }
    }

    // Check if space after the last block
    int lastEnd = blocks.back().end;
    if (totalMemory - lastEnd >= memPerProc) {
        allocations.push_back({ lastEnd, lastEnd + memPerProc, processName });
        return true;
    }

    return false; // No space found
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
