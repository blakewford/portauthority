#include <spawn.h>
#include <udis86.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/signal.h>
#include <byteswap.h>

#include "native.h"

#include <chrono>
using namespace std::chrono;

#ifdef __aarch64__
#define BREAK 0xD4200000 //aarch64 breakpoint instruction
#else
#define BREAK 0xCC //x86 breakpoint instruction
#endif

uint32_t profileNative(const char* executable, uint64_t profilerAddress, uint64_t moduleBound, uint64_t exitAddress, uint64_t pltStart, uint64_t pltEnd, normal* arch, analyzer** analyzers)
{
    uint64_t moduleLow = 0;
    uint64_t moduleHigh = 0;

    bool transition = false;
    microseconds untracked = microseconds{0};
    microseconds startTransition = duration_cast<microseconds>(high_resolution_clock::now().time_since_epoch());
    microseconds endTransition = startTransition;

    pid_t pid = 0;
    int32_t status = 0;
    user_regs_struct registers;

#ifdef __aarch64__
    iovec buffer;
    buffer.iov_base = &registers;
    buffer.iov_len = sizeof(registers);
    iovec* registerBuffer = &buffer;
#else
    user_regs_struct* registerBuffer = &registers;
#endif

    size_t size = 8;
    uint8_t binary[8];

    FILE* reopen = fopen(executable, "r");
    if(fread(binary, 1, size, reopen) != size) return 0;

    int machine = EM_AARCH64;
    bool arch64 = binary[4] == 0x2;
#ifndef __aarch64__
    if(arch64)
    {
        machine = EM_X86_64;
    }
    else
    {
        machine = EM_386;
    }
#endif

    spawnProcess(&pid, executable);

    microseconds startProfile = duration_cast<microseconds>(high_resolution_clock::now().time_since_epoch());

    ptrace(PTRACE_ATTACH, pid, NULL, NULL);
    waitpid(pid, &status, WSTOPPED);

    ptrace(PTRACE_CONT, pid, NULL, NULL);
    //_start causes the process to stop
    waitpid(pid, &status, WSTOPPED);

    long data = setBreakInstruction(pid, profilerAddress);

    //run to break
    ptrace(PTRACE_CONT, pid, NULL, NULL);
    waitpid(pid, &status, WSTOPPED);

    clearBreakInstruction(pid, profilerAddress, data);

    char modulesPath[32];
    sprintf(modulesPath,"/proc/%d/maps", pid);

    char* line = nullptr;
    FILE* modules = fopen(modulesPath, "r");
    if(modules)
    {
        size_t length = 0;
        while(getline(&line, &length, modules) != -1)
        {
            string module(line);
            if(module.find("libopencv_core") != -1)
            {
                char* token = strtok(line, " ");
                token = strtok(nullptr, " ");
                if(strchr(token, 'x') != nullptr) //executable permission
                {
                    char range[64];
                    token = strtok(line, " ");
                    strcpy(range, token);
                    range[strlen(token)] = '\0';
                    token = strtok(range, "-");
                    moduleLow = strtoll(token, nullptr, 16);
                    token = strtok(nullptr, "-");
                    moduleHigh = strtoll(token, nullptr, 16);
                    break;
                }
            }
        }
    }

    ud_t u;
    ud_init(&u);
    ud_set_syntax(&u, UD_SYN_ATT);
    ud_set_mode(&u, arch64 ? 64: 32);
    const int32_t INSTRUCTION_LENGTH_MAX = 7;
    uint8_t instructions[INSTRUCTION_LENGTH_MAX];

    uint64_t next = 0;
    bool fromBranch = false;
    uint32_t instructionCount = 0;
    while(WIFSTOPPED(status))
    {
        uint64_t instructionAddress = 0;
#if defined( __aarch64__) && defined(__linux__)
        ptrace(PTRACE_GETREGSET, pid, NT_PRSTATUS, registerBuffer);
        instructionAddress = registers.pc;
#elif defined (__linux__)
        ptrace(PTRACE_GETREGS, pid, NULL, registerBuffer);
        instructionAddress = registers.rip;
#else
        ptrace(PTRACE_GETREGS, pid, NULL, NULL); // struct regs
#endif
        if(instructionAddress == exitAddress)
        {
            //natural program termination
            break;
        }
        //need better protections here for code that does not exit cleanly, without exit()
        if(instructionAddress < moduleBound)
        {

#ifdef __linux__
            uint64_t value = ptrace(PTRACE_PEEKDATA, pid, instructionAddress, nullptr);
#else
            uint64_t value = ptrace(PTRACE_PEEKDATA, pid, (caddr_t)&instructionAddress, NULL);
#endif
            if(!transition)
            {
                if(startTransition != endTransition)
                {
                    endTransition = duration_cast<microseconds>(high_resolution_clock::now().time_since_epoch());
                    untracked += endTransition-startTransition;
                }
            }
            transition = true;

            if(shouldSkip(instructionAddress, next, value, pltStart, pltEnd))
            {
                uint64_t value = setBreakInstruction(pid, next);

                //run to break
                ptrace(PTRACE_CONT, pid, NULL, NULL);
                waitpid(pid, &status, WSTOPPED);

                clearBreakInstruction(pid, next, value);
            }

            const int32_t size = 16;
            char mnem[size];
            uint8_t byte = disassemble(mnem, size, value, machine);
            next = instructionAddress + byte;
            fromBranch = strstr("BLR", mnem) != nullptr;

            long ndx = arch->find(mnem);
            if(ndx != -1)
            {
//                printf("{\"address\":\"0x%llx\",\"opcode\":\"0x%llx\",\"mnem\":\"%s\"},", instructionAddress, bswap_32(value), mnem);

                uint8_t count = NUM_ANALYZERS;
                const isa_instr* instruction = arch->get_instr(ndx);
                isa_instr modified = *instruction;
                modified.m_size = byte;
                while(count--)
                {
                    analyzers[count]->analyze(instructionAddress, &modified);
                }
            }
            else
            {
                //printf("Not found: %s\n", mnem);
            }
            instructionCount++;
        }
        else
        {
            if(transition)
            {
                startTransition = duration_cast<microseconds>(high_resolution_clock::now().time_since_epoch());
            }
            transition = false;

            if(fromBranch)
            {
                uint64_t value = setBreakInstruction(pid, next);

                //run to break
                ptrace(PTRACE_CONT, pid, NULL, NULL);
                waitpid(pid, &status, WSTOPPED);

                clearBreakInstruction(pid, next, value);
            }

            if(instructionAddress >= moduleLow && instructionAddress <= moduleHigh)
            {
            }
        }

        ptrace(PTRACE_SINGLESTEP, pid, NULL, NULL);
        waitpid(pid, &status, WSTOPPED);
    }

#ifdef __linux__
    kill(pid, SIGKILL);
#endif

    microseconds endProfile = duration_cast<microseconds>(high_resolution_clock::now().time_since_epoch());
    //printf("Runtime (ms): %ld Untracked: %ld\n", duration_cast<milliseconds>(endProfile-startProfile).count(), duration_cast<milliseconds>(untracked).count());

    return instructionCount;
}
