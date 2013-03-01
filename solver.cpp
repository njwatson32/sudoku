#include <boost/unordered_set.hpp>
#include <cstring>
#include <iostream>
#include <vector>

#include "sudoku.h"

using namespace std;
using namespace boost;

typedef unordered_map<int, cellset> symmap;

template <typename T>
struct reversemap {
  typedef unordered_map<T, symmap> t;
};

template <typename InputIterator>
void print_container(InputIterator first, InputIterator last) {
  cout << '{';
  for (; first != last; ++first)
    cout << *first << ',';
  cout << '}' << endl;
}

enum grouptype {
  NONE,
  ROW,
  COL,
  BLK
};

bool ArcReduce(Sudoku &board, const cell &c, bool *error) {
  bool change = false;
  symset &dom = board[c];
  const vector<cell> &conf = board.conflicting(c);
  for (vector<cell>::const_iterator it = conf.begin();
       it != conf.end(); ++it) {
    const symset &dom2 = board[*it];
    if (dom2.size() == 1) {
      if (dom.erase(*dom2.begin()) > 0)
        change = true;
    }
  }
  if (dom.size() == 0)
    *error = true;
  /*
  for (symset::const_iterator it = dom.begin(); it != dom.end(); ++it) {
    const vector<cell> &con = board.conflicting(c);
    for (vector<cell>::const_iterator it2 = con.begin();
         it2 != con.end() ++it2) {
      const symset &dom2 = board[*it2];
      if (dom2.size() == 1 && dom2.find(*it) != dom2.end()) {
        //change->insert(*it2);
        dom.erase(*it);
        change = true;
      }
    }
  }
  */
  return change;
}

bool AC3(Sudoku &board) {
  cellset todo;
  for (int i = 0; i < board.length(); i++)
    for (int j = 0; j < board.length(); j++)
      todo.insert(cell(i, j));
  while (!todo.empty()) {
    cell c = *todo.begin();
    todo.erase(todo.begin());
    bool error = false;
    bool change = ArcReduce(board, c, &error);
    if (error)
      return false;
    if (change) {
      const vector<cell> &conf = board.conflicting(c);
      todo.insert(conf.begin(), conf.end());
    }
  }
  return true;
}

// TODO
bool Swordfish(Sudoku &board) {
  return false;
}

struct groups {
  unsigned int row;
  unsigned int col;
  cell blk;
};

groups SameGroup(const Sudoku &board, const cellset &cells,
                 bool *row, bool *col, bool *blk) {
  groups g;
  if (cells.size() == 0) {
    *row = *col = *blk = false;
    return g;
  }
  cellset::const_iterator it = cells.begin();
  cell c = *it;
  g.row = c.i;
  g.col = c.j;
  g.blk = board.corner(c);
  *row = *col = *blk = true;
  for (++it; it != cells.end(); ++it) {
    cell c2 = *it;
    if (g.row != c2.i)
      *row = false;
    if (g.col != c2.j)
      *col = false;
    if (g.blk != board.corner(c2))
      *blk = false;
  }
  return g;
}

bool erase_all(symset &set1, const symset &set2) {
  bool erased = false;
  for (symset::const_iterator it = set2.begin(); it != set2.end(); ++it)
    if (set1.erase(*it) > 0)
      erased = true;
  return erased;
}

template <typename T>
bool subsetof(const boost::unordered_set<T> &s1,
              const boost::unordered_set<T> &s2) {
  for (typename boost::unordered_set<T>::const_iterator it = s1.begin();
       it != s1.end(); ++it)
    if (s2.find(*it) == s2.end())
      return false;
  return true;
}

// Removes the symbols 'syms' from all other cells in the same
// group as 'cells'.
// If the symbols have already been removed from a group, pass ROW, COL,
// or BLK as appropriate as 'type'. Otherwise, pass NONE.
bool RemoveSymsFromOtherCells(Sudoku &board, const cellset &cells,
                              const symset &syms, grouptype done) {
  bool change = false;
  bool row, col, blk;
  groups g = SameGroup(board, cells, &row, &col, &blk);
  if (row && done != ROW) {
    for (int j = 0; j < board.length(); j++) {
      if (cells.find(cell(g.row, j)) == cells.end())
        change |= erase_all(board[g.row][j], syms);
    }
  }
  if (col && done != COL) {
    for (int i = 0; i < board.length(); i++) {
      if (cells.find(cell(i, g.col)) == cells.end())
        change |= erase_all(board[i][g.col], syms);
    }
  }
  if (blk && done != BLK) {
    ITERBLOCK(i, j, board, g.blk) {
      if (cells.find(cell(i, j)) == cells.end())
        change |= erase_all(board[i][j], syms);
    }
  }
  return change;
}

/**
 * for each cell c of size k:
 *   find other cells c' with D(c) = D(c')
 *   if k cells total, naked exact perm
 *
 * Misses perms like (2,3),(3,4),(2,4)
 * Catches (2,4),(2,4) or (2,3,4),(2,3,4),(2,3,4) or (2,3,4),(2,3),(3,4)
 */
bool FindMostNakedPerms(Sudoku &board, unsigned int max_perm_size) {
  bool change = false;
  // This keeps track of whether a cell needs to be searched for perms
  // in its row, column, or block.
  cellset doneR;
  cellset doneC;
  cellset doneB;
  const vector<cell> &cells = board.OrderedCells();
  for (vector<cell>::const_iterator it = cells.begin();
       it != cells.end(); ++it) {
    cell c = *it;
    const symset &dom = board[c];
    int k = dom.size();
    if (k == 1 || k > max_perm_size)
      continue;

    // Look for naked perm in this row
    if (doneR.find(c) == doneR.end()) {
      cellset foundR;
      foundR.insert(c);
      for (int j2 = 0; j2 < board.length(); j2++) {
        cell c2 (c.i, j2);
        if (board[c2].size() == 1)
          continue;
        if (j2 != c.j && doneR.find(c2) == doneR.end() &&
            subsetof(board[c2], dom)) {
          foundR.insert(c2);
        }
      }
      if (foundR.size() == k) {
        doneR.insert(foundR.begin(), foundR.end());
        change |= RemoveSymsFromOtherCells(board, foundR, dom, NONE);
      }
    }

    // Look for naked perm in this column
    if (doneC.find(c) == doneC.end()) {
      cellset foundC;
      foundC.insert(c);
      for (int i2 = 0; i2 < board.length(); i2++) {
        cell c2 (i2, c.j);
        if (board[c2].size() == 1)
          continue;
        if (i2 != c.i && doneC.find(c2) == doneC.end() &&
            subsetof(board[c2], dom)) {
          foundC.insert(c2);
        }
      }
      if (foundC.size() == k) {
        doneC.insert(foundC.begin(), foundC.end());
        change |= RemoveSymsFromOtherCells(board, foundC, dom, NONE);
      }
    }
        
    // Look for naked perm in this block
    if (doneB.find(c) == doneB.end()) {
      doneB.insert(c);
      cellset foundB;
      foundB.insert(c);
      cell cnr = board.corner(c);
      ITERBLOCK(i2, j2, board, cnr) {
        cell c2 (i2, j2);
        if (board[c2].size() == 1)
          continue;
        if (c2 != c && doneB.find(c2) == doneB.end() &&
            subsetof(board[c2], dom)) {
          foundB.insert(c2);
        }
      }
      if (foundB.size() == k) {
        doneB.insert(foundB.begin(), foundB.end());
        change |= RemoveSymsFromOtherCells(board, foundB, dom, NONE);
      }
    }

  }
  return change;
}

void MakeReverseMaps(const Sudoku &board,
                     reversemap<unsigned int>::t &rowmap,
                     reversemap<unsigned int>::t &colmap,
                     reversemap<cell>::t &blkmap) {
  for (int i = 0; i < board.length(); i++) {
    symmap &mapR = rowmap[i];
    for (int j = 0; j < board.length(); j++) {
      symmap &mapC = colmap[j];
      symmap &mapB = blkmap[board.corner(i, j)];
      const symset &dom = board[i][j];
      if (dom.size() == 1)
        continue;
      cell c = cell(i, j);
      for (symset::const_iterator it = dom.begin(); it != dom.end(); ++it) {
        mapR[*it].insert(c);
        mapC[*it].insert(c);
        mapB[*it].insert(c);
      }
    }
  }
}

// Delete all but syms from perm
bool ProcessHiddenPerm(Sudoku &board, const cellset &perm,
                       const symset &syms) {
  bool change = false;
  for (cellset::const_iterator it = perm.begin(); it != perm.end(); ++it) {
    symset &dom = board[*it];
    symset::iterator it2 = dom.begin();
    while (it2 != dom.end()) {
      if (syms.find(*it2) == syms.end()) {
        it2 = dom.erase(it2);
        change = true;
      }
      else
        ++it2;
    }
  }
  return change;
}

/** 
 * for each sym s:
 *   find c_1,...,c_k s.t. s in D(c)
 *   if union of c_i minus the rest of the group has size k:
 *     we have a hidden perm
 *
 * This will catch the case where a symbol can only go in one cell
 */
bool FindMostHiddenPerms(Sudoku &board, unsigned int max_perm_size) {
  // Large hidden permutations take more time than they're worth
  bool change = false;
  reversemap<unsigned int>::t rowmap;
  reversemap<unsigned int>::t colmap;
  reversemap<cell>::t blkmap;
  MakeReverseMaps(board, rowmap, colmap, blkmap);
  
  // Rows
  for (reversemap<unsigned int>::t::const_iterator it = rowmap.begin();
       it != rowmap.end(); ++it) {
    unsigned int i = it->first;
    const symmap &smap = it->second;
    for (symmap::const_iterator it2 = smap.begin(); it2 != smap.end(); ++it2) {
      int sym = it2->first;
      const cellset &cells = it2->second;
      int k = cells.size();
      if (k < board.blocksize()) {
        symset singleton;
        singleton.insert(sym);
        change |= RemoveSymsFromOtherCells(board, cells, singleton, ROW);
      }
      if (k > max_perm_size)
        continue;
      // (union of cells) \ (union of not cells)
      // if that size is k, we're in business
      symset others;
      for (int j = 0; j < board.length(); j++) {
        if (cells.find(cell(i, j)) == cells.end())
          others.insert(board[i][j].begin(), board[i][j].end());
      }
      symset these;
      for (cellset::const_iterator it3 = cells.begin();
           it3 != cells.end(); ++it3)
        these.insert(board[*it3].begin(), board[*it3].end());
      erase_all(these, others);
      if (these.size() == k) {
        // delete everything from cells not in these
        // check whether naked perm in other group
        change |= ProcessHiddenPerm(board, cells, these);
      }
    }
  }

  // Columns
  for (reversemap<unsigned int>::t::const_iterator it = colmap.begin();
       it != colmap.end(); ++it) {
    unsigned int j = it->first;
    const symmap &smap = it->second;
    for (symmap::const_iterator it2 = smap.begin(); it2 != smap.end(); ++it2) {
      int sym = it2->first;
      const cellset &cells = it2->second;
      int k = cells.size();
      if (k <= board.blocksize()) {
        symset singleton;
        singleton.insert(sym);
        change |= RemoveSymsFromOtherCells(board, cells, singleton, COL);
      }
      if (k > max_perm_size)
        continue;
      // (union of cells) \ (union of not cells)
      // if that size is k, we're in business
      symset others;
      for (int i = 0; i < board.length(); i++) {
        if (cells.find(cell(i, j)) == cells.end())
          others.insert(board[i][j].begin(), board[i][j].end());
      }
      symset these;
      for (cellset::const_iterator it3 = cells.begin();
           it3 != cells.end(); ++it3)
        these.insert(board[*it3].begin(), board[*it3].end());
      erase_all(these, others);
      if (these.size() == k) {
        // delete everything from cells not in these
        // check whether naked perm in other group
        change |= ProcessHiddenPerm(board, cells, these);
      }
    }
  }
  
  // Blocks
  for (reversemap<cell>::t::const_iterator it = blkmap.begin();
       it != blkmap.end(); ++it) {
    cell c = it->first;
    const symmap &smap = it->second;
    for (symmap::const_iterator it2 = smap.begin(); it2 != smap.end(); ++it2) {
      int sym = it2->first;
      const cellset &cells = it2->second;
      int k = cells.size();
      if (k <= board.blocksize()) {
        symset singleton;
        singleton.insert(sym);
        change |= RemoveSymsFromOtherCells(board, cells, singleton, BLK);
      }
      if (k > max_perm_size)
        continue;
      // (union of cells) \ (union of not cells)
      // if that size is k, we're in business
      symset others;
      ITERBLOCK(i, j, board, c) {
        if (cells.find(cell(i, j)) == cells.end())
          others.insert(board[i][j].begin(), board[i][j].end());
      }
      symset these;
      for (cellset::const_iterator it3 = cells.begin();
           it3 != cells.end(); ++it3)
        these.insert(board[*it3].begin(), board[*it3].end());
      erase_all(these, others);
      if (these.size() == k) {
        // delete everything from cells not in these
        // check whether naked perm in other group
        change |= ProcessHiddenPerm(board, cells, these);
      }
    }
  }
  
  return change;
}

// TODO nested while loops, common strategies in the inner one
bool LogicSolve(Sudoku &board) {
  bool change = true;
  bool success = AC3(board);
  unsigned int max_perm_size = board.blocksize();
  while (change) {
    if (!success)
      return false;
    bool res1 = FindMostHiddenPerms(board, max_perm_size);
    if (res1)
      success &= AC3(board);
    bool res2 = FindMostNakedPerms(board, max_perm_size);
    if (res2)
      success &= AC3(board);
    bool res3 = Swordfish(board);
    if (res3)
      success &= AC3(board);
    change = res1 || res2 || res3;
  }
  return true;
}

bool GuessSolve(Sudoku &board) {
  bool success = LogicSolve(board);
  if (!success || board.Solved())
    return success;
  cell guess = board.OrderedCells().front();
  for (symset::const_iterator it = board[guess].begin();
       it != board[guess].end(); ++it) {
    Sudoku board2 = board.Clone();
    symset &dom = board2[guess];
    dom.clear();
    dom.insert(*it);
    if (GuessSolve(board2)) {
      board = board2;
      return true;
    }
  } 
  return false;
}

void print_usage() {
  cout << "Sudoku Solver\n" << endl;
  cout << "solver [--logic] puzzle\n" << endl;
  cout << "Solves the Sudoku puzzle, guessing if necessary. If the --logic\n";
  cout << "flag is provided, the solver will only use logic to try to solve\n";
  cout << "the puzzle, though it may be unable to completely solve it.";
  cout << endl;
  exit(0);
}

bool process_args(int argc, char ***argv) {
  char **args = *argv;
  bool logic = false;
  if (argc > 3 || argc < 2)
    print_usage();
  for (int i = 1; i < argc; i++) {
    if (strncmp(args[i], "--logic", 7) == 0)
      logic = true;
    else
      *argv = args + i;
  }
  if (logic + argc == 3)
    print_usage();
  return logic;
}

int main(int argc, char **argv) {
  bool logic = process_args(argc, &argv);
  cout << argv[0] << endl;
  Sudoku s = Sudoku::ParseFromFile(argv[0]);
  cout << s.ToString() << endl;
  if (logic)
    LogicSolve(s);
  else
    GuessSolve(s);
  cout << s.ToString() << endl;
  cout << (s.Solved() ? "Solved!" : "Unsolved") << endl;
}
