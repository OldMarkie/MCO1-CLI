// PageFaultException.h
#pragma once
#include <stdexcept>
#include <string>

class PageFaultException : public std::runtime_error {
public:
    std::string processName;
    uint32_t address;

    PageFaultException(const std::string& proc, uint32_t addr)
        : std::runtime_error("Page fault at address " + std::to_string(addr)),
        processName(proc), address(addr) {
    }
};
