#include <string.h>

class container_object;

#define MAX_BUFFER_SIZE 64

class isa_instr
{
    public:
        isa_instr(){ memset(m_name, '\0', MAX_BUFFER_SIZE); }
        void populate(container_object* obj);
        const char* get_name(){ return m_name; }
        virtual void populate_specific(container_object* obj) = 0;

    private:
        char m_name[MAX_BUFFER_SIZE];
};

class x86_instr: public isa_instr
{
    public:
        x86_instr():
            m_opcode(0)
        { memset(m_mnem, '\0', MAX_BUFFER_SIZE); }
        void populate_specific(container_object* obj);
        long get_opcode(){ return m_opcode; };
        const char* get_mnemonic(){ return m_mnem; };
        bool get_bool(){ return false; };

    private:
        long m_opcode;
        char m_mnem[MAX_BUFFER_SIZE];
};
