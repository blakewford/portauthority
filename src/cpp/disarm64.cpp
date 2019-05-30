#include <stdint.h>

/*
datamov
arith
logical
bit
branch
control
stack
conver
binary
decimal
shftrot
cond
break
string
inout
flgctrl
segreg
*/

/*
ADR
ADRL
ADRP
AND
ASR
ASRV
AT
B.cond
ADDS
ANDS
B
BFI
BFM
BFXIL
BIC
BL
BLR
BR
BRK
CBNZ
CBZ
CCMN
CCMP
CINC
CINV
CLREX
CLS
CLZ
CMN
BICS
CNEG
CRC32B
CRC32H
CRC32W
CRC32CB
CRC32CH
CRC32CW
CRC32CX
CSEL
CSET
CSETM
CSINC
CSINV
CSNEG
DC
DCPS1
DCPS2
DCPS3
DMB
DRPS
DSB
EON
CMP
EOR
ERET
EXTR
HINT
HLT
HVC
IC
ISB
LSL
LSLV
LSR
LSRV
MADD
MNEG
MOV
MOVK
MOVL
MOVN
MOVZ
MRS
MSR
MSUB
MUL
MVN
NEG
NEGS
NGC
NGCS
NOP
ORN
RBIT
RET
REV
REV16
REV32
ROR
RORV
SBC
SBCS
SBFIZ
SBFM
SBFX
SDIV
SEV
SEVL
SMADDL
SMC
ORR
SMNEGL
SMSUBL
SMULH
SMULL
SUB
SVC
SXTB
SXTH
SXTW
SYS
SYSL
TBNZ
TBZ
TLBI
TST
UBFIZ
UBFM
UBFX
UDIV
SUBS
TST
UMADDL
UMNEGL
UMSUBL
UMULH
UMULL
UXTB
UXTH
WFE
WFI
YIELD
*/

const uint32_t FILTER0 = 0x7FE0FC00;
const uint32_t FILTER1 = 0x7FE00000;
const uint32_t FILTER2 = 0x7FE20000;

const char* arm64_decode(uint32_t opcode)
{
    uint16_t category = (opcode & 0x1E000000) >> 25;
    switch(category)
    {
        case 0: //Technically UNALLOCATED
            break;
        case 1:
        case 2:
        case 3:
//            printf("UNALLOCATED\n");
            break;
        case 4:
        case 6:
        case 12:
        case 14:
//            printf("Load and store\n");
            break;
        case 8:
        case 9:
//            printf("Data processing - immediate\n");
            if((opcode & FILTER1) == 0x11000000)
            {
                return "ADD";
            }
            break;
        case 10:
        case 11:
//            printf("Branch, exception, system\n");
            break;
        case 5:
        case 13:
//            printf("Data processing - register\n");
            if((opcode & FILTER1) == 0x1B200000)
            {
                return "ADD";
            }
            else if((opcode & FILTER2) == 0x0B000000)
            {
                return "ADD";
            }
            break;
        case 7:
        case 15:
//            printf("Data processing - SIMD and floating point\n");
            if((opcode & FILTER0) == 0x1A000000)
            {
                return "ADC";
            }
            else if((opcode & FILTER0) == 0x3A000000)
            {
                return "ADCS";
            }
            break;
        default:
            break;
    }

    return "invalid";
}
