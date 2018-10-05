#include <spawn.h>
#include <udis86.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <sys/ptrace.h>

#define BREAK 0xCC00000000000000 //x86 breakpoint instruction

void profileNative(const char* executable, uint64_t profilerAddress, uint64_t moduleBound, isa* arch, analyzer** analyzers)
{
    x86_isa& instructionSet = (x86_isa&)(*arch);

    pid_t pid = 0;
    int32_t status = 0;
    user_regs_struct registers;

    size_t size = 8;
    uint8_t binary[8];

    FILE* reopen = fopen(executable, "r");
    size_t read = fread(binary, 1, size, reopen);
    if(read != size) return;

    bool amd64 = binary[4] == 0x2;

    posix_spawn(&pid, executable, NULL, NULL, NULL, NULL);
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
                uint8_t count = NUM_ANALYZERS;
                const isa_instr* instruction = instructionSet.get_instr(ndx);
                while(count--)
                {
                    analyzers[count]->analyze(registers.rip, instruction);
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
