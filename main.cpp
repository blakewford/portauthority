#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <libelf.h>

int32_t cachedArgc = 0;
char argvStorage[1024];
char* cachedArgv[64];

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

    unsigned int version = elf_version(EV_CURRENT);
    assert(version != EV_NONE);

    Elf* elf = NULL;
    int executable = 0;
    if(cachedArgc > 1) executable = open(cachedArgv[1], O_RDONLY, 0);
    if(executable)
    {
        elf = elf_begin(executable, ELF_C_READ, NULL);
        Elf_Kind ek = elf_kind(elf);
        switch(ek)
        {
            case ELF_K_ELF:
                break;
            case ELF_K_NONE:
                printf("%s is not an elf file.\n", cachedArgv[1]);
                return -1;
            case ELF_K_AR:
            default:
                assert(0);
        }
    }

    printf("Clustering.\n");

    elf_end(elf);
    close(executable);

}
