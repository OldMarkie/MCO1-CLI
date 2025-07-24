// ProcessControlBlock.h
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <iostream>
#include <cstdint>
#include <variant>
#include <stack>

enum class InstructionType {
    DECLARE,
    ADD,
    SUBTRACT,
    PRINT,
    SLEEP,
    FOR_START,
    FOR_END,
    READ,
    WRITE
};

struct PageTableEntry {
    int frameIndex = -1;        // -1 if not in physical memory
    bool valid = false;         // is the page currently loaded
    bool dirty = false;         // if written
};


using InstructionArg = std::variant<std::string, uint16_t, uint8_t>;

struct ForContext {
    int startIndex;
    int remaining;
};



struct Instruction {
    InstructionType type;
    std::vector<InstructionArg> args;
};

class ProcessControlBlock {
public:
    ProcessControlBlock() = default;

    std::unordered_map<int, PageTableEntry> pageTable;
    int numPages = 0;
    int pageSize = 0;


    ProcessControlBlock(const ProcessControlBlock&) = delete;
    ProcessControlBlock& operator=(const ProcessControlBlock&) = delete;
    ProcessControlBlock(ProcessControlBlock&&) = default;
    ProcessControlBlock& operator=(ProcessControlBlock&&) = default;

    std::string name;
    int instructionPointer = 0;
    bool isFinished = false;

    void generateInstructions(int count, int nesting);
    void executeNextInstruction(int coreID);
    std::string getLog() const;

    void addInstruction(const Instruction& instr);
    void execute(const Instruction& ins, int coreId);

    std::string getStartTime() const;
    int totalInstructions() const;
    int lastExecutedCore = -1; // updated during execution

    std::string startTime;

    bool isValidAddress(uint32_t address) const {
        return address >= allocatedStart && address < allocatedStart + allocatedSize;
    }

    std::unordered_map<uint32_t, uint16_t> processMemory; // address -> value
    int allocatedStart = 0;
    int allocatedSize = 0;



private:
    std::vector<Instruction> instructions;
    std::unordered_map<std::string, uint16_t> variables;
    std::ostringstream logs;
    std::stack<ForContext> forStack;
   


   

};
