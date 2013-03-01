CC    = g++
FLAGS = -std=c++0x -Wall -Wno-sign-compare -O2 #-g
HDRS  = sudoku.h
SRCS  = sudoku.cpp solver.cpp
OUT   = solver

all: $(OUT)

$(OUT): $(HDRS) $(SRCS)
	$(CC) $(FLAGS) -o $(OUT) $(SRCS)

clean:
	rm $(OUT)