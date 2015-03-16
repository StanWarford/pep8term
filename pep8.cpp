//  File: pep8.cpp
//  Simulator of the Pep/8 computer as described in "Computer Systems",
//  Fourth edition, J. Stanley Warford, Jones and Bartlett, Publishers,
//  2010.  ISBN 978-0-7637-7144-7
//  Pepperdine University, Malibu, CA 90265
//  Stan.Warford@pepperdine.edu

//  Released under the GNU General Public License without warrenty
//  as described in http://www.gnu.org/copyleft/gpl.html

//  Version history:

//  February 2015
//  Unix/8.3 Eliminated compiler warning.
//  Stan Warford

//  February 2010
//  UNIX/8.2 Increased LINE_LENGTH from 128 to 1024 to allow for longer lines
//  of object code input.
//  Stan Warford

//  March 2009
//  UNIX/8.1 Corrected allowed addressing modes for SUBSP.
//  Stan Warford

//  December 2005
//  UNIX/8.0 Implemented new instruction set and addressing modes.
//  Fixed trace options single step and scroll. Added adjust display option.
//  Tip Aroonvantanaporn and Stan Warford

//  March 2003
//  UNIX/7.5 Changed & to && in a test in the COMPr instruction.
//  Matt Wells

//  February 2003
//  UNIX/7.4 Fixed an error that prevented files from being reopened without
//  the clear() statement.
//  John Grogg

//  January 2003
//  UNIX/7.3 Fixed an error message misspelling. Fixed a bug in an if
//  statement that tested with = instead of == when incrementing PC by two.
//  Stan Warford

//  December 2002
//  UNIX/7.2 Increased file name length from 32 to 64. Changed usage.
//  Changed CHARO to allow the printing of an 8-bit character. Previous
//  versions masked out the most significant bit allowing only a 7-bit
//  ASCII character to be output. COMPr instruction now sets NZ correctly
//  even when the comparison subtraction overflows. Stan Warford

//  March 2002
//  UNIX/7.1 translated from Pascal to C++ by Scott Mace as an undergraduate
//  project. Eliminated the nontext os ROM file, which is now simply the
//  text object output "os.o" of the asem7 translation of file "os".

//  UNIX/7.0 modified by Stan Warford from Version UNIX/6.0.
//  Changed the file name types to be compatible with the GNU gpc compiler.

//  May 1997
//  UNIX/6.0 modified by Stan Warford to change the instruction set from
//  Pep/5 to Pep/6.
//  Deleted ADDB, BRN, BRZ, BRNZ, SUBSP, and NOP and added BRLE,
//  BRLT, BREQ, BRNE, BRGE, and BRGT.  Switched eA_INDEXED and stack relative
//  addressing mode specifiers.  eA_INDEXED addressing is now defined as
//  Oprnd = Mem [B + X].  Instructions with eA_INDEXED addressing are now unary.
//  Compatible with versions 6.x of assembler and 6.x of operating system.

//  May 1988
//  UNIX/4.0 modified by Stan Warford to change CHARI and CHARO to be byte
//  instructions instead of word instructions.  Also changed the identifiers
//  for the values of eRegSpecType and eAddrModeType enumerated types.
//  Changed the newline character from <CR> to <LF> on CHARI.  Changed the
//  reset and interrupt routines to implement a separate stack area for the
//  operating system.  Compatible with versions 4.x of assembler and 4.x of
//  operating system.

//  October 1987
//  UNIX/3.1 modified by Gerry St. Romain to correct bug in hex address
//  specification for Dump address.  Procedures <Parse> and <DecodeAddress>
//  were replaced.  Function <HexToDec> was deleted.
//  UNIX/3.0  Ported by Stan Warford from Version UCSD/2.0
//  Modified eA_INDEXED addressing mode to eliminate the special case with
//  zero operand.         

//  UCSD/2.0  Written in Pascal by John Rooker as an undergraduate computer
//  science project.

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <iomanip>
#include <ctype.h>
#include <unistd.h>
#include <string>
#include <stdio.h>

using namespace std;

const int MEMORY_SIZE        = 65536;
const int TOP_OF_MEMORY      = 65535;
const int USER_SP            = 65528; //User stack pointer vector.
const int SYSTEM_SP          = 65530; //System stack pointer vector.
const int LOADER_PC          = 65532; //Program counter vector.
const int INTR_PC            = 65534; //Interrupt program counter vector.
const int FILE_NAME_LENGTH   = 64;
const int HEX_BYTE_LENGTH    = 2;
const int HEX_WORD_LENGTH    = 4;
const int UNARY_OPCODES      = 15;
const int BYTE_INSTRUCTION   = 4;
const int LINE_FEED          = 10;
const int CARRIAGE_RETURN    = 13;
const int LINE_LENGTH        = 1024;  //Maximum length of a line of code
const int TRAPS              = 8;     //Number of Traps
const int MNEMON_LENGTH      = 8;
const int UNIMPLEMENTED_INSTRUCTIONS = 8; // Number of unimplemented mnemonics
const int UNARY_TRAPS = 4;                // Number of unimplemented mnemonics guaranteed to be unary
const int IMMEDIATE = 1;                // 2^0. Powers of two to represent addressing mode bitset
const int DIRECT = 2;                   // 2^1
const int INDIRECT = 4;                 // 2^2
const int STACK_RELATIVE = 8;           // 2^3
const int STACK_RELATIVE_DEFERRED = 16; // 2^4
const int INDEXED = 32;                 // 2^5
const int STACK_INDEXED = 64;           // 2^6
const int STACK_INDEXED_DEFERRED = 128; // 2^7

//Enumerated Types
enum MnemonicOpcodes   //All possible opcodes
{
    eM_STOP, eM_RETTR, eM_MOVSPA, eM_MOVFLGA, eM_BR, eM_BRLE, eM_BRLT, eM_BREQ, 
    eM_BRNE, eM_BRGE, eM_BRGT, eM_BRV, eM_BRC, eM_CALL, eM_NOTr, eM_NEGr, 
    eM_ASLr, eM_ASRr, eM_ROLr, eM_RORr, eM_UNIMP0, eM_UNIMP1, eM_UNIMP2, 
    eM_UNIMP3, eM_UNIMP4, eM_UNIMP5, eM_UNIMP6, eM_UNIMP7, eM_CHARI, eM_CHARO, 
    eM_RETn, eM_ADDSP, eM_SUBSP, eM_ADDr, eM_SUBr, eM_ANDr, eM_ORr, eM_CPr, 
    eM_LDr, eM_LDBYTEr, eM_STr, eM_STBYTEr
};

enum eRegSpecType  { eR_R_IS_ACCUMULATOR, eR_R_IS_INDEX_REG }; // 8 bits, unsigned

enum eAddrModeType 
{ 
    eA_IMMEDIATE, eA_DIRECT, eA_INDIRECT, eA_STACK_REL, 
    eA_STACK_REL_DEF, eA_INDEXED, eA_STACK_IND, eA_STACK_IND_DEF 
};

enum eTraceMd { eT_TR_OFF, eT_TR_PROGRAM, eT_TR_TRAPS, eT_TR_LOADER };

//**** Global Records
struct sRegisterType                  // internal CPU registers
{
    int iHigh;                      // most significant byte
    int iLow;                       // least significant byte
};
struct sIRRecType
{
    int iInstr_Spec;                //  8 bits
    sRegisterType sR_OprndSpec;     // 16 bits
};

/*Contains information about user-defined instructions*/
struct sUnimplementedMnemonNode{
    char cID[MNEMON_LENGTH + 1]; //Name of unimplemented opcode
    int iAddrMode;
};

//**** Global Variables
char cHexTable[16];
eTraceMd eTraceMode;
int iMemory[MEMORY_SIZE];
int iRomStartAddr;
char TrapMnemon[TRAPS][MNEMON_LENGTH + 1];
int iUnaryOpcodes[UNARY_OPCODES];
int ByteInstructions[BYTE_INSTRUCTION];
char cCommand[LINE_LENGTH];
bool bLoading;              // Set when loading object file
bool bMachineReset;         // To insure initial load on startup
bool bSingleStep;           // For tracing single step
bool bScrollingTrace;       // For tracing until completion
int iAddrMode;              // Addressing mode integer type
int iRegType;               // Register type integer type
eAddrModeType eA_AddrMode;  // Addressing mode enumerated type
eRegSpecType eR_RegType;    // Register type enumerated type
int nValue;                 // n values

// Input/Output
ifstream trapFile;
ifstream chariInputStream;
ofstream charoOutputStream;
bool bKeyboardInput;       // for program application, used by CHARI
bool bScreenOutput;        // for program application, used by CHARO
bool bBufferIsEmpty;
char cInFileName[FILE_NAME_LENGTH];
char cOutFileName[FILE_NAME_LENGTH];
int numTerminalLines;

//**** Pep/8 CPU registers
sRegisterType sR_Accumulator, sR_IndexRegister, sR_StackPointer, sR_ProgramCounter; // 16 bits
sIRRecType sIR_InstrRegister; // 24 bits
bool bStatusN, bStatusZ, bStatusV, bStatusC;

//**** Constant registers
sRegisterType sR_AtZero, sR_One, sR_Two, sR_NegOne, sR_NegTwo, sR_NegThree;

//**** Keyboard buffer global variables for unbuffering the
//**** UNIX buffered line on interactive input
char cLine[LINE_LENGTH]; //Array of characters for a line of code
int iLineIndex; //Index of line array

//**** Stores the next line of assembly language code to be translated in global cLine[].
void vGetLine(istream& input)
{
    input.getline(cLine, LINE_LENGTH);
    if ((!input.eof ()) && (input.gcount() > 0))
    {
        cLine[input.gcount() - 1] = '\n';
    }
    else
    {
        cLine[input.gcount()] = '\n';
    }
    iLineIndex = 0;
    bBufferIsEmpty = false;
}

//**** Gets the next character to be processed
void vAdvanceInput (char& ch)
{
    ch = cLine[iLineIndex++];
    bBufferIsEmpty = (ch == '\n');
}

//**** Backs up the input
void vBackUpInput ()
{
    iLineIndex--;
}

bool bIsHexDigit (char cChar)
{
    return (((cChar >= 'A') && (cChar <= 'F')) ||
            ((cChar >= 'a') && (cChar <= 'f')) || isdigit(cChar));
}

bool bSearchIntArray (int iArray[], int iArrayLength, int iNum)
{
    int i = 0;
    iArray[iArrayLength] = iNum;
    while (iArray[i] != iNum)
    {
        i++;
    }
    return i != iArrayLength;
}

//**** iInstr_Spec procedures ****
//**** Adds 2 byte pairs and returns in result.  (One word adder)
void Adder (sRegisterType Op1, sRegisterType Op2, sRegisterType& Result,
            bool& Carry, bool& Ovflw)
{
    int Temp;
    Temp = Op1.iLow + Op2.iLow;
    if (Temp > 255)
    {
        Result.iLow = Temp - 256;
        Temp = Op1.iHigh + Op2.iHigh + 1;                 //Carry from low order
    }
    else
    {
        Result.iLow = Temp;
        Temp = Op1.iHigh + Op2.iHigh;
    }
    if (Temp > 255)
    {
        Result.iHigh = Temp - 256;
        Carry = true;
    }
    else
    {
        Result.iHigh = Temp;
        Carry = false;
    }
    Ovflw = (((Op1.iHigh <= 127) && (Op2.iHigh <= 127) &&   // Pos/Pos/Neg
              (Result.iHigh > 127)) ||                       //     or
             ((Op1.iHigh >= 128) && (Op2.iHigh >= 128) &&   // Neg/Neg/Pos
              (Result.iHigh < 128)));
}

//**** Adds 2 byte pairs and returns in result.  (One word adder)
//**** Same as Adder except carry and overflow are not detected.
void FastAdder (sRegisterType Op1, sRegisterType Op2, sRegisterType& Result)
{
    int iTemp;
    iTemp = Op1.iLow + Op2.iLow;
    if (iTemp > 255)
    {
        Result.iLow = iTemp - 256;
        iTemp = Op1.iHigh + Op2.iHigh + 1;                 // Carry from low order
    }
    else
    {
        Result.iLow = iTemp;
        iTemp = Op1.iHigh + Op2.iHigh;
    }
    if (iTemp > 255)
    {
        Result.iHigh = iTemp - 256;
    }
    else
    {
        Result.iHigh = iTemp;
    }
}

//**** Subtracts Op2 from Op1 and returns in result.  (One word)
void Subtractor (sRegisterType Op1, sRegisterType Op2, sRegisterType& Result,
                 bool& Carry, bool& Ovflw)
{
    int iTemp;
    iTemp = Op1.iLow - Op2.iLow;
    if (iTemp < 0)
    {
        Result.iLow = iTemp + 256;
        iTemp = Op1.iHigh - Op2.iHigh - 1;               // Borrow from high order
    }
    else
    {
        Result.iLow = iTemp;
        iTemp = Op1.iHigh - Op2.iHigh;
    }
    if (iTemp < 0)
    {
        Result.iHigh = iTemp + 256;
        Carry = true;
    }
    else
    {
        Result.iHigh = iTemp;
        Carry = false;
    }
    Ovflw = (((Op1.iHigh <= 127) && (Op2.iHigh >= 128) &&   // Pos/Neg/Neg
              (Result.iHigh > 127)) ||                       //     or
             ((Op1.iHigh >= 128) && (Op2.iHigh <= 127) &&   // Neg/Pos/Pos
              (Result.iHigh < 128)));
}

//**** Process the Instruction Specifier (8 bits)
int GetAddressingModeOneBit (int iInstr_Spec) 
{
    return iInstr_Spec % 2 == 0 ? 0 : 5;
}

int GetAddressingModeThreeBits (int iInstr_Spec) 
{
    return (iInstr_Spec % 8);
}

int GetRegisterTypeLastBit (int iInstr_Spec) 
{
    return (iInstr_Spec % 2);
}

int GetRegisterTypeFourthBit (int iInstr_Spec) 
{
    return ((iInstr_Spec / 8) % 2);
}

int GetNValueThreeBits (int iInstr_Spec) 
{
    return (iInstr_Spec % 8);
}

MnemonicOpcodes instr_SpecToMnemon(int iInstr_Spec) 
{
    if (iInstr_Spec == 0) { return eM_STOP; }
    else if (iInstr_Spec == 1) { return eM_RETTR; }
    else if (iInstr_Spec == 2) { return eM_MOVSPA; }
    else if (iInstr_Spec == 3) { return eM_MOVFLGA; }
    else if (iInstr_Spec <= 5) { return eM_BR; }
    else if (iInstr_Spec <= 7) { return eM_BRLE; }
    else if (iInstr_Spec <= 9) { return eM_BRLT; }
    else if (iInstr_Spec <= 11) { return eM_BREQ; }
    else if (iInstr_Spec <= 13) { return eM_BRNE; }
    else if (iInstr_Spec <= 15) { return eM_BRGE; }
    else if (iInstr_Spec <= 17) { return eM_BRGT; }
    else if (iInstr_Spec <= 19) { return eM_BRV; }
    else if (iInstr_Spec <= 21) { return eM_BRC; }
    else if (iInstr_Spec <= 23) { return eM_CALL; }
    else if (iInstr_Spec <= 25) { return eM_NOTr; }
    else if (iInstr_Spec <= 27) { return eM_NEGr; }
    else if (iInstr_Spec <= 29) { return eM_ASLr; }
    else if (iInstr_Spec <= 31) { return eM_ASRr; }
    else if (iInstr_Spec <= 33) { return eM_ROLr; }
    else if (iInstr_Spec <= 35) { return eM_RORr; } 
    else if (iInstr_Spec == 36) { return eM_UNIMP0; }
    else if (iInstr_Spec == 37) { return eM_UNIMP1; }
    else if (iInstr_Spec == 38) { return eM_UNIMP2; }
    else if (iInstr_Spec == 39) { return eM_UNIMP3; }
    else if (iInstr_Spec <= 47) { return eM_UNIMP4; }
    else if (iInstr_Spec <= 55) { return eM_UNIMP5; }
    else if (iInstr_Spec <= 63) { return eM_UNIMP6; }
    else if (iInstr_Spec <= 71) { return eM_UNIMP7; }    
    else if (iInstr_Spec <= 79) { return eM_CHARI; }
    else if (iInstr_Spec <= 87) { return eM_CHARO; }
    else if (iInstr_Spec <= 95) { return eM_RETn; }
    else if (iInstr_Spec <= 103) { return eM_ADDSP; }
    else if (iInstr_Spec <= 111) { return eM_SUBSP; }
    else if (iInstr_Spec <= 127) { return eM_ADDr; }
    else if (iInstr_Spec <= 143) { return eM_SUBr; }
    else if (iInstr_Spec <= 159) { return eM_ANDr; }
    else if (iInstr_Spec <= 175) { return eM_ORr; }
    else if (iInstr_Spec <= 191) { return eM_CPr; }
    else if (iInstr_Spec <= 207) { return eM_LDr; }
    else if (iInstr_Spec <= 223) { return eM_LDBYTEr; }
    else if (iInstr_Spec <= 239) { return eM_STr; }
    else { return eM_STBYTEr; }
}

eAddrModeType ProcessAddressingMode (int AddrMode) 
{
    switch (AddrMode) 
    {
    case 0: return eA_IMMEDIATE; break;
    case 1: return eA_DIRECT; break;
    case 2: return eA_INDIRECT; break;
    case 3: return eA_STACK_REL; break;
    case 4: return eA_STACK_REL_DEF; break;
    case 5: return eA_INDEXED; break;
    case 6: return eA_STACK_IND; break;
    case 7: return eA_STACK_IND_DEF; break;
    default: return eA_IMMEDIATE;
    }
}

eRegSpecType ProcessRegisterType (int RegType) 
{
    switch (RegType) 
    {
    case 0: return eR_R_IS_ACCUMULATOR; break;
    case 1: return eR_R_IS_INDEX_REG; break;
    default: return eR_R_IS_ACCUMULATOR; break; // Should not occur
    }
}

void PrntMnemon (ostream& output)
{
    switch (instr_SpecToMnemon(sIR_InstrRegister.iInstr_Spec))
    {
    case eM_STOP: output << "STOP     "; break;
    case eM_RETTR: output << "RETTR    "; break;
    case eM_MOVSPA: output << "MOVESPA  "; break;
    case eM_MOVFLGA: output << "MOVFLGA  "; break;

    case eM_BR: output << "BR       "; break;
    case eM_BRLE: output << "BRLE     "; break;
    case eM_BRLT: output << "BRLT     "; break;
    case eM_BREQ: output << "BREQ     "; break;
    case eM_BRNE: output << "BRNE     "; break;
    case eM_BRGE: output << "BRGE     "; break;
    case eM_BRGT: output << "BRGT     "; break;
    case eM_BRV: output << "BRV      "; break;
    case eM_BRC: output << "BRC      "; break;
    case eM_CALL: output << "CALL     "; break;

    case eM_NOTr: output << "NOT"; break; 
    case eM_NEGr: output << "NEG"; break;   
    case eM_ASLr: output << "ASL"; break;    
    case eM_ASRr: output << "ASR"; break;    
    case eM_ROLr: output << "ROL"; break;    
    case eM_RORr: output << "ROR"; break;   

    case eM_UNIMP0: output << TrapMnemon[0]; break;  // NOP0
    case eM_UNIMP1: output << TrapMnemon[1]; break;  // NOP1
    case eM_UNIMP2: output << TrapMnemon[2]; break;  // NOP2
    case eM_UNIMP3: output << TrapMnemon[3]; break;  // NOP3
    case eM_UNIMP4: output << TrapMnemon[4]; break;  // NOP
    case eM_UNIMP5: output << TrapMnemon[5]; break;  // DECI 
    case eM_UNIMP6: output << TrapMnemon[6]; break;  // DECO 
    case eM_UNIMP7: output << TrapMnemon[7]; break;  // STRO 

    case eM_CHARI: output << "CHARI    "; break;
    case eM_CHARO: output << "CHARO    "; break;

    case eM_RETn: output << "RET"; break; 
           
    case eM_ADDSP: output << "ADDSP    "; break;
    case eM_SUBSP: output << "SUBSP    "; break;
                 
    case eM_ADDr: output << "ADD"; break;
    case eM_SUBr: output << "SUB"; break;
    case eM_ANDr: output << "AND"; break;
    case eM_ORr: output << "OR"; break;
    case eM_CPr: output << "CP"; break;

    case eM_LDr: output << "LD"; break; 
    case eM_LDBYTEr: output << "LDBYTE"; break; 
    case eM_STr: output << "ST"; break;  
    case eM_STBYTEr: output << "STBYTE"; break;  
    }
      
    MnemonicOpcodes tempMn = instr_SpecToMnemon(sIR_InstrRegister.iInstr_Spec);
    if ((eM_NOTr <= tempMn && tempMn <= eM_RORr) || eM_ADDr <= tempMn)
    {
        switch (eR_RegType)
        {
        case eR_R_IS_ACCUMULATOR: output << "A"; break;
        case eR_R_IS_INDEX_REG: output << "X"; break;
        }
      
        //Column alignment adjustment
        if (tempMn <= eM_ANDr) {
            output << "     ";
        }
        else if (tempMn == eM_ORr || tempMn == eM_CPr || tempMn == eM_LDr || tempMn == eM_STr) {
            output << "      ";
        }
        else {
            output << "  ";
        }
    }
    else if (tempMn == eM_RETn) 
    {
        output << nValue << "     ";
    }
    else if (eM_UNIMP0 <= tempMn && tempMn <= eM_UNIMP7)
    {
        output << " ";
    }
}

//**** Reads one word:  Rslt.iHigh = Mem [Loc], Rslt.iLow = Mem [Loc + 1]
void MemRead (sRegisterType Loc, sRegisterType& Rslt)
{
    int iTemp;
    if (Loc.iHigh < MEMORY_SIZE / (256))
    {
        iTemp = Loc.iHigh * (256) + Loc.iLow;
        Rslt.iHigh = iMemory[iTemp];
        if (iTemp < TOP_OF_MEMORY)
        {
            Rslt.iLow = iMemory[iTemp + 1];
        }
        else
        {
            Rslt.iLow = 0;
        }
    }
    else
    {
        Rslt.iHigh = 0;
        Rslt.iLow = 0;
    }
}

//**** Reads one byte from Mem [Loc] and returns in Byte
void MemByteRead (sRegisterType Loc, int& iByte)
{
    int iTemp;
    if (Loc.iHigh < MEMORY_SIZE / (256))
    {
        iTemp = Loc.iHigh * (256) + Loc.iLow;
        iByte = iMemory[iTemp];
    }
    else
    {
        iByte = 0;
    }
}

//**** Writes one word:  Reg.iHigh to Mem [Loc] and Reg.iLow to Mem[Loc + 1]
void MemWrite (sRegisterType Reg, sRegisterType Loc)
{
    int iTemp;
    if (Loc.iHigh < MEMORY_SIZE / (256))
    {
        iTemp = Loc.iHigh * (256) + Loc.iLow;
        if (iTemp < iRomStartAddr)
        {
            iMemory[iTemp] = Reg.iHigh;
        }
        if (iTemp < iRomStartAddr - 1)
        {
            iMemory[iTemp + 1] = Reg.iLow;
        }
    }
}

//**** Writes one byte to Mem [Loc]
void MemByteWrite (int iByte, sRegisterType Loc)
{
    int iTemp;
    if (Loc.iHigh < MEMORY_SIZE / (256))
    {
        iTemp = Loc.iHigh * (256) + Loc.iLow;
        if (iTemp < iRomStartAddr)
        {
            iMemory[iTemp] = iByte;
        }
    }
}

//**** Determine operand based on addressing mode in instr. register
//**** Sets Oprnd to the operand when the addressing mode is immediate,
//**** but to the address of the operand for the other seven modes
void AddrProcessor (sRegisterType& Operand)
{
    sRegisterType temp1, temp2;
    switch (eA_AddrMode)
    {
    case eA_IMMEDIATE:
        Operand = sIR_InstrRegister.sR_OprndSpec;
        break;
    case eA_DIRECT:
        Operand = sIR_InstrRegister.sR_OprndSpec;
        break;
    case eA_INDIRECT:
        MemRead (sIR_InstrRegister.sR_OprndSpec, Operand);
        break;
    case eA_STACK_REL:
        FastAdder (sR_StackPointer, sIR_InstrRegister.sR_OprndSpec, Operand);
        break;
    case eA_STACK_REL_DEF:
        FastAdder (sR_StackPointer, sIR_InstrRegister.sR_OprndSpec, temp1);
        MemRead (temp1, Operand);
        break;
    case eA_INDEXED:
        FastAdder (sR_IndexRegister, sIR_InstrRegister.sR_OprndSpec, Operand);
        break;
    case eA_STACK_IND: 
        FastAdder (sR_StackPointer, sIR_InstrRegister.sR_OprndSpec, temp1);
        FastAdder (temp1, sR_IndexRegister, Operand);
        break;
    case eA_STACK_IND_DEF: 
        FastAdder (sR_StackPointer, sIR_InstrRegister.sR_OprndSpec, temp1);
        MemRead (temp1, temp2);
        FastAdder (temp2, sR_IndexRegister, Operand);
        break;
    }
}

void LoadReg (sRegisterType& Reg)
{
    sRegisterType Operand;
    AddrProcessor (Operand);
    if (eA_AddrMode == eA_IMMEDIATE)
    {
        Reg = Operand;
    }
    else
    {
        MemRead (Operand, Reg);
    }
}

void SetNZBits (sRegisterType Reg)
{
    bStatusN = (Reg.iHigh > 127);
    bStatusZ = ((Reg.iHigh == 0) && (Reg.iLow == 0));
}

//**** Prints program counter value of instruction that caused machine
//**** error.  Message is 20 characters long.
void PrntRunLoc()
{
    sRegisterType LastLoc;
    if (bSearchIntArray(iUnaryOpcodes, UNARY_OPCODES, instr_SpecToMnemon(sIR_InstrRegister.iInstr_Spec)))
    {  // undo increment step
        FastAdder (sR_ProgramCounter, sR_NegOne, LastLoc);
    }
    else
    {
        FastAdder (sR_ProgramCounter, sR_NegThree, LastLoc);
    }
    cout << "Runtime error at " << cHexTable[LastLoc.iHigh / 16] <<
        cHexTable[LastLoc.iHigh % 16] << cHexTable[LastLoc.iLow / 16] <<
        cHexTable[LastLoc.iLow % 16] << ":  ";
}

void IllegalAddr (bool& bError)
{
    bError = true;
    PrntRunLoc();
    cout << "Illegal addressing mode ";
    switch (eA_AddrMode)
    {
    case eA_IMMEDIATE: 
        cout << "immediate "; 
        break;
    case eA_DIRECT: 
        cout << "direct "; 
        break;
    case eA_INDIRECT: 
        cout << "indirect "; 
        break;
    case eA_STACK_REL: 
        cout << "stack relative "; 
        break;
    case eA_STACK_REL_DEF: 
        cout << "stack relative deferred "; 
        break;
    case eA_INDEXED: 
        cout << "indexed ";
        break;
    case eA_STACK_IND: 
        cout << "stack indexed "; 
        break;
    case eA_STACK_IND_DEF: 
        cout << "stack indexed deferred "; 
        break;
    }
    cout << "with ";
    PrntMnemon (cout);
    cout << endl;
}

void SimSTOP (bool& Halt)
{
    Halt = true;
}

void Pop (sRegisterType& Reg, sRegisterType Size)
{
    MemRead (sR_StackPointer, Reg);   
    FastAdder (sR_StackPointer, Size, sR_StackPointer);
}

void SimRETTR ()
{
    int Flags;
    MemByteRead (sR_StackPointer, Flags);   
    FastAdder (sR_StackPointer, sR_One, sR_StackPointer);
    Flags %= 16;
    bStatusC = Flags % 2 != 0;
    Flags /= 2;
    bStatusV = Flags % 2 != 0;
    Flags /= 2;
    bStatusZ = Flags % 2 != 0;
    Flags /= 2;
    bStatusN = Flags % 2 != 0;

    Pop (sR_Accumulator, sR_Two);
    Pop (sR_IndexRegister, sR_Two);
    Pop (sR_ProgramCounter, sR_Two);
    Pop (sR_StackPointer, sR_AtZero);
}

void SimMOVSPA (bool& Halt)
{
    sR_Accumulator = sR_StackPointer;
}

void SimMOVFLGA(bool& Halt)
{
    sR_Accumulator.iHigh = 0;
    sR_Accumulator.iLow = (bStatusN * 8) + (bStatusZ * 4) + (bStatusV * 2) + bStatusC;
}

void SimBR ()
{
    iAddrMode = GetAddressingModeOneBit (sIR_InstrRegister.iInstr_Spec);
    eA_AddrMode = ProcessAddressingMode(iAddrMode);
   
    LoadReg (sR_ProgramCounter);
}

void SimBRLE ()
{
    iAddrMode = GetAddressingModeOneBit (sIR_InstrRegister.iInstr_Spec);
    eA_AddrMode = ProcessAddressingMode(iAddrMode);
   
    if (bStatusN || bStatusZ)
    {
        LoadReg (sR_ProgramCounter);
    }
}

void SimBRLT ()
{
    iAddrMode = GetAddressingModeOneBit (sIR_InstrRegister.iInstr_Spec);
    eA_AddrMode = ProcessAddressingMode(iAddrMode);
   
    if (bStatusN)
    {
        LoadReg (sR_ProgramCounter);
    }
}

void SimBREQ ()
{
    iAddrMode = GetAddressingModeOneBit (sIR_InstrRegister.iInstr_Spec);
    eA_AddrMode = ProcessAddressingMode(iAddrMode);
   
    if (bStatusZ)
    {
        LoadReg (sR_ProgramCounter);
    }
}

void SimBRNE ()
{
    iAddrMode = GetAddressingModeOneBit (sIR_InstrRegister.iInstr_Spec);
    eA_AddrMode = ProcessAddressingMode(iAddrMode); 
   
    if (!bStatusZ)
    {  
        LoadReg (sR_ProgramCounter);
    }
}

void SimBRGE ()
{
    iAddrMode = GetAddressingModeOneBit (sIR_InstrRegister.iInstr_Spec);
    eA_AddrMode = ProcessAddressingMode(iAddrMode); 
   
    if (!bStatusN)
    {  
        LoadReg (sR_ProgramCounter);
    }
}

void SimBRGT ()
{
    iAddrMode = GetAddressingModeOneBit (sIR_InstrRegister.iInstr_Spec);
    eA_AddrMode = ProcessAddressingMode(iAddrMode);
   
    if (!bStatusN && !bStatusZ)
    { 
        LoadReg (sR_ProgramCounter);
    }
}

void SimBRV ()
{
    iAddrMode = GetAddressingModeOneBit (sIR_InstrRegister.iInstr_Spec);
    eA_AddrMode = ProcessAddressingMode(iAddrMode);  
   
    if (bStatusV)
    { 
        LoadReg (sR_ProgramCounter);
    }
}

void SimBRC ()
{
    iAddrMode = GetAddressingModeOneBit (sIR_InstrRegister.iInstr_Spec);
    eA_AddrMode = ProcessAddressingMode(iAddrMode);
   
    if (bStatusC)
    {   
        LoadReg (sR_ProgramCounter);
    }
}

void SimCALL (bool& bError)
{
    iAddrMode = GetAddressingModeOneBit (sIR_InstrRegister.iInstr_Spec);
    eA_AddrMode = ProcessAddressingMode(iAddrMode);
   
    if ((eA_AddrMode == eA_IMMEDIATE) || (eA_AddrMode == eA_INDEXED))
    {
        FastAdder (sR_StackPointer, sR_NegTwo, sR_StackPointer);
        MemWrite (sR_ProgramCounter, sR_StackPointer);     // Mem [SP] = PC
        LoadReg (sR_ProgramCounter);
    }
    else
    {
        IllegalAddr (bError);
    }
}

void SimNOTr()
{
    iRegType = GetRegisterTypeLastBit (sIR_InstrRegister.iInstr_Spec);
    eR_RegType = ProcessRegisterType(iRegType);
   
    switch (eR_RegType)
    {
    case eR_R_IS_ACCUMULATOR:
        sR_Accumulator.iHigh = 255 - sR_Accumulator.iHigh;
        sR_Accumulator.iLow = 255 - sR_Accumulator.iLow;
        SetNZBits (sR_Accumulator);
        break;
    case eR_R_IS_INDEX_REG:
        sR_IndexRegister.iHigh = 255 - sR_IndexRegister.iHigh;
        sR_IndexRegister.iLow = 255 - sR_IndexRegister.iLow;
        SetNZBits (sR_IndexRegister);
        break;
    }
}

void SimNEGr()
{
    iRegType = GetRegisterTypeLastBit (sIR_InstrRegister.iInstr_Spec);
    eR_RegType = ProcessRegisterType(iRegType);
   
    switch (eR_RegType)
    {
    case eR_R_IS_ACCUMULATOR:
        sR_Accumulator.iHigh = ~sR_Accumulator.iHigh + 256;
        sR_Accumulator.iLow = ~sR_Accumulator.iLow + 257;
        SetNZBits (sR_Accumulator);
        break;
    case eR_R_IS_INDEX_REG:
        sR_IndexRegister.iHigh = ~sR_IndexRegister.iHigh + 256;
        sR_IndexRegister.iLow = ~sR_IndexRegister.iLow + 257;
        SetNZBits (sR_IndexRegister);
        break;
    }
    // v = overflow ?
}

void SimASLr()
{
    iRegType = GetRegisterTypeLastBit (sIR_InstrRegister.iInstr_Spec);
    eR_RegType = ProcessRegisterType(iRegType);
   
    switch (eR_RegType)
    {
    case eR_R_IS_ACCUMULATOR:
        Adder (sR_Accumulator, sR_Accumulator, sR_Accumulator, bStatusC, bStatusV);
        SetNZBits (sR_Accumulator);
        break;
    case eR_R_IS_INDEX_REG:
        Adder (sR_IndexRegister, sR_IndexRegister, sR_IndexRegister, bStatusC, bStatusV);
        SetNZBits (sR_IndexRegister);
        break;
    }
}

void SimASRr()
{
    iRegType = GetRegisterTypeLastBit (sIR_InstrRegister.iInstr_Spec);
    eR_RegType = ProcessRegisterType(iRegType);
   
    int Sign, Carry;
    switch (eR_RegType)
    {
    case eR_R_IS_ACCUMULATOR:
        Sign = sR_Accumulator.iHigh / 128;
        Carry = sR_Accumulator.iHigh % 2;
        bStatusC = (sR_Accumulator.iLow % 2 != 0);
        sR_Accumulator.iHigh = (sR_Accumulator.iHigh / 2) + (Sign * 128);
        sR_Accumulator.iLow = (sR_Accumulator.iLow / 2) + (Carry * 128);
        SetNZBits (sR_Accumulator);
        break;
    case eR_R_IS_INDEX_REG:
        Sign = sR_IndexRegister.iHigh / 128;
        Carry = sR_IndexRegister.iHigh % 2;
        bStatusC = (sR_IndexRegister.iLow % 2 != 0);
        sR_IndexRegister.iHigh = (sR_IndexRegister.iHigh / 2) + (Sign * 128);
        sR_IndexRegister.iLow = (sR_IndexRegister.iLow / 2) + (Carry * 128);
        SetNZBits (sR_IndexRegister);
        break;
    }
}

void SimROLr()
{
    iRegType = GetRegisterTypeLastBit (sIR_InstrRegister.iInstr_Spec);
    eR_RegType = ProcessRegisterType(iRegType);
   
    switch (eR_RegType)
    {
    case eR_R_IS_ACCUMULATOR:
        bStatusC = (sR_Accumulator.iHigh % 128 != 0);
        sR_Accumulator.iHigh = (sR_Accumulator.iHigh * 2) ;
        sR_Accumulator.iLow = (sR_Accumulator.iLow * 2) + bStatusC;
        break;
    case eR_R_IS_INDEX_REG:
        bStatusC = (sR_IndexRegister.iHigh % 128 != 0);
        sR_IndexRegister.iHigh = (sR_IndexRegister.iHigh * 2);
        sR_IndexRegister.iLow = (sR_IndexRegister.iLow * 2) + bStatusC;
        break;
    }
}

void SimRORr()
{
    iRegType = GetRegisterTypeLastBit (sIR_InstrRegister.iInstr_Spec);
    eR_RegType = ProcessRegisterType(iRegType);
   
    switch (eR_RegType)
    {
    case eR_R_IS_ACCUMULATOR:
        bStatusC = (sR_Accumulator.iLow % 2 != 0);
        sR_Accumulator.iHigh = (sR_Accumulator.iHigh / 2) + (bStatusC * 128);
        sR_Accumulator.iLow = (sR_Accumulator.iLow / 2);
        break;
    case eR_R_IS_INDEX_REG:
        bStatusC = (sR_IndexRegister.iLow % 2 != 0);
        sR_IndexRegister.iHigh = (sR_IndexRegister.iHigh / 2) + (bStatusC * 128);
        sR_IndexRegister.iLow = (sR_IndexRegister.iLow / 2);
        break;
    }
}

void SimCHARI (bool& bError)
{
    char Ch;
    sRegisterType Operand;
   
    iAddrMode = GetAddressingModeThreeBits (sIR_InstrRegister.iInstr_Spec);
    eA_AddrMode = ProcessAddressingMode(iAddrMode);
  
    if (bBufferIsEmpty)
    {
        if (bLoading || !bKeyboardInput)
        {
            vGetLine(chariInputStream);
            if (chariInputStream.eof())
            {
                bError = true;
                PrntRunLoc();
                cout << "File read error or read past end of file." << endl;
                return;
            }
        }
        else
        {
            vGetLine(cin);
        }
    }
    vAdvanceInput(Ch);

    if (eA_AddrMode == eA_IMMEDIATE)
    {
        IllegalAddr (bError);
    }
    else  //Addressing mode is valid
    {
        AddrProcessor (Operand);
        MemByteWrite (Ch, Operand);
    }
}

void SimCHARO()
{
    sRegisterType Operand;
    int iData;
   
    iAddrMode = GetAddressingModeThreeBits (sIR_InstrRegister.iInstr_Spec);
    eA_AddrMode = ProcessAddressingMode(iAddrMode);
    AddrProcessor (Operand);
    if (eA_AddrMode == eA_IMMEDIATE)
    {
        iData = Operand.iLow;
    }
    else
    {
        MemByteRead (Operand, iData);
    }
    if (!bScreenOutput)
    {
        if (iData == LINE_FEED || iData == CARRIAGE_RETURN)
        {
            charoOutputStream << endl;
        }
        else
        {
            charoOutputStream << static_cast <char> (iData);
        }
    }
    else
    {
        if (iData == LINE_FEED || iData == CARRIAGE_RETURN)
        {
            cout << endl;
        }
        else
        {
            cout << static_cast <char> (iData);
        }
    }
    if (eTraceMode != eT_TR_OFF && bScreenOutput)
    {
        cout << endl;
    }
}

void SimRETn()
{
    sRegisterType sR_N;
   
    sR_N.iHigh = 0;
    sR_N.iLow = GetNValueThreeBits (sIR_InstrRegister.iInstr_Spec);
   
    FastAdder (sR_StackPointer, sR_N, sR_StackPointer);   // SP = SP + n
    MemRead (sR_StackPointer, sR_ProgramCounter);         // PC = Mem [SP]
    FastAdder (sR_StackPointer, sR_Two, sR_StackPointer); // SP = SP + 2
}

void SimADDSP (bool& bError)
{
    sRegisterType R0;
   
    iAddrMode = GetAddressingModeThreeBits (sIR_InstrRegister.iInstr_Spec);
    eA_AddrMode = ProcessAddressingMode(iAddrMode);
   
    LoadReg (R0);
    Adder (sR_StackPointer, R0, sR_StackPointer, bStatusC, bStatusV);
    SetNZBits (sR_StackPointer);
}

void SimSUBSP (bool& bError)
{
    sRegisterType R0;
   
    iAddrMode = GetAddressingModeThreeBits (sIR_InstrRegister.iInstr_Spec);
    eA_AddrMode = ProcessAddressingMode(iAddrMode);
   
    LoadReg (R0);
    Subtractor (sR_StackPointer, R0, sR_StackPointer, bStatusC, bStatusV);
    SetNZBits (sR_StackPointer);
}

void SimADDr()
{
    sRegisterType R0;
   
    iAddrMode = GetAddressingModeThreeBits (sIR_InstrRegister.iInstr_Spec);
    iRegType = GetRegisterTypeFourthBit (sIR_InstrRegister.iInstr_Spec);
    eA_AddrMode = ProcessAddressingMode(iAddrMode);
    eR_RegType = ProcessRegisterType(iRegType);
         
    LoadReg (R0);
        
    switch (eR_RegType)
    {
    case eR_R_IS_ACCUMULATOR:
        Adder (sR_Accumulator, R0, sR_Accumulator, bStatusC, bStatusV);
        SetNZBits (sR_Accumulator);
        break;
    case eR_R_IS_INDEX_REG:
        Adder (sR_IndexRegister, R0, sR_IndexRegister, bStatusC, bStatusV);
        SetNZBits (sR_IndexRegister);
        break;
    }
}

void SimSUBr()
{
    sRegisterType R0;
   
    iAddrMode = GetAddressingModeThreeBits (sIR_InstrRegister.iInstr_Spec);
    iRegType = GetRegisterTypeFourthBit (sIR_InstrRegister.iInstr_Spec);
    eA_AddrMode = ProcessAddressingMode(iAddrMode);
    eR_RegType = ProcessRegisterType(iRegType);
         
    LoadReg (R0);
    switch (eR_RegType)
    {
    case eR_R_IS_ACCUMULATOR:
        Subtractor (sR_Accumulator, R0, sR_Accumulator, bStatusC, bStatusV);
        SetNZBits (sR_Accumulator);
        break;
    case eR_R_IS_INDEX_REG:
        Subtractor (sR_IndexRegister, R0, sR_IndexRegister, bStatusC, bStatusV);
        SetNZBits (sR_IndexRegister);
        break;
    }
}

void ANDReg (sRegisterType Op1, sRegisterType Op2, sRegisterType& Result)
{
    int iPwr;
    iPwr = 1;
    Result.iHigh = 0;
    Result.iLow = 0;
    while (((Op1.iHigh != 0) && (Op2.iHigh != 0)) ||         // while more to do
           ((Op1.iLow != 0) && (Op2.iLow != 0)))
    {
        if ((Op1.iHigh % 2 != 0) && (Op2.iHigh % 2 != 0))
        {
            Result.iHigh = Result.iHigh + iPwr;
        }
        if ((Op1.iLow % 2 != 0) && (Op2.iLow % 2 != 0))
        {
            Result.iLow = Result.iLow + iPwr;
        }
        Op1.iHigh = Op1.iHigh / 2;
        Op1.iLow = Op1.iLow / 2;
        Op2.iHigh = Op2.iHigh / 2;
        Op2.iLow = Op2.iLow / 2;
        iPwr = iPwr + iPwr;
    }
}

void SimANDr()
{
    sRegisterType R0;
   
    iAddrMode = GetAddressingModeThreeBits (sIR_InstrRegister.iInstr_Spec);
    iRegType = GetRegisterTypeFourthBit (sIR_InstrRegister.iInstr_Spec);
    eA_AddrMode = ProcessAddressingMode(iAddrMode);
    eR_RegType = ProcessRegisterType(iRegType);
   
    LoadReg (R0);
    switch (eR_RegType)
    {
    case eR_R_IS_ACCUMULATOR:
        ANDReg (sR_Accumulator, R0, sR_Accumulator);
        SetNZBits (sR_Accumulator);
        break;
    case eR_R_IS_INDEX_REG:
        ANDReg (sR_IndexRegister, R0, sR_IndexRegister);
        SetNZBits (sR_IndexRegister);
        break;
    }
}

void ORReg (sRegisterType Op1, sRegisterType Op2, sRegisterType& Result)
{
    int Pwr;
    Pwr = 1;
    Result.iHigh = 0;
    Result.iLow = 0;
    while (((Op1.iHigh != 0) || (Op2.iHigh != 0)) ||          // while more to do
           ((Op1.iLow != 0) || (Op2.iLow != 0)))
    {
        if ((Op1.iHigh % 2 != 0) || (Op2.iHigh % 2 != 0))
        {
            Result.iHigh = Result.iHigh + Pwr;
        }
        if ((Op1.iLow % 2 != 0) || (Op2.iLow % 2 != 0))
        {
            Result.iLow = Result.iLow + Pwr;
        }
        Op1.iHigh = Op1.iHigh / 2;
        Op1.iLow = Op1.iLow / 2;
        Op2.iHigh = Op2.iHigh / 2;
        Op2.iLow = Op2.iLow / 2;
        Pwr = Pwr + Pwr;
    }
}

void SimORr()
{  
    sRegisterType R0;
   
    iAddrMode = GetAddressingModeThreeBits (sIR_InstrRegister.iInstr_Spec);
    iRegType = GetRegisterTypeFourthBit (sIR_InstrRegister.iInstr_Spec);
    eA_AddrMode = ProcessAddressingMode(iAddrMode);
    eR_RegType = ProcessRegisterType(iRegType);
   
    LoadReg (R0);
    switch (eR_RegType)
    {
    case eR_R_IS_ACCUMULATOR:
        ORReg (sR_Accumulator, R0, sR_Accumulator);
        SetNZBits (sR_Accumulator);
        break;
    case eR_R_IS_INDEX_REG:
        ORReg (sR_IndexRegister, R0, sR_IndexRegister);
        SetNZBits (sR_IndexRegister);
        break;
    }
}

void SimCPr()
{
    sRegisterType R0, R1, R2;
   
    iAddrMode = GetAddressingModeThreeBits (sIR_InstrRegister.iInstr_Spec);
    iRegType = GetRegisterTypeFourthBit (sIR_InstrRegister.iInstr_Spec);
    eA_AddrMode = ProcessAddressingMode(iAddrMode);
    eR_RegType = ProcessRegisterType(iRegType);
   
    switch (eR_RegType)
    {
    case eR_R_IS_ACCUMULATOR:
        R0 = sR_Accumulator; break;
    case eR_R_IS_INDEX_REG:
        R0 = sR_IndexRegister; break;
    }
    LoadReg (R1);
    Subtractor (R0, R1, R2, bStatusC, bStatusV);
    if ((R0.iHigh <= 127) && (R1.iHigh >= 128)) //Pos minus Neg
    {
        bStatusN = false;
        bStatusZ = false;
    }
    else if ((R0.iHigh >= 128) && (R1.iHigh <= 127)) //Neg minus Pos
    {
        bStatusN = true;
        bStatusZ = false;
    }
    else
    {
        SetNZBits (R2);
    }
}

void SimLDr()
{
    iAddrMode = GetAddressingModeThreeBits (sIR_InstrRegister.iInstr_Spec);
    iRegType = GetRegisterTypeFourthBit (sIR_InstrRegister.iInstr_Spec);
    eA_AddrMode = ProcessAddressingMode(iAddrMode);
    eR_RegType = ProcessRegisterType(iRegType);
   
    switch (eR_RegType)
    {
    case eR_R_IS_ACCUMULATOR:
        LoadReg (sR_Accumulator);
        SetNZBits (sR_Accumulator);
        break;
    case eR_R_IS_INDEX_REG:
        LoadReg (sR_IndexRegister);
        SetNZBits (sR_IndexRegister);
        break;
    }
}

void SimLDBYTEr()
{

    int Temp;
    sRegisterType Operand;
   
    iAddrMode = GetAddressingModeThreeBits (sIR_InstrRegister.iInstr_Spec);
    iRegType = GetRegisterTypeFourthBit (sIR_InstrRegister.iInstr_Spec);
    eA_AddrMode = ProcessAddressingMode(iAddrMode);
    eR_RegType = ProcessRegisterType(iRegType);
         
    AddrProcessor (Operand);
    if (eA_AddrMode == eA_IMMEDIATE)
    {
        Temp = Operand.iLow;
    }
    else
    {
        MemByteRead (Operand, Temp);
    }
    switch (eR_RegType)
    {
    case eR_R_IS_ACCUMULATOR:
        sR_Accumulator.iLow = Temp;
        SetNZBits (sR_Accumulator);
        break;
    case eR_R_IS_INDEX_REG:
        sR_IndexRegister.iLow = Temp;
        SetNZBits (sR_IndexRegister);
        break;
    }
}

void SimSTr (bool& bError)
{
    sRegisterType Operand;
   
    iAddrMode = GetAddressingModeThreeBits (sIR_InstrRegister.iInstr_Spec);
    iRegType = GetRegisterTypeFourthBit (sIR_InstrRegister.iInstr_Spec);
    eA_AddrMode = ProcessAddressingMode(iAddrMode);
    eR_RegType = ProcessRegisterType(iRegType);
   
    if (eA_AddrMode == eA_IMMEDIATE)
    {
        IllegalAddr (bError);
    }
    else  // addressing mode is valid
    {
        AddrProcessor (Operand);
        switch (eR_RegType)
        {
        case eR_R_IS_ACCUMULATOR:
            MemWrite (sR_Accumulator, Operand);
            break;
        case eR_R_IS_INDEX_REG:
            MemWrite (sR_IndexRegister, Operand);
            break;
        }
    }
}

void SimSTBYTEr (bool& bError)
{
    sRegisterType Operand;
   
    iAddrMode = GetAddressingModeThreeBits (sIR_InstrRegister.iInstr_Spec);
    iRegType = GetRegisterTypeFourthBit (sIR_InstrRegister.iInstr_Spec);
    eA_AddrMode = ProcessAddressingMode(iAddrMode);
    eR_RegType = ProcessRegisterType(iRegType);
            
    if (eA_AddrMode == eA_IMMEDIATE)
    {
        IllegalAddr (bError);
    }
    else  // addressing mode is valid
    {
        AddrProcessor (Operand);
        switch (eR_RegType)
        {
        case eR_R_IS_ACCUMULATOR:
            MemByteWrite (sR_Accumulator.iLow, Operand); break;
        case eR_R_IS_INDEX_REG:
            MemByteWrite (sR_IndexRegister.iLow, Operand); break;
        }
    }
}

void Push (sRegisterType Reg, sRegisterType Size)
{
    FastAdder (sR_StackPointer, Size, sR_StackPointer);
    MemWrite (Reg, sR_StackPointer);
}

void SimTRAP(int trapNumber)
{
    if (4 <= trapNumber)
    {
        iAddrMode = GetAddressingModeThreeBits (sIR_InstrRegister.iInstr_Spec);
        eA_AddrMode = ProcessAddressingMode(iAddrMode);
    }
    sRegisterType R0;
    sRegisterType oldSP;
   
    oldSP = sR_StackPointer;                        // Save initial SP value to push later
    sR_StackPointer.iHigh = iMemory[SYSTEM_SP];     // Get system SP value
    sR_StackPointer.iLow  = iMemory[SYSTEM_SP + 1]; // Transfer instr spec to R0
     
    FastAdder (sR_StackPointer, sR_NegOne, sR_StackPointer);
    MemByteWrite (sIR_InstrRegister.iInstr_Spec, sR_StackPointer);      // Push instruction specifier
    Push (oldSP, sR_NegTwo);
    Push (sR_ProgramCounter, sR_NegTwo);
    Push (sR_IndexRegister, sR_NegTwo);
    Push (sR_Accumulator, sR_NegTwo);

    R0.iLow = (bStatusN * 8) + (bStatusZ * 4) + (bStatusV * 2) + bStatusC;
    FastAdder (sR_StackPointer, sR_NegOne, sR_StackPointer);
    MemByteWrite (R0.iLow, sR_StackPointer);               // Push status flags
    sR_ProgramCounter.iHigh = iMemory[INTR_PC];            // Branch to Pep/8 OS
    sR_ProgramCounter.iLow = iMemory[INTR_PC + 1];
}

//**** End of Opcode procedures ****

void Initialize (bool& bError)
{
    char ch;
    trapFile.open("trap");
    if (!trapFile.is_open()) {
        bError = true;
        cout << "Could not open trap file." << endl;
    } 
    else {
        bError = false;
        for (int iLine = 0; iLine < UNIMPLEMENTED_INSTRUCTIONS; iLine++)
        {
            vGetLine(trapFile);
            vAdvanceInput(ch);
            for (int i = 0; i < MNEMON_LENGTH; i++)
            {
                if (isspace(ch)) 
                {
                    TrapMnemon[iLine][i] = ' ';
                }
                else
                {
                    TrapMnemon[iLine][i] = toupper(ch);
                    vAdvanceInput(ch);
                }
            }
            TrapMnemon[iLine][MNEMON_LENGTH] = '\0';
        }
        eTraceMode = eT_TR_OFF;
        bLoading = false;
        bMachineReset = false;
        bKeyboardInput = true;
        bScreenOutput = true;
        bSingleStep = false;
        bScrollingTrace = false;
        cHexTable[0] = '0';
        cHexTable[1] = '1';
        cHexTable[2] = '2';
        cHexTable[3] = '3';
        cHexTable[4] = '4';
        cHexTable[5] = '5';
        cHexTable[6] = '6';
        cHexTable[7] = '7';
        cHexTable[8] = '8';
        cHexTable[9] = '9';
        cHexTable[10] = 'A';
        cHexTable[11] = 'B';
        cHexTable[12] = 'C';
        cHexTable[13] = 'D';
        cHexTable[14] = 'E';
        cHexTable[15] = 'F';
        iUnaryOpcodes[0] = eM_STOP;
        iUnaryOpcodes[1] = eM_RETTR;
        iUnaryOpcodes[2] = eM_MOVSPA;
        iUnaryOpcodes[3] = eM_MOVFLGA;
        iUnaryOpcodes[4] = eM_NOTr;
        iUnaryOpcodes[5] = eM_NEGr;
        iUnaryOpcodes[6] = eM_ASLr;
        iUnaryOpcodes[7] = eM_ASRr;
        iUnaryOpcodes[8] = eM_ROLr;
        iUnaryOpcodes[9] = eM_RORr;
        iUnaryOpcodes[10] = eM_UNIMP0;
        iUnaryOpcodes[11] = eM_UNIMP1;
        iUnaryOpcodes[12] = eM_UNIMP2;
        iUnaryOpcodes[13] = eM_UNIMP3;
        iUnaryOpcodes[14] = eM_RETn;
        ByteInstructions[0] = eM_LDBYTEr;
        ByteInstructions[1] = eM_STBYTEr;
        ByteInstructions[2] = eM_CHARI;
        ByteInstructions[3] = eM_CHARO;
        sR_AtZero.iHigh = 0;
        sR_AtZero.iLow = 0;
        sR_One.iHigh = 0;
        sR_One.iLow = 1;
        sR_Two.iHigh = 0;
        sR_Two.iLow = 2;
        sR_NegOne.iHigh = 255;
        sR_NegOne.iLow = 255;
        sR_NegTwo.iHigh = 255;
        sR_NegTwo.iLow = 254;
        sR_NegThree.iHigh = 255;
        sR_NegThree.iLow = 253;
        sR_Accumulator = sR_AtZero;             // Must be initialized if trace used
        sR_IndexRegister = sR_AtZero;
        numTerminalLines = 22;
        trapFile.close();
        trapFile.clear();
    }
}

//**** Converts a HEX number to a decimal number and returns the decimal number.
int iHexToDec (char ch)
{
    switch (ch)
    {
    case '0':  return 0; break;
    case '1':  return 1; break;
    case '2':  return 2; break;
    case '3':  return 3; break;
    case '4':  return 4; break;
    case '5':  return 5; break;
    case '6':  return 6; break;
    case '7':  return 7; break;
    case '8':  return 8; break;
    case '9':  return 9; break;
    case 'A':  return 10; break;
    case 'B':  return 11; break;
    case 'C':  return 12; break;
    case 'D':  return 13; break;
    case 'E':  return 14; break;
    case 'F':  return 15; break;
    default: return -1;
    }
}

//**** Converts a decimal value between -256 to 255 to a HEX array of characters
//**** Used to convert opcodes to hex
void vDecToHexByte (int iDec, char cHex[HEX_BYTE_LENGTH + 1])
{
    cHex[0] = cHexTable[iDec / 16];
    cHex[1] = cHexTable[iDec % 16];
    cHex[2] = '\0';
}

//**** Converts a HEX byte to a positive decimal integer
int iHexByteToDecInt (char cHex[HEX_BYTE_LENGTH + 1])
{
    return 16 * iHexToDec(cHex[0]) + iHexToDec(cHex[1]);
}

//**** Converts a 16 bit register into a 4 digit HEX no.
void RegToHex (sRegisterType Reg, char HexNum[])
{
    if (Reg.iHigh < 0 || Reg.iHigh > 255)
    {
        cout << "\nSimulator error: Reg.iHigh out of bounds: " << Reg.iHigh << endl;
    }
    else if ( Reg.iLow < 0 || Reg.iLow > 255)
    {
        cout << "\nSimulator error: Reg.iLow out of bounds: " << Reg.iLow << endl;
    }
    HexNum[0] = cHexTable[Reg.iHigh / 16];
    HexNum[1] = cHexTable[Reg.iHigh % 16];
    HexNum[2] = cHexTable[Reg.iLow / 16];
    HexNum[3] = cHexTable[Reg.iLow % 16];
    HexNum[4] = '\0';
}

//**** Initialize RAM using data from pep8os.pepo file
void InstallRom (bool& bError)
{
    ifstream ROMFile;
    int iNumBytes = 0;
    char cByte[HEX_BYTE_LENGTH + 1];
    int iCounter = 0;
    char cNext;
    ROMFile.open("pep8os.pepo");
    if (ROMFile.fail())
    {
        bError = true;
        cout << "Could not open file pep8os.pepo" << endl;
    }
    else
    {
        iCounter = 0;
        int i;
        vGetLine(ROMFile);
        vAdvanceInput(cNext);
        while (!ROMFile.eof())
        {
            if (bIsHexDigit(cNext))
            {
                iCounter++;
            }
            else if (cNext == '\n')
            {
                vGetLine(ROMFile);
            }
            vAdvanceInput(cNext);
        }
        iNumBytes = iCounter / 2;
        ROMFile.close();
        ROMFile.clear();
        if (iNumBytes >= MEMORY_SIZE)
        {
            bError = true;
            cout << "OS is too big to fit into main memory." << endl;
            cout << "NumBytes = " << iNumBytes;
            cout << ", MemorySize = " << MEMORY_SIZE << endl;
        }
        else
        {
            ROMFile.open ("pep8os.pepo");
            iRomStartAddr = TOP_OF_MEMORY - iNumBytes + 1;
            bool bIsEnd = false;
            vGetLine(ROMFile);
            vAdvanceInput(cNext);
            iCounter = 0;
            i = iRomStartAddr;
            while (!ROMFile.eof() && !bIsEnd)
            {
                if (iCounter == 2)
                {
                    cByte[iCounter] = '\0';
                    iMemory[i++] = iHexByteToDecInt(cByte);
                    iCounter = 0;
                    vBackUpInput();
                }
                else if (bIsHexDigit(cNext))
                {
                    cByte[iCounter++] = cNext;
                }
                else if (cNext == '\n')
                {
                    vGetLine(ROMFile);
                }
                else if (cNext == 'z')
                {
                    bIsEnd = true;
                }
                else if (cNext != ' ')
                {
                    cout << "Invalid input in pep8os.pepo" << endl;
                    return;
                }
                vAdvanceInput(cNext);
            }
            if (bIsEnd)
            {
                if (cNext != 'z')
                {
                    cout << "File must end in 'zz'" << endl;
                }
            }
            ROMFile.close();
            ROMFile.clear();
            cout << iRomStartAddr << " bytes RAM free." << endl;
        }
    }
}

void PrintLine (ostream& output)
{
    output << "--------------------------------------------------";
    output << "-----------------------" << endl;
}

void PrintHeading (ostream& output)
{
    PrintLine (output);
    output << "               Oprnd     Instr           Index   Stack   Status" << endl;
    output << "Addr  Mnemon   Spec       Reg     Accum   Reg   Pointer  N Z V C  Operand" << endl;
    PrintLine (output);
}

void PrintTraceLine (ostream& output, sRegisterType Address)
{
    char cHexByte[HEX_BYTE_LENGTH + 1];
    char cHexWord[HEX_WORD_LENGTH + 1];
    sRegisterType R0;
    RegToHex (Address, cHexWord);
    output << cHexWord << "  "; // Print address
    PrntMnemon (output);                // Print mnemonic
    if (bSearchIntArray(iUnaryOpcodes, UNARY_OPCODES, instr_SpecToMnemon(sIR_InstrRegister.iInstr_Spec)))
    {
        output << "                   ";
    }
    else
    {
        RegToHex (sIR_InstrRegister.sR_OprndSpec, cHexWord);
        output << cHexWord << ",";
        switch (eA_AddrMode)
        {         
        case eA_IMMEDIATE: output << "i    "; break;
        case eA_DIRECT: output << "d    "; break;
        case eA_INDIRECT: output << "n    "; break;
        case eA_STACK_REL: output << "s    "; break;
        case eA_STACK_REL_DEF: output << "sf   "; break;
        case eA_INDEXED: output << "x    "; break;
        case eA_STACK_IND: output << "sx   "; break;
        case eA_STACK_IND_DEF: output << "sxf  "; break;
        }
      
        vDecToHexByte (sIR_InstrRegister.iInstr_Spec, cHexByte);
        output << cHexByte;
        RegToHex (sIR_InstrRegister.sR_OprndSpec, cHexWord);
        output << cHexWord << "   ";    // Print instruction reg
    }
    RegToHex (sR_Accumulator, cHexWord);
    output << cHexWord << "   ";                // Print accumulator
    RegToHex (sR_IndexRegister, cHexWord);
    output << cHexWord << "    ";               // Print index register
    RegToHex (sR_StackPointer, cHexWord);
    output << cHexWord << "    ";               // Print stack pointer
    output << bStatusN << " " << bStatusZ << " " << bStatusV << " " << bStatusC << "   ";  // Print status bit
    if (bSearchIntArray(iUnaryOpcodes, UNARY_OPCODES, instr_SpecToMnemon(sIR_InstrRegister.iInstr_Spec)))
    {
        for (int i = 0; i < HEX_WORD_LENGTH; i++)
        {
            cHexWord[i] = '0';
        }
    }
    else
    {
        LoadReg (R0);   // calculate operand
        RegToHex (R0, cHexWord);
    }
    output << cHexWord; // Print iMemory [Oprnd Spec]
}

char GetTracePrompt ()
{
    char cResponse[LINE_LENGTH];
    char ch;
    do
    {
        cin.getline(cResponse, LINE_LENGTH);
        ch = toupper(cResponse[0]);
        if (ch != 'N' && ch != 'C' && ch != 'S' && ch != 'Q' && ch != ' ')
        {
            cout << "Invalid response" << endl;
            cout << "(n)ext page  s(c)roll  (s)ingle step  (q)uit trace: ";
        }
    }
    while (ch != 'N' && ch != 'C' && ch != 'S' && ch != 'Q' && ch != ' ');
    return ch;
}

void Trace (sRegisterType Address, int& LineCount, bool& Halt)
{
    int iTempAddr;
    char ch;

    if (Address.iHigh < MEMORY_SIZE / 256)
    {
        iTempAddr = Address.iHigh * 256 + Address.iLow;
    }
    else
    {
        iTempAddr = TOP_OF_MEMORY;
    }
    if (iTempAddr < iRomStartAddr
        || (iTempAddr >= iRomStartAddr && eTraceMode == eT_TR_TRAPS)
        || eTraceMode == eT_TR_LOADER)
    {
        PrintTraceLine (cout, Address);
        if (bScrollingTrace)
        {
            cout << endl;
        }
        else if (bSingleStep)
        {
            cout << ": ";
            ch = GetTracePrompt ();
            switch (ch)
            {
            case 'N':
                bSingleStep = false;
                cout << endl;
                PrintHeading (cout);
                LineCount = 4;
                break;
            case 'C':
                bSingleStep = false;
                bScrollingTrace = true;
                cout << endl;
                PrintHeading (cout);
                break;
            case 'Q':
                bSingleStep = false;
                bScrollingTrace = false;
                cout << endl;
                PrintLine (cout);
                Halt = true;
                break;
            default:
                break;
            }
        }
        else  // Not single stepping or scrolling trace to completion
        {
            cout << endl;
            LineCount++;
            if (LineCount >= numTerminalLines)
            {
                cout << "(n)ext page  s(c)roll  (s)ingle step  (q)uit trace: ";
                ch = GetTracePrompt ();
                switch (ch)
                {
                case 'N':
                    cout << endl;
                    PrintHeading (cout);
                    LineCount = 4;
                    break;
                case 'C':
                    bScrollingTrace = true;
                    cout << endl;
                    PrintHeading (cout);
                    break;
                case 'S':
                    bSingleStep = true;
                    break;
                case 'Q':
                    bSingleStep = false;
                    bScrollingTrace = false;
                    cout << endl;
                    PrintLine (cout);
                    Halt = true;
                    break;
                default:
                    break;
                }
            }
        }
    }
}

//****  The von Neumann execution cycle
void FetchIncrPC()
{
    //**** Fetch instruction spec.
    MemByteRead (sR_ProgramCounter, sIR_InstrRegister.iInstr_Spec);  // R0.iHigh = Instruction spec.
    //**** Increment ProgramCounter by one
    if (sR_ProgramCounter.iLow == 255)
    {
        sR_ProgramCounter.iLow = 0;
        if (sR_ProgramCounter.iHigh == 255)
        {
            sR_ProgramCounter.iHigh = 0;
        }
        else
        {
            sR_ProgramCounter.iHigh++;
        }
    }
    else
    {
        sR_ProgramCounter.iLow = sR_ProgramCounter.iLow + 1;
    }
    if (!bSearchIntArray(iUnaryOpcodes, UNARY_OPCODES, instr_SpecToMnemon(sIR_InstrRegister.iInstr_Spec)))
    {
        MemRead (sR_ProgramCounter, sIR_InstrRegister.sR_OprndSpec);
        //**** Increment sR_ProgramCounter by sR_Two
        if (sR_ProgramCounter.iLow == 254)
        {
            sR_ProgramCounter.iLow = 0;
            if (sR_ProgramCounter.iHigh == 255)
            {
                sR_ProgramCounter.iHigh = 0;
            }
            else
            {
                sR_ProgramCounter.iHigh++;
            }
        }
        else if (sR_ProgramCounter.iLow == 255)
        {
            sR_ProgramCounter.iLow = 1;
            if (sR_ProgramCounter.iHigh == 255)
            {
                sR_ProgramCounter.iHigh = 0;
            }
            else
            {
                sR_ProgramCounter.iHigh++;
            }
        }
        else
        {
            sR_ProgramCounter.iLow = sR_ProgramCounter.iLow + 2;
        }
    }
}

void Execute (bool& bHalt)
{
    switch (instr_SpecToMnemon(sIR_InstrRegister.iInstr_Spec)) {
    case eM_STOP: SimSTOP(bHalt); break;
    case eM_RETTR: SimRETTR(); break;
    case eM_MOVSPA: SimMOVSPA (bHalt); break;
    case eM_MOVFLGA: SimMOVFLGA (bHalt); break;

    case eM_BR: SimBR(); break;                                                 
    case eM_BRLE: SimBRLE(); break;                                 
    case eM_BRLT: SimBRLT(); break;                                 
    case eM_BREQ: SimBREQ(); break;                                                 
    case eM_BRNE: SimBRNE(); break;                                                 
    case eM_BRGE: SimBRGE(); break;                                                 
    case eM_BRGT: SimBRGT(); break;                                                 
    case eM_BRV: SimBRV(); break;                                                 
    case eM_BRC: SimBRC(); break; 
    case eM_CALL: SimCALL(bHalt); break;

    case eM_NOTr: SimNOTr(); break;
    case eM_NEGr: SimNEGr(); break;
    case eM_ASLr: SimASLr(); break;
    case eM_ASRr: SimASRr(); break;
    case eM_ROLr: SimROLr(); break;
    case eM_RORr: SimRORr(); break;
         
    case eM_UNIMP0: SimTRAP(0); break;
    case eM_UNIMP1: SimTRAP(1); break;
    case eM_UNIMP2: SimTRAP(2); break;
    case eM_UNIMP3: SimTRAP(3); break;
    case eM_UNIMP4: SimTRAP(4); break;
    case eM_UNIMP5: SimTRAP(5); break;
    case eM_UNIMP6: SimTRAP(6); break;
    case eM_UNIMP7: SimTRAP(7); break;

    case eM_CHARI: SimCHARI(bHalt); break;
    case eM_CHARO: SimCHARO(); break;

    case eM_RETn: SimRETn(); break;
           
    case eM_ADDSP: SimADDSP(bHalt); break;
    case eM_SUBSP: SimSUBSP(bHalt); break;
                 
    case eM_ADDr: SimADDr(); break;
    case eM_SUBr: SimSUBr(); break;
                                                
    case eM_ANDr: SimANDr(); break;
    case eM_ORr: SimORr(); break;
    case eM_CPr: SimCPr(); break;

    case eM_LDr: SimLDr(); break;
    case eM_LDBYTEr: SimLDBYTEr(); break;
    case eM_STr: SimSTr(bHalt); break;
    case eM_STBYTEr: SimSTBYTEr(bHalt); break;
    }
}

void StartExecution ()
{
    bool Halt;
    sRegisterType TraceAddr;
    int iLineCount;
    char cResponse[LINE_LENGTH];
    if (!bMachineReset && !bLoading)
    {
        cout << "Execution error: Machine state not initialized." << endl;
        cout << "Use (l)oad command." << endl;
    }
    else
    {
        if (eTraceMode != eT_TR_OFF && !bSingleStep)
        {
            switch (eTraceMode)
            {
            case eT_TR_PROGRAM : cout << "User Program Trace:" << endl; break;
            case eT_TR_TRAPS : cout << "User Program Trace with Traps:" << endl; break;
            case eT_TR_LOADER : cout << "Loader Trace of Operating System:" << endl; break;
            case eT_TR_OFF : break;
            }
            cout << endl;
            PrintHeading (cout);
            iLineCount = 6;
        }
        //**** The von Neumann execution cycle
        Halt = false;
        do
        {
            TraceAddr = sR_ProgramCounter;
            FetchIncrPC();
            Execute (Halt);
            if (eTraceMode != eT_TR_OFF)
            {
                Trace (TraceAddr, iLineCount, Halt);
            }
        }
        while (!Halt);
        if (eTraceMode != eT_TR_OFF)
        {
            PrintLine (cout);
        }
        if (!bKeyboardInput)
        {
            chariInputStream.seekg (0, ios::beg);  // Reset input file to its beginning
        }
    }
}

void LoaderCommand()
{
    char FileName[FILE_NAME_LENGTH];
   
    if (!bKeyboardInput)
    {
        cout << "Data input switched back to keyboard." << endl;
        bKeyboardInput = true;
        chariInputStream.close();
        chariInputStream.clear();
    }
    cout << "Enter object file name (do not include .pepo): ";
    cin.getline(FileName, FILE_NAME_LENGTH);
    int iTemp = cin.gcount() - 1;
    FileName[iTemp++] = '.';
    FileName[iTemp++] = 'p';
    FileName[iTemp++] = 'e';
    FileName[iTemp++] = 'p';
    FileName[iTemp++] = 'o';
    FileName[iTemp] = '\0';
    chariInputStream.open(FileName);
    if (chariInputStream.is_open())
    {
        cout << "Object file is " << FileName << endl;
        bMachineReset = true;
        bBufferIsEmpty = true;
        bLoading = true;
        sR_StackPointer.iHigh = iMemory[SYSTEM_SP];
        sR_StackPointer.iLow = iMemory[SYSTEM_SP + 1];
        sR_ProgramCounter.iHigh = iMemory[LOADER_PC];
        sR_ProgramCounter.iLow = iMemory[LOADER_PC + 1];
        StartExecution ();
        bLoading = false;
    }
    else
    {
        cout << "Could not open object file " << FileName << endl;
    }
    chariInputStream.close();
    chariInputStream.clear();
}

void ExecuteCommand()
{
    bBufferIsEmpty = true;
    sR_StackPointer.iHigh = iMemory[USER_SP];
    sR_StackPointer.iLow = iMemory[USER_SP + 1];
    sR_ProgramCounter.iHigh = 0;
    sR_ProgramCounter.iLow = 0;
    StartExecution ();
}

void DecodeAddress (char Digits[HEX_BYTE_LENGTH + 1], int& Value)
{
    int i,j;
    Value = 0;
    for (i = 0; i < 2; i++)
    {
        if (Digits[i] > '9')
        {
            j =  Digits[i] - 'A' + 10;
        }
        else
        {
            j = Digits[i] - '0';
        }
        Value = 16 * Value + j;
    }
    Digits[i] = '\0';
}

//**** Input starting and ending addresses for DUMP cCommand
void Parse (sRegisterType& StartAddress, sRegisterType& EndAddress)
{
    char Hex[HEX_WORD_LENGTH + 1][HEX_BYTE_LENGTH + 1];
    char c;
    int i,j;
    bool NoError;
    do
    {
        NoError = true;
        cout << endl;
        cout << "Enter address range of dump (HEX)" << endl;
        cout << "Example, 0020-0140: ";
        vGetLine(cin);
        for (i = 0; i < 2; i++)
        {
            for (j = 0; j < 2; j++)
            {
                vAdvanceInput(c);
                Hex[i][j] = c;
            }
            Hex[i][j] = '\0';
        }
        vAdvanceInput(c);
        for (i = 2; i < 4; i++)
        {
            for (j = 0;j < 2; j++)
            {
                vAdvanceInput(c);
                Hex[i][j] = c;
            }
            Hex[i][j] = '\0';
        }
        for (i = 0; i < 4; i++)
        {
            for (j = 0; j < 2; j++)
            {
                Hex[i][j] = toupper(Hex[i][j]);
                if (!bIsHexDigit(Hex[i][j]))
                {
                    cout << "Error in hex specification. Enter Again." << endl;
                    NoError = false;
                }
                else
                {
                    DecodeAddress(Hex[0], StartAddress.iHigh);
                    DecodeAddress(Hex[1], StartAddress.iLow);
                    DecodeAddress(Hex[2], EndAddress.iHigh);
                    DecodeAddress(Hex[3], EndAddress.iLow);
                }
            }
        }
    }
    while (!NoError);
}

void Dump (ostream& output, sRegisterType StartAddress, sRegisterType EndAddress)
{
    int Address;
    int LineAddress;
    char cHexByte[HEX_BYTE_LENGTH + 1];
    char cHexWord[HEX_WORD_LENGTH + 1];
    sRegisterType Sixteen;
    bool Carry, Ovflw;
    Sixteen.iHigh = 0;
    Sixteen.iLow = 16;
    StartAddress.iLow = (StartAddress.iLow / 16) * 16; // Start with new line
    output << "DUMP    0  1  2  3  4  5  6  7  8  9  ";
    output << "A  B  C  D  E  F       ASCII" << endl << endl;
    if (StartAddress.iHigh < (MEMORY_SIZE / (256)))
    {
        Address = StartAddress.iHigh * (256) + StartAddress.iLow;
    }
    else
    {
        Address = MEMORY_SIZE;
    }
    Carry = false;
    while (((StartAddress.iHigh < EndAddress.iHigh)
            || (StartAddress.iHigh == EndAddress.iHigh && StartAddress.iLow <= EndAddress.iLow))
           && !(Carry && StartAddress.iHigh == 0))
    {
        LineAddress = Address;
        RegToHex (StartAddress, cHexWord);
        output << cHexWord << ":  ";
        for (int i = 0; i < 16; i++)
        {
            if (Address < MEMORY_SIZE)
            {
                vDecToHexByte (iMemory[Address++], cHexByte);
            }
            else
            {
                cHexByte[0] = 0;
                cHexByte[1] = 0;
                cHexByte[2] = '\0';
            }
            output << cHexByte << " ";
        }
        output << " ";
        char cTemp;
        for (int i = 0; i < 16; i++)
        {
            cTemp = static_cast <char> (iMemory[LineAddress]);
            if (LineAddress < MEMORY_SIZE)
            {
                if ((iMemory[LineAddress] >= ' ') &&
                    (iMemory[LineAddress] <= '~'))
                {
                    output << cTemp;
                }
                else
                {
                    output << ".";
                }
                LineAddress++;
            }
            else
            {
                output << ".";
            }
        }
        output << endl;
        Adder (StartAddress, Sixteen, StartAddress, Carry, Ovflw);
    }
}

void DumpCommand()
{
    sRegisterType StartAddress;
    sRegisterType EndAddress;
    bool bRangeOK;
    cout << "Pep/8 memory dump:  ";
    do
    {
        bRangeOK = true;
        Parse (StartAddress, EndAddress);
        if ((EndAddress.iHigh == 0) && (EndAddress.iLow == 0))
        {
            EndAddress = StartAddress;
        }
        if ((StartAddress.iHigh > EndAddress.iHigh) ||
            ((StartAddress.iHigh == EndAddress.iHigh) &&
             (StartAddress.iLow > EndAddress.iLow)))
        {
            bRangeOK = false;
            cout << "Address range error. Start address must be less than end address." << endl;
        }
    }
    while (!bRangeOK);
    Dump (cout, StartAddress, EndAddress);
}

void TraceCommand()
{
    char cResponse[LINE_LENGTH];
    char ch;
    do
    {
        cout << "Trace  (p)rogram  (t)rap  (l)oader, or (a)djust display: ";
        cin.getline(cResponse, LINE_LENGTH);
        ch = toupper(cResponse[0]);
        if (ch != 'P' && ch != 'T' && ch != 'L' && ch != ' ' && ch != 'A')
        {
            cout << "Invalid response." << endl;
        }
        else if (ch == 'A')
        {
            cout << "Number of lines per screen dump (" << numTerminalLines << "): ";
            cin.getline(cResponse, LINE_LENGTH);
            ch = toupper(cResponse[0]);
            numTerminalLines = atoi(cResponse);
            numTerminalLines = (numTerminalLines < 8 ? 8 : numTerminalLines);
            cout << endl;
        }
    }
    while (ch != 'P' && ch != 'T' && ch != 'L' && ch != ' ' && ch != 'A');
    bSingleStep = false;
    bScrollingTrace = false;
    switch (ch)
    {
    case 'P':
        eTraceMode = eT_TR_PROGRAM; 
        ExecuteCommand();
        break;
    case 'T':
        eTraceMode = eT_TR_TRAPS;
        ExecuteCommand();
        break;
    case 'L':
        eTraceMode = eT_TR_LOADER;
        LoaderCommand();
        break;
    default:
        break;
    }
    eTraceMode = eT_TR_OFF;
}

void InputCommand()
{
    char cResponse[LINE_LENGTH];
    char ch;
    do
    {
        cout << "Input from  (k)eyboard  (f)ile: ";
        cin.getline(cResponse, LINE_LENGTH);
        ch = toupper(cResponse[0]);
        if (ch != 'K' && ch != 'F' && ch != ' ')
        {
            cout << "Invalid response." << endl;
        }
    }
    while (ch != 'K' && ch != 'F' && ch != ' ');
    chariInputStream.close();
    chariInputStream.clear();
    if (ch == 'K')
    {
        bKeyboardInput = true;
        cout << "Input is from keyboard." << endl;
    }
    else if (ch == 'F')
    {
        cout << "Enter input data file name: ";
        cin.getline(cInFileName, FILE_NAME_LENGTH);
        cInFileName[cin.gcount() - 1] = '\0';
        chariInputStream.open(cInFileName);
        if (chariInputStream.is_open())
        {
            bKeyboardInput = false;
            cout << "Input data file is " << cInFileName << endl;
        }
        else
        {
            bKeyboardInput = true;
            cout << "Could not open input data file " << cInFileName << endl;
            chariInputStream.close();
            chariInputStream.clear();
        }
    }
}

void OutputCommand()
{
    char cResponse[LINE_LENGTH];
    char ch;
    do
    {
        cout << "Output to  (s)creen  (f)ile:  ";
        cin.getline(cResponse, LINE_LENGTH);
        ch = toupper(cResponse[0]);
        if (ch != 'S' && ch != 'F' && ch != ' ')
        {
            cout << "Invalid response." << endl;
        }
    }
    while (ch != 'S' && ch != 'F' && ch != ' ');
    if (charoOutputStream.is_open())
    {
        charoOutputStream.close();
    }
    if (ch == 'S')
    {
        bScreenOutput = true;
        cout << "Output is to screen." << endl;
    }
    else if (ch == 'F')
    {
        cout << "Enter output data file name: ";
        cin.getline(cOutFileName, FILE_NAME_LENGTH);
        cOutFileName[cin.gcount() - 1] = '\0';
        charoOutputStream.open(cOutFileName);
        if (charoOutputStream.is_open())
        {
            bScreenOutput = false;
            cout << "Output data file is " << cOutFileName << endl;
        }
        else
        {
            bScreenOutput = true;
            cout << "Error opening file " << cOutFileName << endl;
        }
    }
}

void MainPrompt()
{
    char ch;
    do
    {
        cout << endl;
        cout << "(l)oad  e(x)ecute  (d)ump  (t)race  (i)nput  (o)utput  (q)uit: ";
        cin.getline(cCommand, LINE_LENGTH);
        ch = toupper(cCommand[0]);
        if (ch == 'L' || ch == 'X' || ch == 'D' || ch == 'T'
            || ch == 'I' || ch == 'O' || ch == 'Q')
        {
            switch (ch)
            {
            case 'L' : LoaderCommand(); break;
            case 'X' : ExecuteCommand(); break;
            case 'D' : DumpCommand(); break;
            case 'T' : TraceCommand(); break;
            case 'I' : InputCommand(); break;
            case 'O' : OutputCommand(); break;
            case 'Q' : break;
            }
        }
        else if (ch != ' ')
        {
            cout << "Invalid command." << endl;
        }
    }
    while (ch != 'Q');
    if (charoOutputStream.is_open())
    {
        charoOutputStream.close ();
    }
}

int main (int argc, char *argv[])
{
    bool bError;
   
    if (argc == 2)
    {
        if (strcmp(argv[1], "-v") == 0)
        {
            cout << "Pep/8 Simulator, version Unix 8.3, Pepperdine University" << endl;
        }
        else
        {
            cerr << "usage: pep8 [-v]" << endl;
            return 2;
        }
    }
    else if (argc > 2)
    {
        cerr << "usage: pep8 [-v]" << endl;
        return 2;
    }
    Initialize (bError);
    if (bError)
    {
        return 1;
    }
    else
    {
        InstallRom (bError);
    }
    if (bError)
    {
        return 3;
    }
    else
    {
        MainPrompt();
        return 0;
    }
}
