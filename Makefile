SRCDIR := src
CXX := g++
FILES := $(SRCDIR)/*.cpp
FLAGS := -g -DASSERTED

all:
	$(CXX) $(FLAGS) $(FILES) -DRTX2060 -o main2060
	$(CXX) $(FLAGS) $(FILES) -DRTX3090 -o main3090
	$(CXX) $(FLAGS) $(FILES) -DRTX2060 -DENPIM -o main2060-pim
	$(CXX) $(FLAGS) $(FILES) -DRTX3090 -DENPIM -o main3090-pim
	$(CXX) $(FLAGS) $(FILES) -DRTX2060 -DENPIM -DNO_OPT -o main2060-pim-noopt
	$(CXX) $(FLAGS) $(FILES) -DRTX3090 -DENPIM -DNO_OPT -o main3090-pim-noopt