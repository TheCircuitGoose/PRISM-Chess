CXX = clang++
CXXFLAGS = -std=c++17 -O3 -march=native -flto -Wall

EXECUTABLES = prism generate mutate tournament prism-tournament

all: $(EXECUTABLES)

prism: prism-default.cpp
	$(CXX) $(CXXFLAGS) -o prism prism-default.cpp

generate: generate.cpp
	$(CXX) $(CXXFLAGS) -o generate generate.cpp

mutate: mutate.cpp
	$(CXX) $(CXXFLAGS) -o mutate mutate.cpp

tournament: tournament.cpp
	$(CXX) $(CXXFLAGS) -o tournament tournament.cpp

prism-tournament: prism-tournament.cpp
	$(CXX) $(CXXFLAGS) -o prism-tournament prism-tournament.cpp

clean:
	rm -f $(EXECUTABLES)

.PHONY: all clean
