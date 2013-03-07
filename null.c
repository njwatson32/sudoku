// This isn't actually used for our implementation, but it was written
// for a literal translation to Haskell. It is fully tested and correct,
// so bindings could be written to use this code.

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef unsigned char *f2row;

#define GET(r, j) (((r)[(j) / 8] & (0x80 >> ((j) % 8))) > 0)
#define GETM(m, i, j) (GET(m[i], j))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

void print_mat(f2row *mat, int m, int n) {
  int i, j;
  for (i = 0; i < m; i++) {
    for (j = 0; j < n; j++)
      printf("%d ", GETM(mat, i, j));
    printf("\n");
  }
  printf("\n");
}

void print_vect(int *v, int n) {
  int i;
  printf("(");
  for (i = 0; i < n; i++) {
    printf("%d", v[i]);
    if (i != n - 1)
      printf(", ");
  }
  printf(")\n");
}

f2row *gen_mat(int *m, int *n) {
  int i, j;
  *m = 1 + rand() % 128;
  *n = 1 + rand() % 128;
  //*n = 10000; *m = 10000;

  int hi = *n % 8 ? *n / 8 + 1 : *n / 8;

  f2row *mat = malloc(*m * sizeof(f2row));
  for (i = 0; i < *m; i++) {
    mat[i] = malloc(hi * sizeof(unsigned char));
    for (j = 0; j < hi; j++) {
      unsigned char r = rand() % 256;
      mat[i][j] = r;
    }
  }

  return mat;
}

void free_mat(f2row *mat, int m) {
  int i;
  for (i = 0; i < m; i++)
    free(mat[i]);
  free(mat);
}

void free_nullspace(int **vects) {
  int i;
  for (i = 0; vects[i]; i++)
    free(vects[i]);
  free(vects);
}

inline int find_nonzero(f2row *mat, int s, int j, int m) {
  int i;
  for (i = s; i < m; i++)
    if (GETM(mat, i, j)) return i;
  return -1;
}

// O(mn(m + n))
int rref(f2row *mat, int m, int n) {
  int i, j, nvs = 0;
  int hi = n % 8 ? n / 8 + 1 : n / 8;

  int ht = 0, wd = 0;
  while (ht < m && wd < n) {
    // find the first nonzero entry in this column at height >= ht
    int r = find_nonzero(mat, ht, wd, m);
    if (r < 0) {
      // keep same height, but check next row over
      wd++;
      // this adds a linear dependency, hence a null vector
      nvs++;
      continue;
    }
    // swap
    f2row tmp = mat[ht];
    mat[ht] = mat[r];
    mat[r] = tmp;
    
    // For each row
    for (i = 0; i < m; i++) {
      // that has a 1 and isn't the wd'th row
      if (i != ht && GETM(mat, i, wd)) {
        f2row row1 = mat[ht]; // used to be row r
        f2row row2 = mat[i];
        // xor the rows
        for (j = 0; j < hi; j++)
          row2[j] ^= row1[j];
      }
    }
    // Leading 1 obtained. Advance both height and width
    ht++;
    wd++;
  }

  // Any remaining columns to the right must be dependent, so add them
  return nvs + (n - wd);
}

// O(m + n)
int *construct_null_vector(f2row *mat, int *leading, int m, int n, int j) {
  int i;
  int *vect = calloc(n, sizeof(int));
  for (i = 0; i < MIN(j, m); i++) {
    if (GETM(mat, i, j))
      vect[leading[i]] = 1;
  }
  vect[j] = 1;
  return vect;
}      

// O((m + n)^2)
int **get_null_vectors(f2row *mat, int m, int n, int nvs) {
  int i, j, ix = 0;
  int **vectors = malloc((nvs + 1) * sizeof(int *));
  int *leading = malloc(m * sizeof(int));
  
  // initialize leading
  for (i = 0; i < m; i++) {
    f2row r = mat[i];
    leading[i] = -1;
    int s = i == 0 ? 0 : leading[i - 1];
    for (j = s; j < n; j++) {
      if (GET(r, j)) {
        leading[i] = j;
        break;
      }
    }
  }
  
  // Find null vectors
  i = 0; j = 0;
  while (i < m && j < n) {
    if (GETM(mat, i, j) == 0) {
      vectors[ix++] = construct_null_vector(mat, leading, m, n, j);
      j++;
    }
    else {
      i++;
      j++;
    }
  }

  // Get any remaining ones to the right
  for (; j < n; j++)
    vectors[ix++] = construct_null_vector(mat, leading, m, n, j);

  free(leading);
  assert(nvs == ix);
  vectors[ix] = NULL;
  return vectors;
}

// Get the nullspace of the matrix. May contain duplicate vectors
int **nullspace(f2row *mat, int m, int n) {
  int nvs = rref(mat, m, n);
  return get_null_vectors(mat, m, n, nvs);
}

int all_zeros(f2row row, int n) {
  int j;
  for (j = 0; j < n; j++) {
    if (GET(row, j))
      return 0;
  }
  return 1;
}

int leading_column(f2row row, int n) {
  int j;
  for (j = 0; j < n; j++)
    if (GET(row, j)) return j;
  return -1;
}

int leading_only_one(f2row *mat, int m, int i, int j) {
  int k;
  for (k = 0; k < m; k++) {
    if (k != i && GETM(mat, k, j))
      return 0;
  }
  return 1;
}

void prop_rref(f2row *mat, int m, int n) {
  int i;
  int current = -1, found = 0;
  for (i = 0; i < m; i++) {
    // Zeros are lowest
    if (found)
      assert(all_zeros(mat[i], n));
    else if (all_zeros(mat[i], n))
      found = 1;

    // Leading ones strictly to right of above leading ones
    int j = leading_column(mat[i], n);
    if (j == -1) continue;
    assert(current < j);
    current = j;

    // The other entries in the column of a leading one are zero
    assert(leading_only_one(mat, m, i, j));
  }
}

void prop_is_null_vector(f2row *mat, int m, int n, int *v) {
  int i, j;
  for (i = 0; i < m; i++) {
    int sum = 0;
    for (j = 0; j < n; j++)
      sum ^= (GETM(mat, i, j) & v[j]);
    assert(sum == 0);
  }
}

void rref_test() {
  int m, n;
  f2row *mat = gen_mat(&m, &n);
  //print_mat(mat, m, n);
  rref(mat, m, n);
  //print_mat(mat, m, n);
  prop_rref(mat, m, n);
  free_mat(mat, m);
}

void nullspace_test() {
  int m, n, i;
  f2row *mat = gen_mat(&m, &n);
  //print_mat(mat, m, n);
  int **null = nullspace(mat, m, n);
  //print_mat(mat, m, n);
  for (i = 0; null[i]; i++) {
    //print_vect(null[i], n);
    prop_is_null_vector(mat, m, n, null[i]);
  }
  free_mat(mat, m);
  free_nullspace(null);
}

int main() {
  int i;
  
  srand(time(NULL));
  for (i = 0; i < 1; i++) {
    rref_test();
    nullspace_test();
  }
  
  return 0;
}
