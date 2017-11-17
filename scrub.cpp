#include <stdio.h>
#include <ctype.h>
int main()
{
    char line[256];
    FILE* input = fopen("test.txt", "r");

    while(fgets(line, sizeof(line), input))
    {
        if(isdigit(line[0]))
        {
            printf("%s", line);
        }
    }

    fclose(input);
    return 0;
}
