#include <stdio.h>
#include <ctype.h>
#include <string.h>
int main()
{
    char line[256];
    FILE* input = fopen("results.txt", "r");

    while(fgets(line, sizeof(line), input))
    {
        if(isdigit(line[0]) && strstr(line, "x") == NULL)
        {
            printf("%s", line);
        }
    }

    fclose(input);
    return 0;
}
