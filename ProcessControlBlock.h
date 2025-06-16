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

    ProcessControlBlock(const ProcessControlBlock&) = delete;
    ProcessControlBlock& operator=(const ProcessControlBlock&) = delete;
    ProcessControlBlock(ProcessControlBlock&&) = default;
    ProcessControlBlock& operator=(ProcessControlBlock&&) = default;

    std::string name;
    int instructionPointer = 0;
    bool isFinished = false;

    void generateInstructions(int count);
    void executeNextInstruction(int coreID);
    std::string getLog() const;

    void addInstruction(const Instruction& instr);
    void execute(const Instruction& ins, int coreId);

    std::string getStartTime() const;
    int totalInstructions() const;
    int lastExecutedCore = -1; // updated during execution

    std::string startTime;



private:
    std::vector<Instruction> instructions;
    std::unordered_map<std::string, uint16_t> variables;
    std::ostringstream logs;

   

};
