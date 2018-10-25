#include <byteswap.h>

bool gTimeout = false;

void* startTimer(void* arg)
{
    int32_t count = *(int32_t*)arg;
    while(count--)
    {
        usleep(1000*1000);
    }

    gTimeout = true;
}

void* runProgram(void* argument)
{
    char buffer[256];
    memset(buffer, '\0', 256);
    sprintf(buffer, "( cat ) | avr-gdb %s -ex \"target remote :1234\" -ex \"break main\" -ex \"c\"", (const char*)argument);
    int error = system(buffer);
}

void profileGdb(const char* executable, uint64_t profilerAddress, uint64_t moduleBound, isa* arch, analyzer** analyzers, int32_t timeout)
{
    avr_isa& instructionSet = (avr_isa&)(*arch);

    pthread_t programThread;
    pthread_create(&programThread, NULL, runProgram, (char*)executable);

    char buffer[1024];
    char buffer2[1024];
    int32_t error = 0;
    int32_t rounds = 0;

    pid_t pid = 0;
    while(pid == 0)
    {
        FILE *cmd;
        char pidString[6];
        memset(pidString, '\0', 6);
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
    sprintf(buffer2, "echo 'x $pc' > /proc/%d/fd/0", pid);
    pthread_t timerThread;
    pthread_create(&timerThread, NULL, startTimer, &timeout);
    while(!gTimeout)
    {
        error = system(buffer);
        error = system(buffer2);
    }
    usleep(1*1000*1000);
    memset(buffer, '\0', 1024);
    sprintf(buffer, "echo quit > /proc/%d/fd/0", pid);
    error = system(buffer);

    char line[256];
    uint32_t opcode = 0x0000; //NOP
    uint32_t instructionCount = 0;
    FILE* log = fopen("gdb.txt", "r");
    while(fgets(line, sizeof(line), log))
    {
        if(strstr(line, "<") != NULL && strstr(line, ":") != NULL)
        {
            opcode = (uint32_t)strtol(strstr(line, ":")+1, NULL, 16);
        }
        else if(strstr(line, "(gdb) 0x") != NULL)
        {
            long address = strtol(strstr(line, "0x"), NULL, 16);
            if(address < moduleBound)
            {
                long ndx = instructionSet.find(decode(opcode));
                if(ndx != -1)
                {
                    uint8_t count = NUM_ANALYZERS;
                    const isa_instr* instruction = instructionSet.get_instr(ndx);
                    if(!strcmp(instruction->m_mnem, "break"))
                    {
                        printf("BREAK\n");
                        break;
                    }
                    while(count--)
                    {
                        analyzers[count]->analyze(address, instruction);
                    }
                }
                else
                {
                    printf("Not found %s 0x%x\n", decode(opcode), opcode);
                }
                opcode = 0x000; //NOP
                instructionCount++;
            }
        }
        if(strstr(line, "__stop_program") != NULL)
        {
            break;
        }
    }
    fclose(log);
    remove("gdb.txt");
    printf("%u\n", instructionCount);
}
