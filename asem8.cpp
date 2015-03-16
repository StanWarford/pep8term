//  File: asem8.cpp
//  Assembler for the Pep/8 computer as described in "Computer Systems",
//  Fourth edition, J. Stanley Warford, Jones and Bartlett, Publishers,
//  2010.  ISBN 978-0-7637-7144-7
//  Pepperdine University, Malibu, CA 90265
//  Stan.Warford@pepperdine.edu

//  Released under the GNU General Public License without warrenty
//  as described in http://www.gnu.org/copyleft/gpl.html

//  Version history: 

/*

  Written by Luciano d'Ilori.

  Version 8.17
  Fixed a bug in the translation of hex constants and eliminated the warnings
  due to lack of default cases in switch statements and lack of possible return
  values in nonvoid functions.
  Warford, 8 February 2015

  Version 8.16
  Increased LINE_LENGTH from 128 to 1024 to allow longer lines in the source
  file, and CODE_MAX_SIZE from 4096 to 32768 to allow longer object programs
  to be assembled.
  Warford, 14 February 2010

  Version 8.15
  Fixed a bug in vGetToken to allow underscore characters _ in identifiers.
  Warford, 31 March 2009

  Version 8.14
  Fixed a bug in vDecToHexWord for decimal values greater than MAX_DEC.
  Warford, 31 December 2005

  Version 8.13
  Comments too long are now truncated instead of causing fatal error.
  Warford, 04 February 2005

  Version 8.12
  Replaced depricated libraries. Added namespace std.
  Warford, 01 Februrary 2005

  Version 8.11
  Now allowing leading zeros in decimal constants.

  Version 8.1
  Added functionality so user can define certain mnemonics.
  Modified assembler syntax to allow certain non-printable characters.
  Updated instruction set. 

  Version 8.04
  Fixed indexed addressing bug.
  <,x> now adds 1 or 5 to opcode depending on instruction

  Version 8.03
  Allows .EQUATE with char and string constants.
  Has more detailed error messages.

*/

/*Constants are in capital letters.
  Function references in comments do not list any possible parameters the functions may have.
  Most functions begin with the lowercase first letter as that of the type they return.
  Most variables begin with the lowercase first letter as that of their type (ex. int iDec).
  For the most part, numbers other than 0 and 1 are either constants, array indices,
  or are given explanations.*/

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <iomanip>
#include <ctype.h>
#include <string>
using namespace std;

/*Constants*/
const int IDENT_LENGTH=8; /*Maximum identifier length*/
const int HEX_LENGTH=4; /*Length of a hexadecimal string*/
const int BYTE_LENGTH=2; /*Length of byte is 2 hex digits*/
const int WORD_LENGTH=4; /*Length of word is 4 hex digits*/
const int CHAR_LENGTH=4; /*Can be \x with 2 hex digits*/
const int DEC_LENGTH=6; /*-32768(6 characters) to 65535*/
const int COMMENT_LENGTH=65; /*Maximum comment length for empty lines*/
const int COMMENT_LENGTH_NONEMPTY=35; /*Maximum comment length for nonempty lines*/
const int COMMENT_LENGTH_NONEMPTY_NO_SYMBOLS=44; /*Maximum comment length for nonempty lines*/
const int STRING_LENGTH=96; /*Maximum string length*/
const int STRING_OPRND_LENGTH=4; /*string operands can be up to 4 hex digits long*/
const int ADDR_MODE_LENGTH=3; /*Maximum length of addr. mode (eg. sxf)*/
const int MAX_LINES=4096; /*pACode[MAX_LINES], pAMnemon[MAX_LINES]*/
const int BYTE=1; /*A byte is 1 byte*/
const int WORD=2; /*A word is 2 bytes*/
const int UNARY=1; /*A unary instruction takes up 1 byte*/
const int NONUNARY=3; /*A nonunary instruction takes up 3 bytes*/
const int OBJ_FILE_LINE_LENGTH=16; /*Number of bytes per line in object(hex) file*/
const int OBJ_CODE_LENGTH=6; /*Number of characters per line for object code in asem listing*/
const int IMMEDIATE=1;/*2^0. Powers of two to represent addressing mode bitset*/
const int DIRECT=2;/*2^1*/
const int INDIRECT=4;/*2^2*/
const int STACK_RELATIVE=8;/*2^3*/
const int STACK_RELATIVE_DEFERRED=16;/*2^4*/
const int INDEXED=32;/*2^5*/
const int STACK_INDEXED=64;/*2^6*/
const int STACK_INDEXED_DEFERRED=128;/*2^7*/
const int HEX=16; /*vGenerateHexCode() and vDecToHexWord()*/
const int HEX2=256; /*vDecToHexWord() for value of second most significant bit in hex*/
const int HEX3=4096; /*vDecToHexWord() for value of most significant bit in hex*/
const int OPERAND_SPACES=14; /*Number of charcter spaces in assembler listing for operands*/
const int ADDR_LENGTH=4; /*Length of addresses (2 bytes or 4 hex digits)*/
const int MAX_ADDR=65535; /*Maximum address location*/
const int MAX_BYTE=255; /*Maximum decimal value for a byte*/
const int MAX_DEC=65535; /*Maximum decimal value*/
const int MIN_BYTE=-256; /*Minimum decimal value for a byte*/
const int MIN_DEC=-32768; /*Minimum decimal value*/
const int LINE_LENGTH=1024; /*Maximum length of a line of code*/
const int CODE_MAX_SIZE=32768; /*Maximum number of bytes of code*/
const int FILE_NAME_LENGTH=64; /*61 characters maximum in a file name*/
const int UNIMPLEMENTED_INSTRUCTIONS = 8; /*Number of unimplemented mnemonics*/
const int UNARY_TRAPS = 4; /*Number of unimplemented mnemonics guaranteed to be unary*/

/*Enumerated Types*/
/*All possible mnemonics*/
enum Mnemon{
    eM_STOP, eM_RETTR, eM_MOVSPA, eM_MOVFLGA, eM_BR, eM_BRLE, eM_BRLT, eM_BREQ, 
    eM_BRNE, eM_BRGE, eM_BRGT, eM_BRV, eM_BRC, eM_CALL, eM_NOTA, eM_NOTX, eM_NEGA, 
    eM_NEGX, eM_ASLA, eM_ASLX, eM_ASRA, eM_ASRX, eM_ROLA, eM_ROLX, eM_RORA, 
    eM_RORX, eM_CHARI, eM_CHARO, eM_RET0, eM_RET1, eM_RET2, eM_RET3, eM_RET4, 
    eM_RET5, eM_RET6, eM_RET7, eM_ADDSP, eM_SUBSP, eM_ADDA, eM_ADDX, eM_SUBA, 
    eM_SUBX, eM_ANDA, eM_ANDX, eM_ORA, eM_ORX, eM_CPA, eM_CPX, eM_LDA, eM_LDX, 
    eM_LDBYTEA, eM_LDBYTEX, eM_STA, eM_STX, eM_STBYTEA, eM_STBYTEX, eM_UNIMP0, 
    eM_UNIMP1, eM_UNIMP2, eM_UNIMP3, eM_UNIMP4, eM_UNIMP5, eM_UNIMP6, eM_UNIMP7, eM_EMPTY
};

/*All possible dot commands*/
enum DotCommand{
    eD_BLOCK, eD_ADDRSS, eD_ASCII, eD_BURN, eD_BYTE, eD_EQUATE, eD_WORD, eD_END,
    eD_EMPTY 
};

/*Tokens found by vGetToken()*/
enum Key{
    eT_ADDRMODE, eT_CHARCONSTANT, eT_COMMENT, eT_DECCONSTANT, eT_DOTCOMMAND,
    eT_EMPTY, eT_HEXCONSTANT, eT_IDENTIFIER, eT_STRING, eT_SYMBOL, eT_INVALID,
    eT_INVALIDADDR, eT_INVALIDCHAR, eT_INVALIDCOMMENT, eT_INVALIDDEC, eT_INVALIDDOTCOMMAND,
    eT_INVALIDHEX, eT_INVALIDSTRING
};

/*States for finite state machine of vGetToken()*/
enum State{
    eS_START, eS_ADDR, eS_ADDRs, eS_ADDRsx, eS_CHAR1, eS_CHAR2, eS_CHARBASH,
    eS_CHARBYTE, eS_COMMENT, eS_DEC, eS_DOT1, eS_DOT2, eS_HEX1, eS_HEX2,
    eS_IDENT, eS_SIGN, eS_STRING, eS_STRINGBASH, eS_STRINGBYTE, eS_STOP
};

/*States for finite state machine of vProcessSourceLine()*/
enum ParseState{
    ePS_START, ePS_COMMENT, ePS_SYMBOLDEC, ePS_INSTRUCTION, ePS_OPRNDSPECDEC,
    ePS_OPRNDSPECHEX, ePS_OPRNDSPECCHAR, ePS_OPRNDSPECSTRING, ePS_OPRNDSPECSYM,
    ePS_DOTCOMMAND, ePS_ASCII, ePS_EQUATE, ePS_CLOSE, ePS_FINISH
};

/*Global Records*/
/*Contains .EQUATE symbols to be used in a linked list*/
struct sEquateNode{
    char cSymValue[ADDR_LENGTH + 1]; /*Value of symbol*/
    char cSymID[IDENT_LENGTH + 1]; /*Symbol identification*/
    sEquateNode* pNext;
};
/*Record for symbol declarations*/
struct sSymbolNode{
    char cSymValue[ADDR_LENGTH + 1]; /*Value of symbol*/
    int iLine;
    char cSymID[IDENT_LENGTH + 1]; /*Symbol name*/
    sSymbolNode* pNext; /*Pointer to next sSymbolNode in the linked list*/
};
/*Record for symbol output declarations*/
struct sSymbolOutputNode{
    int iLine;
    char cSymID[IDENT_LENGTH + 1]; /*Symbol name*/
    sSymbolOutputNode* pNext; /*Pointer to next sSymbolOutputNode in the linked list*/
};
/*Record for symbol declarations*/
struct sUndeclaredsSymbolNode{
    char cSymValue[ADDR_LENGTH + 1]; /*Value of symbol*/
    int iLine;
    char cSymID[IDENT_LENGTH + 1]; /*Symbol identification*/
    sUndeclaredsSymbolNode* pNext; /*Pointer to next sSymbolNode in the linked list*/
};
/*Contains information about comments to be used in a linked list*/
struct sCommentNode{
    int iLine;
    bool bNonemptyLine;
    char cComment[COMMENT_LENGTH + 1];
    sCommentNode* pNext;
};

/*Contains information about user-defined instructions*/
struct sUnimplementedMnemonNode{
    char cID[IDENT_LENGTH + 1]; //Name of unimplemented opcode
    int iAddrMode;
};

/*Global Variables (part 1)*/
ifstream in_file;
ofstream out_file;
char cLine[LINE_LENGTH]; /*Array of characters for a line of code*/
int iLineIndex; /*Index of line array*/
int iSecPassCodeIndex=0; /*Used in second pass of assembly to account for symbols*/
int iCurrentAddress=0; /*Keeps track of the current address*/
sSymbolNode* pSymbol; /*Pointer to linked list of sSymbolNodes*/
sSymbolOutputNode* pSymbolOutput; /*Pointer to linked list of sSymbolNodes for output*/
sUndeclaredsSymbolNode* pUndeclaredSym; /*Pointer to linked list of sUndeclaredsSymbolNodes*/
sCommentNode* pComment=NULL; /*Pointer to first sCommentNode of the comment linked list*/
sEquateNode* pEquate=NULL; /*Pointer to first sEquateNode of the .EQUATE linked list*/
char cDotTable[eD_EMPTY + 1][IDENT_LENGTH + 1]; /*Used for vLookUpDot()*/
char cMnemonTable [eM_EMPTY + 1][IDENT_LENGTH + 1]; /*Used for vLookUpMnemon()*/
int iHexOutputBuffer=0; /*Used for object code output for 16 bytes per line*/
bool bIsAscii=false; /*Keeps track of whether previous token was .ASCII pseudo-op*/
int iBurnStart=0; /*Used first to  store the value of the operand of a .BURN*/
/*and then to  store the value of where the first byte of code should be written.*/
int iBurnAddr=0; /*Used to store the address of a .BURN line*/
int iBurnCounter=0; /*Keeps track of number of .BURNs used in the program.*/
sUnimplementedMnemonNode sUnimpMnemon [UNIMPLEMENTED_INSTRUCTIONS]; /*Array of unimplemented mnemon nodes*/
/* Global variables continued after class ACode declaration*/

/*Utility functions*/
bool bIsHex(char& ch){
    if (('a'<=ch) && (ch<='f')){
        ch=toupper(ch);
        return true;
    }
    else if ((isdigit(ch)) || (('A'<=ch) && (ch<='F')))
        return true;
    else
        return false;
}

/*Stores the next line of assembly language code to be translated in global cLine[].*/
void vGetLine(){
    in_file.getline(cLine, LINE_LENGTH);
    if ((!in_file.eof ()) && (in_file.gcount()>0))
        cLine[in_file.gcount() - 1]='\n';
    else{
        cLine[in_file.gcount()]='\n';
    }
    iLineIndex=0;
}

/*Gets the next character to be processed by vGetToken().*/
void vAdvanceInput (char& ch){
    ch=cLine[iLineIndex++];
}

/*Backs up the input to the current character to be processed by vGetToken().*/
void vBackUpInput (){
    iLineIndex--;
}

/*Outputs the version number of the Pep/8 assembler*/
void vVersionNumber (){
    cerr << "Pep/8 Assembler, version Unix 8.17" << endl;
}

/*Converts a hexadecimal number to a decimal number and returns the decimal number.*/
int iHexToDec (char ch){
    switch (ch){
    case '0': return 0; break;
    case '1': return 1; break;
    case '2': return 2; break;
    case '3': return 3; break;
    case '4': return 4; break;
    case '5': return 5; break;
    case '6': return 6; break;
    case '7': return 7; break;
    case '8': return 8; break;
    case '9': return 9; break;
    case 'A': return 10; break;
    case 'B': return 11; break;
    case 'C': return 12; break;
    case 'D': return 13; break;
    case 'E': return 14; break;
    case 'F': return 15; break;
    default: return -1; // Should not occur
    }
}

/*Converts a decimal number(0-15) to a hexadecimal number(0-F) and returns the hex number.*/
char cDecToHex (int i){
    switch (i){
    case 0: return '0'; break;
    case 1: return '1'; break;
    case 2: return '2'; break;
    case 3: return '3'; break;
    case 4: return '4'; break;
    case 5: return '5'; break;
    case 6: return '6'; break;
    case 7: return '7'; break;
    case 8: return '8'; break;
    case 9: return '9'; break;
    case 10: return 'A'; break;
    case 11: return 'B'; break;
    case 12: return 'C'; break;
    case 13: return 'D'; break;
    case 14: return 'E'; break;
    case 15: return 'F'; break;
    default: return ' '; // Should not occur
    }
}

/*Converts a hexadecimal byte to a positive decimal integer*/
//int iHexByteToDecInt (char cHex[HEX_LENGTH + 1]){
//   return HEX * iHexToDec(cHex[0]) + iHexToDec(cHex[1]);
//}

/*Converts a hexadecimal word to a positive decimal integer*/
int iHexWordToDecInt (char cHex[HEX_LENGTH + 1]){
    return HEX3 * iHexToDec(cHex[0]) + HEX2 * iHexToDec(cHex[1]) + HEX * iHexToDec(cHex[2]) + iHexToDec(cHex[3]);
}

/*Converts a decimal value between -256 to 255 to a hexadecimal array of characters*/
void vDecToHexByte (int iDec, char cHex[BYTE_LENGTH + 1]){
    if (iDec<0){
        iDec=iDec + MAX_BYTE + 1;
    }
    cHex[0]=cDecToHex(iDec / HEX);
    cHex[1]=cDecToHex(iDec % HEX);
    cHex[2]='\0';
}

/*Converts a decimal value between -32768 and 65535 to a hexadecimal array of characters*/
void vDecToHexWord (int iDec, char cHex[ADDR_LENGTH + 1]){
    int iFirstInt;
    int iSecondInt;
    int iThirdInt;
    int iFourthInt;
    if (iDec<0){
        iDec=iDec + MAX_DEC + 1;
    }
    if (iDec>MAX_DEC){
        iDec=iDec - (MAX_DEC + 1);
    }
    iFirstInt=iDec / HEX3;
    iSecondInt=(iDec - HEX3 * iFirstInt) / HEX2;
    iThirdInt=(iDec - HEX3 * iFirstInt - iSecondInt * HEX2) / HEX;
    iFourthInt=(iDec - HEX3 * iFirstInt - iSecondInt * HEX2 - iThirdInt * HEX);
    cHex[0]=cDecToHex(iFirstInt);
    cHex[1]=cDecToHex(iSecondInt);
    cHex[2]=cDecToHex(iThirdInt);
    cHex[3]=cDecToHex(iFourthInt);
    cHex[4]='\0';
}

/*Converts an array of char (decimal constant) to int using FSM implementation.*/
int iCharToInt (char ch []){
    enum CIState {eCIS_START, eCIS_iSign, eCIS_INTEGER};
    CIState state=eCIS_START;
    int iSign;
    int intValue;
    int i=0;
    while (ch[i]!='\0'){
        switch (state) {
        case eCIS_START:
            if (isdigit (ch[i])){
                intValue=ch[i] - '0';
                iSign=1;
                state= eCIS_INTEGER;
            }
            else if (ch[i] == '-'){
                iSign=-1;
                state=eCIS_iSign;
            }
            else if (ch[i] == '+'){
                iSign=1;
                state=eCIS_iSign;
            }
            break;
        case eCIS_iSign:
            if (isdigit(ch[i])){
                intValue=ch[i] - '0';
                state=eCIS_INTEGER;
            }
            break;
        case eCIS_INTEGER:
            if (isdigit (ch[i])){
                intValue=10 * intValue + ch[i] - '0';
            }
            break;
        };
        i++;
    }
    return iSign * intValue;
}

/*Converts an addressing mode to its decimal equivalent.*/
int iAddrModeValue (char addrMode[], bool noAddrModeRequired){
    if ((addrMode[0]=='i') || (addrMode[0]=='\0')){
        return 0;
    }
    else if (addrMode[0]=='d'){
        return 1;
    }
    else if (addrMode[0]=='n'){
        return 2;
    }
    else if (addrMode[0]=='x'){
        if (noAddrModeRequired)/*Branch instructions use a different bit than other instructions to indicate indexed addressing.*/
            return 1;
        else
            return 5;
    }
    else if (addrMode[0]=='s'){
        if (addrMode[1]=='\0'){
            return 3;
        }
        else if (addrMode[1]=='f'){
            return 4;
        }
        else if (addrMode[1]=='x'){
            if (addrMode[2]=='\0'){
                return 6;
            }
            else if (addrMode[2]=='f'){
                return 7;
            }
        }
    }
    return -1; // Should not occur
}

/*True when cAddrMode is a valid addressing mode for a particular instruction.*/
bool bSearchAddrModes (char cAddrMode[], int iAddrMode){
    if (iAddrMode==255)
        return true;
    else if (iAddrMode==0)
        return false;
    else if (cAddrMode[0]=='i')
        return ((iAddrMode & IMMEDIATE) != 0);
    else if (cAddrMode[0]=='d')
        return ((iAddrMode & DIRECT) != 0);
    else if (cAddrMode[0]=='n')
        return ((iAddrMode & INDIRECT) != 0);
    else if (cAddrMode[0]=='x')
        return ((iAddrMode & INDEXED) != 0);
    else if (cAddrMode[0]=='s'){
        if (cAddrMode[1]=='x'){
            if (cAddrMode[2]=='f')
                return ((iAddrMode & STACK_INDEXED_DEFERRED) != 0);
            else
                return ((iAddrMode & STACK_INDEXED) != 0);
        }
        else if (cAddrMode[1]=='f')
            return ((iAddrMode & STACK_RELATIVE_DEFERRED) != 0);
        else
            return ((iAddrMode & STACK_RELATIVE) != 0);
    }
    return false; // Should not occur
}

/*Gives cValue[] the cValue of the symbol named in cID[]*/
/*assertion: symbol has been defined*/
void vGetSymbolValue(char cID[], char cValue[]){
    sSymbolNode* pTemp=pSymbol;
    while (pTemp!=NULL){
        if (strcmp(cID, pTemp->cSymID) == 0){
            strncpy(cValue, pTemp->cSymValue, HEX_LENGTH + 1);
            return;
        }
        pTemp=pTemp->pNext;
    }
}

/*Continues the output following the first line of object code in the assembler*/
/*listing when a .BLOCK command occurs when more than 3 bytes are reserved.*/
void vDotBlockOutputContinued (int iDec){
    int iLineCounter=0;
    int i;
    out_file << endl << "      ";/*6 spaces*/
    for (i=0; i<iDec; i++){
        if (iLineCounter == OBJ_CODE_LENGTH){
            out_file << " " << endl << "      ";/*6 spaces*/
            iLineCounter=0;
        }
        out_file << "00";
        iLineCounter+=2;
    }
    for (i=iLineCounter; i<=OBJ_CODE_LENGTH; i++){
        out_file << " ";
    }
}

/*Continues the output following the first line of object code in the assembler*/
/*listing when a .ASCII command occurs when more than 3 bytes are reserved.*/
void vDotAsciiOutputContinued (char cStr[], const int objLength){
    int i=OBJ_CODE_LENGTH; /*First 6 have already been output in vGenerateHexCode()*/
    int iLineCounter=0;
    while (i< objLength){
        if (iLineCounter>=OBJ_CODE_LENGTH){
            out_file << " " << endl << "      ";/*6 spaces*/
            iLineCounter=0;
        }
        out_file << cStr[i++];
        out_file << cStr[i++];
        iLineCounter+=2; /*A character in ascii takes 2 hex digits*/
    }
    for (i=iLineCounter; i<=OBJ_CODE_LENGTH; i++){
        out_file << " ";
    }
}
/*Gets a line from the trap file*/
void vGetTrapLine (int iLine){
    char ch;
    int i = 0;
    bool bNoPrevI=true;
    bool bNoPrevD=true;
    bool bNoPrevN=true;
    bool bNoPrevS=true;
    bool bNoPrevSF=true;
    bool bNoPrevX=true;
    bool bNoPrevSX=true;
    bool bNoPrevSXF=true;
    sUnimpMnemon[iLine].iAddrMode=0;
    vGetLine();
    vAdvanceInput(ch);
    while (i<IDENT_LENGTH && !isspace(ch)){
        sUnimpMnemon[iLine].cID[i++] = toupper(ch);
        vAdvanceInput(ch);
    }
    sUnimpMnemon[iLine].cID[i]='\0';
    while (!isspace(ch)){
        vAdvanceInput(ch);
    }
    while (isspace(ch) && ch!='\n'){
        vAdvanceInput(ch);
    }
    if (iLine>=UNARY_TRAPS && ch!='\n'){
        do{ /* while ch != '\n' */
            if(toupper(ch)=='I' && bNoPrevI){
                bNoPrevI=false;
                sUnimpMnemon[iLine].iAddrMode+=IMMEDIATE;
                vAdvanceInput(ch);
            }
            else if(toupper(ch)=='D' && bNoPrevD){
                bNoPrevD=false;
                sUnimpMnemon[iLine].iAddrMode+=DIRECT;
                vAdvanceInput(ch);
            }
            else if(toupper(ch)=='N' && bNoPrevN){
                bNoPrevN=false;
                sUnimpMnemon[iLine].iAddrMode+=INDIRECT;
                vAdvanceInput(ch);
            }
            else if(toupper(ch)=='X' && bNoPrevX){
                bNoPrevX=false;
                sUnimpMnemon[iLine].iAddrMode+=INDEXED;
                vAdvanceInput(ch);
            }
            else if(toupper(ch)=='S'){
                vAdvanceInput(ch);
                if(toupper(ch)=='X'){
                    vAdvanceInput(ch);
                    if(toupper(ch)=='F' && bNoPrevSXF){
                        bNoPrevSXF=false;
                        sUnimpMnemon[iLine].iAddrMode+=STACK_INDEXED_DEFERRED;
                        vAdvanceInput(ch);
                    }
                    else if(bNoPrevSX){
                        bNoPrevSX=false;
                        sUnimpMnemon[iLine].iAddrMode+=STACK_INDEXED;
                    }
                }
                else if(toupper(ch)=='F' && bNoPrevSF){
                    bNoPrevSF=false;
                    sUnimpMnemon[iLine].iAddrMode+=STACK_RELATIVE_DEFERRED;
                    vAdvanceInput(ch);
                }
                else if(bNoPrevS){
                    bNoPrevS=false;
                    sUnimpMnemon[iLine].iAddrMode+=STACK_RELATIVE;
                }
            }
            while(!isspace(ch)){
                vAdvanceInput(ch);
            }
            while(isspace(ch) && ch!='\n'){
                vAdvanceInput(ch);
            }
        }while (ch !='\n');
    }
}

/*Buffers for assembler listing*/
/*Buffer for spaces in symbol column in assembler listing*/
void vSymbolBuffer (char cSym[]){
    out_file << ":";
    int i=0;
    while (cSym[i]!='\0'){
        i++;
    }
    for (int j=i; j<IDENT_LENGTH; j++){
        out_file << " ";
    }
}

/*Buffer for spaces in symbol column in symbol table*/
void vSymbolListingBuffer (char cSym[]){
    int i=0;
    while (cSym[i]!='\0'){
        i++;
    }
    for (int j=i; j<=IDENT_LENGTH; j++){
        out_file << " ";
    }
}

/*Buffer for spaces in mnemon column in assembler listing for cDot commands*/
void vDotCommandBuffer (char cDot[]){
    int i=0;
    while (cDot[i]!='\0'){
        i++;
    }
    for (int j=i; j<IDENT_LENGTH - 1; j++){
        out_file << " ";
    }
}

/*Buffer for spaces in mnemon column in assembler listing for cMnemon*/
void vMnemonBuffer (char cMnemon[]){
    int i=0;
    while (cMnemon[i]!='\0'){
        i++;
    }
    for (int j=i; j<IDENT_LENGTH; j++){
        out_file << " ";
    }
}

/*Buffer for spaces in operand column in assembler listing to work with*/
/*operands and addr. modes of various lengths.*/
void vOperandBuffer(char cOperand[], char cAddrMode[], bool bDecSym){
    int iTemp=OPERAND_SPACES;
    if (!bDecSym)
        iTemp-=2;/*0x is 2 spaces*/
    if (cAddrMode[0]!='\0')
        iTemp--; /*comma is one space*/
    int i=0;
    while (cOperand[i++]!='\0'){
        iTemp--;
    }
    i=0;
    while (cAddrMode[i++]!='\0'){
        iTemp--;
    }
    for (int j=iTemp; j>0; j--){
        out_file << " ";
    }
}
/*Buffer for spaces in operand column in assembler listing for*/
/*operands with no addr. mode*/
void vOperandBuffer(char cOperand[], bool bDecSym){
    int iTemp=OPERAND_SPACES;
    if (!bDecSym)
        iTemp-=2;
    int i=0;
    while (cOperand[i++]!='\0'){
        iTemp--;
    }
    for (int j=iTemp; j>0; j--){
        out_file << " ";
    }
}

/*Outputs spaces for a blank address column in the assembler listing*/
void vBlankAddressColumn (){
    out_file << "      ";/*6 spaces*/
}

/*Outputs spaces for a blank object code column in the assembler listing*/
void vBlankObjCodeColumn (){
    out_file << "       ";/*7 spaces*/
}

/*Outputs spaces for a blank symbol column in the assembler listing*/
void vBlankSymbolColumn (){
    out_file << "         ";/*8 spaces*/
}

void vOutputSymbolDecs (){
    if (pSymbol!=NULL){
        if ((pSymbolOutput!=NULL) && (pSymbolOutput->iLine == iSecPassCodeIndex)){
            out_file << pSymbolOutput->cSymID;
            vSymbolBuffer (pSymbolOutput->cSymID);
            pSymbolOutput=pSymbolOutput->pNext;
        }
        else
            vBlankSymbolColumn ();
    }
}

/*Buffer for the object file for the loader*/
void vHexOutputBufferLoader (){
    if (iHexOutputBuffer == OBJ_FILE_LINE_LENGTH - 1){
        out_file << endl;
        iHexOutputBuffer=0;
    }
    else{
        out_file << " ";
        iHexOutputBuffer++;
    }
}

/*Abstract Token Class*/
class AToken {
public:
    virtual Key kTokenType ()=0;
    virtual ~AToken() { };
};

class TAddress : public AToken{
private:
    char cAddrValue[ADDR_MODE_LENGTH + 1];
public:
    TAddress (char str[]) { strncpy (cAddrValue, str, ADDR_MODE_LENGTH + 1); }
    void vGetValue (char str[]) { strncpy (str, cAddrValue, ADDR_MODE_LENGTH + 1); }
    Key kTokenType () { return eT_ADDRMODE; }
};

class TCharConstant : public AToken{
private:
    char cCharValue[CHAR_LENGTH + 1];
    char cCharByteValue[BYTE_LENGTH +1];
public:
    TCharConstant (char str[]) {
        strncpy (cCharValue, str, CHAR_LENGTH + 1);
        if (str[0]=='\\'){
            if (str[1]=='x'){
                cCharByteValue[0]=str[2];
                cCharByteValue[1]=str[3];
                cCharByteValue[2]='\0';
            }
            else if ((str[1]=='\'') || (str[1]=='\"') || (str[1]=='\\'))
                vDecToHexByte(static_cast <int> (str[1]), cCharByteValue);
            else if (str[1]=='b')
                vDecToHexByte(static_cast <int> ('\b'), cCharByteValue);
            else if (str[1]=='f')
                vDecToHexByte(static_cast <int> ('\f'), cCharByteValue);
            else if (str[1]=='n')
                vDecToHexByte(static_cast <int> ('\n'), cCharByteValue);
            else if (str[1]=='r')
                vDecToHexByte(static_cast <int> ('\r'), cCharByteValue);
            else if (str[1]=='t')
                vDecToHexByte(static_cast <int> ('\t'), cCharByteValue);
            else
                vDecToHexByte(static_cast <int> ('\v'), cCharByteValue);
        }
        else
            vDecToHexByte(static_cast <int> (str[0]), cCharByteValue);
    }
    void vGetValue (char str[]) { strncpy (str, cCharValue, CHAR_LENGTH + 1); }
    void vGetByteValue (char str[]) { strncpy (str, cCharByteValue, BYTE_LENGTH + 1); }
    int iLength() { return 1; }/*a char constant is 1 char long*/
    Key kTokenType () { return eT_CHARCONSTANT; }
};

class TComment : public AToken{
private:
    char cCommentValue[COMMENT_LENGTH + 1];
public:
    TComment (char str[]) { strncpy (cCommentValue, str, COMMENT_LENGTH + 1); }
    void vGetValue (char str[]) { strncpy (str, cCommentValue, COMMENT_LENGTH + 1); }
    Key kTokenType () { return eT_COMMENT; }
};

class TDecConstant : public AToken{
private:
    char cDecValue[DEC_LENGTH + 1];
public:
    TDecConstant (char str[]) { strncpy (cDecValue, str, DEC_LENGTH + 1); }
    void vGetValue (char str[]) { strncpy (str, cDecValue, DEC_LENGTH + 1); }
    Key kTokenType () { return eT_DECCONSTANT; }
};

class TDotCommand : public AToken{
private:
    char cDotValue[IDENT_LENGTH + 1];
public:
    TDotCommand (char str[]) { strncpy (cDotValue, str, IDENT_LENGTH + 1); }
    void vGetValue (char str[]) { strncpy (str, cDotValue, IDENT_LENGTH + 1); }
    Key kTokenType () { return eT_DOTCOMMAND; }
};

class TEmpty : public AToken{
public:
    Key kTokenType () { return eT_EMPTY; }
};

class THexConstant : public AToken{
private:
    char cHexValue[HEX_LENGTH + 1];
public:
    THexConstant (char str[]) { strncpy (cHexValue, str, HEX_LENGTH + 1); }
    void vGetValue (char str[]) { strncpy (str, cHexValue, HEX_LENGTH + 1); }
    Key kTokenType () { return eT_HEXCONSTANT; }
};

class TIdentifier : public AToken{
private:
    char cIdentValue[IDENT_LENGTH + 1];
public:
    TIdentifier (char str[]) { strncpy (cIdentValue, str, IDENT_LENGTH + 1); }
    void vGetValue (char str[]) { strncpy (str, cIdentValue, IDENT_LENGTH + 1); }
    Key kTokenType () { return eT_IDENTIFIER; }
};

class TString : public AToken{
private:
    char cStringValue[STRING_LENGTH + 1];
    char cStringByteValue[2 * STRING_LENGTH +1];/*each char is 2 hex digits*/
    char cTempByteValue[BYTE_LENGTH + 1];
    int i;/*string length*/
    int j;/*object length (how many hex digits)*/
public:
    TString (char str[]) {
        strncpy (cStringValue, str, STRING_LENGTH + 1);
        i=0; j=0;
        do{
            if (str[i] == '\\') {
                int k=i+1;
                if (str[k] == 'x') {
                    i+=2;
                    cStringByteValue[j++]=str[i++];
                    cStringByteValue[j++]=str[i++];
                }
                else if ((str[k] == '\'') || (str[k] == '\"') || (str[k] == '\\')){
                    vDecToHexByte(static_cast <int> (str[k]), cTempByteValue);
                    cStringByteValue[j++]=cTempByteValue[0];
                    cStringByteValue[j++]=cTempByteValue[1];
                    i+=2;
                }
                else if (str[k] == 'b'){
                    vDecToHexByte(static_cast <int> ('\b'), cTempByteValue);
                    cStringByteValue[j++]=cTempByteValue[0];
                    cStringByteValue[j++]=cTempByteValue[1];
                    i+=2;
                }
                else if (str[k] == 'f'){
                    vDecToHexByte(static_cast <int> ('\f'), cTempByteValue);
                    cStringByteValue[j++]=cTempByteValue[0];
                    cStringByteValue[j++]=cTempByteValue[1];
                    i+=2;
                }
                else if (str[k] == 'n'){
                    vDecToHexByte(static_cast <int> ('\n'), cTempByteValue);
                    cStringByteValue[j++]=cTempByteValue[0];
                    cStringByteValue[j++]=cTempByteValue[1];
                    i+=2;
                }
                else if (str[k] == 'r'){
                    vDecToHexByte(static_cast <int> ('\r'), cTempByteValue);
                    cStringByteValue[j++]=cTempByteValue[0];
                    cStringByteValue[j++]=cTempByteValue[1];
                    i+=2;
                }
                else if (str[k] == 't'){
                    vDecToHexByte(static_cast <int> ('\t'), cTempByteValue);
                    cStringByteValue[j++]=cTempByteValue[0];
                    cStringByteValue[j++]=cTempByteValue[1];
                    i+=2;
                }
                else{
                    vDecToHexByte(static_cast <int> ('\v'), cTempByteValue);
                    cStringByteValue[j++]=cTempByteValue[0];
                    cStringByteValue[j++]=cTempByteValue[1];
                    i+=2;
                }
            }
            else{
                vDecToHexByte(static_cast <int> (str[i++]), cTempByteValue);
                cStringByteValue[j++]=cTempByteValue[0];
                cStringByteValue[j++]=cTempByteValue[1];
            }
        }
        while (str[i]!='\0');
        cStringByteValue[j]='\0';
    }
    void vGetValue (char str[]) { strncpy (str, cStringValue, STRING_LENGTH + 1); }
    void vGetObjValue (char str[], const int length) {
        strncpy (str, cStringByteValue, length);
        str[length]='\0';
    }
    int iLength() { return i; }
    int iObjLength() { return j; }
    Key kTokenType () { return eT_STRING; }
};

class TSymbol : public AToken{
private:
    char cSymbolValue[IDENT_LENGTH + 1];
public:
    TSymbol (char str[]) { strncpy (cSymbolValue, str, IDENT_LENGTH + 1); }
    void vGetValue (char str[]) { strncpy (str, cSymbolValue, IDENT_LENGTH + 1); }
    Key kTokenType () { return eT_SYMBOL; }
};

class TInvalid : public AToken{
public:
    Key kTokenType () { return eT_INVALID; }
};

class TInvalidAddr : public AToken{
public:
    Key kTokenType () { return eT_INVALIDADDR; }
};

class TInvalidChar : public AToken{
public:
    Key kTokenType () { return eT_INVALIDCHAR; }
};

class TInvalidComment : public AToken{
public:
    Key kTokenType () { return eT_INVALIDCOMMENT; }
};

class TInvalidDec : public AToken{
public:
    Key kTokenType () { return eT_INVALIDDEC; }
};

class TInvalidDotCommand : public AToken{
public:
    Key kTokenType () { return eT_INVALIDDOTCOMMAND; }
};

class TInvalidHex : public AToken{
public:
    Key kTokenType () { return eT_INVALIDHEX; }
};

class TInvalidString : public AToken{
public:
    Key kTokenType () { return eT_INVALIDSTRING; }
};

/*Abstract Mnemonic Class*/
class AMnemon{
public:
    virtual int iOpCode ()=0; /*Returns the integer operation code of a given mnemonic*/
    virtual void vMnemonOutput ()=0; /*Outputs the mnemonic for the assembler listing*/
    virtual bool bNoAddrModeRequired ()=0; /*For nonunary instructions, true when no addressing mode is required*/
    virtual bool bValidAddrMode (char ch[])=0;/*True when given addressing mode is valid for the instruction*/
    virtual bool bIsUnary ()=0;/*True when the instruction is unary*/
    virtual ~AMnemon() { };
};

class UnaryOp : public AMnemon{
public:
    bool bValidAddrMode (char ch[]) {return false; }
    bool bIsUnary () {return true; }
    bool bNoAddrModeRequired () {return false; }
};

class BranchOp : public AMnemon{
public: 
    bool bValidAddrMode (char ch[]) {return ((ch[0] == 'i') || (ch[0] == 'x')); }
    bool bIsUnary () {return false; }
    bool bNoAddrModeRequired () {return true; }
};

class GeneralOp : public AMnemon{
public: 
    bool bValidAddrMode (char ch[]) {return true; }
    bool bIsUnary () {return false; }
    bool bNoAddrModeRequired () {return false; }
};

class InputOp : public AMnemon{
public: 
    bool bValidAddrMode (char ch[]) {return (ch[0] != 'i'); }
    bool bIsUnary () {return false; }
    bool bNoAddrModeRequired () {return false; }
};

class Stop : public UnaryOp{
public:
    int iOpCode () { return 0; }
    void vMnemonOutput () { out_file << "STOP    "; }
};

class Rettr : public UnaryOp{
public:
    int iOpCode () { return 1; }
    void vMnemonOutput () { out_file << "RETTR   "; }
};

class Movspa : public UnaryOp{
public:
    int iOpCode () { return 2; }
    void vMnemonOutput () { out_file << "MOVSPA  "; }
};

class Movflga : public UnaryOp{
public:
    int iOpCode () { return 3; }
    void vMnemonOutput () { out_file << "MOVFLGA "; }
};

class Br : public BranchOp{
public:
    int iOpCode () { return 4; }
    void vMnemonOutput () { out_file << "BR      "; }
};

class Brle : public BranchOp{
public:
    int iOpCode () { return 6; }
    void vMnemonOutput () { out_file << "BRLE    "; }
};

class Brlt : public BranchOp{
public:
    int iOpCode () { return 8; }
    void vMnemonOutput () { out_file << "BRLT    "; }
};

class Breq : public BranchOp{
public:
    int iOpCode () { return 10; }
    void vMnemonOutput () { out_file << "BREQ    "; }
};

class Brne : public BranchOp{
public:
    int iOpCode () { return 12; }
    void vMnemonOutput () { out_file << "BRNE    "; }
};

class Brge : public BranchOp{
public:
    int iOpCode () { return 14; }
    void vMnemonOutput () { out_file << "BRGE    "; }
};

class Brgt : public BranchOp{
public:
    int iOpCode () { return 16; }
    void vMnemonOutput () { out_file << "BRGT    "; }
};

class Brv : public BranchOp{
public:
    int iOpCode () { return 18; }
    void vMnemonOutput () { out_file << "BRV     "; }
};

class Brc : public BranchOp{
public:
    int iOpCode () { return 20; }
    void vMnemonOutput () { out_file << "BRC     "; }
};

class Call : public BranchOp{
public:
    int iOpCode () { return 22; }
    void vMnemonOutput () { out_file << "CALL    "; }
};

class Nota : public UnaryOp{
public:
    int iOpCode () { return 24; }
    void vMnemonOutput () { out_file << "NOTA    "; }
};

class Notx : public UnaryOp{
public:
    int iOpCode () { return 25; }
    void vMnemonOutput () { out_file << "NOTX    "; }
};

class Nega : public UnaryOp{
public:
    int iOpCode () { return 26; }
    void vMnemonOutput () { out_file << "NEGA    "; }
};

class Negx : public UnaryOp{
public:
    int iOpCode () { return 27; }
    void vMnemonOutput () { out_file << "NEGX    "; }
};

class Asla : public UnaryOp{
public:
    int iOpCode () { return 28; }
    void vMnemonOutput () { out_file << "ASLA    "; }
};

class Aslx : public UnaryOp{
public:
    int iOpCode () { return 29; }
    void vMnemonOutput () { out_file << "ASLX    "; }
};

class Asra : public UnaryOp{
public:
    int iOpCode () { return 30; }
    void vMnemonOutput () { out_file << "ASRA    "; }
};

class Asrx : public UnaryOp{
public:
    int iOpCode () { return 31; }
    void vMnemonOutput () { out_file << "ASRX    "; }
};

class Rola : public UnaryOp{
public:
    int iOpCode () { return 32; }
    void vMnemonOutput () { out_file << "ROLA    "; }
};

class Rolx : public UnaryOp{
public:
    int iOpCode () { return 33; }
    void vMnemonOutput () { out_file << "ROLX    "; }
};

class Rora : public UnaryOp{
public:
    int iOpCode () { return 34; }
    void vMnemonOutput () { out_file << "RORA    "; }
};

class Rorx : public UnaryOp{
public:
    int iOpCode () { return 35; }
    void vMnemonOutput () { out_file << "RORX    "; }
};

class Unimp0 : public AMnemon{ /*default Nop0*/
public:
    int iOpCode () { return 36; }
    bool bValidAddrMode(char ch[]) {return bSearchAddrModes(ch, sUnimpMnemon[0].iAddrMode);}
    bool bNoAddrModeRequired () {return false; }
    bool bIsUnary () {return true; }
    void vMnemonOutput () {
        out_file << sUnimpMnemon[0].cID; 
        vMnemonBuffer(sUnimpMnemon[0].cID);
    }
};

class Unimp1 : public AMnemon{ /*default Nop1*/
public:
    int iOpCode () { return 37; }
    bool bValidAddrMode(char ch[]) {return bSearchAddrModes(ch, sUnimpMnemon[1].iAddrMode);}
    bool bNoAddrModeRequired () {return false; }
    bool bIsUnary () {return true; }
    void vMnemonOutput () {
        out_file << sUnimpMnemon[1].cID; 
        vMnemonBuffer(sUnimpMnemon[1].cID);
    }
};

class Unimp2 : public AMnemon{ /*default Nop2*/
public:
    int iOpCode () { return 38; }
    bool bValidAddrMode(char ch[]) {return bSearchAddrModes(ch, sUnimpMnemon[2].iAddrMode);}
    bool bNoAddrModeRequired () {return false; }
    bool bIsUnary () {return true; }
    void vMnemonOutput () {
        out_file << sUnimpMnemon[2].cID; 
        vMnemonBuffer(sUnimpMnemon[2].cID);
    }
};

class Unimp3 : public AMnemon{ /*default Nop3*/
public:
    int iOpCode () { return 39; }
    bool bValidAddrMode(char ch[]) {return bSearchAddrModes(ch, sUnimpMnemon[3].iAddrMode);}
    bool bNoAddrModeRequired () {return false; }
    bool bIsUnary () {return true; }
    void vMnemonOutput () {
        out_file << sUnimpMnemon[3].cID; 
        vMnemonBuffer(sUnimpMnemon[3].cID);
    }
};

class Unimp4 : public AMnemon{ /*default Nop*/
public:
    int iOpCode () { return 40; }
    bool bValidAddrMode(char ch[]) {return bSearchAddrModes(ch, sUnimpMnemon[4].iAddrMode);}
    bool bNoAddrModeRequired () {return false; }
    bool bIsUnary () {return (sUnimpMnemon[4].iAddrMode==0); }
    void vMnemonOutput () {
        out_file << sUnimpMnemon[4].cID; 
        vMnemonBuffer(sUnimpMnemon[4].cID);
    }
};

class Unimp5 : public AMnemon{ /*default Deci*/
public:
    int iOpCode () { return 48; }
    bool bValidAddrMode(char ch[]) {return bSearchAddrModes(ch, sUnimpMnemon[5].iAddrMode);}
    bool bNoAddrModeRequired () {return false; }
    bool bIsUnary () {return (sUnimpMnemon[5].iAddrMode==0); }
    void vMnemonOutput () {
        out_file << sUnimpMnemon[5].cID; 
        vMnemonBuffer(sUnimpMnemon[5].cID);
    }
};

class Unimp6 : public AMnemon{ /*default Deco*/
public:
    int iOpCode () { return 56; }
    bool bValidAddrMode(char ch[]) {return bSearchAddrModes(ch, sUnimpMnemon[6].iAddrMode);}
    bool bNoAddrModeRequired () {return false; }
    bool bIsUnary () {return (sUnimpMnemon[6].iAddrMode==0); }
    void vMnemonOutput () {
        out_file << sUnimpMnemon[6].cID; 
        vMnemonBuffer(sUnimpMnemon[6].cID);
    }
};

class Unimp7 : public AMnemon{ /*default Stro*/
public:
    int iOpCode () { return 64; }
    bool bValidAddrMode(char ch[]) {return bSearchAddrModes(ch, sUnimpMnemon[7].iAddrMode);}
    bool bNoAddrModeRequired () {return false; }
    bool bIsUnary () {return (sUnimpMnemon[7].iAddrMode==0); }
    void vMnemonOutput () {
        out_file << sUnimpMnemon[7].cID; 
        vMnemonBuffer(sUnimpMnemon[7].cID);
    }
};

class Chari : public InputOp{
public:
    int iOpCode () { return 72; }
    void vMnemonOutput () { out_file << "CHARI   "; }
};

class Charo : public GeneralOp{
public:
    int iOpCode () { return 80; }
    void vMnemonOutput () { out_file << "CHARO   "; }
};

class Ret0 : public UnaryOp{
public:
    int iOpCode () { return 88; }
    void vMnemonOutput () { out_file << "RET0    "; }
};

class Ret1 : public UnaryOp{
public:
    int iOpCode () { return 89; }
    void vMnemonOutput () { out_file << "RET1    "; }
};

class Ret2 : public UnaryOp{
public:
    int iOpCode () { return 90; }
    void vMnemonOutput () { out_file << "RET2    "; }
};

class Ret3 : public UnaryOp{
public:
    int iOpCode () { return 91; }
    void vMnemonOutput () { out_file << "RET3    "; }
};

class Ret4 : public UnaryOp{
public:
    int iOpCode () { return 92; }
    void vMnemonOutput () { out_file << "RET4    "; }
};

class Ret5 : public UnaryOp{
public:
    int iOpCode () { return 93; }
    void vMnemonOutput () { out_file << "RET5    "; }
};

class Ret6 : public UnaryOp{
public:
    int iOpCode () { return 94; }
    void vMnemonOutput () { out_file << "RET6    "; }
};

class Ret7 : public UnaryOp{
public:
    int iOpCode () { return 95; }
    void vMnemonOutput () { out_file << "RET7    "; }
};

class Addsp : public GeneralOp{
public:
    int iOpCode () { return 96; }
    void vMnemonOutput () { out_file << "ADDSP   "; }
};

class Subsp : public GeneralOp{
public:
    int iOpCode () { return 104; }
    void vMnemonOutput () { out_file << "SUBSP   "; }
};

class Adda : public GeneralOp{
public:
    int iOpCode () { return 112; }
    void vMnemonOutput () { out_file << "ADDA    "; }
};

class Addx : public GeneralOp{
public:
    int iOpCode () { return 120; }
    void vMnemonOutput () { out_file << "ADDX    "; }
};

class Suba : public GeneralOp{
public:
    int iOpCode () { return 128; }
    void vMnemonOutput () { out_file << "SUBA    "; }
};

class Subx : public GeneralOp{
public:
    int iOpCode () { return 136; }
    void vMnemonOutput () { out_file << "SUBX    "; }
};

class Anda : public GeneralOp{
public:
    int iOpCode () { return 144; }
    void vMnemonOutput () { out_file << "ANDA    "; }
};

class Andx : public GeneralOp{
public:
    int iOpCode () { return 152; }
    void vMnemonOutput () { out_file << "ANDX    "; }
};

class Ora : public GeneralOp{
public:
    int iOpCode () { return 160; }
    void vMnemonOutput () { out_file << "ORA     "; }
};

class Orx : public GeneralOp{
public:
    int iOpCode () { return 168; }
    void vMnemonOutput () { out_file << "ORX     "; }
};

class Cpa : public GeneralOp{
public:
    int iOpCode () { return 176; }
    void vMnemonOutput () { out_file << "CPA     "; }
};

class Cpx : public GeneralOp{
public:
    int iOpCode () { return 184; }
    void vMnemonOutput () { out_file << "CPX     "; }
};

class Lda : public GeneralOp{
public:
    int iOpCode () { return 192; }
    void vMnemonOutput () { out_file << "LDA     "; }
};

class Ldx : public GeneralOp{
public:
    int iOpCode () { return 200; }
    void vMnemonOutput () { out_file << "LDX     "; }
};

class Ldbytea : public GeneralOp{
public:
    int iOpCode () { return 208; }
    void vMnemonOutput () { out_file << "LDBYTEA "; }
};

class Ldbytex : public GeneralOp{
public:
    int iOpCode () { return 216; }
    void vMnemonOutput () { out_file << "LDBYTEX "; }
};

class Sta : public InputOp{
public:
    int iOpCode () { return 224; }
    void vMnemonOutput () { out_file << "STA     "; }
};

class Stx : public InputOp{
public:
    int iOpCode () { return 232; }
    void vMnemonOutput () { out_file << "STX     "; }
};

class Stbytea : public InputOp{
public:
    int iOpCode () { return 240; }
    void vMnemonOutput () { out_file << "STBYTEA "; }
};

class Stbytex : public InputOp{
public:
    int iOpCode () { return 248; }
    void vMnemonOutput () { out_file << "STBYTEX "; }
};

/*Abstract Code Class*/

class ACode{
public:
    virtual ~ACode () {};
    virtual bool bIsError ()=0;
    virtual void vGenerateCode ()=0;
};

/*Global variables, part 2*/
AToken* pPrevAT=new TEmpty; /*Used to detect strings in vGetToken() if .ASCII was previous token*/
ACode* pACode[MAX_LINES + 1];/*Array of pointers to abstract code*/
int iCodeIndex; /*Used as index of pACode and pAMnemon arrays*/

/*Error class*/

class Error : public ACode{
public:
    bool bIsError () { return true; }
};

class eNoEnd : public Error
{
public:
    void vGenerateCode (){
        cerr << "Missing .END sentinal" << endl;
    }
};

class eTooLong : public Error{
public:
    void vGenerateCode (){
        cerr << "Program too long. Listing table overflow." << endl;
    }
};

class eSymPrevDef : public Error{
public:
    void vGenerateCode (){
        cerr << "Symbol previously defined." << endl;
    }
};

class eProgTooLong : public Error{
public:
    void vGenerateCode (){
        cerr << "Program too long. Code table overflow." << endl;
    }
};

class eInstrDotExp : public Error{
public:
    void vGenerateCode (){
        cerr << "Instruction or dot command expected." << endl;
    }
};

class eInvSyntax : public Error{
public:
    void vGenerateCode (){
        cerr << "Invalid syntax." << endl;
    }
};

class eSymInstrDotExp : public Error{
public:
    void vGenerateCode (){
        cerr << "Symbol, instruction, or dot command expected." << endl;
    }
};

class eInvMnemon : public Error{
public:
    void vGenerateCode (){
        cerr << "Invalid Mnemonic." << endl;
    }
};

class eCommExp : public Error{
public:
    void vGenerateCode (){
        cerr << "Comment expected." << endl;
    }
};

class eCommentTooLong : public Error{
public:
    void vGenerateCode (){
        cerr << "Comment too long." << endl;
    }
};

class eOprndSpecExp : public Error{
public:
    void vGenerateCode (){
        cerr << "Operand specifier expected." << endl;
    }
};

class eNoDecConst : public Error{
public:
    void vGenerateCode (){
        cerr << "Invalid decimal constant." << endl;
    }
};

class eNoHexConst : public Error{
public:
    void vGenerateCode (){
        cerr << "Invalid hexadecimal constant." << endl;
    }
};

class eNoCharConst : public Error{
public:
    void vGenerateCode (){
        cerr << "Invalid character constant." << endl;
    }
};

class eAddrExp : public Error{
public:
    void vGenerateCode (){
        cerr << "Addressing mode expected." << endl;
    }
};

class eAddrCommExp : public Error{
public:
    void vGenerateCode (){
        cerr << "Addressing mode or comment expected." << endl;
    }
};

class eNoAddr : public Error{
public:
    void vGenerateCode (){
        cerr << "Invalid addressing mode." << endl;
    }
};

class eNoAddrmode : public Error{
public:
    void vGenerateCode (){
        cerr << "This instruction cannot have this addressing mode." << endl;
    }
};

class eDecOverflow : public Error{
public:
    void vGenerateCode (){
        cerr << "Decimal overflow. Range is -32768 to 65535." << endl;
    }
};

class eNoDotCom : public Error{
public:
    void vGenerateCode (){
        cerr << "Invalid dot command." << endl;
    }
};

class eNoString : public Error{
public:
    void vGenerateCode (){
        cerr << "Invalid string expression." << endl;
    }
};

class eDecHexExp : public Error{
public:
    void vGenerateCode (){
        cerr << "Decimal or hex constant expected." << endl;
    }
};

class eConstExp : public Error{
public:
    void vGenerateCode (){
        cerr << "Constant expected." << endl;
    }
};

class eNoAddrModeWithChar : public Error{
public:
    void vGenerateCode (){
        cerr << "Addressing mode always required with char constant operands." << endl;
    }
};

class eNoAddrModeWithString : public Error{
public:
    void vGenerateCode (){
        cerr << "Addressing mode always required with string operands." << endl;
    }
};

class eSymExpWithAddrss : public Error{
public:
    void vGenerateCode (){
        cerr << "Symbol required after .ADDRSS pseudo-op." << endl;
    }
};

class eSymBeforeEquate : public Error{
public:
    void vGenerateCode (){
        cerr << "Symbol required before .EQUATE pseudo-op." << endl;
    }
};

class eConstOverflow : public Error{
public:
    void vGenerateCode (){
        cerr << "Constant overflow. Range is 0 to 255 (dec)." << endl;
    }
};

class eByteOutOfRange : public Error{
public:
    void vGenerateCode (){
        cerr << "Byte value out of range." << endl;
    }
};

class eSymNotDefined : public Error{
public:
    void vGenerateCode (){
        cerr << "Reference to undefined symbol." << endl;
    }
};

class eAddrOverflow : public Error{
public:
    void vGenerateCode (){
        cerr << "Address overflow. Range is 0 to 65535 (dec)." << endl;
    }
};

class eOneBurn : public Error{
public:
    void vGenerateCode (){
        cerr << "More than one .BURN pseudo-op not allowed in program." << endl;
    }
};

class eStrOprndTooLong : public Error{
public:
    void vGenerateCode (){
        cerr << "The string is too long to be a valid operand." << endl;
    }
};

class eByteStrTooLong : public Error{
public:
    void vGenerateCode (){
        cerr << "The string is too long to be used with .BYTE pseudo-op." << endl;
    }
};

class eWordStrTooLong : public Error{
public:
    void vGenerateCode (){
        cerr << "The string is too long to be used with .WORD pseudo-op." << endl;
    }
};
class eEquateStrTooLong : public Error{
public:
    void vGenerateCode (){
        cerr << "The string is too long to be used with .EQUATE pseudo-op." << endl;
    }
};

class eOperandUnexp : public Error{
public:
    void vGenerateCode (){
        cerr << "Unexpected operand specifier." << endl;
    }
};

/*Valid Class*/
class Valid : public ACode{
protected:
    Mnemon mnemonic;
    DotCommand dotcom;
public:
    virtual ~Valid () {};
    bool bIsError () { return false; } /*Since the valid class would not contain errors*/
    virtual int iAddressCounter ()=0; /*Returns how many bytes each class takes up*/
    virtual void vGenerateHexCode (bool asemList)=0; /*Generates the object code*/
    virtual void vBurnAddressChange ()=0; /*Changes iAddress to account for a .BURN*/
};

class ZeroArg : public Valid{
public:
    ZeroArg (DotCommand dot) { dotcom=dot; }
    int iAddressCounter () { return 0; }
    void vBurnAddressChange () {}
    void vGenerateCode () { out_file << "             "; }/*13 spaces*/
    void vGenerateHexCode (bool asemList) { }
};

class DotEnd : public Valid{
private:
    int iAddress;
    char cFirstArg[IDENT_LENGTH + 1];
public:
    DotEnd (int iAddr, DotCommand dot, char fArg[]){
        iAddress=iAddr;
        dotcom=dot; 
        strncpy (cFirstArg, fArg, IDENT_LENGTH + 1);
    }
    int iAddressCounter () { return 0; }
    void vBurnAddressChange () { iAddress+=iBurnStart; }
    void vGenerateCode (){
        char cAddr[ADDR_LENGTH + 1];
        vDecToHexWord (iAddress, cAddr);
        out_file << cAddr << "  ";
        vBlankObjCodeColumn();
        vOutputSymbolDecs();
        out_file << "." << cFirstArg;
        vDotCommandBuffer(cFirstArg);
        out_file << "              ";/*14 spaces*/
    }
    void vGenerateHexCode (bool asemList) { }
};

class UnaryInstruction : public Valid{
private:
    int iAddress;
    AMnemon* pAMnemonic;
    char cFirstArg[IDENT_LENGTH + 1];
public:
    UnaryInstruction (int iAddr, Mnemon mn, AMnemon* pAMnemonTemp, char fArg[]){
        iAddress=iAddr;
        mnemonic=mn;
        pAMnemonic=pAMnemonTemp;
        strncpy (cFirstArg, fArg, IDENT_LENGTH + 1);
    }
    ~UnaryInstruction () { delete pAMnemonic; }
    int iAddressCounter () { return UNARY; }
    void vBurnAddressChange () { iAddress+=iBurnStart; }
    void vGenerateCode (){
        char cAddr[ADDR_LENGTH + 1];
        vDecToHexWord (iAddress, cAddr);
        out_file << cAddr << "  ";
        if ((iBurnCounter == 0) || (iAddress>=iBurnAddr)){
            vGenerateHexCode(true);
        }
        else{
            vBlankObjCodeColumn();
        }
        vOutputSymbolDecs();
        pAMnemonic->vMnemonOutput();
        out_file << "              ";/*14 spaces*/
    }
    void vGenerateHexCode (bool asemList){
        char cTemp[BYTE_LENGTH + 1];
        vDecToHexByte(pAMnemonic->iOpCode(), cTemp);
        if (asemList){
            out_file << cTemp << "     ";/*5 spaces*/
        }
        else{
            if ((iBurnCounter == 0) || (iAddress>=iBurnAddr)){
                out_file << cTemp;
                vHexOutputBufferLoader();
            }
        }
    }
};

class DotComDec : public Valid{
private:
    int iAddress;
    char cFirstArg[IDENT_LENGTH + 1];
    char cSecondArg[DEC_LENGTH + 1];
public:
    DotComDec (int iAddr, DotCommand dot, char fArg[], char sArg[]){
        iAddress=iAddr;
        dotcom=dot;
        strncpy (cFirstArg, fArg, IDENT_LENGTH + 1);
        strncpy (cSecondArg, sArg, DEC_LENGTH + 1);
    }
    int iAddressCounter(){
        switch (dotcom){
        case eD_BLOCK:
            return iCharToInt(cSecondArg);
            break;
        case eD_BURN:
            return 0;
            break;
        case eD_BYTE:
            return BYTE;
            break;
        case eD_EQUATE:
            return 0;
            break;
        case eD_WORD:
            return WORD;
            break;
        default:
            return -1; // Should not occur
        } 
    }
    void vBurnAddressChange () { iAddress+=iBurnStart; }
    void vGenerateCode (){
        int iDec=iCharToInt(cSecondArg);
        char cAddr[ADDR_LENGTH + 1];
                
        if (dotcom!=eD_EQUATE){
            vDecToHexWord (iAddress, cAddr);
            out_file << cAddr << "  ";
        }
        else{
            out_file << "      ";/*6 spaces*/
        }
        if ((iBurnCounter == 0) || (iAddress>=iBurnAddr)){
            vGenerateHexCode(true);
        }
        else{
            vBlankObjCodeColumn();
        }
        vOutputSymbolDecs();
        out_file << "." << cFirstArg;
        vDotCommandBuffer(cFirstArg);
        out_file << cSecondArg;
        vOperandBuffer(cSecondArg, true);
        if ((pComment!=NULL) && (pComment->iLine == iSecPassCodeIndex)){
            if (pComment->bNonemptyLine){
                if (pSymbol == NULL){
                    pComment->cComment[COMMENT_LENGTH_NONEMPTY_NO_SYMBOLS - 1]='\0';
                }
                else{
                    pComment->cComment[COMMENT_LENGTH_NONEMPTY - 1]='\0';
                }
            }
            out_file << ";" << pComment->cComment;
            sCommentNode* p=pComment;
            pComment=pComment->pNext;
            delete p;
        }
        if ((dotcom == eD_BLOCK) && (iDec>OBJ_CODE_LENGTH / BYTE_LENGTH) &&
            ((iBurnCounter == 0) || (iAddress>=iBurnAddr))){
            vDotBlockOutputContinued(iDec - OBJ_CODE_LENGTH / BYTE_LENGTH);
        }
    }
    void vGenerateHexCode (bool asemList){
        int i;
        int iDec=iCharToInt(cSecondArg);
        char cVal[ADDR_LENGTH + 1];
        int lineCounter=0;
        if (asemList){
            switch (dotcom){
            case eD_BLOCK:
                if (iDec<=OBJ_CODE_LENGTH / BYTE_LENGTH){
                    for (i=0; i<iDec; i++){
                        out_file << "00";
                        lineCounter += 2;
                    }
                    for (i=lineCounter; i<=OBJ_CODE_LENGTH; i++){
                        out_file << " ";
                    }
                }
                else{
                    for (i=0; i<OBJ_CODE_LENGTH / BYTE_LENGTH; i++){
                        out_file << "00";
                    }
                    out_file << " ";
                }
                break;
            case eD_BURN:
                vBlankObjCodeColumn();
                break;
            case eD_BYTE:
                vDecToHexByte(iDec, cVal);
                out_file << cVal << "     ";/*5 spaces*/
                break;
            case eD_EQUATE:
                vBlankObjCodeColumn();
                break;
            case eD_WORD:
                vDecToHexWord(iDec, cVal);
                out_file << cVal << "   ";
                break;
            default:
                break; // Should not occur
            }
        }
        else{
            if ((iBurnCounter == 0) || (iAddress>=iBurnAddr)){
                switch (dotcom){
                case eD_BLOCK:
                    for (i=0; i<iDec; i++){
                        out_file << "00";
                        vHexOutputBufferLoader();
                    }
                    break;
                case eD_BURN:
                    break;
                case eD_BYTE:
                    vDecToHexByte(iDec, cVal);
                    out_file << cVal;
                    vHexOutputBufferLoader();
                    break;
                case eD_EQUATE:
                    break;
                case eD_WORD:
                    vDecToHexWord(iDec, cVal);
                    out_file << cVal[0] << cVal[1];
                    vHexOutputBufferLoader();
                    out_file << cVal[2] << cVal[3];
                    vHexOutputBufferLoader();
                    break;
                default:
                    break; // Should not occur
                }
            }
        }
    }
};

class DotComHex : public Valid{
private:
    int iAddress;
    char cFirstArg[IDENT_LENGTH + 1];
    char cSecondArg[HEX_LENGTH + 1];
public:
    DotComHex (int iAddr, DotCommand dot, char fArg[], char sArg[]){
        iAddress=iAddr;
        dotcom=dot;
        strncpy (cFirstArg, fArg, IDENT_LENGTH + 1);
        strncpy (cSecondArg, sArg, HEX_LENGTH + 1);
    }
    int iAddressCounter (){
        switch (dotcom){
        case eD_BLOCK:
            cSecondArg[0]='0';
            cSecondArg[1]='0';
            return iHexWordToDecInt(cSecondArg);
            break;
        case eD_BURN:
            return 0;
            break;
        case eD_EQUATE:
            return 0;
            break;
        case eD_WORD:
            return WORD;
            break;
        case eD_BYTE:
            return BYTE;
            break;
        default:
            break; // Should not occur
        }
        return -1; // Should not occur
    }
    void vBurnAddressChange () { iAddress+=iBurnStart; }
    void vGenerateCode (){
        int iDec=iHexWordToDecInt(cSecondArg);
        char cAddr[ADDR_LENGTH + 1];
        if (dotcom!=eD_EQUATE){
            vDecToHexWord (iAddress, cAddr);
            out_file << cAddr << "  ";
        }
        else{
            out_file << "      ";/*6 spaces*/
        }
        if ((iBurnCounter == 0) || (iAddress>=iBurnAddr)){
            vGenerateHexCode(true);
        }
        else{
            vBlankObjCodeColumn();
        }
        vOutputSymbolDecs();
        out_file << "." << cFirstArg;
        vDotCommandBuffer(cFirstArg);
        if (dotcom!=eD_BYTE){
            out_file << "0x" << cSecondArg;
            vOperandBuffer(cSecondArg, false);
        }
        else{
            out_file << "0x" << cSecondArg[2] << cSecondArg[3];
            vOperandBuffer(cSecondArg, true);/*true compensates for the 2 spaces*/
        }
        if ((pComment!=NULL) && (pComment->iLine == iSecPassCodeIndex)){
            if (pComment->bNonemptyLine){
                if (pSymbol == NULL){
                    pComment->cComment[COMMENT_LENGTH_NONEMPTY_NO_SYMBOLS - 1]='\0';
                }
                else{
                    pComment->cComment[COMMENT_LENGTH_NONEMPTY - 1]='\0';
                }
            }
            out_file << ";" << pComment->cComment;
            sCommentNode* p=pComment;
            pComment=pComment->pNext;
            delete p;
        }
        if ((dotcom == eD_BLOCK) && (iDec>OBJ_CODE_LENGTH / BYTE_LENGTH) && ((iBurnCounter == 0) || (iAddress>=iBurnAddr))){
            vDotBlockOutputContinued(iDec - OBJ_CODE_LENGTH / BYTE_LENGTH);
        }
    }
    void vGenerateHexCode (bool asemList){
        int i;
        int iDec=iHexWordToDecInt(cSecondArg);
        int lineCounter=0;
        if (asemList){
            switch (dotcom){
            case eD_BLOCK:
                if (iDec<=OBJ_CODE_LENGTH / BYTE_LENGTH){
                    for (i=0; i<iDec; i++){
                        out_file << "00";
                        lineCounter=lineCounter + 2;
                    }
                    for (i=lineCounter; i<=OBJ_CODE_LENGTH; i++){
                        out_file << " ";
                    }
                }
                else{
                    for (i=0; i<OBJ_CODE_LENGTH / BYTE_LENGTH; i++){
                        out_file << "00";
                    }
                    out_file << " ";
                }
                break;
            case eD_BURN:
                vBlankObjCodeColumn();
                break;
            case eD_EQUATE:
                vBlankObjCodeColumn();
                break;
            case eD_WORD:
                out_file << cSecondArg << "   ";
                break;
            case eD_BYTE:
                out_file << cSecondArg[2] << cSecondArg[3] << "     ";/*5 spaces*/
                break;
            default:
                break; // Should not occur
            }
        }
        else{
            if ((iBurnCounter == 0) || (iAddress>=iBurnAddr)){
                switch (dotcom){
                case eD_BLOCK:
                    for (i=0; i<iDec; i++){
                        out_file << "00";
                        vHexOutputBufferLoader();
                    }
                    break;
                case eD_BURN:
                    break;
                case eD_EQUATE:
                    break;
                case eD_WORD:
                    out_file << cSecondArg[0] << cSecondArg[1];
                    vHexOutputBufferLoader();
                    out_file << cSecondArg[2] << cSecondArg[3];
                    vHexOutputBufferLoader();
                    break;
                case eD_BYTE:
                    out_file << cSecondArg[2] << cSecondArg[3];
                    vHexOutputBufferLoader();
                    break;
                default:
                    break; // Should not occur
                }
            }
        }
    }
};

class DotComChar : public Valid{
private:
    int iAddress;
    char cFirstArg[IDENT_LENGTH + 1];
    char cSecondArg[IDENT_LENGTH + 1];
    char cByteArg[BYTE_LENGTH + 1];
public:
    DotComChar (int iAddr, DotCommand dot, char fArg[], char sArg[], char byteArg[]){
        iAddress=iAddr;
        dotcom=dot;
        strncpy (cFirstArg, fArg, IDENT_LENGTH + 1);
        strncpy (cSecondArg, sArg, IDENT_LENGTH + 1);
        strncpy (cByteArg, byteArg, BYTE_LENGTH + 1);
    }
    int iAddressCounter (){ 
        switch (dotcom){
        case eD_EQUATE:
            return 0;
            break;
        case eD_WORD:
            return WORD;
            break;
        case eD_BYTE:
            return BYTE;
            break;
        default:
            break; // Should not occur
        }
        return -1; // Should not occur
    }
    void vBurnAddressChange () { iAddress+=iBurnStart; }
    void vGenerateCode (){
        char cAddr[ADDR_LENGTH + 1];
        if (dotcom!=eD_EQUATE){
            vDecToHexWord (iAddress, cAddr);
            out_file << cAddr << "  ";
        }
        else{
            out_file << "      ";/*6 spaces*/
        }
        if ((iBurnCounter == 0) || (iAddress>=iBurnAddr)){
            vGenerateHexCode(true);
        }
        else{
            vBlankObjCodeColumn();
        }
        vOutputSymbolDecs();
        out_file << "." << cFirstArg;
        vDotCommandBuffer(cFirstArg);
        out_file << "\'" << cSecondArg << "\'";
        vOperandBuffer(cSecondArg, false);
        if ((pComment!=NULL) && (pComment->iLine == iSecPassCodeIndex)){
            if (pComment->bNonemptyLine){
                if (pSymbol == NULL){
                    pComment->cComment[COMMENT_LENGTH_NONEMPTY_NO_SYMBOLS - 1]='\0';
                }
                else{
                    pComment->cComment[COMMENT_LENGTH_NONEMPTY - 1]='\0';
                }
            }
            out_file << ";" << pComment->cComment;
            sCommentNode* p=pComment;
            pComment=pComment->pNext;
            delete p;
        }
    }
    void vGenerateHexCode (bool asemList){
        if (asemList){
            switch (dotcom){
            case eD_EQUATE:
                vBlankObjCodeColumn();
                break;
            case eD_WORD:
                out_file << "00" << cByteArg << "   ";
                break;
            case eD_BYTE:
                out_file << cByteArg << "     ";/*5 spaces*/
                break;
            default:
                break; // Should not occur
            }
        }
        else {
            switch (dotcom){
            case eD_EQUATE:
                break;
            case eD_WORD:
                if ((iBurnCounter == 0) || (iAddress>=iBurnAddr)){
                    out_file << "00";
                    vHexOutputBufferLoader();
                    out_file << cByteArg;
                    vHexOutputBufferLoader();
                }
                break;
            case eD_BYTE:
                if ((iBurnCounter == 0) || (iAddress>=iBurnAddr)){
                    out_file << cByteArg;
                    vHexOutputBufferLoader();
                default:
                    break;
                }
            }
        }
    }
};
class DotComString : public Valid{
private:
    int iAddress;
    int iLength;
    char cFirstArg[IDENT_LENGTH + 1];
    char cSecondArg[IDENT_LENGTH + 1];/*string operand specifier can't be longer than 8 char*/
    char cWordArg[WORD_LENGTH + 1];
public:
    DotComString (int iAddr, DotCommand dot, char fArg[], char sArg[], char wordArg[], int length){
        iAddress=iAddr;
        iLength=length;
        dotcom=dot;
        strncpy (cFirstArg, fArg, IDENT_LENGTH + 1);
        strncpy (cSecondArg, sArg, IDENT_LENGTH + 1);
        strncpy (cWordArg, wordArg, WORD_LENGTH + 1);
    }
    int iAddressCounter (){
        switch (dotcom){
        case eD_EQUATE:
            return 0;
            break;
        case eD_WORD:
            return WORD;
            break;
        case eD_BYTE:
            return BYTE;
            break;
        default:
            break; // Should not occur
        }
        return -1; // Should not occur
    }
    void vBurnAddressChange () { iAddress+=iBurnStart; }
    void vGenerateCode (){
        char cAddr[ADDR_LENGTH + 1];
        if (dotcom!=eD_EQUATE){
            vDecToHexWord (iAddress, cAddr);
            out_file << cAddr << "  ";
        }
        else{
            out_file << "      ";/*6 spaces*/
        }
        if ((iBurnCounter == 0) || (iAddress>=iBurnAddr)){
            vGenerateHexCode(true);
        }
        else{
            vBlankObjCodeColumn();
        }
        vOutputSymbolDecs();
        out_file << "." << cFirstArg;
        vDotCommandBuffer(cFirstArg);
        out_file << "\'" << cSecondArg << "\'";
        vOperandBuffer(cSecondArg, false);
        if ((pComment!=NULL) && (pComment->iLine == iSecPassCodeIndex)){
            if (pComment->bNonemptyLine){
                if (pSymbol == NULL){
                    pComment->cComment[COMMENT_LENGTH_NONEMPTY_NO_SYMBOLS - 1]='\0';
                }
                else{
                    pComment->cComment[COMMENT_LENGTH_NONEMPTY - 1]='\0';
                }
            }
            out_file << ";" << pComment->cComment;
            sCommentNode* p=pComment;
            pComment=pComment->pNext;
            delete p;
        }
    }
    void vGenerateHexCode (bool asemList){
        if (asemList){
            switch (dotcom){
            case eD_EQUATE:
                vBlankObjCodeColumn();
                break;
            case eD_WORD:
                if (iLength==2){
                    out_file << "00" << cWordArg[0] << cWordArg[1] << "   ";
                }
                else{
                    out_file << cWordArg << "   ";
                }
                break;
            case eD_BYTE:
                out_file << cWordArg[0] << cWordArg[1] << "     ";/*5 spaces*/
                break;
            default:
                break; // Should not occur
            }
        }
        else {
            switch (dotcom){
            case eD_EQUATE:
                break;
            case eD_WORD:
                if (iLength == 2){
                    out_file << "00";
                    vHexOutputBufferLoader();
                    out_file << cWordArg;
                    vHexOutputBufferLoader();
                }
                else{
                    out_file << cWordArg[0] << cWordArg [1];
                    vHexOutputBufferLoader();
                    out_file << cWordArg[2] << cWordArg [3];
                    vHexOutputBufferLoader();
                }
                break;
            case eD_BYTE:
                if ((iBurnCounter == 0) || (iAddress>=iBurnAddr)){
                    out_file << cWordArg;
                    vHexOutputBufferLoader();
                default:
                    break; // Should not occur
                }
            }
        }
    }
};
class DotComSym : public Valid{
private:
    int iAddress;
    char cFirstArg[IDENT_LENGTH + 1];
    char cSecondArg[IDENT_LENGTH + 1];
public:
    DotComSym (int iAddr, DotCommand dot, char fArg[], char sArg[]){
        iAddress=iAddr;
        dotcom=dot;
        strncpy (cFirstArg, fArg, IDENT_LENGTH + 1);
        strncpy (cSecondArg, sArg, IDENT_LENGTH + 1);
    }
    int iAddressCounter () { return 2; }
    void vBurnAddressChange () { iAddress+=iBurnStart; }
    void vGenerateCode (){
        char cAddr[ADDR_LENGTH + 1];
        vDecToHexWord (iAddress, cAddr);
        out_file << cAddr << "  ";
        if ((iBurnCounter == 0) || (iAddress>=iBurnAddr)){
            vGenerateHexCode(true);
        }
        else{
            vBlankObjCodeColumn();
        }
        vOutputSymbolDecs();
        out_file << "." << cFirstArg;
        vDotCommandBuffer(cFirstArg);
        out_file << cSecondArg;
        vOperandBuffer(cSecondArg, true);
    }
    void vGenerateHexCode (bool asemList){
        char cVal[ADDR_LENGTH + 1];
        vGetSymbolValue(cSecondArg, cVal);
        if (asemList){
            out_file << cVal << "   ";
        }
        else{
            if ((iBurnCounter == 0) || (iAddress>=iBurnAddr)){
                out_file << cVal[0] << cVal[1];
                vHexOutputBufferLoader();
                out_file << cVal[2] << cVal[3];
                vHexOutputBufferLoader();
            }
        }
    }
};

class DotComAscii : public Valid{
private:
    int iAddress;
    int iLength;
    int iObjLength;
    char cFirstArg[IDENT_LENGTH + 1];
    char cSecondArg[STRING_LENGTH + 1];
    char cByteArg[2 * STRING_LENGTH + 1];
public:
    DotComAscii (int iAddr, DotCommand dot, char fArg[], char sArg[], int length, char byteArg[], int objLength){
        iAddress=iAddr;
        dotcom=dot;
        strncpy (cFirstArg, fArg, IDENT_LENGTH + 1);
        strncpy (cSecondArg, sArg, length + 1);
        strncpy (cByteArg, byteArg, objLength + 1);
        iLength=length;
        iObjLength=objLength;
    }
    int iAddressCounter () {
        return iObjLength/2;/*iObjLength guaranteed even*/
    }
    void vBurnAddressChange () { iAddress += iBurnStart; }
    void vGenerateCode (){
        char cAddr[ADDR_LENGTH + 1];
        vDecToHexWord (iAddress, cAddr);
        out_file << cAddr << "  ";
        if ((iBurnCounter == 0) || (iAddress>=iBurnAddr)){
            vGenerateHexCode(true);
        }
        else{
            vBlankObjCodeColumn();
        }
        vOutputSymbolDecs();
        out_file << "." << cFirstArg;
        vDotCommandBuffer(cFirstArg);
        out_file << "\"" << cSecondArg << "\"";
        if (iLength<OPERAND_SPACES-2){ /*take off 2 for the " "*/
            for (int i=iLength; i<OPERAND_SPACES-2; i++){
                out_file << " ";
            }
        }
        else{
            out_file << " ";
        }
        if ((pComment!=NULL) && (pComment->iLine == iSecPassCodeIndex)){
            if (pComment->bNonemptyLine){
                if (pSymbol == NULL){
                    pComment->cComment[COMMENT_LENGTH_NONEMPTY_NO_SYMBOLS - 1]='\0';
                }
                else{
                    pComment->cComment[COMMENT_LENGTH_NONEMPTY - 1]='\0';
                }
            }
            out_file << ";" << pComment->cComment;
            sCommentNode* p=pComment;
            pComment=pComment->pNext;
            delete p;
        }
        if ((iObjLength>OBJ_CODE_LENGTH) && ((iBurnCounter == 0) || (iAddress>=iBurnAddr))){
            out_file << endl << "      "; /*6 spaces*/
            vDotAsciiOutputContinued(cByteArg, iObjLength);
        }
    }
    void vGenerateHexCode (bool asemList){
        if (asemList){
            if (iObjLength<=OBJ_CODE_LENGTH){
                int i=0;
                int lineCounter=0;
                while (cByteArg[i]!='\0'){
                    out_file << cByteArg[i++];
                    out_file << cByteArg[i++];
                    lineCounter+=2; /*A character in ASCII takes 2 hex digits*/
                }
                for (int j=lineCounter; j<=OBJ_CODE_LENGTH; j++){
                    out_file << " ";
                }
            }
            else{
                for (int j=0; j<OBJ_CODE_LENGTH; ){
                    out_file << cByteArg[j++];
                    out_file << cByteArg[j++];
                }
                out_file << " ";
            }
        }
        else{
            if ((iBurnCounter == 0) || (iAddress>=iBurnAddr)){
                int i=0;
                while (cByteArg[i]!='\0'){
                    out_file << cByteArg[i++];
                    out_file << cByteArg[i++];
                    vHexOutputBufferLoader();
                }
            }
        }
    }
};

class InstructionDec : public Valid{
private:
    int iAddress;
    AMnemon* pAMnemonic;
    char cFirstArg[IDENT_LENGTH + 1];
    char cSecondArg[DEC_LENGTH + 1];
    char cThirdArg[ADDR_MODE_LENGTH + 1];
public:
    InstructionDec (int iAddr, Mnemon mn, AMnemon* pAMnemonTemp, char fArg[], char sArg[], char tArg[]){
        iAddress=iAddr;
        mnemonic=mn;
        pAMnemonic=pAMnemonTemp;
        strncpy (cFirstArg, fArg, IDENT_LENGTH + 1);
        strncpy (cSecondArg, sArg, DEC_LENGTH + 1);
        strncpy (cThirdArg, tArg, ADDR_MODE_LENGTH + 1);
    }
    InstructionDec (int iAddr, Mnemon mn, AMnemon* pAMnemonTemp, char fArg[], char sArg[]){
        iAddress=iAddr;
        mnemonic=mn;
        pAMnemonic=pAMnemonTemp;
        strncpy (cFirstArg, fArg, IDENT_LENGTH + 1);
        strncpy (cSecondArg, sArg, DEC_LENGTH + 1);
        cThirdArg[0]='\0';
    }
    ~InstructionDec () { delete pAMnemonic; }
    int iAddressCounter () { return NONUNARY; }
    void vBurnAddressChange () { iAddress+=iBurnStart; }
    void vGenerateCode (){
        char cAddr[ADDR_LENGTH + 1];
        vDecToHexWord (iAddress, cAddr);
        out_file << cAddr << "  ";
        if ((iBurnCounter == 0) || (iAddress>=iBurnAddr)){
            vGenerateHexCode(true);
        }
        else{
            vBlankObjCodeColumn();
        }
        vOutputSymbolDecs();
        pAMnemonic->vMnemonOutput();
        out_file << cSecondArg;
        if (cThirdArg[0]!='\0')
            out_file << "," << cThirdArg;
        vOperandBuffer(cSecondArg, cThirdArg, true);
    }
    void vGenerateHexCode (bool asemList){
        char cByte[BYTE_LENGTH + 1];
        char cWord[HEX_LENGTH + 1];
        int iDec=iCharToInt(cSecondArg);
        vDecToHexWord(iDec, cWord);
        vDecToHexByte(pAMnemonic->iOpCode() + iAddrModeValue(cThirdArg, pAMnemonic->bNoAddrModeRequired()), cByte);
        if (asemList){
            out_file << cByte << cWord << " ";
        }
        else{
            if ((iBurnCounter == 0) || (iAddress>=iBurnAddr)){
                out_file << cByte;
                vHexOutputBufferLoader();
                out_file << cWord[0] << cWord[1];
                vHexOutputBufferLoader();
                out_file << cWord[2] << cWord[3];
                vHexOutputBufferLoader();
            }
        }
    }
};

class InstructionChar : public Valid{
private:
    int iAddress;
    AMnemon* pAMnemonic;
    char cFirstArg[IDENT_LENGTH + 1];
    char cSecondArg[CHAR_LENGTH + 1];
    char cThirdArg[ADDR_MODE_LENGTH + 1];
    char cByteArg[BYTE_LENGTH + 1];
public:
    InstructionChar (int iAddr, Mnemon mn, AMnemon* pAMnemonTemp, char fArg[], char sArg[], char tArg[], char byteArg[]){
        iAddress=iAddr;
        mnemonic=mn;
        pAMnemonic=pAMnemonTemp;
        strncpy (cFirstArg, fArg, IDENT_LENGTH + 1);
        strncpy (cSecondArg, sArg, CHAR_LENGTH + 1);
        strncpy (cThirdArg, tArg, ADDR_MODE_LENGTH + 1);
        strncpy (cByteArg, byteArg, BYTE_LENGTH + 1);
    }
    ~InstructionChar () { delete pAMnemonic; }
    int iAddressCounter () { return NONUNARY; }
    void vBurnAddressChange () { iAddress+=iBurnStart; }
    void vGenerateCode (){
        char cAddr[ADDR_LENGTH + 1];
        vDecToHexWord (iAddress, cAddr);
        out_file << cAddr << "  ";
        if ((iBurnCounter == 0) || (iAddress>=iBurnAddr)){
            vGenerateHexCode(true);
        }
        else{
            vBlankObjCodeColumn();
        }
        vOutputSymbolDecs();
        pAMnemonic->vMnemonOutput();
        out_file << "\'" << cSecondArg << "\'" << "," << cThirdArg;
        vOperandBuffer(cSecondArg, cThirdArg, false);
    }
    void vGenerateHexCode (bool asemList){
        char cByte[BYTE_LENGTH + 1];
        vDecToHexByte(pAMnemonic->iOpCode() + iAddrModeValue(cThirdArg, pAMnemonic->bNoAddrModeRequired()), cByte);
        if (asemList){
            out_file << cByte << "00" << cByteArg << " ";/*00 right-justifies the char*/
        }
        else{
            if ((iBurnCounter == 0) || (iAddress>=iBurnAddr)){
                out_file << cByte;
                vHexOutputBufferLoader();
                out_file << "00";
                vHexOutputBufferLoader();
                out_file << cByteArg;
                vHexOutputBufferLoader();
            }
        }
    }
};

class InstructionString : public Valid{
private:
    int iAddress;
    int iLength;
    AMnemon* pAMnemonic;
    char cFirstArg[IDENT_LENGTH + 1];
    char cSecondArg[STRING_LENGTH + 1];
    char cThirdArg[ADDR_MODE_LENGTH + 1];
    char cWordArg[WORD_LENGTH + 1];
public:
    InstructionString (int iAddr, Mnemon mn, AMnemon* pAMnemonTemp, char fArg[], char sArg[], char tArg[], char wordArg[], int length){
        iAddress=iAddr;
        iLength=length;
        mnemonic=mn;
        pAMnemonic=pAMnemonTemp;
        strncpy (cFirstArg, fArg, IDENT_LENGTH + 1);
        strncpy (cSecondArg, sArg, STRING_LENGTH + 1);
        strncpy (cThirdArg, tArg, ADDR_MODE_LENGTH + 1);
        strncpy (cWordArg, wordArg, WORD_LENGTH + 1);
    }
    ~InstructionString () { delete pAMnemonic; }
    int iAddressCounter () { return NONUNARY; }
    void vBurnAddressChange () { iAddress+=iBurnStart; }
    void vGenerateCode (){
        char cAddr[ADDR_LENGTH + 1];
        vDecToHexWord (iAddress, cAddr);
        out_file << cAddr << "  ";
        if ((iBurnCounter == 0) || (iAddress>=iBurnAddr)){
            vGenerateHexCode(true);
        }
        else{
            vBlankObjCodeColumn();
        }
        vOutputSymbolDecs();
        pAMnemonic->vMnemonOutput();
        out_file << "\"" << cSecondArg << "\"" << "," << cThirdArg;
        vOperandBuffer(cSecondArg, cThirdArg, false);
    }
    void vGenerateHexCode (bool asemList){
        char cByte[BYTE_LENGTH + 1];
        vDecToHexByte(pAMnemonic->iOpCode() + iAddrModeValue(cThirdArg, pAMnemonic->bNoAddrModeRequired()), cByte);
        if (asemList){
            if (iLength == 2){/*length is guaranteed to be 2 or 4*/
                out_file << cByte << "00" << cWordArg << " ";
            }
            else{
                out_file << cByte << cWordArg << " ";
            }
        }
        else{
            if ((iBurnCounter == 0) || (iAddress>=iBurnAddr)){
                out_file << cByte;
                vHexOutputBufferLoader();
                if (iLength == 2){
                    out_file << "00";
                    vHexOutputBufferLoader();
                    out_file << cWordArg;
                    vHexOutputBufferLoader();
                }
                else{
                    out_file << cWordArg[0] << cWordArg [1];
                    vHexOutputBufferLoader();
                    out_file << cWordArg[2] << cWordArg [3];
                    vHexOutputBufferLoader();
                }
            }
        }
    }
};

class InstructionHex : public Valid{
private:
    int iAddress;
    AMnemon* pAMnemonic;
    char cFirstArg[IDENT_LENGTH + 1];
    char cSecondArg[HEX_LENGTH + 1];
    char cThirdArg[ADDR_MODE_LENGTH + 1];
public:
    InstructionHex (int iAddr, Mnemon mn, AMnemon* pAMnemonTemp, char fArg[], char sArg[], char tArg[]){
        iAddress=iAddr;
        mnemonic=mn;
        pAMnemonic=pAMnemonTemp;
        strncpy (cFirstArg, fArg, IDENT_LENGTH + 1);
        strncpy (cSecondArg, sArg, HEX_LENGTH + 1);
        strncpy (cThirdArg, tArg, ADDR_MODE_LENGTH + 1);
    }
    InstructionHex (int iAddr, Mnemon mn, AMnemon* pAMnemonTemp, char fArg[], char sArg[]){
        iAddress=iAddr;
        mnemonic=mn;
        pAMnemonic=pAMnemonTemp;
        strncpy (cFirstArg, fArg, IDENT_LENGTH + 1);
        strncpy (cSecondArg, sArg, HEX_LENGTH + 1);
        cThirdArg[0]='\0';
    }

    ~InstructionHex () { delete pAMnemonic; }
    int iAddressCounter () { return NONUNARY; }
    void vBurnAddressChange () { iAddress+=iBurnStart; }
    void vGenerateCode (){
        char cAddr[ADDR_LENGTH + 1];
        vDecToHexWord (iAddress, cAddr);
        out_file << cAddr << "  ";
        if ((iBurnCounter == 0) || (iAddress>=iBurnAddr)){
            vGenerateHexCode(true);
        }
        else{
            vBlankObjCodeColumn();
        }
        vOutputSymbolDecs();
        pAMnemonic->vMnemonOutput();
        out_file << "0x" << cSecondArg;
        if (cThirdArg[0]!='\0')
            out_file << "," << cThirdArg;
        vOperandBuffer(cSecondArg, cThirdArg, false);
    }
    void vGenerateHexCode (bool asemList){
        char cByte[BYTE_LENGTH + 1];
        vDecToHexByte(pAMnemonic->iOpCode() + iAddrModeValue(cThirdArg, pAMnemonic->bNoAddrModeRequired()), cByte);
        if (asemList){
            out_file << cByte << cSecondArg << " ";
        }
        else{
            if ((iBurnCounter == 0) || (iAddress>=iBurnAddr)){
                out_file << cByte;
                vHexOutputBufferLoader();
                out_file << cSecondArg[0] << cSecondArg[1];
                vHexOutputBufferLoader();
                out_file << cSecondArg[2] << cSecondArg[3];
                vHexOutputBufferLoader();
            }
        }
    }
};

class InstructionSym : public Valid{
private:
    int iAddress;
    AMnemon* pAMnemonic;
    char cFirstArg[IDENT_LENGTH + 1];
    char cSecondArg[IDENT_LENGTH + 1];
    char cThirdArg[ADDR_MODE_LENGTH +1];
public:
    InstructionSym (int iAddr, Mnemon mn, AMnemon* pAMnemonTemp, char fArg[], char sArg[], char tArg[]){
        iAddress=iAddr;
        mnemonic=mn;
        pAMnemonic=pAMnemonTemp;
        strncpy (cFirstArg, fArg, IDENT_LENGTH + 1);
        strncpy (cSecondArg, sArg, IDENT_LENGTH + 1);
        strncpy (cThirdArg, tArg, ADDR_MODE_LENGTH + 1);
    }
    InstructionSym (int iAddr, Mnemon mn, AMnemon* pAMnemonTemp, char fArg[], char sArg[]){
        iAddress=iAddr;
        mnemonic=mn;
        pAMnemonic=pAMnemonTemp;
        strncpy (cFirstArg, fArg, IDENT_LENGTH + 1);
        strncpy (cSecondArg, sArg, IDENT_LENGTH + 1);
        cThirdArg[0]='\0';
    }
    ~InstructionSym () { delete pAMnemonic; }
    int iAddressCounter () { return NONUNARY; }
    void vBurnAddressChange () { iAddress+=iBurnStart; }
    void vGenerateCode (){
        char cAddr[ADDR_LENGTH + 1];
        vDecToHexWord (iAddress, cAddr);
        out_file << cAddr << "  ";
        if ((iBurnCounter == 0) || (iAddress>=iBurnAddr)){
            vGenerateHexCode(true);
        }
        else{
            vBlankObjCodeColumn();
        }
        vOutputSymbolDecs();
        pAMnemonic->vMnemonOutput();
        out_file << cSecondArg;
        if (cThirdArg[0]!='\0')
            out_file << "," << cThirdArg;
        vOperandBuffer(cSecondArg, cThirdArg, true);
    }
    void vGenerateHexCode (bool asemList){
        char cTemp[HEX_LENGTH + 1];
        char cByte[BYTE_LENGTH + 1];
        vDecToHexByte(pAMnemonic->iOpCode() + iAddrModeValue(cThirdArg, pAMnemonic->bNoAddrModeRequired()), cByte);
        vGetSymbolValue(cSecondArg, cTemp);
        if (asemList){
            out_file << cByte << cTemp << " ";
        }
        else{
            if ((iBurnCounter == 0) || (iAddress>=iBurnAddr)){
                out_file << cByte;
                vHexOutputBufferLoader();
                out_file << cTemp[0] << cTemp[1];
                vHexOutputBufferLoader();
                out_file << cTemp[2] << cTemp[3];
                vHexOutputBufferLoader();
            }
        }
    }
};

/*Table and object initializations*/
/*Initializes all global tables with their values*/
void vInitGlobalTables (){
    strncpy (cDotTable[eD_ADDRSS], "ADDRSS", IDENT_LENGTH + 1);
    strncpy (cDotTable[eD_ASCII], "ASCII", IDENT_LENGTH + 1);
    strncpy (cDotTable[eD_BLOCK], "BLOCK", IDENT_LENGTH + 1);
    strncpy (cDotTable[eD_BURN], "BURN", IDENT_LENGTH + 1);
    strncpy (cDotTable[eD_BYTE], "BYTE", IDENT_LENGTH + 1);
    strncpy (cDotTable[eD_END], "END", IDENT_LENGTH + 1);
    strncpy (cDotTable[eD_EQUATE], "EQUATE", IDENT_LENGTH + 1);
    strncpy (cDotTable[eD_WORD], "WORD", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_STOP], "STOP", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_RETTR], "RETTR", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_MOVSPA], "MOVSPA", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_MOVFLGA], "MOVFLGA", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_BR], "BR", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_BRLE], "BRLE", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_BRLT], "BRLT", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_BREQ], "BREQ", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_BRNE], "BRNE", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_BRGE], "BRGE", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_BRGT], "BRGT", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_BRV], "BRV", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_BRC], "BRC", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_CALL], "CALL", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_NOTA], "NOTA", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_NOTX], "NOTX", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_NEGA], "NEGA", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_NEGX], "NEGX", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_ASLA], "ASLA", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_ASLX], "ASLX", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_ASRA], "ASRA", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_ASRX], "ASRX", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_ROLA], "ROLA", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_ROLX], "ROLX", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_RORA], "RORA", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_RORX], "RORX", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_CHARI], "CHARI", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_CHARO], "CHARO", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_RET0], "RET0", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_RET1], "RET1", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_RET2], "RET2", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_RET3], "RET3", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_RET4], "RET4", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_RET5], "RET5", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_RET6], "RET6", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_RET7], "RET7", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_ADDSP], "ADDSP", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_SUBSP], "SUBSP", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_ADDA], "ADDA", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_ADDX], "ADDX", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_SUBA], "SUBA", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_SUBX], "SUBX", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_ANDA], "ANDA", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_ANDX], "ANDX", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_ORA], "ORA", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_ORX], "ORX", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_CPA], "CPA", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_CPX], "CPX", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_LDA], "LDA", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_LDX], "LDX", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_LDBYTEA], "LDBYTEA", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_LDBYTEX], "LDBYTEX", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_STA], "STA", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_STX], "STX", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_STBYTEA], "STBYTEA", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_STBYTEX], "STBYTEX", IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_UNIMP0], sUnimpMnemon[0].cID, IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_UNIMP1], sUnimpMnemon[1].cID, IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_UNIMP2], sUnimpMnemon[2].cID, IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_UNIMP3], sUnimpMnemon[3].cID, IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_UNIMP4], sUnimpMnemon[4].cID, IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_UNIMP5], sUnimpMnemon[5].cID, IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_UNIMP6], sUnimpMnemon[6].cID, IDENT_LENGTH + 1);
    strncpy (cMnemonTable[eM_UNIMP7], sUnimpMnemon[7].cID, IDENT_LENGTH + 1);
}

/*Initializes the object of mnemon found in vLookUpMnemon() */
void vInitMnemonObjects  (Mnemon mnemon, AMnemon*& pAMnemonTemp){
    switch (mnemon){
    case eM_STOP: pAMnemonTemp=new Stop (); break;
    case eM_RETTR: pAMnemonTemp=new Rettr (); break;
    case eM_MOVSPA: pAMnemonTemp=new Movspa (); break;
    case eM_MOVFLGA: pAMnemonTemp=new Movflga (); break;
    case eM_BR: pAMnemonTemp=new Br (); break;
    case eM_BRLE: pAMnemonTemp=new Brle (); break;
    case eM_BRLT: pAMnemonTemp=new Brlt (); break;
    case eM_BREQ: pAMnemonTemp=new Breq (); break;
    case eM_BRNE: pAMnemonTemp=new Brne (); break;
    case eM_BRGE: pAMnemonTemp=new Brge (); break;
    case eM_BRGT: pAMnemonTemp=new Brgt (); break;
    case eM_BRV: pAMnemonTemp=new Brv (); break;
    case eM_BRC: pAMnemonTemp=new Brc (); break;
    case eM_CALL: pAMnemonTemp=new Call (); break;
    case eM_NOTA: pAMnemonTemp=new Nota (); break;
    case eM_NOTX: pAMnemonTemp=new Notx (); break;
    case eM_NEGA: pAMnemonTemp=new Nega (); break;
    case eM_NEGX: pAMnemonTemp=new Negx (); break;
    case eM_ASLA: pAMnemonTemp=new Asla (); break;
    case eM_ASLX: pAMnemonTemp=new Aslx (); break;
    case eM_ASRA: pAMnemonTemp=new Asra (); break;
    case eM_ASRX: pAMnemonTemp=new Asrx (); break;
    case eM_ROLA: pAMnemonTemp=new Rola (); break;
    case eM_ROLX: pAMnemonTemp=new Rolx (); break;
    case eM_RORA: pAMnemonTemp=new Rora (); break;
    case eM_RORX: pAMnemonTemp=new Rorx (); break;
    case eM_CHARI: pAMnemonTemp=new Chari (); break;
    case eM_CHARO: pAMnemonTemp=new Charo (); break;
    case eM_RET0: pAMnemonTemp=new Ret0 (); break;
    case eM_RET1: pAMnemonTemp=new Ret1 (); break;
    case eM_RET2: pAMnemonTemp=new Ret2 (); break;
    case eM_RET3: pAMnemonTemp=new Ret3 (); break;
    case eM_RET4: pAMnemonTemp=new Ret4 (); break;
    case eM_RET5: pAMnemonTemp=new Ret5 (); break;
    case eM_RET6: pAMnemonTemp=new Ret6 (); break;
    case eM_RET7: pAMnemonTemp=new Ret7 (); break;
    case eM_ADDSP: pAMnemonTemp=new Addsp (); break;
    case eM_SUBSP: pAMnemonTemp=new Subsp (); break;
    case eM_ADDA: pAMnemonTemp=new Adda (); break;
    case eM_ADDX: pAMnemonTemp=new Addx (); break;
    case eM_SUBA: pAMnemonTemp=new Suba (); break;
    case eM_SUBX: pAMnemonTemp=new Subx (); break;
    case eM_ANDA: pAMnemonTemp=new Anda (); break;
    case eM_ANDX: pAMnemonTemp=new Andx (); break;
    case eM_ORA: pAMnemonTemp=new Ora (); break;
    case eM_ORX: pAMnemonTemp=new Orx (); break;
    case eM_CPA: pAMnemonTemp=new Cpa (); break;
    case eM_CPX: pAMnemonTemp=new Cpx (); break;
    case eM_LDA: pAMnemonTemp=new Lda (); break;
    case eM_LDX: pAMnemonTemp=new Ldx (); break;
    case eM_LDBYTEA: pAMnemonTemp=new Ldbytea (); break;
    case eM_LDBYTEX: pAMnemonTemp=new Ldbytex (); break;
    case eM_STA: pAMnemonTemp=new Sta (); break;
    case eM_STX: pAMnemonTemp=new Stx (); break;
    case eM_STBYTEA: pAMnemonTemp=new Stbytea (); break;
    case eM_STBYTEX: pAMnemonTemp=new Stbytex (); break;
    case eM_UNIMP0: pAMnemonTemp=new Unimp0 (); break;
    case eM_UNIMP1: pAMnemonTemp=new Unimp1 (); break;
    case eM_UNIMP2: pAMnemonTemp=new Unimp2 (); break;
    case eM_UNIMP3: pAMnemonTemp=new Unimp3 (); break;
    case eM_UNIMP4: pAMnemonTemp=new Unimp4 (); break;
    case eM_UNIMP5: pAMnemonTemp=new Unimp5 (); break;
    case eM_UNIMP6: pAMnemonTemp=new Unimp6 (); break;
    case eM_UNIMP7: pAMnemonTemp=new Unimp7 (); break;
    default: break; // Should not occur
    }
}

/*Table Search functions*/
/*Looks up to see if mn is a valid mnemonic*/
void vLookUpMnemon (char cID[], Mnemon& mn, AMnemon*& pAMnemonTemp, bool& bFnd){
    for (int i=0; i<=IDENT_LENGTH; i++){
        cID[i]=toupper (cID[i]);
    }
    strncpy (cMnemonTable[eM_EMPTY], cID, IDENT_LENGTH + 1);
    mn=eM_STOP;
    while (strcmp (cMnemonTable[mn], cID)!=0){
        mn=Mnemon (mn + 1);
    }
    bFnd=(mn!=eM_EMPTY);
    if (bFnd){
        vInitMnemonObjects(mn, pAMnemonTemp);
    }
}

/*Looks up to see if dot is a valid dot command*/
void vLookUpDot (char cID[], DotCommand& dot, bool& bFnd){
    for (int i=0; i<=IDENT_LENGTH; i++){
        cID[i]=toupper (cID[i]);
    }
    strncpy (cDotTable[eD_EMPTY], cID, IDENT_LENGTH + 1);
    dot=eD_BLOCK;
    while (strcmp (cDotTable[dot], cID)!=0){
        dot=DotCommand (dot + 1);
    }
    bFnd=(dot!=eD_EMPTY);
    if (dot == eD_ASCII){
        bIsAscii=true;
    }
}

/*Symbol and Comment functions*/
/*Searches to see if cID[] has been declared*/
bool bLookUpSymbol (char cID[]){
    sSymbolNode* p=pSymbol;
    while ((p!=NULL) && (strcmp (cID, p->cSymID)>0)){
        p=p->pNext;
    }
    if (p!=NULL){
        return (strcmp (cID, p->cSymID) == 0);
    }
    else{
        return false;
    }
}

/*Installs a symbol declaration in a linked list of symbols with their values*/
void vInstallSymbol (char cID[]){
    sSymbolNode *p, *q;
    sSymbolNode* pTemp=new sSymbolNode;
    char addrHex[ADDR_LENGTH + 1];
    strncpy (pTemp->cSymID, cID, IDENT_LENGTH + 1);
    vDecToHexWord (iCurrentAddress, addrHex);
    strncpy (pTemp->cSymValue, addrHex, ADDR_LENGTH + 1);
    pTemp->iLine=iCodeIndex;
    q=NULL;
    p=pSymbol;
    while ((p!=NULL) && (strcmp (cID, p->cSymID)>0)){
        q=p; /* q follows p.*/
        p=p->pNext;
    }
    if ((p!=NULL) && (strcmp (cID, p->cSymID) == 0)){
        delete pACode[iCodeIndex];
        pACode[iCodeIndex]=new eSymPrevDef;
        delete pTemp;
        return;
    }
    pTemp->pNext=p;
    if ((q!=NULL)){
        q->pNext=pTemp;
    }
    else{
        pSymbol=pTemp;
    }
}

/*Installs a symbol output declaration in a linked list of symbols with their lines*/
void vInstallSymbolOutput (char cID[]){
    sSymbolOutputNode *p, *q;
    sSymbolOutputNode* pTemp=new sSymbolOutputNode;
    strncpy (pTemp->cSymID, cID, IDENT_LENGTH + 1);
    pTemp->iLine=iCodeIndex;
    q=NULL;
    p=pSymbolOutput;
    while (p!=NULL){
        q=p;  /*q follows p.*/
        p=p->pNext;
    }
    pTemp->pNext=p;
    if (q!=NULL){
        q->pNext=pTemp;
    }
    else{
        pSymbolOutput=pTemp;
    }
}

/*Changes cSymValue[] to cVal[] of the symbol named cID[] to account for .EQUATE*/
void vChangeSymValEquate (char cID[], char cVal[])
{
    sSymbolNode* p=pSymbol;
    while (p!=NULL){
        if (strcmp (cID, p->cSymID)!=0){
            p=p->pNext;
        }
        else{
            strncpy (p->cSymValue, cVal, ADDR_LENGTH + 1);
            return;
        }
    }
}

/*Installs cVal[] and cID into the pEquate linked list*/
void vInstallEquateNode (char cID[], char cVal[]){
    sEquateNode* p=new sEquateNode;
    strncpy (p->cSymValue, cVal, ADDR_LENGTH + 1);
    strncpy (p->cSymID, cID, IDENT_LENGTH + 1);
    p->pNext=pEquate;
    pEquate=p;
}

/*Changes the value of every symbol to account for .BURN*/
void vChangeSymValBurn (int iBurnStartAddress){
    sSymbolNode* p=pSymbol;
    char cVal[ADDR_LENGTH + 1];
    while (p!=NULL){
        vDecToHexWord(iHexWordToDecInt (p->cSymValue) + iBurnStartAddress, cVal);
        strncpy (p->cSymValue, cVal, ADDR_LENGTH + 1);
        p=p->pNext;
    }
}

/*Installs an undeclared symbol in a linked list of undeclared symbols with their values*/
void vInstallUndeclaredSymbol (char cID[]){
    sUndeclaredsSymbolNode *p, *q;
    sUndeclaredsSymbolNode* pTemp=new sUndeclaredsSymbolNode;
    strncpy (pTemp->cSymID, cID, IDENT_LENGTH + 1);
    pTemp->iLine=iCodeIndex;
    pTemp->pNext=NULL;
    q=NULL;
    p=pUndeclaredSym;
    while (p!=NULL){
        q=p; /*q follows p.*/
        p=p->pNext;
    }
    pTemp->pNext=p;
    if ((q!=NULL)){
        q->pNext=pTemp;
    }
    else{
        pUndeclaredSym=pTemp;
    }
}

/*Installs a comment in a linked list of comments with their lines and values*/
void vInstallComment (char cID[], bool bNonempty){
    sCommentNode *p, *q;
    sCommentNode* pTemp=new sCommentNode;
    strncpy (pTemp->cComment, cID, COMMENT_LENGTH + 1);
    pTemp->bNonemptyLine=bNonempty;
    pTemp->iLine=iCodeIndex;
    pTemp->pNext=NULL;
    q=NULL;
    p=pComment;
    while (p!=NULL){
        q=p; /*q follows p.*/
        p=p->pNext;
    }
    pTemp->pNext=p;
    if (q!=NULL){
        q->pNext=pTemp;
    }
    else{
        pComment=pTemp;
    }
}

/*Lexical Analyzer (finds tokens in the language)*/
void vGetToken (AToken*& pAT){
    char cNextChar;
    int i;
    char cLocalIdentValue[IDENT_LENGTH + 1];
    char cLocalCommentValue[COMMENT_LENGTH + 1];
    char cLocalHexValue[HEX_LENGTH + 1];
    char cLocalDecValue[DEC_LENGTH + 1];
    char cLocalCharValue[CHAR_LENGTH + 1];
    char cLocalStringValue[STRING_LENGTH + 1];
    char cAddr[ADDR_MODE_LENGTH + 1];
    State state=eS_START;
    pAT=new TEmpty;
    do{
        vAdvanceInput (cNextChar);
        switch (state){
        case eS_START:
            i=0;
            if (cNextChar == ',')
                state=eS_ADDR;
            else if (cNextChar == '\'')
                state=eS_CHAR1;
            else if (cNextChar == ';')
                state=eS_COMMENT;
            else if (cNextChar == '.')
                state=eS_DOT1;
            else if (cNextChar == '\n')
                state=eS_STOP;
            else if (cNextChar == '\"')
                state=eS_STRING;
            else if (isalpha (cNextChar) || cNextChar == '_'){
                cLocalIdentValue[i++]=cNextChar;
                state=eS_IDENT;
            }
            else if ((cNextChar == '+') || (cNextChar == '-')){
                if (cNextChar == '-')
                    cLocalDecValue[i++]=cNextChar;
                state=eS_SIGN;
            }
            else if (cNextChar == '0'){
                vAdvanceInput (cNextChar);
                if ((cNextChar == 'x') || (cNextChar == 'X'))
                    state=eS_HEX1;
                else{
                    cLocalDecValue[i++]='0';
                    vBackUpInput ();
                    state=eS_DEC;
                }
            }
            else if (isdigit (cNextChar)){
                cLocalDecValue[i++]=cNextChar;
                state=eS_DEC;
            }
            else if ((cNextChar!=' ') && (cNextChar!='\t')){
                delete pAT;
                pAT=new TInvalid;
                state=eS_STOP;
            }
            break;
        case eS_ADDR:
            cNextChar=tolower(cNextChar);
            if ((cNextChar=='i') || (cNextChar=='d') || (cNextChar=='n') || (cNextChar=='x')){
                cAddr[i++]=cNextChar;
                cAddr[i]='\0';
                delete pAT;
                pAT=new TAddress (cAddr);
                state=eS_STOP;
            }
            else if (cNextChar=='s'){
                cAddr[i++]=cNextChar;
                state=eS_ADDRs;
            }
            else if ((cNextChar!=' ') && (cNextChar!='\t')){
                vBackUpInput ();
                delete pAT;
                pAT=new TInvalidAddr;
                state=eS_STOP;
            }
            break;
        case eS_ADDRs:
            cNextChar=tolower(cNextChar);
            if (cNextChar=='f'){
                cAddr[i++]=cNextChar;
                cAddr[i]='\0';
                delete pAT;
                pAT=new TAddress (cAddr);
                state=eS_STOP;
            }
            else if (cNextChar=='x'){
                cAddr[i++]=cNextChar;
                state=eS_ADDRsx;
            }
            else{
                cAddr[i]='\0';
                vBackUpInput();
                delete pAT;
                pAT=new TAddress (cAddr);
                state=eS_STOP;
            }
            break;
        case eS_ADDRsx:
            cNextChar=tolower(cNextChar);
            if (cNextChar=='f'){
                cAddr[i++]=cNextChar;
                cAddr[i]='\0';
                delete pAT;
                pAT=new TAddress (cAddr);
                state=eS_STOP;
            }
            else{
                cAddr[i]='\0';
                vBackUpInput();
                delete pAT;
                pAT=new TAddress (cAddr);
                state=eS_STOP;
            }
            break;
        case eS_CHAR1:
            if (cNextChar == '\\'){
                cLocalCharValue[i++]=cNextChar;
                state=eS_CHARBASH;
            }
            else if (cNextChar!='\''){
                cLocalCharValue[i++]=cNextChar;
                state=eS_CHAR2;
            }
            else{
                vBackUpInput ();
                delete pAT;
                pAT=new TInvalidChar;
                state=eS_STOP;
            }
            break;
        case eS_CHAR2:
            if (cNextChar == '\''){
                cLocalCharValue[i]='\0';
                delete pAT;
                pAT=new TCharConstant (cLocalCharValue);
                state=eS_STOP;
            }
            else{
                vBackUpInput ();
                delete pAT;
                pAT=new TInvalidChar;
                state=eS_STOP;
            }
            break;
        case eS_CHARBASH:
            if ((cNextChar == 'x') || (cNextChar == 'X')){
                cLocalCharValue[i++]='x';
                state=eS_CHARBYTE;
            }
            else if ((cNextChar == '\\') || (cNextChar == '\"') || (cNextChar == '\'') || (cNextChar == 'b') || (cNextChar == 'f') || (cNextChar == 'n') || (cNextChar == 'r') || (cNextChar == 't') || (cNextChar == 'v')){
                cLocalCharValue[i++]=cNextChar;
                state=eS_CHAR2;
            }
            else{
                vBackUpInput ();
                delete pAT;
                pAT=new TInvalidChar;
                state=eS_STOP;
            }
            break;
        case eS_CHARBYTE:
            if (bIsHex(cNextChar)){
                cLocalCharValue[i++]=cNextChar;
                vAdvanceInput (cNextChar);
                if (bIsHex(cNextChar)){
                    cLocalCharValue[i++]=cNextChar;
                    state=eS_CHAR2;
                }
                else{
                    vBackUpInput ();
                    delete pAT;
                    pAT=new TInvalidChar;
                    state=eS_STOP;
                }
            }
            else{
                vBackUpInput ();
                delete pAT;
                pAT=new TInvalidChar;
                state=eS_STOP;
            }
            break;
        case eS_COMMENT:
            if (cNextChar == '\n'){
                cLocalCommentValue[i]='\0';
                vBackUpInput ();
                delete pAT;
                pAT=new TComment (cLocalCommentValue);
                state=eS_STOP;
            }
            else if (i<COMMENT_LENGTH)
                cLocalCommentValue[i++]=cNextChar;
//          else{
//             vBackUpInput ();
//             delete pAT;
//             pAT=new TInvalidComment;
//             state=eS_STOP;
//          }
            break;
        case eS_DEC:
            if (isdigit (cNextChar) && (i<DEC_LENGTH))
                cLocalDecValue[i++]=cNextChar;
            else{
                cLocalDecValue[i]='\0';
                vBackUpInput ();
                delete pAT;
                pAT=new TDecConstant (cLocalDecValue);
                state=eS_STOP;
            }
            break;
        case eS_DOT1:
            if (isalpha (cNextChar)){
                cLocalIdentValue[i++]=cNextChar;
                state=eS_DOT2;
            }
            else{
                vBackUpInput ();
                delete pAT;
                pAT=new TInvalidDotCommand;
                state=eS_STOP;
            }
            break;
        case eS_DOT2:
            if ((i<IDENT_LENGTH) && (isalnum (cNextChar)))
                cLocalIdentValue[i++]=cNextChar;
            else{
                cLocalIdentValue[i]='\0';
                vBackUpInput ();
                delete pAT;
                pAT=new TDotCommand (cLocalIdentValue);
                state=eS_STOP;
            }
            break;
        case eS_HEX1:
            if (bIsHex(cNextChar)){
                state=eS_HEX2;
                for (int j=0; j<HEX_LENGTH-1;)
                    cLocalHexValue[j++]='0';
                i++;
                cLocalHexValue[HEX_LENGTH-1]=cNextChar;
                cLocalHexValue[HEX_LENGTH]='\0';
            }
            else{
                vBackUpInput ();
                delete pAT;
                pAT=new TInvalidHex;
                state=eS_STOP;
            }
            break;
        case eS_HEX2:
            if ((bIsHex(cNextChar)) && (i<HEX_LENGTH)){
                for (int j=0; j<HEX_LENGTH-1; j++)
                    cLocalHexValue[j]=cLocalHexValue[j+1];
                cLocalHexValue[HEX_LENGTH-1]=cNextChar;
                i++;
            }
            else{
                vBackUpInput ();
                delete pAT;
                pAT=new THexConstant (cLocalHexValue);
                state=eS_STOP;
            }
            break;
        case eS_IDENT:
            if ((isalnum (cNextChar) || cNextChar == '_') && (i<IDENT_LENGTH))
                cLocalIdentValue[i++]=cNextChar;
            else if (cNextChar == ':'){
                cLocalIdentValue[i]='\0';
                delete pAT;
                pAT=new TSymbol (cLocalIdentValue);
                state=eS_STOP;
            }
            else{
                cLocalIdentValue[i]='\0';
                vBackUpInput ();
                delete pAT;
                pAT=new TIdentifier (cLocalIdentValue);
                state=eS_STOP;
            }
            break;
        case eS_SIGN:
            if (cNextChar=='0'){
                i=1;
                cLocalDecValue[0]='0';
                cLocalDecValue[1]='\0';
                delete pAT;
                pAT=new TDecConstant (cLocalDecValue);
                state=eS_STOP;
            }
            else if (isdigit(cNextChar)){
                cLocalDecValue[i++]=cNextChar;
                state=eS_DEC;
            }
            else{
                vBackUpInput ();
                delete pAT;
                pAT=new TInvalidDec;
                state=eS_STOP;
            }
            break;
        case eS_STRING:
            if (cNextChar == '\\'){
                cLocalStringValue[i++]=cNextChar;
                state=eS_STRINGBASH;
            }
            else if ((cNextChar!='\"') && (cNextChar!='\n') && (i<STRING_LENGTH))
                cLocalStringValue[i++]=cNextChar;
            else if ((cNextChar == '\"') && (i>0)){
                cLocalStringValue[i]='\0';
                delete pAT;
                pAT=new TString(cLocalStringValue);
                state=eS_STOP;
            }
            else{
                vBackUpInput ();
                delete pAT;
                pAT=new TInvalidString;
                state=eS_STOP;
            }
            break;
        case eS_STRINGBASH:
            if ((cNextChar == 'x') || (cNextChar == 'X')){
                cLocalStringValue[i++]='x';
                state=eS_STRINGBYTE;
            }
            else if ((cNextChar == '\\') || (cNextChar == '\"') || (cNextChar == '\'') || (cNextChar == 'b') || (cNextChar == 'f') || (cNextChar == 'n') || (cNextChar == 'r') || (cNextChar == 't') || (cNextChar == 'v')){
                cLocalStringValue[i++]=cNextChar;
                state=eS_STRING;
            }
            else{
                vBackUpInput ();
                delete pAT;
                pAT=new TInvalidString;
                state=eS_STOP;
            }
            break;
        case eS_STRINGBYTE:
            if (bIsHex(cNextChar)){
                cLocalStringValue[i++]=cNextChar;
                vAdvanceInput (cNextChar);
                if (bIsHex(cNextChar)){
                    cLocalStringValue[i++]=cNextChar;
                    state=eS_STRING;
                }
                else{
                    vBackUpInput ();
                    delete pAT;
                    pAT=new TInvalidString;
                    state=eS_STOP;
                }
            }
            else{
                vBackUpInput ();
                delete pAT;
                pAT=new TInvalidString;
                state=eS_STOP;
            }
            break;
        default:
            break; // Should not occur
        }
    }
    while ((state!=eS_STOP) && (pAT->kTokenType ()!=eT_INVALID));
    pPrevAT=pAT;
}

/*Parser*/
/*Determines whether a string of tokens is a valid line of assembly language*/
/*code using a FSM implementation.*/


void vProcessSourceLine (bool& term){
    char cLocalStringVal[STRING_LENGTH + 1];
    char cLocalStringObjVal[STRING_OPRND_LENGTH + 1];
    char cLocalAsciiObjVal[2 * STRING_LENGTH + 1];/*each char is 2 hex digits*/
    char cLocalSymVal[IDENT_LENGTH + 1];
    char cLocalIdentVal[IDENT_LENGTH + 1];
    char cLocalSecondIdentVal[IDENT_LENGTH + 1];
    char cLocalHexVal[HEX_LENGTH + 1];
    char cLocalDecVal[DEC_LENGTH + 1];
    char cLocalCharVal[CHAR_LENGTH + 1];
    char cLocalCharByteVal[BYTE_LENGTH + 1];
    char cLocalAddrModeVal[ADDR_MODE_LENGTH + 1];
    char cLocalCommentVal[COMMENT_LENGTH + 1];
    bool bSymDeclared=false;
    TIdentifier* pTIdent=NULL;
    TDotCommand* pTDot=NULL;
    TAddress* pTAddress=NULL;
    THexConstant* pTHex=NULL;
    TDecConstant* pTDec=NULL;
    TCharConstant* pTChar=NULL;
    TSymbol* pTSym=NULL;
    TString* pTString=NULL;
    TComment* pTComm=NULL;
    AToken* pAToken=NULL;
    Valid* pValid=NULL;
    AMnemon* pAMnemonTemp=NULL;
    Mnemon mnemon;
    DotCommand dotcom;
    int iTemp;
    int iObjLength;
    int iStrLength;
    bool bFound;
    pACode[iCodeIndex]=new ZeroArg (eD_EMPTY);
    ParseState psState=ePS_START;
    do{
        vGetToken (pAToken);
        switch (psState){
        case ePS_START:
            if (pAToken->kTokenType () == eT_IDENTIFIER){
                pTIdent=static_cast <TIdentifier*> (pAToken);
                pTIdent->vGetValue (cLocalIdentVal);
                vLookUpMnemon (cLocalIdentVal, mnemon, pAMnemonTemp, bFound);
                if (bFound){
                    if (pAMnemonTemp->bIsUnary()){
                        delete pACode[iCodeIndex];
                        pACode[iCodeIndex]=new UnaryInstruction (iCurrentAddress, mnemon, pAMnemonTemp, cLocalIdentVal);
                        pValid=static_cast <Valid*> (pACode[iCodeIndex]);
                        iCurrentAddress+=pValid->iAddressCounter();
                        psState=ePS_CLOSE;
                    }
                    else{
                        psState=ePS_INSTRUCTION;
                    }
                }
                else{
                    delete pACode[iCodeIndex];
                    pACode[iCodeIndex]=new eInvMnemon;
                }
            }
            else if (pAToken->kTokenType () == eT_DOTCOMMAND){
                pTDot=static_cast <TDotCommand*> (pAToken);
                pTDot->vGetValue (cLocalIdentVal);
                vLookUpDot (cLocalIdentVal, dotcom, bFound);
                if (bFound){
                    if (dotcom == eD_END){
                        delete pACode[iCodeIndex];
                        pACode[iCodeIndex]=new DotEnd (iCurrentAddress, eD_END, cLocalIdentVal);
                        pValid=static_cast <Valid*> (pACode[iCodeIndex]);
                        iCurrentAddress+=pValid->iAddressCounter();
                        term=true;
                        psState=ePS_CLOSE;
                    }
                    else if (dotcom == eD_ASCII){
                        psState=ePS_ASCII;
                    }
                    else{
                        psState=ePS_DOTCOMMAND;
                    }
                }
                else{
                    delete pACode[iCodeIndex];
                    pACode[iCodeIndex]=new eNoDotCom;
                }
            }
            else if (pAToken->kTokenType () == eT_SYMBOL){
                pTSym=static_cast <TSymbol*> (pAToken);
                pTSym->vGetValue (cLocalSymVal);
                vInstallSymbol (cLocalSymVal);
                vInstallSymbolOutput (cLocalSymVal);
                psState=ePS_SYMBOLDEC;
            }
            else if (pAToken->kTokenType () == eT_EMPTY){
                delete pACode[iCodeIndex];
                pACode[iCodeIndex]=new ZeroArg (eD_EMPTY);
                psState=ePS_FINISH;
            }
            else if (pAToken->kTokenType () == eT_COMMENT){
                pTComm=static_cast <TComment*> (pAToken);
                pTComm->vGetValue (cLocalCommentVal);
                vInstallComment (cLocalCommentVal, false);
                psState=ePS_COMMENT;
            }
            else if (pAToken->kTokenType () == eT_INVALIDCOMMENT){
                delete pACode[iCodeIndex];
                pACode[iCodeIndex]=new eCommentTooLong;
            }
            else if (pAToken->kTokenType () == eT_INVALID){
                delete pACode[iCodeIndex];
                pACode[iCodeIndex]=new eInvSyntax;
            }
            else{
                delete pACode[iCodeIndex];
                pACode[iCodeIndex]=new eSymInstrDotExp;
            }
            break;
        case ePS_SYMBOLDEC:
            bSymDeclared=true;
            if (pAToken->kTokenType () == eT_IDENTIFIER){
                pTIdent=static_cast <TIdentifier*> (pAToken);
                pTIdent->vGetValue (cLocalIdentVal);
                vLookUpMnemon (cLocalIdentVal, mnemon, pAMnemonTemp, bFound);
                if (bFound){
                    if (pAMnemonTemp->bIsUnary()){
                        delete pACode[iCodeIndex];
                        pACode[iCodeIndex]=new UnaryInstruction (iCurrentAddress, mnemon, pAMnemonTemp, cLocalIdentVal);
                        pValid=static_cast <Valid*> (pACode[iCodeIndex]);
                        iCurrentAddress+=pValid->iAddressCounter();
                        psState=ePS_CLOSE;
                    }
                    else{
                        psState=ePS_INSTRUCTION;
                    }
                }
                else{
                    delete pACode[iCodeIndex];
                    pACode[iCodeIndex]=new eInvMnemon;
                }
            }
            else if (pAToken->kTokenType () == eT_DOTCOMMAND){
                pTDot=static_cast <TDotCommand*> (pAToken);
                pTDot->vGetValue (cLocalIdentVal);
                vLookUpDot (cLocalIdentVal, dotcom, bFound);
                if (bFound){
                    if (dotcom == eD_END){
                        delete pACode[iCodeIndex];
                        pACode[iCodeIndex]=new DotEnd (iCurrentAddress, eD_END, cLocalIdentVal);
                        pValid=static_cast <Valid*> (pACode[iCodeIndex]);
                        iCurrentAddress+=pValid->iAddressCounter();
                        term=true;
                        psState=ePS_CLOSE;
                    }
                    else if (dotcom == eD_EQUATE){
                        psState=ePS_EQUATE;
                    }
                    else if (dotcom == eD_ASCII){
                        psState=ePS_ASCII;
                    }
                    else{
                        psState=ePS_DOTCOMMAND;
                    }
                }
                else{
                    delete pACode[iCodeIndex];
                    pACode[iCodeIndex]=new eNoDotCom;
                }
            }
            else if (pAToken->kTokenType () == eT_INVALID){
                delete pACode[iCodeIndex];
                pACode[iCodeIndex]=new eInvSyntax;
            }
            else{
                delete pACode[iCodeIndex];
                pACode[iCodeIndex]=new eInstrDotExp;
            }
            break;
        case ePS_DOTCOMMAND:
            if (pAToken->kTokenType () == eT_IDENTIFIER){
                pTIdent=static_cast <TIdentifier*> (pAToken);
                pTIdent->vGetValue (cLocalSecondIdentVal);
                vInstallUndeclaredSymbol(cLocalSecondIdentVal);
                if (dotcom == eD_ADDRSS){
                    delete pACode[iCodeIndex];
                    pACode[iCodeIndex]=new DotComSym (iCurrentAddress, dotcom, cLocalIdentVal, cLocalSecondIdentVal);
                    pValid=static_cast <Valid*> (pACode[iCodeIndex]);
                    iCurrentAddress+=pValid->iAddressCounter();
                    psState=ePS_CLOSE;
                }
                else if (dotcom == eD_EQUATE){
                    delete pACode[iCodeIndex];
                    pACode[iCodeIndex]=new eSymBeforeEquate;
                }
                else{
                    delete pACode[iCodeIndex];
                    pACode[iCodeIndex]=new eConstExp;
                }
            }
            else if (pAToken->kTokenType () == eT_HEXCONSTANT){
                pTHex=static_cast <THexConstant*> (pAToken);
                pTHex->vGetValue (cLocalHexVal);
                if (dotcom == eD_ADDRSS){
                    delete pACode[iCodeIndex];
                    pACode[iCodeIndex]=new eSymExpWithAddrss;
                }
                else if (dotcom == eD_BLOCK){
                    delete pACode[iCodeIndex];
                    if ((cLocalHexVal[0]!='0') || (cLocalHexVal[1]!='0')){
                        pACode[iCodeIndex]=new eConstOverflow;
                    }
                    else{
                        pACode[iCodeIndex]=new DotComHex (iCurrentAddress, dotcom, cLocalIdentVal, cLocalHexVal);
                        pValid=static_cast <Valid*> (pACode[iCodeIndex]);
                        iCurrentAddress+=pValid->iAddressCounter();
                        psState=ePS_CLOSE;
                    }
                }
                else if (dotcom == eD_BURN){
                    if (iBurnCounter == 0){
                        delete pACode[iCodeIndex];
                        pACode[iCodeIndex]=new DotComHex (iCurrentAddress, dotcom, cLocalIdentVal, cLocalHexVal);
                        pValid=static_cast <Valid*> (pACode[iCodeIndex]);
                        iBurnAddr=iCurrentAddress;
                        iCurrentAddress+=pValid->iAddressCounter();
                        psState=ePS_CLOSE;
                        iBurnStart=iHexWordToDecInt(cLocalHexVal);
                        iBurnCounter++;
                    }
                    else{
                        delete pACode[iCodeIndex];
                        pACode[iCodeIndex]=new eOneBurn;
                    }
                }
                else if (dotcom == eD_BYTE){
                    if ((cLocalHexVal[0] == '0') && (cLocalHexVal[1] == '0')){
                        delete pACode[iCodeIndex];
                        pACode[iCodeIndex]=new DotComHex (iCurrentAddress, dotcom, cLocalIdentVal, cLocalHexVal);
                        pValid=static_cast <Valid*> (pACode[iCodeIndex]);
                        iCurrentAddress+=pValid->iAddressCounter();
                        psState=ePS_CLOSE;
                    }
                    else{
                        delete pACode[iCodeIndex];
                        pACode[iCodeIndex]=new eByteOutOfRange;
                    }
                }
                else if (dotcom == eD_EQUATE){
                    delete pACode[iCodeIndex];
                    pACode[iCodeIndex]=new eSymBeforeEquate;
                }
                else if (dotcom == eD_WORD){
                    delete pACode[iCodeIndex];
                    pACode[iCodeIndex]=new DotComHex (iCurrentAddress, dotcom, cLocalIdentVal, cLocalHexVal);
                    pValid=static_cast <Valid*> (pACode[iCodeIndex]);
                    iCurrentAddress+=pValid->iAddressCounter();
                    psState=ePS_CLOSE;
                }
            }
            else if (pAToken->kTokenType () == eT_CHARCONSTANT){
                pTChar=static_cast <TCharConstant*> (pAToken);
                pTChar->vGetValue (cLocalCharVal);
                pTChar->vGetByteValue (cLocalCharByteVal);
                if (dotcom == eD_ADDRSS){
                    delete pACode[iCodeIndex];
                    pACode[iCodeIndex]=new eSymExpWithAddrss;
                }
                else if ((dotcom == eD_BLOCK) || (dotcom == eD_BURN)){
                    delete pACode[iCodeIndex];
                    pACode[iCodeIndex]=new eDecHexExp;
                }
                else if ((dotcom == eD_BYTE) || (dotcom == eD_WORD)){
                    delete pACode[iCodeIndex];
                    pACode[iCodeIndex]=new DotComChar (iCurrentAddress, dotcom, cLocalIdentVal, cLocalCharVal, cLocalCharByteVal);
                    pValid=static_cast <Valid*> (pACode[iCodeIndex]);
                    iCurrentAddress+=pValid->iAddressCounter();
                    psState=ePS_CLOSE;
                }
                else if (dotcom == eD_EQUATE){
                    delete pACode[iCodeIndex];
                    pACode[iCodeIndex]=new eSymBeforeEquate;
                }
            }
            else if (pAToken->kTokenType () == eT_DECCONSTANT){
                pTDec=static_cast <TDecConstant*> (pAToken);
                pTDec->vGetValue (cLocalDecVal);
                if (dotcom == eD_ADDRSS){
                    delete pACode[iCodeIndex];
                    pACode[iCodeIndex]=new eSymExpWithAddrss;
                }
                else if (dotcom == eD_BLOCK){
                    iTemp=iCharToInt (cLocalDecVal);
                    if ((iTemp>=0) && (iTemp<=MAX_BYTE)){
                        delete pACode[iCodeIndex];
                        pACode[iCodeIndex]=new DotComDec (iCurrentAddress, dotcom, cLocalIdentVal, cLocalDecVal);
                        pValid=static_cast <Valid*> (pACode[iCodeIndex]);
                        iCurrentAddress+=pValid->iAddressCounter();
                        psState=ePS_CLOSE;
                    }
                    else{
                        delete pACode[iCodeIndex];
                        pACode[iCodeIndex]=new eConstOverflow;
                    }
                }
                else if (dotcom == eD_BURN){
                    if (iBurnCounter == 0){
                        iTemp=iCharToInt (cLocalDecVal);
                        if ((iTemp>=0) && (iTemp<=MAX_ADDR)){
                            delete pACode[iCodeIndex];
                            pACode[iCodeIndex]=new DotComDec (iCurrentAddress, dotcom, cLocalIdentVal, cLocalDecVal);
                            pValid=static_cast <Valid*> (pACode[iCodeIndex]);
                            iBurnAddr=iCurrentAddress;
                            iCurrentAddress+=pValid->iAddressCounter();
                            psState=ePS_CLOSE;
                            iBurnStart=iCharToInt(cLocalDecVal);
                            iBurnCounter++;
                        }
                        else {
                            delete pACode[iCodeIndex];
                            pACode[iCodeIndex]=new eAddrOverflow;
                        }
                    }
                    else{
                        delete pACode[iCodeIndex];
                        pACode[iCodeIndex]=new eOneBurn;
                    }
                }
                else if (dotcom == eD_BYTE){
                    iTemp=iCharToInt (cLocalDecVal);
                    if ((iTemp>=MIN_BYTE) && (iTemp<=MAX_BYTE)){
                        delete pACode[iCodeIndex];
                        pACode[iCodeIndex]=new DotComDec (iCurrentAddress, dotcom, cLocalIdentVal, cLocalDecVal);
                        pValid=static_cast <Valid*> (pACode[iCodeIndex]);
                        iCurrentAddress+=pValid->iAddressCounter();
                        psState=ePS_CLOSE;
                    }
                    else{
                        delete pACode[iCodeIndex];
                        pACode[iCodeIndex]=new eByteOutOfRange;
                    }
                }
                else if (dotcom == eD_EQUATE){
                    iTemp=iCharToInt (cLocalDecVal);
                    if ((iTemp>=MIN_DEC) && (iTemp<=MAX_DEC)){
                        delete pACode[iCodeIndex];
                        pACode[iCodeIndex]=new eSymBeforeEquate;
                    }
                    else{
                        delete pACode[iCodeIndex];
                        pACode[iCodeIndex]=new eDecOverflow;
                    }
                }
                else if (dotcom == eD_WORD){
                    iTemp=iCharToInt (cLocalDecVal);
                    if ((iTemp>=MIN_DEC) && (iTemp<=MAX_DEC)){
                        delete pACode[iCodeIndex];
                        pACode[iCodeIndex]=new DotComDec (iCurrentAddress, dotcom, cLocalIdentVal, cLocalDecVal);
                        pValid=static_cast <Valid*> (pACode[iCodeIndex]);
                        iCurrentAddress+=pValid->iAddressCounter();
                        psState=ePS_CLOSE;
                    }
                    else{
                        delete pACode[iCodeIndex];
                        pACode[iCodeIndex]=new eDecOverflow;
                    }
                }
            }
            else if (pAToken->kTokenType () == eT_STRING){
                if (dotcom == eD_ADDRSS){
                    delete pACode[iCodeIndex];
                    pACode[iCodeIndex]=new eSymExpWithAddrss;
                }
                else if ((dotcom == eD_BLOCK) || (dotcom == eD_BURN)){
                    delete pACode[iCodeIndex];
                    pACode[iCodeIndex]=new eDecHexExp;
                }
                else if (dotcom == eD_BYTE){
                    pTString=static_cast <TString*> (pAToken);
                    iObjLength=pTString->iObjLength();
                    if (iObjLength == BYTE_LENGTH){
                        pTString->vGetValue (cLocalStringVal);
                        pTString->vGetObjValue (cLocalStringObjVal, iObjLength);
                        delete pACode[iCodeIndex];
                        pACode[iCodeIndex]=new DotComString (iCurrentAddress, dotcom, cLocalIdentVal, cLocalStringVal, cLocalStringObjVal, iObjLength);
                        pValid=static_cast <Valid*> (pACode[iCodeIndex]);
                        iCurrentAddress+=pValid->iAddressCounter();
                        psState=ePS_CLOSE;
                    }
                    else{
                        delete pACode[iCodeIndex];
                        pACode[iCodeIndex]=new eByteStrTooLong;
                    }
                }
                else if (dotcom == eD_WORD){
                    pTString=static_cast <TString*> (pAToken);
                    iObjLength=pTString->iObjLength();
                    if (iObjLength<=WORD_LENGTH){
                        pTString->vGetValue (cLocalStringVal);
                        pTString->vGetObjValue (cLocalStringObjVal, iObjLength);
                        delete pACode[iCodeIndex];
                        pACode[iCodeIndex]=new DotComString (iCurrentAddress, dotcom, cLocalIdentVal, cLocalStringVal, cLocalStringObjVal, iObjLength);
                        pValid=static_cast <Valid*> (pACode[iCodeIndex]);
                        iCurrentAddress+=pValid->iAddressCounter();
                        psState=ePS_CLOSE;
                    }
                    else{
                        delete pACode[iCodeIndex];
                        pACode[iCodeIndex]=new eWordStrTooLong;
                    }
                }
                else if (dotcom == eD_EQUATE){
                    delete pACode[iCodeIndex];
                    pACode[iCodeIndex]=new eSymBeforeEquate;
                }
            }
            else if (pAToken->kTokenType () == eT_INVALIDDEC){
                delete pACode[iCodeIndex];
                pACode[iCodeIndex]=new eNoDecConst;
            }
            else if (pAToken->kTokenType () == eT_INVALIDHEX){
                delete pACode[iCodeIndex];
                pACode[iCodeIndex]=new eNoHexConst;
            }
            else if (pAToken->kTokenType () == eT_INVALIDCHAR){
                delete pACode[iCodeIndex];
                pACode[iCodeIndex]=new eNoCharConst;
            }
            else if (pAToken->kTokenType () == eT_INVALIDSTRING){
                delete pACode[iCodeIndex];
                pACode[iCodeIndex]=new eNoString;
            }
            else if (pAToken->kTokenType () == eT_INVALIDADDR){
                delete pACode[iCodeIndex];
                pACode[iCodeIndex]=new eNoAddr;
            }
            else if (pAToken->kTokenType () == eT_INVALIDCOMMENT){
                delete pACode[iCodeIndex];
                pACode[iCodeIndex]=new eCommentTooLong;
            }
            else if (pAToken->kTokenType () == eT_INVALIDDOTCOMMAND){
                delete pACode[iCodeIndex];
                pACode[iCodeIndex]=new eNoDotCom;
            }
            else if (pAToken->kTokenType () == eT_INVALID){
                delete pACode[iCodeIndex];
                pACode[iCodeIndex]=new eInvSyntax;
            }
            else{
                delete pACode[iCodeIndex];
                pACode[iCodeIndex]=new eConstExp;
            }
            break;
        case ePS_ASCII:
            if (pAToken->kTokenType () == eT_STRING){
                pTString=static_cast <TString*> (pAToken);
                iObjLength=pTString->iObjLength();
                iStrLength=pTString->iLength();
                pTString->vGetValue (cLocalStringVal);
                pTString->vGetObjValue (cLocalAsciiObjVal, iObjLength);
                delete pACode[iCodeIndex];
                pACode[iCodeIndex]=new DotComAscii (iCurrentAddress, dotcom, cLocalIdentVal, cLocalStringVal, iStrLength, cLocalAsciiObjVal, iObjLength);
                pValid=static_cast <Valid*> (pACode[iCodeIndex]);
                iCurrentAddress+=pValid->iAddressCounter();
                psState=ePS_CLOSE;
            }
            else{
                delete pACode[iCodeIndex];
                pACode[iCodeIndex]=new eNoString;
            }
            break;
        case ePS_EQUATE:
            if (pAToken->kTokenType () == eT_HEXCONSTANT){
                pTHex=static_cast <THexConstant*> (pAToken);
                pTHex->vGetValue (cLocalHexVal);
                delete pACode[iCodeIndex];
                vChangeSymValEquate(cLocalSymVal, cLocalHexVal);
                vInstallEquateNode(cLocalSymVal, cLocalHexVal);
                pACode[iCodeIndex]=new DotComHex (iCurrentAddress, dotcom, cLocalIdentVal, cLocalHexVal);
                pValid=static_cast <Valid*> (pACode[iCodeIndex]);
                iCurrentAddress+=pValid->iAddressCounter();
                psState=ePS_CLOSE;
            }
            else if (pAToken->kTokenType () == eT_DECCONSTANT){
                char cVal[ADDR_LENGTH + 1];
                pTDec=static_cast <TDecConstant*> (pAToken);
                pTDec->vGetValue (cLocalDecVal);
                int iTemp=iCharToInt(cLocalDecVal);
                vDecToHexWord(iTemp, cVal);
                vChangeSymValEquate(cLocalSymVal, cVal);
                vInstallEquateNode(cLocalSymVal, cVal);
                delete pACode[iCodeIndex];
                pACode[iCodeIndex]=new DotComDec (iCurrentAddress, dotcom, cLocalIdentVal, cLocalDecVal);
                pValid=static_cast <Valid*> (pACode[iCodeIndex]);
                iCurrentAddress+=pValid->iAddressCounter();
                psState=ePS_CLOSE;
            }
            else if (pAToken->kTokenType () == eT_CHARCONSTANT){
                pTChar=static_cast <TCharConstant*> (pAToken);
                pTChar->vGetValue (cLocalCharVal);
                pTChar->vGetByteValue (cLocalCharByteVal);
                cLocalStringObjVal[0]='0';
                cLocalStringObjVal[1]='0';
                cLocalStringObjVal[2]=cLocalCharByteVal[0];
                cLocalStringObjVal[3]=cLocalCharByteVal[1];
                cLocalStringObjVal[4]='\0';
                delete pACode[iCodeIndex];
                vChangeSymValEquate(cLocalSymVal, cLocalStringObjVal);
                vInstallEquateNode(cLocalSymVal, cLocalStringObjVal);
                pACode[iCodeIndex]=new DotComChar (iCurrentAddress, dotcom, cLocalIdentVal, cLocalCharVal, cLocalCharByteVal);
                pValid=static_cast <Valid*> (pACode[iCodeIndex]);
                iCurrentAddress+=pValid->iAddressCounter();
                psState=ePS_CLOSE;
            }
            else if (pAToken->kTokenType () == eT_STRING){
                pTString=static_cast <TString*> (pAToken);
                iObjLength=pTString->iObjLength();
                if (iObjLength == WORD_LENGTH){
                    pTString->vGetValue (cLocalStringVal);
                    pTString->vGetObjValue (cLocalStringObjVal, iObjLength);
                    delete pACode[iCodeIndex];
                    vChangeSymValEquate(cLocalSymVal, cLocalStringObjVal);
                    vInstallEquateNode(cLocalSymVal, cLocalStringObjVal);
                    pACode[iCodeIndex]=new DotComString (iCurrentAddress, dotcom, cLocalIdentVal, cLocalStringVal, cLocalStringObjVal, iObjLength);
                    pValid=static_cast <Valid*> (pACode[iCodeIndex]);
                    iCurrentAddress+=pValid->iAddressCounter();
                    psState=ePS_CLOSE;
                }
                else if (iObjLength == BYTE_LENGTH){
                    pTString->vGetValue (cLocalStringVal);
                    pTString->vGetObjValue (cLocalCharByteVal, iObjLength);
                    cLocalStringObjVal[0]='0';
                    cLocalStringObjVal[1]='0';
                    cLocalStringObjVal[2]=cLocalCharByteVal[0];
                    cLocalStringObjVal[3]=cLocalCharByteVal[1];
                    cLocalStringObjVal[4]='\0';
                    delete pACode[iCodeIndex];
                    vChangeSymValEquate(cLocalSymVal, cLocalStringObjVal);
                    vInstallEquateNode(cLocalSymVal, cLocalStringObjVal);
                    pACode[iCodeIndex]=new DotComString (iCurrentAddress, dotcom, cLocalIdentVal, cLocalStringVal, cLocalStringObjVal, iObjLength);
                    pValid=static_cast <Valid*> (pACode[iCodeIndex]);
                    iCurrentAddress+=pValid->iAddressCounter();
                    psState=ePS_CLOSE;
                }
                else{
                    delete pACode[iCodeIndex];
                    pACode[iCodeIndex]=new eEquateStrTooLong;
                }
            }
            else{ 
                delete pACode[iCodeIndex];
                pACode[iCodeIndex]=new eInvSyntax;
            }
            break;
        case ePS_INSTRUCTION:
            if (pAToken->kTokenType () == eT_IDENTIFIER){
                pTIdent=static_cast <TIdentifier*> (pAToken);
                pTIdent->vGetValue (cLocalSecondIdentVal);
                vInstallUndeclaredSymbol(cLocalSecondIdentVal);
                psState=ePS_OPRNDSPECSYM;
            }
            else if (pAToken->kTokenType () == eT_HEXCONSTANT){
                pTHex=static_cast <THexConstant*> (pAToken);
                pTHex->vGetValue (cLocalHexVal);
                psState=ePS_OPRNDSPECHEX;
            }
            else if (pAToken->kTokenType () == eT_DECCONSTANT){
                pTDec=static_cast <TDecConstant*> (pAToken);
                pTDec->vGetValue (cLocalDecVal);
                iTemp=iCharToInt (cLocalDecVal);
                if ((iTemp>=MIN_DEC) && (iTemp<=MAX_DEC)){
                    psState=ePS_OPRNDSPECDEC;
                }
                else{
                    delete pACode[iCodeIndex];
                    pACode[iCodeIndex]=new eDecOverflow;
                }
            }
            else if (pAToken->kTokenType () == eT_CHARCONSTANT){
                pTChar=static_cast <TCharConstant*> (pAToken);
                pTChar->vGetValue (cLocalCharVal);
                pTChar->vGetByteValue(cLocalCharByteVal);
                psState=ePS_OPRNDSPECCHAR;
            }
            else if (pAToken->kTokenType () == eT_STRING){
                pTString=static_cast <TString*> (pAToken);
                iObjLength=pTString->iObjLength();
                if (iObjLength<=STRING_OPRND_LENGTH){
                    pTString->vGetValue (cLocalStringVal);
                    pTString->vGetObjValue (cLocalStringObjVal, iObjLength);
                    psState=ePS_OPRNDSPECSTRING;
                }
                else{
                    delete pACode[iCodeIndex];
                    pACode[iCodeIndex]=new eStrOprndTooLong;
                }
            }
            else if (pAToken->kTokenType () == eT_INVALIDDEC){
                delete pACode[iCodeIndex];
                pACode[iCodeIndex]=new eNoDecConst;
            }
            else if (pAToken->kTokenType () == eT_INVALIDHEX){
                delete pACode[iCodeIndex];
                pACode[iCodeIndex]=new eNoHexConst;
            }
            else if (pAToken->kTokenType () == eT_INVALIDCHAR){
                delete pACode[iCodeIndex];
                pACode[iCodeIndex]=new eNoCharConst;
            }
            else if (pAToken->kTokenType () == eT_INVALIDSTRING){
                delete pACode[iCodeIndex];
                pACode[iCodeIndex]=new eNoString;
            }
            else if (pAToken->kTokenType () == eT_INVALIDADDR){
                delete pACode[iCodeIndex];
                pACode[iCodeIndex]=new eNoAddr;
            }
            else if (pAToken->kTokenType () == eT_INVALIDCOMMENT){
                delete pACode[iCodeIndex];
                pACode[iCodeIndex]=new eCommentTooLong;
            }
            else if (pAToken->kTokenType () == eT_INVALIDDOTCOMMAND){
                delete pACode[iCodeIndex];
                pACode[iCodeIndex]=new eNoDotCom;
            }
            else if (pAToken->kTokenType () == eT_INVALID){
                delete pACode[iCodeIndex];
                pACode[iCodeIndex]=new eInvSyntax;
            }
            else{
                delete pACode[iCodeIndex];
                pACode[iCodeIndex]=new eOprndSpecExp;
            }
            break;
        case ePS_OPRNDSPECDEC:
            if (pAToken->kTokenType () == eT_ADDRMODE){
                pTAddress=static_cast <TAddress*> (pAToken);
                pTAddress->vGetValue (cLocalAddrModeVal);
                if (pAMnemonTemp->bValidAddrMode(cLocalAddrModeVal)){
                    delete pACode[iCodeIndex];
                    pACode[iCodeIndex]=new InstructionDec (iCurrentAddress, mnemon, pAMnemonTemp, cLocalIdentVal, cLocalDecVal, cLocalAddrModeVal);
                    pValid=static_cast <Valid*> (pACode[iCodeIndex]);
                    iCurrentAddress+=pValid->iAddressCounter();
                    psState=ePS_CLOSE;
                }
                else{
                    delete pACode[iCodeIndex];
                    pACode[iCodeIndex]=new eNoAddrmode;
                }
            }
            else if (pAToken->kTokenType () == eT_INVALID){
                delete pACode[iCodeIndex];
                pACode[iCodeIndex]=new eInvSyntax;
            }
            else if (pAMnemonTemp->bNoAddrModeRequired()){
                if (pAToken->kTokenType () == eT_EMPTY){
                    delete pACode[iCodeIndex];
                    pACode[iCodeIndex]=new InstructionDec(iCurrentAddress, mnemon, pAMnemonTemp, cLocalIdentVal, cLocalDecVal);
                    pValid=static_cast <Valid*> (pACode[iCodeIndex]);
                    iCurrentAddress+=pValid->iAddressCounter();
                    psState=ePS_FINISH;
                }
                else if (pAToken->kTokenType () == eT_COMMENT){
                    delete pACode[iCodeIndex];
                    pACode[iCodeIndex]=new InstructionDec(iCurrentAddress, mnemon, pAMnemonTemp, cLocalIdentVal, cLocalDecVal);
                    pValid=static_cast <Valid*> (pACode[iCodeIndex]);
                    iCurrentAddress+=pValid->iAddressCounter();
                    psState=ePS_FINISH;
                    pTComm=static_cast <TComment*> (pAToken);
                    pTComm->vGetValue (cLocalCommentVal);
                    iCurrentAddress=iCurrentAddress - pValid->iAddressCounter();
                    vInstallComment (cLocalCommentVal,true);
                    iCurrentAddress+=pValid->iAddressCounter();
                    psState=ePS_COMMENT;
                }
                else{
                    delete pACode[iCodeIndex];
                    pACode[iCodeIndex]=new eAddrCommExp;
                }
            }
            else{
                delete pACode[iCodeIndex];
                pACode[iCodeIndex]=new eAddrExp;
            }
            break;
        case ePS_OPRNDSPECHEX:
            if (pAToken->kTokenType () == eT_ADDRMODE){
                pTAddress=static_cast <TAddress*> (pAToken);
                pTAddress->vGetValue (cLocalAddrModeVal);
                if (pAMnemonTemp->bValidAddrMode(cLocalAddrModeVal)){
                    delete pACode[iCodeIndex];
                    pACode[iCodeIndex]=new InstructionHex (iCurrentAddress, mnemon, pAMnemonTemp, cLocalIdentVal, cLocalHexVal, cLocalAddrModeVal);
                    pValid=static_cast <Valid*> (pACode[iCodeIndex]);
                    iCurrentAddress+=pValid->iAddressCounter();
                    psState=ePS_CLOSE;
                }
                else{
                    delete pACode[iCodeIndex];
                    pACode[iCodeIndex]=new eNoAddrmode;
                }
            }
            else if (pAToken->kTokenType () == eT_INVALID){
                delete pACode[iCodeIndex];
                pACode[iCodeIndex]=new eInvSyntax;
            }
            else if (pAMnemonTemp->bNoAddrModeRequired()){
                if (pAToken->kTokenType () == eT_EMPTY){
                    delete pACode[iCodeIndex];
                    pACode[iCodeIndex]=new InstructionHex(iCurrentAddress, mnemon, pAMnemonTemp, cLocalIdentVal, cLocalHexVal);
                    pValid=static_cast <Valid*> (pACode[iCodeIndex]);
                    iCurrentAddress+=pValid->iAddressCounter();
                    psState=ePS_FINISH;
                }
                else if (pAToken->kTokenType () == eT_COMMENT){
                    delete pACode[iCodeIndex];
                    pACode[iCodeIndex]=new InstructionHex(iCurrentAddress, mnemon, pAMnemonTemp, cLocalIdentVal, cLocalHexVal);
                    pValid=static_cast <Valid*> (pACode[iCodeIndex]);
                    iCurrentAddress+=pValid->iAddressCounter();
                    psState=ePS_FINISH;
                    pTComm=static_cast <TComment*> (pAToken);
                    pTComm->vGetValue (cLocalCommentVal);
                    iCurrentAddress=iCurrentAddress - pValid->iAddressCounter();
                    vInstallComment (cLocalCommentVal, true);
                    iCurrentAddress+=pValid->iAddressCounter();
                    psState=ePS_COMMENT;
                }
                else{
                    delete pACode[iCodeIndex];
                    pACode[iCodeIndex]=new eAddrCommExp;
                }
            }
            else{
                delete pACode[iCodeIndex];
                pACode[iCodeIndex]=new eAddrExp;
            }
            break;
        case ePS_OPRNDSPECCHAR:
            if (pAToken->kTokenType () == eT_ADDRMODE){
                pTAddress=static_cast <TAddress*> (pAToken);
                pTAddress->vGetValue (cLocalAddrModeVal);
                if (pAMnemonTemp->bValidAddrMode(cLocalAddrModeVal)){
                    delete pACode[iCodeIndex];
                    pACode[iCodeIndex]=new InstructionChar (iCurrentAddress, mnemon, pAMnemonTemp, cLocalIdentVal, cLocalCharVal, cLocalAddrModeVal, cLocalCharByteVal);
                    pValid=static_cast <Valid*> (pACode[iCodeIndex]);
                    iCurrentAddress+=pValid->iAddressCounter();
                    psState=ePS_CLOSE;
                }
                else{
                    delete pACode[iCodeIndex];
                    pACode[iCodeIndex]=new eNoAddrmode;
                }
            }
            else if (pAToken->kTokenType () == eT_INVALID){
                delete pACode[iCodeIndex];
                pACode[iCodeIndex]=new eInvSyntax;
            }
            else if (pAMnemonTemp->bNoAddrModeRequired()){
                delete pACode[iCodeIndex];
                pACode[iCodeIndex]=new eNoAddrModeWithChar;
            }
            else{
                delete pACode[iCodeIndex];
                pACode[iCodeIndex]=new eAddrExp;
            }
            break;
        case ePS_OPRNDSPECSTRING:
            if (pAToken->kTokenType () == eT_ADDRMODE){
                pTAddress=static_cast <TAddress*> (pAToken);
                pTAddress->vGetValue (cLocalAddrModeVal);
                if (pAMnemonTemp->bValidAddrMode(cLocalAddrModeVal)){
                    delete pACode[iCodeIndex];
                    pACode[iCodeIndex]=new InstructionString (iCurrentAddress, mnemon, pAMnemonTemp, cLocalIdentVal, cLocalStringVal, cLocalAddrModeVal, cLocalStringObjVal, iObjLength);
                    pValid=static_cast <Valid*> (pACode[iCodeIndex]);
                    iCurrentAddress+=pValid->iAddressCounter();
                    psState=ePS_CLOSE;
                }
                else{
                    delete pACode[iCodeIndex];
                    pACode[iCodeIndex]=new eNoAddrmode;
                }
            }
            else if (pAToken->kTokenType () == eT_INVALID){
                delete pACode[iCodeIndex];
                pACode[iCodeIndex]=new eInvSyntax;
            }
            else if (pAMnemonTemp->bNoAddrModeRequired()){
                delete pACode[iCodeIndex];
                pACode[iCodeIndex]=new eNoAddrModeWithString;
            }
            else{
                delete pACode[iCodeIndex];
                pACode[iCodeIndex]=new eAddrExp;
            }
            break;
        case ePS_OPRNDSPECSYM:
            if (pAToken->kTokenType () == eT_ADDRMODE){
                pTAddress=static_cast <TAddress*> (pAToken);
                pTAddress->vGetValue (cLocalAddrModeVal);
                if (pAMnemonTemp->bValidAddrMode(cLocalAddrModeVal)){
                    delete pACode[iCodeIndex];
                    pACode[iCodeIndex]=new InstructionSym (iCurrentAddress, mnemon, pAMnemonTemp, cLocalIdentVal, cLocalSecondIdentVal, cLocalAddrModeVal);
                    pValid=static_cast <Valid*> (pACode[iCodeIndex]);
                    iCurrentAddress+=pValid->iAddressCounter();
                    psState=ePS_CLOSE;
                }
                else{
                    delete pACode[iCodeIndex];
                    pACode[iCodeIndex]=new eNoAddrmode;
                }
            }
            else if (pAToken->kTokenType () == eT_INVALID){
                delete pACode[iCodeIndex];
                pACode[iCodeIndex]=new eInvSyntax;
            }
            else if (pAMnemonTemp->bNoAddrModeRequired()){
                if (pAToken->kTokenType () == eT_EMPTY){
                    delete pACode[iCodeIndex];
                    pACode[iCodeIndex]=new InstructionSym(iCurrentAddress, mnemon, pAMnemonTemp, cLocalIdentVal, cLocalSecondIdentVal);
                    pValid=static_cast <Valid*> (pACode[iCodeIndex]);
                    iCurrentAddress+=pValid->iAddressCounter();
                    psState=ePS_FINISH;
                }
                else if (pAToken->kTokenType () == eT_COMMENT){
                    delete pACode[iCodeIndex];
                    pACode[iCodeIndex]=new InstructionSym(iCurrentAddress, mnemon, pAMnemonTemp, cLocalIdentVal, cLocalSecondIdentVal);
                    pValid=static_cast <Valid*> (pACode[iCodeIndex]);
                    iCurrentAddress+=pValid->iAddressCounter();
                    psState=ePS_FINISH;
                    pTComm=static_cast <TComment*> (pAToken);
                    pTComm->vGetValue (cLocalCommentVal);
                    iCurrentAddress=iCurrentAddress - pValid->iAddressCounter();
                    vInstallComment (cLocalCommentVal, true);
                    iCurrentAddress+=pValid->iAddressCounter();
                    psState=ePS_COMMENT;
                }
                else{
                    delete pACode[iCodeIndex];
                    pACode[iCodeIndex]=new eAddrCommExp;
                }
            }
            else{
                delete pACode[iCodeIndex];
                pACode[iCodeIndex]=new eAddrExp;
            }
            break;
        case ePS_COMMENT:
            if (pAToken->kTokenType () == eT_EMPTY){
                psState=ePS_FINISH;
            }
            break;
        case ePS_CLOSE:
            if (pAToken->kTokenType () == eT_EMPTY){
                psState=ePS_FINISH;
            }
            else if (pAToken->kTokenType () == eT_COMMENT){
                pTComm=static_cast <TComment*> (pAToken);
                pTComm->vGetValue (cLocalCommentVal);
                iCurrentAddress=iCurrentAddress - pValid->iAddressCounter();
                vInstallComment (cLocalCommentVal, true);
                iCurrentAddress+=pValid->iAddressCounter();
                psState=ePS_COMMENT;
            }
            else if (pAToken->kTokenType () == eT_INVALID){
                delete pACode[iCodeIndex];
                pACode[iCodeIndex]=new eInvSyntax;
            }
            else if (pAToken->kTokenType () == eT_INVALIDCOMMENT){
                delete pACode[iCodeIndex];
                pACode[iCodeIndex]=new eCommentTooLong;
            }
            else if ((pAToken->kTokenType () == eT_CHARCONSTANT) || (pAToken->kTokenType () == eT_DECCONSTANT) || (pAToken->kTokenType () == eT_HEXCONSTANT) || (pAToken->kTokenType () == eT_STRING)){
                delete pACode[iCodeIndex];
                pACode[iCodeIndex]=new eOperandUnexp;
            }
            else{
                delete pACode[iCodeIndex];
                pACode[iCodeIndex]=new eCommExp;
            }
            break;
        default:
            break; // Should not occur
        }
        delete pAToken;
        if (iCodeIndex>=MAX_LINES){
            delete pACode[iCodeIndex];
            pACode[iCodeIndex]=new eTooLong;
            term=true;
        }
        if (iCurrentAddress>=CODE_MAX_SIZE - 2){
            delete pACode[iCodeIndex];
            pACode[iCodeIndex]=new eProgTooLong;
            term=true;
        }
    }
    while ((psState!=ePS_FINISH) && (!pACode[iCodeIndex]->bIsError ()));
}

int main (int argc, char *argv[]){
    bool bTerminate=false;
    iCodeIndex=0;
    int iLineErrors[MAX_LINES]; /*Keeps track of lines containing errors*/
    int iErrorIndex=0; /*Index for iLineErrors[]*/
    Valid* pValid;
    int i;
    int j;
    char sourceFileName[FILE_NAME_LENGTH];
    char objectFileName[FILE_NAME_LENGTH];
    char listingFileName[FILE_NAME_LENGTH];
    bool bTemp=false;
    bool bListing=false;
    bool bVersion=false;
    for (j=0; j<=MAX_LINES; j++){ /*Initialize pACode array*/
        pACode[j]=NULL;
    }
    /*Input trap file*/
    in_file.open("trap");
    if (in_file.fail()){
        cerr << "Could not open trap file." << endl;
        return 1;
    }
    for (i = 0; i < UNIMPLEMENTED_INSTRUCTIONS; i++) {
        vGetTrapLine(i);
    }
    i = 0;
    in_file.close();
   
    /*Analyze input command*/
    if (argc == 1){
        return 0;
    }
    else if (argc == 2){
        if (argv[1][0] == '-'){
            if (strcmp(argv[1], "-v") == 0){
                vVersionNumber();
                return 0;
            }
            else{
                cerr << "usage: asem8 [-v] [[-l] sourceFile]" << endl;
                return 2;
            }
        }
        else{
            if (strlen(argv[1])>FILE_NAME_LENGTH - 3){
                cerr << "Source file name too long" << endl;
                return 2;
            }
            else{
                strncpy (sourceFileName, argv[1], FILE_NAME_LENGTH);
            }
        }
    }
    else if (argc == 3){
        if (strcmp(argv[1], "-v") == 0){
            bVersion=true;
        }
        else if (strcmp(argv[1], "-l") == 0){
            bListing=true;
        }
        else{
            cerr << "usage: asem8 [-v] [[-l] sourceFile]" << endl;
            return 2;
        }
        if (argv[2][0] == '-'){
            cerr << "usage: asem8 [-v] [[-l] sourceFile]" << endl;
            return 2;
        }
        else{
            if (strlen(argv[1])>FILE_NAME_LENGTH - 3){
                cerr << "Source file name too long" << endl;
                return 2;
            }
            else{
                strncpy (sourceFileName, argv[2], FILE_NAME_LENGTH);
            }
        }
    }
    else if (argc == 4){
        if (strcmp(argv[1], "-v") == 0 && strcmp(argv[2], "-l") == 0 && argv[3][0]!='-'){
            bVersion=true;
            bListing=true;
            if (strlen(argv[1])>FILE_NAME_LENGTH - 3){
                cerr << "Source file name too long" << endl;
                return 2;
            }
            else{
                strncpy (sourceFileName, argv[3], FILE_NAME_LENGTH);
            }
        }
        else{
            cerr << "usage: asem8 [-v] [[-l] sourceFile]" << endl;
            return 2;
        }
    }
    else{
        cerr << "usage: asem8 [-v] [[-l] sourceFile]" << endl;
        return 2;
    }
    int k = strlen(sourceFileName);
    if ((sourceFileName[k-1]!='p') || (sourceFileName[k-2]!='e') || (sourceFileName[k-3]!='p') || (sourceFileName[k-4]!='.')){
        cerr << "Source file should have a \".pep\" extension" << endl;
        return 2;
    }
    in_file.open(sourceFileName);
    if (in_file.fail()){
        cerr << "Could not open " << sourceFileName << "." << endl;
        return 3;
    }
    if (bVersion){
        vVersionNumber();
    }
    vInitGlobalTables ();
    while (!(in_file.eof() || bTerminate)){ /*First pass of assembler*/
        vGetLine();
        vProcessSourceLine (bTerminate);
        if (pACode[iCodeIndex]->bIsError()){
            iLineErrors[iErrorIndex++]=iCodeIndex;
        }
        iCodeIndex++;
    } 
    in_file.close();
    sUndeclaredsSymbolNode* q;
    i=0;
    while (pUndeclaredSym!=NULL){ /*Check for undeclared symbols and resolve addresses*/
        if (!bLookUpSymbol(pUndeclaredSym->cSymID)){
            delete pACode[pUndeclaredSym->iLine];
            pACode[pUndeclaredSym->iLine]=new eSymNotDefined;
            int iTemp=0;
            while ((i<iErrorIndex) && (iLineErrors[i]<pUndeclaredSym->iLine)){
                i++;
            }
            if (iLineErrors[i]!=pUndeclaredSym->iLine){
                iTemp=iLineErrors[i];
                iLineErrors[i]=pUndeclaredSym->iLine; /*Insert new error line number*/
                for (j=iErrorIndex + 1; j>i + 1; j--){
                    iLineErrors[j]=iLineErrors[j - 1]; /*Shift values to the right*/
                }
                iLineErrors[j]=iTemp;
            }
            iErrorIndex++;
        }
        q=pUndeclaredSym; /*Deallocate pUndeclaredSym linked list*/
        pUndeclaredSym=pUndeclaredSym->pNext;
        delete q;
    }
    if ((iBurnCounter>0) && (iErrorIndex == 0)){ /*Change addresses and symbol values if a .BURN was encountered*/
        iBurnStart=iBurnStart - iCurrentAddress + 1;
        vChangeSymValBurn(iBurnStart);
        sEquateNode* p=pEquate;
        while (p!=NULL){
            vChangeSymValEquate(p->cSymID, p->cSymValue);
            p=p->pNext;
        }
        iBurnAddr=iBurnAddr + iBurnStart;
        for (iSecPassCodeIndex=0; iSecPassCodeIndex<iCodeIndex; iSecPassCodeIndex++){
            pValid=static_cast <Valid*> (pACode[iSecPassCodeIndex]);
            pValid->vBurnAddressChange();
        }
    }
    if ((iErrorIndex == 0) && (bTerminate) && (bListing)){ /*Create assembler listing*/
        strncpy (listingFileName, sourceFileName, FILE_NAME_LENGTH);
        strcat(listingFileName, "l");
        out_file.open(listingFileName);
        out_file << setiosflags(ios::fixed) << setiosflags(ios::showpoint)
                 << setprecision(2);
        out_file << "-------------------------------------------------------------------------------" << endl;
        out_file << "      Object" << endl;/*6 spaces*/
        if (pSymbol == NULL){
            out_file << "Addr  code   Mnemon  Operand       Comment" << endl;/*7 spaces*/
        }
        else{
            out_file << "Addr  code   Symbol   Mnemon  Operand       Comment" << endl;
        }
        out_file << "-------------------------------------------------------------------------------" << endl;
        for (iSecPassCodeIndex=0; iSecPassCodeIndex<iCodeIndex; iSecPassCodeIndex++){
            pACode[iSecPassCodeIndex]->vGenerateCode ();
            if ((pComment!=NULL) && (pComment->iLine == iSecPassCodeIndex)){
                if (pComment->bNonemptyLine){
                    if (pSymbol == NULL){
                        pComment->cComment[COMMENT_LENGTH_NONEMPTY_NO_SYMBOLS - 1]='\0';
                    }
                    else{
                        pComment->cComment[COMMENT_LENGTH_NONEMPTY - 1]='\0';
                    }
                }
                out_file << ";" << pComment->cComment;
                sCommentNode* p=pComment; /*Deallocate pComment linked list*/
                pComment=pComment->pNext;
                delete p;
            }
            out_file << endl;
        }
        out_file << "-------------------------------------------------------------------------------" << endl;
        if (pSymbol!=NULL) { /*Output symbol table for assembler listing*/
            out_file << endl << endl;
            out_file << "Symbol table" << endl;
            out_file << "--------------------------------------" << endl;
            out_file << "Symbol    Value        Symbol    Value" << endl;/*8 spaces*/
            out_file << "--------------------------------------" << endl;
            sSymbolNode* p=pSymbol;
            bTemp=false;
            while (p!=NULL){
                out_file << p->cSymID;
                vSymbolListingBuffer(p->cSymID);
                out_file << " " << p->cSymValue;
                p=p->pNext;
                if (bTemp){
                    out_file << endl;
                    bTemp=false;
                }
                else{
                    vBlankSymbolColumn();
                    bTemp=true;
                }
            }
            if (bTemp){
                out_file << endl;
            }
            out_file << "--------------------------------------" << endl;
        }
        out_file.close();
    }
    if ((iErrorIndex == 0) && (bTerminate)) {/*Generate object file*/
        strncpy (objectFileName, sourceFileName, FILE_NAME_LENGTH);
        strcat(objectFileName, "o");
        out_file.open(objectFileName);
        out_file << setiosflags(ios::fixed) << setiosflags(ios::showpoint)
                 << setprecision(2);
        for (iSecPassCodeIndex=0; iSecPassCodeIndex<iCodeIndex; iSecPassCodeIndex++){
            pValid=static_cast <Valid*> (pACode[iSecPassCodeIndex]);
            pValid->vGenerateHexCode (false);
        }
        out_file << "zz" << endl;
        out_file.close();
    }
    else {
        /*Errors were detected*/
        if (!bTerminate) {/*To account for absence of .END pseudo-op*/
            delete pACode[iCodeIndex];
            pACode[iCodeIndex]=new eNoEnd;
            iLineErrors[iErrorIndex++]=iCodeIndex;
        }
        cerr << iErrorIndex;
        if (iErrorIndex == 1){
            cerr << " error was detected. No object code generated." << endl;
        }
        else{
            cerr << " errors were detected. No object code generated." << endl;
        }
        for (i=0; i<iErrorIndex; i++) {/*Generate error messages*/
            cerr << "Error on line "<< iLineErrors[i] + 1 << ": ";
            pACode[iLineErrors[i]]->vGenerateCode ();
        }
    }
    sSymbolNode* p;
    while (pSymbol!=NULL) {/*Deallocate pSymbol linked list*/
        p=pSymbol;
        pSymbol=pSymbol->pNext;
        delete p;
    }
    sSymbolOutputNode* qTemp;
    while (pSymbolOutput!=NULL) {/*Deallocate pSymbolOutput linked list*/
        qTemp=pSymbolOutput;
        pSymbolOutput=pSymbolOutput->pNext;
        delete qTemp;
    }
    sEquateNode* pTemp;
    while (pEquate!=NULL) {/*Deallocate pEquate linked list*/
        pTemp=pEquate;
        pEquate=pEquate->pNext;
        delete pTemp;
    }
    for (i=0; i<=iCodeIndex; i++) {/*Deallocate pACode array*/
        delete pACode[i];
    }
    return 0;
}
