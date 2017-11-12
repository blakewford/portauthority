#include <stdio.h>
#include <ctype.h>
#include <cstring>
#include <stdlib.h>
#include <algorithm>

int main(int argc, char** argv)
{
    char buffer[1024];
    memset(buffer, '\0', 1024);
    sprintf(buffer, "readelf --debug-dump=frames %s > /tmp/frames", argv[1]);
    system(buffer);
    memset(buffer, '\0', 1024);
    sprintf(buffer, "readelf -s %s > /tmp/symbols", argv[1]);
    system(buffer);
    memset(buffer, '\0', 1024);
    sprintf(buffer, "objdump -d %s > /tmp/disasm", argv[1]);
    system(buffer);
    memset(buffer, '\0', 1024);
//output from a scripted run
    system("");
    memset(buffer, '\0', 1024);

    char line[256];
    FILE* input = fopen("/tmp/frames", "r");

    while(fgets(line, sizeof(line), input))
    {
        long value = strtol(line, NULL, 16);
        if(value > 15) //Hack
        {
            char* rangeStart = strstr(line, "pc");
            if(rangeStart != NULL)
            {
                char temp[17];
                char* range = strstr(rangeStart, "=");
                memcpy(temp, ++range, 16);
                temp[16] = '\0';
                printf("%s ", temp);
                printf("%s", strchr(range, '.')+2);
            }
        } 
    }

    fclose(input);
    input = fopen("/tmp/symbols", "r");

    long highestAddress = 0;
    while(fgets(line, sizeof(line), input))
    {
        if(strstr(line, " FUNC ") != NULL )
        {
            long address = strtol(strstr(line, ":")+1, NULL, 16);
            if(address != 0)
            {
                highestAddress = std::max(highestAddress, address);
                printf("%s", line+59);
            }
        } 
    }

    fclose(input);
    input = fopen("/tmp/disasm", "r");

    while (fgets(line, sizeof(line), input))
    {
        if(isspace(line[0]) && strlen(line) > 1)
        {
            char temp[6];
            memcpy(temp, line+32, 5);
            temp[5] = '\0';
            printf("%s\n", temp);
        }
    }
/*
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
*/
    return 0;
}
