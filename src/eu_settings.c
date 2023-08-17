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

HBITMAP
on_setting_app_icon(void)
{
    return util_shield_icon(NULL, IDI_APPLICATION);
}

HBITMAP
on_setting_customize_icon(void)
{
    return util_shield_icon(eu_module_handle(), MAKEINTRESOURCE(IDM_SETTING_ICON));
}

HBITMAP
on_setting_lua_icon(const int resid)
{
    return util_shield_icon(eu_module_handle(), MAKEINTRESOURCE(resid));
}

static HBITMAP
on_setting_load_icon(const TCHAR *path)
{
    int nid;            // ID of resource that best fits current screen
    HINSTANCE hexe;     // handle to loaded .EXE file
    HRSRC hresource;    // handle to FindResource
    HRSRC hmem;         // handle to LoadResource
    BYTE *lpresource;   // pointer to resource data
    HICON hicon = NULL;
    HBITMAP hmap = NULL;
    const uint32_t dpi = eu_get_dpi(NULL);
    const int scx = Scintilla_GetSystemMetricsForDpi(SM_CXSMICON, dpi);
    const int scy = Scintilla_GetSystemMetricsForDpi(SM_CYSMICON, dpi);
    // Load the file from which to copy the icon.
    do
    {
        // Note: LoadLibrary should have a fully explicit path.
        if ((hexe = (HINSTANCE)LoadLibrary(path)) == NULL)
        {
            //Error loading module -- fail as securely as possible
            eu_logmsg("%s: LoadLibrary return false\n", __FUNCTION__);
            break;
        }
        for (int i = 1; i < UINT16_MAX; ++i)
        {
            if ((hresource = FindResource(hexe, MAKEINTRESOURCE(i), RT_GROUP_ICON)))
            {
                break;
            }
        }
        // Load and lock the icon directory.
        if (!(hmem = hresource ? LoadResource(hexe, hresource): NULL))
        {
            eu_logmsg("%s: LoadResource return false\n", __FUNCTION__);
            break;
        }
        lpresource = LockResource(hmem);
        // Get the identifier of the icon that is most appropriate
        // for the video display.
        if (!(nid = LookupIconIdFromDirectoryEx((PBYTE) lpresource, TRUE, scx, scy, LR_DEFAULTCOLOR)))
        {
            break;
        }
        // Find the bits for the nid icon
        if (!(hresource = FindResource(hexe, MAKEINTRESOURCE(nid), MAKEINTRESOURCE(RT_ICON))))
        {
            break;
        }
        // Load and lock the icon.
        if (!(hmem = LoadResource(hexe, hresource)))
        {
            break;
        }
        lpresource = LockResource(hmem);
        // Create a handle to the icon.
        hicon = CreateIconFromResourceEx((PBYTE) lpresource, SizeofResource(hexe, hresource), TRUE, 0x00030000, scx, scy, LR_DEFAULTCOLOR);
    } while(0);
    if (hicon)
    {
        hmap = util_icon_bitmap(hicon, scx, scy);
        DestroyIcon(hicon);
    }
    else
    {
        hmap = on_setting_app_icon();
    }
    eu_close_dll(hexe);
    return hmap;
}

static void
on_setting_check_env(char ***ppvec, const size_t vec_len)
{
    if (ppvec && vec_len > 0)
    {
        char *p1 = NULL;
        for (size_t i = 0; i < vec_len; ++i)
        {
            char *p2 = NULL;
            char var[QW_SIZE + 1] = {0};
            char u8_buf[MAX_BUFFER] = {0};
            WCHAR buf[MAX_BUFFER] = {0};
            WCHAR *u16_var = NULL;
            p1 = strchr((*ppvec)[i], '%');
            if (NULL != p1)
            {
                p2 = strrchr(p1, '%');
            }
            if (NULL != p2 && p2 > p1 && p2 - p1 < QW_SIZE)
            {
                _snprintf(var, p2 - p1 + 1, "%s", p1);
                u16_var = eu_utf8_utf16(var, NULL);
            }
            if (u16_var)
            {
                ExpandEnvironmentStrings(u16_var, buf, MAX_BUFFER);
                util_make_u8(buf, u8_buf, MAX_BUFFER - 1);
            }
            if (p2 && u16_var)
            {
                strncat(u8_buf, p2 + 1, MAX_BUFFER);
                free((*ppvec)[i]);
                (*ppvec)[i] = strdup(u8_buf);
            }
            eu_safe_free(u16_var);
        }
    }
}

static void
on_setting_parser_args(const char *param, char ***ppvec)
{
    if (STR_NOT_NUL(param))
    {
        util_split(param, " \r\n\t\v\f", ppvec, true);
        on_setting_check_env(ppvec, cvector_size(*ppvec));
    }
}

void
on_setting_update_menu(const HMENU setting_menu)
{
    struct eu_config *pconf = eu_get_config();
    if (setting_menu && pconf)
    {
        HMENU hcustomize = GetSubMenu(setting_menu, CUSTOMIZED_CMD_SUB);
        if (hcustomize)
        {
            int i = 0;
            int count = GetMenuItemCount(hcustomize);
            for (i = count; i > 2; --i)
            {
                DeleteMenu(hcustomize, i - 1, MF_BYPOSITION);
            }
            for (i = 0, count = GetMenuItemCount(hcustomize); i < DW_SIZE; ++i)
            {
                TCHAR *path = NULL;
                TCHAR *ptr_row = NULL;
                if (pconf->m_customize[i].name[0] && pconf->m_customize[i].path[0])
                {
                    errno = 0;
                    int resid = atoi(pconf->m_customize[i].name);
                    if (resid >= IDS_LUAJIT_EVALUATION && resid < INT_MAX && errno == 0 && ((ptr_row = (TCHAR *)calloc(sizeof(TCHAR), DW_SIZE + 1))))
                    {
                        eu_i18n_load_str(resid, ptr_row, DW_SIZE);
                    }
                    else
                    {
                        ptr_row = eu_utf8_utf16(pconf->m_customize[i].name, NULL);
                    }
                }
                if (STR_NOT_NUL(ptr_row) && AppendMenu(hcustomize, MF_POPUP | MF_STRING, IDM_SET_SETTINGS_CONFIG + count, ptr_row))
                {
                    pconf->m_customize[i].posid = IDM_SET_SETTINGS_CONFIG + count;
                    if (pconf->m_customize[i].micon >= IDM_LUAJIT_ICON)
                    {
                        pconf->m_customize[i].hbmp = (uintptr_t)on_setting_lua_icon(pconf->m_customize[i].micon);
                    }
                    else if (!pconf->m_customize[i].micon && (path = util_to_abs(pconf->m_customize[i].path)))
                    {
                        pconf->m_customize[i].hbmp = (uintptr_t)on_setting_load_icon(path);
                    }
                    if (pconf->m_customize[i].hbmp && pconf->m_customize[i].posid > 0)
                    {
                        util_icons_menu_item(setting_menu, pconf->m_customize[i].posid, (HBITMAP)pconf->m_customize[i].hbmp);
                    }
                    ++count;
                }
                eu_safe_free(path);
                eu_safe_free(ptr_row);
            }
        }
    }
}

void
on_setting_execute(HWND hwnd, const int wm_id)
{
    bool wine = util_under_wine();
    struct eu_config *pconf = eu_get_config();
    if (pconf && hwnd && wm_id >= IDM_SET_LUAJIT_EXECUTE)
    {
        for (int i = 0; i < DW_SIZE; ++i)
        {
            bool use_lua = false;
            TCHAR *cmd_exec = NULL;
            cvector_vector_type(char *) cmd_vec = NULL;
            if (pconf->m_customize[i].posid == wm_id && pconf->m_customize[i].path[0] && ((cmd_exec = (TCHAR *)calloc(sizeof(TCHAR), MAX_BUFFER + 1))))
            {
                TCHAR *p = NULL;
                TCHAR *plugin = NULL;
                TCHAR *path = util_to_abs(pconf->m_customize[i].path);
                if (path)
                {
                    if ((p = _tcsrchr(path, _T('.'))) && _tcsicmp(p, _T(".lua")) == 0)
                    {
                        use_lua = true;
                    }
                    if (use_lua)
                    {
                        cvector_push_back(cmd_vec, strdup("skylark.exe"));
                        cvector_push_back(cmd_vec, eu_utf16_utf8(path, NULL));
                        util_update_env(on_tabpage_focus_at());
                        on_setting_parser_args(pconf->m_customize[i].param, &cmd_vec);
                        eu_lua_script_evp((const int)cvector_size(cmd_vec), cmd_vec);
                    }
                    else if (pconf->m_customize[i].param[0])
                    {
                        util_update_env(on_tabpage_focus_at());
                        on_setting_parser_args(pconf->m_customize[i].param, &cmd_vec);
                        if (wine && (plugin = util_winexy_get()))
                        {
                            _sntprintf(cmd_exec, MAX_BUFFER, L"%s \"%s\"", plugin, path);
                        }
                        else
                        {
                            _sntprintf(cmd_exec, MAX_BUFFER, _T("%s"), path);
                        }
                        size_t vec_len = cvector_size(cmd_vec);
                        if (vec_len > 0)
                        {
                            for (size_t k = 0; k < vec_len; ++k)
                            {
                                TCHAR *u16_param = cmd_vec[k] ? eu_utf8_utf16(cmd_vec[k], NULL) : NULL;
                                if (u16_param)
                                {
                                    _tcsncat(cmd_exec, _T(" "), MAX_BUFFER);
                                    _tcsncat(cmd_exec, u16_param, MAX_BUFFER);
                                    free(u16_param);
                                }
                            }
                        }
                    }
                    else
                    {
                        if (wine && (plugin = util_winexy_get()))
                        {
                            _sntprintf(cmd_exec, MAX_BUFFER, L"%s \"%s\"", plugin, path);
                        }
                        else
                        {
                            _sntprintf(cmd_exec, MAX_BUFFER, _T("%s"), path);
                        }
                    }
                    if (true)
                    {
                        free(path);
                    }
                    if (!use_lua)
                    {
                        CloseHandle(eu_new_process(cmd_exec, NULL, NULL, pconf->m_customize[i].hide ? 0 : 2, NULL));
                    }
                }
                eu_safe_free(plugin);
            }
            eu_safe_free(cmd_exec);
            cvector_free_each_and_free(cmd_vec, free);
        }
    }
}

static void
on_setting_edit_save(HWND hdlg, struct eu_config *pconf)
{
    customize_set *pcustomize = NULL;
    HWND hname = GetDlgItem(hdlg, IDC_SETTING_NAME_EDIT);
    HWND hpath = GetDlgItem(hdlg, IDC_SETTING_PATH_EDIT);
    HWND harg = GetDlgItem(hdlg, IDC_SETTING_PARAM_EDIT);
    TCHAR *ptr_name = (TCHAR *)calloc(sizeof(TCHAR), QW_SIZE);
    TCHAR *ptr_path = (TCHAR *)calloc(sizeof(TCHAR), MAX_PATH);
    TCHAR *ptr_arg = (TCHAR *)calloc(sizeof(TCHAR), MAX_PATH);
    for (int i = 0; i < DW_SIZE; ++i)
    {
        if (!pconf->m_customize[i].name[0] || !pconf->m_customize[i].path[0])
        {
            pcustomize = &pconf->m_customize[i];
            break;
        }
    }
    if (pcustomize && hname && hpath && harg && ptr_name && ptr_path && ptr_arg)
    {
        HMENU root_menu = GetMenu(eu_hwnd_self());
        HMENU setting_menu = root_menu ? GetSubMenu(root_menu, THEME_MENU) : NULL;
        Edit_GetText(hname, ptr_name, QW_SIZE - 1);
        Edit_GetText(hpath, ptr_path, MAX_PATH - 1);
        Edit_GetText(harg, ptr_arg, MAX_PATH - 1);
        if (setting_menu && ptr_name[0] && ptr_path[0] && util_make_u8(ptr_name, pcustomize->name, QW_SIZE)[0])
        {
            util_path2unix(ptr_path, MAX_PATH);
            if (util_make_u8(ptr_path, pcustomize->path, MAX_PATH)[0])
            {
                pcustomize->hide = IsDlgButtonChecked(hdlg, IDC_SETTING_HIDE_RUN);
                util_make_u8(ptr_arg, pcustomize->param, MAX_PATH);
                on_setting_update_menu(setting_menu);
                eu_session_backup(SESSION_CONFIG);
            }
        }
    }
}

static int
on_setting_browser_files(HWND hdlg)
{
    TCHAR *path = NULL;
    HWND hwnd_edit = GetDlgItem(hdlg, IDC_SETTING_PATH_EDIT);
    if (hwnd_edit && (path = (TCHAR *)calloc(sizeof(TCHAR), MAX_PATH+1)) && !on_file_open_filename_dlg(hdlg, path, MAX_PATH))
    {
        Edit_SetText(hwnd_edit, path);
    }
    eu_safe_free(path);
    return 1;
}

static INT_PTR CALLBACK
on_setting_proc(HWND hdlg, uint32_t message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
            SetWindowLongPtr(hdlg, GWLP_USERDATA, lParam);
            if (on_dark_enable())
            {
                const int buttons[] = {IDOK, IDCANCEL, IDC_SETTING_BROWSER_BTN};
                for (int id = 0; id < _countof(buttons); ++id)
                {
                    HWND btn = GetDlgItem(hdlg, buttons[id]);
                    on_dark_set_theme(btn, L"Explorer", NULL);
                }
                on_dark_set_theme(GetDlgItem(hdlg, IDC_SETTING_HIDE_RUN), L"", L"");
                on_dark_set_theme(hdlg, L"Explorer", NULL);
            }
            return (INT_PTR)util_creater_window(hdlg, eu_hwnd_self());
        CASE_WM_CTLCOLOR_SET:
        {
            return on_dark_set_contorl_color(wParam);
        }
        case WM_SETTINGCHANGE:
        {
            if (on_dark_enable() && on_dark_color_scheme_change(lParam))
            {
                SendMessage(hdlg, WM_THEMECHANGED, 0, 0);
            }
            break;
        }
        case WM_THEMECHANGED:
        {
            if (on_dark_enable())
            {
                on_dark_allow_window(hdlg, true);
                on_dark_refresh_titlebar(hdlg);
                const int buttons[] = {IDOK, IDCANCEL, IDC_SETTING_BROWSER_BTN};
                for (int id = 0; id < _countof(buttons); ++id)
                {
                    HWND btn = GetDlgItem(hdlg, buttons[id]);
                    on_dark_allow_window(btn, true);
                    SendMessage(btn, WM_THEMECHANGED, 0, 0);
                }
                UpdateWindow(hdlg);
            }
            break;
        }
        case WM_COMMAND:
        {
            struct eu_config *pconf = NULL;
            WORD mid = LOWORD(wParam);
            switch (mid)
            {
                case IDC_SETTING_BROWSER_BTN:
                {
                    return on_setting_browser_files(hdlg);
                }
                case IDOK:
                    if ((pconf = (struct eu_config *) GetWindowLongPtr(hdlg, GWLP_USERDATA)) != NULL)
                    {
                        on_setting_edit_save(hdlg, pconf);
                    }
                    SendMessage(hdlg, WM_CLOSE, 0, 0);
                    break;
                case IDCANCEL:
                    SendMessage(hdlg, WM_CLOSE, 0, 0);
                    break;
                default:
                    break;
            }
            break;
        }
        case WM_CLOSE:
            return (INT_PTR)EndDialog(hdlg, LOWORD(wParam));
        default:
            break;
    }
    return 0;
}

void
on_setting_manager(void)
{
    struct eu_config *pconf = eu_get_config();
    if (pconf)
    {
        i18n_dlgbox(eu_module_hwnd(), IDD_SETTING_DIALOG, on_setting_proc, (LPARAM)pconf);
    }
    else
    {
        eu_logmsg("%s: error, eu_config point null\n", __FUNCTION__);
    }
}
