// ProcessControlBlock.cpp
#include "ProcessControlBlock.h"
#include <cstdlib>
#include <thread>
#include <chrono>
#include <algorithm>
#include <fstream> // Add at top
#include <iomanip> // Add for time formatting

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
            instr.args = { "var" + std::to_string(rand() % 10), std::to_string(rand() % 100) };
            break;
        case InstructionType::ADD:
        case InstructionType::SUBTRACT:
            instr.args = {
                "var" + std::to_string(rand() % 10),
                rand() % 2 ? std::to_string(rand() % 10) : "var" + std::to_string(rand() % 10),
                rand() % 2 ? std::to_string(rand() % 10) : "var" + std::to_string(rand() % 10)
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
        uint16_t value = std::stoi(ins.args[1]);
        variables[var] = value;
    }
    else if (ins.type == InstructionType::ADD || ins.type == InstructionType::SUBTRACT) {
        const std::string& dest = ins.args[0];
        uint16_t left = 0, right = 0;

        if (variables.count(ins.args[1])) left = variables[ins.args[1]];
        else left = std::stoi(ins.args[1]);

        if (variables.count(ins.args[2])) right = variables[ins.args[2]];
        else right = std::stoi(ins.args[2]);

        if (ins.type == InstructionType::ADD)
            variables[dest] = std::min<uint32_t>(left + right, UINT16_MAX);
        else
            variables[dest] = (left > right) ? (left - right) : 0;
    }
    else if (ins.type == InstructionType::PRINT) {
        std::string message = ins.args[0];
        logs << "[LOG] " << message << "\n";

#ifndef DISABLE_PRINT_LOG
        std::string filename = name + ".txt";
        std::ifstream checkFile(filename);
        bool isNew = checkFile.peek() == std::ifstream::traits_type::eof(); // check if empty
        checkFile.close();

        std::ofstream outFile(filename, std::ios::app);
        if (isNew) {
            outFile << "Process name: " << name << "\n\nLogs:\n\n";
        }
        auto now = std::chrono::system_clock::now();
        auto now_c = std::chrono::system_clock::to_time_t(now);
#ifdef _WIN32
        std::tm timeinfo;
        localtime_s(&timeinfo, &now_c);
#else
        std::tm timeinfo;
        localtime_r(&now_c, &timeinfo);
#endif
        outFile << "[" << std::put_time(&timeinfo, "%H:%M:%S") << "] ";
        outFile << "[Core " << coreId << "] ";
        outFile << message << "\n";
#endif
    }
    else if (ins.type == InstructionType::SLEEP) {
        int ticks = std::stoi(ins.args[0]);
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

