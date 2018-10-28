#include <set>
#include <stdio.h>
#include <stdint.h>
#include "analyzer.h"

class coverageAnalyzer: public analyzer
{
public:

    coverageAnalyzer(uint64_t textStart, uint64_t textSize)
    : m_textStart(textStart)
    , m_textSize(textSize)
    , m_count(0)
    {

    }

    virtual void report()
    {
        printf("<div><font color=\"gray\"><b>%.2f%c Coverage</b></font><br/><br/></div>\n", (m_count/(double)m_textSize)*100, '%');
    }

    virtual void console()
    {
        for(auto iter = m_addresses.begin(); iter != m_addresses.end(); iter++)
        {
            printf("0x%lx\n", *iter);
        }
        printf("0x%lx 0x%lx 0x%lx\n", m_textStart, m_count, m_textSize);
    }

    virtual void analyze(uint64_t address, const isa_instr* instruction)
    {
        if(m_addresses.insert(address).second)
        {
            m_count += instruction->m_size;
        }
    }

private:
    uint64_t m_textStart;
    uint64_t m_textSize;
    uint64_t m_count;

    std::set <uint64_t> m_addresses;
};