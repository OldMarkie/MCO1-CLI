// ProcessControlBlock.cpp
#include "ProcessControlBlock.h"
#include <cstdlib>
#include <thread>
#include <chrono>
#include <algorithm>
#include <fstream> // Add at top
#include <iomanip> // Add for time formatting
#include <iostream> 

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
            instr.args = {
                InstructionArg("Hello world from " + name + "!")
            };

            break;
        case InstructionType::SLEEP:
            instr.args = {
                InstructionArg(uint16_t(1 + rand() % 3))                           // sleep ticks (1–3)
            };
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
        if (auto msg = std::get_if<std::string>(&ins.args[0])) {
            logs << "[LOG] " << *msg << "\n";
        }
        else {
            logs << "[LOG] [INVALID PRINT ARGUMENT]\n";
        }
    }

    else if (ins.type == InstructionType::SLEEP) {
        int ticks = static_cast<int>(std::get<uint16_t>(ins.args[0]));
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



