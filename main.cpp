#include <iostream>  
#include <string>  
#include "menuFunc.cpp"

using namespace std;

std::map<std::string, ScreenSession> menuFunc::sessions;

int main() {

	menuFunc::mainMenu();

	return 0;
}