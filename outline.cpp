#include <stdio.h>
#include <ctype.h>
#include <cstring>
#include <stdlib.h>
#include <unistd.h>
#include <algorithm>
#include <pthread.h>

void* runProgram(void*)
{
    system("( cat ) | gdb /tmp/stripped -ex \"break main\" -ex \"run\"");
}

int main(int argc, char** argv)
{
    char buffer[1024];
    memset(buffer, '\0', 1024);
    sprintf(buffer, "objcopy -g -Ielf64-x86-64 -Oelf64-x86-64 %s /tmp/stripped", argv[1]);
    system(buffer);
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
                //printf("%s ", temp);
                //printf("%s", strchr(range, '.')+2);
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
                //printf("%s", line+59);
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
            //printf("%s\n", temp);
        }
    }

    pthread_t programThread;
    pthread_create(&programThread, NULL, runProgram, NULL);

    pid_t pid = 0;
    while(pid == 0)
    {
        char pidString[6];
        FILE *cmd = popen("pidof stripped", "r");
        fgets(pidString, 6, cmd);
        pid = strtoul(pidString, NULL, 10);
        pclose(cmd);
        usleep(0);
    }

    memset(buffer, '\0', 1024);
    sprintf(buffer, "echo set confirm off > /proc/%d/fd/0", pid);
    system(buffer);

    memset(buffer, '\0', 1024);
    sprintf(buffer, "echo set logging on > /proc/%d/fd/0", pid);
    system(buffer);

    memset(buffer, '\0', 1024);
    sprintf(buffer, "echo set logging redirect on > /proc/%d/fd/0", pid);
    system(buffer);

    int32_t count = 10;
    sprintf(buffer, "echo si > /proc/%d/fd/0", pid);
    while(count--)
    {
        system(buffer);
    }

    usleep(1*1000*1000);

    memset(buffer, '\0', 1024);
    sprintf(buffer, "echo quit > /proc/%d/fd/0", pid);
    system(buffer);

    printf("\n");

    fclose(input);
    input = fopen("gdb.txt", "r");

    while (fgets(line, sizeof(line), input))
    {
        if(strstr(line, "(gdb) 0x") != NULL)
        {
            if(strtol(strstr(line, "0x"), NULL, 16) < highestAddress)
            {
                //printf("%s", line);
            }
        }
    }    
    
    fclose(input);
    remove("gdb.txt");

    return 0;
}
