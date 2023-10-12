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

#define CLANGDLL _T("plugins\\clang-format.dll")

static HBITMAP g_settings_hbmp = NULL;
static HBITMAP g_shield_hbmp = NULL;

int
menu_height(void)
{
    int m_height = 0;
    if (eu_get_config()->m_menubar)
    {
        HWND hwnd = eu_module_hwnd();
        int cy_border = GetSystemMetrics(SM_CYFRAME);
        int cy_caption = GetSystemMetrics(SM_CYCAPTION);
        RECT window_rect;
        GetWindowRect(hwnd, &window_rect);
        POINT client_top_left = { 0, 0 };
        ClientToScreen(hwnd, &client_top_left);
        m_height = client_top_left.y - window_rect.top - cy_caption - cy_border;
    }
    return m_height;
}

void
menu_bmp_destroy(void)
{
    if (g_settings_hbmp)
    {
        DeleteObject(g_settings_hbmp);
        g_settings_hbmp = NULL;
    }
    if (g_shield_hbmp)
    {
        DeleteObject(g_shield_hbmp);
        g_shield_hbmp = NULL;
    }
    eu_logmsg("menu_bmp_destroy\n");
}

void
menu_destroy(const HWND hwnd)
{
    HMENU hmenu = GetMenu(hwnd);
    if(hmenu)
    {
        DestroyMenu(hmenu);
    }
    menu_bmp_destroy();
}

HMENU
menu_load(const uint16_t mid)
{
    HMENU hpop = NULL;
    HMENU hmenu = i18n_load_menu(mid);
    if(hmenu)
    {
        hpop = GetSubMenu(hmenu, 0);
        RemoveMenu(hmenu, 0, MF_BYPOSITION);
        DestroyMenu(hmenu);
    }
    return hpop;
}

int
menu_pop_track(const HWND hwnd, const uint16_t mid, LPARAM lparam, const uint32_t flags, ptr_menu_callback fn, void *param)
{
    HMENU hpop = menu_load(mid);
    if(hpop)
    {
        POINT pt;
        if (!lparam)
        {
            GetCursorPos(&pt);
        }
        else
        {
            pt.x = GET_X_LPARAM(lparam);
            pt.y = GET_Y_LPARAM(lparam);
        }
        if (fn)
        {
            fn(hpop, param);
        }
        if (flags == (uint32_t)-1)
        {
            TrackPopupMenu(hpop, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
        }
        else
        {
            TrackPopupMenu(hpop, flags, pt.x, pt.y, 0, hwnd, NULL);
        }
        DestroyMenu(hpop);
        return 1;
    }
    return 0;
}

void
menu_switch_theme(void)
{
    HWND hwnd = eu_module_hwnd();
    if (hwnd)
    {
        HMENU root_menu = GetMenu(hwnd);
        HMENU view_menu = GetSubMenu(root_menu, THEME_MENU);
        HMENU theme_menu = GetSubMenu(view_menu, THEME_MENU_SUB);
        if (view_menu && theme_menu)
        {
            for (int index = 0, count = GetMenuItemCount(theme_menu); index < count; ++index)
            {
                bool select = false;
                TCHAR buf[QW_SIZE] = { 0 };
                TCHAR theme[QW_SIZE] = { 0 };
                int len = GetMenuString(root_menu, IDM_STYLETHEME_BASE + index, buf, QW_SIZE - 1, MF_BYCOMMAND);
                if (len > 0 && MultiByteToWideChar(CP_UTF8, 0, eu_get_theme()->name, -1, theme, QW_SIZE) > 0)
                {
                    TCHAR* pbuf = on_theme_query_name(buf);
                    if (pbuf && _tcscmp(pbuf, theme) == 0)
                    {
                        select = true;
                    }
                    if (_stricmp(eu_get_config()->window_theme, eu_get_theme()->name))
                    {
                        strncpy(eu_get_config()->window_theme, eu_get_theme()->name, QW_SIZE);
                    }
                }
                util_set_menu_item(root_menu, IDM_STYLETHEME_BASE + index, select);
            }
        }
    }
}

static void
menu_setting_icons(const HMENU menu)
{
    if (g_settings_hbmp || (g_settings_hbmp = on_setting_customize_icon()))
    {
        util_icons_menu_item(menu, IDM_SET_SETTINGS_CONFIG, g_settings_hbmp);
    }
}

static void
menu_shield_icons(const HMENU menu, const uint32_t res_min, const uint32_t res_max)
{
    if (!on_reg_admin())
    {
        if (g_shield_hbmp || (g_shield_hbmp = util_shield_icon(NULL, IDI_SHIELD)) != NULL)
        {
            for (uint32_t i = (uint32_t)res_min; i <= res_max; ++i)
            {
                util_icons_menu_item(menu, i, g_shield_hbmp);
            }
        }
    }
}

static void
menu_update_hexview(const HMENU root_menu, const bool hex_mode, const bool init)
{
    if (root_menu)
    {
        util_enable_menu_item(root_menu, IDM_EDIT_UNDO, init || !hex_mode);
        util_enable_menu_item(root_menu, IDM_EDIT_REDO, init || !hex_mode);
        util_enable_menu_item(root_menu, IDM_UPDATE_SELECTION, init || !hex_mode);
        util_enable_menu_item(root_menu, IDM_EDIT_PLACEHOLDE10, init || !hex_mode);
        util_enable_menu_item(root_menu, IDM_EDIT_PLACEHOLDE11, init || !hex_mode);
        util_enable_menu_item(root_menu, IDM_EDIT_PLACEHOLDE12, init || !hex_mode);
        util_enable_menu_item(root_menu, IDM_EDIT_PLACEHOLDE13, init || !hex_mode);
        util_enable_menu_item(root_menu, IDM_SEARCH_REPLACE, init || !hex_mode);
        util_enable_menu_item(root_menu, IDM_SELECTION_RECTANGLE, init || !hex_mode);
        util_enable_menu_item(root_menu, IDM_SEARCH_MULTISELECT_README, init || !hex_mode);
        util_enable_menu_item(root_menu, IDM_VIEW_TAB_WIDTH, init || !hex_mode);
        util_enable_menu_item(root_menu, IDM_TAB_CONVERT_SPACES, init || !hex_mode);
        util_enable_menu_item(root_menu, IDM_VIEW_DOCUMENT_MAP, init || !hex_mode);
        util_enable_menu_item(root_menu, IDM_VIEW_HIGHLIGHT_BRACE, init || !hex_mode);
        util_enable_menu_item(root_menu, IDM_VIEW_HIGHLIGHT_STR, init || !hex_mode);
        util_enable_menu_item(root_menu, IDM_VIEW_HIGHLIGHT_FOLD, init || !hex_mode);
        util_enable_menu_item(root_menu, IDM_VIEW_CODE_HINT, init || !hex_mode);
        util_enable_menu_item(root_menu, IDM_EDIT_AUTO_INDENTATION, init || !hex_mode);
        util_enable_menu_item(root_menu, IDM_VIEW_LINENUMBER_VISIABLE, init || !hex_mode);
        util_enable_menu_item(root_menu, IDM_VIEW_BOOKMARK_VISIABLE, init || !hex_mode);
        util_enable_menu_item(root_menu, IDM_VIEW_WHITESPACE_VISIABLE, init || !hex_mode);
        util_enable_menu_item(root_menu, IDM_VIEW_NEWLINE_VISIABLE, init || !hex_mode);
        util_enable_menu_item(root_menu, IDM_VIEW_INDENTGUIDES_VISIABLE, init || !hex_mode);
        util_enable_menu_item(root_menu, IDM_VIEW_WRAPLINE_MODE, init || !hex_mode);
        util_enable_menu_item(root_menu, IDM_SOURCECODE_GOTODEF, init || !hex_mode);
        util_enable_menu_item(root_menu, IDM_EDIT_AUTO_CLOSECHAR, init || !hex_mode);
        util_enable_menu_item(root_menu, IDM_SOURCEE_ENABLE_ACSHOW, init || !hex_mode);
        util_enable_menu_item(root_menu, IDM_SOURCEE_ACSHOW_CHARS, init || !hex_mode);
        util_enable_menu_item(root_menu, IDM_SOURCE_ENABLE_CTSHOW, init || !hex_mode);
        util_enable_menu_item(root_menu, IDM_SETTING_FONTQUALITY, init || !hex_mode);
        util_enable_menu_item(root_menu, IDM_SETTING_RENDER, init || !hex_mode);
        util_enable_menu_item(root_menu, IDM_VIEW_HISTORY_PLACEHOLDE, init || !hex_mode);
        util_enable_menu_item(root_menu, IDM_SEARCH_HISTORY_PLACEHOLDE, init || !hex_mode);
    }
}

static void
menu_default_keys(const HMENU hmenu, const int id, const int pos)
{
    wchar_t mdata[FILESIZE+1] = {0};
    if (GetMenuString(hmenu, id, mdata, FILESIZE, MF_BYCOMMAND) > 0)
    {
        if (_tcschr(mdata, _T('\t')))
        {
            _tcsncat(mdata, _T(","), FILESIZE);
        }
        else
        {
            _tcsncat(mdata, _T("\t"), FILESIZE);
        }
        if (id == IDM_VIEW_ZOOMIN)
        {
            _tcsncat(mdata, _T("Ctrl+MouseWheelUP"), FILESIZE);
        }
        else if (id == IDM_VIEW_ZOOMOUT)
        {
            _tcsncat(mdata, _T("Ctrl+MouseWheelDown"), FILESIZE);
        }
        else if (id == IDM_SELECTION_RECTANGLE)
        {
            _tcsncat(mdata, _T("Alt+MouseLdown"), FILESIZE);
        }
        ModifyMenu(hmenu, pos, MF_BYPOSITION|MF_STRING, id, mdata);
    }
}

static void
menu_modify_string(const HMENU hmenu, const uint32_t id, const TCHAR *mdata)
{
    MENUITEMINFO mii = {sizeof(MENUITEMINFO), MIIM_STRING|MIIM_BITMAP};
    if (GetMenuItemInfo(hmenu, id, false, &mii))
    {   // 不使用ModifyMenu, 它破坏了菜单栏图标
        mii.dwTypeData = (TCHAR *)mdata;
        mii.cch = (uint32_t)_tcslen(mdata);
        SetMenuItemInfo(hmenu, id, false, &mii);
    }
}

static uint32_t
menu_update_string(const HMENU hmenu, const int pos)
{
    uint32_t id = 0;
    if ((id = GetMenuItemID(hmenu, pos)) > 0)
    {
        bool redraw = true;
        eue_accel *p = NULL;
        wchar_t mdata[FILESIZE+1] = {0};
        if (GetMenuString(hmenu, id, mdata, FILESIZE, MF_BYCOMMAND) > 0)
        {
            if (_tcschr(mdata, _T('\t')))
            {
                redraw = false;
            }
        }
        if (redraw && (p = eu_get_accel()) != NULL && p->accel_num > 0)
        {
            for (int i = 0; i < p->accel_num; ++i)
            {
                if (!(p->accel_ptr[i].fVirt && p->accel_ptr[i].key && p->accel_ptr[i].cmd))
                {
                    continue;
                }
                if (p->accel_ptr[i].cmd == id)
                {
                    if (!_tcschr(mdata, _T('\t')))
                    {
                        _tcsncat(mdata, _T("\t"), FILESIZE);
                    }
                    else
                    {
                        continue;
                    }
                    if (p->accel_ptr[i].fVirt & FCONTROL)
                    {
                        _tcsncat(mdata, _T("Ctrl+"), FILESIZE);
                    }
                    if (p->accel_ptr[i].fVirt & FSHIFT)
                    {
                        _tcsncat(mdata, _T("Shift+"), FILESIZE);
                    }
                    if (p->accel_ptr[i].fVirt & FALT)
                    {
                        _tcsncat(mdata, _T("Alt+"), FILESIZE);
                    }
                    if ((p->accel_ptr[i].key > 0x2f && p->accel_ptr[i].key < 0x5b)||
                        (((p->accel_ptr[i].key > 0x20 && p->accel_ptr[i].key < 0x30)||
                        (p->accel_ptr[i].key > 0x5a && p->accel_ptr[i].key < 0x7f))&&
                        !(p->accel_ptr[i].fVirt & FVIRTKEY)))
                    {
                        int len = eu_int_cast(_tcslen(mdata));
                        _sntprintf(mdata+len, FILESIZE - len, _T("%c"), p->accel_ptr[i].key);
                    }
                    else
                    {
                        switch (p->accel_ptr[i].key)
                        {
                            case VK_F1:         _tcsncat(mdata, _T("F1"),      FILESIZE); break;
                            case VK_F2:         _tcsncat(mdata, _T("F2"),      FILESIZE); break;
                            case VK_F3:         _tcsncat(mdata, _T("F3"),      FILESIZE); break;
                            case VK_F4:         _tcsncat(mdata, _T("F4"),      FILESIZE); break;
                            case VK_F5:         _tcsncat(mdata, _T("F5"),      FILESIZE); break;
                            case VK_F6:         _tcsncat(mdata, _T("F6"),      FILESIZE); break;
                            case VK_F7:         _tcsncat(mdata, _T("F7"),      FILESIZE); break;
                            case VK_F8:         _tcsncat(mdata, _T("F8"),      FILESIZE); break;
                            case VK_F9:         _tcsncat(mdata, _T("F9"),      FILESIZE); break;
                            case VK_F10:        _tcsncat(mdata, _T("F10"),     FILESIZE); break;
                            case VK_F11:        _tcsncat(mdata, _T("F11"),     FILESIZE); break;
                            case VK_F12:        _tcsncat(mdata, _T("F12"),     FILESIZE); break;
                            case VK_NUMPAD0:    _tcsncat(mdata, _T("0"),       FILESIZE); break;
                            case VK_NUMPAD1:    _tcsncat(mdata, _T("1"),       FILESIZE); break;
                            case VK_NUMPAD2:    _tcsncat(mdata, _T("2"),       FILESIZE); break;
                            case VK_NUMPAD3:    _tcsncat(mdata, _T("3"),       FILESIZE); break;
                            case VK_NUMPAD4:    _tcsncat(mdata, _T("4"),       FILESIZE); break;
                            case VK_NUMPAD5:    _tcsncat(mdata, _T("5"),       FILESIZE); break;
                            case VK_NUMPAD6:    _tcsncat(mdata, _T("6"),       FILESIZE); break;
                            case VK_NUMPAD7:    _tcsncat(mdata, _T("7"),       FILESIZE); break;
                            case VK_NUMPAD8:    _tcsncat(mdata, _T("8"),       FILESIZE); break;
                            case VK_NUMPAD9:    _tcsncat(mdata, _T("9"),       FILESIZE); break;
                            case VK_OEM_PLUS:   _tcsncat(mdata, _T("+"),       FILESIZE); break;
                            case VK_OEM_COMMA:  _tcsncat(mdata, _T(","),       FILESIZE); break;
                            case VK_OEM_MINUS:  _tcsncat(mdata, _T("-"),       FILESIZE); break;
                            case VK_OEM_PERIOD: _tcsncat(mdata, _T("."),       FILESIZE); break;
                            case VK_SUBTRACT:   _tcsncat(mdata, _T("-"),       FILESIZE); break;
                            case VK_ADD:        _tcsncat(mdata, _T("+"),       FILESIZE); break;
                            case VK_DIVIDE:     _tcsncat(mdata, _T("/"),       FILESIZE); break;
                            case VK_MULTIPLY:   _tcsncat(mdata, _T("*"),       FILESIZE); break;
                            case VK_INSERT:     _tcsncat(mdata, _T("Ins"),     FILESIZE); break;
                            case VK_DELETE:     _tcsncat(mdata, _T("Del"),     FILESIZE); break;
                            case VK_TAB:        _tcsncat(mdata, _T("Tab"),     FILESIZE); break;
                            case VK_DOWN:       _tcsncat(mdata, _T("↓"),       FILESIZE); break;
                            case VK_RIGHT:      _tcsncat(mdata, _T("→"),       FILESIZE); break;
                            case VK_UP:         _tcsncat(mdata, _T("↑"),       FILESIZE); break;
                            case VK_LEFT:       _tcsncat(mdata, _T("←"),       FILESIZE); break;
                            case VK_HOME:       _tcsncat(mdata, _T("Home"),    FILESIZE); break;
                            case VK_NEXT:       _tcsncat(mdata, _T("PageDown"),FILESIZE); break;
                            case VK_PRIOR:      _tcsncat(mdata, _T("PageUp"),  FILESIZE); break;
                            case VK_END:        _tcsncat(mdata, _T("End"),     FILESIZE); break;
                            case VK_BACK:       _tcsncat(mdata, _T("Back"),    FILESIZE); break;
                            case VK_SPACE:      _tcsncat(mdata, _T("Space"),   FILESIZE); break;
                            case VK_RETURN:     _tcsncat(mdata, _T("Return"),  FILESIZE); break;
                            case VK_ESCAPE:     _tcsncat(mdata, _T("Esc"),     FILESIZE); break;
                            default:            _tcsncat(mdata, _T("Unkown"),  FILESIZE); break;
                        }
                    }
                    menu_modify_string(hmenu, id, mdata);
                }
            }
            if (id == IDM_VIEW_ZOOMOUT)
            {
                menu_default_keys(hmenu, IDM_VIEW_ZOOMOUT, pos);
            }
            if (id == IDM_VIEW_ZOOMIN)
            {
                menu_default_keys(hmenu, IDM_VIEW_ZOOMIN, pos);
            }
            if (id == IDM_SELECTION_RECTANGLE)
            {
                menu_default_keys(hmenu, IDM_SELECTION_RECTANGLE, pos);
            }
        }
    }
    return id;
}

bool
menu_setup(HWND hwnd)
{
    if (eu_get_config() && SetMenu(hwnd, i18n_load_menu(IDC_SKYLARK)))
    {
        HMENU root_menu = GetMenu(hwnd);
        HMENU setting_menu = root_menu ? GetSubMenu(root_menu, THEME_MENU) : NULL;
        setting_menu ? on_setting_update_menu(setting_menu) : (void)0;
        if (!eu_get_config()->m_menubar)
        {
            SetMenu(hwnd, NULL);
        }
        return true;
    }
    return false;
}

void
menu_update_item(const HMENU menu, const bool init)
{
    if (menu)
    {
        int count = GetMenuItemCount(menu);
        eu_tabpage *pnode = on_tabpage_focus_at();
        if (count > 0 && eu_get_config() && pnode)
        {
            int enable = 0;
            menu_update_hexview(menu, pnode->hex_mode, init);
            for (int i = 0; i < count; ++i)
            {
                uint32_t m_id = menu_update_string(menu, i);
                switch (m_id)
                {
                    case IDM_HISTORY_BASE:
                        on_file_update_recent_menu();
                        break;
                    case IDM_FILE_EXIT_WHEN_LAST_TAB:          /* File menu */
                        util_enable_menu_item(menu, IDM_FILE_SAVE, init || (on_sci_doc_modified(pnode) && !(pnode->file_attr & FILE_ATTRIBUTE_READONLY)));
                        util_enable_menu_item(menu, IDM_FILE_SAVEAS, init || pnode->filename[0]);
                        util_enable_menu_item(menu, IDM_FILE_REMOTE_FILESERVERS, init || util_exist_libcurl());
                        util_set_menu_item(menu, IDM_FILE_WRITE_COPY, eu_get_config()->m_write_copy);
                        util_set_menu_item(menu, IDM_FILE_SESSION, eu_get_config()->m_session);
                        util_set_menu_item(menu, IDM_FILE_EXIT_WHEN_LAST_TAB, eu_get_config()->m_exit);
                        break;
                    case IDM_FILE_NEWFILE_WINDOWS_EOLS:
                        util_set_menu_item(menu, IDM_FILE_NEWFILE_WINDOWS_EOLS, (eu_get_config()->new_file_eol == 0));
                        util_set_menu_item(menu, IDM_FILE_NEWFILE_MAC_EOLS, (eu_get_config()->new_file_eol == 1));
                        util_set_menu_item(menu, IDM_FILE_NEWFILE_UNIX_EOLS, (eu_get_config()->new_file_eol == 2));
                        break;
                    case IDM_FILE_NEWFILE_ENCODING_UTF8:
                        util_set_menu_item(menu, IDM_FILE_NEWFILE_ENCODING_UTF8, (eu_get_config()->new_file_enc == IDM_UNI_UTF8));
                        util_set_menu_item(menu, IDM_FILE_NEWFILE_ENCODING_UTF8B, (eu_get_config()->new_file_enc == IDM_UNI_UTF8B));
                        util_set_menu_item(menu, IDM_FILE_NEWFILE_ENCODING_UTF16LE, (eu_get_config()->new_file_enc == IDM_UNI_UTF16LEB));
                        util_set_menu_item(menu, IDM_FILE_NEWFILE_ENCODING_UTF16BE, (eu_get_config()->new_file_enc == IDM_UNI_UTF16BEB));
                        util_set_menu_item(menu, IDM_FILE_NEWFILE_ENCODING_ANSI, (eu_get_config()->new_file_enc == IDM_OTHER_ANSI));
                        break;
                    case IDM_FILE_RELOAD_CURRENT:
                        util_enable_menu_item(menu, IDM_FILE_RELOAD_CURRENT, pnode && !pnode->hex_mode && !url_has_remote(pnode->pathfile));
                        break;
                    case IDM_FILE_ADD_FAVORITES:
                        util_enable_menu_item(menu, IDM_FILE_ADD_FAVORITES, init || !pnode->is_blank);
                    case IDM_FILE_RESTART_ADMIN:
                        init ? (void)0 : menu_shield_icons(menu, IDM_FILE_RESTART_ADMIN, IDM_FILE_RESTART_ADMIN);
                        util_enable_menu_item(menu, IDM_FILE_RESTART_ADMIN, init || !util_under_wine());
                        break;
                    case IDM_EDIT_UNDO:                       /* Edit menu */
                    case IDM_EDIT_REDO:
                        util_enable_menu_item(menu, IDM_EDIT_UNDO, init || eu_sci_call(pnode,SCI_CANUNDO, 0, 0));
                        util_enable_menu_item(menu, IDM_EDIT_REDO, init || eu_sci_call(pnode,SCI_CANREDO, 0, 0));
                        break;
                    case IDM_EDIT_CUT:
                    case IDM_EDIT_COPY:
                    case IDM_EDIT_PASTE:
                    case IDM_EDIT_DELETE:
                        util_enable_menu_item(menu, IDM_EDIT_CUT, init || (!pnode->plugin && (pnode->hex_mode || util_can_selections(pnode))));
                        util_enable_menu_item(menu, IDM_EDIT_COPY, init || (!pnode->pmod && TAB_NOT_NUL(pnode)));
                        util_enable_menu_item(menu, IDM_EDIT_PASTE, init || !pnode->plugin);
                        util_enable_menu_item(menu, IDM_EDIT_DELETE, init || (!pnode->plugin && TAB_NOT_NUL(pnode)));
                        util_enable_menu_item(menu, IDM_EDIT_PLACEHOLDE2, init || (!pnode->hex_mode && TAB_NOT_NUL(pnode)));
                        util_enable_menu_item(menu, IDM_EDIT_PLACEHOLDE5, init || (!pnode->hex_mode && TAB_NOT_NUL(pnode)));
                        util_enable_menu_item(menu, IDM_EDIT_PLACEHOLDE6, init || (!pnode->hex_mode && TAB_NOT_NUL(pnode)));
                        util_enable_menu_item(menu, IDM_EDIT_SELECT_GROUP, init || util_can_selections(pnode));
                        util_enable_menu_item(menu, IDM_EDIT_PLACEHOLDE7, init || (!pnode->hex_mode && eu_exist_libssl() && util_can_selections(pnode)));
                        util_enable_menu_item(menu, IDM_EDIT_PLACEHOLDE8, init || (!pnode->hex_mode && eu_exist_libssl() && util_can_selections(pnode)));
                        util_enable_menu_item(menu, IDM_EDIT_PLACEHOLDE9, init || (!pnode->hex_mode && eu_exist_libssl() && util_can_selections(pnode)));
                        break;
                    case IDM_EDIT_COPY_INCREMENTAL:
                        util_enable_menu_item(menu, IDM_EDIT_COPY_INCREMENTAL, init || (!pnode->plugin && TAB_NOT_NUL(pnode)));
                    case IDM_EDIT_COPY_RTF:
                        util_enable_menu_item(menu, IDM_EDIT_COPY_RTF, init || (!pnode->hex_mode && !pnode->plugin && TAB_NOT_NUL(pnode)));
                        break;
                    case IDM_EDIT_SWAP_CLIPBOARD:
                        util_enable_menu_item(menu, IDM_EDIT_SWAP_CLIPBOARD, init || (!pnode->hex_mode && !pnode->plugin && eu_sci_call(pnode, SCI_CANPASTE, 0, 0)));
                        break;
                    case IDM_EDIT_CLEAR_CLIPBOARD:
                        util_enable_menu_item(menu, IDM_EDIT_CLEAR_CLIPBOARD, init || on_edit_can_paste());
                        break;
                    case IDM_EDIT_OTHER_EDITOR:
                        util_enable_menu_item(menu, IDM_EDIT_OTHER_EDITOR, init || (!pnode->plugin && !pnode->is_blank));
                        break;
                    case IDM_EDIT_OTHER_BCOMPARE:
                    {
                        int num = on_tabpage_sel_number(NULL, false);
                        util_enable_menu_item(menu, IDM_EDIT_OTHER_BCOMPARE, num > 1 && num < 4);
                        break;
                    }
                    case IDM_EDIT_SLASH_BACKSLASH:
                    case IDM_EDIT_BACKSLASH_SLASH:
                    {
                        enable = !pnode->hex_mode && !pnode->plugin && TAB_NOT_NUL(pnode) && util_can_selections(pnode);
                        util_enable_menu_item(menu, IDM_EDIT_SLASH_BACKSLASH, init || enable);
                        util_enable_menu_item(menu, IDM_EDIT_BACKSLASH_SLASH, init || enable);
                    }
                    case IDM_UPDATE_SELECTION:              /* Search menu */
                        util_set_menu_item(menu, IDM_UPDATE_SELECTION, pnode->begin_pos >= 0);
                        util_set_menu_item(menu, IDM_SELECTION_RECTANGLE, eu_sci_call(pnode, SCI_GETSELECTIONMODE, 0, 0) > 0);
                        break;
                    case IDM_SEARCH_MATCHING_BRACE:
                    case IDM_SEARCH_MATCHING_BRACE_SELECT:
                    {
                        on_search_jmp_matching_brace(pnode, &enable);
                        util_enable_menu_item(menu, IDM_SEARCH_MATCHING_BRACE, init || enable);
                        util_enable_menu_item(menu, IDM_SEARCH_MATCHING_BRACE_SELECT, init || enable);
                        break;
                    }
                    case IDM_VIEW_SCROLLCURSOR:            /* View menu */
                    {
                        util_set_menu_item(menu, IDM_VIEW_FILETREE, eu_get_config()->m_ftree_show);
                        util_set_menu_item(menu, IDM_VIEW_DOCUMENT_MAP, pnode->map_show);
                        util_set_menu_item(menu, IDM_VIEW_SYMTREE, pnode->sym_show);
                        util_enable_menu_item(menu, IDM_VIEW_SYMTREE, init || pnode->hwnd_symlist || pnode->hwnd_symtree);
                        util_set_menu_item(GetSubMenu(menu, TAB_MENU_PANELS_SUB), IDM_VIEW_FULLSCREEN, eu_get_config()->m_fullscreen);
                        util_set_menu_item(GetSubMenu(menu, TAB_MENU_PANELS_SUB), IDM_VIEW_MENUBAR, eu_get_config()->m_menubar);
                        util_set_menu_item(GetSubMenu(menu, TAB_MENU_PANELS_SUB), IDM_VIEW_STATUSBAR, eu_get_config()->m_statusbar);
                        util_set_menu_item(GetSubMenu(menu, TAB_MENU_HILIGHT_SUB), IDM_VIEW_HIGHLIGHT_BRACE, eu_get_config()->eu_brace.matching);
                        util_set_menu_item(GetSubMenu(menu, TAB_MENU_HILIGHT_SUB), IDM_VIEW_HIGHLIGHT_STR, eu_get_config()->m_light_str);
                        util_set_menu_item(GetSubMenu(menu, TAB_MENU_HILIGHT_SUB), IDM_VIEW_HIGHLIGHT_FOLD, eu_get_config()->light_fold);
                        util_set_menu_item(GetSubMenu(menu, TAB_DISPLAY_SUB), IDM_VIEW_LINENUMBER_VISIABLE, eu_get_config()->m_linenumber);
                        util_set_menu_item(GetSubMenu(menu, TAB_DISPLAY_SUB), IDM_VIEW_BOOKMARK_VISIABLE, eu_get_config()->eu_bookmark.visable);
                        util_set_menu_item(GetSubMenu(menu, TAB_DISPLAY_SUB), IDM_VIEW_WHITESPACE_VISIABLE, eu_get_config()->ws_visiable);
                        util_set_menu_item(GetSubMenu(menu, TAB_DISPLAY_SUB), IDM_VIEW_NEWLINE_VISIABLE, eu_get_config()->newline_visialbe);
                        util_set_menu_item(GetSubMenu(menu, TAB_DISPLAY_SUB), IDM_VIEW_INDENTGUIDES_VISIABLE, eu_get_config()->m_indentation);
                        util_set_menu_item(GetSubMenu(menu, TAB_DISPLAY_SUB), IDM_VIEW_TIPS_ONTAB, eu_get_config()->m_tab_tip);
                        util_set_menu_item(GetSubMenu(menu, TAB_DISPLAY_SUB), IDM_VIEW_CODE_HINT, eu_get_config()->m_code_hint);
                        enable = eu_get_config()->block_fold && pnode->foldline;
                        util_set_menu_item(GetSubMenu(menu, TAB_DISPLAY_SUB), IDM_VIEW_FOLDLINE_VISIABLE, enable);
                        util_enable_menu_item(GetSubMenu(menu, TAB_DISPLAY_SUB), IDM_VIEW_FOLDLINE_VISIABLE, init || (!pnode->hex_mode && !pnode->plugin && pnode->foldline));
                        util_set_menu_item(menu, IDM_EDIT_AUTO_INDENTATION, eu_get_config()->m_ident);
                        util_set_menu_item(menu, IDM_VIEW_HEXEDIT_MODE, pnode->hex_mode);
                        util_enable_menu_item(menu, IDM_VIEW_HEXEDIT_MODE, init || (pnode->codepage != IDM_OTHER_BIN && TAB_NOT_NUL(pnode)));
                        if (pnode->doc_ptr)
                        {
                            util_update_menu_chars(menu, IDM_VIEW_TAB_WIDTH, pnode->doc_ptr->tab_width > 0 ? pnode->doc_ptr->tab_width : eu_get_config()->tab_width);
                            util_set_menu_item(menu, IDM_TAB_CONVERT_SPACES, pnode->doc_ptr->tab_convert_spaces >= 0 ? pnode->doc_ptr->tab_convert_spaces : eu_get_config()->tab2spaces);
                        }
                        else
                        {
                            util_update_menu_chars(menu, IDM_VIEW_TAB_WIDTH, eu_get_config()->tab_width);
                            util_set_menu_item(menu, IDM_TAB_CONVERT_SPACES, eu_get_config()->tab2spaces);
                        }
                        util_set_menu_item(GetSubMenu(menu, TAB_MENU_TITLEBAR_SUB), IDM_VIEW_TITLEBAR_ICON, eu_get_config()->eu_titlebar.icon);
                        util_set_menu_item(GetSubMenu(menu, TAB_MENU_TITLEBAR_SUB), IDM_VIEW_TITLEBAR_NAME, eu_get_config()->eu_titlebar.name);
                        util_set_menu_item(GetSubMenu(menu, TAB_MENU_TITLEBAR_SUB), IDM_VIEW_TITLEBAR_PATH, eu_get_config()->eu_titlebar.path);
                        util_switch_menu_group(menu, TAB_HISTORY_SUB, IDM_VIEW_HISTORY_NONE, IDM_VIEW_HISTORY_ALL, eu_get_config()->history_mask);
                        util_switch_menu_group(menu, TAB_MENU_TOOLBAR_SUB, IDB_SIZE_0, IDB_SIZE_128, eu_get_config()->m_toolbar);
                        util_switch_menu_group(menu, TAB_MENU_ACTIVE_SUB, IDM_VIEW_LEFT_TAB, IDM_VIEW_FAR_RIGHT_TAB, eu_get_config()->m_tab_active);
                        util_switch_menu_group(menu, TAB_MENU_CLOSE_SUB, IDM_VIEW_TAB_RIGHT_CLICK, IDM_VIEW_TAB_LEFT_DBCLICK, eu_get_config()->m_close_way);
                        util_switch_menu_group(menu, TAB_MENU_NEW_SUB, IDM_VIEW_TAB_RIGHT_NEW, IDM_VIEW_TAB_DBCLICK_NEW, eu_get_config()->m_new_way);
                        util_switch_menu_group(menu, TAB_MENU_CBUTTON_SUB, IDM_TABCLOSE_FOLLOW, IDM_TABCLOSE_NONE, eu_get_config()->m_close_draw);
                        util_set_menu_item(menu, IDM_VIEW_SCROLLCURSOR, eu_get_config()->scroll_to_cursor);
                        break;
                    }
                    case IDM_VIEW_SWITCH_TAB:
                    {
                        util_enable_menu_item(menu, IDM_VIEW_SWITCH_TAB, init || (g_tabpages && TabCtrl_GetItemCount(g_tabpages) > 1));
                        break;
                    }
                    case IDM_VIEW_WRAPLINE_MODE:      /* Format menu */
                    {
                        enable = eu_exist_file(CLANGDLL);
                        util_set_menu_item(menu, IDM_VIEW_WRAPLINE_MODE, eu_get_config()->line_mode);
                        util_enable_menu_item(menu, IDM_EDIT_PLACEHOLDE_JSON, init || (pnode->doc_ptr && !pnode->hex_mode && pnode->doc_ptr->doc_type == DOCTYPE_JSON && enable));
                        util_enable_menu_item(menu, IDM_EDIT_PLACEHOLDE_JS, init || (pnode->doc_ptr && !pnode->hex_mode && pnode->doc_ptr->doc_type == DOCTYPE_JAVASCRIPT && enable));
                        util_enable_menu_item(menu, IDM_EDIT_PLACEHOLDE_CLANG, init || (
                                             (pnode->doc_ptr && !pnode->hex_mode && enable &&
                                             (pnode->doc_ptr->doc_type == DOCTYPE_CPP ||
                                             pnode->doc_ptr->doc_type == DOCTYPE_CS ||
                                             pnode->doc_ptr->doc_type == DOCTYPE_VERILOG ||
                                             pnode->doc_ptr->doc_type == DOCTYPE_JAVA ||
                                             pnode->doc_ptr->doc_type == DOCTYPE_JAVASCRIPT ||
                                             pnode->doc_ptr->doc_type == DOCTYPE_JSON))));
                        util_enable_menu_item(menu, IDM_EDIT_PLACEHOLDE_XML, init ||(pnode->doc_ptr && !pnode->hex_mode && pnode->doc_ptr->doc_type == DOCTYPE_XML));
                        util_enable_menu_item(menu, IDM_EDIT_PLACEHOLDE_LUA, init ||(pnode->doc_ptr && !pnode->hex_mode && pnode->doc_ptr->doc_type == DOCTYPE_LUA));
                        util_enable_menu_item(menu, IDM_EDIT_PLACEHOLDE_ICODE, init || (!pnode->hex_mode && !pnode->plugin && TAB_NOT_NUL(pnode)));
                        util_enable_menu_item(menu, IDM_FORMAT_PLACEHOLDE_PUNCTUATION, init || (!pnode->hex_mode && !pnode->plugin && TAB_NOT_NUL(pnode)));
                        break;
                    }
                    case IDM_FORMAT_HYPERLINKHOTSPOTS:
                    {
                        util_set_menu_item(menu, IDM_FORMAT_HYPERLINKHOTSPOTS, eu_get_config()->m_hyperlink);
                        util_enable_menu_item(menu, IDM_FORMAT_HYPERLINKHOTSPOTS, init || (!pnode->hex_mode && !pnode->plugin && TAB_NOT_NUL(pnode)));
                        break;
                    }
                    case IDM_FORMAT_CHECK_INDENTATION:
                    {
                        util_enable_menu_item(menu, IDM_FORMAT_CHECK_INDENTATION, init || (!pnode->hex_mode && !pnode->plugin && TAB_NOT_NUL(pnode)));
                        break;
                    }
                    case IDM_EDIT_QRCODE:
                        util_enable_menu_item(menu, IDM_EDIT_QRCODE, init || (!pnode->plugin && util_can_selections(pnode)));
                        break;
                    case IDM_SOURCE_BLOCKFOLD_TOGGLE: /* Programming menu */
                    case IDM_SOURCE_BLOCKFOLD_CONTRACTALL:
                    case IDM_SOURCE_BLOCKFOLD_EXPANDALL:
                        enable = eu_get_config()->block_fold && pnode->foldline && !pnode->hex_mode && !pnode->plugin && TAB_NOT_NUL(pnode);
                        util_enable_menu_item(menu, IDM_SOURCE_BLOCKFOLD_TOGGLE, init || enable);
                        util_enable_menu_item(menu, IDM_SOURCE_BLOCKFOLD_CONTRACTALL, init || enable);
                        util_enable_menu_item(menu, IDM_SOURCE_BLOCKFOLD_EXPANDALL, init || enable);
                        break;
                    case IDM_SOURCECODE_GOTODEF:
                        enable = pnode->doc_ptr && !pnode->hex_mode && pnode->hwnd_symlist;
                        util_enable_menu_item(menu, IDM_SOURCECODE_GOTODEF, init || enable);
                        break;
                    case IDM_EDIT_AUTO_CLOSECHAR:
                        util_set_menu_item(menu, IDM_EDIT_AUTO_CLOSECHAR, eu_get_config()->eu_brace.autoc);
                        util_set_menu_item(menu, IDM_SOURCEE_ENABLE_ACSHOW, eu_get_config()->eu_complete.enable);
                        util_update_menu_chars(menu, IDM_SOURCEE_ACSHOW_CHARS, eu_get_config()->eu_complete.characters);
                        util_set_menu_item(menu, IDM_SOURCE_ENABLE_CTSHOW, eu_get_config()->eu_calltip.enable);
                        break;
                    case IDM_DATABASE_INSERT_CONFIG:
                    case IDM_DATABASE_EXECUTE_SQL:
                        enable = pnode->doc_ptr && !pnode->hex_mode && (pnode->doc_ptr->doc_type == DOCTYPE_SQL||pnode->doc_ptr->doc_type == DOCTYPE_REDIS);
                        util_enable_menu_item(menu, IDM_DATABASE_INSERT_CONFIG, init || enable);
                        util_enable_menu_item(menu, IDM_DATABASE_EXECUTE_SQL, init || enable);
                        util_switch_menu_group(menu, PROGRAM_SNIPPET_SUB, IDM_SOURCE_SNIPPET_ENABLE, IDM_SOURCE_SNIPPET_ENABLE, eu_get_config()->eu_complete.snippet);
                        util_enable_menu_item(menu, IDM_EDIT_COMMENT_GROUP, init || (!pnode->hex_mode && pnode->doc_ptr && TAB_NOT_NUL(pnode)));
                        util_enable_menu_item(menu, IDM_SOURCE_SNIPPET_GROUP, init || (!pnode->hex_mode && pnode->doc_ptr));
                        break;
                    case IDM_PROGRAM_EXECUTE_ACTION:
                        enable = pnode->doc_ptr && !pnode->hex_mode && TAB_NOT_NUL(pnode);
                        util_enable_menu_item(menu, IDM_PROGRAM_EXECUTE_ACTION,  init || enable);
                        break;
                    case IDM_ENV_FILE_POPUPMENU:      /* Settings menu */
                        on_reg_update_menu();
                        i18n_update_multi_lang(menu);
                        i18n_update_menu(menu);
                        on_theme_update_item();
                        menu_switch_theme();
                        if (!init)
                        {   // 盾牌图标
                            menu_shield_icons(menu, IDM_ENV_FILE_POPUPMENU, IDM_ENV_SET_ASSOCIATED_WITH);
                            // 定制命令图标
                            menu_setting_icons(menu);
                            // 定制命令菜单更新
                            on_setting_update_menu(menu);
                        }
                        break;
                    case IDM_VIEW_FONTQUALITY_NONE:
                        util_set_menu_item(menu, IDM_VIEW_FONTQUALITY_NONE, IDM_VIEW_FONTQUALITY_NONE == eu_get_config()->m_quality);
                        util_set_menu_item(menu, IDM_VIEW_FONTQUALITY_STANDARD, IDM_VIEW_FONTQUALITY_STANDARD == eu_get_config()->m_quality);
                        util_set_menu_item(menu, IDM_VIEW_FONTQUALITY_CLEARTYPE, IDM_VIEW_FONTQUALITY_CLEARTYPE == eu_get_config()->m_quality);
                        break;
                    case IDM_SET_RENDER_TECH_GDI:
                        util_set_menu_item(menu, IDM_SET_RENDER_TECH_GDI, IDM_SET_RENDER_TECH_GDI == eu_get_config()->m_render);
                        util_set_menu_item(menu, IDM_SET_RENDER_TECH_D2D, IDM_SET_RENDER_TECH_D2D == eu_get_config()->m_render);
                        util_set_menu_item(menu, IDM_SET_RENDER_TECH_D2DRETAIN, IDM_SET_RENDER_TECH_D2DRETAIN == eu_get_config()->m_render);
                        break;
                    case IDM_SET_RESET_CONFIG:
                        util_enable_menu_item(menu, IDM_SET_RESET_CONFIG, init || (eu_hwnd_self() == share_envent_get_hwnd()));
                        break;
                    case IDM_SET_LOGGING_ENABLE:
                        util_set_menu_item(menu, IDM_SET_LOGGING_ENABLE, eu_get_config()->m_logging);
                        break;
                    case IDM_FILE_SAVE_NOTIFY:
                        util_update_menu_chars(menu, IDM_FILE_SAVE_NOTIFY, eu_get_config()->m_up_notify);
                        util_set_menu_item(menu, IDM_FILE_SAVE_NOTIFY, eu_get_config()->m_session && eu_get_config()->m_up_notify > 0);
                        util_enable_menu_item(menu, IDM_FILE_SAVE_NOTIFY,  init || (!pnode->hex_mode && eu_hwnd_self() == share_envent_get_hwnd()));
                        break;
                    case IDM_SKYLAR_AUTOMATIC_UPDATE:
                        enable = util_upcheck_exist();
                        util_enable_menu_item(menu, IDM_SKYLAR_AUTOMATIC_UPDATE, init || (eu_hwnd_self() == share_envent_get_hwnd() && enable));
                        util_set_menu_item(menu, IDM_SKYLAR_AUTOMATIC_UPDATE, eu_get_config()->upgrade.enable && enable);
                        break;
                    case IDM_ABOUT:                  /* Help menu */
                        util_enable_menu_item(menu, IDM_DONATION, init || util_exist_libcurl());
                        break;
                    default:
                        break;
                }
            }
        }
    }
}
