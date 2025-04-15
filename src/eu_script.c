/******************************************************************************
 * This file is part of Skylark project
 * Copyright ©2025 Hua andy <hua.andy@gmail.com>

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * at your option any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include "framework.h"
#endif

#define luajit_c

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "luajit.h"
#include "lj_arch.h"

#if LJ_TARGET_POSIX
#include <unistd.h>
#define lua_stdin_is_tty()	isatty(0)
#elif LJ_TARGET_WINDOWS
#include <io.h>
#ifdef __BORLANDC__
#define lua_stdin_is_tty()	isatty(_fileno(stdin))
#else
#define lua_stdin_is_tty()	_isatty(_fileno(stdin))
#endif
#else
#define lua_stdin_is_tty()	1
#endif

#ifndef lua_pushglobaltable
#define lua_pushglobaltable(L) lua_pushvalue(L,LUA_GLOBALSINDEX)
#endif

/* check that argument has no extra characters at the end */
#define l_notail(x) {if ((x)[2] != '\0') return -1;}
#define FLAGS_INTERACTIVE    1
#define FLAGS_VERSION        2
#define FLAGS_EXEC           4
#define FLAGS_OPTION         8
#define FLAGS_NOENV          16

static lua_State *pstate = NULL;
static obs_skylark *psb_client = NULL;

static int
l_collectargs(char **argv, int *flags)
{
    int i;
    for (i = 1; argv[i] != NULL; i++)
    {
        if (argv[i][0] != '-') /* Not an option? */
            return i;
        switch (argv[i][1])
        { /* Check option. */
            case '-':
                l_notail(argv[i]);
                return i + 1;
            case '\0':
                return i;
            case 'i':
                l_notail(argv[i]);
                *flags |= FLAGS_INTERACTIVE;
                /* fallthrough */
            case 'v':
                l_notail(argv[i]);
                *flags |= FLAGS_VERSION;
                break;
            case 'e':
                *flags |= FLAGS_EXEC;
                /* fallthrough */
            case 'l':
                *flags |= FLAGS_OPTION;
                if (argv[i][2] == '\0')
                {
                    i++;
                    if (argv[i] == NULL) return -1;
                }
                break;
            case 'b':  /* LuaJIT extension */
                if (*flags) return -1;
                *flags |= FLAGS_EXEC;
                return i + 1;
            case 'E':
                *flags |= FLAGS_NOENV;
                break;
            default:
                return -1; /* invalid option */
        }
    }
    return i;
}

static void
l_message(const char *msg)
{
    char *pname = eu_utf16_utf8(__ORIGINAL_NAME, NULL);
    if (pname)
    {
        fputs(pname, stderr);
        fputc(':', stderr);
        fputc(' ', stderr);
        free(pname);
    }
    fputs(msg, stderr);
    fputc('\n', stderr);
    fflush(stderr);
}

static int
l_report(lua_State *L, int status)
{
    if (status && !lua_isnil(L, -1))
    {
        const char *msg = lua_tostring(L, -1);
        if (msg == NULL)
        {
            msg = "(error object is not a string)";
        }
        l_message(msg);
        lua_pop(L, 1);
    }
    return status;
}

static int
l_traceback(lua_State *L)
{
    if (!lua_isstring(L, 1))
    { /* Non-string error object? Try metamethod. */
        if (lua_isnoneornil(L, 1) || !luaL_callmeta(L, 1, "__tostring") || !lua_isstring(L, -1))
        {
            return 1;     /* Return non-string error object. */
        }
        lua_remove(L, 1); /* Replace object by result of __tostring metamethod. */
    }
    luaL_traceback(L, L, lua_tostring(L, 1), 1);
    return 1;
}

static int
report_gui(lua_State *L, int status)
{
    if (status && !lua_isnil(L, -1))
    {
        char *gui_msg = NULL;
        const char *msg = lua_tostring(L, -1);
        if (msg == NULL)
        {
            msg = "(error object is not a string)";
        }
        if ((gui_msg = util_unix_newline(msg, strlen(msg))))
        {
            on_result_append_text_utf8("%s", gui_msg);
            free(gui_msg);
        }
        lua_pop(L, 1);
    }
    return status;
}

static void
print_jit_status(lua_State *L)
{
    int n;
    const char *s;
    lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED");
    lua_getfield(L, -1, "jit"); /* Get jit.* module table. */
    lua_remove(L, -2);
    lua_getfield(L, -1, "status");
    lua_remove(L, -2);
    n = lua_gettop(L);
    lua_call(L, 0, LUA_MULTRET);
    fputs(lua_toboolean(L, n) ? "JIT: ON" : "JIT: OFF", stdout);
    for (n++; (s = lua_tostring(L, n)); n++)
    {
        putc(' ', stdout);
        fputs(s, stdout);
    }
    putc('\n', stdout);
    lua_settop(L, 0); /* clear stack */
}

static void
print_version(void)
{
    fputs("» » " LUAJIT_VERSION "\n", stdout);
}

static void
l_createargtable(lua_State *L, char **argv, int argc, int argf)
{
    int i;
    lua_createtable(L, argc - argf, argf);
    for (i = 0; i < argc; i++)
    {
        lua_pushstring(L, argv[i]);
        lua_rawseti(L, -2, i - argf);
    }
    lua_setglobal(L, "arg");
}

static void
write_prompt(lua_State *L, int firstline)
{
    const char *p;
    lua_getfield(L, LUA_GLOBALSINDEX, firstline ? "_PROMPT" : "_PROMPT2");
    p = lua_tostring(L, -1);
    if (p == NULL)
    {
        p = firstline ? LUA_PROMPT : LUA_PROMPT2;
    }
    fputs(p, stdout);
    fflush(stdout);
    lua_pop(L, 1); /* remove global */
}

static int
incomplete(lua_State *L, int status)
{
    if (status == LUA_ERRSYNTAX)
    {
        size_t lmsg;
        const char *msg = lua_tolstring(L, -1, &lmsg);
        const char *tp = msg + lmsg - (sizeof(LUA_QL("<eof>")) - 1);
        if (strstr(msg, LUA_QL("<eof>")) == tp)
        {
            lua_pop(L, 1);
            return 1;
        }
    }
    return 0; /* else... */
}

static int
pushline(lua_State *L, int firstline)
{
    char buf[LUA_MAXINPUT];
    write_prompt(L, firstline);
    if (fgets(buf, LUA_MAXINPUT, stdin))
    {
        size_t len = strlen(buf);
        if (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r'))
        {
            buf[len - 1] = '\0';
        }
        if (firstline && buf[0] == '=')
        {
            lua_pushfstring(L, "return %s", buf + 1);
        }
        else
        {
            lua_pushstring(L, buf);
        }
        return 1;
    }
    return 0;
}

static int
loadline(lua_State *L)
{
    int status;
    lua_settop(L, 0);
    if (!pushline(L, 1)) return -1; /* no input */
    for (;;)
    { /* repeat until gets a complete line */
        status = luaL_loadbuffer(L, lua_tostring(L, 1), lua_strlen(L, 1), "=stdin");
        if (!incomplete(L, status)) break; /* cannot try to add lines? */
        if (!pushline(L, 0))
        {   /* no more input? */
            return -1;
        }
        lua_pushliteral(L, "\n"); /* add a new line... */
        lua_insert(L, -2);        /* ...between the two lines */
        lua_concat(L, 3);         /* join them */
    }
    lua_remove(L, 1); /* remove line */
    return status;
}

static int
docall(lua_State *L, int narg, int clear)
{
    int status;
    int base = lua_gettop(L) - narg; /* function index */
    lua_pushcfunction(L, l_traceback); /* push l_traceback function */
    lua_insert(L, base);             /* put it under chunk and args */
    status = lua_pcall(L, narg, (clear ? 0 : LUA_MULTRET), base);
    lua_remove(L, base); /* remove l_traceback function */
    /* force a complete garbage collection in case of errors */
    if (status != LUA_OK)
    {
        lua_gc(L, LUA_GCCOLLECT, 0);
    }
    return status;
}

static int
dostring(lua_State *L, const char *s, const char *name)
{
    int status = luaL_loadbuffer(L, s, strlen(s), name) || docall(L, 0, 1);
    return report_gui(L, status);
}

/* Load add-on module. */
static int
loadjitmodule(lua_State *L)
{
    lua_getglobal(L, "require");
    lua_pushliteral(L, "jit.");
    lua_pushvalue(L, -3);
    lua_concat(L, 2);
    if (lua_pcall(L, 1, 1, 0))
    {
        const char *msg = lua_tostring(L, -1);
        if (msg && !strncmp(msg, "module ", 7))
        {
            goto nomodule;
        }
        return l_report(L, 1);
    }
    lua_getfield(L, -1, "start");
    if (lua_isnil(L, -1))
    {
    nomodule:
        l_message("unknown luaJIT command or jit.* modules not installed");
        return 1;
    }
    lua_remove(L, -2); /* Drop module table. */
    return 0;
}

static int
dofile(lua_State *L, const char *name)
{
    int status = luaL_loadfile(L, name) || docall(L, 0, 1);
    return l_report(L, status);
}

static int
dobyte(lua_State *L, const char *fname, const char *save)
{
    lua_pushliteral(L, "bcsave");
    if (loadjitmodule(L))
    {
        return 1;
    }
    lua_pushstring(L, fname);
    lua_pushstring(L, save);
    return l_report(L, lua_pcall(L, 2, 0, 0));
}

/* Save or list bytecode. */
static int
dobytecode(lua_State *L, char **argv)
{
    int narg = 0;
    lua_pushliteral(L, "bcsave");
    if (loadjitmodule(L))
    {
        return 1;
    }
    if (argv[0][2])
    {
        narg++;
        argv[0][1] = '-';
        lua_pushstring(L, argv[0] + 1);
    }
    for (argv++; *argv != NULL; narg++, argv++)
    {
        lua_pushstring(L, *argv);
    }
    l_report(L, lua_pcall(L, narg, 0, 0));
    return -1;
}

static int
dolibrary(lua_State *L, const char *name)
{
    lua_getglobal(L, "require");
    lua_pushstring(L, name);
    return l_report(L, docall(L, 1, 1));
}

static int
l_runargs(lua_State *L, char **argv, int argn)
{
    for (int i = 1; i < argn; i++)
    {
        if (argv[i] == NULL) continue;
        lua_assert(argv[i][0] == '-');
        switch (argv[i][1])
        {
            case 'e':
            {
                const char *chunk = argv[i] + 2;
                if (*chunk == '\0') chunk = argv[++i];
                lua_assert(chunk != NULL);
                if (dostring(L, chunk, "=(command line)") != 0) return 1;
                break;
            }
            case 'l':
            {
                const char *filename = argv[i] + 2;
                if (*filename == '\0') filename = argv[++i];
                lua_assert(filename != NULL);
                if (dolibrary(L, filename)) return 1;
                break;
            }
            case 'b': /* LuaJIT extension. */
                return dobytecode(L, argv + i);
            default:
                break;
        }
    }
    return LUA_OK;
}

static int
handle_luainit(lua_State *L)
{
    const char *init = getenv(LUA_INIT);
    if (init == NULL)
        return LUA_OK;
    else if (init[0] == '@')
        return dofile(L, init + 1);
    else
        return dostring(L, init, "=" LUA_INIT);
}

static int
handle_script(lua_State *L, char **argx)
{
    int status;
    const char *fname = argx[0];
    if (strcmp(fname, "-") == 0 && strcmp(argx[-1], "--") != 0) fname = NULL; /* stdin */
    status = luaL_loadfile(L, fname);
    if (status == LUA_OK)
    {
        /* Fetch args from arg table. LUA_INIT or -e might have changed them. */
        int narg = 0;
        lua_getglobal(L, "arg");
        if (lua_istable(L, -1))
        {
            do
            {
                narg++;
                lua_rawgeti(L, -narg, narg);
            } while (!lua_isnil(L, -1));
            lua_pop(L, 1);
            lua_remove(L, -narg);
            narg--;
        }
        else
        {
            lua_pop(L, 1);
        }
        status = docall(L, narg, 0);
    }
    return l_report(L, status);
}

static int
dotty(lua_State *L)
{
    int status = -1;
    while ((status = loadline(L)) != -1)
    {
        if (status == LUA_OK)
        {
            status = docall(L, 0, 0);
        }
        l_report(L, status);
        if (status == LUA_OK && lua_gettop(L) > 0)
        { /* any result to print? */
            lua_getglobal(L, "print");
            lua_insert(L, 1);
            if (lua_pcall(L, lua_gettop(L) - 1, 0, 0) != 0)
            {
                l_message(lua_pushfstring(L, "error calling " LUA_QL("print") " (%s)", lua_tostring(L, -1)));
            }
        }
    }
    lua_settop(L, 0); /* clear stack */
    fputs("\n", stdout);
    fflush(stdout);
    return status;
}

int
on_script_bcsaved(char *arg1, char *arg2)
{
    int status = -1;
    lua_State *L = NULL;
    char *arg0 = NULL;
    if (!arg1 || !arg2 || (L = lua_open()) == NULL)
    {
        return -1;
    }
    lua_gc(L, LUA_GCSTOP, 0);
    luaL_openlibs(L);
    lua_gc(L, LUA_GCRESTART, -1);
    if ((arg0 = _strdup("-be")) != NULL)
    {
        char *argv[] = {arg0, arg1, arg2, NULL};
        status = dobytecode(L, (char **)argv);
        free(arg0);
    }
    lua_close(L);
    return status;
}

int
eu_lua_script_evp(const int argc, char **argv)
{
    int flags = 0;
    int argn = 0;
    int status = -1;
    lua_State *L = NULL;
    do
    {
        if (!(L = lua_open()))
        {
            eu_logmsg("Scripts: %s, cannot create state[not enough memory]\n", __FUNCTION__);
            return -1;
        }
        if ((argn = l_collectargs(argv, &flags)) < 0)
        {
            eu_logmsg("Scripts: %s, Invalid args\n", __FUNCTION__);
            break;
        }
        if ((flags & FLAGS_NOENV))
        {
            lua_pushboolean(L, 1);
            lua_setfield(L, LUA_REGISTRYINDEX, "LUA_NOENV");
        }
        /* Stop collector during library initialization. */
        lua_gc(L, LUA_GCSTOP, 0);
        luaL_openlibs(L);
        lua_gc(L, LUA_GCRESTART, -1);
        l_createargtable(L, argv, argc, argn);
        if (!(flags & FLAGS_NOENV))
        {
            if ((status = handle_luainit(L)) != LUA_OK)
            {
                break;
            }
        }
        if ((flags & FLAGS_VERSION))
        {
            print_version();
        }
        if ((status = l_runargs(L, argv, argn)) != LUA_OK)
        {
            break;
        }
        if (argc > argn)
        {
            if ((status = handle_script(L, argv + argn)) != LUA_OK)
            {
                break;
            }
        }
        if ((flags & FLAGS_INTERACTIVE))
        {
            print_jit_status(L);
            status = dotty(L);
        }
        else if (argc == argn && !(flags & (FLAGS_EXEC | FLAGS_VERSION)))
        {
            if (lua_stdin_is_tty())
            {
                print_version();
                print_jit_status(L);
                status = dotty(L);
            }
            else
            {   /* Executes stdin as a file. */
                status = dofile(L, NULL);
            }
        }
    } while(0);
    if (L != NULL)
    {
        l_report(L, status);
        lua_close(L);
    }
    return status;
}

int
eu_lua_script_convert(const TCHAR *fname, const TCHAR *save)
{
    int status;
    char *filepath = NULL;
    char *savepath = NULL;
    if (!fname)
    {
        return -1;
    }
    lua_State *L = lua_open();
    if (L == NULL)
    {
        eu_logmsg("Scripts: %s, cannot create state[not enough memory]\n", __FUNCTION__);
        return -1;
    }
    /* Stop collector during library initialization. */
    lua_gc(L, LUA_GCSTOP, 0);
    luaL_openlibs(L);
    lua_gc(L, LUA_GCRESTART, -1);
#ifdef _UNICODE
    if ((filepath = eu_utf16_utf8(fname, NULL)) == NULL)
    {
        eu_logmsg("Scripts: %s, eu_utf16_utf8 convert failed\n", __FUNCTION__);
        lua_close(L);
        return -1;
    }
#else
    filepath = fname;
#endif
    if (!save)
    {
        status = dofile(L, filepath);
    }
    else
    {
    #ifdef _UNICODE
        if ((savepath = eu_utf16_utf8(save, NULL)) == NULL)
        {
            eu_logmsg("Scripts: %s, eu_utf16_utf8 convert failed\n", __FUNCTION__);
            lua_close(L);
            return -1;
        }
    #else
        savepath = save;
    #endif
        status = dobyte(L, filepath, savepath);
    }
    lua_close(L);
#ifdef _UNICODE
    if (filepath)
    {
        free(filepath);
    }
    if (savepath)
    {
        free(savepath);
    }
#endif
    return status;
}

int
do_lua_func(const char *fname, const char *func, const char *arg)
{
    int status;
    if (!fname)
    {
        return -1;
    }
    lua_State *L = lua_open();
    if (L == NULL)
    {
        eu_logmsg("Scripts: %s, cannot create state[not enough memory]\n", __FUNCTION__);
        return -1;
    }
    /* Stop collector during library initialization. */
    lua_gc(L, LUA_GCSTOP, 0);
    luaL_openlibs(L);
    lua_gc(L, LUA_GCRESTART, -1);
    status = dofile(L, fname);
    if (status == LUA_OK)
    {
        lua_getglobal(L, func);
        lua_pushstring(L, arg);
        status = lua_pcall(L, 1, 0, 0);
        if (status == LUA_OK)
        {
            lua_pop(L, 1);
        }
        else
        {
            eu_logmsg("Scripts: %s, [%s:%s lua_pcall failed]\n", __FUNCTION__, fname, func);
        }
    }
    lua_close(L);
    return status;
}

int
do_lua_point(const char *fname, const char *func, void *arg)
{
    int status;
    if (!fname)
    {
        return -1;
    }
    lua_State *L = lua_open();
    if (L == NULL)
    {
        eu_logmsg("Scripts: %s, cannot create state[not enough memory]\n", __FUNCTION__);
        return -1;
    }
    lua_gc(L, LUA_GCSTOP, 0);
    luaL_openlibs(L);
    lua_gc(L, LUA_GCRESTART, -1);
    status = dofile(L, fname);
    if (status == LUA_OK)
    {
        lua_getglobal(L, func);
        lua_pushlightuserdata(L, arg);
        status = lua_pcall(L, 1, 0, 0);
        if (status == LUA_OK)
        {
            lua_pop(L, 1);
        }
        else
        {
            eu_logmsg("Scripts: %s, [%s:%s lua_pcall failed]\n", __FUNCTION__, fname, func);
        }
    }
    lua_close(L);
    return status;
}

int
do_lua_integer(const char *fname, const char *func, const int arg)
{
    int status;
    if (!fname)
    {
        return -1;
    }
    lua_State *L = lua_open();
    if (L == NULL)
    {
        eu_logmsg("Scripts: %s, cannot create state[not enough memory]\n", __FUNCTION__);
        return -1;
    }
    /* Stop collector during library initialization. */
    lua_gc(L, LUA_GCSTOP, 0);
    luaL_openlibs(L);
    lua_gc(L, LUA_GCRESTART, -1);
    status = dofile(L, fname);
    if (status == LUA_OK)
    {
        lua_getglobal(L, func);
        lua_pushinteger(L, arg);
        status = lua_pcall(L, 1, 0, 0);
        if (status == LUA_OK)
        {
            lua_pop(L, 1);
        }
        else
        {
            eu_logmsg("Scripts: %s, [%s:%s lua_pcall failed]\n", __FUNCTION__, fname, func);
        }
    }
    lua_close(L);
    return status;
}

int
do_lua_parser_doctype(const char *fname, const char *func)
{
    int status;
    uintptr_t doc_point = 0;
    pstate = lua_open();
    if (pstate == NULL)
    {
        eu_logmsg("Scripts: %s, cannot create state[not enough memory]\n", __FUNCTION__);
        return -1;
    }
    /* Stop collector during library initialization. */
    lua_gc(pstate, LUA_GCSTOP, 0);
    luaL_openlibs(pstate);
    lua_gc(pstate, LUA_GCRESTART, -1);
    status = dofile(pstate, fname);
    if (status == LUA_OK)
    {
        lua_getglobal(pstate, func);
        /* pops the function and 0 parameters, pushes 1 results */
        status = lua_pcall(pstate, 0, 1, 0);
        if (status != LUA_OK)
        {
            eu_logmsg("Scripts: %s, lua_pcall failed\n", __FUNCTION__);
            lua_close(pstate);
            pstate = NULL;
        }
        else
        {
            doc_point = (uintptr_t)lua_tonumber(pstate, -1);
            lua_pop(pstate, 1);
        }
    }
    if (doc_point)
    {
        on_doc_set_ptr((doctype_t *)doc_point);
        on_doc_set_vec();
    }
    return status;
}

void
do_lua_parser_release(void)
{
    if (pstate)
    {
        lua_close(pstate);
        pstate = NULL;
    }
    if (psb_client)
    {
        cvector_free(psb_client);
    }
}

/** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * 成功, 返回一个长度为1024的宽字符串, 使用后需要释放内存
 * 失败, 返回空
 */
TCHAR*
do_lua_parser_path(const char *file)
{
    TCHAR *path = NULL;
    lua_State *L = lua_open();
    if (L == NULL)
    {
        eu_logmsg("Scripts: %s, cannot create state[not enough memory]\n", __FUNCTION__);
        return NULL;
    }
    /* Stop collector during library initialization. */
    lua_gc(L, LUA_GCSTOP, 0);
    luaL_openlibs(L);
    lua_gc(L, LUA_GCRESTART, -1);
    if (dofile(L, file) == LUA_OK)
    {
        lua_getglobal(L, "profile_dir");
        if (lua_isstring(L, -1))
        {
            const char *value = lua_tostring(L, -1);
            if (STR_NOT_NUL(value) && (path = (TCHAR *)calloc(sizeof(TCHAR), MAX_BUFFER)))
            {
                util_make_u16(value, path, MAX_BUFFER - 1);
            }
        }
    }
    lua_close(L);
    return path;
}

int
eu_lua_script_exec(const TCHAR *fname)
{
    int status;
    char *path = NULL;
    lua_State *L = lua_open();
    if (L == NULL)
    {
        eu_logmsg("Scripts: %s, cannot create state[not enough memory]\n", __FUNCTION__);
        return -1;
    }
    if ((path = eu_utf16_utf8(fname, NULL)) == NULL)
    {
        eu_logmsg("Scripts: %s, eu_utf16_utf8 convert failed\n", __FUNCTION__);
        lua_close(L);
        return -1;
    }
    /* Stop collector during library initialization. */
    lua_gc(L, LUA_GCSTOP, 0);
    luaL_openlibs(L);
    lua_gc(L, LUA_GCRESTART, -1);
    status = dofile(L, path);
    lua_close(L);
    free(path);
    return status;
}

bool
do_lua_setting_path(eu_tabpage *pnode)
{
    TCHAR lua_path[ENV_LEN + 1] = {0};
    if (!pnode)
    {
        _sntprintf(lua_path, ENV_LEN, _T("LUA_PATH=%s\\lua\\?.lua;%s\\conf\\conf.d\\?.lua;%s\\script-opts\\?.lua"),
                   eu_module_path, eu_module_path, eu_config_path);
    }
    else
    {
        _sntprintf(lua_path, ENV_LEN, _T("LUA_PATH=%s\\lua\\?.lua;%s\\conf\\conf.d\\?.lua;%s\\script-opts\\?.lua;%s?.lua"),
                   eu_module_path, eu_module_path, eu_config_path, pnode->pathname);
    }
    return (_tputenv(lua_path) == 0);
}

int
do_lua_code(const char *s, const char *filename)
{
    int status;
    if (!s)
    {
        return -1;
    }
    lua_State *L = lua_open();
    if (L == NULL)
    {
        eu_logmsg("Scripts: %s, cannot create state[not enough memory]\n", __FUNCTION__);
        return -1;
    }
    print_version();
    /* Stop collector during library initialization. */
    lua_gc(L, LUA_GCSTOP, 0);
    luaL_openlibs(L);
    lua_gc(L, LUA_GCRESTART, -1);
    status = dostring(L, s, STR_NOT_NUL(filename) ? filename : "skylark");
    if (status)
    {
        eu_logmsg("Scripts: %s, dostring return false\n", __FUNCTION__);
    }
    lua_close(L);
    return status;
}

int
do_byte_code(eu_tabpage *pnode)
{
    HANDLE pfile = NULL;
    char *pbuffer = NULL;
    int status = 1;
    uint32_t written = 0;
    size_t buf_len = 0;
    char *utf8 = NULL;
    TCHAR filename[FILESIZE+1]= {0};
    TCHAR pname[MAX_PATH+1] = {0};
    TCHAR psave[MAX_BUFFER] = {0};
    if (!pnode)
    {
        return 1;
    }
    if (!pnode->presult)
    {
        pnode->result_show = on_result_launch(pnode);
    }
    if (!RESULT_SHOW(pnode))
    {
        return 1;
    }
    if ((pfile = util_mk_temp(pname, NULL)) == INVALID_HANDLE_VALUE)
    {
        eu_logmsg("Scripts: util_mk_temp return failed, cause:%lu\n", GetLastError());
        goto allclean;
    }
    if (!(pbuffer = util_strdup_content(pnode, &buf_len)))
    {
        eu_logmsg("Scripts: util_strdup_content failed\n");
        goto allclean;
    }
    if (!WriteFile(pfile, pbuffer, eu_uint_cast(buf_len), &written, NULL))
    {
        eu_logmsg("Scripts: WriteFile failed\n");
        goto allclean;
    }
    FlushFileBuffers(pfile);
    share_close(pfile);
    _sntprintf(filename, FILESIZE, _T("%s"), pnode->filename);
    if (pnode->pathname[1] == L':')
    {
        _sntprintf(psave, MAX_BUFFER, _T("%s%s.bin"), pnode->pathname, filename);
    }
    else if (GetEnvironmentVariable(_T("USERPROFILE"), psave, MAX_BUFFER) > 0)
    {
        _tcsncat(psave, _T("\\"), MAX_BUFFER);
        _tcsncat(psave, filename, MAX_BUFFER);
        _tcsncat(psave, _T(".bin"), MAX_BUFFER);
    }
    if (psave[0])
    {
        status = eu_lua_script_convert(pname, psave);
    }
allclean:
    pnode->presult->pwant = on_toolbar_no_highlight;
    on_result_lexer(pnode->presult);
    eu_window_resize();
    if (!status)
    {
        char u8_path[MAX_BUFFER] = {0};
        LOAD_I18N_RESSTR(IDS_LUA_CONV_SUCCESS, m_format);
        if ((utf8 = eu_utf16_utf8(m_format, NULL)) != NULL)
        {
            on_result_append_text_utf8(utf8, util_make_u8(psave, u8_path, MAX_BUFFER));
            free(utf8);
        }
    }
    else
    {
        LOAD_I18N_RESSTR(IDS_LUA_CONV_FAIL, m_format);
        if ((utf8 = eu_utf16_utf8(m_format, NULL)) != NULL)
        {
            on_result_append_text_utf8(utf8);
            free(utf8);
        }
    }
    if (pfile)
    {
        CloseHandle(pfile);
    }
    if (pbuffer)
    {
        free(pbuffer);
    }
    DeleteFile(pname);
    return status;
}

static int
script_process_dir(lua_State *L)
{
    size_t usz = 0;
    char *utf8path = NULL;
    if (!(utf8path = eu_utf16_utf8(eu_module_path, &usz)))
    {
        lua_pushnil(L);
        return 2;
    }
    lua_pushlstring(L, utf8path, usz-1);
    free(utf8path);
    return 1;
}

static int
script_config_dir(lua_State *L)
{
    size_t usz = 0;
    char *utf8path = NULL;
    if (!(utf8path = eu_utf16_utf8(eu_config_path, &usz)))
    {
        lua_pushnil(L);
        return 2;
    }
    lua_pushlstring(L, utf8path, usz-1);
    free(utf8path);
    return 1;
}

static int
script_mkdir(lua_State *L)
{
    int ret = 2;
    size_t sz = 0;
    const char *utf8path = luaL_checklstring(L, 1, &sz);
    wchar_t *path = utf8path ? eu_utf8_utf16(utf8path, &sz) : NULL;
    if (path)
    {
        CreateDirectoryW(path, NULL);
        ret = 1;
    }
    lua_pushinteger(L, ret);
    eu_safe_free(path);
    return ret;
}

static int
script_register_event(lua_State *L)
{
    int ret = 0;
    size_t sz = 0;
    obs_skylark obs = {0};
    const char *pname = luaL_checklstring(L, 2, &sz);
    obs.index = luaL_checkint(L, 1);
    if (obs.index >= SKYLARK_INIT && pname && sz > 0)
    {
        lua_Debug ar = {0};
        if (lua_getstack(L, 1, &ar) > 0 && lua_getinfo(L, "S", &ar) > 0 && STR_NOT_NUL(ar.source))
        {
            _snprintf(obs.pfile, MAX_BUFFER - 1, "%s", ar.source[0] == '@' ? &ar.source[1] : ar.source);
            _snprintf(obs.pname, MAX_PATH - 1, "%s", pname);
            cvector_push_back(psb_client, obs);
            ret = 1;  // 成功, 函数有一个返回值, 反之则返回nil
        }
    }
    lua_pushinteger(L, (lua_Integer)ret);
    return ret;
}

static void __stdcall
on_script_loader_request(PTP_CALLBACK_INSTANCE inst, PVOID lp, PTP_WAIT wait, TP_WAIT_RESULT result)
{
    HANDLE hfile;
    WIN32_FIND_DATA data;
    TCHAR filepath[MAX_BUFFER];
    _sntprintf(filepath, MAX_BUFFER, _T("%s\\lua\\loader\\*.lua"), eu_module_path);
    if ((hfile = FindFirstFile(filepath, &data)) != INVALID_HANDLE_VALUE)
    {
        char *u8 = NULL;
        TCHAR lua[MAX_BUFFER];
        do
        {
            if (WaitForSingleObject(eu_threadpool_handle(), 0) != WAIT_TIMEOUT)
            {
                eu_logmsg("Scripts: recv g_threadpool_sem, thread %lu exit ...\n", GetCurrentThreadId());
                break;
            }
            if (_tcscmp(data.cFileName, _T(".")) == 0 || _tcscmp(data.cFileName, _T("..")) == 0)
            {
                continue;
            }
            if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                continue;
            }
            if (_sntprintf(lua, MAX_BUFFER, _T("%s\\lua\\loader\\%s"), eu_module_path, data.cFileName) > 0 && (u8 = eu_utf16_utf8(lua, NULL)) != NULL)
            {
                if (do_lua_func(u8, "main", "") == SKYLARK_OK)
                {
                    eu_logmsg("Scripts: [%s] register\n", u8);
                }
                free(u8);
            }
        } while (FindNextFile(hfile, &data));
        FindClose(hfile);
    }
    on_script_loader_event(SKYLARK_INIT, NULL);
}

static const struct
luaL_Reg cb[] = {{"lprocessdir", script_process_dir},
                 {"lconfdir", script_config_dir},
                 {"lmkdir", script_mkdir},
                 {"register_event", script_register_event},
                 {NULL, NULL}
                };

int
luaopen_euapi(void *L)
{
    luaL_register(L, "euapi", cb);
    return 0;
}

void
on_script_loader_event(const int event, void *pnode)
{
    int index = on_tabpage_get_index((eu_tabpage *)pnode);
    for (size_t i = 0; i < cvector_size(psb_client); ++i)
    {
        if (psb_client[i].index == event)
        {
            if (event == SKYLARK_COMMANDS)
            {
                do_lua_point(psb_client[i].pfile, psb_client[i].pname, pnode);
            }
            else
            {
                do_lua_integer(psb_client[i].pfile, psb_client[i].pname, index);
            }
        }
    }
}

void
on_script_loader(void)
{
    if (eu_hwnd_self() == share_envent_get_hwnd())
    {
        TASK_ARG hv = {0};
        eu_threadpool_add(on_script_loader_request, &hv);
    }
}
