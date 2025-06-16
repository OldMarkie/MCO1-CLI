//All of the Front-End Components

#pragma once

#include <iostream>
#include "ScreenSession.h"

class cmdArt {
public:
    static void showArt() {
        std::cout << "\033[1;31m" << R"(
	
    ___                     (_)                    _        _               ___     ___     ___      ___    ___     ___   __   __ 
   | _ \    _ _    ___      | |    ___     __     | |_     (_)      o O O  / __|   / __|   / _ \    | _ \  / __|   | __|  \ \ / / 
   |  _/   | '_|  / _ \    _/ |   / -_)   / _|    |  _|     _      o      | (__    \__ \  | (_) |   |  _/  \__ \   | _|    \ V /  
  _|_|_   _|_|_   \___/   |__/_   \___|   \__|_   _\__|   _(_)_   TS__[O]  \___|   |___/   \___/   _|_|_   |___/   |___|   _|_|_  
_| """ |_|"""""|_|"""""|_|"""""|_|"""""|_|"""""|_|"""""|_|"""""| {======|_|"""""|_|"""""|_|"""""|_| """ |_|"""""|_|"""""|_| """ | 
"-0-0-'"-0-0-'"-0-0-'"-0-0-'"-0-0-'"-0-0-'"-0-0-'"-0-0-'./o--000'"-0-0-'"-0-0-'"-0-0-'"-0-0-'"-0-0-'"-0-0-'"-0-0-' 
)" << "\033[0m";
        std::cout << "\033[32mWelcome, User!\033[0m\n\n";
    }

    static void showMenu() {
        std::cout << "\033[1;33m"
            << "==== Main Menu ====\n"
            << "| -initialize     |\n"
            << "| -screen         |\n"
            << "| -scheduler-start|\n"
            << "| -scheduler-stop |\n"
            << "| -report-util    |\n"
            << "| -clear          |\n"
            << "| -exit           |\n"
            << "===================\n"
            << "\033[0m";
    }

    static void visualClear() {
        system("cls");
    }

    static void screenMenu(const std::string& screenName, const ScreenSession& s) {
        std::cout << "\n==== Screen Console Placeholder ====\n";
        std::cout << "Process Name      : " << s.processName << "\n";
        std::cout << "Instruction Line  : " << s.currentLine << " / " << s.totalLines << "\n";
        std::cout << "Created At        : " << s.timestamp << "\n";
        std::cout << "====================================\n";

        std::cout << "\nType 'exit' to return to main menu.\n> ";

    }

    static void displayNewSesh(const std::string& screenName) {
        std::cout << "Screen session '" << screenName << "' started.\n\n";
    }
};

