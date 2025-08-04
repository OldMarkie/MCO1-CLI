#pragma once
#include <stdexcept>
#include <string>

class MemoryAccessViolation : public std::runtime_error {
public:
    uint32_t address;

    MemoryAccessViolation(uint32_t addr)
        : std::runtime_error("Memory access violation at address " + std::to_string(addr)),
        address(addr) {
    }
};
