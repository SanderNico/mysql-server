# include <iostream>
# include <cmath>
# include <cstdlib>
# include <ctime>
# include <limits>
# include "sql/count_min_sketch/CountMinSketch.h"

/**
   Class definition for CountMinSketch.
   public operations:
   // overloaded updates
   void update(int item, int c);
   void update(char *item, int c);
   // overloaded estimates
   unsigned int estimate(int item);
   unsigned int estimate(char *item);
**/


// CountMinSketch constructor
// ep -> error 0.01 < ep < 1 (the smaller the better)
// gamma -> probability for error (the smaller the better) 0 < gamm < 1
CountMinSketch::CountMinSketch(float ep, float gamm) {
  epsilon = ep;
  gamma = gamm;
  width = ceil(exp(1)/epsilon);
  depth = ceil(log(1/gamma));
  total = 0;
  // initialize counter array of arrays, C
  C = new int *[depth];
  unsigned int i, j;
  for (i = 0; i < depth; i++) {
    C[i] = new int[width];
    for (j = 0; j < width; j++) {
      C[i][j] = 0;
    }
  }
  // initialize d pairwise independent hashes
  srand(time(NULL));
  hashes = new int* [depth];
  for (i = 0; i < depth; i++) {
    hashes[i] = new int[2];
    genajbj(hashes, i);
  }
}

// CountMinSkectch destructor
CountMinSketch::~CountMinSketch() {
  // free array of counters, C
  unsigned int i;
  for (i = 0; i < depth; i++) {
    delete[] C[i];
  }
  delete[] C;
  
  // free array of hash values
  for (i = 0; i < depth; i++) {
    delete[] hashes[i];
  }
  delete[] hashes;
}

// CountMinSketch totalcount returns the
// total count of all items in the sketch
unsigned int CountMinSketch::totalcount() {
  return total;
}

// countMinSketch update item count (int)
void CountMinSketch::update(int item, int c) {
  // total = total + c;
  // unsigned int hashval = 0;
  // for (unsigned int j = 0; j < depth; j++) {
  //   hashval = ((long)hashes[j][0]*item+hashes[j][1])%LONG_PRIME%width;
  //   C[j][hashval] = C[j][hashval] + c;
  // }
  C[2][3] = 2;
}

// countMinSketch update item count (string)
void CountMinSketch::update(const char *str, int c) {
  int hashval = hashstr(str);
  update(hashval, c);
}

// CountMinSketch estimate item count (int)
unsigned int CountMinSketch::estimate(int item) {
  int minval = std::numeric_limits<int>::max();
  unsigned int hashval = 0;
  for (unsigned int j = 0; j < depth; j++) {
    hashval = (hashes[j][0]*item+hashes[j][1])%width;
    minval = MIN(minval, C[j][hashval]);
  }
  return minval;
}

// CountMinSketch estimate item count (string)
unsigned int CountMinSketch::estimate(const char *str) {
  int hashval = hashstr(str);
  return estimate(hashval);
}

// generates aj,bj from field Z_p for use in hashing
void CountMinSketch::genajbj(int** hashes, int i) {
  hashes[i][0] = int(float(rand())*float(LONG_PRIME)/float(RAND_MAX) + 1);
  hashes[i][1] = int(float(rand())*float(LONG_PRIME)/float(RAND_MAX) + 1);
}

// generates a hash value for a sting
// same as djb2 hash function
unsigned int CountMinSketch::hashstr(const char *str) {
  unsigned long hash = 5381;
  int c;
  while ( (c = *str++) ) {
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  }
  return hash;
}
