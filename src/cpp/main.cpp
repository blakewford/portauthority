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
//#include <byteswap.h>

#include <udis86.h>
#include "parser.cpp"

int32_t cachedArgc = 0;
char argvStorage[1024];
char* cachedArgv[64];

#define BREAK 0xCC00000000000000 //x86 breakpoint instruction

const char* gCategories[] =
{
    "datamov",
    "arith",
    "logical",
    "bit",
    "branch",
    "control",
    "stack",
    "conver",
    "binary",
    "decimal",
    "shftrot",
    "cond",
    "break",
    "string",
    "inout",
    "flgctrl",
    "segreg"
};

uint64_t gCategoryCount[sizeof(gCategories)/sizeof(const char*)];

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

extern "C"
{
    void parse(const char* json, isa* instr);
}

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

void serve();

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

    char* json = NULL;
    FILE* x86 = fopen("x86.json", "r");
    if(x86)
    {
        fseek(x86, 0, SEEK_END);
        int32_t size = ftell(x86);
        rewind(x86);
        json = (char*)malloc(size);
        size_t read = fread(json, 1, size, x86);
        if(read != size) return -1;
    }

    x86_isa instructionSet;
    parse(json, &instructionSet);
    free(json);

    int32_t argument = 1;
    bool createReport = false;
    if(cachedArgc > 1 && !strcmp(cachedArgv[1], "--remote"))
    {
        serve();
        return 0;
    }

    if(cachedArgc > 1 && !strcmp(cachedArgv[1], "--temp"))
    {
        FILE* temp = fopen("/tmp/cluster-profile", "w");
        if(temp)
        {
            const char* response = "{}";
            fwrite(response, strlen(response), 1, temp);
            fclose(temp);
            argument++;
        }
    }

    if(cachedArgc > 1 && !strcmp(cachedArgv[1], "--report"))
    {
        createReport = true;
        argument++;
    }

    bool amd64 = false;
    uint64_t moduleBound = 0;
    uint64_t profilerAddress = 0;
    const char* FUNCTION_NAME = "main";

    FILE* executable = 0;
    if(cachedArgc > 1) executable = fopen(cachedArgv[argument], "r");
    if(executable)
    {
        fseek(executable, 0, SEEK_END);
        int32_t size = ftell(executable);
        rewind(executable);
        uint8_t* binary = (uint8_t*)malloc(size);
        size_t read = fread(binary, 1, size, executable);
        if(read != size) return -1;

        amd64 = binary[4] == 0x2;

        uint64_t offset = 0;
        uint16_t headerSize = 0;
        uint16_t numHeaders = 0;
        uint16_t stringsIndex = 0;
        if(amd64)
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
        posix_spawn(&pid, cachedArgv[argument], NULL, NULL, &cachedArgv[argument+1], NULL);

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

        const int32_t INSTRUCTION_LENGTH_MAX = 7;
        uint8_t instructions[INSTRUCTION_LENGTH_MAX];

        ud_t u;
        ud_init(&u);
        ud_set_mode(&u, amd64 ? 64: 32);
        ud_set_syntax(&u, UD_SYN_ATT);
        while(WIFSTOPPED(status))
        {
            ptrace(PTRACE_GETREGS, pid, NULL, &registers);
            if(registers.rip == 0)
            {
                //natural program termination
                break;
            }

            if(registers.rip < moduleBound)
            {
                uint64_t value = ptrace(PTRACE_PEEKDATA, pid, registers.rip, NULL);
                //printf("%llx ", registers.rip);
                for(int32_t i = 0; i < INSTRUCTION_LENGTH_MAX; i++)
                {
                    instructions[i] = value&0xFF;
                    value >>= 8;
                }

                int byte = 0;
                bool invalid = true;
                while(invalid && (byte <= INSTRUCTION_LENGTH_MAX))
                {
                    byte++;
                    ud_set_input_buffer(&u, instructions, byte);
                    ud_disassemble(&u);
                    invalid = strcmp(ud_insn_asm(&u), "invalid ") == 0;
                }
                char mnem[16];
                memset(mnem, '\0', 16);
                const char* disasm = ud_insn_asm(&u);

                byte = 0;
                char c = disasm[0];
                while(c != '\0' && c != ' ')
                {
                    mnem[byte++] = c;
                    c = disasm[byte];
                }
                long ndx = instructionSet.find(mnem);
                if(ndx != -1)
                {
                    //printf("%lX %s %s %s\n", instructionSet.get_opcode(ndx), mnem, instructionSet.get_group(ndx), instructionSet.get_subgroup(ndx));
                    int32_t count = sizeof(gCategories)/sizeof(const char*);
                    while(count--)
                    {
                        if(!strcmp(instructionSet.get_subgroup(ndx), gCategories[count]))
                        {
                            gCategoryCount[count]++;
                            break;
                        }
                    }
                }
                else
                {
                    //printf("Not found: %s\n", mnem);
                }
            }
            ptrace(PTRACE_SINGLESTEP, pid, NULL, NULL);
            waitpid(pid, &status, WSTOPPED);
        }
        kill(pid, SIGKILL);
    }

    uint64_t total = 0;
    int32_t count = sizeof(gCategories)/sizeof(const char*);
    while(count--)
    {
        total += gCategoryCount[count];
    }

    if(createReport)
    {
        printf("addRange(%.0f, \"red\");\n",     (gCategoryCount[0]/(double)total) * 100);
        printf("addRange(%.0f, \"orange\");\n",  (gCategoryCount[1]/(double)total) * 100);
        printf("addRange(%.0f, \"yellow\");\n",  (gCategoryCount[2]/(double)total) * 100);
        printf("addRange(%.0f, \"green\");\n",   (gCategoryCount[3]/(double)total) * 100);
        printf("addRange(%.0f, \"blue\");\n",    (gCategoryCount[4]/(double)total) * 100);
        printf("addRange(%.0f, \"indigo\");\n",  (gCategoryCount[5]/(double)total) * 100);
        printf("addRange(%.0f, \"violet\");\n",  (gCategoryCount[6]/(double)total) * 100);
        printf("addRange(%.0f, \"white\");\n",   (gCategoryCount[7]/(double)total) * 100);
        printf("addRange(%.0f, \"silver\");\n",  (gCategoryCount[8]/(double)total) * 100);
        printf("addRange(%.0f, \"gray\");\n",    (gCategoryCount[9]/(double)total) * 100);
        printf("addRange(%.0f, \"black\");\n",   (gCategoryCount[10]/(double)total) * 100);
        printf("addRange(%.0f, \"maroon\");\n",  (gCategoryCount[11]/(double)total) * 100);
        printf("addRange(%.0f, \"olive\");\n",   (gCategoryCount[12]/(double)total) * 100);
        printf("addRange(%.0f, \"lime\");\n",    (gCategoryCount[13]/(double)total) * 100);
        printf("addRange(%.0f, \"aqua\");\n",    (gCategoryCount[14]/(double)total) * 100);
        printf("addRange(%.0f, \"fuchsia\");\n", (gCategoryCount[15]/(double)total) * 100);
        printf("addRange(%.0f, \"purple\");\n",  (gCategoryCount[16]/(double)total) * 100);

        printf("\n");

        //key
        printf("<font color=\"red\">%s</font></br>\n",     gCategories[0]);
        printf("<font color=\"orange\">%s</font></br>\n",  gCategories[1]);
        printf("<font color=\"yellow\">%s</font></br>\n",  gCategories[2]);
        printf("<font color=\"green\">%s</font></br>\n",   gCategories[3]);
        printf("<font color=\"blue\">%s</font></br>\n",    gCategories[4]);
        printf("<font color=\"indigo\">%s</font></br>\n",  gCategories[5]);
        printf("<font color=\"violet\">%s</font></br>\n",  gCategories[6]);
        printf("<font color=\"white\">%s</font></br>\n",   gCategories[7]);
        printf("<font color=\"silver\">%s</font></br>\n",  gCategories[8]);
        printf("<font color=\"gray\">%s</font></br>\n",    gCategories[9]);
        printf("<font color=\"black\">%s</font></br>\n",   gCategories[10]);
        printf("<font color=\"maroon\">%s</font></br>\n",  gCategories[11]);
        printf("<font color=\"olive\">%s</font></br>\n",   gCategories[12]);
        printf("<font color=\"lime\">%s</font></br>\n",    gCategories[13]);
        printf("<font color=\"aqua\">%s</font></br>\n",    gCategories[14]);
        printf("<font color=\"fuchsia\">%s</font></br>\n", gCategories[15]);
        printf("<font color=\"purple\">%s</font></br>\n",  gCategories[16]);

    }
    else
    {
        printf("\e[1mmain.cpp:126:36: \e[95mwarning:\e[0m energy inefficiency detected\n"); //<-- VS Code output format
    }

    fclose(executable);

}

#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

#define GUI_PORT 0xBFF

void serve()
{
    int32_t fd, sock;
    struct sockaddr_in address;
      
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd == 0)
        return;

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( GUI_PORT );
      
    if(bind(fd, (struct sockaddr*)&address, sizeof(address)) == -1)
        return;

    if(listen(fd, 1) == -1)
        return;

    int32_t length = sizeof(address);
    if((sock = accept(fd, (struct sockaddr *)&address, (socklen_t*)&length)) == -1)
        return;

    char c;
    int argc;
    read(sock, &c, 1);
    argc = atoi(&c);
    cachedArgc = argc;

    char* storagePointer = argvStorage;
    while(argc--)
    {
        cachedArgv[argc] = storagePointer;
        int32_t length = read(sock, &cachedArgv[argc], 1024);
        storagePointer+=(length+1);
    }

    const char* response = "{}";
    write(sock, response, strlen(response));
    close(sock);
    close(fd);
}

