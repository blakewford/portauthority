#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <deque>

#ifdef __linux__
#include <libelf.h>
#else
#include <sys/types.h>
#include <sys/ptrace.h>
#define EM_ARM 40
#define EM_X86_64 62
#define EM_AVR 83
#define EM_AARCH64 183
#define EM_386 3
#endif

std::deque<uint64_t> gAddresses;
#include "parser.cpp"
#include "categoryAnalyzer.cpp"
#include "coverageAnalyzer.cpp"

#include <map>

#include <fstream>
#include <sstream>

#define NUM_ANALYZERS 3

int32_t cachedArgc = 0;
char argvStorage[1024];
char* cachedArgv[64];

int32_t subprocessCachedArgc = 0;
char subprocessArgvStorage[1024];
char* subprocessCachedArgv[64];

struct sectionInfo
{
    uint32_t type;
    uint32_t index;
    uint64_t address;
    uint64_t offset;
    uint64_t size;
    bool plt;
    bool text;
    bool symbols;
    bool debugLine;
    bool stringTable;
};

#include "disavr.cpp"
#include "disarm64.cpp"
#include "dumpbin.cpp"
#include "profile.cpp"

struct sections
{
    uint32_t numSections;
    sectionInfo* si;
};

#pragma pack(push)
#pragma pack(1)
struct lineInfoHeader
{
    uint32_t size;
    uint16_t version;
    uint32_t headerLength;
    uint8_t  minInstrLength;
    uint8_t  maxInstrLength;
    uint8_t  isStatement;
    int8_t   lineBase;
    uint8_t  lineRange;
    uint8_t  opBase;
    uint8_t  opLength[11];
};
#pragma pack(pop)

struct lineInfo
{
    char fileName[256];
    int32_t lineNumber;
};

std::map<uint64_t, lineInfo*> gAddressToLineTable;
#include "energyAnalyzer.cpp"
#include "native.cpp"
#include "gdb.cpp"

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

int32_t readULEB(uint8_t*& binary)
{
    int32_t shift = 0;
    int32_t value = 0;
    while(true)
    {
        uint8_t byte = *binary;
        value |= (byte & 0x7F) << shift;
        binary++;
        if((byte & 0x80) == 0)
            break;
        shift += 7;
    }

    return value;
}

void runLineNumberProgram(uint8_t*& binary, const sectionInfo& debugLine, const char* binaryPath)
{
    void* lineData = (binary + debugLine.offset);
    lineInfoHeader* lines = (lineInfoHeader*)lineData;
    char* includePaths = ((char*)lineData) + sizeof(lineInfoHeader);

    while(*includePaths != '\0')
    {
        while(*includePaths != '\0')
            includePaths++;
        //add include
        includePaths++;
    }
    int32_t value = 0;
    char* files = ++includePaths;
    uint8_t* values = (uint8_t*)files;
    while(*values != '\0')
    {
        values += strlen(files)+1;
        //directory
        value = readULEB(values);
        //modification time
        value = readULEB(values);
        //file size
        value = readULEB(values);
    }
    int32_t line = 1;
    uint8_t* previous;
    uint64_t lineAddress = 0;
    uint8_t* program = (uint8_t*)++values;
    uint8_t opcode = program[0];
 /*
    while(true)
    {
        uint64_t offset = 0;
        switch(opcode)
        {
            case 0:
               value = readULEB(++program);
               switch(readULEB(program))
               {
                   case 2:
                       lineAddress = *(uint64_t*)program;
                       break;
                   default:
                       assert(0);
               }
               program += (value-1);
               break;
           case 5:
               value = readULEB(++program);
               break;
           default:
               ++program;
               break;
       }
       opcode = program[0];
    }
*/
    char command[256];
    memset(command, '\0', 256);
    sprintf(command, "readelf --debug-dump=decodedline %s > /tmp/decodedline", binaryPath);
    int32_t error = system(command );

    std::string l;
    std::ifstream decodedLine;
    decodedLine.open("/tmp/decodedline");
    std::getline(decodedLine, l);
    std::getline(decodedLine, l);
    std::getline(decodedLine, l);
    std::getline(decodedLine, l);

    std::string token;
    while(std::getline(decodedLine, l))
    {
        stringstream stream(l);
        while(stream >> token)
        {
            lineInfo* pInfo = new lineInfo();
            memset(pInfo->fileName, '\0', 256);
            strcpy(pInfo->fileName, token.c_str());
            stream >> pInfo->lineNumber;
            stream >> token;
            lineAddress = (int32_t)strtol(token.c_str(), nullptr, 0);
            if(lineAddress != 0)
            {
                gAddressToLineTable.insert(std::make_pair(lineAddress, pInfo));
            }
        }
    }
}

int main(int argc, char** argv)
{
    setvbuf(stdout, nullptr, _IONBF, 0);

    cachedArgc = argc;
    char* storagePointer = argvStorage;
    while(argc--)
    {
        cachedArgv[argc] = storagePointer;
        int32_t length = strlen(argv[argc]);
        strcat(storagePointer, argv[argc]);
        storagePointer+=(length+1);
    }

    loadProfiles();

    isa* instructionSet = nullptr;

    const char* binaryPath;
    const char* replayPath;

    bool dump = false;
    bool replay = false;
    bool execute = false;
    bool createReport = false;
    bool statistics = false;

    int32_t runtimeBias = 0;
    const char* breakFunction = "";
    const char* endFunction = "";
    uint64_t breakAddress = 0;
    uint64_t endAddress = 0;

    char* subprocessStoragePointer = subprocessArgvStorage;
    int32_t arg = cachedArgc;
    while(arg--)
    {
        if(!strcmp(cachedArgv[arg], "--report"))
        {
            createReport = true;
        }
        else if(!strcmp(cachedArgv[arg], "--statistics"))
        {
            statistics = true;
        }        
        else if(!strcmp(cachedArgv[arg], "--dumpbin"))
        {
            dump = true;
        }
        else if(!strcmp(cachedArgv[arg], "--replay"))
        {
            replay = true;
            replayPath = cachedArgv[arg+1];
        }
        else if(!strcmp(cachedArgv[arg], "--execute"))
        {
            execute = true;
            binaryPath = cachedArgv[arg+1];
        }
        else if(!strcmp(cachedArgv[arg], "--break"))
        {
            breakFunction = cachedArgv[arg+1];
        }
        else if(!strcmp(cachedArgv[arg], "--end"))
        {
            endFunction = cachedArgv[arg+1];
        }
        else if(!strcmp(cachedArgv[arg], "--break-at-address"))
        {
            breakAddress = strtol(cachedArgv[arg+1], NULL, 16);
        }
        else if(!strcmp(cachedArgv[arg], "--end-at-address"))
        {
            endAddress = strtol(cachedArgv[arg+1], NULL, 16);
        }
        else if(!strcmp(cachedArgv[arg], "--bias"))
        {
            runtimeBias = atoi(cachedArgv[arg+1]);
        }
        else if(!strcmp(cachedArgv[arg], "--arg"))
        {
            subprocessCachedArgv[subprocessCachedArgc] = subprocessStoragePointer;
            int32_t length = strlen(cachedArgv[arg+1]);
            strcat(subprocessStoragePointer, cachedArgv[arg+1]);
            subprocessStoragePointer+=(length+1);
            subprocessCachedArgc++;
        }
    }

    if(execute)
    {
        subprocessCachedArgv[subprocessCachedArgc] = subprocessStoragePointer;
        int32_t length = strlen(binaryPath);
        strcat(subprocessStoragePointer, binaryPath);
        subprocessStoragePointer+=(length+1);
        subprocessCachedArgc++;

        int32_t i = 0;
        int32_t j = subprocessCachedArgc-1;
        while(i < j)
        {
            auto temp = subprocessCachedArgv[i];
            subprocessCachedArgv[i] = subprocessCachedArgv[j];
            subprocessCachedArgv[j] = temp;
            i++;
            j--;
        }
    }

    if((!replay && !execute) || (replay && execute))
    {
        printf("Either --replay or --execute argument required!\n");
        return -1;
    }

    if(dump && !execute)
    {
        printf("Using --dumpbin requires --execute argument!\n");
        return -1;
    }

    bool arch64 = false;
    bool useGdb = false;
    uint8_t machine = 0;
    uint64_t pltSize = 0;
    uint64_t pltStart = 0;
    uint64_t textSize = 0;
    uint64_t textStart = 0;
    uint64_t moduleBound = 0;
    uint64_t profilerAddress = 0;
    uint64_t exitAddress = 0;

    FILE* executable = nullptr;
    if(replay)
    {
        machine = EM_AARCH64;
        textSize = ~0;
    }
    else
    {
        executable = fopen(binaryPath, "r");
        if(executable)
        {
            fseek(executable, 0, SEEK_END);
            int32_t size = ftell(executable);
            rewind(executable);
            uint8_t* binary = (uint8_t*)malloc(size);
            size_t read = fread(binary, 1, size, executable);
            if(read != size) return -1;
    
            arch64 = binary[4] == 0x2;
            uint64_t offset = 0;
            uint16_t headerSize = 0;
            uint16_t numHeaders = 0;
            uint64_t entryAddress = 0;
            uint16_t stringsIndex = 0;

#ifdef __linux__
            if(arch64)
            {
                Elf64_Ehdr* header = (Elf64_Ehdr*)binary;
                headerSize = header->e_shentsize;
                numHeaders = header->e_shnum;
                offset = header->e_shoff;
                stringsIndex = header->e_shstrndx;
                machine = header->e_machine;
                entryAddress = header->e_entry;
            }
            else
            {
                Elf32_Ehdr* header = (Elf32_Ehdr*)binary;
                headerSize = header->e_shentsize;
                numHeaders = header->e_shnum;
                offset = header->e_shoff;
                stringsIndex = header->e_shstrndx;
                machine = header->e_machine;
                entryAddress = header->e_entry;
            }
#else
            machine = EM_AARCH64;
#endif

            useGdb = machine == EM_AVR || machine == EM_ARM;
            if(breakFunction == "" && breakAddress == 0)
            {
                const char* warning = "\e[93mUsing default entry point\e[0m\n";
                fwrite(warning, strlen(warning), 1, stderr);
    
                breakFunction =  machine == EM_AVR ? "__vectors": "main";
            }
            if(endFunction == "" && endAddress == 0)
            {
                const char* warning = "\e[93mUsing default exit point\e[0m\n";
                fwrite(warning, strlen(warning), 1, stderr);
    
                endFunction =  machine == EM_AVR ? "__stop_program": "_fini";
            }

            sections sect;
            int32_t ndx = 0;
            const int16_t totalHeaders = numHeaders;
            sect.si = (sectionInfo*)malloc(sizeof(sectionInfo)*numHeaders);
            while(numHeaders--)
            {
#ifdef __linux__
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
#endif    
                offset += headerSize;
                ndx++;
            }

            int32_t pltIndex = 0;
            int32_t textIndex = 0;
            int32_t symbolsIndex = 0;
            int32_t debugLineIndex = 0;
            int32_t stringTableIndex = 0;
    
            ndx = 0;
            numHeaders = totalHeaders;
 
#ifdef __linux__
            int32_t init        = getIndexForString(binary, sect.si[stringsIndex], ".init");
            int32_t text        = getIndexForString(binary, sect.si[stringsIndex], ".text");
            int32_t debugLine   = getIndexForString(binary, sect.si[stringsIndex], ".debug_line");
            int32_t symbolTable = getIndexForString(binary, sect.si[stringsIndex], ".symtab");
            int32_t stringTable = getIndexForString(binary, sect.si[stringsIndex], ".strtab");
#else
            int32_t init        = 0;
            int32_t text        = 0;
            int32_t debugLine   = 0;
            int32_t symbolTable = 0;
            int32_t stringTable = 0;
#endif

            while(numHeaders--)
            {
                if(sect.si[ndx].index == init)
                {
                    pltIndex = ndx+1;
                    sect.si[pltIndex].plt = true;
                }
                sect.si[ndx].text        = sect.si[ndx].index == text;
                sect.si[ndx].debugLine   = sect.si[ndx].index == debugLine;
                sect.si[ndx].symbols     = sect.si[ndx].index == symbolTable;
                sect.si[ndx].stringTable = sect.si[ndx].index == stringTable;
                if(sect.si[ndx].symbols)
                    symbolsIndex = ndx;
                if(sect.si[ndx].stringTable)
                    stringTableIndex = ndx;
                if(sect.si[ndx].debugLine)
                    debugLineIndex = ndx;
                if(sect.si[ndx].text)
                    textIndex = ndx;
                ndx++;
            }

            if(dump)
            {
                dumpbin(binary, arch64, machine, entryAddress, &sect.si[textIndex], gAddresses);
            }

#ifdef __linux__
            pltSize = sect.si[pltIndex].size;
            pltStart = sect.si[pltIndex].address;
            textSize = sect.si[textIndex].size;
            textStart = sect.si[textIndex].address;
            profilerAddress = textStart; //reasonable default
            runLineNumberProgram(binary, sect.si[debugLineIndex], binaryPath);
#endif

            ndx = 0;

            uint8_t type = 0;
            uint32_t name = 0;
            uint64_t symbolSize = 0;
            uint64_t address = 0;
            uint64_t highestAddress = 0;

#ifdef __linux__
            int32_t symbols = sect.si[symbolsIndex].size / (headerSize == sizeof(Elf64_Shdr) ? sizeof(Elf64_Sym): sizeof(Elf32_Sym));  
#else
            int32_t symbols = 0;
#endif
            char buffer[256];
            while(symbols--)
            {
#ifdef __linux__
                if(headerSize == sizeof(Elf64_Shdr))
                {
                    Elf64_Sym* symbols = (Elf64_Sym*)(binary + sect.si[symbolsIndex].offset);
                    type = ELF64_ST_TYPE(symbols[ndx].st_info);
                    name = symbols[ndx].st_name;
                    symbolSize = symbols[ndx].st_size;
                    address = symbols[ndx].st_value;
                }
                else
                {
                    Elf32_Sym* symbols = (Elf32_Sym*)(binary + sect.si[symbolsIndex].offset);
                    type = ELF32_ST_TYPE(symbols[ndx].st_info);
                    name = symbols[ndx].st_name;
                    symbolSize = symbols[ndx].st_size;
                    address = symbols[ndx].st_value;
                } 
#endif
                if(type == 2) //function
                {
                    highestAddress = highestAddress < address ? address: highestAddress;
                    highestAddress += symbolSize;
                    getStringForIndex(binary, sect.si[stringTableIndex], name, buffer, 256);
                    if(breakAddress == 0 && !strcmp(breakFunction, buffer))
                    {
                        profilerAddress = address;
                    }
                    if(endAddress == 0 && !strcmp(endFunction, buffer))
                    {
                        exitAddress = address;
                    }
                }
                ndx++;
            }

            if(breakAddress != 0) profilerAddress = breakAddress;
            if(endAddress != 0) exitAddress = endAddress;
    
            if(highestAddress == 0) highestAddress = ~0;
            moduleBound = highestAddress;

#ifdef __linux__
            free(sect.si);
#endif
            free(binary);
        }
    }

    char* json = nullptr;
    FILE* library = nullptr;
    if(machine == EM_AVR)
    {
        library = fopen("avr.json", "r");
        instructionSet = new avr_isa();
    }
    else if(machine == EM_ARM)
    {
    }
    else if(machine == EM_AARCH64)
    {
        library = fopen("aarch64.json", "r");
        instructionSet = new aarch64_isa();
    }
    else
    {
        library = fopen("x86.json", "r");
        instructionSet = new x86_isa();
    }
    if(library)
    {
        fseek(library, 0, SEEK_END);
        int32_t size = ftell(library);
        rewind(library);
        json = (char*)malloc(size);
        size_t read = fread(json, 1, size, library);
        if(read != size) return -1;
    }
    parse(json, instructionSet);
    free(json);

    energyAnalyzer energy;
    categoryAnalyzer division;
    coverageAnalyzer coverage(textStart, textSize);

    analyzer* analyzers[NUM_ANALYZERS];
    analyzers[0] = &energy;
    analyzers[1] = &division;
    analyzers[2] = &coverage;

    //profilerAddress = 0x8049a6d;

//    printf("{\"triple\":\"x86_64-pc-linux-gnu\",\"size\":970668,\"run\":[");

    const char* reportPath;
    uint32_t instructionCount = 0;
    if(execute)
    {
        reportPath = binaryPath;
        coverage.adjustCount(runtimeBias);
        if(useGdb)
        {
            instructionCount = profileGdb(binaryPath, machine, profilerAddress, moduleBound, exitAddress, instructionSet, analyzers);
        }
        else
        {
            instructionCount = profileNative(binaryPath, profilerAddress, moduleBound, exitAddress, pltStart, pltStart + pltSize, (normal*)instructionSet, analyzers);
        }
        instructionCount += runtimeBias;
    }
    else if(replay)
    {
        reportPath = replayPath;
        std::string empty;
        std::string addressStr;
        std::string instructionStr;
    
        std::ifstream replay;
        replay.open(replayPath);
        while(!replay.eof())
        {
            replay >> addressStr;
            replay >> empty;
            replay >> instructionStr;
    
            uint64_t instructionAddress = strtol(addressStr.c_str(), nullptr, 16);
            if(machine == EM_AARCH64)
            {
                instructionStr = instructionStr.substr(0,10);
            }
            else
            {
                instructionStr = instructionStr.substr(0,16);
            }

            uint64_t instruction = strtol(instructionStr.c_str(), nullptr, 16);
            instruction = bswap_32(instruction);

            const int32_t size = 16;
            char mnem[size];
            uint8_t byte = disassemble(mnem, size, instruction, machine);
            long ndx = ((normal*)instructionSet)->find(mnem);
            if(ndx != -1)
            {
                uint8_t count = NUM_ANALYZERS;
                const isa_instr* instruction = ((normal*)instructionSet)->get_instr(ndx);
                isa_instr modified = *instruction;
                modified.m_size = byte;
                while(count--)
                {
                    analyzers[count]->analyze(instructionAddress, &modified);
                }
            }
        }
        replay.close();
    }

    printf("]}");

    delete instructionSet;
    instructionSet = nullptr;

    if(createReport)
    {
        analyzer::header(reportPath, !useGdb, machine, instructionCount, coverage.getCycleCount());
        energy.report();
        coverage.report();
        division.report();
        analyzer::footer();
    }
    else if(statistics)
    {
        printf(" %d", instructionCount);
        energy.statistics();
        coverage.statistics();
        division.statistics();
    }
    else
    {
        energy.console();
        division.console();
        coverage.console();
    }

    fclose(executable);
    unloadProfiles();

    return 0;
}
