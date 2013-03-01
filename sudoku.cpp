#include <algorithm>
#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>

#include "sudoku.h"

using namespace std;

ostream &operator<<(ostream &out, const cell &c) {
  stringstream str;
  str << '(' << c.i << ',' << c.j << ')';
  out << str.str();
  return out;
}

size_t hash_value(const cell &c) {
  boost::hash<unsigned int> hasher;
  return hasher(c.i) + hasher(c.j);
}

const string Sudoku::symbols =
  "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz@$";

Sudoku::Sudoku(unsigned int length)
  : length_(length), blocksize_(static_cast<unsigned int>(sqrt(length))) {
  assert(blocksize_ * blocksize_ == length_);
  board_ = new symset*[length];
  for (int i = 0; i < length; i++)
    board_[i] = new symset[length];
  InitializeConflicting();
}

Sudoku::Sudoku(string *board, unsigned int length)
  : length_(length), blocksize_(static_cast<unsigned int>(sqrt(length))) {
  assert(blocksize_ * blocksize_ == length_);
  // Compute alphabet, adding symbols if necessary.
  symset alphabet;
  for (int i = 0; i < length; i++)
    for (int j = 0; j < length; j++)
      if (board[i][j] != unknown)
        alphabet.insert(board[i][j]);
  assert(alphabet.size() <= length);
#ifdef VERBOSE
  vector<string> added;
#endif
  for (int i = 0; i < symbols.size(); i++) {
    if (alphabet.size() == length)
      break;
    const pair<symset::iterator, bool> &res = alphabet.insert(symbols[i]);
    if (res.second) {
#ifdef VERBOSE
      string sym = "" + symbols[i];
      added.push_back(sym);
#endif
    }
  }
  assert(alphabet.size() == length);
#ifdef VERBOSE
  cout << "Added " << boost::algorithm::join(added, ", ")
       << " to alphabet" << endl;
#endif
  // Construct board
  board_ = new symset*[length];
    for (int i = 0; i < length; i++)
      board_[i] = new symset[length];
  for (int i = 0; i < length; i++) {
    for (int j = 0; j < length; j++) {
      if (board[i][j] == unknown)
        board_[i][j] = alphabet;
      else
        board_[i][j].insert(board[i][j]);
    }
  }
  InitializeConflicting();
}

Sudoku Sudoku::ParseFromFile(const string &path) {
  ifstream puzzle (path.c_str(), ifstream::in);
  assert(puzzle.good());
  string line;
  getline(puzzle, line);
  boost::algorithm::trim(line);
  int length = line.size();
  string *board = new string[length];
  board[0] = line;
  for (int i = 1; i < length; i++) {
    string nextline;
    assert(puzzle.good());
    getline(puzzle, nextline);
    boost::algorithm::trim(nextline);
    assert(nextline.size() == length);
    board[i] = nextline;
  }
  Sudoku s (board, length);
  delete[] board;
  return s;
}

void Sudoku::InitializeConflicting() {
  for (int i = 0; i < length_; i++) {
    for (int j = 0; j < length_; j++) {
      vector<cell> con;
      for (int i2 = 0; i2 < length_; i2++) {
        if (i2 != i)
          con.push_back(cell(i2, j));
      }
      for (int j2 = 0; j2 < length_; j2++) {
        if (j2 != j)
          con.push_back(cell(i, j2));
      }
      cell c = corner(i, j);
      ITERBLOCK(i2, j2, *this, c) {
        if (i2 != i || j2 != j)
          con.push_back(cell(i2, j2));
      }
      conflicting_[cell(i, j)] = con;
    }
  }
}

bool Sudoku::Solved() const {
  for (int i = 0; i < length_; i++) {
    for (int j = 0; j < length_; j++) {
      const symset &dom = board_[i][j];
      if (dom.size() != 1)
        return false;
      const vector<cell> &con = conflicting_.find(cell(i, j))->second;
      for (vector<cell>::const_iterator it = con.begin();
           it != con.end(); ++it) {
        if (dom == board_[it->i][it->j])
          return false;
      }
    }
  }
  return true;
}

class smaller_cell {
private:
  const Sudoku &board_;
public:
  smaller_cell(const Sudoku &board) : board_(board) { }
  bool operator()(const cell &c1, const cell &c2) {
    return board_[c1].size() < board_[c2].size();
  }
};

vector<cell> Sudoku::OrderedCells() const {
  vector<cell> cells;
  for (int i = 0; i < length_; i++)
    for (int j = 0; j < length_; j++)
      if (board_[i][j].size() > 1)
        cells.push_back(cell(i, j));
  smaller_cell comp (*this);
  sort(cells.begin(), cells.end(), comp);
  return cells;
}

Sudoku Sudoku::Clone() const {
  Sudoku sudoku (length_);
  for (int i = 0; i < length_; i++)
    for (int j = 0; j < length_; j++)
      sudoku.board_[i][j] = board_[i][j];
  return sudoku;
}

string to_char(const symset &dom) {
  char s = Sudoku::unknown;
  if (dom.size() == 1)
    s = static_cast<char>(*dom.begin());
  return string(1, s);
}

string ncopies(const string &str, int n) {
  stringstream s;
  for (int i = 0; i < n; i++)
    s << str;
  return s.str();
}

string Sudoku::ToString() const {
  unsigned int n = blocksize_;
  string dashes = string(2 * n, '-');
  string delim = " " + dashes + "+" + ncopies(dashes + "-+", n - 2) +
    dashes + "\n";
  stringstream str;
  str << "\n";
  for (int i = 0; i < length_; i++) {
    if (i % n == 0 && i > 0)
      str << delim;
    for (int j = 0; j < length_; j++) {
      if (j % n == 0 && j > 0)
        str << " |";
      str << " " + to_char(board_[i][j]);
    }
    str << "\n";
  }
  return str.str();
}

void Sudoku::PrintPossibilities() const {
  for (int i = 0; i < length_; i++) {
    for (int j = 0; j < length_; j++) {
      cout << cell(i, j) << ": {";
      const symset &dom = board_[i][j];
      for (symset::const_iterator it = dom.begin(); it != dom.end(); ++it)
        cout << static_cast<char>(*it) << ',';
      cout << '}' << endl;
    }
  }
}
