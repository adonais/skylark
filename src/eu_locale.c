/******************************************************************************
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

#define ENV_MENU 6
#define LOCALE_SUB_MENU 5

def_localization sz_localization[] =
{
    {_T("English"), _T("en-us.dll")},
    {_T("中文简体"), _T("zh-cn.dll")},/*
    {_T("台灣繁體"), _T("zh-tw.dll")},
    {_T("Français"), _T("fr-fr.dll")},
    {_T("한국어"), _T("ko-kr.dll")},
    {_T("日本語"), _T("ja-jp.dll")},
    {_T("Deutsch"), _T("de-de.dll")},
    {_T("Español"), _T("es-es.dll")},
    {_T("Català"), _T("ca-es.dll")},
    {_T("Galego"), _T("gl-es.dll")},
    {_T("Euskara"), _T("eu-es.dll")},
    {_T("Español argentina"), _T("es_ar.dll")},
    {_T("Italiano"), _T("it-it.dll")},
    {_T("Português"), _T("pt-pt.dll")},
    {_T("Português brasileiro"), _T("pt-br.dll")},
    {_T("Nederlands"), _T("nl-nl.dll")},
    {_T("Русский"), _T("ru-ru.dll")},
    {_T("ไทย"), _T("th-th.dll")},
    {_T("Polski"), _T("pl-pl.dll")},
    {_T("Česky"), _T("cs-cz.dll")},
    {_T("Magyar"), _T("hu-hu.dll")},
    {_T("Română"), _T("ro-ro.dll")},
    {_T("Türkçe"), _T("tr-tr.dll")},
    {_T("فارسی"), _T("fa-ir.dll")},
    {_T("Українська"), _T("uk-ua.dll")},
    {_T("עברית"), _T("he-il.dll")},
    {_T("Norsk"), _T("nb-no.dll")},
    {_T("العربية"), _T("ar-eg.dll")},
    {_T("Suomi"), _T("fi-fi.dll")},
    {_T("Lietuvių"), _T("lt-lt.dll")},
    {_T("Ελληνικά"), _T("el-gr.dll")},
    {_T("Svenska"), _T("sv-se.dll")},
    {_T("Slovenščina"), _T("sl-sl.dll")},
    {_T("Slovenčina"), _T("sk-sk.dll")},
    {_T("Dansk"), _T("da-dk.dll")},
    {_T("Български"), _T("bg-bg.dll")},
    {_T("Bahasa Indonesia"), _T("id-id.dll")},
    {_T("Gjuha shqipe"), _T("sq-al.dll")},
    {_T("Hrvatski jezik"), _T("hr-hr.dll")},
    {_T("ქართული ენა"), _T("ge-ge.dll")},
    {_T("Беларуская мова"), _T("by-by.dll")},
    {_T("Srpski"), _T("sr-rs.dll")},
    {_T("Cрпски"), _T("sr-sp-cyrl.dll")},
    {_T("Bahasa Melayu"), _T("ms-my.dll")},
    {_T("Lëtzebuergesch"), _T("fr-lu.dll")},
    {_T("Afrikaans"), _T("af-za.dll")},
    {_T("Қазақша"), _T("kk-kz.dll")},
    {_T("O‘zbekcha"), _T("uz-uz-latn.dll")},
    {_T("Ўзбекча"), _T("uz-uz-cyrl.dll")},
    {_T("latviešu valoda"), _T("lv-lv.dll")},
    {_T("தமிழ்"), _T("ta-in.dll")},
    {_T("Azərbaycan dili"), _T("az-az.dll")},
    {_T("हिन्दी"), _T("hi-in.dll")},
    {_T("मराठी"), _T("mr-in.dll")},
    {_T("tiếng Việt"), _T("vi-vn.dll")},
    {_T("Монгол хэл"), _T("mn-mn.dll")},
    {_T("eesti keel"), _T("et-ee.dll")},
    {_T("کوردی‬"), _T("kmr-iq.dll")},
    {_T("Gaeilge"), _T("ga-ie.dll")},
    {_T("नेपाली"), _T("ne-np.dll")}, */
    {_T("\0"), _T("\0")}
};

static bool
get_locale_path(TCHAR *path, int len)
{
    bool ret = false;
    HANDLE m_map = NULL;
    TCHAR *memory = NULL;
    TCHAR *p = NULL;
    if (!(m_map = share_open(FILE_MAP_READ, SKYLARK_LOCK_LANG))) 
    {
        printf("share_open error in %s\n", __FUNCTION__);
        return false;
    }   
    if ((memory = (TCHAR *) share_map(m_map, MAX_PATH*sizeof(TCHAR), FILE_MAP_READ)))
    {
        _sntprintf(path, len-1, _T("%s"), memory);
        ret = true;
        share_unmap(memory);
    }
    share_close(m_map);
    return ret;
}

static bool
get_locale_file(TCHAR *path, int len)
{
    bool ret = false;
    HANDLE m_map = NULL;
    TCHAR *memory = NULL;
    TCHAR *p = NULL;
    if (!(m_map = share_open(FILE_MAP_READ, SKYLARK_LOCK_LANG))) 
    {
        printf("share_open error in %s\n", __FUNCTION__);
        return false;
    }   
    if ((memory = (TCHAR *) share_map(m_map, MAX_PATH*sizeof(TCHAR), FILE_MAP_READ)))
    {
        if ((p = _tcsrchr(memory, _T('\\'))))
        {
            _sntprintf(path, len-1, _T("%s"), p+1);
            ret = true;
        }
        share_unmap(memory);
    }
    share_close(m_map);
    return ret;
}

bool
eu_i18n_load_str(uint16_t id, TCHAR *str, int len)
{
    bool ret = true;
    TCHAR lang_path[MAX_PATH] = {0};
    int buf_len = MAX_LOADSTRING;
    if (len > 0)
    {
        buf_len = len - 1;
    }
    if (!g_skylark_lang)
    {
        if (!get_locale_path(lang_path, MAX_PATH-1))
        {
            printf("get_locale_path return false\n");
            return false;
        }
        if (!(g_skylark_lang = LoadLibraryEx(lang_path, NULL, LOAD_LIBRARY_AS_DATAFILE)))
        {
            printf("LoadLibraryEx return false, lang_path = %ls\n", lang_path);
            return false;
        }
    }
    if (!LoadString(g_skylark_lang, id, str, buf_len))
    {
        printf("LoadString %d return false\n", id);
        ret = false;
    }
    return ret;
}

HMENU
i18n_load_menu(int res_id)
{
    return LoadMenu(g_skylark_lang, MAKEINTRESOURCE(res_id));
}

HWND
i18n_create_dialog(HWND hwnd, int res_id, DLGPROC fn)
{
    HRSRC hr = NULL;
    if (g_skylark_lang == NULL)
    {
        printf("g_skylark_lang is null\n");
        return NULL;
    }
    if ((hr = FindResource(g_skylark_lang, MAKEINTRESOURCE(res_id), RT_DIALOG)) == NULL)
    {
        printf("FindResource failed\n");
        return NULL;
    }
    return CreateDialog(g_skylark_lang, MAKEINTRESOURCE(res_id), hwnd, fn);
}

bool
i18n_reload_lang(void)
{
    HANDLE m_map = NULL;
    TCHAR *memory = NULL; 
    do
    {
        if (!(m_map = share_open(FILE_MAP_READ, SKYLARK_LOCK_LANG))) 
        {
            printf("share_open error in %s\n", __FUNCTION__);
            break;
        }   
        if (!(memory = (TCHAR *) share_map(m_map, MAX_PATH*sizeof(TCHAR), FILE_MAP_READ)))
        {
            printf("share_map error in %s\n", __FUNCTION__);
            break;
        }
        if (!(g_skylark_lang = LoadLibraryEx(memory, NULL, LOAD_LIBRARY_AS_DATAFILE)))
        {
            printf("LoadLibraryEx error, cause: %lu\n", GetLastError());
            break;
        }         
    } while(0);
    if (memory)
    {
        share_unmap(memory);
    }
    if (m_map)
    {
        share_close(m_map);
    }
    return (NULL != g_skylark_lang);
}


intptr_t
i18n_dlgbox(HWND hwnd, int res_id, DLGPROC fn, LPARAM param)
{
    HRSRC hr = NULL;
    if (!(g_skylark_lang || i18n_reload_lang()))
    {
        return 0;
    }
    if ((hr = FindResource(g_skylark_lang, MAKEINTRESOURCE(res_id), RT_DIALOG)) == NULL)
    {
        printf("FindResource failed, cause: %lu\n", GetLastError());
        return 0;
    }
    return (intptr_t)DialogBoxParam(g_skylark_lang, MAKEINTRESOURCE(res_id), hwnd, fn, param);
}

void
i18n_update_multi_lang(HMENU root_menu)
{
    if (root_menu)
    {
        int index;
        HMENU menu_lang = NULL;
        if (!(menu_lang = GetSubMenu(root_menu, LOCALE_SUB_MENU)))
        {
            printf("menu_lang is null\n");
            return;
        }
        int count = GetMenuItemCount(menu_lang);
        for (index = 0; index < count; ++index)
        {
            DeleteMenu(menu_lang, 0, MF_BYPOSITION);
        }
        for (index = 0; index < MAX_MULTI_LANG && sz_localization[index].desc[0]; ++index)
        {
            AppendMenu(menu_lang, MF_POPUP | MF_STRING, IDM_LOCALES_BASE + index, sz_localization[index].desc);
        }
    }
}

void
i18n_update_menu(HMENU root_menu)
{
    int index;
    TCHAR lang_name[ACNAME_LEN] = {0};
    TCHAR current_lang[ACNAME_LEN] = {0};
    HMENU lang_menu = NULL;
    lang_menu = root_menu ? GetSubMenu(root_menu, LOCALE_SUB_MENU) : NULL;
    if (!(root_menu && lang_menu))
    {
        return;
    }
    if (!get_locale_file(lang_name, ACNAME_LEN))
    {
        printf("get_locale_file return false\n");
        return;
    }
    for (index = 0; index < MAX_MULTI_LANG && sz_localization[index].desc[0]; ++index)
    {
        if (!_tcscmp(sz_localization[index].dll, lang_name))
        {
            _tcsncpy(current_lang, sz_localization[index].desc, ACNAME_LEN-1);
            break;
        }
    }
    int count = GetMenuItemCount(lang_menu);
    for (index = 0; index < count; ++index)
    {
        bool select = false;
        TCHAR buf[ACNAME_LEN] = { 0 };
        int len = GetMenuString(root_menu, IDM_LOCALES_BASE + index, buf, ACNAME_LEN - 1, MF_BYCOMMAND);
        if (len > 0 && _tcscmp(buf, current_lang) == 0)
        {
            char *u8_lang = eu_utf16_utf8(lang_name, NULL);
            if (u8_lang)
            {
                strncpy(eu_get_config()->m_language, u8_lang, ACNAME_LEN - 1);
                free(u8_lang);
            }
            select = true;
        }
        util_set_menu_item(lang_menu, IDM_LOCALES_BASE + index, select);
    }
}

static int
eu_refresh_interface(HMODULE new_lang, const TCHAR *lang_path)
{
    HANDLE m_map = NULL;
    TCHAR *memory = NULL;
    if (!(m_map = share_open(FILE_MAP_WRITE | FILE_MAP_READ, SKYLARK_LOCK_LANG)))
    {
        printf("share_open error in %s\n", __FUNCTION__);
        return 1;
    }
    if (!(memory = (TCHAR *) share_map(m_map, MAX_PATH*sizeof(TCHAR), FILE_MAP_WRITE | FILE_MAP_READ)))
    {
        share_close(m_map);
        return 1;
    }
    HMENU hmenu = GetMenu(eu_module_hwnd());
    if (hmenu)
    {   // 销毁顶层菜单
        DestroyMenu(hmenu);
    }
    HWND htool = GetDlgItem(eu_module_hwnd(), IDC_TOOLBAR);
    if (htool)
    {   // 销毁工具栏
        DestroyWindow(htool);
    }
    if (on_toolbar_clip_hwnd())
    {   // 销毁剪贴板
        DestroyWindow(on_toolbar_clip_hwnd());
    }
    if (g_statusbar)
    {   // 销毁状态栏
        DestroyWindow(g_statusbar);
    } 
    if (eu_get_search_hwnd())
    {   // 销毁搜索框
        DestroyWindow(eu_get_search_hwnd());
    }
    // 销毁文件管理器
    if (g_treebar)
    {
        DestroyWindow(g_treebar);
    }
    /* 销毁所有右键菜单 */
    on_tabpage_destroy_rclick();
    if (true)
    {   // 更新全局变量与共享内存
        FreeLibrary(g_skylark_lang);
        g_skylark_lang = new_lang;
        memcpy(memory, lang_path, MAX_PATH*sizeof(TCHAR));
        share_unmap(memory);
        share_close(m_map);
    }
    hmenu = i18n_load_menu(IDC_SKYLARK);
    if (!hmenu)
    {
        return 1;
    }
    if (!SetMenu(eu_module_hwnd(), hmenu))
    {
        return 1;
    }
    if (on_tabpage_create_rclick())
    {
        return 1;
    }
    if (on_toolbar_create(eu_module_hwnd()) != 0)
    {
        printf("on_toolbar_create return false\n");
        return 1;
    }
    if (!on_statusbar_init(eu_module_hwnd()))
    {
        printf("on_statusbar_init return false\n");
        return 1;
    }
    if (!on_search_create_box())
    {
        printf("on_search_create_box return false\n");
        return 1;
    }
    if (on_treebar_create_box(eu_module_hwnd()))
    {
        printf("on_treebar_create_box return false\n");
        return 1;
    }             
    return 0;
}

int
i18n_switch_locale(HWND hwnd, int id)
{
    TCHAR buf[ACNAME_LEN] = {0};
    TCHAR old[ACNAME_LEN] = {0};
    TCHAR sel[ACNAME_LEN] = {0};
    TCHAR lang_path[MAX_PATH] = {0};
    HMODULE new_lang = NULL;
    if (!GetMenuString(GetMenu(hwnd), id, buf, ACNAME_LEN, MF_BYCOMMAND))
    {
        return 1;
    }
    for (int i = 0; i < MAX_MULTI_LANG && sz_localization[i].desc[0]; ++i)
    {
        if (!_tcscmp(sz_localization[i].desc, buf))
        {
            _tcsncpy(sel, sz_localization[i].dll, ACNAME_LEN-1);
            break;
        }
    }
    if (!MultiByteToWideChar(CP_UTF8, 0, eu_get_config()->m_language, -1, old, ACNAME_LEN))
    {
        return 1;
    }    
    if (_tcscmp(sel, old) == 0)
    {
        return 0;
    }
     /* 加载新语言文件并动态刷新界面 */
    _sntprintf(lang_path, MAX_PATH-1, _T("%s\\locales\\%s"), eu_module_path, sel);
    new_lang = LoadLibraryEx(lang_path, NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (!new_lang)
    {
        return 1;
    }
    if (eu_refresh_interface(new_lang, lang_path) == 0)
    {
        HMENU root_menu = GetMenu(hwnd);
        HMENU menu_env = root_menu ? GetSubMenu(root_menu, ENV_MENU) : NULL;
        on_tabpage_newdoc_reload();    
        eu_window_resize(hwnd);
        i18n_update_multi_lang(menu_env);
        i18n_update_menu(menu_env);
    }
    return 0;
}
