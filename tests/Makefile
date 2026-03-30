CC = g++
CC_FLAGS = -O2 -mtune=native -Wall

%: %.cpp ../*hpp
	$(CC) $(CXXFLAGS) $< -o $@

all: testRingBuffer testLogger
	./testRingBuffer
	./testLogger

clean:
	rm testRingBuffer testLogger test.log
