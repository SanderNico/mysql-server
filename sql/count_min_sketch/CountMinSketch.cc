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
  CountMinSketch::epsilon = ep;
  CountMinSketch::gamma = gamm;
  CountMinSketch::width = ceil(exp(1)/epsilon);
  CountMinSketch::depth = ceil(log(1/gamma));
  CountMinSketch::total = 0;
  // initialize counter array of arrays, C
  CountMinSketch::C = new int *[depth];
  unsigned int i, j;
  for (i = 0; i < CountMinSketch::depth; i++) {
    CountMinSketch::C[i] = new int[width];
    for (j = 0; j < CountMinSketch::width; j++) {
      CountMinSketch::C[i][j] = 0;
    }
  }
  // initialize d pairwise independent hashes
  srand(time(NULL));
  CountMinSketch::hashes = new int* [depth];
  for (i = 0; i < CountMinSketch::depth; i++) {
    CountMinSketch::hashes[i] = new int[2];
    CountMinSketch::genajbj(hashes, i);
  }
}

// CountMinSkectch destructor
CountMinSketch::~CountMinSketch() {
  // free array of counters, C
  unsigned int i;
  for (i = 0; i < CountMinSketch::depth; i++) {
    delete[] CountMinSketch::C[i];
  }
  delete[] CountMinSketch::C;
  
  // free array of hash values
  for (i = 0; i < CountMinSketch::depth; i++) {
    delete[] CountMinSketch::hashes[i];
  }
  delete[] CountMinSketch::hashes;
}

// CountMinSketch totalcount returns the
// total count of all items in the sketch
unsigned int CountMinSketch::totalcount() {
  return total;
}

// countMinSketch update item count (int)
void CountMinSketch::update(int item, int c) {
  CountMinSketch::total = total + c;
  unsigned int hashval = 0;
  for (unsigned int j = 0; j < CountMinSketch::depth; j++) {
    hashval = ((long)CountMinSketch::hashes[j][0]*item+CountMinSketch::hashes[j][1])%LONG_PRIME%width;
    CountMinSketch::C[j][hashval] = CountMinSketch::C[j][hashval] + c;
  }
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
  for (unsigned int j = 0; j < CountMinSketch::depth; j++) {
    hashval = ((long)CountMinSketch::hashes[j][0]*item+CountMinSketch::hashes[j][1])%LONG_PRIME%width;
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
  while (c = *str++) {
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  }
  return hash;
}
