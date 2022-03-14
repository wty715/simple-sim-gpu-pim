SRCDIR := src
CXX := g++
FILES := $(SRCDIR)/*.cpp
FLAGS := -g -DASSERTED

all:
	$(CXX) $(FLAGS) $(FILES) -o main
	$(CXX) $(FLAGS) $(FILES) -DENPIM -o main-pim
	$(CXX) $(FLAGS) $(FILES) -DENPIM -DOPT_FSM -o main-pim-fsm
	$(CXX) $(FLAGS) $(FILES) -DENPIM -DOPT_FSM -DOPT_INTRA -o main-pim-fsm-intra

debug:
	$(CXX) $(FLAGS) $(FILES) -DENPIM -DOPT_INTRA -DOPT_FSM -DDEBUGGING -o main-dbg

req:
	$(CXX) $(FLAGS) $(FILES) -DREQUESTED -o main-req

clean:
	rm main main-pim main-pim-fsm main-pim-fsm-intra main-dbg