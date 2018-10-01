#pragma once

#include <parser.h>

class analyzer
{
public:
    virtual void report() = 0; 
    virtual void console() = 0;   
    virtual void analyze(uint64_t address, const isa_instr* instruction) = 0;
};

void analyzer::analyze(uint64_t address, const isa_instr* instruction)
{
    printf("%lX %lX %s %s %s\n", address, instruction->m_opcode, instruction->m_mnem, instruction->m_group, instruction->m_subgroup);
}