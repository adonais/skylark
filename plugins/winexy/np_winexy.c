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
#define GCALC1 "/usr/bin/gnome-calculator"
#define GCALC2 "/usr/bin/xcalc"
#define GCALC3 "/usr/bin/galculator"
#define KCALC1 "/usr/bin/kcalc"
#define KCALC2 "/usr/bin/kalk"
#define KCALC3 "/usr/bin/kalgebra"
#define MAX_PARAMETER 256
#ifndef X_OK
#define X_OK 1 /* Test for execute permission.  */
#endif
#ifndef F_OK
#define F_OK 0 /* Test for existence.  */
#endif

extern pid_t fork(void);

static int
np_which(const char *pname, char *out, const int out_len)
{
    int  ret = 0;
    const char *path[] = {"/bin",
                          "/usr/bin",
                          "/usr/local/bin",
                          "/sbin",
                          "/usr/sbin",
                          "/usr/local/sbin",
                          "/opt/bin",
                          NULL
                         };
    if (pname && out && out_len > 0)
    {
        for (int i = 0; path[i]; ++i)
        {
            snprintf(out, out_len, "%s/%s", path[i], pname);
            if (access(out, F_OK) == 0 && access(out, X_OK) == 0)
            {
                ret = 1;
                printf("out = [%s]\n", out);
                break;
            }
        }
        if (!ret)
        {
            *out = 0;
        }
    }
    return ret;
}

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
create_tool_process(const char* const *param)
{
    return create_terminal_process(param);
}

static int
create_calc_process(void)
{
    int status = -1;
    pid_t pid = -1;
    char const *exep[MAX_PARAMETER] = { NULL };
    char calc_manger[MAX_PARAMETER] = { 0 };
    if ((pid = fork()) == 0)
    {   /* This is the child process.  Execute the calc command. */
        if (access(GCALC1, F_OK) == 0 && access(GCALC1, X_OK) == 0)
        {
            snprintf(calc_manger, MAX_PARAMETER - 1, "%s", GCALC1);
        }
        else if (access(GCALC2, F_OK) == 0 && access(GCALC2, X_OK) == 0)
        {
            snprintf(calc_manger, MAX_PARAMETER - 1, "%s", GCALC2);
        }
        else if (access(GCALC3, F_OK) == 0 && access(GCALC3, X_OK) == 0)
        {
            snprintf(calc_manger, MAX_PARAMETER - 1, "%s", GCALC3);
        }
        else if (access(KCALC1, F_OK) == 0 && access(KCALC1, X_OK) == 0)
        {
            snprintf(calc_manger, MAX_PARAMETER - 1, "%s", KCALC1);
        }
        else if (access(KCALC2, F_OK) == 0 && access(KCALC2, X_OK) == 0)
        {
            snprintf(calc_manger, MAX_PARAMETER - 1, "%s", KCALC2);
        }
        else if (access(KCALC3, F_OK) == 0 && access(KCALC3, X_OK) == 0)
        {
            snprintf(calc_manger, MAX_PARAMETER - 1, "%s", KCALC3);
        }
        if (calc_manger[0])
        {
            exep[0] = calc_manger;
        }
        else
        {
            exep[0] = "/usr/bin/speedcrunch";
        }
        _exit(execvp(exep[0], (const char* const *)exep));
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
    int i = 1;
    int xdg = 0;
    int status = -1;
    pid_t pid = -1;
    char const *exep[MAX_PARAMETER] = { NULL };
    char file_manger[MAX_PARAMETER] = { 0 };
    if ((pid = fork()) == 0)
    {   /* This is the child process.  Execute the shell command. */
        if (!hide)
        {
            if (access(NAUTILUS, F_OK) == 0 && access(NAUTILUS, X_OK) == 0)
            {
                snprintf(file_manger, MAX_PARAMETER - 1, "%s", NAUTILUS);
            }
            else if (access(DOLPHIN, F_OK) == 0 && access(DOLPHIN, X_OK) == 0)
            {
                snprintf(file_manger, MAX_PARAMETER - 1, "%s", DOLPHIN);
            }
            else if (access(XDGOPEN, F_OK) == 0 && access(XDGOPEN, X_OK) == 0)
            {
                xdg = 1;
                snprintf(file_manger, MAX_PARAMETER - 1, "%s", XDGOPEN);
            }
        }
        if (file_manger[0])
        {
            int k = 0;
            exep[0] = file_manger;
            for (i = 1; (i < MAX_PARAMETER - 1) && param[k]; ++i, ++k)
            {
                exep[i] = param[k];
            }
            if (xdg)
            {
                for (k = i; k > 0; --k)
                {
                    if (exep[k] && strrchr(exep[k], '/'))
                    {
                        strrchr(exep[k], '/')[0] = 0;
                        exep[k + 1] = NULL;
                        break;
                    }
                }
            }
            status = execvp(exep[0], (const char* const *)exep);
        }
        else
        {
            char path[260] = {0};
            const char *p = NULL;
            exep[0] = param[0];
            printf("exep[0] = %s\n", exep[0]);
            if (exep[0] && !(p = strchr(exep[0], '/')) && np_which(exep[0], path, 260))
            {
                exep[0] = path;
            }
            for (i = 1; (i < MAX_PARAMETER - 1) && param[i]; ++i)
            {
                exep[i] = param[i];
            }
            if (exep[0])
            {
                status = execvp(exep[0], (const char* const *)exep);
            }
        }
        _exit(status);
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
    else if (argc == 2 && strcmp(argv[1], "calc") == 0)
    {
        ret = create_calc_process();
    }
    else if (argc == 3 && strcmp(argv[1], "xtool") == 0)
    {
        ret = create_tool_process((const char* const *)&argv[2]);
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
