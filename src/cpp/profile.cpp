#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma pack(push)
#pragma pack(1)
struct format
{
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
    char name[128];
    float count;

    format()
    {
        memset(name, '\0', 128);
    }
};
#pragma pack(pop)

#define FORMAT_SIZE 18
#define FORMAT_COUNT 90

int32_t main()
{
    const char* files[] = {"./profiles/aarch64.csv", "./profiles/x86.csv"};
    int32_t count = sizeof(files)/sizeof(const char*);
    format* formats[count][FORMAT_COUNT];

    int32_t k = 0;
    char* csv = nullptr;
    while(count--)
    {
        FILE* library = fopen(files[count], "r");    
        if(library)
        {
            fseek(library, 0, SEEK_END);
            int32_t size = ftell(library);
            rewind(library);
            csv = (char*)malloc(size);
            size_t read = fread(csv, 1, size, library);
            if(read != size) return -1;

            format* f = new format();
            int32_t i = 0;
            int32_t j = 0;
            char* prev = csv;
            char* current = csv;
            while(size)
            {
                while(*current != '\n')
                    current++;
    
                std::string line(prev, current);
                std::string name = strtok((char*)line.c_str(), " ");
                float value = atof(strtok(nullptr, " "));
                switch(i)
                {
                    case 0:
                        strcpy(f->name, name.c_str());
                        f->count = value;
                        break;                                              
                    default:
                        switch(i)
                        {
                            case 1:
                               f->segreg = value;
                               break;
                            case 17:
                               f->datamov = value;
                               break;
                        }
                        break;
                }
    
                i++;
                if(i == FORMAT_SIZE)
                {
                    formats[k][j] = f;
                    f = new format();
                    i = 0;
                    j++;
                }
                prev = ++current;
                size -= line.length() + 1;
            }

            free(csv);
            fclose(library);
            k++;
        }
    }

    count = sizeof(files)/sizeof(const char*);
    while(count--)
    {
        int32_t size = FORMAT_COUNT;
        while(size--)
        {
//            delete formats[count][size];
        } 
    }   

    return 0;
}
