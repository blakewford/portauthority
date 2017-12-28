#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <libelf.h>

int32_t cachedArgc = 0;
char argvStorage[1024];
char* cachedArgv[64];

struct sectionInfo
{
    uint32_t type;
    uint32_t index;
    uint64_t address;
    uint64_t offset;
    uint64_t size;
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

        printf("%d\n", getIndexForString(binary, sect.si[stringsIndex], ".symtab"));

        free(sect.si);
        free(binary);
    }

    printf("Clustering.\n");
    fclose(executable);

}
