#pragma once

#include <string>
#include <unordered_map>
#include "ScreenSession.h"

class Console {
public:
    static void drawMainMenu();
    static void drawScreenSession(const std::string& screenName);
    static void showUnknownCommand();
    static void clear();
    static void showArt();
};