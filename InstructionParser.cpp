#include "InstructionParser.h"
#include <sstream>
#include <regex>

std::optional<Instruction> InstructionParser::parse(const std::string& line, const std::string& processName) {
    std::istringstream ss(line);
    std::string type;
    ss >> type;

    Instruction instr;

    if (type == "DECLARE") {
        std::string var;
        uint16_t val;
        ss >> var >> val;
        instr.type = InstructionType::DECLARE;
        instr.args = { var, val };
        return instr;
    }

    if (type == "ADD" || type == "SUBTRACT") {
        std::string dest, op1, op2;
        ss >> dest >> op1 >> op2;
        instr.type = (type == "ADD") ? InstructionType::ADD : InstructionType::SUBTRACT;

        auto arg1 = std::isdigit(op1[0]) ? InstructionArg(uint16_t(std::stoi(op1))) : InstructionArg(op1);
        auto arg2 = std::isdigit(op2[0]) ? InstructionArg(uint16_t(std::stoi(op2))) : InstructionArg(op2);
        instr.args = { dest, arg1, arg2 };
        return instr;
    }

    if (type == "PRINT") {
        std::string raw;
        std::getline(ss, raw);
        std::smatch match;
        std::regex rgx(R"regex("([^"]*)"\s*\+\s*(\w+))regex");

        if (std::regex_search(raw, match, rgx)) {
            std::string base = match[1];
            std::string var = match[2];
            instr.type = InstructionType::PRINT;
            instr.args = { InstructionArg(base + var) };  // Simplified
        }
        else {
            instr.type = InstructionType::PRINT;
            instr.args = { InstructionArg("Hello world from " + processName + "!") };
        }

        return instr;
    }

    if (type == "SLEEP") {
        int ticks;
        ss >> ticks;
        instr.type = InstructionType::SLEEP;
        instr.args = { uint16_t(ticks) };
        return instr;
    }

    if (type == "READ") {
        std::string var, addrStr;
        ss >> var >> addrStr;
        int addr = std::stoi(addrStr, nullptr, 16);
        instr.type = InstructionType::READ;
        instr.args = { var, uint16_t(addr) };
        return instr;
    }

    if (type == "WRITE") {
        std::string addrStr;
        uint16_t val;
        ss >> addrStr >> val;
        int addr = std::stoi(addrStr, nullptr, 16);
        instr.type = InstructionType::WRITE;
        instr.args = { uint16_t(addr), val };
        return instr;
    }

    return std::nullopt;  // invalid
}
