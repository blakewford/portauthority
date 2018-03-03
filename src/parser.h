#include <string.h>
#include "rapidjson/reader.h"
using namespace rapidjson;
using namespace std;

class container_object;

#define MAX_BUFFER_SIZE 64
#define USE_RAPID 1

#ifndef USE_RAPID
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
#else
struct isa_instr
{
    bool Null()
    {
        return true;
    }

    bool Bool(bool b)
    {
        return true;
    }

    bool Int(int i)
    {
        return true;
    }

    bool Uint(unsigned u)
    {
        return true;
    }

    bool Int64(int64_t i)
    {
        return true;
    }

    bool Uint64(uint64_t u)
    {
        return true;
    }

    bool Double(double d)
    {
        return true;
    }

    bool RawNumber(const char* str, SizeType length, bool copy)
    {
        return true;
    }

    bool String(const char* str, SizeType length, bool copy)
    {
        return true;
    }

    bool StartObject()
    {
        return true;
    }

    bool Key(const char* str, SizeType length, bool copy)
    {
        return true;
    }

    bool EndObject(SizeType memberCount)
    {
        return true;
    }

    bool StartArray()
    {
        return true;
    }

    bool EndArray(SizeType elementCount)
    {
        return true;
    }

    void populate(container_object* obj);
    const char* get_name(){ return m_name; }
    virtual void populate_specific(container_object* obj) = 0;

    private:
        char m_name[MAX_BUFFER_SIZE];

};

struct x86_instr: public isa_instr
{
    void populate_specific(container_object* obj);
    long get_opcode(){ return m_opcode; };
    const char* get_mnemonic(){ return m_mnem; };

    private:
        long m_opcode;
        char m_mnem[MAX_BUFFER_SIZE];
};
#endif
