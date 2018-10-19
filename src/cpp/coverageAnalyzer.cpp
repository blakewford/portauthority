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
    {

    }

    virtual void report()
    {
    }

    virtual void console()
    {
        for(auto iter = m_addresses.begin(); iter != m_addresses.end(); iter++)
        {
            printf("0x%lx\n", *iter);
        }
        printf("0x%lx 0x%lx\n", m_textStart, m_textSize);
    }

    virtual void analyze(uint64_t address, const isa_instr* instruction)
    {
        m_addresses.insert(address);
    }

private:
    uint64_t m_textStart;
    uint64_t m_textSize;

    std::set <uint64_t> m_addresses;
};