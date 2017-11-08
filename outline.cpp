#include <stdio.h>
#include <ctype.h>
#include <cstring>
#include <stdlib.h>
#include <algorithm>

//readelf --debug-dump=frames
//readelf -s 
//objdump -d
//output from a scripted run
int main(int argc, char** argv)
{
    char line[256];
    FILE* input = fopen(argv[1], "r");

    while(fgets(line, sizeof(line), input))
    {
        long value = strtol(line, NULL, 16);
        if(value > 15) //Hack
        {
            char* rangeStart = strstr(line, "pc");
            if(rangeStart != NULL)
            {
                char* range = strstr(rangeStart, "=");
                printf("%s", ++range);
            }
        } 
    }

    fclose(input);
    input = fopen(argv[2], "r");

    long highestAddress = 0;
    while(fgets(line, sizeof(line), input))
    {
        if(strstr(line, " FUNC ") != NULL )
        {
            long address = strtol(strstr(line, ":")+1, NULL, 16);
            if(address != 0)
            {
                highestAddress = std::max(highestAddress, address);
                printf("%s", line);
            }
        } 
    }

    fclose(input);
    input = fopen(argv[3], "r");

    while (fgets(line, sizeof(line), input))
    {
        if(isspace(line[0]) && strlen(line) > 1)
        {
            printf("%s", line);
        }
    }

    fclose(input);
    input = fopen(argv[4], "r");

    while (fgets(line, sizeof(line), input))
    {
        if(strstr(line, "(gdb) 0x") != NULL)
        {
            if(strtol(strstr(line, "0x"), NULL, 16) < highestAddress)
            {
                printf("%s", line);
            }
        }
    }    
    
    fclose(input);
    return 0;
}
