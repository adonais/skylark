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

#ifndef _SKYLARK_LOCAL_H_
#define _SKYLARK_LOCAL_H_

#define MAX_LOADSTRING 192
#define MAX_MULTI_LANG 99

#define LOAD_APP_RESSTR(tid, var)                              \
    TCHAR var[MAX_LOADSTRING] = { 0 };                         \
    LoadString(eu_module_handle(), tid, var, MAX_LOADSTRING - 1)

#define LOAD_I18N_RESSTR(tid, var)                             \
    TCHAR var[MAX_LOADSTRING] = { 0 };                         \
    eu_i18n_load_str(tid, var, 0);
    
#define MSG_BOX(tid, cid, mb)                                  \
    do                                                         \
    {                                                          \
        TCHAR text[MAX_LOADSTRING] = { 0 };                    \
        TCHAR cap[MAX_LOADSTRING] = { 0 };                     \
        eu_i18n_load_str(tid, text, 0);                        \
        eu_i18n_load_str(cid, cap, 0);                         \
        eu_msgbox(eu_module_hwnd(), text, cap, mb);            \
    } while (0)

#define MSG_BOX_ERR(tid, cid, mb)                              \
    do                                                         \
    {                                                          \
        TCHAR txt[MAX_LOADSTRING+1] = { 0 };                   \
        LOAD_I18N_RESSTR(cid, cap);                            \
        LOAD_I18N_RESSTR(tid, msg);                            \
        _sntprintf(txt, MAX_LOADSTRING, msg, _tcserror(errno));\
        eu_msgbox(eu_module_hwnd(), txt, cap, mb);             \
    } while (0)
    
#define MSG_BOX_SEL(tid, cid, mb, var)                         \
    do                                                         \
    {                                                          \
        LOAD_I18N_RESSTR(cid, cap);                            \
        TCHAR txt[MAX_PATH+1] = { 0 };                         \
        LoadString(g_skylark_lang, tid, txt, MAX_PATH);        \
        var = eu_msgbox(eu_module_hwnd(), txt, cap, mb);       \
    } while (0)

#define print_err_msg(fmt, str)                                      \
    do                                                               \
    {                                                                \
        TCHAR txt[MAX_LOADSTRING + 1] = { 0 };                       \
        LOAD_I18N_RESSTR(IDC_MSG_ERROR, cap);                        \
        LOAD_I18N_RESSTR(fmt, msg);                                  \
        _sntprintf(txt, MAX_LOADSTRING, msg, str);                   \
        eu_msgbox(eu_module_hwnd(), txt, cap, MB_ICONERROR | MB_OK); \
    } while (0)

typedef struct _def_localization
{
    TCHAR desc[ACNAME_LEN];
    TCHAR dll[ACNAME_LEN];
}def_localization;

#ifdef __cplusplus
extern "C"
{
#endif

HMENU i18n_load_menu(int res_id);
HWND i18n_create_dialog(HWND hwnd, int res_id, DLGPROC fn);
intptr_t i18n_dlgbox(HWND hwnd, int res_id, DLGPROC fn, LPARAM param);
void i18n_update_multi_lang(HMENU root_menu);
void i18n_update_menu(HMENU root_menu);
bool i18n_reload_lang(void);
int i18n_switch_locale(HWND hwnd, int id);

#ifdef __cplusplus
}
#endif

#endif // _SKYLARK_LOCAL_H_
