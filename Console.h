#pragma once

#include <string>
#include <unordered_map>
#include "ScreenSession.h"

class Console {
public:
    static void drawMainMenu(); // now includes menuFunc logic
    static void drawScreenSession(const std::string& screenName); // now includes ScreenSessionManager logic
    static void showUnknownCommand();
    static void clear();
    static void showArt();
};
