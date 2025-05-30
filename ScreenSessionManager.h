#pragma once

#include <iostream>
#include <string>
#include <unordered_map>
#include "ScreenSession.h"
#include "Console.h"

class ScreenSessionManager {
public:
    ScreenSessionManager(std::unordered_map<std::string, ScreenSession>& sessions)
        : sessions_(sessions) {
    }

    void enterScreen(const std::string& screenName) {
        auto it = sessions_.find(screenName);
        if (it == sessions_.end()) {
            std::cerr << "Screen not found.\n";
            return;
        }

        bool inScreen = true;
        const ScreenSession& session = it->second;

        Console::drawScreenSession(screenName, session);

        while (inScreen) {
            std::string screenInput;
            std::cin >> screenInput;

            if (screenInput == "exit") {
                inScreen = false;
                Console::clear();
                Console::drawMainMenu();
            }
            else {
                Console::clear();
                Console::showUnknownCommand();
                Console::drawScreenSession(screenName, session);
            }
        }
    }

private:
    std::unordered_map<std::string, ScreenSession>& sessions_;
};
