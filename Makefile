SRCDIR := src
CXX := g++
FILES := $(SRCDIR)/*.cpp
FLAGS := -g -DASSERTED

all:
	$(CXX) $(FLAGS) $(FILES) -o main
	$(CXX) $(FLAGS) $(FILES) -DENPIM -o main-pim
	$(CXX) $(FLAGS) $(FILES) -DENPIM -DNO_OPT -o main-pim-noopt