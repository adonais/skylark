#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <windows.h>

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

static int np_which(const char *pname, char *out, const int out_len) {
    int ret            = 0;
    const char *path[] = {"/bin",
                          "/usr/bin",
                          "/usr/local/bin",
                          "/sbin",
                          "/usr/sbin",
                          "/usr/local/sbin",
                          "/opt/bin",
                          NULL};
    if (pname && out && out_len > 0) {
        for (int i = 0; path[i]; ++i) {
            snprintf(out, out_len, "%s/%s", path[i], pname);
            if (access(out, F_OK) == 0 && access(out, X_OK) == 0) {
                ret = 1;
                printf("out = [%s]\n", out);
                break;
            }
        }
        if (!ret) {
            *out = 0;
        }
    }
    return ret;
}

static int create_terminal_process(const char *const *evp) {
    int status = -1;
    pid_t pid  = fork();
    if (pid == 0) { /* This is the child process.  Execute the terminal command. */
        status = execvp(evp[0], evp);
        _exit(EXIT_FAILURE);
    } else if (pid > 0) {
        status = (int)pid;
    }
    return status;
}

static int create_tool_process(const char *const *param) { return create_terminal_process(param); }

static int create_calc_process(void) {
    int status                      = -1;
    pid_t pid                       = -1;
    char const *exep[MAX_PARAMETER] = {NULL};
    char calc_manger[MAX_PARAMETER] = {0};
    if ((pid = fork()) == 0) { /* This is the child process.  Execute the calc command. */
        if (access(GCALC1, F_OK) == 0 && access(GCALC1, X_OK) == 0) {
            snprintf(calc_manger, MAX_PARAMETER - 1, "%s", GCALC1);
        } else if (access(GCALC2, F_OK) == 0 && access(GCALC2, X_OK) == 0) {
            snprintf(calc_manger, MAX_PARAMETER - 1, "%s", GCALC2);
        } else if (access(GCALC3, F_OK) == 0 && access(GCALC3, X_OK) == 0) {
            snprintf(calc_manger, MAX_PARAMETER - 1, "%s", GCALC3);
        } else if (access(KCALC1, F_OK) == 0 && access(KCALC1, X_OK) == 0) {
            snprintf(calc_manger, MAX_PARAMETER - 1, "%s", KCALC1);
        } else if (access(KCALC2, F_OK) == 0 && access(KCALC2, X_OK) == 0) {
            snprintf(calc_manger, MAX_PARAMETER - 1, "%s", KCALC2);
        } else if (access(KCALC3, F_OK) == 0 && access(KCALC3, X_OK) == 0) {
            snprintf(calc_manger, MAX_PARAMETER - 1, "%s", KCALC3);
        }
        if (calc_manger[0]) {
            exep[0] = calc_manger;
        } else {
            exep[0] = "/usr/bin/speedcrunch";
        }
        _exit(execvp(exep[0], (const char *const *)exep));
    } else if (pid > 0) {
        status = (int)pid;
    }
    return status;
}

static int create_shell_process(const char *const *param, const int hide) {
    int i                           = 1;
    int xdg                         = 0;
    int status                      = -1;
    pid_t pid                       = -1;
    char const *exep[MAX_PARAMETER] = {NULL};
    char file_manger[MAX_PARAMETER] = {0};
    if ((pid = fork()) == 0) { /* This is the child process.  Execute the shell command. */
        if (!hide) {
            if (access(NAUTILUS, F_OK) == 0 && access(NAUTILUS, X_OK) == 0) {
                snprintf(file_manger, MAX_PARAMETER - 1, "%s", NAUTILUS);
            } else if (access(DOLPHIN, F_OK) == 0 && access(DOLPHIN, X_OK) == 0) {
                snprintf(file_manger, MAX_PARAMETER - 1, "%s", DOLPHIN);
            } else if (access(XDGOPEN, F_OK) == 0 && access(XDGOPEN, X_OK) == 0) {
                xdg = 1;
                snprintf(file_manger, MAX_PARAMETER - 1, "%s", XDGOPEN);
            }
        }
        if (file_manger[0]) {
            int k   = 0;
            exep[0] = file_manger;
            for (i = 1; (i < MAX_PARAMETER - 1) && param[k]; ++i, ++k) {
                exep[i] = param[k];
            }
            if (xdg) {
                for (k = i; k > 0; --k) {
                    if (exep[k] && strrchr(exep[k], '/')) {
                        strrchr(exep[k], '/')[0] = 0;
                        exep[k + 1]              = NULL;
                        break;
                    }
                }
            }
            status = execvp(exep[0], (const char *const *)exep);
        } else {
            char path[260] = {0};
            const char *p  = NULL;
            exep[0]        = param[0];
            printf("exep[0] = %s\n", exep[0]);
            if (exep[0] && !(p = strchr(exep[0], '/')) && np_which(exep[0], path, 260)) {
                exep[0] = path;
            }
            for (i = 1; (i < MAX_PARAMETER - 1) && param[i]; ++i) {
                exep[i] = param[i];
            }
            if (exep[0]) {
                status = execvp(exep[0], (const char *const *)exep);
            }
        }
        _exit(status);
    } else if (pid > 0) {
        status = (int)pid;
    }
    return status;
}

static LPSTR *CommandLineToArgv_wine(LPSTR lpCmdline, int *numargs) {
    DWORD argc;
    LPSTR *argv;
    LPSTR s;
    LPSTR d;
    LPSTR cmdline;
    int qcount, bcount;

    if (!numargs || *lpCmdline == 0) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    /* --- First count the arguments */
    argc = 1;
    s    = lpCmdline;
    /* The first argument, the executable path, follows special rules */
    if (*s == '"') {
        /* The executable path ends at the next quote, no matter what */
        s++;
        while (*s)
            if (*s++ == '"') break;
    } else {
        /* The executable path ends at the next space, no matter what */
        while (*s && *s != ' ' && *s != '\t') s++;
    }
    /* skip to the first argument, if any */
    while (*s == ' ' || *s == '\t') s++;
    if (*s) argc++;

    /* Analyze the remaining arguments */
    qcount = bcount = 0;
    while (*s) {
        if ((*s == ' ' || *s == '\t') && qcount == 0) {
            /* skip to the next argument and count it if any */
            while (*s == ' ' || *s == '\t') s++;
            if (*s) argc++;
            bcount = 0;
        } else if (*s == '\\') {
            /* '\', count them */
            bcount++;
            s++;
        } else if (*s == '"') {
            /* '"' */
            if ((bcount & 1) == 0) qcount++; /* unescaped '"' */
            s++;
            bcount = 0;
            /* consecutive quotes, see comment in copying code below */
            while (*s == '"') {
                qcount++;
                s++;
            }
            qcount = qcount % 3;
            if (qcount == 2) qcount = 0;
        } else {
            /* a regular character */
            bcount = 0;
            s++;
        }
    }

    /* Allocate in a single lump, the string array, and the strings that go
     * with it. This way the caller can make a single LocalFree() call to free
     * both, as per MSDN.
     */
    argv =
        LocalAlloc(LMEM_FIXED, (argc + 1) * sizeof(LPSTR) + (strlen(lpCmdline) + 1) * sizeof(char));
    if (!argv) return NULL;
    cmdline = (LPSTR)(argv + argc + 1);
    strcpy(cmdline, lpCmdline);

    /* --- Then split and copy the arguments */
    argv[0] = d = cmdline;
    argc        = 1;
    /* The first argument, the executable path, follows special rules */
    if (*d == '"') {
        /* The executable path ends at the next quote, no matter what */
        s = d + 1;
        while (*s) {
            if (*s == '"') {
                s++;
                break;
            }
            *d++ = *s++;
        }
    } else {
        /* The executable path ends at the next space, no matter what */
        while (*d && *d != ' ' && *d != '\t') d++;
        s = d;
        if (*s) s++;
    }
    /* close the executable path */
    *d++ = 0;
    /* skip to the first argument and initialize it if any */
    while (*s == ' ' || *s == '\t') s++;
    if (!*s) {
        /* There are no parameters so we are all done */
        argv[argc] = NULL;
        *numargs   = argc;
        return argv;
    }

    /* Split and copy the remaining arguments */
    argv[argc++] = d;
    qcount = bcount = 0;
    while (*s) {
        if ((*s == ' ' || *s == '\t') && qcount == 0) {
            /* close the argument */
            *d++   = 0;
            bcount = 0;

            /* skip to the next one and initialize it if any */
            do {
                s++;
            } while (*s == ' ' || *s == '\t');
            if (*s) argv[argc++] = d;
        } else if (*s == '\\') {
            *d++ = *s++;
            bcount++;
        } else if (*s == '"') {
            if ((bcount & 1) == 0) {
                /* Preceded by an even number of '\', this is half that
                 * number of '\', plus a quote which we erase.
                 */
                d -= bcount / 2;
                qcount++;
            } else {
                /* Preceded by an odd number of '\', this is half that
                 * number of '\' followed by a '"'
                 */
                d    = d - bcount / 2 - 1;
                *d++ = '"';
            }
            s++;
            bcount = 0;
            /* Now count the number of consecutive quotes. Note that qcount
             * already takes into account the opening quote if any, as well as
             * the quote that lead us here.
             */
            while (*s == '"') {
                if (++qcount == 3) {
                    *d++   = '"';
                    qcount = 0;
                }
                s++;
            }
            if (qcount == 2) qcount = 0;
        } else {
            /* a regular character */
            *d++   = *s++;
            bcount = 0;
        }
    }
    *d         = '\0';
    argv[argc] = NULL;
    *numargs   = argc;

    return argv;
}

#ifdef WINE_TEST
int main(void) {
    int ret                         = 0;
    int arg_c                       = 0;
    char const *exep[MAX_PARAMETER] = {NULL};
    const char *term                = TERMINAL;
    const char *arg_1               = "-e";
    char **arglist                  = CommandLineToArgv_wine(GetCommandLineA(), &arg_c);
    if (arglist) {
        if (arg_c < 2) {
            printf("Parameter error\n");
            ret = 1;
        } else if (arg_c == 2 && strcmp(arglist[1], "calc") == 0) {
            printf("calc runing\n");
            ret = create_calc_process();
        } else if (arg_c == 3 && strcmp(arglist[1], "xtool") == 0) {
            ret = create_tool_process((const char *const *)&arglist[2]);
        } else if (arg_c >= 3 && (strcmp(arglist[1], "explorer.exe") == 0 ||
                                  strcmp(arglist[1], "hide.exe") == 0)) {
            ret = create_shell_process((const char *const *)&arglist[2],
                                       strcmp(arglist[1], "hide.exe") == 0);
        } else {
            exep[0] = term;
            if (arg_c > 2 || strcmp(arglist[1], "x-terminal-emulator")) {
                exep[1] = arg_1;
                for (int k = 1, j = 2; (k < arg_c) && (j < MAX_PARAMETER - 1); k++, j++) {
                    exep[j] = arglist[k];
                    printf("arglist[%d] = %s\n", k, arglist[k]);
                }
            }
            ret = create_terminal_process((const char *const *)exep);
        }
        LocalFree(arglist);
    }
    printf("ret = %d\n", ret);
    return (ret > 0 ? 0 : ret);
}
#endif
