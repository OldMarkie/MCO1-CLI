// ProcessControlBlock.h
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <iostream>
#include <cstdint>

enum class InstructionType {
    DECLARE,
    ADD,
    SUBTRACT,
    PRINT,
    SLEEP,
    FOR_START,
    FOR_END
};

struct Instruction {
    InstructionType type;
    std::vector<std::string> args;
};

class ProcessControlBlock {
public:
    ProcessControlBlock() = default;

    // Rule of 5: deleted copy, defaulted move
    ProcessControlBlock(const ProcessControlBlock&) = delete;
    ProcessControlBlock& operator=(const ProcessControlBlock&) = delete;
    ProcessControlBlock(ProcessControlBlock&&) = default;
    ProcessControlBlock& operator=(ProcessControlBlock&&) = default;

    std::string name;
    int instructionPointer = 0;
    bool isFinished = false;

    void generateInstructions(int count);
    void executeNextInstruction();
    std::string getLog() const;

private:
    std::vector<Instruction> instructions;
    std::unordered_map<std::string, uint16_t> variables;
    std::ostringstream logs;

    void execute(const Instruction& ins);
};
