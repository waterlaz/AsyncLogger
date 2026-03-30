#include "../AsyncLogger.hpp"
#include <cstdlib>

std::vector<int> testData(1000);
std::vector<int> copyData;
bool isRunning = true;

RingBuffer<int> buffer(15);

void producer() {
    for(auto && t: testData) {
        t = std::rand() % 100;
        buffer.push(t);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    isRunning = false;
}

void consumer() {
    while(isRunning) {
        int t;
        while(buffer.pop(t)) {
            copyData.push_back(t);
        }
    }
}

int main(){
    std::thread consumerThread(consumer);
    std::thread producerThread(producer);
    consumerThread.join();
    producerThread.join();
    for(size_t i=0; i<testData.size(); i++) {
        if(testData[i] != copyData[i]) {
            std::cout << "Error at index " << i << ": " << testData[i] << " != " << copyData[i] << std::endl;
            return 1;
        }
    }
    std::cout<<"Passed!\n";
}
