// File: prob0624.cpp
// Computer Systems, Fourth Edition
// Problem 6.24

#include <iostream>
using namespace std;

int product, n, m;

void times (int& prod, int mpr, int mcand) {
   prod = 0;
   while (mpr != 0) {
      if (mpr % 2 == 1) {
         prod = prod + mcand;
      }
      mpr /= 2;
      mcand *= 2;
   }
}

int main () {
   cin >> n >> m;
   times (product, n, m);
   cout << "Product: " << product << endl;
   return 0;
}

