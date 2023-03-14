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

#ifndef _SKYLARK_LOCAL_H_
#define _SKYLARK_LOCAL_H_

#define MAX_LOADSTRING 192
#define MAX_MULTI_LANG 99

#define LOAD_APP_RESSTR(tid, var)                              \
    TCHAR var[MAX_LOADSTRING] = {0};                           \
    LoadString(eu_module_handle(), tid, var, MAX_LOADSTRING - 1)

#define LOAD_I18N_RESSTR(tid, var)                             \
    TCHAR var[MAX_PATH] = {0};                                 \
    eu_i18n_load_str(tid, var, MAX_PATH);

#define MSG_BOX(tid, cid, mb)                                   \
    do                                                          \
    {                                                           \
        TCHAR _txt[MAX_LOADSTRING] = {0};                       \
        TCHAR _cap[MAX_LOADSTRING] = {0};                       \
        eu_i18n_load_str(tid, _txt, 0);                         \
        eu_i18n_load_str(cid, _cap, 0);                         \
        eu_msgbox(eu_module_hwnd(), _txt, _cap, mb);            \
    } while (0)

#define MSG_BOX_ERR(tid, cid, mb)                                \
    do                                                           \
    {                                                            \
        TCHAR _txt[MAX_LOADSTRING+1] = {0};                      \
        LOAD_I18N_RESSTR(cid, _cap);                             \
        LOAD_I18N_RESSTR(tid, _msg);                             \
        _sntprintf(_txt, MAX_LOADSTRING, _msg, _tcserror(errno));\
        eu_msgbox(eu_module_hwnd(), _txt, _cap, mb);             \
    } while (0)

#define MSG_BOX_SEL(tid, cid, mb, var)                           \
    do                                                           \
    {                                                            \
        LOAD_I18N_RESSTR(cid, _cap);                             \
        TCHAR _txt[MAX_PATH+1] = {0};                            \
        LoadString(g_skylark_lang, tid, _txt, MAX_PATH);         \
        var = eu_msgbox(eu_module_hwnd(), _txt, _cap, mb);       \
    } while (0)

#define print_err_msg(fmt, str)                                        \
    do                                                                 \
    {                                                                  \
        TCHAR _txt[MAX_LOADSTRING + 1] = {0};                          \
        LOAD_I18N_RESSTR(IDC_MSG_ERROR, _cap);                         \
        LOAD_I18N_RESSTR(fmt, _msg);                                   \
        _sntprintf(_txt, MAX_LOADSTRING, _msg, str);                   \
        eu_msgbox(eu_module_hwnd(), _txt, _cap, MB_ICONERROR | MB_OK); \
    } while (0)

typedef struct _def_localization
{
    TCHAR desc[QW_SIZE];
    TCHAR dll[QW_SIZE];
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
