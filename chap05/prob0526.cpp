;File: prob0526.cpp
;Computer Systems, Fourth Edition
;Problem 5.26
;
#include <iostream>
using namespace std;

const int amount = 20000;
int num;
int sum;
 
int main () {
   cin >> num;
   sum = num + amount;
   cout << "sum = " << sum << endl;
   return 0;
}
