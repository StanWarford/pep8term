//  File: stripCR.cpp
//  Utility for the Pep/8 computer as described in "Computer Systems",
//  Fourth edition, J. Stanley Warford, Jones and Bartlett, Publishers,
//  2010.  ISBN 978-0-7637-7144-7
//  Pepperdine University, Malibu, CA 90265
//  Stan.Warford@pepperdine.edu

//  Released under the GNU General Public License without warrenty
//  as described in http://www.gnu.org/copyleft/gpl.html

// Strips the <CR> character from DOS files, which
// use <CR><LF> at the end of each line, to make the source files compatible
// with the <LF> terminated lines of Unix/Linux/OS X. This utility takes a
// stream of characters from the standard input and produces an output
// stream of characters to the standard output. You should run DOS files
// through this utility or its equivalent before using asem8, as the Pep/8
// assembler assumes <LF> terminated lines. It also appends a <LF> character
// at the end of the file to assure that the last line terminates with a
// newline.

#include <iostream>

int main ()
{
   char c;
   while ((c = getchar()) != EOF)
   {
      if (c != '\r')
      {
         putchar (c);
      }
   }
   putchar ('\n');
   return 0;
}

