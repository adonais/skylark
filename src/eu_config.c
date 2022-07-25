/*******************************************************************************
 * This file is part of Skylark project
 * Copyright ©2022 Hua andy <hua.andy@gmail.com>

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

bool WINAPI
eu_config_parser_path(wchar_t **args, int argc, wchar_t *path)
{
    bool ret = false;
    int arg_c = argc;
    LPWSTR *ptr_arg = NULL;
    if (args)
    {
        ptr_arg = args;
    }
    else
    {
        ptr_arg = CommandLineToArgvW(GetCommandLineW(), &arg_c);
    }
    if (ptr_arg)
    {
        for (int i = 1; i < arg_c; ++i)
        {
            if (_tcsncmp(ptr_arg[i], _T("-"), 1) == 0)
            {
                continue;
            }
            if (_tcsncmp(ptr_arg[i], _T("-restart"), 8) == 0)
            {
                ++i;
                continue;
            }
            if (_tcsncmp(ptr_arg[i], _T("-lua"), 4) == 0)
            {
                if ((i + 1) < arg_c && _tcsncmp(ptr_arg[i+1], _T("-b"), 2) == 0)
                {
                    i += 3;
                }
                else
                {
                    ++i;
                }
                continue;
            }
            if (_tcslen(ptr_arg[i]) > 0)
            {
                TCHAR *p = NULL;
                if ((p = _tcschr(ptr_arg[i], ':')) != NULL)
                {
                    _tcsncpy(path, ptr_arg[i], MAX_PATH - 1);
                }
                else
                {
                    GetFullPathName(ptr_arg[i], MAX_PATH, path, &p);
                    if (!p && _tcslen(path) < MAX_PATH - 2)
                    {
                        path[_tcslen(path)] = _T('*');
                    }
                    else if (eu_exist_dir(path) && _tcslen(path) < MAX_PATH - 2)
                    {
                        if (path[_tcslen(path) - 1] != _T('\\'));
                        {
                            path[_tcslen(path)] = _T('\\');
                        }
                        path[_tcslen(path)] = _T('*');
                    }
                }
                ret = true;
                break;
            }
        }
        if (ptr_arg != args)
        {
            LocalFree(ptr_arg);
        }
    }
    return ret;
}

void WINAPI
eu_postion_setup(wchar_t **args, int argc, file_backup *pbak)
{
    int arg_c = argc;
    LPWSTR *ptr_arg = NULL;
    if (args)
    {
        ptr_arg = args;
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
                pbak->x = _tstoi64(&ptr_arg[i][2]);
            }
            else if (!_tcsncmp(ptr_arg[i], _T("-c"), 2) && _tcslen(ptr_arg[i]) > 2)
            {
                pbak->y = _tstoi(&ptr_arg[i][2]);
            }
        }
        if (ptr_arg != args)
        {
            LocalFree(ptr_arg);
        }
    }
}

bool WINAPI
eu_check_arg(const wchar_t **args, int argc, const wchar_t *argument)
{
    bool ret = false;
    int arg_c = argc;
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
        if (ptr_arg != args)
        {
            LocalFree(ptr_arg);
        }
    }
    return ret;
}

static int
on_config_file_args(void)
{
    int  ret = 1;
    int  arg_c = 0;
    file_backup bak = {0};
    LPWSTR *args = CommandLineToArgvW(GetCommandLineW(), &arg_c);
    if (args == NULL)
    {
        return 1;
    }
    else if (arg_c >= 2 && eu_config_parser_path(args, arg_c, bak.rel_path))
    {
        eu_postion_setup(args, arg_c, &bak);
        share_send_msg(&bak);
        ret = 0;
    }
    LocalFree(args);
    return ret;
}

static int
on_config_parser_bakup(void *data, int count, char **column, char **names)
{
    int index = -1;
    *(int *)data = 0;
    file_backup filebak = {0};
    wchar_t path[MAX_PATH] = {0};
    wchar_t *open_sql = _tgetenv(_T("OPEN_FROM_SQL"));
    if (open_sql && !eu_config_parser_path(NULL, 0, path))
    {
        printf("eu_config_parser_path return false\n");
        return 1;
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
            filebak.postion = _atoi64(column[i]);
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
    if (open_sql)
    {
        if (_tcslen(path) > 0 && !_tcsicmp(filebak.rel_path, path))
        {
            share_send_msg(&filebak);
            return 1;
        }
    }
    else
    {
        if (filebak.rel_path[0] || filebak.bak_path[0])
        {
            if (!_tcsicmp(filebak.rel_path, path))
            {
                eu_postion_setup(NULL, 0, &filebak);
            }
            share_send_msg(&filebak);
        }
    }
    return 0;
}

static unsigned __stdcall
on_config_load_file(void *lp)
{
    int is_blank = 1;
    wchar_t *open_sql = _tgetenv(_T("OPEN_FROM_SQL"));
    if (open_sql || !eu_get_config()->m_instance)
    {
        if (open_sql || eu_get_config()->m_session)
        {
            int err = on_sql_do_session("SELECT * FROM skylark_session;", on_config_parser_bakup, &is_blank);
            if (err == SQLITE_ABORT)
            {
                printf("callback abort in %s, cause: %d\n", __FUNCTION__, err);
                return 1;
            }
        }
    }
    if ((open_sql  || on_config_file_args()) && is_blank)
    {   // 没有参数, 也没有缓存文件, 新建空白标签
        file_backup bak = {0};
        share_send_msg(&bak);
    }
    return 0;
}

static void
on_config_create_accel(void)
{
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
        }
        else
        {
            printf("CreateAcceleratorTable failed, cause: %lu\n", GetLastError());
        }
    }
}

bool WINAPI
eu_load_main_config(void)
{
    int  m = 0;
    char *lua_path = NULL;
    TCHAR path[MAX_PATH+1] = {0};
    m = _sntprintf(path, MAX_PATH, _T("%s\\conf\\conf.d\\eu_main.lua"), eu_module_path);
    if (!(m > 0 && m < MAX_PATH) || ((lua_path = eu_utf16_utf8(path, NULL)) == NULL))
    {
        return false;
    }
    if (do_lua_func(lua_path, "run", "") != 0)
    {
        printf("eu_main.lua exec failed\n");
        free(lua_path);
        return false;
    }
    return true;
}

bool WINAPI
eu_load_config(HMODULE *pmod)
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
    if (do_lua_parser_doctype(lua_path, "fill_my_docs"))
    {
        printf("eu_docs exec failed\n");
        goto load_fail;
    }
    if (_stricmp(eu_get_config()->window_theme, "white") == 0)
    {
        ret = on_theme_set_classic(pmod);
    }
    else
    {
        ret = true;
    }
load_fail:
    eu_safe_free(lua_path);
    return ret;
}

void WINAPI
eu_load_file(void)
{
    CloseHandle((HANDLE) _beginthreadex(NULL, 0, on_remote_load_config, NULL, 0, NULL));
    CloseHandle((HANDLE) _beginthreadex(NULL, 0, on_config_load_file, NULL, 0, NULL));
    on_config_create_accel();
}
