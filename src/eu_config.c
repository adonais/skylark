/*******************************************************************************
 * This file is part of Skylark project
 * Copyright ©2021 Hua andy <hua.andy@gmail.com>

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
    else if (arg_c == 2 && args[1][0] && _tcscmp(args[1], _T("-restart")))
    {
        file_backup bak = {0};
        _tcscpy(bak.rel_path, args[1]);
        share_send_msg(&bak);
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
    if (on_config_file_args() && is_blank)
    {   // 没有参数, 也没有缓存文件, 新建空白标签
        file_backup bak = {0};
        share_send_msg(&bak);
    }
    return 0;
}

int WINAPI
eu_load_config(HMODULE *pmod)
{
    char *lua_path = NULL;
    TCHAR path[MAX_PATH] = { 0 };
    _sntprintf(path, MAX_PATH - 1, _T("%s\\conf\\conf.d\\eu_docs.lua"), eu_module_path);
    if ((lua_path = eu_utf16_utf8(path, NULL)) == NULL)
    {
        return 1;
    }
    if (do_lua_parser_doctype(lua_path, "fill_my_docs"))
    {
        printf("lua failed\n");
    }
    if (_stricmp(eu_get_config()->window_theme, "white") == 0)
    {
        on_theme_set_classic(pmod);
    }    
    free(lua_path);
    return 0;
}

void WINAPI
eu_load_file(void)
{
    CloseHandle((HANDLE) _beginthreadex(NULL, 0, on_remote_load_config, NULL, 0, NULL));
    CloseHandle((HANDLE) _beginthreadex(NULL, 0, on_config_load_file, NULL, 0, NULL));
}

