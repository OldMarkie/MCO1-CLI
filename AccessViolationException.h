#pragma once
#include <string>
#include <exception>
#include <sstream>   // <-- Required for std::stringstream
#include <iomanip>   // <-- Required for std::hex

class AccessViolationException : public std::exception {
private:
    std::string processName;
    uint32_t address;

public:
    AccessViolationException(const std::string& procName, uint32_t addr)
        : processName(procName), address(addr) {
    }

    const char* what() const noexcept override {
        return "Access violation: invalid memory access";
    }

    std::string getProcessName() const { return processName; }

    std::string addressHex() const {
        std::stringstream ss;
        ss << "0x" << std::hex << address;
        return ss.str();
    }
};
