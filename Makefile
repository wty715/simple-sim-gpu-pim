SRCDIR := src
CXX := g++
FILES := $(SRCDIR)/*.cpp
FLAGS := -g

all:
	$(CXX) $(FLAGS) $(FILES) -o main