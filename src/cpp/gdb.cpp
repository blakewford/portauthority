#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <byteswap.h>
#include <sys/socket.h>
#include <netinet/in.h>

bool gTimeout = false;
#define GDB_PORT 1234

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

char gLastPacket[64];
bool packetRead(int fd)
{
    char byte;
    uint8_t ndx = 0;
    char buffer[64];
    memset(buffer, '\0', 64);

    ssize_t err = 0;
    err = read(fd, &byte, 1); //ack
    while(byte != '#')
    {
        err = read(fd, &byte, 1);
        buffer[ndx++] = byte;
    }

    if(!strcmp(gLastPacket, buffer))
    {
        return false;
    }

    strcpy(gLastPacket, buffer);

    char checksum[2];
    err = read(fd, checksum, 2);

    return true;
}

bool packetWrite(int fd, const char* replay)
{
    ssize_t err = 0;
    err = write(fd, "+", 1); //ack
    err = write(fd, replay, strlen(replay));
    return packetRead(fd);
}

void singleStep()
{
    int32_t fd;
    struct sockaddr_in address;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd == 0)
        return;

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(GDB_PORT);

    if(connect(fd, (struct sockaddr*)&address, sizeof(address)) == -1)
        return;

    const char* replay = "";

    //replay sniffed initialization packets
    packetWrite(fd, "$qSupported:multiprocess+;swbreak+;hwbreak+;qRelocInsn+#c9");
    packetWrite(fd, "$Hg0#df");
    packetWrite(fd, "$qTStatus#49");
    packetWrite(fd, "$?#3f");
    packetWrite(fd, "$qfThreadInfo#bb");
    packetWrite(fd, "$qL1160000000000000000#55");
    packetWrite(fd, "$Hc-1#09");
    packetWrite(fd, "$qC#b4");
    packetWrite(fd, "$qAttached#8f");
    packetWrite(fd, "$qOffsets#4b");
    packetWrite(fd, "$qL1160000000000000000#55");
    packetWrite(fd, "$qSymbol::#5b");

    while(packetWrite(fd, "$s#73"))
        ;

    close(fd);
}
