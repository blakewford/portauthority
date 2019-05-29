#include <stdint.h>

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
            printf("UNALLOCATED\n");
            break;
        case 4:
        case 6:
        case 12:
        case 14:
            printf("Load and store\n");
            break;
        case 8:
        case 9:
            printf("Data processing - immediate\n");
            break;
        case 10:
        case 11:
            printf("Branch, exception, system\n");
            break;
        case 5:
        case 13:
            printf("Data processing - register\n");
            break;
        case 7:
        case 15:
            printf("Data processing - SIMD and floating point\n");
            break;
        default:
            break;
    }

    return "invalid";
}
