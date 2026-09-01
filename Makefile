CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17

programa: main.cpp 
	$(CXX) $(CXXFLAGS) main.cpp -o programa

run: programa
	./programa

clean:
	rm -f programa
