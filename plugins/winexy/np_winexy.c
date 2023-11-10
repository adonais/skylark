#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <windef.h>

/* Execute the command using this shell program.  */
#define TERMINAL "/bin/x-terminal-emulator"
#define SHELL "/bin/bash"
#define NAUTILUS "/usr/bin/nautilus"
#define DOLPHIN "/usr/bin/dolphin"
#define XDGOPEN "/usr/bin/xdg-open"
#define MAX_PARAMETER 256
#ifndef X_OK
#define X_OK 1 /* Test for execute permission.  */
#endif
#ifndef F_OK
#define F_OK 0 /* Test for existence.  */
#endif

extern pid_t fork(void);

static int
create_terminal_process(const char* const *evp)
{
    int status = -1;
    pid_t pid = fork();
    if (pid == 0)
    {   /* This is the child process.  Execute the terminal command. */
        status = execvp(evp[0], evp);
        _exit(EXIT_FAILURE);
    }
    else if (pid > 0)
    {
        status = (int)pid;
    }
    return status;
}

static int
create_shell_process(const char* const *param, const int hide)
{
    int status = -1;
    pid_t pid = -1;
    char const *exep[MAX_PARAMETER] = { NULL };
    char file_manger[MAX_PARAMETER] = { 0 };
    char param_manger[MAX_PARAMETER] = { 0 };
    if ((pid = fork()) == 0)
    {   /* This is the child process.  Execute the shell command. */
        if (!hide)
        {
            if (access(NAUTILUS, F_OK) == 0 && access(NAUTILUS, X_OK) == 0)
            {
                snprintf(file_manger, MAX_PARAMETER - 1, "%s", NAUTILUS);
                snprintf(param_manger, MAX_PARAMETER - 1, "%s", param[0]);
            }
            else if (access(DOLPHIN, F_OK) == 0 && access(DOLPHIN, X_OK) == 0)
            {
                snprintf(file_manger, MAX_PARAMETER - 1, "%s", DOLPHIN);
                snprintf(param_manger, MAX_PARAMETER - 1, "%s", param[0]);
            }
            else if (access(XDGOPEN, F_OK) == 0 && access(XDGOPEN, X_OK) == 0)
            {
                snprintf(file_manger, MAX_PARAMETER - 1, "%s", XDGOPEN);
                snprintf(param_manger, MAX_PARAMETER - 1, "%s", param[0]);
                if (strrchr(param_manger, '/'))
                {
                    strrchr(param_manger, '/')[0] = 0;
                }
            }
        }
        if (file_manger[0] && param_manger[0])
        {
            exep[0] = file_manger;
            exep[1] = param_manger;
        }
        else
        {
            exep[0] = param[0];
            for (int i = 1; (i < MAX_PARAMETER - 1) && param[i]; ++i)
            {
                exep[i] = param[i];
            }
        }
        _exit(execvp(exep[0], (const char* const *)exep));
    }
    else if (pid > 0)
    {
        status = (int)pid;
    }
    return status;
}

#ifdef WINE_TEST
int
main(int argc, char **argv)
{
    int ret = 0;
    char const *exep[MAX_PARAMETER] = { NULL };
    const char *term = TERMINAL;
    const char *arg_1 = "-e";
    if (argc < 2)
    {
        printf("Parameter error\n");
        ret = 1;
    }
    else if (argc >= 3 && (strcmp(argv[1], "explorer.exe") == 0 || strcmp(argv[1], "hide.exe") == 0))
    {
        ret = create_shell_process((const char * const*)&argv[2], strcmp(argv[1], "hide.exe") == 0);
    }
    else
    {
        exep[0] = term;
        if (argc > 2 || strcmp(argv[1], "x-terminal-emulator"))
        {
            exep[1] = arg_1;
            for (int k = 1, j = 2; (k < argc) && (j < MAX_PARAMETER - 1); k++, j++)
            {
                exep[j] = argv[k];
                printf("argv[%d] = %s\n", k, argv[k]);
            }
        }
        ret = create_terminal_process((const char* const *)exep);
    }
    printf("ret = %d\n", ret);
    return (ret > 0 ? 0 : ret);
}
#endif
