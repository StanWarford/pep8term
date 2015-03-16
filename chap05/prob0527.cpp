;File: prob0527.cpp
;Computer Systems, Fourth Edition
;Problem 5.27
;
#include <iostream>
using namespace std;

int width;
int length;
int perim;
 
int main () {
   cin >> width >> length;
   perim = (width + length) * 2;
   cout << "w = " << width << endl;
   cout << "1 = " << length << endl;
   cout << endl;
   cout << "p = " << perim << endl;
   return 0;
}
