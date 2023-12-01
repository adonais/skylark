#include <stdio.h>
#include <stdlib.h>

int
main(void)
{
    char buffer[64] = { 0 };
    FILE *pipe = popen("xdotool search --name '^Skylark - '", "r");
    if (!pipe)
    {
        perror("popen failed");
        return 1;
    }
    if (fgets(buffer, sizeof(buffer), pipe))
    {
        printf("%s", buffer);
    }
    pclose(pipe);
    if (*buffer)
    {
        char process[260] = { 0 };
        snprintf(process, 260, "xdotool windowactivate --sync %s", buffer);
        return system(process);
    }
    return 0;
}

