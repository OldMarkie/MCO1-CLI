#pragma once

#include <string>

struct ScreenSession {
    std::string processName;
    int currentLine;
    int totalLines;
    std::string timestamp;
};
