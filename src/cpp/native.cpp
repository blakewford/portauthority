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

uint32_t profileNative(const char* executable, uint64_t profilerAddress, uint64_t moduleBound, uint64_t exitAddress, isa* arch, analyzer** analyzers)
{
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
    FILE* process = popen("./suspend.sh ~/ID-15-Shadow-Runner/runner", "r");

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
    ptrace(PTRACE_POKEDATA, pid, profilerAddress, *bytes);
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
#endif

    uint32_t instructionCount = 0;
    while(WIFSTOPPED(status))
    {
#ifdef __aarch64__
        ptrace(PTRACE_GETREGSET, pid, NT_PRSTATUS, registerBuffer);
        if(registers.pc == exitAddress)
#else
        ptrace(PTRACE_GETREGS, pid, NULL, registerBuffer);
        if(registers.rip == exitAddress)
#endif
        {
            //natural program termination
            break;
        }
        //need better protections here for code that does not exit cleanly, without exit()
#ifdef __aarch64__
        if(registers.pc < moduleBound)
        {
            uint64_t value = ptrace(PTRACE_PEEKDATA, pid, registers.pc, NULL);
            const char* disasm = arm64_decode((uint32_t)value);

            char mnem[16];
            int byte = strlen(disasm);
            memset(mnem, '\0', 16);
            strcpy(mnem, disasm);
#else
        if(registers.rip < moduleBound)
        {
            uint64_t value = ptrace(PTRACE_PEEKDATA, pid, registers.rip, NULL);
            //printf("%llx\n", registers.rip);
            if(!transition)
            {
                if(startTransition != endTransition)
                {
                    endTransition = duration_cast<microseconds>(high_resolution_clock::now().time_since_epoch());
                    untracked += endTransition-startTransition;
                }
            }
            transition = true;
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
#ifdef __aarch64__
                    analyzers[count]->analyze(registers.pc, &modified);
#else
                    analyzers[count]->analyze(registers.rip, &modified);
#endif
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
        }

        ptrace(PTRACE_SINGLESTEP, pid, NULL, NULL);
        waitpid(pid, &status, WSTOPPED);
    }
    kill(pid, SIGKILL);

    microseconds endProfile = duration_cast<microseconds>(high_resolution_clock::now().time_since_epoch());
    //printf("Runtime (ms): %ld Untracked: %ld\n", duration_cast<milliseconds>(endProfile-startProfile).count(), duration_cast<milliseconds>(untracked).count());

    return instructionCount;
}
