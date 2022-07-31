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

#define CLANGDLL _T("clang-format.dll")

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

HMENU
menu_load(uint16_t mid)
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
menu_pop_track(HWND hwnd, uint16_t mid, LPARAM lparam, const uint32_t flags, ptr_menu_callback fn, void *param)
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
                TCHAR buf[ACNAME_LEN] = { 0 };
                TCHAR theme[ACNAME_LEN] = { 0 };
                int len = GetMenuString(root_menu, IDM_STYLETHEME_BASE + index, buf, ACNAME_LEN - 1, MF_BYCOMMAND);
                if (len > 0 && MultiByteToWideChar(CP_UTF8, 0, eu_get_theme()->name, -1, theme, ACNAME_LEN) > 0)
                {
                    TCHAR* pbuf = on_theme_query_name(buf);
                    if (pbuf && _tcscmp(pbuf, theme) == 0)
                    {
                        select = true;
                    }
                    if (_stricmp(eu_get_config()->window_theme, eu_get_theme()->name))
                    {
                        strncpy(eu_get_config()->window_theme, eu_get_theme()->name, ACNAME_LEN);
                    }
                }
                util_set_menu_item(root_menu, IDM_STYLETHEME_BASE + index, select);
            }
        }
    }
}

static void
menu_update_hexview(HMENU root_menu, bool hex_mode)
{
    if (root_menu)
    {
        util_enable_menu_item(root_menu, IDM_EDIT_UNDO, !hex_mode);
        util_enable_menu_item(root_menu, IDM_EDIT_REDO, !hex_mode);
        util_enable_menu_item(root_menu, IDM_EDIT_DELETE, !hex_mode);
        util_enable_menu_item(root_menu, IDM_EDIT_PASTE, !hex_mode);
        util_enable_menu_item(root_menu, IDM_EDIT_PLACEHOLDE1, !hex_mode);
        util_enable_menu_item(root_menu, IDM_EDIT_PLACEHOLDE7, !hex_mode && eu_exist_libssl());
        util_enable_menu_item(root_menu, IDM_EDIT_PLACEHOLDE8, !hex_mode && eu_exist_libssl());
        util_enable_menu_item(root_menu, IDM_EDIT_PLACEHOLDE9, !hex_mode && eu_exist_libssl());
        util_enable_menu_item(root_menu, IDM_UPDATE_SELECTION, !hex_mode);
        util_enable_menu_item(root_menu, IDM_EDIT_PLACEHOLDE10, !hex_mode);
        util_enable_menu_item(root_menu, IDM_EDIT_PLACEHOLDE11, !hex_mode);
        util_enable_menu_item(root_menu, IDM_EDIT_PLACEHOLDE12, !hex_mode);
        util_enable_menu_item(root_menu, IDM_EDIT_PLACEHOLDE13, !hex_mode);
        util_enable_menu_item(root_menu, IDM_SEARCH_REPLACE, !hex_mode);
        util_enable_menu_item(root_menu, IDM_SELECTION_RECTANGLE, !hex_mode);
        util_enable_menu_item(root_menu, IDM_SEARCH_MULTISELECT_README, !hex_mode);
        util_enable_menu_item(root_menu, IDM_VIEW_TAB_WIDTH, !hex_mode);
        util_enable_menu_item(root_menu, IDM_TAB_CONVERT_SPACES, !hex_mode);
        util_enable_menu_item(root_menu, IDM_VIEW_DOCUMENT_MAP, !hex_mode);
        util_enable_menu_item(root_menu, IDM_VIEW_HIGHLIGHT_STR, !hex_mode);
        util_enable_menu_item(root_menu, IDM_VIEW_HIGHLIGHT_FOLD, !hex_mode);
        util_enable_menu_item(root_menu, IDM_EDIT_AUTO_INDENTATION, !hex_mode);
        util_enable_menu_item(root_menu, IDM_VIEW_LINENUMBER_VISIABLE, !hex_mode);
        util_enable_menu_item(root_menu, IDM_VIEW_BOOKMARK_VISIABLE, !hex_mode);
        util_enable_menu_item(root_menu, IDM_VIEW_WHITESPACE_VISIABLE, !hex_mode);
        util_enable_menu_item(root_menu, IDM_VIEW_NEWLINE_VISIABLE, !hex_mode);
        util_enable_menu_item(root_menu, IDM_VIEW_INDENTGUIDES_VISIABLE, !hex_mode);
        util_enable_menu_item(root_menu, IDM_VIEW_WRAPLINE_MODE, !hex_mode);
        util_enable_menu_item(root_menu, IDM_SOURCECODE_GOTODEF, !hex_mode);
        util_enable_menu_item(root_menu, IDM_EDIT_AUTO_CLOSECHAR, !hex_mode);
        util_enable_menu_item(root_menu, IDM_SOURCEE_ENABLE_ACSHOW, !hex_mode);
        util_enable_menu_item(root_menu, IDM_SOURCEE_ACSHOW_CHARS, !hex_mode);
        util_enable_menu_item(root_menu, IDM_SOURCE_ENABLE_CTSHOW, !hex_mode);
        util_enable_menu_item(root_menu, IDM_SETTING_FONTQUALITY, !hex_mode);
        util_enable_menu_item(root_menu, IDM_SETTING_RENDER, !hex_mode);
    }
}

static void
menu_default_keys(HMENU hmenu, int id, int pos)
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
        if (id == IDM_VIEW_ZOOMOUT)
        {
            _tcsncat(mdata, _T("Ctrl+MouseWheelUP"), FILESIZE);
        }
        else if (id == IDM_VIEW_ZOOMIN)
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

static uint32_t
menu_update_string(HMENU hmenu, int pos)
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
                    ModifyMenu(hmenu, pos, MF_BYPOSITION|MF_STRING, id, mdata);
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

void
menu_update_item(HMENU menu)
{
    if (menu)
    {
        int count = GetMenuItemCount(menu);
        eu_tabpage *pnode = on_tabpage_focus_at();
        if (count > 0 && eu_get_config() && pnode)
        {
            int enable = 0;
            menu_update_hexview(menu, pnode->hex_mode);
            for (int i = 0; i < count; ++i)
            {
                uint32_t m_id = menu_update_string(menu, i);
                switch (m_id)
                {
                    case IDM_HISTORY_BASE:
                        on_file_update_recent_menu();
                        break;
                    case IDM_FILE_EXIT_WHEN_LAST_TAB:          /* File menu */
                        util_enable_menu_item(menu, IDM_FILE_SAVE, on_sci_doc_modified(pnode));
                        util_enable_menu_item(menu, IDM_FILE_SAVEAS, pnode->filename[0]);
                        util_enable_menu_item(menu, IDM_FILE_REMOTE_FILESERVERS, util_exist_libcurl());
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
                    case IDM_EDIT_UNDO:                       /* Edit menu */
                    case IDM_EDIT_REDO:
                        util_enable_menu_item(menu, IDM_EDIT_UNDO, eu_sci_call(pnode,SCI_CANUNDO, 0, 0));
                        util_enable_menu_item(menu, IDM_EDIT_REDO, eu_sci_call(pnode,SCI_CANREDO, 0, 0));
                        break;    
                    case IDM_EDIT_CUT:    
                    case IDM_EDIT_COPY:
                    case IDM_EDIT_PASTE:
                    case IDM_EDIT_DELETE:    
                        util_enable_menu_item(menu, IDM_EDIT_CUT, util_can_selections(pnode));
                        util_enable_menu_item(menu, IDM_EDIT_COPY, util_can_selections(pnode));
                        util_enable_menu_item(menu, IDM_EDIT_PASTE, eu_sci_call(pnode,SCI_CANPASTE, 0, 0));
                        util_enable_menu_item(menu, IDM_EDIT_DELETE, eu_sci_call(pnode, SCI_GETLENGTH, 0, 0) > 0);
                        util_enable_menu_item(menu, IDM_EDIT_PLACEHOLDE2, !pnode->hex_mode && eu_sci_call(pnode, SCI_GETLENGTH, 0, 0) > 0);
                        util_enable_menu_item(menu, IDM_EDIT_PLACEHOLDE5, !pnode->hex_mode && eu_sci_call(pnode, SCI_GETLENGTH, 0, 0) > 0);
                        util_enable_menu_item(menu, IDM_EDIT_PLACEHOLDE6, !pnode->hex_mode && eu_sci_call(pnode, SCI_GETLENGTH, 0, 0) > 0);
                        util_enable_menu_item(menu, IDM_EDIT_SELECT_GROUP, util_can_selections(pnode));
                        break;
                    case IDM_EDIT_OTHER_EDITOR:
                        util_enable_menu_item(menu, IDM_EDIT_OTHER_EDITOR, !pnode->is_blank);
                        break;
                    case IDM_UPDATE_SELECTION:              /* Search menu */
                        util_set_menu_item(menu, IDM_UPDATE_SELECTION, pnode->begin_pos >= 0);
                        util_set_menu_item(menu, IDM_SELECTION_RECTANGLE, eu_sci_call(pnode, SCI_GETSELECTIONMODE, 0, 0) > 0);
                        break;
                    case IDM_SEARCH_MATCHING_BRACE:
                    case IDM_SEARCH_MATCHING_BRACE_SELECT:
                    {
                        on_search_jmp_matching_brace(pnode, &enable);
                        util_enable_menu_item(menu, IDM_SEARCH_MATCHING_BRACE, enable);
                        util_enable_menu_item(menu, IDM_SEARCH_MATCHING_BRACE_SELECT, enable);
                        break;
                    }
                    case IDM_VIEW_INDENTGUIDES_VISIABLE:      /* View menu */
                        util_set_menu_item(menu, IDM_VIEW_FILETREE, eu_get_config()->m_ftree_show);
                        util_set_menu_item(menu, IDM_VIEW_DOCUMENT_MAP, pnode->map_show);
                        util_set_menu_item(menu, IDM_VIEW_SYMTREE, pnode->sym_show);
                        util_enable_menu_item(menu, IDM_VIEW_SYMTREE, pnode->hwnd_symlist || pnode->hwnd_symtree);
                        util_set_menu_item(menu, IDM_VIEW_FULLSCREEN, eu_get_config()->m_fullscreen);
                        util_set_menu_item(menu, IDM_VIEW_MENUBAR, eu_get_config()->m_menubar);
                        util_set_menu_item(menu, IDM_VIEW_TOOLBAR, eu_get_config()->m_toolbar);
                        util_set_menu_item(menu, IDM_VIEW_STATUSBAR, eu_get_config()->m_statusbar);
                        util_set_menu_item(menu, IDM_VIEW_HIGHLIGHT_STR, eu_get_config()->m_light_str);
                        util_set_menu_item(menu, IDM_VIEW_HIGHLIGHT_FOLD, eu_get_config()->light_fold);
                        util_set_menu_item(menu, IDM_VIEW_WRAPLINE_MODE, eu_get_config()->line_mode);
                        util_set_menu_item(menu, IDM_VIEW_LINENUMBER_VISIABLE, eu_get_config()->m_linenumber);
                        util_set_menu_item(menu, IDM_VIEW_BOOKMARK_VISIABLE, eu_get_config()->bookmark_visable);
                        util_set_menu_item(menu, IDM_VIEW_WHITESPACE_VISIABLE, eu_get_config()->ws_visiable);
                        util_set_menu_item(menu, IDM_VIEW_NEWLINE_VISIABLE, eu_get_config()->newline_visialbe);
                        util_set_menu_item(menu, IDM_EDIT_AUTO_INDENTATION, eu_get_config()->m_ident);
                        enable = eu_get_config()->block_fold && pnode->foldline;
                        util_set_menu_item(menu, IDM_SOURCE_BLOCKFOLD_VISIABLE, enable);
                        util_enable_menu_item(menu, IDM_SOURCE_BLOCKFOLD_VISIABLE, !pnode->hex_mode && pnode->foldline);
                        util_set_menu_item(menu, IDM_VIEW_INDENTGUIDES_VISIABLE, eu_get_config()->m_indentation);
                        util_enable_menu_item(menu, IDM_VIEW_HEXEDIT_MODE, pnode->codepage != IDM_OTHER_BIN && eu_sci_call(pnode, SCI_GETLENGTH, 0, 0) > 0);
                        util_set_menu_item(menu, IDM_VIEW_HEXEDIT_MODE, pnode->hex_mode);
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
                        util_set_menu_item(menu, IDM_VIEW_TIPS_ONTAB, eu_get_config()->m_tab_tip);
                        util_switch_menu_group(menu, TAB_MENU_ACTIVE_SUB, IDM_VIEW_LEFT_TAB, IDM_VIEW_FAR_RIGHT_TAB, eu_get_config()->m_tab_active);
                        util_switch_menu_group(menu, TAB_MENU_CLOSE_SUB, IDM_VIEW_TAB_RIGHT_CLICK, IDM_VIEW_TAB_LEFT_DBCLICK, eu_get_config()->m_close_way);
                        break;
                    case IDM_VIEW_SWITCH_TAB:
                        util_enable_menu_item(menu, IDM_VIEW_SWITCH_TAB, g_tabpages && TabCtrl_GetItemCount(g_tabpages) > 1);
                        break;
                    case IDM_VIEW_WRAPLINE_MODE:      /* Format menu */
                        util_set_menu_item(menu, IDM_VIEW_WRAPLINE_MODE, eu_get_config()->line_mode);
                        util_enable_menu_item(menu, IDM_EDIT_PLACEHOLDE16, (pnode->doc_ptr && !pnode->hex_mode && pnode->doc_ptr->doc_type == DOCTYPE_JSON && eu_exist_file(CLANGDLL)));
                        util_enable_menu_item(menu, IDM_EDIT_PLACEHOLDE17,
                                             (pnode->doc_ptr && !pnode->hex_mode && eu_exist_file(CLANGDLL) &&
                                             (pnode->doc_ptr->doc_type == DOCTYPE_CPP ||
                                             pnode->doc_ptr->doc_type == DOCTYPE_CS ||
                                             pnode->doc_ptr->doc_type == DOCTYPE_JAVA ||
                                             pnode->doc_ptr->doc_type == DOCTYPE_JAVASCRIPT ||
                                             pnode->doc_ptr->doc_type == DOCTYPE_JSON)));
                        util_enable_menu_item(menu, IDM_EDIT_PLACEHOLDE18, (pnode->doc_ptr && !pnode->hex_mode && pnode->doc_ptr->doc_type == DOCTYPE_LUA));
                        util_enable_menu_item(menu, IDM_EDIT_PLACEHOLDE19, !pnode->hex_mode && eu_sci_call(pnode, SCI_GETLENGTH, 0, 0) > 0);
                        break;
                    case IDM_EDIT_QRCODE:
                        util_enable_menu_item(menu, IDM_EDIT_QRCODE, util_can_selections(pnode));
                        break;
                    case IDM_SOURCE_BLOCKFOLD_TOGGLE: /* Programming menu */
                    case IDM_SOURCE_BLOCKFOLD_CONTRACT:
                    case IDM_SOURCE_BLOCKFOLD_EXPAND:    
                        enable = eu_get_config()->block_fold && pnode->foldline && !pnode->hex_mode && eu_sci_call(pnode, SCI_GETLENGTH, 0, 0) > 0;
                        util_enable_menu_item(menu, IDM_SOURCE_BLOCKFOLD_TOGGLE, enable);
                        util_enable_menu_item(menu, IDM_SOURCE_BLOCKFOLD_CONTRACT, enable);
                        util_enable_menu_item(menu, IDM_SOURCE_BLOCKFOLD_EXPAND, enable);
                        util_enable_menu_item(menu, IDM_SOURCE_BLOCKFOLD_CONTRACTALL, enable);
                        util_enable_menu_item(menu, IDM_SOURCE_BLOCKFOLD_EXPANDALL, enable);
                        break;
                    case IDM_SOURCECODE_GOTODEF:
                        enable = pnode->doc_ptr && !pnode->hex_mode && pnode->hwnd_symlist;
                        util_enable_menu_item(menu, IDM_SOURCECODE_GOTODEF, enable);
                        break;    
                    case IDM_EDIT_AUTO_CLOSECHAR:
                        util_set_menu_item(menu, IDM_EDIT_AUTO_CLOSECHAR, eu_get_config()->auto_close_chars);
                        util_set_menu_item(menu, IDM_SOURCEE_ENABLE_ACSHOW, eu_get_config()->m_acshow);
                        util_update_menu_chars(menu, IDM_SOURCEE_ACSHOW_CHARS, eu_get_config()->acshow_chars);
                        util_set_menu_item(menu, IDM_SOURCE_ENABLE_CTSHOW, eu_get_config()->m_ctshow);
                        break;
                    case IDM_DATABASE_INSERT_CONFIG:
                    case IDM_DATABASE_EXECUTE_SQL:
                        enable = pnode->doc_ptr && !pnode->hex_mode && (pnode->doc_ptr->doc_type == DOCTYPE_SQL||pnode->doc_ptr->doc_type == DOCTYPE_REDIS);
                        util_enable_menu_item(menu, IDM_DATABASE_INSERT_CONFIG, enable);
                        util_enable_menu_item(menu, IDM_DATABASE_EXECUTE_SQL, enable);
                        util_switch_menu_group(menu, TAB_MENU_SNIPPET_SUB, IDM_SOURCE_SNIPPET_ENABLE, IDM_SOURCE_SNIPPET_ENABLE, eu_get_config()->m_snippet_enable);
                        util_enable_menu_item(menu, IDM_EDIT_PLACEHOLDE21, !pnode->hex_mode && pnode->doc_ptr && eu_sci_call(pnode, SCI_GETLENGTH, 0, 0) > 0);
                        util_enable_menu_item(menu, IDM_SOURCE_SNIPPET_GROUP, !pnode->hex_mode && pnode->doc_ptr && eu_sci_call(pnode, SCI_GETLENGTH, 0, 0) > 0);
                        break;
                    case IDM_PROGRAM_EXECUTE_ACTION:
                        enable = pnode->doc_ptr && !pnode->hex_mode && eu_sci_call(pnode, SCI_GETLENGTH, 0, 0) > 0;
                        util_enable_menu_item(menu, IDM_PROGRAM_EXECUTE_ACTION,  enable);
                        break;
                    case IDM_ENV_FILE_POPUPMENU:      /* Settings menu */
                        on_reg_update_menu();
                        i18n_update_multi_lang(menu);
                        i18n_update_menu(menu);
                        on_theme_update_item();
                        menu_switch_theme();
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
                    case IDM_ABOUT:                  /* Help menu */
                        util_enable_menu_item(menu, IDM_DONATION, util_exist_libcurl());
                        break;
                    default:
                        break;
                }
            }
        }
    }
}
