#include "../AsyncLogger.hpp"
#include <cstdlib>

const int testCount = 1000;
const std::string fileName = "test.log";


std::vector<std::string> lines({
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit.",
    "Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.",
    "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.",
    "Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur.",
    "Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum."
});

void fillLogFile() {
    AsyncLogger logger(fileName);
    for(int i=0; i<testCount; i++) {
        auto& line = lines[i % lines.size()];
        logger<<line;
        if(i % 4) {
            logger<<"\n";
        } else {
            logger<<std::endl;
        }
    }
}

void testLogFile() {
    std::ifstream file(fileName);
    std::string line;
    int count = 0;
    while(std::getline(file, line)) {
        auto &expectedLine = lines[count % lines.size()];
        if(line != expectedLine) {
            std::cerr<<"Line "<<count<<" does not match expected line.\n";
            std::cerr<<"Expected: "<<expectedLine<<"\n";
            std::cerr<<"Got: "<<line<<"\n";
            exit(1);
        }
        count++;
    }
}

int main(){
    fillLogFile();
    testLogFile();
    std::cout<<"Passed!\n";
    return 0;
}
