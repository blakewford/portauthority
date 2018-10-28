#pragma once

#include <parser.h>

class analyzer
{
public:
    static  void header(const char* executable, bool native, int32_t machine, uint32_t instructionCount);
    static  void footer();

    virtual void report() = 0;
    virtual void console() = 0;
    virtual void analyze(uint64_t address, const isa_instr* instruction) = 0;
};

void analyzer::header(const char* executable, bool native, int32_t machine, uint32_t instructionCount)
{
    char data[64];
    memset(data, '\0', 64);

    if(native)
    {
        FILE* lscpu = popen("lscpu", "r");
        if(lscpu == nullptr) return;

        uint8_t skip = 13;
        while(skip--)
        {
            char* value = fgets(data, sizeof(data)-1, lscpu);
        }
        pclose(lscpu);
    }
    else
    {
        strcpy(data, "Unknown");
    }

    const char* arch = "";
    switch(machine)
    {
        case EM_AARCH64:
            arch = "AArch64";
            break;
        case EM_ARM:
            arch = "ARM";
            break;
        case EM_AVR:
            arch = "Atmel AVR 8-bit microcontroller";
            break;
        case EM_386:
            arch = "Intel 80386";
            break;
        case EM_X86_64:
            arch = "Advanced Micro Devices X86-64";
            break;
        default:
            break;
    }

    printf("<html>\n<body bgcolor=\"#ccecfc\">\n<div id=\"chart\"><br/><font color=\"gray\"><b>%s<br/>%s<br/>%s<br/>Instruction count: %u<br/><br/></b></font></div>\n<script>\n", arch, data, executable, instructionCount);
}

void analyzer::footer()
{
    printf("</body></html>");
}

void analyzer::analyze(uint64_t address, const isa_instr* instruction)
{
    printf("%lX %lX %s %s %s\n", address, instruction->m_opcode, instruction->m_mnem, instruction->m_group, instruction->m_subgroup);
}
