// ProcessControlBlock.cpp
#include "ProcessControlBlock.h"
#include <cstdlib>
#include <thread>
#include <chrono>
#include <algorithm>
#include <fstream> // Add at top
#include <iomanip> // Add for time formatting
#include <iostream> 

int safeStoi(const std::string& s, int fallback = 0) {
    try {
        return std::stoi(s);
    }
    catch (...) {
        return fallback;
    }
}

void ProcessControlBlock::generateInstructions(int count) {
    static const std::vector<InstructionType> types = {
        InstructionType::DECLARE,
        InstructionType::ADD,
        InstructionType::SUBTRACT,
        InstructionType::PRINT,
        InstructionType::SLEEP
    };


    for (int i = 0; i < count; ++i) {
        Instruction instr;
        instr.type = types[rand() % types.size()];
        switch (instr.type) {
        case InstructionType::DECLARE:
            instr.args = { "var" + std::to_string(rand() % 100), std::to_string(rand() % 65536) };
            break;
        case InstructionType::ADD:
        case InstructionType::SUBTRACT:
            instr.args = {
                "var" + std::to_string(rand() % 100),
                rand() % 2 ? std::to_string(rand() % 65536) : "var" + std::to_string(rand() % 100),
                rand() % 2 ? std::to_string(rand() % 65536) : "var" + std::to_string(rand() % 100)
            };
            break;
        case InstructionType::PRINT:
            instr.args = { "Hello world from " + name };
            break;
        case InstructionType::SLEEP:
            instr.args = { std::to_string(1 + rand() % 3) };
            break;
        default:
            break;
        }
        instructions.push_back(instr);
    }
}

void ProcessControlBlock::executeNextInstruction(int coreId) {
    if (instructionPointer >= instructions.size()) {
        isFinished = true;
        logs << "\n[Finished] Process " << name << " completed.\n";
        return;
    }

    lastExecutedCore = coreId; // <- record core
    Instruction& current = instructions[instructionPointer];
    execute(current, coreId);
    ++instructionPointer;
}


void ProcessControlBlock::execute(const Instruction& ins, int coreId) {
    if (isFinished) return;

    if (ins.type == InstructionType::DECLARE) {
        const std::string& var = ins.args[0];
        uint16_t value = safeStoi(ins.args[1]);  // safely parse value
        variables[var] = value;
    }
    else if (ins.type == InstructionType::ADD || ins.type == InstructionType::SUBTRACT) {
        const std::string& dest = ins.args[0];
        const std::string& arg1 = ins.args[1];
        const std::string& arg2 = ins.args[2];

        // Auto-declare destination if missing
        if (variables.count(dest) == 0) variables[dest] = 0;

        // Get value for arg1
        uint16_t left = (variables.count(arg1) > 0) ? variables[arg1] : safeStoi(arg1);

        // Auto-declare if it's a new variable used in arg1
        if (isalpha(arg1[0]) && variables.count(arg1) == 0) variables[arg1] = 0;

        // Get value for arg2
        uint16_t right = (variables.count(arg2) > 0) ? variables[arg2] : safeStoi(arg2);

        // Auto-declare if it's a new variable used in arg2
        if (isalpha(arg2[0]) && variables.count(arg2) == 0) variables[arg2] = 0;

        // Apply the operation
        if (ins.type == InstructionType::ADD)
            variables[dest] = std::min<uint32_t>(left + right, UINT16_MAX);
        else
            variables[dest] = (left > right) ? (left - right) : 0;
    }
    else if (ins.type == InstructionType::PRINT) {
        std::string message = ins.args[0];
        logs << "[LOG] " << message << "\n";
    }
    else if (ins.type == InstructionType::SLEEP) {
        int ticks = safeStoi(ins.args[0], 1);  // replaced std::stoi
        std::this_thread::sleep_for(std::chrono::milliseconds(ticks * 50));
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



