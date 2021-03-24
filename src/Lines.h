#include <list>
#include <string>

typedef std::list<std::string> LinesList;

void shiftLines(std::list<std::string>& lines);
std::list<std::string>& shiftLinesRet(std::list<std::string>& lines);
LinesList& commentLines(LinesList& lines);
void printLines(const std::list<std::string>& lines);
void addLines(std::list<std::string>& lines, const std::list<std::string>& addedLines);