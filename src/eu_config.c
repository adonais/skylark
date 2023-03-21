/*******************************************************************************
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
            }
            else if (!_tcsncmp(ptr_arg[i], _T("-c"), 2) && _tcslen(ptr_arg[i]) > 2)
            {
                pbak->y = _tstoi(&ptr_arg[i][2]);
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

static void
on_config_open_args(file_backup **pbak)
{
    int  arg_c = 0;
    LPWSTR *args = CommandLineToArgvW(GetCommandLineW(), &arg_c);
    if (args == NULL || pbak == NULL)
    {
        return;
    }
    if (arg_c >= 2)
    {
        cvector_vector_type(file_backup) vpath = NULL;
        if (eu_config_parser_path(args, arg_c, &vpath))
        {
            for (size_t i = 0; i < cvector_size(vpath); ++i)
            {
                if (vpath[i].rel_path[0])
                {
                    int at = on_config_cvector_at(*pbak, vpath[i].rel_path);
                    if (at >= 0 && (vpath[i].x > 0 || vpath[i].y > 0 || vpath[i].hex))
                    {
                        cvector_erase(*pbak, at);
                    }
                    cvector_push_back(*pbak, vpath[i]);
                }
            }
        }
        cvector_free(vpath);
    }
    LocalFree(args);
}

static int
on_config_parser_bakup(void *data, int count, char **column, char **names)
{
    file_backup *pbak = NULL;
    file_backup filebak = {0};
    if (data)
    {
        pbak = *(file_backup **)data;
    }
    for (int i = 0; i < count; ++i)
    {
        if (STRCMP(names[i], ==, "szTabId"))
        {
            filebak.tab_id = (short)atoi(column[i]);
        }
        else if (STRCMP(names[i], ==, "szRealPath"))
        {
            MultiByteToWideChar(CP_UTF8, 0, column[i], -1, filebak.rel_path, MAX_PATH);
        }
        else if (STRCMP(names[i], ==, "szBakPath"))
        {
            MultiByteToWideChar(CP_UTF8, 0, column[i], -1, filebak.bak_path, MAX_PATH);
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
            if (!filebak.focus)
            {
                filebak.focus = -1;
            }
        }
        else if (STRCMP(names[i], ==, "szZoom"))
        {
            filebak.zoom = atoi(column[i]);
        }
        else if (STRCMP(names[i], ==, "szStatus"))
        {
            filebak.status = atoi(column[i]);
        }
    }
    if (filebak.rel_path[0] || filebak.bak_path[0])
    {
        cvector_push_back(pbak, filebak);
    }
    if (data)
    {
        *(file_backup **)data = pbak;
    }
    return 0;
}

static unsigned __stdcall
on_config_load_file(void *lp)
{
    int err = 0;
    cvector_vector_type(file_backup) vbak = NULL;
    if (eu_get_config()->m_session)
    {
        err = on_sql_do_session("SELECT * FROM skylark_session;", on_config_parser_bakup, (void *)&vbak); // 这里又导致工作目录变更了
    }
    else
    {
        err = on_sql_do_session("SELECT * FROM skylar_ver;", NULL, NULL);
    }
    on_config_open_args(&vbak);
    printf("vbak size = %zu\n", cvector_size(vbak));
    if (cvector_size(vbak) < 1)
    {
        file_backup bak = {0};
        share_send_msg(&bak);
    }
    else
    {
        for (size_t i = 0; i < cvector_size(vbak); ++i)
        {
            if (vbak[i].rel_path[0] || vbak[i].bak_path[0])
            {
                share_send_msg(&vbak[i]);
            }
        }
    }
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
        {   // 恢复原数据
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
            printf("CreateAcceleratorTable failed, cause: %lu\n", GetLastError());
        }
    }
    return ret;
}

static bool
on_config_sync_snippet(void)
{
    TCHAR p1[MAX_PATH+1] = {0};
    TCHAR p2[MAX_PATH+1] = {0};
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
                         NULL};
    _sntprintf(p2, MAX_PATH, _T("%s\\conf\\snippets"), eu_module_path);
    if (!eu_exist_dir(p2))
    {
        if (!eu_mk_dir(p2))
        {
            return false;
        }
    }
    for (int i = 0; snippets[i]; ++i)
    {
        _sntprintf(p1, MAX_PATH, _T("%s\\conf\\conf.d\\snippets\\%s"), eu_module_path, snippets[i]);
        _sntprintf(p2, MAX_PATH, _T("%s\\conf\\snippets\\%s"), eu_module_path, snippets[i]);
        CopyFile(p1, p2, TRUE);
    }
    return true;
}

static bool
on_config_lua_execute(const wchar_t *file)
{
    bool ret = false;
    char *lua_path = NULL;
    TCHAR path[MAX_PATH+1] = {0};
    int  m = _sntprintf(path, MAX_PATH, _T("%s\\conf\\conf.d\\%s"), eu_module_path, file);
    if ((m > 0 && m < MAX_PATH) && ((lua_path = eu_utf16_utf8(path, NULL)) != NULL))
    {
        ret = (do_lua_func(lua_path, "run", "") == 0);
        free(lua_path);
    }
    return ret;
}

static int
on_config_skyver_callbak(void *data, int count, char **column, char **names)
{
    UNREFERENCED_PARAMETER(data);
    UNREFERENCED_PARAMETER(count);
    UNREFERENCED_PARAMETER(names);
    int status = atoi(column[0]);
    if (data)
    {
        *(int *)data = status;
    }
    return (int)(status == VERSION_UPDATE_COMPLETED);
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
eu_config_check_arg(const wchar_t **args, int arg_c, const wchar_t *argument)
{
    bool ret = false;
    LPWSTR *ptr_arg = NULL;
    if (!argument)
    {
        return false;
    }
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
            if (!_tcscmp(ptr_arg[i], argument))
            {
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
        for (int i = 1; i < arg_c; ++i)
        {
            file_backup data = {-1, -1};
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
            if (ptr_arg[i][0] != L'-' && wcslen(ptr_arg[i]) > 0)
            {
                WCHAR *p = NULL;
                size_t len = 0;
                if ((p = wcschr(ptr_arg[i], L':')) != NULL)
                {   // 处理以绝对路径打开的文件或目录
                    wcsncpy(data.rel_path, ptr_arg[i], MAX_PATH);
                    on_config_file_url(data.rel_path, (int)wcslen(data.rel_path), p);
                    len = wcslen(data.rel_path);
                    if (!url_has_remote(data.rel_path) && eu_exist_dir(data.rel_path) && len < MAX_PATH - 2)
                    {
                        if (data.rel_path[len - 1] != L'\\')
                        {
                            data.rel_path[len++] = L'\\';
                        }
                        data.rel_path[len] = L'*';
                    }
                }
                else
                {   // 处理以相对路径打开的文件或目录
                    GetFullPathNameW(ptr_arg[i], MAX_PATH, data.rel_path, &p);
                    len = wcslen(data.rel_path);
                    if (eu_exist_dir(data.rel_path) && len < MAX_PATH - 2)
                    {
                        if (data.rel_path[len - 1] != L'\\')
                        {
                            data.rel_path[len++] = L'\\';
                        }
                        data.rel_path[len] = L'*';
                    }
                }
                if (eu_config_check_arg(ptr_arg, arg_c, L"-hex"))
                {
                    data.hex = 1;
                }
                if (true)
                {
                    on_config_setup_postion(ptr_arg, arg_c, &data);
                    cvector_push_back(*pbak, data);
                }
                if (!ret)
                {
                    ret = true;
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
eu_config_load_accel(void)
{
    return on_config_lua_execute(_T("eu_input.lua"));
}

bool
eu_config_load_toolbar(void)
{
    return on_config_lua_execute(_T("eu_gui.lua"));
}

bool
eu_config_load_main(void)
{
    return on_config_lua_execute(_T("eu_main.lua"));
}

bool
eu_config_load_docs(void)
{
    int  m = 0;
    bool ret = false;
    char *lua_path = NULL;
    TCHAR path[MAX_PATH+1] = {0};
    m = _sntprintf(path, MAX_PATH, _T("%s\\conf\\conf.d\\eu_docs.lua"), eu_module_path);
    if (!(m > 0 && m < MAX_PATH) || ((lua_path = eu_utf16_utf8(path, NULL)) == NULL))
    {
        goto load_fail;
    }
    if (!on_config_sync_snippet())
    {
        goto load_fail;
    }
    if (do_lua_parser_doctype(lua_path, "fill_my_docs"))
    {
        printf("eu_docs exec failed\n");
        goto load_fail;
    }
    ret = true;
load_fail:
    eu_safe_free(lua_path);
    return ret;
}

bool
eu_config_load_files(void)
{
    HWND hwnd = eu_hwnd_self();
    HWND share = share_envent_get_hwnd();
    if (hwnd == share)
    {
        int err = on_sql_post("SELECT szExtra FROM skylar_ver;", on_config_skyver_callbak, NULL);
        if (err == SQLITE_ABORT)
        {
            _tputenv(_T("OPEN_FROM_SQL="));
            if (on_update_do())
            {
                on_update_sql();
                eu_save_config();
                return false;
            }
            else if (eu_get_config()->upgrade.flags != VERSION_LATEST)
            {
                on_update_sql();
            }
        }
    }
    CloseHandle((HANDLE) _beginthreadex(NULL, 0, on_remote_load_config, NULL, 0, NULL));
    CloseHandle((HANDLE) _beginthreadex(NULL, 0, on_config_load_file, NULL, 0, NULL));
    return on_config_create_accel();
}
