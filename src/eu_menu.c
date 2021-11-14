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

void
menu_switch_theme(HWND hwnd)
{
    if (hwnd)
    {
        HMENU view_menu = GetSubMenu(GetMenu(hwnd), THEME_MENU);
        HMENU theme_menu = GetSubMenu(view_menu, THEME_MENU_SUB);
        if (view_menu && theme_menu)
        {
            int count = GetMenuItemCount(theme_menu);
            for (int index = 0; index < count; ++index)
            {
                bool select = false;
                TCHAR buf[ACNAME_LEN] = { 0 };
                TCHAR theme[ACNAME_LEN] = { 0 };
                int len = GetMenuString(GetMenu(hwnd), IDM_STYLETHEME_BASE + index, buf, ACNAME_LEN - 1, MF_BYCOMMAND);
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
                util_set_menu_item(hwnd, IDM_STYLETHEME_BASE + index, select);
            }
        }
    }
}

static void
menu_update_input(int nid, int width)
{
    TCHAR *pstart = NULL;
    TCHAR *pend = NULL;
    TCHAR m_text[256] = {0};
    TCHAR new_text[256] = {0};
    MENUITEMINFO mii = {0};
    HWND hwnd = eu_module_hwnd();
    if (hwnd)
    {
        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask = MIIM_STRING;
        mii.dwTypeData = m_text;
        mii.cch = _countof(m_text) - 1;
        GetMenuItemInfo(GetMenu(hwnd), nid, 0, &mii);
        pstart = _tcschr(m_text, _T('['));
        if (pstart)
        {
            pend = _tcschr(pstart + 1, _T(']'));
            if (pend)
            {
                sntprintf(new_text,
                          _countof(new_text) - 1,
                          _T("%.*s%d%.*s"),
                          pstart - m_text + 1,
                          m_text,
                          width,
                          _tcslen(pend) + 1,
                          pend);
            }
            if (_tcslen(new_text) > 0)
            {
                mii.cch = (uint32_t) _tcslen(new_text);
                mii.dwTypeData = new_text;
                SetMenuItemInfo(GetMenu(hwnd), nid, 0, &mii);
            }
        }
    }
}

static void
menu_update_rside(HWND hwnd, eu_tabpage *p)
{
    bool m_show = false;
    if (p->hwnd_symlist)
    {
        m_show = (GetWindowLongPtr(p->hwnd_symlist, GWL_STYLE) & WS_VISIBLE);
    }
    else if (p->hwnd_symtree)
    {
        m_show = (GetWindowLongPtr(p->hwnd_symtree, GWL_STYLE) & WS_VISIBLE);
    }
    util_set_menu_item(hwnd, IDM_VIEW_SYMTREE, m_show);
    util_enable_menu_item(hwnd, IDM_VIEW_SYMTREE, p->hwnd_symlist || p->hwnd_symtree);
}

static void
menu_update_hexview(HWND hwnd, bool hex_mode)
{
    util_enable_menu_item(hwnd, IDM_EDIT_UNDO, !hex_mode);
    util_enable_menu_item(hwnd, IDM_EDIT_REDO, !hex_mode);
    util_enable_menu_item(hwnd, IDM_EDIT_DELETE, !hex_mode);
    util_enable_menu_item(hwnd, IDM_EDIT_PLACEHOLDE1, !hex_mode);
    util_enable_menu_item(hwnd, IDM_EDIT_PLACEHOLDE2, !hex_mode);
    util_enable_menu_item(hwnd, IDM_EDIT_PLACEHOLDE3, !hex_mode);
    util_enable_menu_item(hwnd, IDM_EDIT_PLACEHOLDE4, !hex_mode);
    util_enable_menu_item(hwnd, IDM_EDIT_PLACEHOLDE5, !hex_mode);
    util_enable_menu_item(hwnd, IDM_EDIT_PLACEHOLDE6, !hex_mode);
    util_enable_menu_item(hwnd, IDM_EDIT_PLACEHOLDE7, !hex_mode);
    util_enable_menu_item(hwnd, IDM_EDIT_PLACEHOLDE8, !hex_mode);
    util_enable_menu_item(hwnd, IDM_EDIT_PLACEHOLDE9, !hex_mode);
    util_enable_menu_item(hwnd, IDM_UPDATE_SELECTION, !hex_mode);
    util_enable_menu_item(hwnd, IDM_EDIT_PLACEHOLDE10, !hex_mode);
    util_enable_menu_item(hwnd, IDM_EDIT_PLACEHOLDE11, !hex_mode);
    util_enable_menu_item(hwnd, IDM_EDIT_PLACEHOLDE12, !hex_mode);
    util_enable_menu_item(hwnd, IDM_EDIT_PLACEHOLDE13, !hex_mode);
    util_enable_menu_item(hwnd, IDM_SEARCH_REPLACE, !hex_mode);
    util_enable_menu_item(hwnd, IDM_SELECTION_RECTANGLE, !hex_mode);
    util_enable_menu_item(hwnd, IDM_SEARCH_MULTISELECT_README, !hex_mode);
    util_enable_menu_item(hwnd, IDM_VIEW_TAB_WIDTH, !hex_mode);
    util_enable_menu_item(hwnd, IDM_TAB_CONVERT_SPACES, !hex_mode);
    util_enable_menu_item(hwnd, IDM_VIEW_HIGHLIGHT_STR, !hex_mode);
    util_enable_menu_item(hwnd, IDM_VIEW_HIGHLIGHT_FOLD, !hex_mode);
    util_enable_menu_item(hwnd, IDM_EDIT_AUTO_INDENTATION, !hex_mode);
    util_enable_menu_item(hwnd, IDM_VIEW_LINENUMBER_VISIABLE, !hex_mode);
    util_enable_menu_item(hwnd, IDM_VIEW_BOOKMARK_VISIABLE, !hex_mode);
    util_enable_menu_item(hwnd, IDM_VIEW_WHITESPACE_VISIABLE, !hex_mode);
    util_enable_menu_item(hwnd, IDM_VIEW_NEWLINE_VISIABLE, !hex_mode);
    util_enable_menu_item(hwnd, IDM_VIEW_INDENTGUIDES_VISIABLE, !hex_mode);
    util_enable_menu_item(hwnd, IDM_SOURCE_BLOCKFOLD_VISIABLE, !hex_mode);
    util_enable_menu_item(hwnd, IDM_EDIT_PLACEHOLDE16, !hex_mode);
    util_enable_menu_item(hwnd, IDM_EDIT_PLACEHOLDE17, !hex_mode);
    util_enable_menu_item(hwnd, IDM_EDIT_PLACEHOLDE18, !hex_mode);
    util_enable_menu_item(hwnd, IDM_EDIT_PLACEHOLDE19, !hex_mode);
    util_enable_menu_item(hwnd, IDM_VIEW_WRAPLINE_MODE, !hex_mode);
    util_enable_menu_item(hwnd, IDM_EDIT_PLACEHOLDE20, !hex_mode);
}

void
menu_update_text_status(HWND hwnd, eu_tabpage *pnode)
{
    if (eu_get_config()->m_menubar)
    {
        util_enable_menu_item(hwnd, IDM_EDIT_CUT, util_can_selections(pnode));
        util_enable_menu_item(hwnd, IDM_EDIT_COPY, util_can_selections(pnode));
    }
    if (eu_get_config()->m_toolbar)
    {
        on_toolbar_setup_button(IDM_EDIT_CUT, util_can_selections(pnode) ? 2 : 1);
        on_toolbar_setup_button(IDM_EDIT_COPY, util_can_selections(pnode) ? 2 : 1);
    }
}

void
menu_update_all(HWND hwnd, eu_tabpage *pnode)
{
    if (eu_get_config()->m_menubar)
    {
        if (hwnd)
        {
            on_file_update_recent();
            util_set_menu_item(hwnd, IDM_FILE_WRITE_COPY, eu_get_config()->m_write_copy);
            util_set_menu_item(hwnd, IDM_FILE_SESSION, eu_get_config()->m_session);
            util_set_menu_item(hwnd, IDM_FILE_NEWFILE_WINDOWS_EOLS, (eu_get_config()->new_file_eol == 0));
            util_set_menu_item(hwnd, IDM_FILE_NEWFILE_MAC_EOLS, (eu_get_config()->new_file_eol == 1));
            util_set_menu_item(hwnd, IDM_FILE_NEWFILE_UNIX_EOLS, (eu_get_config()->new_file_eol == 2));
            util_set_menu_item(hwnd, IDM_FILE_NEWFILE_ENCODING_UTF8, (eu_get_config()->new_file_enc == IDM_UNI_UTF8));
            util_set_menu_item(hwnd, IDM_FILE_NEWFILE_ENCODING_UTF8B, (eu_get_config()->new_file_enc == IDM_UNI_UTF8B));
            util_set_menu_item(hwnd, IDM_FILE_NEWFILE_ENCODING_UTF16LE, (eu_get_config()->new_file_enc == IDM_UNI_UTF16LEB));
            util_set_menu_item(hwnd, IDM_FILE_NEWFILE_ENCODING_UTF16BE, (eu_get_config()->new_file_enc == IDM_UNI_UTF16BEB));
            util_set_menu_item(hwnd, IDM_FILE_NEWFILE_ENCODING_ANSI, (eu_get_config()->new_file_enc == IDM_OTHER_ANSI));
            util_set_menu_item(hwnd, IDM_EDIT_AUTO_CLOSECHAR, eu_get_config()->auto_close_chars);
            util_set_menu_item(hwnd, IDM_EDIT_AUTO_INDENTATION, eu_get_config()->m_ident);
            
            on_theme_update_menu(hwnd);
            i18n_update_multi_lang(hwnd);
            menu_switch_theme(hwnd);
            i18n_update_menu(hwnd);
            
            util_set_menu_item(hwnd, IDM_VIEW_HIGHLIGHT_STR, eu_get_config()->m_light_str);
            util_set_menu_item(hwnd, IDM_VIEW_FILETREE, eu_get_config()->m_ftree_show);
            util_set_menu_item(hwnd, IDM_VIEW_FULLSCREEN, eu_get_config()->m_fullscreen);
            util_set_menu_item(hwnd, IDM_VIEW_MENUBAR, eu_get_config()->m_menubar);
            util_set_menu_item(hwnd, IDM_VIEW_TOOLBAR, eu_get_config()->m_toolbar);
            util_set_menu_item(hwnd, IDM_VIEW_STATUSBAR, eu_get_config()->m_statusbar);
            util_set_menu_item(hwnd, IDM_VIEW_HIGHLIGHT_FOLD, eu_get_config()->light_fold);
            util_set_menu_item(hwnd, IDM_VIEW_WRAPLINE_MODE, eu_get_config()->line_mode);
            util_set_menu_item(hwnd, IDM_VIEW_LINENUMBER_VISIABLE, eu_get_config()->m_linenumber);
            util_set_menu_item(hwnd, IDM_VIEW_BOOKMARK_VISIABLE, eu_get_config()->bookmark_visable);
            util_set_menu_item(hwnd, IDM_VIEW_WHITESPACE_VISIABLE, eu_get_config()->ws_visiable);
            util_set_menu_item(hwnd, IDM_VIEW_NEWLINE_VISIABLE, eu_get_config()->newline_visialbe);
            util_set_menu_item(hwnd, IDM_VIEW_INDENTGUIDES_VISIABLE, eu_get_config()->m_indentation);
            util_set_menu_item(hwnd, IDM_SOURCEE_ENABLE_ACSHOW, eu_get_config()->m_acshow);
            menu_update_input(IDM_SOURCEE_ACSHOW_CHARS, eu_get_config()->acshow_chars);
            util_set_menu_item(hwnd, eu_get_config()->m_quality, true);
            util_set_menu_item(hwnd, eu_get_config()->m_render, true);
            util_set_menu_item(hwnd, IDM_SOURCE_ENABLE_CTSHOW, eu_get_config()->m_ctshow);
            
            util_set_menu_item(hwnd, IDM_SOURCE_BLOCKFOLD_VISIABLE, eu_get_config()->block_fold);
            util_enable_menu_item(hwnd, IDM_SOURCE_BLOCKFOLD_TOGGLE, eu_get_config()->block_fold);
            util_enable_menu_item(hwnd, IDM_SOURCE_BLOCKFOLD_CONTRACT, eu_get_config()->block_fold);
            util_enable_menu_item(hwnd, IDM_SOURCE_BLOCKFOLD_EXPAND, eu_get_config()->block_fold);
            util_enable_menu_item(hwnd, IDM_SOURCE_BLOCKFOLD_CONTRACTALL, eu_get_config()->block_fold);
            util_enable_menu_item(hwnd, IDM_SOURCE_BLOCKFOLD_EXPANDALL, eu_get_config()->block_fold);

            if (pnode)
            {
                util_enable_menu_item(hwnd, IDM_FILE_SAVE, on_sci_doc_modified(pnode));
                util_enable_menu_item(hwnd, IDM_FILE_SAVEAS, pnode->filename[0]);
                util_enable_menu_item(hwnd, IDM_FILE_PRINT, true);
                util_enable_menu_item(hwnd, IDM_EDIT_PASTE, eu_sci_call(pnode,SCI_CANPASTE, 0, 0));
                util_enable_menu_item(hwnd, IDM_VIEW_HEXEDIT_MODE, pnode->codepage != IDM_OTHER_BIN);
                util_set_menu_item(hwnd, IDM_VIEW_HEXEDIT_MODE, pnode->hex_mode);
                util_set_menu_item(hwnd, IDM_UPDATE_SELECTION, pnode->begin_pos >= 0);
                util_set_menu_item(hwnd, IDM_SELECTION_RECTANGLE, eu_sci_call(pnode, SCI_GETSELECTIONMODE, 0, 0) > 0);
                menu_update_hexview(hwnd, pnode->hex_mode);
                menu_update_rside(hwnd, pnode);                
                util_enable_menu_item(hwnd, IDM_DATABASE_INSERT_CONFIG, (pnode->doc_ptr && !pnode->hex_mode && pnode->doc_ptr->doc_type == DOCTYPE_SQL));
                util_enable_menu_item(hwnd, IDM_DATABASE_EXECUTE_SQL, (pnode->doc_ptr && !pnode->hex_mode && pnode->doc_ptr->doc_type == DOCTYPE_SQL));
                util_enable_menu_item(hwnd, IDM_REDIS_INSERT_CONFIG, (pnode->doc_ptr && !pnode->hex_mode && pnode->doc_ptr->doc_type == DOCTYPE_REDIS));
                util_enable_menu_item(hwnd, IDM_REDIS_EXECUTE_COMMAND, (pnode->doc_ptr && !pnode->hex_mode && pnode->doc_ptr->doc_type == DOCTYPE_REDIS));
                util_enable_menu_item(hwnd, IDM_EDIT_PLACEHOLDE16, (pnode->doc_ptr && !pnode->hex_mode && pnode->doc_ptr->doc_type == DOCTYPE_JSON));
                util_enable_menu_item(hwnd, IDM_EDIT_PLACEHOLDE17, 
                                     (pnode->doc_ptr && !pnode->hex_mode && 
                                     (pnode->doc_ptr->doc_type == DOCTYPE_CPP ||
                                     pnode->doc_ptr->doc_type == DOCTYPE_CS ||
                                     pnode->doc_ptr->doc_type == DOCTYPE_JAVA ||
                                     pnode->doc_ptr->doc_type == DOCTYPE_JAVASCRIPT ||
                                     pnode->doc_ptr->doc_type == DOCTYPE_JSON)));
                util_enable_menu_item(hwnd, IDM_EDIT_PLACEHOLDE18, (pnode->doc_ptr && !pnode->hex_mode && pnode->doc_ptr->doc_type == DOCTYPE_LUA));
                util_enable_menu_item(hwnd, IDM_PROGRAM_EXECUTE_ACTION, pnode->doc_ptr && !pnode->hex_mode);
                if (pnode->doc_ptr)
                {
                    menu_update_input(IDM_VIEW_TAB_WIDTH, pnode->doc_ptr->tab_width > 0 ? pnode->doc_ptr->tab_width : eu_get_config()->tab_width);
                    util_set_menu_item(hwnd, IDM_TAB_CONVERT_SPACES, pnode->doc_ptr->tab_convert_spaces >= 0 ? pnode->doc_ptr->tab_convert_spaces : eu_get_config()->tab2spaces);   
                }
                else
                {
                    menu_update_input(IDM_VIEW_TAB_WIDTH, eu_get_config()->tab_width);
                    util_set_menu_item(hwnd, IDM_TAB_CONVERT_SPACES, eu_get_config()->tab2spaces); 
                }           
                if (pnode->hwnd_sc)
                {
                    SetFocus(pnode->hwnd_sc);
                }
            }
        }  
    }
    if (eu_get_config()->m_toolbar)
    {
        on_toolbar_update_button();
    }
}
