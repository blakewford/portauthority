bool gTimeout = false;

void* startTimer(void*)
{
    int count = cachedArgc > 2 ? atoi(cachedArgv[2]): 60;
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
    sprintf(buffer, "( cat ) | avr-gdb %s -ex \"target remote :1234\" -ex \"break main\" -ex \"c\"", argument);
    int error = system(buffer);
}

void profileGdb(const char* executable)
{
    pthread_t programThread;
    pthread_create(&programThread, NULL, runProgram, (char*)executable);

    char buffer[1024];
    int32_t error = 0;
    int32_t rounds = 0;

    pid_t pid = 0;
    while(pid == 0)
    {
        char pidString[6];
        memset(pidString, '\0', 6);
        FILE *cmd;
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
    pthread_t timerThread;
    pthread_create(&timerThread, NULL, startTimer, NULL);
    while(!gTimeout)
    {
        error = system(buffer);
    }
    usleep(1*1000*1000);
    memset(buffer, '\0', 1024);
    sprintf(buffer, "echo quit > /proc/%d/fd/0", pid);
    error = system(buffer);
    printf("\n");
} 