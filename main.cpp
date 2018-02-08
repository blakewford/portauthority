#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <spawn.h>

#include <sys/user.h>
#include <sys/wait.h>
#include <sys/ptrace.h>

#include <libelf.h>
#include <byteswap.h>

int32_t cachedArgc = 0;
char argvStorage[1024];
char* cachedArgv[64];

#define BREAK 0xCC00000000000000 //x86 breakpoint instruction

struct sectionInfo
{
    uint32_t type;
    uint32_t index;
    uint64_t address;
    uint64_t offset;
    uint64_t size;
    bool symbols;
    bool stringTable;
};

struct sections
{
    uint32_t numSections;
    sectionInfo* si;
};

int32_t getIndexForString(uint8_t* binary, sectionInfo& info, const char* search)
{
    char stringBuffer[info.size];
    memcpy(stringBuffer, &binary[info.offset], info.size);

    char nameBuffer[64];
    memset(nameBuffer, '\0', 64);

    int32_t ndx = 0;
    int32_t cursor = 0;
    uint32_t length = info.size;
    while(length--)
    {
        nameBuffer[cursor] = stringBuffer[ndx];
        if(nameBuffer[cursor] == '\0')
        {
            if(!strcmp(nameBuffer, search))
                return ndx - strlen(nameBuffer);
            memset(nameBuffer, '\0', 64);
            cursor = 0;
        }
        else
        {
            cursor++;
        }
        ndx++;
    }

    return -1;
}

void getStringForIndex(uint8_t* binary, sectionInfo& info, int32_t index, char* buffer, int32_t bufferSize)
{
    char stringBuffer[info.size];
    memcpy(stringBuffer, &binary[info.offset], info.size);

    memset(buffer, '\0', bufferSize);

    int32_t cursor = 0;
    while(stringBuffer[index + cursor] != '\0')
    {
        buffer[cursor] = stringBuffer[index + cursor];
        cursor++;
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

    uint64_t moduleBound = 0;
    uint64_t profilerAddress = 0;
    const char* FUNCTION_NAME = "main";

    FILE* executable = 0;
    if(cachedArgc > 1) executable = fopen(cachedArgv[1], "r");
    if(executable)
    {
        fseek(executable, 0, SEEK_END);
        int32_t size = ftell(executable);
        rewind(executable);
        uint8_t* binary = (uint8_t*)malloc(size);
        size_t read = fread(binary, 1, size, executable);
        if(read != size) return -1;

        uint64_t offset = 0;
        uint16_t headerSize = 0;
        uint16_t numHeaders = 0;
        uint16_t stringsIndex = 0;
        if(binary[4] == 0x2)
        {
            Elf64_Ehdr* header = (Elf64_Ehdr*)binary;
            headerSize = header->e_shentsize;
            numHeaders = header->e_shnum;
            offset = header->e_shoff;
            stringsIndex = header->e_shstrndx;
        }
        else
        {
            Elf32_Ehdr* header = (Elf32_Ehdr*)binary;
            headerSize = header->e_shentsize;
            numHeaders = header->e_shnum;
            offset = header->e_shoff;
            stringsIndex = header->e_shstrndx;
        }

        sections sect;
        int32_t ndx = 0;
        const int16_t totalHeaders = numHeaders;
        sect.si = (sectionInfo*)malloc(sizeof(sectionInfo)*numHeaders);
        while(numHeaders--)
        {
            if(headerSize == sizeof(Elf64_Shdr))
            {
                Elf64_Shdr* section = (Elf64_Shdr*)(binary + offset);
                sect.si[ndx].index = section->sh_name;
                sect.si[ndx].address = section->sh_addr;
                sect.si[ndx].type = section->sh_type;
                sect.si[ndx].offset = section->sh_offset;
                sect.si[ndx].size = section->sh_size;
            }
            else
            {
                Elf32_Shdr* section = (Elf32_Shdr*)(binary + offset);
                sect.si[ndx].index = section->sh_name;
                sect.si[ndx].address = section->sh_addr;
                sect.si[ndx].type = section->sh_type;
                sect.si[ndx].offset = section->sh_offset;
                sect.si[ndx].size = section->sh_size;
            }

            offset += headerSize;
            ndx++;
        }

        int32_t symbolsIndex = 0;
        int32_t stringTableIndex = 0;

        ndx = 0;
        numHeaders = totalHeaders;
        int32_t symbolTable = getIndexForString(binary, sect.si[stringsIndex], ".symtab");
        int32_t stringTable = getIndexForString(binary, sect.si[stringsIndex], ".strtab");
        while(numHeaders--)
        {
            sect.si[ndx].symbols = sect.si[ndx].index == symbolTable;
            sect.si[ndx].stringTable = sect.si[ndx].index == stringTable;
            if(sect.si[ndx].symbols)
                symbolsIndex = ndx;
            if(sect.si[ndx].stringTable)
                stringTableIndex = ndx;
            ndx++;
        }

        ndx = 0;

        uint8_t type = 0;
        uint32_t name = 0;
        uint64_t address = 0;
        uint64_t highestAddress = 0;
        int32_t symbols = sect.si[symbolsIndex].size / (headerSize == sizeof(Elf64_Shdr) ? sizeof(Elf64_Sym): sizeof(Elf32_Sym));

        char buffer[256];
        while(symbols--)
        {
            if(headerSize == sizeof(Elf64_Shdr))
            {
                Elf64_Sym* symbols = (Elf64_Sym*)(binary + sect.si[symbolsIndex].offset);
                type = ELF64_ST_TYPE(symbols[ndx].st_info);
                name = symbols[ndx].st_name;
                address = symbols[ndx].st_value;
            }
            else
            {
                Elf32_Sym* symbols = (Elf32_Sym*)(binary + sect.si[symbolsIndex].offset);
                type = ELF32_ST_TYPE(symbols[ndx].st_info);
                name = symbols[ndx].st_name;
                address = symbols[ndx].st_value;
            }

            if(type == 2) //function
            {
                highestAddress = highestAddress < address ? address: highestAddress;
                getStringForIndex(binary, sect.si[stringTableIndex], name, buffer, 256);
                if(!strcmp(FUNCTION_NAME, buffer))
                {
                    profilerAddress = address;
                }
            }
            ndx++;
        }

        moduleBound = highestAddress;

        free(sect.si);
        free(binary);
    }

    if(profilerAddress != 0)
    {
        int32_t status = 0;
        user_regs_struct registers;

        pid_t pid;
        posix_spawn(&pid, cachedArgv[1], NULL, NULL, NULL, NULL);

        ptrace(PTRACE_ATTACH, pid, NULL, NULL);
        waitpid(pid, &status, WSTOPPED);

        profilerAddress -= (sizeof(uint64_t));
        profilerAddress++;
        long data = ptrace(PTRACE_PEEKDATA, pid, profilerAddress, NULL);

        //set our starting breakpoint
        ptrace(PTRACE_POKEDATA, pid, profilerAddress, (data&0x00FFFFFFFFFFFFFF) | BREAK);
        ptrace(PTRACE_CONT, pid, NULL, NULL);

        //_start causes the process to stop
        waitpid(pid, &status, WSTOPPED);

        //run to break
        ptrace(PTRACE_CONT, pid, NULL, NULL);
        waitpid(pid, &status, WSTOPPED);

        //replace instruction
        ptrace(PTRACE_POKEDATA, pid, profilerAddress, data);

        while(WIFSTOPPED(status))
        {
            ptrace(PTRACE_SINGLESTEP, pid, NULL, NULL);
            waitpid(pid, &status, WSTOPPED);

            ptrace(PTRACE_GETREGS, pid, NULL, &registers);
            if(registers.rip == 0)
            {
                //natural program termination
                break;
            }

            if(registers.rip < moduleBound)
            {
                uint64_t value = ptrace(PTRACE_PEEKDATA, pid, registers.rip, NULL);
                printf("%llx ", registers.rip);
                int32_t bytes = 7; //max instruction length
                while(bytes--)
                {
                    printf("%02lx ", value&0xFF);
                    value >>= 8;
                }
                printf("\n");
            }
        }
        kill(pid, SIGKILL);
    }

    printf("Clustering.\n");
    fclose(executable);

}
