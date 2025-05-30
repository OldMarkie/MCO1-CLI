#pragma once

#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

struct ScreenSession {
    std::string processName;
    int currentLine;
    int totalLines;
    std::string timestamp;
};

// Utility function to create a new ScreenSession
inline ScreenSession createNewScreenSession(const std::string& screenName) {
    auto now = std::chrono::system_clock::now();
    std::time_t timeNow = std::chrono::system_clock::to_time_t(now);
    std::tm localTime;

#ifdef _WIN32
    localtime_s(&localTime, &timeNow);
#else
    localtime_r(&timeNow, &localTime);
#endif

    std::ostringstream timeStream;
    timeStream << std::put_time(&localTime, "%m/%d/%Y, %I:%M:%S %p");

    return ScreenSession{
        screenName,
        1,   // currentLine
        10,  // totalLines
        timeStream.str()
    };
}
