#include <stdio.h>
#include <ctype.h>
#include <cstring>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <algorithm>
#include <pthread.h>

#include <map>
#include <string>
#include <vector>
#include "rapidxml/rapidxml.hpp"

typedef std::pair<uint64_t, uint64_t> Range;
typedef std::pair<std::string, std::string> Groups;

std::vector<Range> gRanges;
std::map<uint64_t, std::string> gSymbols;
std::map<uint64_t, std::string> gDisassembly;
std::map<std::string, Groups> gGroups;

const char* gCategories[] =
{"datamov", "stack", "conver", "arith", "binary", "decimal", "logical", "shftrot", "bit", "branch", "cond", "break", "string", "inout", "flgctrl", "segreg", "control"};

int32_t gCategoryCount[sizeof(gCategories)/sizeof(const char*)];

void* runProgram(void*)
{
    system("( cat ) | gdb /tmp/stripped -ex \"break main\" -ex \"run\"");
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
            int8_t count = 6;
            char temp[count];
            memset(temp, '\0', count);
            char* mnem = line+32;
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

    pthread_t programThread;
    pthread_create(&programThread, NULL, runProgram, NULL);

    pid_t pid = 0;
    while(pid == 0)
    {
        char pidString[6];
        memset(pidString, '\0', 6);
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

    char* binary = NULL;
    FILE* executable = fopen("x86reference.xml","rb");
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

        gGroups[mnem] = std::pair<std::string, std::string>(group1, group2);

        current = current->next_sibling();
    }

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

    free(binary);

    input = fopen("gdb.txt", "r");

    while (fgets(line, sizeof(line), input))
    {
        if(strstr(line, "(gdb) 0x") != NULL)
        {
            long address =strtol(strstr(line, "0x"), NULL, 16);
            if(address < highestAddress)
            {
                std::string& str = gDisassembly[address];
                std::transform(str.begin(), str.end(),str.begin(), ::toupper);
                updateCategoryCount(gGroups[str].second.c_str());
            }
        }
    }

    fclose(input);
    remove("gdb.txt");

    count = sizeof(gCategoryCount)/sizeof(int32_t);
    while(count--)
    {
        //printf("%d\n", gCategoryCount[count]);
    }

    return 0;
}
