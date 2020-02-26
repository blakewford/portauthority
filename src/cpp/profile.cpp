#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma pack(push)
#pragma pack(1)
struct format
{
    char name[128];
    float normalized;
    float ticks;
    float instructions;
    float identified;
    float segreg;
    float flgctrl;
    float inout;
    float str;
    float brk;
    float cond;
    float shftrot;
    float decimal;
    float binary;
    float conver;
    float stack;
    float control;
    float branch;
    float bit;
    float logical;
    float arith;
    float datamov;

    format()
    {
        memset(name, '\0', 128);
    }
};
#pragma pack(pop)

#define FORMAT_COUNT 72
const char* profiles[] = {"./profiles/aarch64.csv", "./profiles/x86.csv"};
format* formats[sizeof(profiles)/sizeof(const char*)][FORMAT_COUNT];

bool loadProfiles()
{
    int32_t k = 0;
    char* csv = nullptr;
    int32_t count = sizeof(profiles)/sizeof(const char*);
    while(count--)
    {
        FILE* library = fopen(profiles[count], "r");    
        if(library)
        {
            fseek(library, 0, SEEK_END);
            int32_t size = ftell(library);
            rewind(library);
            csv = (char*)malloc(size);
            size_t read = fread(csv, 1, size, library);
            if(read != size) return false;

            int32_t j = 0;
            char* prev = csv;
            char* current = csv;
            while(size)
            {
                format* f = new format();
                while(*current != '\n')
                    current++;
    
                std::string line(prev, current);
                std::string name = strtok((char*)line.c_str(), " ");

                f->normalized = atof(strtok(nullptr, " "));
                f->ticks = atof(strtok(nullptr, " "));
                f->instructions = atof(strtok(nullptr, " "));
                f->identified = atof(strtok(nullptr, " "));
                f->segreg = atof(strtok(nullptr, " "));
                f->flgctrl = atof(strtok(nullptr, " "));
                f->inout = atof(strtok(nullptr, " "));
                f->str = atof(strtok(nullptr, " "));
                f->brk = atof(strtok(nullptr, " "));
                f->cond = atof(strtok(nullptr, " "));
                f->shftrot = atof(strtok(nullptr, " "));
                f->decimal = atof(strtok(nullptr, " "));
                f->binary = atof(strtok(nullptr, " "));
                f->conver = atof(strtok(nullptr, " "));
                f->stack = atof(strtok(nullptr, " "));
                f->control = atof(strtok(nullptr, " "));
                f->branch = atof(strtok(nullptr, " "));
                f->bit = atof(strtok(nullptr, " "));
                f->logical = atof(strtok(nullptr, " "));
                f->arith = atof(strtok(nullptr, " "));
                f->datamov = atof(strtok(nullptr, " "));

                formats[k][j] = f;
                j++;

                prev = ++current;
                size -= line.length() + 1;
            }

            free(csv);
            fclose(library);
            k++;
        }
    }

    return true;
}

void unloadProfiles()
{
    int32_t count = sizeof(profiles)/sizeof(const char*);
    while(count--)
    {
        int32_t size = FORMAT_COUNT;
        while(size--)
        {
            delete formats[count][size];
        } 
    }    
}
