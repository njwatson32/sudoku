#ifndef __SUDOKU_HEADER__
#define __SUDOKU_HEADER__

#include <cmath>
#include <ostream>
#include <string>
#include <boost/functional/hash.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>

class cell {
public:
  unsigned int i;
  unsigned int j;
  
  cell() { }
  cell(unsigned int i, unsigned int j) : i(i), j(j) { }
  
  bool operator==(const cell &other) const {
    return i == other.i && j == other.j;
  }

  bool operator!=(const cell &other) const {
    return i != other.i || j != other.j;
  }
};

std::ostream &operator<<(std::ostream &out, const cell &c);

std::size_t hash_value(const cell &c);

typedef boost::unordered_set<int> symset;
typedef boost::unordered_map<cell, std::vector<cell> > cellmap;
typedef boost::unordered_set<cell> cellset;

#define ITERBLOCK(INAME, JNAME, BOARD, C)                               \
  for (int INAME = C.i; INAME < C.i + (BOARD).blocksize(); INAME++)     \
    for (int JNAME = C.j; JNAME < C.j + (BOARD).blocksize(); JNAME++)   \

class Sudoku {
private:
  symset **board_;
  unsigned int length_;
  unsigned int blocksize_;
  cellmap conflicting_;

public:
  static const char unknown = '*';
  static const std::string symbols;

  Sudoku(unsigned int length);
  Sudoku(std::string *board, unsigned int length);

  // Parses the Sudoku puzzle from a file, assuming no spaces are present
  // and using the above symbol for unknown.
  static Sudoku ParseFromFile(const std::string &path);

  // The side length of the puzzle.
  unsigned int length() const { return length_; }
  // The sidelength of a block.
  unsigned int blocksize() const { return blocksize_; }

  // Gets the remaining possibilities for this cell.
  symset &domain(int i, int j) { return board_[i][j]; }
  const symset &domain(int i, int j) const { return board_[i][j]; }
  symset &domain(const cell &c) { return board_[c.i][c.j]; }
  const symset &domain(const cell &c) const { return board_[c.i][c.j]; }

  // Syntactic sugar for the domain accessor.
  symset *operator[](unsigned int i) { return board_[i]; }
  const symset *operator[](unsigned int i) const { return board_[i]; }
  symset &operator[](const cell &c) { return board_[c.i][c.j]; }
  const symset &operator[](const cell &c) const { return board_[c.i][c.j]; }

  // Gets the corner of the block containing this cell.
  cell corner(int i, int j) const {
    return cell(i - i % blocksize_, j - j % blocksize_);
  }
  cell corner(const cell &c) const {
    return cell(c.i - c.i % blocksize_, c.j - c.j % blocksize_);
  }

  // Gets the list of cells that cannot share the same value with this cell.
  const std::vector<cell> &conflicting(int i, int j) const {
    return conflicting_.find(cell(i, j))->second;
  }
  const std::vector<cell> &conflicting(const cell &c) const {
    return conflicting_.find(c)->second;
  }

  // Sees whether the puzzle is solved.
  bool Solved() const;
  // Gets a list of unsolved cells, sorted in increasing order of
  // remaining possibilities.
  std::vector<cell> OrderedCells() const;
  // Makes a deep copy of the board.
  Sudoku Clone() const;
  // A really nice string representation of the board.
  std::string ToString() const;
  // (Debugging only) Prints the possibilities for each cell.
  void PrintPossibilities() const;

private:
  // Initializes the structure containing conflicting cell information.
  void InitializeConflicting();
};

#endif // __SUDOKU_HEADER__
