#include <cctype>
#include <string>
#include <udis86.h>
#include <algorithm>

void dumpbin(const uint8_t* binary, bool amd64, uint8_t machine, uint64_t entryAddress, sectionInfo* info, std::deque<uint64_t>& addresses)
{
    uint64_t address = entryAddress;
    if(machine == EM_ARM)
    {
        address = info->address;
    }

    uint64_t offset = info->offset;
    while(offset--)
    {
        binary++;
    }

    int64_t size = info->size;

    if(machine == EM_X86_64)
    {
        const int32_t INSTRUCTION_LENGTH_MAX = 7;

        ud_t u;
        ud_init(&u);
        ud_set_mode(&u, amd64 ? 64: 32);
        ud_set_syntax(&u, UD_SYN_ATT);
        while(size > 0)
        {
            uint32_t* instruction = (uint32_t*)binary;

            int byte = 0;
            bool invalid = true;
            while(invalid && (byte <= INSTRUCTION_LENGTH_MAX))
            {
                byte++;
                ud_set_input_buffer(&u, (uint8_t*)binary, byte);
                ud_disassemble(&u);
                invalid = strcmp(ud_insn_asm(&u), "invalid ") == 0;
            }

            addresses.push_back(address);
            address += byte;
            binary += byte;
            size -= byte;

        }
    }
    else if(machine == EM_AVR)
    {
        while(size > 0)
        {
            uint32_t instruction = *(uint32_t*)binary;

            uint8_t opcode0 = (instruction & 0xFF00) >> 8;
            uint8_t opcode1 = instruction & 0xFF;
            uint8_t opcode2 = (instruction & 0xFF000000) >> 24;
            uint8_t opcode3 = (instruction & 0xFF0000) >> 16;

            int byte = 2;
            switch(opcode2)
            {
                case 0x90:
                case 0x91:
                    if((opcode3 & 0xF) == 0x0)
                    {
                        byte = 4;
                    }
                    break;
                case 0x92:
                case 0x93:
                    if((opcode3 & 0xF) == 0x0)
                    {
                        byte = 4;
                    }
                    break;
                case 0x94:
                case 0x95:
                    switch(opcode3)
                    {
                        case 0xC:
                        case 0xD:
                            byte = 4;
                            break;
                        case 0xE:
                        case 0xF:
                            byte = 4;
                            break;
                        default:
                            break;
                    }
                    break;
                default:
                    break;
            }

            addresses.push_back(address);
            address += byte;
            binary += byte;
            size -= byte;
        }
    }
    else
    {
        while(size > 0)
        {
            addresses.push_back(address);
            uint32_t instruction = *(uint32_t*)binary;

            bool invalid = true;
            const char* test = arm64_decode(instruction);
            invalid = strcmp(test, "invalid") == 0;
            if(!invalid)
            {
                std::string clean(test);
                std::transform(clean.begin(), clean.end(), clean.begin(),[](unsigned char c){ return std::tolower(c); });
                printf("     %" PRIu64 ":	%u 	%s\n", address, instruction, clean.c_str());
            }

            address += 4;
            binary += 4;
            size -= 4;
        }
    }
}
