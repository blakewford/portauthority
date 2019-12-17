#include <spawn.h>
#include <udis86.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/signal.h>

#include <chrono>
using namespace std::chrono;

#ifdef __aarch64__
#define BREAK 0xD4200000 //aarch64 breakpoint instruction
#else
#define BREAK 0xCC //x86 breakpoint instruction
#endif

uint32_t profileNative(const char* executable, uint64_t profilerAddress, uint64_t moduleBound, uint64_t exitAddress, uint64_t pltStart, uint64_t pltEnd, isa* arch, analyzer** analyzers)
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
    aarch64_isa& instructionSet = (aarch64_isa&)(*arch);

    iovec buffer;
    buffer.iov_base = &registers;
    buffer.iov_len = sizeof(registers);
    iovec* registerBuffer = &buffer;
#else
    x86_isa& instructionSet = (x86_isa&)(*arch);
    user_regs_struct* registerBuffer = &registers;
#endif

    size_t size = 8;
    uint8_t binary[8];

    FILE* reopen = fopen(executable, "r");
    if(fread(binary, 1, size, reopen) != size) return 0;

    bool arch64 = binary[4] == 0x2;

#ifdef __aarch64__
    int32_t arg = 0;
    std::string command;
    command += "./suspend.sh";
    while(arg < subprocessCachedArgc)
    {
        command += " ";
        command += subprocessCachedArgv[arg++];
    }

    FILE* process = popen(command.c_str(), "r");

    char pidStr[6];
    memset(pidStr, '\0', 6);
    char* cursor = pidStr;

    int fd = fileno(process);
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    ssize_t bytes = 0;
    while(bytes >= 0)
    {
        usleep(1000*100);
        bytes = read(fd, cursor, 1);
        cursor++;
    }

    pid = atoi(pidStr);
#else
    posix_spawn(&pid, executable, NULL, NULL, subprocessCachedArgv, NULL);
#endif

    microseconds startProfile = duration_cast<microseconds>(high_resolution_clock::now().time_since_epoch());

    ptrace(PTRACE_ATTACH, pid, NULL, NULL);
    waitpid(pid, &status, WSTOPPED);

#ifndef __aarch64__
    ud_t u;
    ud_init(&u);
    ud_set_mode(&u, arch64 ? 64: 32);
    ud_set_syntax(&u, UD_SYN_ATT);

    const int32_t INSTRUCTION_LENGTH_MAX = 7;
    uint8_t instructions[INSTRUCTION_LENGTH_MAX];

    uint64_t value = ptrace(PTRACE_PEEKDATA, pid, profilerAddress, NULL);

    memcpy(instructions, &value, INSTRUCTION_LENGTH_MAX);

    int byte = 0;
    bool invalid = true;
    while(invalid && (byte <= INSTRUCTION_LENGTH_MAX))
    {
        byte++;
        ud_set_input_buffer(&u, &instructions[byte-1], byte);
        ud_disassemble(&u);
        invalid = strcmp(ud_insn_asm(&u), "invalid ") == 0;
    }
#endif

    long data = ptrace(PTRACE_PEEKDATA, pid, profilerAddress, NULL);

    //set our starting breakpoint
#ifdef __aarch64__
    ptrace(PTRACE_POKEDATA, pid, profilerAddress, BREAK);
#else
    uint8_t bytes[sizeof(long)];
    memset(bytes, '\0', sizeof(long));
    bytes[byte-1] = BREAK;
    ptrace(PTRACE_POKEDATA, pid, profilerAddress, *(long*)bytes);
#endif

    ptrace(PTRACE_CONT, pid, NULL, NULL);
    //_start causes the process to stop
    waitpid(pid, &status, WSTOPPED);
    //run to break
    ptrace(PTRACE_CONT, pid, NULL, NULL);
    waitpid(pid, &status, WSTOPPED);

    //replace instruction
    ptrace(PTRACE_POKEDATA, pid, profilerAddress, data);

#ifndef __aarch64__
    ptrace(PTRACE_GETREGS, pid, NULL, registerBuffer);
    registerBuffer->rip = profilerAddress;
    ptrace(PTRACE_SETREGS, pid, NULL, registerBuffer);
#else
    ptrace(PTRACE_GETREGSET, pid, NT_PRSTATUS, registerBuffer);
    registers.pc = profilerAddress;
    ptrace(PTRACE_SETREGSET, pid, NULL, registerBuffer);
#endif

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

    uint64_t next = 0;
    bool fromBranch = false;
    uint32_t instructionCount = 0;
    while(WIFSTOPPED(status))
    {
        uint64_t instructionAddress = 0;
#ifdef __aarch64__
        ptrace(PTRACE_GETREGSET, pid, NT_PRSTATUS, registerBuffer);
        instructionAddress = registers.pc;
#else
        ptrace(PTRACE_GETREGS, pid, NULL, registerBuffer);
        instructionAddress = registers.rip;
#endif
        if(instructionAddress == exitAddress)
        {
            //natural program termination
            break;
        }
        //need better protections here for code that does not exit cleanly, without exit()
        if(instructionAddress < moduleBound)
        {
            uint64_t value = ptrace(PTRACE_PEEKDATA, pid, instructionAddress, nullptr);
            //printf("%llx\n", instructionAddress);
            if(!transition)
            {
                if(startTransition != endTransition)
                {
                    endTransition = duration_cast<microseconds>(high_resolution_clock::now().time_since_epoch());
                    untracked += endTransition-startTransition;
                }
            }
            transition = true;

            const char* test = arm64_decode((uint32_t)value);
            if(next != 0 && ((instructionAddress >= pltStart && instructionAddress <= pltEnd) || !strcmp(test, "LDAXR") || !strcmp(test, "STLXR")))
            {
                uint64_t value = ptrace(PTRACE_PEEKDATA, pid, next, nullptr);
#ifdef __aarch64__
                ptrace(PTRACE_POKEDATA, pid, next, BREAK);
#else
                memcpy(instructions, &value, INSTRUCTION_LENGTH_MAX);

                uint8_t bytes[sizeof(long)];
                memset(bytes, BREAK, sizeof(long));
                ptrace(PTRACE_POKEDATA, pid, next, *(long*)bytes);
#endif
                //run to break
                ptrace(PTRACE_CONT, pid, NULL, NULL);
                waitpid(pid, &status, WSTOPPED);

                //replace instruction
                ptrace(PTRACE_POKEDATA, pid, next, value);

#ifndef __aarch64__
                ptrace(PTRACE_GETREGS, pid, NULL, registerBuffer);
                registerBuffer->rip = next;
                ptrace(PTRACE_SETREGS, pid, NULL, registerBuffer);
#else
                ptrace(PTRACE_GETREGSET, pid, NT_PRSTATUS, registerBuffer);
                registers.pc = next;
                ptrace(PTRACE_SETREGSET, pid, NULL, registerBuffer);
#endif
            }

#ifdef __aarch64__
            const char* disasm = arm64_decode((uint32_t)value);

            char mnem[16];
            int byte = strlen(disasm);
            memset(mnem, '\0', 16);
            strcpy(mnem, disasm);
            next = instructionAddress + 4;
#else
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
            next = instructionAddress + byte;

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
#endif
            long ndx = instructionSet.find(mnem);
            if(ndx != -1)
            {
                uint8_t count = NUM_ANALYZERS;
                const isa_instr* instruction = instructionSet.get_instr(ndx);
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

            if(instructionAddress >= moduleLow && instructionAddress <= moduleHigh)
            {
            }
        }

        ptrace(PTRACE_SINGLESTEP, pid, NULL, NULL);
        waitpid(pid, &status, WSTOPPED);
    }
    kill(pid, SIGKILL);

    microseconds endProfile = duration_cast<microseconds>(high_resolution_clock::now().time_since_epoch());
    //printf("Runtime (ms): %ld Untracked: %ld\n", duration_cast<milliseconds>(endProfile-startProfile).count(), duration_cast<milliseconds>(untracked).count());

    return instructionCount;
}
