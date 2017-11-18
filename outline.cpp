#include <stdio.h>
#include <ctype.h>
#include <cstring>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <algorithm>
#include <pthread.h>

#include <map>
#include <string>
#include <vector>
#include "rapidxml/rapidxml.hpp"

int32_t cachedArgc = 0;
char argvStorage[1024];
char* cachedArgv[64];

typedef std::pair<uint64_t, uint64_t> Range;
typedef std::pair<std::string, std::string> Groups;

std::vector<Range> gRanges;
std::map<uint64_t, std::string> gSymbols;
std::map<uint64_t, std::string> gDisassembly;
std::map<std::string, Groups> gGroups;

bool gTimeout = false;

#ifndef SIMAVR
const char* gCategories[] =
{"datamov", "stack", "conver", "arith", "binary", "decimal", "logical", "shftrot", "bit", "branch", "cond", "break", "string", "inout", "flgctrl", "segreg", "control"};
#else
const char* gCategories[] =
{"datamov", "arith", "logical", "bit", "branch", "control"};
#endif

int32_t gCategoryCount[sizeof(gCategories)/sizeof(const char*)];

void* startTimer(void*)
{
    int count = cachedArgc > 2 ? atoi(cachedArgv[2]): 60;
    while(count--)
    {
        usleep(1000*1000);
    }

    gTimeout = true;
}

void* runRemoteGDB(void*)
{
    int error = system("~/simavr/simavr/run_avr -f 16000000 -m atmega1284 /tmp/stripped --gdb");
}

void* runProgram(void*)
{
#ifndef SIMAVR
    int error = system("( cat ) | gdb /tmp/stripped -ex \"break main\" -ex \"run\"");
#else
    int error = system("( cat ) | avr-gdb /tmp/stripped -ex \"target remote :1234\" -ex \"break main\" -ex \"c\""); //TODO: Start from arbitrary symbol
#endif
}

void updateCategoryCount(const char* category)
{
    int32_t count = sizeof(gCategories)/sizeof(const char*);
    while(count--)
    {
        if(!strcmp(category, gCategories[count]))
        {
            gCategoryCount[count]++;
            break;
        }
    }
}

int main(int argc, char** argv)
{

    cachedArgc = argc;
    char* storagePointer = argvStorage;
    while(argc--)
    {
        cachedArgv[argc] = storagePointer;
        int32_t length = strlen(argv[argc]);
        strcat(storagePointer, argv[argc]);
        storagePointer+=(length+1);
    }

    char buffer[1024];
    memset(buffer, '\0', 1024);
#ifndef SIMAVR
    sprintf(buffer, "objcopy -g -Ielf64-x86-64 -Oelf64-x86-64 %s /tmp/stripped", argv[1]);
#else
    sprintf(buffer, "avr-objcopy -g -Ielf32-avr -Oelf32-avr %s /tmp/stripped", argv[1]);
#endif

    int error = system(buffer);

#ifndef SIMAVR
    const char* prefix = "";
#else
    const char* prefix = "avr-";
#endif

    memset(buffer, '\0', 1024);
    sprintf(buffer, "%sreadelf --debug-dump=frames %s > /tmp/frames", prefix, argv[1]);
    error = system(buffer);
    memset(buffer, '\0', 1024);
    sprintf(buffer, "%sreadelf -s %s > /tmp/symbols", prefix, argv[1]);
    error = system(buffer);
    memset(buffer, '\0', 1024);
    sprintf(buffer, "%sobjdump -d %s > /tmp/disasm", prefix, argv[1]);
    error = system(buffer);
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
                gRanges.push_back(Range(strtol(temp, NULL, 16), strtol(strchr(range, '.')+2, NULL, 16)));
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
                gSymbols[address] = line+59;
            }
        }
    }

    fclose(input);
    input = fopen("/tmp/disasm", "r");

    while (fgets(line, sizeof(line), input))
    {
        if(isspace(line[0]) && strlen(line) > 1)
        {
            int8_t count;
#ifndef SIMAVR
            count = 6;
#else
            count = 4;
#endif
            char temp[count];
            memset(temp, '\0', count);
            char* mnem;
#ifndef SIMAVR
            mnem = line+32;
#else
            int padding = 0;
            if(highestAddress >= 4096)
            {
                padding += 2;
            }
            if(highestAddress >= 65536)
            {
                padding += 2;
            }
            mnem = line+19+padding;
#endif
            int8_t i = 0;
            while(count--)
            {
                char c = *(mnem+i);
                if(!isspace(c))
                    temp[i] = c;
                i++;
            }
            long address = strtol(line, NULL, 16);
            if(address != 0)
            {
                gDisassembly[address] = temp;
            }
        }
    }

#ifdef SIMAVR
    pthread_t remoteThread;
    pthread_create(&remoteThread, NULL, runRemoteGDB, NULL);

    usleep( 3000 );
#endif

    pthread_t programThread;
    pthread_create(&programThread, NULL, runProgram, NULL);

    pid_t pid = 0;
    int32_t rounds = 0;
    while(pid == 0)
    {
        char pidString[6];
        memset(pidString, '\0', 6);
        FILE *cmd;
        if(rounds % 2 == 0)
        {
            cmd = popen("pidof gdb", "r");
        }
        else
        {
            cmd = popen("pidof avr-gdb", "r");
        }
        rounds++;
        char* value = fgets(pidString, 6, cmd);
        pid = strtoul(pidString, NULL, 10);
        pclose(cmd);
        usleep(0);
    }

    memset(buffer, '\0', 1024);
    sprintf(buffer, "echo set confirm off > /proc/%d/fd/0", pid);
    error = system(buffer);

    memset(buffer, '\0', 1024);
    sprintf(buffer, "echo set logging on > /proc/%d/fd/0", pid);
    error = system(buffer);

    memset(buffer, '\0', 1024);
    sprintf(buffer, "echo set logging redirect on > /proc/%d/fd/0", pid);
    error = system(buffer);

    sprintf(buffer, "echo si > /proc/%d/fd/0", pid);

    pthread_t timerThread;
    pthread_create(&timerThread, NULL, startTimer, NULL);
    while(!gTimeout)
    {
        error = system(buffer);
    }

    usleep(1*1000*1000);

    memset(buffer, '\0', 1024);
    sprintf(buffer, "echo quit > /proc/%d/fd/0", pid);
    error = system(buffer);

    printf("\n");

    fclose(input);

    char* binary = NULL;
    FILE* executable;
#ifndef SIMAVR
    executable = fopen("x86reference.xml","rb");
#else
    executable = fopen("avrReference.xml","rb");
#endif
    if(executable)
    {
        fseek(executable, 0, SEEK_END);
        int32_t size = ftell(executable);
        rewind(executable);
        binary = (char*)malloc(size);
        size_t read = fread(binary, 1, size, executable);
        if(read != size) return -1;
        fclose(executable);
    }

    rapidxml::xml_document<> doc;
    doc.parse<0>(binary);

    rapidxml::xml_node<>* root = doc.first_node()->first_node();

    rapidxml::xml_node<>* current = root->first_node();

    std::string mnem;
    std::string group1;
    std::string group2;
    std::string cycles;
    while(current != NULL)
    {
        rapidxml::xml_node<>* sub = current->first_node()->first_node();
        while(sub != NULL && strcmp(sub->name(), "syntax"))
            sub = sub->next_sibling();
        while(sub != NULL && strcmp(sub->name(), "mnem"))
            sub = sub->first_node();
        mnem = sub != NULL ? sub->value(): "";

        sub = current->first_node()->first_node();
        while(sub != NULL && strcmp(sub->name(), "grp1"))
            sub = sub->next_sibling();
        group1 = sub != NULL ? sub->value(): "";

        sub = current->first_node()->first_node();
        while(sub != NULL && strcmp(sub->name(), "grp2"))
            sub = sub->next_sibling();
        group2 = sub != NULL ? sub->value(): "";

#ifndef SIMAVR
        gGroups[mnem] = std::pair<std::string, std::string>(group1, group2);
#else
        sub = current->first_node()->first_node();
        while(sub != NULL && strcmp(sub->name(), "cycles"))
            sub = sub->next_sibling();
        cycles = sub != NULL ? sub->value(): "1";
        gGroups[mnem] = std::pair<std::string, std::string>(cycles, group2);
#endif

        current = current->next_sibling();
    }

#ifndef SIMAVR
    root = doc.first_node()->first_node()->next_sibling();
    current = current = root->first_node();
    while(current != NULL)
    {
        rapidxml::xml_node<>* sub = current->first_node()->first_node();
        while(sub != NULL && strcmp(sub->name(), "syntax"))
            sub = sub->next_sibling();
        while(sub != NULL && strcmp(sub->name(), "mnem"))
            sub = sub->first_node();
        mnem = sub != NULL ? sub->value(): "";

        sub = current->first_node()->first_node();
        while(sub != NULL && strcmp(sub->name(), "grp1"))
            sub = sub->next_sibling();
        group1 = sub != NULL ? sub->value(): "";

        sub = current->first_node()->first_node();
        while(sub != NULL && strcmp(sub->name(), "grp2"))
            sub = sub->next_sibling();
        group2 = sub != NULL ? sub->value(): "";

        auto value = gGroups.find(mnem);
        if(value == gGroups.end())
            gGroups[mnem] = std::pair<std::string, std::string>(group1, group2);

        current = current->next_sibling();
    }
#endif
    free(binary);

    input = fopen("gdb.txt", "r");

    int cycleCount = 0;
    int instructionCount = 0;
    while (fgets(line, sizeof(line), input))
    {
        if(strstr(line, "(gdb) 0x") != NULL)
        {
            long address = strtol(strstr(line, "0x"), NULL, 16);
            if(address < highestAddress)
            {
                std::string& str = gDisassembly[address];
                std::transform(str.begin(), str.end(),str.begin(), ::toupper);
                updateCategoryCount(gGroups[str].second.c_str());
                instructionCount++;
#ifdef SIMAVR
                int cycles = atoi(gGroups[str].first.c_str());
                cycleCount += cycles > 0 ? cycles: 1;
#endif
            }
        }
        if(strstr(line, "__stop_program") != NULL)
        {
            printf("AVR exit()\n");
            break;
        }
    }

    fclose(input);
    remove("gdb.txt");

    int count = sizeof(gCategoryCount)/sizeof(int32_t);
    while(count--)
    {
        //printf("%s ", gCategories[count]);
    }
    //printf("\n");

    count = sizeof(gCategoryCount)/sizeof(int32_t);
    while(count--)
    {
        printf("%d ", gCategoryCount[count]);
    }
    printf("\n");

    // printf("radius = 0.5;module dot(){sphere(radius);}translate([1,1,0]){dot();}\n");


#ifdef SIMAVR
    pid = 0;
    while(pid == 0)
    {
        char pidString[6];
        memset(pidString, '\0', 6);
        FILE *cmd = popen("pidof run_avr", "r");
        fgets(pidString, 6, cmd);
        pid = strtoul(pidString, NULL, 10);
        pclose(cmd);
        usleep(0);
    }

    kill( pid, SIGKILL );
#endif

    printf("Instructions %d, Cycles %d\n", instructionCount, cycleCount);

    return 0;
}
