#pragma once
#include <regex>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iostream>

static std::string loadFile(std::string filePath) {
	std::ifstream test(filePath);
	if(!test) {
		std::cout << filePath << std::endl;
		printf("Did you forget to pass the filepath? ;D\n");
		throw std::exception();
	}
	std::ifstream inFile(filePath);
	std::string fileAsString;

	if (inFile.is_open()) {
		inFile.clear();
		inFile.seekg(0, std::ios::beg);
		std::string line;
		while (std::getline(inFile, line)) {
			fileAsString.append(line);
			fileAsString.append("\n");
		}
		inFile.close();
	}
	return fileAsString;
}