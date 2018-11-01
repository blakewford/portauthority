#include <udis86.h>

void dumpbin(const uint8_t* binary, bool amd64, uint64_t entryAddress, sectionInfo* info, std::deque<uint64_t>& addresses)
{
    uint64_t address = entryAddress;
    uint64_t offset = info->offset;
    while(offset--)
    {
        binary++;
    }

    const int32_t INSTRUCTION_LENGTH_MAX = 7;

    ud_t u;
    ud_init(&u);
    ud_set_mode(&u, amd64 ? 64: 32);
    ud_set_syntax(&u, UD_SYN_ATT);
    uint64_t size = info->size;
    while(size)
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