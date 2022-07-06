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

static volatile long last_focus = -1;

static int
on_config_file_args(void)
{
    int  ret = 1;
    int  arg_c = 0;
    LPWSTR *args = CommandLineToArgvW(GetCommandLineW(), &arg_c);
    if (args == NULL)
    {
        return 1;
    }
    else if (arg_c >= 2 && args[1][0] && _tcscmp(args[1], _T("-restart")))
    {
        file_backup bak = {0};
        if (arg_c > 2)
        {
            if (_tcscmp(args[1], _T("-noremote")) == 0)
            {
                _tcscpy(bak.rel_path, args[2]);
                share_send_msg(&bak);
            }
        }
        else
        {
            _tcscpy(bak.rel_path, args[1]);
            share_send_msg(&bak);
        }
        ret = 0;
    }
    LocalFree(args);
    return ret;
}

static int
on_config_parser_bakup(void *data, int count, char **column, char **names)
{
    *(int *)data = 0;
    file_backup filebak = {0};
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
        else if (STRCMP(names[i], ==, "szLine"))
        {
            filebak.lineno = _atoi64(column[i]);
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
    }
    if (filebak.focus)
    {
        _InterlockedExchange(&last_focus, filebak.tab_id);
    }
    if (filebak.rel_path[0] || filebak.bak_path[0])
    {
        share_send_msg(&filebak);
    }
    return 0;
}

static unsigned __stdcall
on_config_load_file(void *lp)
{
    int is_blank = 1;
    if (!eu_get_config()->m_instance)
    {
        if (eu_get_config()->m_session)
        {
            const char *sql = "SELECT * FROM skylark_session;";
            int err = eu_sqlite3_send(sql, on_config_parser_bakup, &is_blank);
            if (err != 0)
            {
                printf("eu_sqlite3_send failed in %s, cause: %d\n", __FUNCTION__, err);
                return 1;
            }
            if (last_focus >= 0)
            {
                printf("last_focus = %ld\n", last_focus);
                on_tabpage_select_index(last_focus);
            }
        }        
    }
    if (on_config_file_args() && is_blank)
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

