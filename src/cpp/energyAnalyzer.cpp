#include <stdio.h>
#include "analyzer.h"

class energyAnalyzer: public analyzer
{
public:

    virtual void report()
    {
    }

    virtual void console()
    {
        printf("\e[1mmain.cpp:126:36: \e[95mwarning:\e[0m energy inefficiency detected\n"); //<-- VS Code output format
    }

    virtual void analyze(const isa_instr* instruction)
    {
    }
};