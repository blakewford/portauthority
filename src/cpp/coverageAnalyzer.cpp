#include <stdio.h>
#include <stdint.h>
#include "analyzer.h"

class coverageAnalyzer: public analyzer
{
public:

    virtual void report()
    {
    }

    virtual void console()
    {
    }

    virtual void analyze(uint64_t address, const isa_instr* instruction)
    {   
    }
};