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

void ProcessControlBlock::generateInstructions(int remaining, int nesting) {
    static const std::vector<InstructionType> types = {
        InstructionType::DECLARE, InstructionType::ADD,
        InstructionType::SUBTRACT, InstructionType::PRINT, InstructionType::SLEEP
    };

    while (remaining > 0) {
        // Try FOR loop if space allows for FOR_START + FOR_END + body
        if (nesting < 3 && remaining >= 4 && rand() % 1000 < 1) {
            int repeats = 2 + rand() % 3;
            instructions.push_back({ InstructionType::FOR_START, { InstructionArg(uint16_t(repeats)) } });
            --remaining;

            int bodySize = std::min(remaining - 1, 2 + rand() % 3);
            generateInstructions(bodySize, nesting + 1);
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
        }

        instructions.push_back(instr);
        --remaining;
    }
}




void ProcessControlBlock::executeNextInstruction(int coreId) {
    if (instructionPointer >= instructions.size()) {
        isFinished = true;
        logs << "\n[Finished] Process " << name << " completed.\n";
        return;
    }

    Instruction& current = instructions[instructionPointer];

    if (current.type == InstructionType::FOR_START) {
        int repeats = std::get<uint16_t>(current.args[0]);
        forStack.push({ instructionPointer, repeats });
        ++instructionPointer;
        return;
    }

    if (current.type == InstructionType::FOR_END) {
        if (!forStack.empty()) {
            ForContext& top = forStack.top();
            top.remaining--;
            if (top.remaining > 0) {
                instructionPointer = top.startIndex + 1;  // go back inside the loop
            }
            else {
                forStack.pop(); // done looping
                ++instructionPointer;
            }
        }
        else {
            ++instructionPointer; // malformed loop, just move on
        }
        return;
    }

    lastExecutedCore = coreId;
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
                logs << "[" << timeStream.str() << "] "
                    << "[Core " << coreId << "] "
                    << *msg << "\n";
            }
            else {
                logs << "[" << timeStream.str() << "] "
                    << "[Core " << coreId << "] "
                    << "[INVALID PRINT ARGUMENT]\n";
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



