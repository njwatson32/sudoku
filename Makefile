all: sudoku

sudoku: sudoku.h sudoku.cpp solver.cpp
	g++ -g -Wall -Wno-sign-compare -std=c++0x -o solver sudoku.cpp solver.cpp