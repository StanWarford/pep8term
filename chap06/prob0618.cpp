// File: prob0618.cpp
// Computer Systems, Fourth Edition
// Problem 6.18

#include <iostream>
using namespace std;

int times (int mpr, int mcand) {
   if (mpr == 0) {
      return 0;
   }
   else if (mpr % 2 == 1) {
      return times (mpr / 2, mcand * 2) + mcand;
   }
   else {
      return times (mpr / 2, mcand * 2);
   }
}

int main () {
   int n, m;
   cin >> n >> m;
   cout << "Product: " << times (n, m) << endl;
   return 0;
}
