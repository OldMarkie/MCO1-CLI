// ProcessControlBlock.cpp
#include "ProcessControlBlock.h"
#include <cstdlib>
#include <thread>
#include <chrono>
#include <algorithm>
#include <fstream> // Add at top
#include <iomanip> // Add for time formatting
#include <iostream> 

#include "MemoryManager.h"
#include "Config.h"
#include "PageFaultException.h"
#include "MemoryAccessViolation.h"
extern MemoryManager* memoryManager;
extern Config globalConfig;


uint16_t resolveValue(const InstructionArg& arg, std::unordered_map<std::string, uint16_t>& variables) {
    if (auto val = std::get_if<uint16_t>(&arg)) return *val;
    if (auto var = std::get_if<std::string>(&arg)) {
        if (variables.count(*var) == 0) variables[*var] = 0;
        return variables[*var];
    }
    return 0;
}


int safeStoi(const std::string& s, int fallback = 0) {
    try {
        return std::stoi(s);
    }
    catch (...) {
        return fallback;
    }
}

void ProcessControlBlock::generateInstructions(int remaining, int nesting, int maxAddressableBytes) {
    static const std::vector<InstructionType> types = {
    InstructionType::DECLARE, InstructionType::ADD,
    InstructionType::SUBTRACT, InstructionType::PRINT,
    InstructionType::SLEEP, InstructionType::READ,
    InstructionType::WRITE
    };


    while (remaining > 0) {
        // Try FOR loop if space allows for FOR_START + FOR_END + body
        if (nesting < 3 && remaining >= 4 && rand() % 100000000000000000 < 1) {
            int repeats = 2 + rand() % 3;
            instructions.push_back({ InstructionType::FOR_START, { InstructionArg(uint16_t(repeats)) } });
            --remaining;

            int bodySize = std::min(remaining - 1, 2 + rand() % 3);
            generateInstructions(bodySize, nesting + 1, maxAddressableBytes);
            remaining -= bodySize;

            instructions.push_back({ InstructionType::FOR_END, {} });
            --remaining;
            continue;
        }

        Instruction instr;
        instr.type = types[rand() % types.size()];
        switch (instr.type) {
        case InstructionType::DECLARE:
            instr.args = {
                InstructionArg("var" + std::to_string(rand() % 100)),
                InstructionArg(uint16_t(rand() % 65536))
            };
            break;
        case InstructionType::ADD:
        case InstructionType::SUBTRACT:
            instr.args = {
                InstructionArg("var" + std::to_string(rand() % 100)),
                rand() % 2 ? InstructionArg(uint16_t(rand() % 65536)) : InstructionArg("var" + std::to_string(rand() % 100)),
                rand() % 2 ? InstructionArg(uint16_t(rand() % 65536)) : InstructionArg("var" + std::to_string(rand() % 100))
            };
            break;
        case InstructionType::PRINT:
            instr.args = { InstructionArg("Hello world from " + name + "!") };
            break;
        case InstructionType::SLEEP:
            instr.args = { InstructionArg(uint16_t(1 + rand() % 5)) };
            break;
        case InstructionType::READ: {
            uint32_t safeAddr = rand() % std::max(16, maxAddressableBytes);  // Always >16 to avoid 0
            instr.args = {
            InstructionArg("var" + std::to_string(rand() % 100)),
            InstructionArg(static_cast<uint32_t>(safeAddr))  // safer and preferred
            };
            break;
        }
        case InstructionType::WRITE: {
            uint32_t safeAddr = rand() % std::max(16, maxAddressableBytes);
            instr.args = {
            InstructionArg(static_cast<uint32_t>(safeAddr)),
            InstructionArg(static_cast<uint16_t>(rand() % 65536))
            };
            break;
        }


        }


        instructions.push_back(instr);
        --remaining;
    }
}




bool ProcessControlBlock::executeNextInstruction(int coreId) {
    if (instructionPointer >= instructions.size()) {
        isFinished = true;
        logs << "\n[Finished] Process " << name << " completed.\n";
        return true;
    }

    Instruction& current = instructions[instructionPointer];

    // FOR loop start
    if (current.type == InstructionType::FOR_START) {
        int repeats = std::get<uint16_t>(current.args[0]);
        forStack.push({ instructionPointer, repeats });
        ++instructionPointer;
        return true;
    }

    // FOR loop end
    if (current.type == InstructionType::FOR_END) {
        if (!forStack.empty()) {
            ForContext& top = forStack.top();
            top.remaining--;
            if (top.remaining > 0) {
                instructionPointer = top.startIndex + 1;  // jump back
            }
            else {
                forStack.pop();  // exit loop
                ++instructionPointer;
            }
        }
        else {
            ++instructionPointer;  // malformed loop fallback
        }
        return true;
    }

    try {
        memoryManager->ensurePagesPresent(name, current, variables);

        lastExecutedCore = coreId;
        execute(current, coreId);
        ++instructionPointer;

        if (instructionPointer >= instructions.size()) {
            isFinished = true;
            logs << "\n[Finished] Process " << name << " completed.\n";
        }

        return true;  // success
    }
    catch (const PageFaultException& pf) {
        memoryManager->handlePageFault(pf.processName, pf.address);
        return false;  // retry later
    }
    catch (const MemoryAccessViolation& mv) {
        logs << "[Core " << coreId << "] MEMORY ACCESS VIOLATION at " << mv.address << ": Access violation\n";
        isFinished = true;
        return true;  // mark as complete
    }
}





void ProcessControlBlock::execute(const Instruction& ins, int coreId) {
    if (isFinished) return;

    if (ins.type == InstructionType::DECLARE) {
        const std::string& var = std::get<std::string>(ins.args[0]);
        uint16_t value = std::get<uint16_t>(ins.args[1]);
        variables[var] = value;
    }
    else if (ins.type == InstructionType::ADD || ins.type == InstructionType::SUBTRACT) {
        const std::string& dest = std::get<std::string>(ins.args[0]);
        uint16_t left = resolveValue(ins.args[1], variables);
        uint16_t right = resolveValue(ins.args[2], variables);

        if (variables.count(dest) == 0) variables[dest] = 0;

        if (ins.type == InstructionType::ADD)
            variables[dest] = std::min<uint32_t>(left + right, UINT16_MAX);
        else
            variables[dest] = (left > right) ? (left - right) : 0;
    }
    else if (ins.type == InstructionType::PRINT) {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm timeinfo;
#ifdef _WIN32
        localtime_s(&timeinfo, &now_c);
#else
        localtime_r(&now_c, &timeinfo);
#endif
        std::ostringstream timeStream;
        timeStream << std::put_time(&timeinfo, "%I:%M:%S%p");

        if (auto msg = std::get_if<std::string>(&ins.args[0])) {
            logs << "[" << timeStream.str() << "] [Core " << coreId << "] " << *msg << "\n";
        }
        else {
            logs << "[" << timeStream.str() << "] [Core " << coreId << "] [INVALID PRINT ARGUMENT]\n";
        }
    }
    else if (ins.type == InstructionType::SLEEP) {
        int ticks = static_cast<int>(std::get<uint16_t>(ins.args[0]));
        std::this_thread::sleep_for(std::chrono::milliseconds(ticks * 50));
    }
    else if (ins.type == InstructionType::READ) {
        const std::string& varName = std::get<std::string>(ins.args[0]);
        uint32_t addr = 0;

        try {
            if (std::holds_alternative<uint32_t>(ins.args[1])) {
                addr = std::get<uint32_t>(ins.args[1]);
            }
            else if (std::holds_alternative<std::string>(ins.args[1])) {
                addr = std::stoul(std::get<std::string>(ins.args[1]), nullptr, 16);
            }
            else {
                throw std::runtime_error("Invalid address type in READ");
            }

            // trigger fault if needed
            memoryManager->readMemory(name, addr);
            uint16_t val = memoryManager->readMemory(name, addr);

            if (variables.size() < 32) {
                variables[varName] = val;
            }

            logs << "[Core " << coreId << "] READ 0x" << std::hex << addr << std::dec
                << " = " << val << " into " << varName << "\n";
        }
        catch (const std::exception& e) {
            logs << "[Core " << coreId << "] MEMORY ACCESS VIOLATION at 0x"
                << std::hex << addr << std::dec << ": " << e.what() << "\n";
            isFinished = true;
            violationTime = currentTimeString();
            std::ostringstream oss;
            oss << "0x" << std::hex << addr;
            violationAddr = oss.str();
        }
    }
    else if (ins.type == InstructionType::WRITE) {
        uint16_t value = resolveValue(ins.args[1], variables);
        uint32_t addr = 0;

        try {
            if (std::holds_alternative<uint32_t>(ins.args[0])) {
                addr = std::get<uint32_t>(ins.args[0]);
            }
            else if (std::holds_alternative<std::string>(ins.args[0])) {
                addr = std::stoul(std::get<std::string>(ins.args[0]), nullptr, 16);
            }
            else {
                throw std::runtime_error("Invalid address type in WRITE");
            }

            memoryManager->writeMemory(name, addr, value); // triggers fault if needed
            memoryManager->writeMemory(name, addr, value); // retry actual write

            logs << "[Core " << coreId << "] WRITE " << value << " to 0x"
                << std::hex << addr << std::dec << "\n";
        }
        catch (const std::exception& e) {
            logs << "[Core " << coreId << "] MEMORY ACCESS VIOLATION at 0x"
                << std::hex << addr << std::dec << ": " << e.what() << "\n";
            isFinished = true;
            violationTime = currentTimeString();
            std::ostringstream oss;
            oss << "0x" << std::hex << addr;
            violationAddr = oss.str();
        }
    }
}


std::string ProcessControlBlock::getLog() const {
    return logs.str();
}

void ProcessControlBlock::addInstruction(const Instruction& instr) {
    instructions.push_back(instr);
}

std::string ProcessControlBlock::getStartTime() const {
    return startTime;
}

int ProcessControlBlock::totalInstructions() const {
    return static_cast<int>(instructions.size());
}

std::string ProcessControlBlock::currentTimeString() const {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm timeinfo;
#ifdef _WIN32
    localtime_s(&timeinfo, &now_c);
#else
    localtime_r(&now_c, &timeinfo);
#endif
    std::ostringstream oss;
    oss << std::put_time(&timeinfo, "%I:%M:%S%p");
    return oss.str();
}




