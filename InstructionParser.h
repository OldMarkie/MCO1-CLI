#pragma once
#include "ProcessControlBlock.h"
#include <optional>
#include <string>

class InstructionParser {
public:
    static std::optional<Instruction> parse(const std::string& line, const std::string& processName);
};
