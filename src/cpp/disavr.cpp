#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>

bool longOpcode(uint16_t opcode)
{
    uint8_t opcode0 = (opcode & 0xFF00) >> 8;
    uint8_t opcode1 = opcode & 0xFF;

    switch(opcode0)
    {
        case 0x90:
        case 0x91:
            if((opcode1 & 0xF) == 0) //lds
            {
                return true;
            }
            break;
        case 0x92:
        case 0x93:
            if((opcode1 & 0xF) == 0) //sts
            {
                return true;
            }
            break;
        case 0x94:
        case 0x95:
            if((opcode1 & 0xF) > 0xB) //jmp / call
            {
                return true;
            }
            break;
    }

    return false;
}

void handleUnimplemented()
{
    assert(0);
}

int32_t fetch(uint16_t opcode)
{
    uint8_t opcode0 = (opcode & 0xFF00) >> 8;
    uint8_t opcode1 = opcode & 0xFF;
    if(((opcode0 == 0x95) && (opcode1 == 0x98))) //break
        return false;

    switch(opcode0)
    {
        case 0x0:
            if(opcode1 == 0x00) //nop
            {
                break;
            }
            handleUnimplemented();
        case 0x1: //movw
            break;
        case 0x2: //muls
        case 0x3: //mulsu
            break;
        case 0x4:
        case 0x5:
        case 0x6:
        case 0x7: //cpc
            break;
        case 0x8:
        case 0x9:
        case 0xA:
        case 0xB: //sbc
            break;
        case 0xC:
        case 0xD:
        case 0xE:
        case 0xF: //add
            break;
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13: //cpse
            //longOpcode
            break;
        case 0x14:
        case 0x15:
        case 0x16:
        case 0x17: //cp
           break;
        case 0x18:
        case 0x19:
        case 0x1A:
        case 0x1B: //sub
           break;
        case 0x1C:
        case 0x1D:
        case 0x1E:
        case 0x1F: //adc
           break;
        case 0x20:
        case 0x21:
        case 0x22:
        case 0x23: //and
           break;
        case 0x24:
        case 0x25:
        case 0x26:
        case 0x27: //eor
           break;
        case 0x28:
        case 0x29:
        case 0x2A:
        case 0x2B: //or
           break;
        case 0x2C:
        case 0x2D:
        case 0x2E:
        case 0x2F: //mov
           break;
        case 0x30:
        case 0x31:
        case 0x32:
        case 0x33:
        case 0x34:
        case 0x35:
        case 0x36:
        case 0x37:
        case 0x38:
        case 0x39:
        case 0x3A:
        case 0x3B:
        case 0x3C:
        case 0x3D:
        case 0x3E:
        case 0x3F: //cpi
            break;
        case 0x40:
        case 0x41:
        case 0x42:
        case 0x43:
        case 0x44:
        case 0x45:
        case 0x46:
        case 0x47:
        case 0x48:
        case 0x49:
        case 0x4A:
        case 0x4B:
        case 0x4C:
        case 0x4D:
        case 0x4E:
        case 0x4F: //sbci
            break;
        case 0x50:
        case 0x51:
        case 0x52:
        case 0x53:
        case 0x54:
        case 0x55:
        case 0x56:
        case 0x57:
        case 0x58:
        case 0x59:
        case 0x5A:
        case 0x5B:
        case 0x5C:
        case 0x5D:
        case 0x5E:
        case 0x5F: //subi
            break;
        case 0x60:
        case 0x61:
        case 0x62:
        case 0x63:
        case 0x64:
        case 0x65:
        case 0x66:
        case 0x67:
        case 0x68:
        case 0x69:
        case 0x6A:
        case 0x6B:
        case 0x6C:
        case 0x6D:
        case 0x6E:
        case 0x6F: //ori
            break;
        case 0x70:
        case 0x71:
        case 0x72:
        case 0x73:
        case 0x74:
        case 0x75:
        case 0x76:
        case 0x77:
        case 0x78:
        case 0x79:
        case 0x7A:
        case 0x7B:
        case 0x7C:
        case 0x7D:
        case 0x7E:
        case 0x7F: //andi
            break;
        case 0x80:
        case 0x81:
            if((opcode1 & 0xF) >= 0x8) //ld (ldd) y
            {
                break;
            }
            if((opcode1 & 0xF) < 0x8) //ld (ldd) z
            {
                break;
            }
            handleUnimplemented();
        case 0x82:
        case 0x83:
            if((opcode1 & 0xF) >= 0x8) //st (std) y
            {
                break;
            }
            if((opcode1 & 0xF) < 0x8) //st (std) z
            {
                break;
            }
            handleUnimplemented();
        case 0x84:
        case 0x85:
        case 0x8C:
        case 0x8D:
            if((opcode1 & 0xF) >= 0x8) //ld (ldd) y
            {
                break;
            }
            if((opcode1 & 0xF) < 0x8) //ld (ldd) z
            {
                break;
            }
            handleUnimplemented();
        case 0x86:
        case 0x87:
            if((opcode1 & 0xF) >= 0x8) //st (std) y
            {
                break;
            }
            if((opcode1 & 0xF) < 0x8) //st (std) z
            {
                break;
            }
            handleUnimplemented();
        case 0x88:
        case 0x89:
            if((opcode1 & 0xF) >= 0x8) //ld (ldd) y
            {
                break;
            }
            if((opcode1 & 0xF) < 0x8) //ld (ldd) z
            {
                break;
            }
            handleUnimplemented();
        case 0x8A:
        case 0x8B:
        case 0x8E:
        case 0x8F:
            if((opcode1 & 0xF) >= 0x8) //st (std) y
            {
                break;
            }
            if((opcode1 & 0xF) < 0x8) //st (std) z
            {
                break;
            }
            handleUnimplemented();
        case 0x90:
        case 0x91:
            if((opcode1 & 0xF) == 0x0) //lds
            {
               break;
            }
            if((opcode1 & 0xF) == 0x1) //ld z+
            {
                break;
            }
            if((opcode1 & 0xF) == 0x2) //ld -z
            {
                break;
            }
            if((opcode1 & 0xF) == 0x4) //lpm (rd, z)
            {
                break;
            }
            if((opcode1 & 0xF) == 0x5) //lpm (rd, z+)
            {
                break;
            }
            if((opcode1 & 0xF) == 0x7) //elpm
            {
                break;
            }
            if((opcode1 & 0xF) == 0x9) //ld y+
            {
                break;
            }
            if((opcode1 & 0xF) == 0xA) //ld -y
            {
                break;
            }
            if((opcode1 & 0xF) == 0xC) //ld x
            {
                break;
            }
            if((opcode1 & 0xF) == 0xD) //ld x+
            {
                break;
            }
            if((opcode1 & 0xF) == 0xF) //pop
            {
                break;
            }
            handleUnimplemented();
        case 0x92:
        case 0x93:
           if((opcode1 & 0xF) == 0x0) //sts
           {
              break;
           }
           if((opcode1 & 0xF) == 0x1) //st (std) z+
           {
               break;
           }
           if((opcode1 & 0xF) == 0x2) //st (std) -z
           {
               break;
           }
           if((opcode1 & 0xF) == 0x9) //st (std) y+
           {
               break;
           }
           if((opcode1 & 0xF) == 0xA) //st (std) -y
           {
               break;
           }
           if((opcode1 & 0xF) == 0xF) //push
           {
               break;
           }
           if((opcode1 & 0xF) == 0xC) //st x
           {
               break;
           }
           if((opcode1 & 0xF) == 0xD) //st x+
           {
               break;
           }
           if((opcode1 & 0xF) == 0xE) //st -x
           {
               break;
           }
           handleUnimplemented();
        case 0x94:
        case 0x95:
            if((opcode0 == 0x94) && (opcode1 == 0x08)) //sec
            {
                break;
            }
            if((opcode0 == 0x94) && (opcode1 == 0x09)) //ijmp
            {
                break;
            }
            if((opcode0 == 0x94) && (opcode1 == 0x68)) //set
            {
                break;
            }
            if((opcode0 == 0x94) && (opcode1 == 0x78)) //sei
            {
                break;
            }
            if((opcode0 == 0x94) && (opcode1 == 0xE8)) //clt
            {
                break;
            }
            if((opcode0 == 0x94) && (opcode1 == 0xF8)) //cli
            {
                break;
            }
            if((opcode1 == 0x88) || (opcode1 == 0xA8)) //sleep || wdr
            {
                break;
            }
            if((opcode0 == 0x95) && (opcode1 == 0x8)) //ret
            {
                break;
            }
            if((opcode0 == 0x95) && (opcode1 == 0x9)) //icall
            {
                break;
            }
            if((opcode0 == 0x95) && (opcode1 == 0x18)) //reti
            {
                break;
            }
            switch(opcode1 & 0x0F)
            {
                case 0x0: //com
                    break;
                case 0x1: //neg
                    break;
                case 0x2: //swap
                    break;
                case 0x3: //inc
                    break;
                case 0x5: //asr
                    break;
                case 0x6: //lsr
                    break;
                case 0x7: //ror
                    break;
                case 0xA: //dec
                    break;
                case 0xC:
                case 0xD: //jmp
                    break;
                case 0xE:
                case 0xF: //call
                    break;
                default:
                    handleUnimplemented();
                    break;
            }
            break;
        case 0x96: //adiw
        case 0x97: //sbiw
            switch((opcode1 & 0x30) >> 4)
            {
                case 0:
                    break;
                case 1:
                    break;
                case 2:
                    break;
                case 3:
                    break;
                default:
                    handleUnimplemented();
            }
            if(opcode0 == 0x96)
            {
                break;
            }
            if(opcode0 == 0x97)
            {
                break;
            }
            switch((opcode1 & 0x30) >> 4)
            {
                case 0:
                    break;
                case 1:
                    break;
                case 2:
                    break;
                case 3:
                    break;
                default:
                    handleUnimplemented();
            }
            break;
        case 0x98: //cbi
            break;
        case 0x9A: //sbi
            break;
        case 0x9B: //sbis
            //longOpcode
            break;
        case 0x9C:
        case 0x9D:
        case 0x9E:
        case 0x9F: //mul
           break;
        case 0xA0:
        case 0xA1:
        case 0xA4:
        case 0xA5:
        case 0xA8:
        case 0xA9:
        case 0xAC:
        case 0xAD:
            if((opcode1 & 0xF) < 0x8) //ld (ldd) z
            {
                break;
            }
            if((opcode1 & 0xF) >= 0x8) //ld (ldd) y
            {
                break;
            }
            handleUnimplemented();
        case 0xA2:
        case 0xA3:
        case 0xA6:
        case 0xA7:
        case 0xAA:
        case 0xAB:
        case 0xAE:
        case 0xAF:
            if((opcode1 & 0xF) < 0x8) //st (std) z
            {
                break;
            }
            if((opcode1 & 0xF) >= 0x8) //st (std) y
            {
                break;
            }
            handleUnimplemented();
        case 0xB0:
        case 0xB1:
        case 0xB2:
        case 0xB3:
        case 0xB4:
        case 0xB5:
        case 0xB6:
        case 0xB7: //in
            break;
        case 0xB8:
        case 0xB9:
        case 0xBA:
        case 0xBB:
        case 0xBC:
        case 0xBD:
        case 0xBE:
        case 0xBF: //out
            break;
        case 0xC0:
        case 0xC1:
        case 0xC2:
        case 0xC3:
        case 0xC4:
        case 0xC5:
        case 0xC6:
        case 0xC7:
        case 0xC8:
        case 0xC9:
        case 0xCA:
        case 0xCB:
        case 0xCC:
        case 0xCD:
        case 0xCE:
        case 0xCF: //rjmp
            if((opcode0 == 0xCF) && (opcode1 == 0xFF))
            {
                //Program Exit
                return false;
            }
            break;
        case 0xD0:
        case 0xD1:
        case 0xD2:
        case 0xD3:
        case 0xD4:
        case 0xD5:
        case 0xD6:
        case 0xD7:
        case 0xD8:
        case 0xD9:
        case 0xDA:
        case 0xDB:
        case 0xDC:
        case 0xDD:
        case 0xDE:
        case 0xDF: //rcall
            break;
        case 0xE0:
        case 0xE1:
        case 0xE2:
        case 0xE3:
        case 0xE4:
        case 0xE5:
        case 0xE6:
        case 0xE7:
        case 0xE8:
        case 0xE9:
        case 0xEA:
        case 0xEB:
        case 0xEC:
        case 0xED:
        case 0xEE:
        case 0xEF: //ldi
            break;
        case 0xF0:
        case 0xF1:
        case 0xF2:
        case 0xF3:
            if((((opcode0 & 0x0C) >> 2) == 0x0) && ((opcode1 & 0x7) == 0x0)) //brcs
            {
                break;
            }
            if((((opcode0 & 0x0C) >> 2) == 0x0) && ((opcode1 & 0x7) == 0x1)) //breq
            {
                break;
            }
            if((((opcode0 & 0x0C) >> 2) == 0x0) && ((opcode1 & 0x7) == 0x2)) //brmi
            {
                break;
            }
            if((((opcode0 & 0x0C) >> 2) == 0x0) && ((opcode1 & 0x7) == 0x4)) //brlt
            {
                break;
            }
            if((((opcode0 & 0x0C) >> 2) == 0x0) && ((opcode1 & 0x7) == 0x6)) //brts
            {
                break;
            }
            handleUnimplemented();
        case 0xF4:
        case 0xF5:
        case 0xF6:
        case 0xF7:
            if((((opcode0 & 0x0C) >> 2) == 0x1) && ((opcode1 & 0x7) == 0x2)) //brpl
            {
                break;
            }
            if((((opcode0 & 0x0C) >> 2) == 0x1) && ((opcode1 & 0x7) == 0x0)) //brcc
            {
                break;
            }
            if((((opcode0 & 0x0C) >> 2) == 0x1) && ((opcode1 & 0x7) == 0x1)) //brne
            {
                break;
            }
            if((((opcode0 & 0x0C) >> 2) == 0x1) && ((opcode1 & 0x7) == 0x4)) //brge
            {
                break;
            }
            if((((opcode0 & 0x0C) >> 2) == 0x1) && ((opcode1 & 0x7) == 0x6)) //brtc
            {
                break;
            }
            handleUnimplemented();
        case 0xF8:
        case 0xF9: //bld
            if((opcode1 & 0xF) < 0x8)
            {
                break;
            }
            handleUnimplemented();
        case 0xFA:
        case 0xFB: //bst
            break;
        case 0xFC:
        case 0xFD: //sbrc
            if((opcode1 & 0xF) < 0x8)
            {
                //longOpcode
                break;
            }
            handleUnimplemented();
        case 0xFE:
        case 0xFF: //sbrs
            if((opcode1 & 0xF) < 0x8)
            {
                //longOpcode
                break;
            }
            handleUnimplemented();
        default:
            handleUnimplemented();
            break;
    }

    return true;
}