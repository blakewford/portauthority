#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <byteswap.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define GDB_PORT 1234

char gLastPacket[64];
bool packetRead(int fd, uint32_t& value)
{
    value = 0;

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

    if(strstr(buffer, "T05") != nullptr)
    {
        std::string message(buffer);
        value = bswap_32(strtol(message.substr(message.find_last_of(":")+1).c_str(), nullptr, 16));
    }
    else if(strstr(buffer, "$") != nullptr && strstr(buffer, "#"))
    {
        std::string message(buffer);
        value = bswap_32(strtol(message.substr(1, message.length()-1).c_str(), nullptr, 16));
    }

    strcpy(gLastPacket, buffer);

    char checksum[2];
    err = read(fd, checksum, 2);

    return true;
}

bool packetWrite(int fd, uint32_t& address, const char* replay)
{
    ssize_t err = 0;
    err = write(fd, replay, strlen(replay));
    return packetRead(fd, address);
}

uint32_t profileGdb(const char* executable, uint64_t profilerAddress, uint64_t moduleBound, isa* arch, analyzer** analyzers, int32_t timeout)
{
    avr_isa& instructionSet = (avr_isa&)(*arch);

    int32_t fd;
    struct sockaddr_in socketAddress;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd == 0)
        return 0;

    socketAddress.sin_family = AF_INET;
    socketAddress.sin_addr.s_addr = INADDR_ANY;
    socketAddress.sin_port = htons(GDB_PORT);

    if(connect(fd, (struct sockaddr*)&socketAddress, sizeof(socketAddress)) == -1)
        return 0;

    uint32_t address = 0;
    //replay sniffed initialization packets
    packetWrite(fd, address, "+$qSupported:multiprocess+;swbreak+;hwbreak+;qRelocInsn+#c9");
    packetWrite(fd, address, "+$Hg0#df");
    packetWrite(fd, address, "+$qTStatus#49");
    packetWrite(fd, address, "+$?#3f");
    packetWrite(fd, address, "+$qfThreadInfo#bb");
    packetWrite(fd, address, "+$qL1160000000000000000#55");
    packetWrite(fd, address, "+$Hc-1#09");
    packetWrite(fd, address, "+$qC#b4");
    packetWrite(fd, address, "+$qAttached#8f");
    packetWrite(fd, address, "+$qOffsets#4b");
    packetWrite(fd, address, "+$qL1160000000000000000#55");
    packetWrite(fd, address, "+$qSymbol::#5b");

    uint32_t opcode = 0x0000; //NOP
    uint32_t instructionCount = 0;
    while(packetWrite(fd, address, "+$s#73"))
    {
        if(address < moduleBound)
        {
            uint8_t size = 16;
            char buffer[size];
            memset(buffer, '\0', size);
            sprintf(buffer, "%x", address);

            uint32_t checksum = 0;
            while(size--)
            {
                checksum += buffer[size];
            }
            checksum += 'm';
            checksum += ',';
            checksum += '4';

            //+$mce,4#95
            sprintf(buffer, "+$m%x,4#%x\n", address, checksum%256);
            packetWrite(fd, opcode, buffer);
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
                //printf("Not found %s 0x%x\n", decode(opcode), opcode);
            }
            instructionCount++;
        }
    }

    close(fd);
    return instructionCount;
}