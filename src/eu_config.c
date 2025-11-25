/*******************************************************************************
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
#include "framework.h"

#define ascii_special_symbol(ch) \
        ((ch > 0x20 && ch < 0x30)||(ch > 0x39 && ch < 0x41)||(ch > 0x5a && ch < 0x7f))

static void
on_config_setup_postion(const wchar_t **args, int arg_c, file_backup *pbak)
{
    LPWSTR *ptr_arg = NULL;
    if (args)
    {
        ptr_arg = (LPWSTR *)args;
    }
    else
    {
        ptr_arg = CommandLineToArgvW(GetCommandLineW(), &arg_c);
    }
    if (ptr_arg)
    {
        for (int i = 0; i < arg_c; ++i)
        {
            if (!_tcsncmp(ptr_arg[i], _T("-n"), 2) && _tcslen(ptr_arg[i]) > 2)
            {
                pbak->x = _tstoz(&ptr_arg[i][2]);
                if (pbak->x == -1)
                {
                    pbak->x = -2;
                }
            }
            else if (!_tcsncmp(ptr_arg[i], _T("-c"), 2) && _tcslen(ptr_arg[i]) > 2)
            {
                pbak->y = _tstoi(&ptr_arg[i][2]);
                if (pbak->y == -1)
                {
                    pbak->y = -2;
                }
            }
        }
        if (args != (const wchar_t **)ptr_arg)
        {
            LocalFree(ptr_arg);
        }
    }
}

static inline int
on_config_cvector_at(const file_backup *pbak, const TCHAR *pfile)
{
    for (int i = 0; i < eu_int_cast(cvector_size(pbak)); ++i)
    {
        if (pbak[i].rel_path[0] && _tcsicmp(pbak[i].rel_path, pfile) == 0)
        {
            return i;
        }
    }
    return -1;
}

static bool
on_config_open_args(file_backup **pbak)
{
    int arg_c = 0;
    bool ret = false;
    LPWSTR *args = CommandLineToArgvW(GetCommandLineW(), &arg_c);
    if (pbak && args && arg_c >= 2)
    {
        cvector_vector_type(file_backup) vpath = NULL;
        if ((ret = eu_config_parser_path(args, arg_c, &vpath)))
        {
            for (size_t i = 0; i < cvector_size(vpath); ++i)
            {
                if (vpath[i].rel_path[0])
                {
                    int at = on_config_cvector_at(*pbak, vpath[i].rel_path);
                    if (at >= 0)
                    {
                        if (!(vpath[i].x > 0 || vpath[i].y > 0))
                        {
                            vpath[i].x = (*pbak)[at].x;
                            vpath[i].y = (*pbak)[at].y;
                            vpath[i].postion = (*pbak)[at].postion;
                        }
                        vpath[i].view = (*pbak)[at].view;
                        cvector_erase(*pbak, (size_t)at);
                    }
                    cvector_push_back(*pbak, vpath[i]);
                }
            }
        }
        cvector_free(vpath);
    }
    if (args)
    {
        LocalFree(args);
    }
    return ret;
}

static int
on_config_parser_bakup(void *data, int count, char **column, char **names)
{
    file_backup filebak = {-1, -1, 0, -1, -1};
    file_backup **pbak = (file_backup **)data;
    for (int i = 0; i < count; ++i)
    {
        if (STRCMP(names[i], ==, "szTabId"))
        {
            filebak.tab_id = (short)atoi(column[i]);
        }
        else if (STRCMP(names[i], ==, "szRealPath"))
        {
            MultiByteToWideChar(CP_UTF8, 0, column[i], -1, filebak.rel_path, MAX_BUFFER);
        }
        else if (STRCMP(names[i], ==, "szBakPath"))
        {
            MultiByteToWideChar(CP_UTF8, 0, column[i], -1, filebak.bak_path, MAX_BUFFER);
        }
        else if (STRCMP(names[i], ==, "szMark"))
        {
            strncpy(filebak.mark_id, column[i], MAX_BUFFER-1);
        }
        else if (STRCMP(names[i], ==, "szFold"))
        {
            strncpy(filebak.fold_id, column[i], MAX_BUFFER-1);
        }
        else if (STRCMP(names[i], ==, "szLine"))
        {
            filebak.postion = _atoz(column[i]);
        }
        else if (STRCMP(names[i], ==, "szCp"))
        {
            filebak.cp = atoi(column[i]);
        }
        else if (STRCMP(names[i], ==, "szBakCp"))
        {
            filebak.bakcp = atoi(column[i]);
        }
        else if (STRCMP(names[i], ==, "szEol"))
        {
            filebak.eol = atoi(column[i]);
        }
        else if (STRCMP(names[i], ==, "szBlank"))
        {
            filebak.blank = atoi(column[i]);
        }
        else if (STRCMP(names[i], ==, "szHex"))
        {
            filebak.hex = atoi(column[i]);
        }
        else if (STRCMP(names[i], ==, "szFocus"))
        {
            filebak.focus = atoi(column[i]);
        }
        else if (STRCMP(names[i], ==, "szZoom"))
        {
            filebak.zoom = atoi(column[i]);
        }
        else if (STRCMP(names[i], ==, "szStatus"))
        {
            filebak.status = atoi(column[i]);
        }
        else if (STRCMP(names[i], ==, "szView"))
        {
            filebak.view = atoi(column[i]);
        }
    }
    if (filebak.rel_path[0] || filebak.bak_path[0])
    {
        cvector_push_back(*pbak, filebak);
    }
    return SKYLARK_OK;
}

static int
on_config_parser_one(void *data, int count, char **column, char **names)
{
    int tab_id = -1;
    bool done = false;
    file_backup filebak = {-1, -1, 0, -1, -1};
    file_backup **pbak = (file_backup **)data;
    if (cvector_size(*pbak) == 1)
    {
        done = true;
        tab_id = (*pbak)[0].tab_id;
    }
    for (int i = 0; i < count; ++i)
    {
        if (STRCMP(names[i], ==, "szTabId"))
        {
            filebak.tab_id = (short)atoi(column[i]);
        }
        else if (STRCMP(names[i], ==, "szRealPath"))
        {
            MultiByteToWideChar(CP_UTF8, 0, column[i], -1, filebak.rel_path, MAX_BUFFER);
        }
        else if (STRCMP(names[i], ==, "szBakPath"))
        {
            MultiByteToWideChar(CP_UTF8, 0, column[i], -1, filebak.bak_path, MAX_BUFFER);
        }
        else if (STRCMP(names[i], ==, "szMark"))
        {
            strncpy(filebak.mark_id, column[i], MAX_BUFFER-1);
        }
        else if (STRCMP(names[i], ==, "szFold"))
        {
            strncpy(filebak.fold_id, column[i], MAX_BUFFER-1);
        }
        else if (STRCMP(names[i], ==, "szLine"))
        {
            filebak.postion = _atoz(column[i]);
        }
        else if (STRCMP(names[i], ==, "szCp"))
        {
            filebak.cp = atoi(column[i]);
        }
        else if (STRCMP(names[i], ==, "szBakCp"))
        {
            filebak.bakcp = atoi(column[i]);
        }
        else if (STRCMP(names[i], ==, "szEol"))
        {
            filebak.eol = atoi(column[i]);
        }
        else if (STRCMP(names[i], ==, "szBlank"))
        {
            filebak.blank = atoi(column[i]);
        }
        else if (STRCMP(names[i], ==, "szHex"))
        {
            filebak.hex = atoi(column[i]);
        }
        else if (STRCMP(names[i], ==, "szFocus"))
        {
            filebak.focus = atoi(column[i]);
        }
        else if (STRCMP(names[i], ==, "szZoom"))
        {
            filebak.zoom = atoi(column[i]);
        }
        else if (STRCMP(names[i], ==, "szStatus"))
        {
            filebak.status = atoi(column[i]);
        }
        else if (STRCMP(names[i], ==, "szView"))
        {
            filebak.view = atoi(column[i]);
        }
    }
    if (done && tab_id >= 0 && filebak.tab_id == tab_id && (filebak.rel_path[0] || filebak.bak_path[0]))
    {
        filebak.focus = 1;
        cvector_push_back(*pbak, filebak);
        return SKYLARK_SQL_END;
    }
    return SKYLARK_OK;
}

static unsigned __stdcall
on_config_load_file(void *lp)
{
    int error = 0;
    size_t count = 0;
    file_backup bak = {-1, -1, 0, -1, -1};
    cvector_vector_type(file_backup) vbak = NULL;
    if (eu_get_config()->m_session)
    {
        if (eu_get_config()->m_instance)
        {
            if (eu_config_check_arg(NULL, 0, NULL, &bak.tab_id))
            {
                cvector_push_back(vbak, bak);
                error = on_sql_do_session("SELECT * FROM skylark_session;", on_config_parser_one, (void *)&vbak);
            }
        }
        else
        {   /* on_config_parser_bakup导致工作目录变更 */
            on_sql_do_session("SELECT * FROM skylark_session ORDER BY szTabId ASC;", on_config_parser_bakup, (void *)&vbak);
        }
    }
    else
    {
        on_sql_do_session("SELECT szVersion FROM skylar_ver;", NULL, NULL);
    }
    if (error == 0 && on_config_open_args(&vbak) && vbak)
    {   // 当定位目录时, 修正可能错误的焦点标签
        size_t save = 0;
        for (; count < cvector_size(vbak) - 1; ++count)
        {
            if (vbak[count].focus)
            {
                save = count;
                vbak[count].focus = 0;
            }
        }
        if (url_que_mark(vbak[count].rel_path))
        {
            vbak[save].focus = 1;
        }
        else
        {
            vbak[count].focus = 1;
        }
        eu_logmsg("Config: run with arguments\n");
    }
    /* 正常标签与空标签一起关闭时可能没有获取焦点 */
    if ((count = cvector_size(vbak)) == 1 && (!vbak[0].focus))
    {
        vbak[0].focus = 1;
    }
    else if (count < 1)
    {
        bak.focus = 1;
        bak.rel_path[0] = L'\0';
        cvector_push_back(vbak, bak);
        ++count;
    }
    share_send_msg(vbak, count);
    cvector_free(vbak);
    return 0;
}

static bool
on_config_create_accel(void)
{
    bool ret = false;
    eue_accel *p = eu_get_accel();
    if (p && p->accel_num > 0)
    {
        int i = 0;
        uint16_t old[MAX_ACCELS] = {0};
        for (; i < p->accel_num; ++i)
        {
            if (ascii_special_symbol(p->accel_ptr[i].key) && !(p->accel_ptr[i].fVirt & FVIRTKEY))
            {
                int16_t key = VkKeyScanEx(p->accel_ptr[i].key, GetKeyboardLayout(0));
                if ((key &= 0xff) != -1)
                {
                    old[i] = p->accel_ptr[i].key;
                    p->accel_ptr[i].key = key;
                    p->accel_ptr[i].fVirt |= FVIRTKEY;
                }
            }
        }
        p->haccel = CreateAcceleratorTable(p->accel_ptr, p->accel_num);
        if (p->haccel)
        {   /* 恢复原数据供菜单栏识别 */
            for (i = 0; i < p->accel_num; ++i )
            {
                if (old[i] > 0)
                {
                    p->accel_ptr[i].key = old[i];
                    p->accel_ptr[i].fVirt &= ~FVIRTKEY;
                }
            }
            ret = true;
        }
        else
        {
            eu_logmsg("Config: createAcceleratorTable failed, cause: %lu\n", GetLastError());
        }
    }
    return ret;
}

static bool
on_config_sync_snippet(void)
{
    TCHAR p1[MAX_BUFFER] = {0};
    TCHAR p2[MAX_BUFFER] = {0};
    TCHAR *snippets[] = {_T("cpp.snippets"),
                         _T("cshape.snippets"),
                         _T("css.snippets"),
                         _T("golang.snippets"),
                         _T("javascript.snippets"),
                         _T("json.snippets"),
                         _T("julia.snippets"),
                         _T("luascript.snippets"),
                         _T("perl.snippets"),
                         _T("php.snippets"),
                         _T("rust.snippets"),
                         _T("text.snippets"),
                         _T("verilog.snippets"),
                         _T("pascal.snippets"),
                         _T("tcl.snippets"),
                         NULL};
    _sntprintf(p2, MAX_BUFFER, _T("%s\\snippets"), eu_config_path);
    if (!eu_exist_dir(p2))
    {
        if (!eu_mk_dir(p2))
        {
            return false;
        }
    }
    for (int i = 0; snippets[i]; ++i)
    {
        _sntprintf(p1, MAX_BUFFER, _T("%s\\conf\\conf.d\\snippets\\%s"), eu_module_path, snippets[i]);
        _sntprintf(p2, MAX_BUFFER, _T("%s\\snippets\\%s"), eu_config_path, snippets[i]);
        CopyFile(p1, p2, TRUE);
    }
    return true;
}

static char*
on_config_lua_get(const wchar_t *file)
{
    char *lua_path = NULL;
    TCHAR path[MAX_BUFFER] = {0};
    int  m = _sntprintf(path, MAX_BUFFER, _T("%s\\conf\\conf.d\\%s"), eu_module_path, file);
    if ((m > 0 && m < MAX_BUFFER) && ((lua_path = eu_utf16_utf8(path, NULL)) != NULL))
    {
        return lua_path;
    }
    return NULL;
}

static wchar_t*
on_config_lua_execute(const wchar_t *file)
{
    TCHAR *path = NULL;
    char *lua_path = on_config_lua_get(file);
    if (lua_path)
    {
        path = do_lua_parser_path(lua_path);
        free(lua_path);
    }
    return path;
}

static int
on_config_edition(const char *str)
{
    int v = 0;
    if (str)
    {
        int i = 0;
        const char *p = str;
        for (i = 100, v = atoi(p) * 10000; i && (p = strchr(p, '.')) != NULL; i /= 100)
        {
            v += (atoi(++p) * i);
        }
    }
    return v;
}

static int
on_config_skyver_callbak(void *data, int count, char **column, char **names)
{
    int *pd = (int *)data;
    for (int i = 0; i < count; ++i)
    {
        if (STRCMP(names[i], ==, "szExtra"))
        {
            *pd = atoi(column[i]);
        }
    }
    return 0;
}

static bool
on_config_create_cache(void)
{
    bool ret = false;
    TCHAR cache_path[MAX_BUFFER] = {0};
    if (_sntprintf(cache_path, MAX_BUFFER, _T("%s\\cache"), eu_config_path) > 0)
    {
        if (!eu_exist_dir(cache_path))
        {
            ret = eu_mk_dir(cache_path);
        }
        else
        {
            ret = true;
        }
    }
    return ret;
}

static bool
on_config_update_db(void)
{
    if (eu_hwnd_self() == share_envent_get_hwnd())
    {
        int v = -1;
        on_sql_post("SELECT szExtra FROM skylar_ver;", on_config_skyver_callbak, &v);
        if (v == VERSION_UPDATE_COMPLETED)
        {
            if (on_update_excute())
            {
                on_update_sql();
                eu_session_backup(SESSION_CONFIG);
                return false;
            }
            else if (eu_get_config()->upgrade.flags != VERSION_LATEST)
            {
                on_update_sql();
            }
        }
    }
    return true;
}

void
on_config_file_url(wchar_t *path, int len, const wchar_t *p)
{
    if (STR_NOT_NUL(path) && p && len > 0)
    {
        if (url_has_file(path))
        {
            if (wcslen(p) > 4 && wcsncmp(p, L":///", 4) == 0)
            {   // 加1, 是要把字符串结束符0也拷贝进去
                memmove(path, &p[4], sizeof(wchar_t) * (wcslen(&p[4]) + 1));
                len = (int)wcslen(path);
                while (--len > 0)
                {
                    if (path[len] == L'/' || path[len] == L'!')
                    {
                        path[len] = L'\0';
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }
        util_unix2path(path, (int)wcslen(path));
    }
}

bool
eu_config_check_arg(const wchar_t **args, int arg_c, const wchar_t *argument, int *buf)
{
    bool ret = false;
    LPWSTR *ptr_arg = NULL;
    if (args)
    {
        ptr_arg = (LPWSTR *)args;
    }
    else
    {
        ptr_arg = CommandLineToArgvW(GetCommandLineW(), &arg_c);
    }
    if (ptr_arg)
    {
        for (int i = 1; i < arg_c; ++i)
        {
            if (argument)
            {
                if (!wcscmp(ptr_arg[i], argument))
                {
                    ret = true;
                    break;
                }
            }
            else if (ptr_arg[i][0] == L'#')
            {
                if (buf)
                {
                    *buf = _wtoi(&ptr_arg[i][1]);
                }
                ret = true;
                break;
            }
        }
        if (ptr_arg != (LPWSTR *)args)
        {
            LocalFree(ptr_arg);
        }
    }
    return ret;
}

bool
eu_config_parser_path(const wchar_t **args, int arg_c, file_backup **pbak)
{
    bool ret = false;
    LPWSTR *ptr_arg = NULL;
    if (args)
    {
        ptr_arg = (LPWSTR *)args;
    }
    else
    {
        ptr_arg = CommandLineToArgvW(GetCommandLineW(), &arg_c);
    }
    if (ptr_arg && pbak)
    {
        const bool hex = eu_config_check_arg(ptr_arg, arg_c, L"-hex", NULL);
        const bool view = eu_config_check_arg(ptr_arg, arg_c, L"-v1", NULL);
        for (int i = 1; i < arg_c; ++i)
        {
            file_backup data = {-1, -1, 0, -1};
            data.hex = (int)hex;
            data.view = (int)view;
            on_config_setup_postion(ptr_arg, arg_c, &data);
            if (wcsncmp(ptr_arg[i], L"-restart", 8) == 0)
            {
                i += 2;
                continue;
            }
            if (wcsncmp(ptr_arg[i], L"-lua", 4) == 0)
            {
                if ((i + 1) < arg_c && wcsncmp(ptr_arg[i+1], L"-b", 2) == 0)
                {
                    i += 3;
                }
                else
                {
                    ++i;
                }
                continue;
            }
            if (wcsncmp(ptr_arg[i], L"-lua", 4) == 0)
            {
                if ((i + 1) < arg_c && wcsncmp(ptr_arg[i+1], L"-b", 2) == 0)
                {
                    i += 3;
                }
                else
                {
                    ++i;
                }
                continue;
            }
            if (util_under_wine() && wcsicmp(ptr_arg[i], L"Z:") == 0)
            {   // wine启动器自动添加了此参数
                continue;
            }
            if (ptr_arg[i][0] != L'-' && wcslen(ptr_arg[i]) > 0)
            {
                WCHAR *p = NULL;
                size_t len = 0;
                bool que = false;
                bool star = false;
                if ((p = wcschr(ptr_arg[i], L':')) != NULL)
                {   // 处理以绝对路径打开的文件或目录
                    wcsncpy(data.rel_path, ptr_arg[i], MAX_BUFFER);
                    on_config_file_url(data.rel_path, (int)wcslen(data.rel_path), p);
                    len = wcslen(data.rel_path);
                    star = len > 1 && data.rel_path[len - 1] == L'*';
                    if ((que = (len > 1)) && data.rel_path[len - 1] == L'?')
                    {
                        ret |= 0x1;
                    }
                    else if (!url_has_remote(data.rel_path) && (eu_exist_dir(data.rel_path) || star) && len < MAX_BUFFER - 2)
                    {
                        if (star)
                        {
                            data.rel_path[len - 1] = L'\0';
                            --len;
                        }
                        if (data.rel_path[len - 1] == L'\\')
                        {
                            data.rel_path[len- 1] = L'\0';
                        }
                        if (util_bfs_search(data.rel_path, pbak, &data))
                        {
                            ret |= 0x1;
                        }
                        data.rel_path[0] = L'\0';
                    }
                }
                else
                {   /* 处理以相对路径打开的文件或目录 */
                    GetFullPathNameW(ptr_arg[i], MAX_BUFFER, data.rel_path, &p);
                    len = wcslen(data.rel_path);
                    star = len > 1 && data.rel_path[len - 1] == L'*';
                    if ((que = (len > 1)) && data.rel_path[len - 1] == L'?')
                    {
                        ret |= 0x1;
                    }
                    else if ((eu_exist_dir(data.rel_path) || star) && len < MAX_BUFFER - 2)
                    {
                        if (star)
                        {
                            data.rel_path[len - 1] = L'\0';
                            --len;
                        }
                        if (data.rel_path[len - 1] == L'\\')
                        {
                            data.rel_path[len- 1] = L'\0';
                        }
                        if (util_bfs_search(data.rel_path, pbak, &data))
                        {
                            ret |= 0x1;
                        }
                        data.rel_path[0] = L'\0';
                    }
                }
                if (data.rel_path[0])
                {
                    cvector_push_back(*pbak, data);
                    ret |= 0x1;
                }
            }
        }
    }
    if (ptr_arg && args != (const wchar_t **)ptr_arg)
    {
        LocalFree(ptr_arg);
    }
    return ret;
}

bool
on_config_lua_run(const wchar_t *file, const char *parg)
{
    bool ret = false;
    char *lua_path = on_config_lua_get(file);
    if (lua_path)
    {
        ret = (do_lua_func(lua_path, "run", parg) == 0);
        free(lua_path);
    }
    return ret;
}

int
on_config_accel_loader(void)
{
    eue_accel *paccel = eu_get_accel();
    HACCEL h = paccel ? paccel->haccel : NULL;
    if (h && on_config_lua_run(_T("eu_input.lua"), ""))
    {
        HWND hwnd = eu_hwnd_self();
        HMENU menu = hwnd ? GetMenu(hwnd) : NULL;
        DestroyAcceleratorTable(h);
        on_config_create_accel();
        if (menu && eu_get_config()->m_menubar)
        {
            SetMenu(hwnd, NULL);
            SetMenu(hwnd, i18n_load_menu(IDC_SKYLARK));
        }
        return 0;
    }
    return SKYLARK_ACCEL_FAILED;
}

bool
eu_config_load_accel(void)
{
    return on_config_lua_run(_T("eu_input.lua"), "");
}

bool
eu_config_load_toolbar(void)
{
    return on_config_lua_run(_T("eu_gui.lua"), "");
}

bool
eu_config_load_main(void)
{
    return on_config_lua_run(_T("eu_main.lua"), "");
}

bool
eu_config_load_docs(void)
{
    int  m = 0;
    bool ret = false;
    char *lua_path = NULL;
    TCHAR path[MAX_BUFFER] = {0};
    m = _sntprintf(path, MAX_BUFFER, _T("%s\\conf\\conf.d\\eu_docs.lua"), eu_module_path);
    if (!(m > 0 && m < MAX_BUFFER) || ((lua_path = eu_utf16_utf8(path, NULL)) == NULL))
    {
        goto load_fail;
    }
    if (!on_config_sync_snippet())
    {
        goto load_fail;
    }
    if (do_lua_parser_doctype(lua_path, "fill_my_docs"))
    {
        eu_logmsg("Config: eu_docs exec failed\n");
        goto load_fail;
    }
    ret = true;
load_fail:
    eu_safe_free(lua_path);
    return ret;
}

/**************************************************************************************
 * 调用euapi导出函数之前, 应该先初始化euapi路径变量, 可以写入dllmain
 * eu_module_path是主进程路径, eu_config_path是配置文件夹路径
 * euapi.dll大量函数依赖这两个全局变量, 所以应该最先初始化
 **************************************************************************************/
bool
eu_config_init_path(void)
{
    bool ret = false;
    TCHAR *path = NULL;
    const TCHAR *p = NULL;
    do
    {
        if (!eu_process_path()[0])
        {
            break;
        }
        if ((p = _wgetenv(EU_CONFIG_DIR)))
        {
            _sntprintf(eu_config_path, MAX_BUFFER - 1, _T("%s"), p);
            ret = true;
            break;
        }
        if ((path = on_config_lua_execute(_T("eu_portable.lua"))) && path[0])
        {
            ret = true;
            break;
        }
        if (path || (path = (TCHAR *)calloc(sizeof(TCHAR), MAX_BUFFER)))
        {
            const GUID fid = FOLDERID_RoamingAppData;
            _sntprintf(path, MAX_BUFFER - 1, _T("%s\\conf"), eu_module_path);
            if (!(ret = util_try_path(path)) && util_shell_path(&fid, path, MAX_BUFFER))
            {
                _tcsncat(path, _T("\\skylark_editor"), MAX_BUFFER);
                ret = true;
                break;
            }
        }
    } while(0);
    if (ret)
    {
        if (STR_NOT_NUL(path))
        {
            util_unix2path(path, (int)_tcslen(path));
            _sntprintf(eu_config_path, MAX_BUFFER - 1, _T("%s=%s"), EU_CONFIG_DIR, path);
            if (_wputenv(eu_config_path))
            {
                *eu_config_path = 0;
                ret = false;
            }
            else
            {
                _sntprintf(eu_config_path, MAX_BUFFER - 1, _T("%s"), path);
                ret = true;
            }
        }
        if (ret)
        {
            if (!eu_exist_dir(eu_config_path))
            {
                ret = eu_mk_dir(eu_config_path);
            }
            if ((ret = on_config_create_cache()))
            {
                ret = do_lua_setting_path(NULL);
            }
        }
    }
    eu_safe_free(path);
    return ret;
}

bool
eu_config_load_files(void)
{
    if (on_config_update_db())
    {
        CloseHandle((HANDLE) _beginthreadex(NULL, 0, on_config_load_file, NULL, 0, NULL));
        return on_config_create_accel();
    }
    return false;
}
