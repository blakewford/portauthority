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
    , m_adjustment(0)
    , m_cycles(0)
    , m_bytes(0)
    {

    }

    void adjustCount(int32_t adjustment)
    {
        m_bytes += adjustment;
        m_adjustment = adjustment;
    }

    uint64_t getCycleCount()
    {
        return m_cycles + m_adjustment;
    }

    virtual void report()
    {
        printf("<div><font color=\"gray\"><b>%.2f%c Coverage</b></font><br/><br/></div>\n", (m_bytes/(double)m_textSize)*100, '%');
    }

    virtual void console()
    {
        for(auto iter = m_addresses.begin(); iter != m_addresses.end(); iter++)
        {
            printf("0x%lx\n", *iter);
        }
        printf("0x%lx 0x%lx 0x%lx %lu\n", m_textStart, m_bytes, m_textSize, m_cycles);
    }

    virtual void analyze(uint64_t address, const isa_instr* instruction)
    {
        if(m_addresses.insert(address).second)
        {
            m_bytes += instruction->m_size;
        }
        m_cycles += instruction->m_cycles;
    }

private:
    uint64_t m_textStart;
    uint64_t m_textSize;
    uint64_t m_bytes;
    uint64_t m_cycles;
    int32_t  m_adjustment;

    std::set <uint64_t> m_addresses;
};
