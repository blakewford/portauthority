#include <map>
#include <vector>
#include <stdint.h>
#include <iostream>
#include <algorithm>
#include "analyzer.h"

class energyAnalyzer: public analyzer
{
public:

    energyAnalyzer()
    : m_addInstr(0)
    , m_subInstr(0)
    {
        m_addAddresses.clear();
    }

    virtual void report()
    {
    }

    virtual void statistics()
    {
    }    

    virtual void console()
    {
        std::map<std::string, int32_t> output;
        auto iter = std::unique(m_addAddresses.begin(), m_addAddresses.end());
        for(iter; iter != m_addAddresses.end(); iter++)
        {
            auto line = gAddressToLineTable.find(*iter);
            if(line != gAddressToLineTable.end())
            {
                lineInfo* pInfo = line->second;
                //VS Code output format
                std::string file = pInfo->fileName;
                auto test = output.insert(std::make_pair(file, pInfo->lineNumber));
                if(test.second)
                {
                    printf("\e[1m%s:%d:0: \e[95mwarning:\e[0m energy inefficiency detected\n", pInfo->fileName, pInfo->lineNumber);
                }
            }
        }
    }

    virtual void analyze(uint64_t address, const isa_instr* instruction)
    {
        if(!strcmp(instruction->m_mnem, "add"))
        {
            m_addAddresses.push_back(address);
            m_addInstr++;
        }
        if(!strcmp(instruction->m_mnem, "sub"))
        {
            m_subInstr++;
        }  
    }

private:

    int32_t m_addInstr;
    int32_t m_subInstr;
    std::vector<uint64_t> m_addAddresses;
};
