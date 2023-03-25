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
create_terminal_process(const char *const *evp)
{
    int status;
    pid_t pid = fork();
    if (pid == 0)
    {
        /* This is the child process.  Execute the shell command. */
        status = execvp(evp[0], evp);
        _exit(EXIT_FAILURE);
    }
    else if (pid < 0)
        /* The fork failed.  Report failure. */
        status = -1;
    return status;
}

static int
create_shell_process(const char *param)
{
    int status = EXIT_FAILURE;
    char const *exep[8 + 1] = { NULL };
    char file_manger[MAX_PARAMETER + 1] = { 0 };
    char param_manger[MAX_PARAMETER + 1] = { 0 };
    pid_t pid = fork();
    if (pid == 0)
    {
        /* This is the child process.  Execute the shell command. */
        if (access(NAUTILUS, F_OK) == 0 && access(NAUTILUS, X_OK) == 0)
        {
            snprintf(file_manger, MAX_PARAMETER, "%s", NAUTILUS);
            snprintf(param_manger, MAX_PARAMETER, "%s", param);
        }
        else if (access(DOLPHIN, F_OK) == 0 && access(DOLPHIN, X_OK) == 0)
        {
            snprintf(file_manger, MAX_PARAMETER, "%s", DOLPHIN);
            snprintf(param_manger, MAX_PARAMETER, "%s", param);
        }
        else if (access(XDGOPEN, F_OK) == 0 && access(XDGOPEN, X_OK) == 0)
        {
            snprintf(file_manger, MAX_PARAMETER, "%s", XDGOPEN);
            snprintf(param_manger, MAX_PARAMETER, "%s", param);
            if (strrchr(param_manger, '/'))
            {
                strrchr(param_manger, '/')[0] = 0;
            }
        }
        if (file_manger[0] && param_manger[0])
        {
            exep[0] = file_manger;
            exep[1] = param_manger;
            execvp(exep[0], exep);
        }
        else
        {
            status = -1;
        }
        _exit(status);
    }
    else if (pid < 0)
        /* The fork failed.  Report failure. */
        status = -1;
    return status;
}

#ifdef WINE_TEST
int
main(int argc, char **argv)
{
    int ret = 0;
    char const *exep[MAX_PARAMETER + 1] = { NULL };
    const char *term = TERMINAL;
    const char *arg_1 = "-e";
    if (argc < 2)
    {
        printf("Parameter error\n");
        ret = 1;
    }
    else if (argc == 3 && strcmp(argv[1], "explorer.exe") == 0)
    {
        ret = create_shell_process(argv[2]);
    }
    else
    {
        exep[0] = term;
        if (argc > 2 || strcmp(argv[1], "x-terminal-emulator"))
        {
            exep[1] = arg_1;
            for (int k = 1, j = 2; (k < argc) && (j < MAX_PARAMETER); k++, j++)
            {
                exep[j] = argv[k];
                printf("argv[%d] = %s\n", k, argv[k]);
            }
        }
        ret = create_terminal_process(exep);
    }
    printf("ret = %d\n", ret);
    return ret;
}
#endif
