;File: prob0531.cpp
;Computer Systems, Fourth Edition
;Problem 5.31
;
#include <iostream>
using namespace std;
 
int num;
 
int main () {
   cin >> num;
   num = num % 16;
   cout << "num = " << num << endl;
   return 0;
}
