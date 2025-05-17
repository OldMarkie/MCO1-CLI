// ProcessManager.cpp
#include "ProcessManager.h"
#include "InstructionSet.h"
#include <iostream>

Process::Process(const std::string& name, int id) : name(name), id(id), finished(false) {}

void Process::execute() {
    while (instructionPtr < instructions.size()) {
        instructions[instructionPtr]->run(*this);
        ++instructionPtr;
    }
    finished = true;
}

std::string Process::getName() const { return name; }
bool Process::isFinished() const { return finished; }
void Process::addInstruction(std::shared_ptr<Instruction> inst) {
    instructions.push_back(inst);
}
std::unordered_map<std::string, uint16_t>& Process::getVariables() { return variables; }
void Process::log(const std::string& message) { std::cout << "[" << name << "] " << message << std::endl; }

std::shared_ptr<Process> ProcessManager::createProcess(const std::string& name) {
    auto proc = std::make_shared<Process>(name, nextId++);
    processes[name] = proc;
    return proc;
}

std::shared_ptr<Process> ProcessManager::getProcessByName(const std::string& name) {
    return processes.count(name) ? processes[name] : nullptr;
}

std::vector<std::shared_ptr<Process>> ProcessManager::listProcesses() {
    std::vector<std::shared_ptr<Process>> list;
    for (auto& [_, p] : processes) list.push_back(p);
    return list;
}