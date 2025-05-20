#include <iostream>  
#include <string>  
#include "menuFunc.h"

using namespace std;

std::map<std::string, menuFunc::ScreenSession> menuFunc::sessions;

int main() {

	menuFunc::mainMenu();

	return 0;
}