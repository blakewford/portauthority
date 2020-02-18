#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct format
{
    char name[128];
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
                switch(i)
                {
                    case 0:
                        strcpy(f->name, line.c_str());
                        break;                                              
                    default:
                        strtok((char*)line.c_str(), " ");
                        atof(strtok(nullptr, " "));
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
