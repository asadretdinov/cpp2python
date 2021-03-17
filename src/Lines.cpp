#include "Lines.h"
#include <iostream>

void shiftLines(std::list<std::string>& lines) {
	for (auto& s : lines) s = std::string("\t") + s;
}

std::list<std::string>& shiftLinesRet(std::list<std::string>& lines) {
	shiftLines(lines);
	return lines;
}

void printLines(const std::list<std::string>& lines) {
	for (const auto& s : lines) {
		std::cout << s << "\n";
	}
}

void addLines(std::list<std::string>& lines, const std::list<std::string>& addedLines) {
	lines.insert(lines.end(), addedLines.begin(), addedLines.end());
}