#include <boost/unordered_set.hpp>
#include <cstring>
#include <iostream>
#include <vector>

#include "sudoku.h"

using namespace std;

typedef boost::unordered_map<int, cellset> symmap;

template <typename T>
struct reversemap {
  typedef boost::unordered_map<T, symmap> t;
};

enum grouptype {
  NONE,
  ROW,
  COL,
  BLK
};

template <typename InputIterator>
void print_container(InputIterator first, InputIterator last) {
  cout << '{';
  for (; first != last; ++first)
    cout << *first << ',';
  cout << '}' << endl;
}

template <typename T>
bool erase_all(boost::unordered_set<T> &s1,
               const boost::unordered_set<T> &s2) {
  bool erased = false;
  for (typename boost::unordered_set<T>::const_iterator it = s2.begin();
       it != s2.end(); ++it)
    if (s1.erase(*it) > 0)
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

// ---------------------------------------------------------------------------
// -------------------------------- AC3 --------------------------------------
// ---------------------------------------------------------------------------

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
  return change;
}

bool AC3(Sudoku &board) {
  cellset todo;
  for (int i = 0; i < board.length(); i++) {
    for (int j = 0; j < board.length(); j++) {
      if (board[i][j].size() == 0)
        return false;
      todo.insert(cell(i, j));
    }
  }
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

// ---------------------------------------------------------------------------
// ----------------------------- Swordfish -----------------------------------
// ---------------------------------------------------------------------------

// TODO
bool Swordfish(Sudoku &board) {
  return false;
}

// ---------------------------------------------------------------------------
// --------------------------- Symbol Removal --------------------------------
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// ------------------------- Naked Permutations ------------------------------
// ---------------------------------------------------------------------------

// Checks to see whether cell 'c' with domain 'dom' is a superset of a
// naked permutation in a group (which is given by 'TYPE').
template <grouptype TYPE>
bool SearchGroupForNaked(Sudoku &board, cellset &done, const symset &dom,
                         const cell &c) {
  if (done.find(c) == done.end()) {
    cellset found;
    found.insert(c);
    for (int x = 0; x < board.length(); x++) {
      cell c2 (c);
      if (TYPE == ROW)
        c2.j = x;
      else if (TYPE == COL)
        c2.i = x;
      if (board[c2].size() == 1)
        continue;
      if (c2 != c && done.find(c2) == done.end() &&
          subsetof(board[c2], dom)) {
        found.insert(c2);
      }
    }
    if (found.size() == dom.size()) {
      done.insert(found.begin(), found.end());
      return RemoveSymsFromOtherCells(board, found, dom, NONE);
    }
  }
  return false;
}

// Checks to see whether cell 'c' with domain 'dom' is a superset of a
// naked permutation in a group (which is given by 'TYPE').
template <>
bool SearchGroupForNaked<BLK>(Sudoku &board, cellset &done, const symset &dom,
                              const cell &c) {
  if (done.find(c) == done.end()) {
    done.insert(c);
    cellset found;
    found.insert(c);
    cell cnr = board.corner(c);
    ITERBLOCK(i2, j2, board, cnr) {
      cell c2 (i2, j2);
      if (board[c2].size() == 1)
        continue;
      if (c2 != c && done.find(c2) == done.end() &&
          subsetof(board[c2], dom)) {
        found.insert(c2);
      }
    }
    if (found.size() == dom.size()) {
      done.insert(found.begin(), found.end());
      return RemoveSymsFromOtherCells(board, found, dom, NONE);
    }
  }
  return false;
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

    change |= SearchGroupForNaked<ROW>(board, doneR, dom, c);
    change |= SearchGroupForNaked<COL>(board, doneC, dom, c);
    change |= SearchGroupForNaked<BLK>(board, doneB, dom, c);
  }
  return change;
}

// ---------------------------------------------------------------------------
// ------------------------- Hidden Permutations -----------------------------
// ---------------------------------------------------------------------------

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

// Delete all but the symbols in 'syms' from the cells in 'perm'.
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

template <grouptype T>
struct MapKey {
  typedef unsigned int key;
};

template <>
struct MapKey<BLK> {
  typedef cell key;
};

template <grouptype TYPE>
struct Map {
  typedef typename reversemap<typename MapKey<TYPE>::key>::t t;
};

template <grouptype TYPE>
void FindOtherSyms(const Sudoku &board, const cellset &cells,
                   symset &others, typename MapKey<TYPE>::key x) {
}

template <>
void FindOtherSyms<ROW>(const Sudoku &board, const cellset &cells,
                        symset &others, MapKey<ROW>::key i) {
  for (int j = 0; j < board.length(); j++) {
    if (cells.find(cell(i, j)) == cells.end())
      others.insert(board[i][j].begin(), board[i][j].end());
  }
}

template <>
void FindOtherSyms<COL>(const Sudoku &board, const cellset &cells,
                        symset &others, MapKey<COL>::key j) {
  for (int i = 0; i < board.length(); i++) {
    if (cells.find(cell(i, j)) == cells.end())
      others.insert(board[i][j].begin(), board[i][j].end());
  }
}

template <>
void FindOtherSyms<BLK>(const Sudoku &board,  const cellset &cells,
                        symset &others, MapKey<BLK>::key c) {
  ITERBLOCK(i, j, board, c) {
    if (cells.find(cell(i, j)) == cells.end())
      others.insert(board[i][j].begin(), board[i][j].end());
  }
}

template <grouptype TYPE>
bool SearchGroupForHidden(Sudoku &board, unsigned int max_perm_size,
                          const typename Map<TYPE>::t &grpmap) {
  bool change = false;
  for (typename Map<TYPE>::t::const_iterator it = grpmap.begin();
       it != grpmap.end(); ++it) {
    typename MapKey<TYPE>::key x = it->first;
    const symmap &smap = it->second;
    for (symmap::const_iterator it2 = smap.begin(); it2 != smap.end(); ++it2) {
      int sym = it2->first;
      const cellset &cells = it2->second;
      int k = cells.size();
      if (k <= board.blocksize()) {
        symset singleton;
        singleton.insert(sym);
        change |= RemoveSymsFromOtherCells(board, cells, singleton, TYPE);
      }
      if (k > max_perm_size)
        continue;
      // (union of cells) \ (union of not cells)
      // if that size is k, we're in business
      symset others;
      FindOtherSyms<TYPE>(board, cells, others, x);      
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
  
  change |= SearchGroupForHidden<ROW>(board, max_perm_size, rowmap);
  change |= SearchGroupForHidden<COL>(board, max_perm_size, colmap);
  change |= SearchGroupForHidden<BLK>(board, max_perm_size, blkmap);
  
  return change;
}

// ---------------------------------------------------------------------------
// ------------------------------ Solvers ------------------------------------
// ---------------------------------------------------------------------------

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
  // TODO smarter guess?
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
