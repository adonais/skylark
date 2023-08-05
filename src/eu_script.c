/******************************************************************************
 * This file is part of Skylark project
 * Copyright ©2023 Hua andy <hua.andy@gmail.com>

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

#ifndef lua_pushglobaltable
#define lua_pushglobaltable(L) lua_pushvalue(L,LUA_GLOBALSINDEX)
#endif

static lua_State *pstate;

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
report(lua_State *L, int status)
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

static int
traceback(lua_State *L)
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
    lua_pushcfunction(L, traceback); /* push traceback function */
    lua_insert(L, base);             /* put it under chunk and args */
    status = lua_pcall(L, narg, (clear ? 0 : LUA_MULTRET), base);
    lua_remove(L, base); /* remove traceback function */
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
        return report(L, 1);
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
    return report(L, status);
}

/* Save or list bytecode. */
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
    return report(L, lua_pcall(L, 2, 0, 0));
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
        eu_logmsg("%s: cannot create state: not enough memory\n", __FUNCTION__);
        return -1;
    }
    /* Stop collector during library initialization. */
    lua_gc(L, LUA_GCSTOP, 0);
    luaL_openlibs(L);
    lua_gc(L, LUA_GCRESTART, -1);
#ifdef _UNICODE
    if ((filepath = eu_utf16_utf8(fname, NULL)) == NULL)
    {
        eu_logmsg("%s: eu_utf16_utf8 convert failed\n", __FUNCTION__);
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
            eu_logmsg("%s: eu_utf16_utf8 convert failed\n", __FUNCTION__);
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
        eu_logmsg("%s: cannot create state: not enough memory\n", __FUNCTION__);
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
            eu_logmsg("%s: lua_pcall failed\n", __FUNCTION__);
        }
    }
    lua_close(L);
    return status;
}

static int
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
        eu_logmsg("%s: cannot create state: not enough memory\n", __FUNCTION__);
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
        eu_logmsg("%s: cannot create state: not enough memory\n", __FUNCTION__);
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
            eu_logmsg("%s: lua_pcall failed\n", __FUNCTION__);
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
        eu_logmsg("%s: cannot create state: not enough memory\n", __FUNCTION__);
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
        eu_logmsg("%s: cannot create state: not enough memory\n", __FUNCTION__);
        return -1;
    }
    if ((path = eu_utf16_utf8(fname, NULL)) == NULL)
    {
        eu_logmsg("%s: eu_utf16_utf8 convert failed\n", __FUNCTION__);
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
        _sntprintf(lua_path, ENV_LEN, _T("LUA_PATH=%s\\conf\\conf.d\\?.lua;%s\\script-opts\\?.lua"), eu_module_path, eu_config_path);
    }
    else
    {
        _sntprintf(lua_path, ENV_LEN, _T("LUA_PATH=%s\\conf\\conf.d\\?.lua;%s\\script-opts\\?.lua;%s?.lua"), eu_module_path, eu_config_path, pnode->pathname);
    }
    return (_tputenv(lua_path) == 0);
}

int
do_lua_code(const char *s)
{
    int status;
    if (!s)
    {
        return -1;
    }
    lua_State *L = lua_open();
    if (L == NULL)
    {
        eu_logmsg("%s: cannot create state: not enough memory\n", __FUNCTION__);
        return -1;
    }
    print_version();
    /* Stop collector during library initialization. */
    lua_gc(L, LUA_GCSTOP, 0);
    luaL_openlibs(L);
    lua_gc(L, LUA_GCRESTART, -1);
    status = dostring(L, s, "line");
    if (status)
    {
        eu_logmsg("%s: dostring return false\n", __FUNCTION__);
    }
    lua_close(L);
    return status;
}

static int
do_jit_proc(const TCHAR *pname, const TCHAR *psave)
{
    int status = 1;
    TCHAR *penv = NULL;
    TCHAR *plua_env = NULL;
    if ((penv = _tgetenv(_T("LUA_PATH"))))
    {
        // 保存LUA_PATH环境遍历, 执行jit后再恢复
        size_t env_len = _tcslen(penv) + 16;
        plua_env = (TCHAR *)calloc(sizeof(TCHAR), env_len);
        _sntprintf(plua_env, env_len-1, _T("LUA_PATH=%s"), penv);
    }
    if (plua_env)
    {
        _tputenv(_T("LUA_PATH="));
        status = eu_lua_script_convert(pname, psave);
        _tputenv(plua_env);
        free(plua_env);
    }
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
        eu_logmsg("util_mk_temp return failed, cause:%lu\n", GetLastError());
        goto allclean;
    }
    if (!(pbuffer = util_strdup_content(pnode, &buf_len)))
    {
        eu_logmsg("util_strdup_content failed\n");
        goto allclean;
    }
    if (!WriteFile(pfile, pbuffer, eu_uint_cast(buf_len), &written, NULL))
    {
        eu_logmsg("WriteFile failed\n");
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
        status = do_jit_proc(pname, psave);
    }
allclean:
    pnode->presult->pwant = on_toolbar_no_highlight;
    on_proc_resize(NULL);
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
    int usz = 0;
    char *utf8path = NULL;
    wchar_t path[MAX_BUFFER] = {0};
    // 使用中间变量保存路径
    // 使用clang编译时, 直接转换eu_module_path导致lua crash, why?
    _snwprintf(path, MAX_BUFFER, _T("%s"), eu_module_path);
    if (!(utf8path = eu_utf16_utf8(path, (size_t *)&usz)))
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
    int usz = 0;
    char *utf8path = NULL;
    wchar_t path[MAX_BUFFER] = {0};
    // 使用中间变量保存路径
    _snwprintf(path, MAX_BUFFER, _T("%s"), eu_config_path);
    if (!(utf8path = eu_utf16_utf8(path, (size_t *)&usz)))
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
    int ret = 0;
    size_t sz = 0;
    const char *utf8path = luaL_checklstring(L, 1, &sz);
    wchar_t *path = utf8path ? eu_utf8_utf16(utf8path, &sz) : NULL;
    if (path)
    {
        ret = CreateDirectoryW(path, NULL);
        free(path);
    }
    lua_pushinteger(L, ret);
    return ret;
}

static const struct
luaL_Reg cb[] = {{"lprocessdir", script_process_dir}, {"lconfdir", script_config_dir}, {"lmkdir", script_mkdir}, {NULL, NULL}};

int
luaopen_euapi(void *L)
{
    luaL_register(L, "euapi", cb);
    return 0;
}
