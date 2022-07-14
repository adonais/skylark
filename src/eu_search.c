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
#include <shlobj.h>

static int max_nav_count;
static HWND hwnd_found_box;
static HWND hwnd_search_dlg;
static LONG_PTR orig_combo_proc;
static LONG_PTR orig_tab_proc;
static LIST_HEAD(list_trace);
static LIST_HEAD(list_files);
static LIST_HEAD(list_folders);
static volatile long search_id = 0;
static HANDLE search_event_final = NULL;
static HWND hwnd_regxp_tips = NULL;

#define INCLUDE_FOLDER_SUB     0x00000001
#define INCLUDE_FOLDER_HIDDEN  0x00000002
#define INCLUDE_FILE_UTF8      0x00000004
#define ON_OTHER_PAGE          0x00000001
#define ON_REPLACE_THIS        0x00000002
#define ON_REPLACE_ALL         0x00000004
#define HSCRALL_LEN            768

#define DLG_BTN_CHECK(h, i)                 \
    (IsDlgButtonChecked(h, i) == BST_CHECKED)
#define CONTROL_HANDLE(hc, resi, sw)        \
hc = GetDlgItem(hwnd_search_dlg, resi);     \
if (hc)                                     \
{                                           \
    if (sw == SW_HIDE)                      \
    {                                       \
        ShowWindow(hc, sw);                 \
    }                                       \
    else                                    \
    {                                       \
        ShowWindow(hc, SW_SHOW);            \
    }                                       \
}

static void
on_search_fill_combo(void)
{
    HWND hwnd_type = GetDlgItem(hwnd_search_dlg, IDC_SEARCH_FY_CBO);
    if (hwnd_type)
    {
        const TCHAR *index_str[] =
        {
            _T("*.*"),
            _T("*.hpp"),
            _T("*.h"),
            _T("*.cpp"),
            _T("*.c"),
            NULL
        };
        for (int i = 0; index_str[i]; ++i)
        {
            int v = (int)ComboBox_FindStringExact(hwnd_type, -1, (LPARAM)index_str[i]);
            if (v == CB_ERR)
            {
                ComboBox_InsertString(hwnd_type, 0, (LPARAM)index_str[i]);
            }
        }
    }
}

static void
on_search_set_result(int res_id, int num, int count)
{
    HWND stc = GetDlgItem(hwnd_search_dlg, IDC_SEARCH_TIPS_STC);
    if (stc)
    {
        TCHAR msg[MAX_LOADSTRING] = {0};
        LOAD_I18N_RESSTR(res_id, str);
        _sntprintf(msg, MAX_LOADSTRING-1, str, num, count);
        Static_SetText(stc, msg);
    }
}

static void
on_search_set_tip(int res_id)
{
    HWND stc = GetDlgItem(hwnd_search_dlg, IDC_SEARCH_TIPS_STC);
    if (stc)
    {
        LOAD_I18N_RESSTR(res_id, str);
        Static_SetText(stc, str);
    }
}

static void
on_search_tab_ui(int index)
{
    HANDLE hc = NULL;
    RECT rc = {0};
    eu_tabpage *pnode = on_tabpage_focus_at();
    if (!pnode)
    {
        return;
    }
    if (index == 0)
    {
        CONTROL_HANDLE(hc, IDC_SEARCH_RP_STC, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_RP_CBO, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_FY_STC, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_FY_CBO, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_DIR_STC, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_DIR_CBO, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_FILES_BROWSE_BTN, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_RE_BTN, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_REALL_BTN, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_ALL_BTN, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_CD_CHK, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_SUB_CHK, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_HIDE_CHK, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_UTF8_CHK, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_FOUNDLIST, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_PGB_STC, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_PGB1, SW_HIDE)
        hc = GetDlgItem(hwnd_search_dlg, IDC_WHAT_FOLDER_CBO);
        EnableWindow(hc, true);
        hc = GetDlgItem(hwnd_search_dlg, IDC_MATCH_CASE);
        if (!pnode->hex_mode)
        {
            EnableWindow(hc, true);
            CONTROL_HANDLE(hc, IDC_MATCH_ALL_FILE, SW_SHOW)
            CONTROL_HANDLE(hc, IDC_MATCH_LOOP, SW_SHOW)
            CONTROL_HANDLE(hc, IDC_MATCH_WDSTART, SW_SHOW)
            CONTROL_HANDLE(hc, IDC_MATCH_WORD, SW_SHOW)
            CONTROL_HANDLE(hc, IDC_SEARCH_PRE_BTN, SW_SHOW)
            CONTROL_HANDLE(hc, IDC_SEARCH_NEXT_BTN, SW_SHOW)
            GetWindowRect(hc, &rc);
            MapWindowPoints(NULL, hwnd_search_dlg, (LPPOINT)&rc, 2);
            CONTROL_HANDLE(hc, IDC_SEARCH_COUNT_BTN, SW_SHOW)
            MoveWindow(hc, rc.left, rc.bottom + 3, rc.right - rc.left, rc.bottom - rc.top, TRUE);
            CONTROL_HANDLE(hc, IDC_SEARCH_CLOSE_BTN, SW_SHOW)
            MoveWindow(hc, rc.left, rc.bottom + (rc.bottom - rc.top) + 6, rc.right - rc.left, rc.bottom - rc.top, TRUE);
            CONTROL_HANDLE(hc, IDC_SEARCH_HEX_STRINGS, SW_HIDE)
            CONTROL_HANDLE(hc, IDC_SEARCH_HEX_STC, SW_HIDE)
            hc = GetDlgItem(hwnd_search_dlg, IDC_MODE_REGEXP);
            EnableWindow(hc, true);
        }
        else
        {
            if (!DLG_BTN_CHECK(hwnd_search_dlg, IDC_SEARCH_HEX_STRINGS))
            {
                EnableWindow(hc, false);
                CONTROL_HANDLE(hc, IDC_SEARCH_HEX_STC, SW_SHOW)
                LOAD_I18N_RESSTR(IDS_SEARCH_HEX_TIPS, hex_str);
                Static_SetText(hc, hex_str);
            }
            else
            {
                CONTROL_HANDLE(hc, IDC_SEARCH_HEX_STC, SW_HIDE)
            }
            CONTROL_HANDLE(hc, IDC_MATCH_ALL_FILE, SW_HIDE)
            CONTROL_HANDLE(hc, IDC_MATCH_LOOP, SW_SHOW)
            CONTROL_HANDLE(hc, IDC_MATCH_WORD, SW_HIDE)
            CONTROL_HANDLE(hc, IDC_MATCH_WDSTART, SW_HIDE)
            CONTROL_HANDLE(hc, IDC_SEARCH_PRE_BTN, SW_SHOW)
            CONTROL_HANDLE(hc, IDC_SEARCH_NEXT_BTN, SW_SHOW)
            GetWindowRect(hc, &rc);
            MapWindowPoints(NULL, hwnd_search_dlg, (LPPOINT)&rc, 2);
            CONTROL_HANDLE(hc, IDC_SEARCH_COUNT_BTN, SW_HIDE)
            hc = GetDlgItem(hwnd_search_dlg, IDC_MODE_REGEXP);
            EnableWindow(hc, false);
            CONTROL_HANDLE(hc, IDC_SEARCH_CLOSE_BTN, SW_SHOW)
            MoveWindow(hc, rc.left, rc.bottom + (rc.bottom - rc.top) + 1, rc.right - rc.left, rc.bottom - rc.top, TRUE);
            CONTROL_HANDLE(hc, IDC_SEARCH_HEX_STRINGS, SW_SHOW)
        }
        LOAD_I18N_RESSTR(IDC_MSG_SEARCH_TIT1, tit_str);
        SetWindowText(hwnd_search_dlg, tit_str);
    }
    else if (index == 1)
    {
        CONTROL_HANDLE(hc, IDC_SEARCH_RP_STC, SW_SHOW)
        CONTROL_HANDLE(hc, IDC_SEARCH_FY_STC, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_FY_CBO, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_DIR_STC, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_DIR_CBO, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_FILES_BROWSE_BTN, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_ALL_BTN, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_COUNT_BTN, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_CD_CHK, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_SUB_CHK, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_HIDE_CHK, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_UTF8_CHK, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_FOUNDLIST, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_PGB_STC, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_PGB1, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_HEX_STRINGS, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_HEX_STC, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_MATCH_ALL_FILE, SW_SHOW)
        CONTROL_HANDLE(hc, IDC_MATCH_LOOP, SW_SHOW)
        CONTROL_HANDLE(hc, IDC_MATCH_WDSTART, SW_SHOW)
        CONTROL_HANDLE(hc, IDC_MATCH_WORD, SW_SHOW)
        hc = GetDlgItem(hwnd_search_dlg, IDC_MATCH_CASE);
        EnableWindow(hc, true);
        hc = GetDlgItem(hwnd_search_dlg, IDC_MODE_REGEXP);
        EnableWindow(hc, true);
        CONTROL_HANDLE(hc, IDC_SEARCH_PRE_BTN, SW_SHOW)
        CONTROL_HANDLE(hc, IDC_SEARCH_NEXT_BTN, SW_SHOW)
        GetWindowRect(hc, &rc);
        MapWindowPoints(NULL, hwnd_search_dlg, (LPPOINT)&rc, 2);
        CONTROL_HANDLE(hc, IDC_SEARCH_RE_BTN, SW_SHOW)
        MoveWindow(hc, rc.left, rc.bottom + 4, rc.right - rc.left, rc.bottom - rc.top, TRUE);
        CONTROL_HANDLE(hc, IDC_SEARCH_REALL_BTN, SW_SHOW)
        MoveWindow(hc, rc.left, rc.bottom + (rc.bottom - rc.top) + 6, rc.right - rc.left, rc.bottom - rc.top, TRUE);
        CONTROL_HANDLE(hc, IDC_SEARCH_CLOSE_BTN, SW_SHOW)
        MoveWindow(hc, rc.left, rc.bottom + (rc.bottom - rc.top) * 2 + 8, rc.right - rc.left, rc.bottom - rc.top, TRUE);
        hc = GetDlgItem(hwnd_search_dlg, IDC_WHAT_FOLDER_CBO);
        if (pnode->hex_mode)
        {
            EnableWindow(hc, false);
        }
        else
        {
            EnableWindow(hc, true);
        }
        CONTROL_HANDLE(hc, IDC_SEARCH_RP_CBO, SW_SHOW)
        if (pnode->hex_mode)
        {
            EnableWindow(hc, false);
        }
        else
        {
            EnableWindow(hc, true);
        }
        LOAD_I18N_RESSTR(IDC_MSG_SEARCH_TIT2, tit_str);
        SetWindowText(hwnd_search_dlg, tit_str);
    }
    else if (index == 2)
    {
        CONTROL_HANDLE(hc, IDC_SEARCH_RP_STC, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_RP_CBO, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_FY_STC, SW_SHOW)
        CONTROL_HANDLE(hc, IDC_SEARCH_FY_CBO, SW_SHOW)
        CONTROL_HANDLE(hc, IDC_SEARCH_DIR_STC, SW_SHOW)
        CONTROL_HANDLE(hc, IDC_SEARCH_DIR_CBO, SW_SHOW)
        CONTROL_HANDLE(hc, IDC_FILES_BROWSE_BTN, SW_SHOW)
        CONTROL_HANDLE(hc, IDC_SEARCH_NEXT_BTN, SW_HIDE)
        GetWindowRect(hc, &rc);
        MapWindowPoints(NULL, hwnd_search_dlg, (LPPOINT)&rc, 2);
        EnableWindow(hc, true);
        hc = GetDlgItem(hwnd_search_dlg, IDC_WHAT_FOLDER_CBO);
        EnableWindow(hc, true);
        CONTROL_HANDLE(hc, IDC_MATCH_ALL_FILE, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_MATCH_LOOP, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_MATCH_WDSTART, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_PRE_BTN, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_COUNT_BTN, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_RE_BTN, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_REALL_BTN, SW_HIDE)
        hc = GetDlgItem(hwnd_search_dlg, IDC_MODE_REGEXP);
        EnableWindow(hc, true);
        hc = GetDlgItem(hwnd_search_dlg, IDC_MATCH_CASE);
        EnableWindow(hc, true);
        CONTROL_HANDLE(hc, IDC_MATCH_WORD, SW_SHOW)
        CONTROL_HANDLE(hc, IDC_SEARCH_ALL_BTN, SW_SHOW)
        MoveWindow(hc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
        CONTROL_HANDLE(hc, IDC_SEARCH_COUNT_BTN, SW_SHOW)
        MoveWindow(hc, rc.left, rc.bottom + 4, rc.right - rc.left, rc.bottom - rc.top, TRUE);
        CONTROL_HANDLE(hc, IDC_SEARCH_CD_CHK, SW_SHOW)
        MoveWindow(hc, rc.left, rc.bottom + (rc.bottom - rc.top) + 12, rc.right - rc.left, rc.bottom - rc.top, TRUE);
        CONTROL_HANDLE(hc, IDC_SEARCH_SUB_CHK, SW_SHOW)
        MoveWindow(hc, rc.left, rc.bottom + (rc.bottom - rc.top) * 2 + 15, rc.right - rc.left, rc.bottom - rc.top, TRUE);
        CONTROL_HANDLE(hc, IDC_SEARCH_HIDE_CHK, SW_SHOW)
        MoveWindow(hc, rc.left, rc.bottom + (rc.bottom - rc.top) * 3 + 18, rc.right - rc.left, rc.bottom - rc.top, TRUE);
        CONTROL_HANDLE(hc, IDC_SEARCH_UTF8_CHK, SW_SHOW)
        MoveWindow(hc, rc.left, rc.bottom + (rc.bottom - rc.top) * 4 + 21, rc.right - rc.left, rc.bottom - rc.top, TRUE);
        LOAD_I18N_RESSTR(IDC_MSG_SEARCH_TIT3, tit_str);
        SetWindowText(hwnd_search_dlg, tit_str);
        CONTROL_HANDLE(hc, IDC_SEARCH_CLOSE_BTN, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_FOUNDLIST, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_HEX_STRINGS, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_HEX_STC, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_REGXP_TIPS_STC, SW_HIDE)
        if (search_id)
        {
            CONTROL_HANDLE(hc, IDC_PGB_STC, SW_SHOW)
            CONTROL_HANDLE(hc, IDC_PGB1, SW_SHOW)
        }
        else
        {
            CONTROL_HANDLE(hc, IDC_PGB_STC, SW_HIDE)
            CONTROL_HANDLE(hc, IDC_PGB1, SW_HIDE)
        }
        on_search_set_tip(IDC_MSG_SEARCH_STR1);
        on_search_fill_combo();
    }
    if (hwnd_search_dlg)
    {
        UpdateWindow(hwnd_search_dlg);
    }
}

static void
on_search_init_option(void)
{
    HWND hwnd_loop = GetDlgItem(hwnd_search_dlg, IDC_MATCH_LOOP);
    HWND hwnd_word = GetDlgItem(hwnd_search_dlg, IDC_MATCH_WORD);
    HWND hwnd_wdst = GetDlgItem(hwnd_search_dlg, IDC_MATCH_WDSTART);
    HWND hwnd_case = GetDlgItem(hwnd_search_dlg, IDC_MATCH_CASE);
    HWND hwnd_file = GetDlgItem(hwnd_search_dlg, IDC_MATCH_ALL_FILE);
    HWND hwnd_mode = GetDlgItem(hwnd_search_dlg, IDC_MODE_NORMAL);
    HWND hwnd_rgxp = GetDlgItem(hwnd_search_dlg, IDC_MODE_REGEXP);
    Button_SetCheck(hwnd_file, BST_UNCHECKED);
    Button_SetCheck(hwnd_loop, BST_UNCHECKED);
    Button_SetCheck(hwnd_word, BST_UNCHECKED);
    Button_SetCheck(hwnd_wdst, BST_UNCHECKED);
    Button_SetCheck(hwnd_rgxp, BST_UNCHECKED);
    Button_SetCheck(hwnd_case, BST_CHECKED);
    Button_SetCheck(hwnd_mode, BST_CHECKED);
    EnableWindow(hwnd_word, true);
    EnableWindow(hwnd_wdst, true);
    WPARAM wParam = MAKELONG(IDC_SEARCH_HEX_STRINGS, 0);
    PostMessage(hwnd_search_dlg, WM_COMMAND, wParam, 0);
}

void
on_search_jmp_line(eu_tabpage *pnode, sptr_t goto_num, sptr_t current_num)
{
    if (pnode)
    {
        sptr_t line_count = eu_sci_call(pnode, SCI_GETLINECOUNT, 0, 0);
        sptr_t screen_line_count = eu_sci_call(pnode, SCI_LINESONSCREEN, 0, 0);
        if (goto_num > current_num)
        {
            sptr_t jump_lineno = goto_num + screen_line_count / 2;
            if (jump_lineno > line_count)
            {
                jump_lineno = line_count;
            }
            eu_sci_call(pnode, SCI_GOTOLINE, jump_lineno, 0);
            eu_sci_call(pnode, SCI_GOTOLINE, goto_num, 0);
        }
        else
        {
            sptr_t jump_lineno = goto_num - screen_line_count / 2;
            if (jump_lineno < 1)
            {
                jump_lineno = 1;
            }
            eu_sci_call(pnode, SCI_GOTOLINE, jump_lineno, 0);
            eu_sci_call(pnode, SCI_GOTOLINE, goto_num, 0);
        }
    }
}

void
on_search_jmp_pos(eu_tabpage *pnode, sptr_t pos)
{
    if (!pos)
    {
        eu_sci_call(pnode, SCI_GOTOPOS, 0, 0);
    }
    else if (pos > 0)
    {
        sptr_t line = eu_sci_call(pnode, SCI_LINEFROMPOSITION, pos, 0);
        on_search_jmp_line(pnode, line, 0);
        eu_sci_call(pnode, SCI_GOTOPOS, pos, 0);
    }
}

static size_t
on_search_build_flags(HWND hwnd_search)
{
    size_t flags = 0;
    if (DLG_BTN_CHECK(hwnd_search, IDC_MODE_REGEXP))
    {
        flags |= SCFIND_REGEXP;
    }
    if (DLG_BTN_CHECK(hwnd_search, IDC_MATCH_CASE))
    {
        flags |= SCFIND_MATCHCASE;
    }
    if (flags & SCFIND_REGEXP)
    {   // 如果是正则表达式, 忽略这两个标志
        if (DLG_BTN_CHECK(hwnd_search, IDC_MATCH_WORD))
        {
            flags |= SCFIND_WHOLEWORD;
        }
        if (DLG_BTN_CHECK(hwnd_search, IDC_MATCH_WDSTART))
        {
            flags |= SCFIND_WORDSTART;
        }
    }
    return flags;
}

static size_t
on_search_folder_flags(HWND hwnd_search)
{
    size_t flags = 0;
    if (DLG_BTN_CHECK(hwnd_search, IDC_SEARCH_SUB_CHK))
    {
        flags |= INCLUDE_FOLDER_SUB;
    }
    if (DLG_BTN_CHECK(hwnd_search, IDC_SEARCH_HIDE_CHK))
    {
        flags |= INCLUDE_FOLDER_HIDDEN;
    }
    if (DLG_BTN_CHECK(hwnd_search, IDC_SEARCH_UTF8_CHK))
    {
        flags |= INCLUDE_FILE_UTF8;
    }
    return flags;
}

int
on_search_combo_callback(void *data, int count, char **column, char **names)
{
    int i;
    HWND hwnd_combo = GetDlgItem(hwnd_search_dlg, (int)(uintptr_t)data);
    if (!hwnd_combo)
    {
        // if return 1, eu_sqlite3_exec error :query aborted
        return 0;
    }
    for (i = 0; i < count && column[i]; i++)
    {
        if (column[i][0] != 0)
        {
            TCHAR *ptr_row = NULL;
        #ifdef _UNICODE
            if ((ptr_row = eu_utf8_utf16(column[i], NULL)) == NULL)
            {
                return 1;
            }
        #else
            ptr_row = column[i];
        #endif
            int v = (int)ComboBox_FindStringExact(hwnd_combo, -1, ptr_row);
            if (v == CB_ERR)
            {
                ComboBox_InsertString(hwnd_combo, 0, (LPARAM)ptr_row);
            }
        #ifdef _UNICODE
            free(ptr_row);
        #endif
        }
    }
    return 0;
}

static char *
on_search_get_combo_str(int resid)
{
    HWND hwnd_cb = NULL;
    TCHAR text[MAX_PATH+1]= {0};
    hwnd_cb = GetDlgItem(hwnd_search_dlg, resid);
    ComboBox_GetText(hwnd_cb, text, MAX_PATH);
    if (_tcslen(text) < 1)
    {
        return NULL;
    }
    return eu_utf16_utf8(text, NULL);
}

static void
on_search_set_focus(eu_tabpage *pnode, HWND hwnd_what)
{
    if (pnode && pnode->hwnd_sc)
    {
        SendMessage(pnode->hwnd_sc, WM_KILLFOCUS, 0, 0);
        SetFocus(hwnd_what);
    }
}

int
on_search_find_thread(eu_tabpage *pnode)
{
    if (pnode)
    {
        HWND hwnd_what = GetDlgItem(hwnd_search_dlg, IDC_WHAT_FOLDER_CBO);
        HWND hwnd_tab = GetDlgItem(hwnd_search_dlg, IDD_SEARCH_TAB_1);
        util_push_text_dlg(pnode, hwnd_what);
        TabCtrl_SetCurSel(hwnd_tab, 0);
        on_search_tab_ui(0);
        on_search_init_option();
        util_creater_window(hwnd_search_dlg, eu_module_hwnd());
        ShowWindow(hwnd_search_dlg, SW_SHOW);
        SendMessage(hwnd_search_dlg, DM_SETDEFID, IDC_SEARCH_NEXT_BTN, 0);
        eu_get_find_history(on_search_combo_callback);
        on_search_set_focus(pnode, hwnd_what);
    }
    return 0;
}

int
on_search_find_pre(eu_tabpage *pnode)
{
    if (pnode)
    {
        HWND hwnd_what = GetDlgItem(hwnd_search_dlg, IDC_WHAT_FOLDER_CBO);
        HWND hwnd_tab = GetDlgItem(hwnd_search_dlg, IDD_SEARCH_TAB_1);
        util_push_text_dlg(pnode, hwnd_what);
        TabCtrl_SetCurSel(hwnd_tab, 0);
        on_search_tab_ui(0);
        SendMessage(hwnd_search_dlg, WM_COMMAND, IDC_SEARCH_PRE_BTN, 0);
    }
    return 0;
}

int
on_search_find_next(eu_tabpage *pnode)
{
    if (pnode)
    {
        HWND hwnd_what = GetDlgItem(hwnd_search_dlg, IDC_WHAT_FOLDER_CBO);
        HWND hwnd_tab = GetDlgItem(hwnd_search_dlg, IDD_SEARCH_TAB_1);
        util_push_text_dlg(pnode, hwnd_what);
        TabCtrl_SetCurSel(hwnd_tab, 0);
        on_search_tab_ui(0);
        SendMessage(hwnd_search_dlg, WM_COMMAND, IDC_SEARCH_NEXT_BTN, 0);
    }
    return 0;
}

int
on_search_replace_thread(eu_tabpage *pnode)
{
    if (pnode && !pnode->hex_mode)
    {
        HWND hwnd_what = GetDlgItem(hwnd_search_dlg, IDC_WHAT_FOLDER_CBO);
        HWND hwnd_tab = GetDlgItem(hwnd_search_dlg, IDD_SEARCH_TAB_1);
        if (hwnd_what && hwnd_tab)
        {
            util_push_text_dlg(pnode, hwnd_what);
            TabCtrl_SetCurSel(hwnd_tab, 1);
            on_search_tab_ui(1);
            on_search_init_option();
            util_creater_window(hwnd_search_dlg, eu_module_hwnd());
            ShowWindow(hwnd_search_dlg, SW_SHOW);
            SendMessage(hwnd_search_dlg, DM_SETDEFID, IDC_SEARCH_RE_BTN, 0);
            eu_get_find_history(on_search_combo_callback);
            eu_get_replace_history(on_search_combo_callback);
            on_search_set_focus(pnode, hwnd_what);
        }
    }
    return 0;
}

int
on_search_file_thread(const TCHAR *path)
{
    HWND hwnd_what = GetDlgItem(hwnd_search_dlg, IDC_WHAT_FOLDER_CBO);
    HWND hwnd_tab = GetDlgItem(hwnd_search_dlg, IDD_SEARCH_TAB_1);
    HWND hwnd_folder = GetDlgItem(hwnd_search_dlg, IDC_SEARCH_DIR_CBO);
    if (hwnd_what && hwnd_tab && hwnd_folder)
    {
        eu_tabpage *pnode = on_tabpage_focus_at();
        util_push_text_dlg(pnode, hwnd_what);
        TabCtrl_SetCurSel(hwnd_tab, 2);
        on_search_tab_ui(2);
        on_search_init_option();
        util_creater_window(hwnd_search_dlg, eu_module_hwnd());
        if (path)
        {
            SendMessage(hwnd_folder, WM_SETTEXT, 0, (LPARAM) path);
        }
        ShowWindow(hwnd_search_dlg, SW_SHOW);
        SendMessage(hwnd_search_dlg, DM_SETDEFID, IDC_SEARCH_ALL_BTN, 0);
        eu_get_find_history(on_search_combo_callback);
        eu_get_folder_history(on_search_combo_callback);
        on_search_set_focus(pnode, hwnd_what);
    }
    return 0;
}

void
on_search_set_selection(eu_tabpage *pnode)
{
    if (pnode)
    {
        if (pnode->begin_pos == -1)
        {
            pnode->begin_pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
        }
        else
        {
            eu_sci_call(pnode, SCI_SETANCHOR, pnode->begin_pos, 0);
            pnode->begin_pos = -1;
        }
    }
}

void
on_search_set_rectangle(eu_tabpage *pnode)
{
    if (pnode)
    {
        if (eu_sci_call(pnode, SCI_GETSELECTIONMODE, 0, 0) == SC_SEL_STREAM)
        {
            if (eu_sci_call(pnode, SCI_GETZOOM, 0, 0) == 0)
            {
                on_view_zoom_in(pnode);
                on_view_zoom_in(pnode);
                pnode->zoom_level = SELECTION_ZOOM_LEVEEL;
            }
            eu_sci_call(pnode, SCI_SETSELECTIONMODE, SC_SEL_THIN, 0);
            SendMessage(eu_module_hwnd(), WM_SYSKEYDOWN, VK_SHIFT, 1<<29);
        }
        else
        {
            SendMessage(pnode->hwnd_sc, WM_KEYDOWN, VK_ESCAPE, 0);
        }
    }
}

void
on_search_select_all(eu_tabpage *pnode)
{
    if (pnode)
    {
        eu_sci_call(pnode, SCI_SELECTALL, 0, 0);
    }
}

void
on_search_select_word(eu_tabpage *pnode)
{
    if (pnode)
    {
        sptr_t pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
        sptr_t wd_start = eu_sci_call(pnode, SCI_WORDSTARTPOSITION, pos, true);
        sptr_t wd_end = eu_sci_call(pnode, SCI_WORDENDPOSITION, pos, true);
        eu_sci_call(pnode, SCI_SETSEL, wd_start, wd_end);
    }
}

void
on_search_select_line(eu_tabpage *pnode)
{
    if (pnode)
    {
        sptr_t pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
        sptr_t current_line = eu_sci_call(pnode, SCI_LINEFROMPOSITION, pos, 0);
        sptr_t current_start = eu_sci_call(pnode, SCI_POSITIONFROMLINE, current_line, 0);
        sptr_t current_end = eu_sci_call(pnode, SCI_GETLINEENDPOSITION, current_line, 0);
        sptr_t eol_mode = eu_sci_call(pnode, SCI_GETEOLMODE, 0, 0);
        eu_sci_call(pnode, SCI_SETSEL, current_start, current_end + (eol_mode == 0 ? 2 : 1));
    }
}

void
on_search_select_left_word(eu_tabpage *pnode)
{
    if (pnode)
    {
        eu_sci_call(pnode, SCI_WORDPARTLEFTEXTEND, 0, 0);
    }
}

void
on_search_select_right_word(eu_tabpage *pnode)
{
    if (pnode)
    {
        eu_sci_call(pnode, SCI_WORDPARTRIGHTEXTEND, 0, 0);
    }
}

void
on_search_select_left_group(eu_tabpage *pnode)
{
    if (pnode)
    {
        eu_sci_call(pnode, SCI_WORDLEFTEXTEND, 0, 0);
    }
}

void
on_search_select_right_group(eu_tabpage *pnode)
{
    if (pnode)
    {
        eu_sci_call(pnode, SCI_WORDRIGHTEXTEND, 0, 0);
    }
}

void
on_search_cumulative_previous_block(eu_tabpage *pnode)
{
    if (pnode)
    {
        eu_sci_call(pnode, SCI_PARAUPEXTEND, 0, 0);
    }
}

void
on_search_cumulative_next_block(eu_tabpage *pnode)
{
    if (pnode)
    {
        eu_sci_call(pnode, SCI_PARADOWNEXTEND, 0, 0);
    }
}

void
on_search_move_to_lgroup(eu_tabpage *pnode)
{
    if (pnode)
    {
        eu_sci_call(pnode, SCI_WORDPARTLEFT, 0, 0);
    }
}

void
on_search_move_to_rgroup(eu_tabpage *pnode)
{
    if (pnode)
    {
        eu_sci_call(pnode, SCI_WORDPARTRIGHT, 0, 0);
    }
}

void
on_search_move_to_lword(eu_tabpage *pnode)
{
    if (pnode)
    {
        eu_sci_call(pnode, SCI_WORDLEFT, 0, 0);
    }
}

void
on_search_move_to_rword(eu_tabpage *pnode)
{
    if (pnode)
    {
        eu_sci_call(pnode, SCI_WORDRIGHT, 0, 0);
    }
}

void
on_search_move_to_top_block(eu_tabpage *pnode)
{
    if (pnode)
    {
        eu_sci_call(pnode, SCI_PARAUP, 0, 0);
    }
}

void
on_search_move_to_bottom_block(eu_tabpage *pnode)
{
    if (pnode)
    {
        eu_sci_call(pnode, SCI_PARADOWN, 0, 0);
    }
}

void
on_search_jmp_home(eu_tabpage *pnode)
{
    if (pnode)
    {
        if (pnode->hex_mode)
        {
            SendMessage(pnode->hwnd_sc, WM_KEYDOWN, VK_HOME, 0);
        }
        else
        {
            sptr_t pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
            sptr_t current_line = eu_sci_call(pnode, SCI_LINEFROMPOSITION, pos, 0);
            if (current_line > 0)
            {
                on_search_jmp_line(pnode, 0, current_line);
            }
        }
    }
}

void
on_search_jmp_end(eu_tabpage *pnode)
{
    if (pnode)
    {
        if (pnode->hex_mode)
        {
            SendMessage(pnode->hwnd_sc, WM_KEYDOWN, VK_END, 0);
        }
        else
        {
            sptr_t pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
            sptr_t current_line = eu_sci_call(pnode, SCI_LINEFROMPOSITION, pos, 0);
            sptr_t max_line = eu_sci_call(pnode, SCI_GETLINECOUNT, 0, 0);
            if (current_line != max_line)
            {
                on_search_jmp_line(pnode, max_line, current_line);
            }
        }
    }
}

void
on_search_jmp_specified_line(eu_tabpage *pnode)
{
    sptr_t line = 0;
    TCHAR lineno[32] = {0};
    TCHAR tip_str[MAX_LOADSTRING]  = {0};
    if (pnode)
    {
        if (pnode->hex_mode)
        {
            eu_i18n_load_str(IDC_MSG_SEARCH_STR5, tip_str, 0);
        }
        else
        {
            eu_i18n_load_str(IDC_MSG_SEARCH_ERR2, tip_str, 0);
        }
        if (eu_input(tip_str, lineno, _countof(lineno)))
        {
            if (pnode->hex_mode)
            {

                if (_stscanf(lineno, _T("%I64x"), &line) == 1)
                {
                    SendMessage(pnode->hwnd_sc, HVM_SETLINE, line, 0);
                }

            }
            else
            {
                line = _tstoi64(lineno);
                sptr_t pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
                sptr_t current_line = eu_sci_call(pnode, SCI_LINEFROMPOSITION, pos, 0);
                on_search_jmp_line(pnode, line - 1, current_line);
            }
        }
    }
}

void
on_search_toggle_mark(eu_tabpage *pnode, sptr_t lineno)
{
    sptr_t mark_num;
    sptr_t current_line;
    if (pnode)
    {
        if (lineno < 0)
        {
            sptr_t pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
            current_line = eu_sci_call(pnode, SCI_LINEFROMPOSITION, pos, 0);
        }
        else
        {
            current_line = lineno;
        }
        mark_num = eu_sci_call(pnode, SCI_MARKERGET, current_line, 0);
        if (mark_num == MARGIN_BOOKMARK_VALUE)
        {
            eu_sci_call(pnode, SCI_MARKERADD, current_line, MARGIN_BOOKMARK_VALUE);
        }
        else
        {
            eu_sci_call(pnode, SCI_MARKERDELETE, current_line, MARGIN_BOOKMARK_VALUE);
        }
    }
}

void
on_search_add_mark(eu_tabpage *pnode, sptr_t lineno)
{
    sptr_t current_line = lineno;
    if (pnode)
    {
        if (current_line < 0)
        {
            sptr_t pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
            current_line = eu_sci_call(pnode, SCI_LINEFROMPOSITION, pos, 0);
        }
        eu_sci_call(pnode, SCI_MARKERADD, current_line, MARGIN_BOOKMARK_VALUE);
    }
}

void
on_search_remove_marks_this(eu_tabpage *pnode)
{
    if (pnode)
    {
        sptr_t pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
        sptr_t current_line = eu_sci_call(pnode, SCI_LINEFROMPOSITION, pos, 0);
        eu_sci_call(pnode, SCI_MARKERDELETE, current_line, MARGIN_BOOKMARK_VALUE);
    }
}

void
on_search_remove_marks_all(eu_tabpage *pnode)
{
    eu_tabpage *p = NULL;
    for (int index = 0, count = TabCtrl_GetItemCount(g_tabpages); index < count; ++index)
    {
        TCITEM tci = {TCIF_PARAM};
        TabCtrl_GetItem(g_tabpages, index, &tci);
        p = (eu_tabpage *) (tci.lParam);
        if (p)
        {
            eu_sci_call(p, SCI_MARKERDELETEALL, MARGIN_BOOKMARK_VALUE, 0);
        }
    }
}

void
on_search_jmp_premark_this(eu_tabpage *pnode)
{
    if (pnode)
    {
        sptr_t pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
        sptr_t current_line = eu_sci_call(pnode, SCI_LINEFROMPOSITION, pos, 0);
        sptr_t find_line = eu_sci_call(pnode, SCI_MARKERPREVIOUS, current_line - 1, MARGIN_BOOKMARK_MASKN);
        if (find_line != -1)
        {
            on_search_jmp_line(pnode, find_line, current_line);
        }
    }
}

void
on_search_jmp_next_mark_this(eu_tabpage *pnode)
{
    if (pnode)
    {
        sptr_t pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
        sptr_t current_line = eu_sci_call(pnode, SCI_LINEFROMPOSITION, pos, 0);
        sptr_t find_line = eu_sci_call(pnode, SCI_MARKERNEXT, current_line + 1, MARGIN_BOOKMARK_MASKN);
        if (find_line != -1)
        {
            on_search_jmp_line(pnode, find_line, current_line);
        }
    }
}

int
on_search_jmp_premark_all(eu_tabpage *pnode)
{
    int count;
    int index;
    sptr_t pos;
    sptr_t current_line;
    sptr_t find_line;
    sptr_t max_line;
    eu_tabpage *p = NULL;
    if (!pnode)
    {
        return EUE_TAB_NULL;
    }
    count = TabCtrl_GetItemCount(g_tabpages);
    for (index = 0; index < count; index++)
    {
        TCITEM tci = {TCIF_PARAM};
        TabCtrl_GetItem(g_tabpages, index, &tci);
        p = (eu_tabpage *) (tci.lParam);
        if (p == pnode)
        {
            pos = eu_sci_call(p, SCI_GETCURRENTPOS, 0, 0);
            current_line = eu_sci_call(p, SCI_LINEFROMPOSITION, pos, 0);
            find_line = eu_sci_call(p, SCI_MARKERPREVIOUS, current_line - 1, MARGIN_BOOKMARK_MASKN);
            if (find_line != -1)
            {
                on_search_jmp_line(p, find_line, current_line);
                break;
            }
            else
            {
                for (index--; index >= 0; index--)
                {
                    memset(&tci, 0, sizeof(TCITEM));
                    tci.mask = TCIF_PARAM;
                    TabCtrl_GetItem(g_tabpages, index, &tci);
                    p = (eu_tabpage *) (tci.lParam);
                    max_line = eu_sci_call(p, SCI_GETLINECOUNT, 0, 0);
                    find_line = eu_sci_call(p, SCI_MARKERPREVIOUS, max_line - 1, MARGIN_BOOKMARK_MASKN);
                    if (find_line != -1)
                    {
                        on_tabpage_select_index(index);
                        on_search_jmp_line(p, find_line, max_line);
                        return SKYLARK_OK;
                    }
                }
                return EUE_UNKOWN_ERR;
            }
        }
    }
    return SKYLARK_OK;
}

int
on_search_jmp_next_mark_all(eu_tabpage *pnode)
{
    sptr_t pos;
    sptr_t current_line;
    sptr_t find_line;
    int index;
    eu_tabpage *p = NULL;
    if (!pnode)
    {
        return EUE_TAB_NULL;
    }
    int count = TabCtrl_GetItemCount(g_tabpages);
    for (index = 0; index < count; index++)
    {
        TCITEM tci = {TCIF_PARAM};
        TabCtrl_GetItem(g_tabpages, index, &tci);
        p = (eu_tabpage *) (tci.lParam);
        if (p == pnode)
        {
            pos = eu_sci_call(p, SCI_GETCURRENTPOS, 0, 0);
            current_line = eu_sci_call(p, SCI_LINEFROMPOSITION, pos, 0);
            find_line = eu_sci_call(p, SCI_MARKERNEXT, current_line + 1, MARGIN_BOOKMARK_MASKN);
            if (find_line != -1)
            {
                on_search_jmp_line(p, find_line, current_line);
                break;
            }
            else
            {
                for (index++; index < count; index++)
                {
                    memset(&tci, 0, sizeof(TCITEM));
                    tci.mask = TCIF_PARAM;
                    TabCtrl_GetItem(g_tabpages, index, &tci);
                    p = (eu_tabpage *) (tci.lParam);
                    find_line = eu_sci_call(p, SCI_MARKERNEXT, 0, MARGIN_BOOKMARK_MASKN);
                    if (find_line != -1)
                    {
                        on_tabpage_select_index(index);
                        on_search_jmp_line(p, find_line, 0);
                        return SKYLARK_OK;
                    }
                }
                return EUE_UNKOWN_ERR;
            }
        }
    }
    return SKYLARK_OK;
}

void
on_search_page_mark(eu_tabpage *pnode, char *szmark, int size)
{
    int offset = 0;
    sptr_t find_line = 0;
    sptr_t current_line = 0;
    sptr_t line = -1;
    if (!pnode || pnode->hex_mode)
    {
        return;
    }
    line = eu_sci_call(pnode, SCI_GETLINECOUNT, 0, 0) + 1;
    *szmark = 0;
    while (find_line != -1 && find_line < line)
    {
        find_line = eu_sci_call(pnode, SCI_MARKERNEXT, current_line, MARGIN_BOOKMARK_MASKN);
        if (find_line >= 0)
        {
            offset = eu_int_cast(strlen(szmark));
            if (offset >= size)
            {
                break;
            }
            _snprintf(szmark+offset, size-offset, "%I64d;", find_line);
            current_line = find_line + 1;
        }
    }
}

void
on_search_fold_kept(eu_tabpage *pnode, char *szfold, int size)
{
    int offset = 0;
    sptr_t header_line = 0;
    *szfold = 0;
    do
    {
        header_line = eu_sci_call(pnode, SCI_CONTRACTEDFOLDNEXT, header_line, 0);
        if (header_line != -1)
        {
            offset = eu_int_cast(strlen(szfold));
            if (offset >= size)
            {
                break;
            }
            _snprintf(szfold+offset, size-offset, "%I64d;", header_line);
            ++header_line;
        }
    } while (header_line != -1);
}

void
on_search_update_fold(eu_tabpage *pnode, char *szfold)
{
    if (pnode)
    {
        char *p = strtok(szfold, ";");
        while (p)
        {
            sptr_t line = _atoi64(p);
            uint32_t level = (uint32_t)(eu_sci_call(pnode, SCI_GETFOLDLEVEL, line, 0) & SC_FOLDLEVELNUMBERMASK);
            if (level == 0x400)
            {   // 如果是父节点, 展开一次再折叠
                eu_sci_call(pnode, SCI_FOLDLINE, line, SC_FOLDACTION_EXPAND);
            }
            eu_sci_call(pnode, SCI_FOLDLINE, line, SC_FOLDACTION_CONTRACT);
            p = strtok(NULL, ";");
        }
    }
}

void
on_search_update_mark(eu_tabpage *pnode, char *szmark)
{
    if (pnode)
    {
        char *p = strtok(szmark, ";");
        while (p)
        {
            on_search_add_mark(pnode, _atoi64(p));
            p = strtok(NULL, ";");
        }
    }
}

int
on_search_add_navigate_list(eu_tabpage *pnode, int64_t pos)
{
    struct navigate_trace *curr = NULL;
    if (!pnode)
    {
        return EUE_TAB_NULL;
    }
    if (max_nav_count + 1 > MAX_TRACE_COUNT)
    {
        struct navigate_trace *first = list_first_entry(&list_trace, struct navigate_trace, ng_node);
        list_del(&(first->ng_node));
        free(first);
        --max_nav_count;
    }
    curr = (struct navigate_trace *) calloc(1, sizeof(struct navigate_trace));
    if (curr == NULL)
    {
        printf("memory allocation failed\n");
        return EUE_OUT_OF_MEMORY;
    }
    curr->pnode = pnode;
    curr->hwnd_sc = pnode->hwnd_sc;
    curr->last_pos = pos;
    list_add_tail(&(curr->ng_node), &list_trace);
    ++max_nav_count;
    return SKYLARK_OK;
}

int
on_search_update_navigate_list(eu_tabpage *pnode, int64_t pos)
{
    struct navigate_trace *last = NULL;
    if (!pnode)
    {
        return EUE_TAB_NULL;
    }
    last = list_last_entry(&list_trace, struct navigate_trace, ng_node);
    if (last && last->pnode == pnode)
    {
        last->last_pos = pos;
    }
    return SKYLARK_OK;
}

int
on_search_back_navigate_this(void)
{
    struct navigate_trace *curr = NULL;
    struct navigate_trace *prev = NULL;

    list_for_each_entry_safe_reverse(curr, prev, &list_trace, struct navigate_trace, ng_node)
    {
        eu_tabpage *pnode = on_tabpage_focus_at();
        if (!pnode)
        {
            return EUE_TAB_NULL;
        }
        if (curr->hwnd_sc != pnode->hwnd_sc)
        {
            continue;
        }
        list_del(&(curr->ng_node));
        free(curr);
        --max_nav_count;
        if (&(prev->ng_node) != &list_trace)
        {
            if (prev->hwnd_sc == pnode->hwnd_sc)
            {
                sptr_t text_len = eu_sci_call(pnode, SCI_GETLENGTH, 0, 0);
                sptr_t go_pos = prev->last_pos > text_len - 1 ? text_len - 1 : prev->last_pos;
                eu_sci_call(pnode, SCI_GOTOPOS, go_pos, 0);
            }
        }
        return SKYLARK_OK;
    }
    return EUE_UNKOWN_ERR;
}

int
on_search_back_navigate_all(void)
{
    struct navigate_trace *curr = NULL;
    struct navigate_trace *prev = NULL;
    list_for_each_entry_safe_reverse(curr, prev, &list_trace, struct navigate_trace, ng_node)
    {
        eu_tabpage *pnode = on_tabpage_focus_at();
        if (!pnode)
        {
            return EUE_TAB_NULL;
        }
        if (curr->pnode != pnode)
        {
            on_tabpage_selection(curr->pnode, -1);
            return SKYLARK_OK;
        }
        list_del(&(curr->ng_node));
        free(curr);
        --max_nav_count;
        if (&(prev->ng_node) != &list_trace)
        {
            if (prev->pnode != pnode)
            {
                on_tabpage_selection(prev->pnode, -1);
            }
            sptr_t text_len = eu_sci_call(pnode, SCI_GETLENGTH, 0, 0);
            sptr_t go_pos = prev->last_pos > text_len - 1 ? text_len - 1 : prev->last_pos;
            eu_sci_call(pnode, SCI_GOTOPOS, go_pos, 0);
        }
        return SKYLARK_OK;
    }
    return EUE_UNKOWN_ERR;
}

void
on_search_clean_navigate_this(eu_tabpage *pnode)
{
    if (!pnode)
    {
        return;
    }
    struct navigate_trace *curr = NULL;
    struct navigate_trace *next = NULL;
    list_for_each_entry_safe(curr, next, &list_trace, struct navigate_trace, ng_node)
    {
        if (curr->hwnd_sc == pnode->hwnd_sc)
        {
            list_del(&(curr->ng_node));
            free(curr);
            --max_nav_count;
        }
    }
}

static void
on_search_tab_draw(HWND hwnd, HDC hdc, int tab)
{
    RECT rc = {0};
    int  sel_tab = TabCtrl_GetCurSel(hwnd);
    TCHAR m_text[MAX_LOADSTRING] = {0};
    TabCtrl_GetItemRect(hwnd, tab, &rc);
    switch (tab)
    {
        case 0:
            eu_i18n_load_str(IDC_MSG_SEARCH_TIT1, m_text, MAX_LOADSTRING-1);
            break;
        case 1:
            eu_i18n_load_str(IDC_MSG_SEARCH_TIT2, m_text, MAX_LOADSTRING-1);
            break;
        case 2:
            eu_i18n_load_str(IDC_MSG_SEARCH_TIT3, m_text, MAX_LOADSTRING-1);
            break;
        default:
            break;
    }
    if (sel_tab != tab)
    {
        FrameRect(hdc, &rc, GetSysColorBrush(COLOR_3DDKSHADOW));
        if (*m_text)
        {
            DrawText(hdc, m_text, (int)_tcslen(m_text), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
    }
    else
    {
        colour hh = on_dark_light_color(rgb_dark_bk_color, 1.5f);
        SetBkColor(hdc, hh);
        ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
        DrawText(hdc, m_text, (int)_tcslen(m_text), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
}

static LRESULT CALLBACK
on_search_tab_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_ERASEBKGND:
            if (!on_dark_enable())
            {
                break;
            }
            RECT rc = { 0 };
            GetClientRect(hwnd, &rc);
            FillRect((HDC)wParam, &rc, (HBRUSH)on_dark_get_brush());
            return 1;
        case WM_PAINT:
        {
            if (GetWindowLongPtr(hwnd, GWL_STYLE) & TCS_OWNERDRAWFIXED)
            {
                PAINTSTRUCT    ps;
                HDC hdc = BeginPaint(hwnd, & ps);
                HBRUSH hbr_bkgnd = (HBRUSH)on_dark_get_brush();
                // 绘制标签
                HGDIOBJ old_font = SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));
                if (old_font)
                {
                    set_btnface_color(hdc, true);
                    set_text_color(hdc, true);
                    on_search_tab_draw(hwnd, hdc, 0);
                    on_search_tab_draw(hwnd, hdc, 1);
                    on_search_tab_draw(hwnd, hdc, 2);
                    HGDIOBJ hfont = SelectObject(hdc, old_font);
                    if (hfont)
                    {
                        DeleteObject(hfont);
                    }
                }
                EndPaint(hwnd, &ps);
            }
            break;
        }
        case WM_LBUTTONUP:
        {
            int ntab = TabCtrl_GetCurSel(hwnd);
            HWND hwnd_what = GetDlgItem(hwnd_search_dlg, IDC_WHAT_FOLDER_CBO);
            on_search_tab_ui(ntab);
            switch (ntab)
            {
                case 0:
                    SendMessage(hwnd_search_dlg, DM_SETDEFID, IDC_SEARCH_NEXT_BTN, 0);
                    break;
                case 1:
                    eu_get_replace_history(on_search_combo_callback);
                    SendMessage(hwnd_search_dlg, DM_SETDEFID, IDC_SEARCH_RE_BTN, 0);
                    break;
                case 2:
                    eu_get_folder_history(on_search_combo_callback);
                    SendMessage(hwnd_search_dlg, DM_SETDEFID, IDC_SEARCH_ALL_BTN, 0);
                    break;
                default:
                    break;
            }
            if (hwnd_what)
            {
                on_search_set_focus(on_tabpage_focus_at(), hwnd_what);
            }
            break;
        }
        case WM_THEMECHANGED:
        {
            printf("search tabs WM_THEMECHANGED\n");
            uintptr_t style = GetWindowLongPtr(hwnd, GWL_STYLE);
            if (on_dark_enable())
            {
                SetWindowLongPtr(hwnd, GWL_STYLE, style | TCS_OWNERDRAWFIXED);
            }
            else
            {
                SetWindowLongPtr(hwnd, GWL_STYLE, style & ~TCS_OWNERDRAWFIXED);
            }
            break;
        }
        case WM_DESTROY:
        {
            printf("search tabs WM_DESTROY\n");
            break;
        }
        default:
            break;
    }
    return CallWindowProc((WNDPROC)orig_tab_proc, hwnd, message, wParam, lParam);
}

//功能:向tab中添加标签
static bool
on_search_add_page(HWND htab, LPTSTR str_lable, int iid)
{

    TCITEM ti = {TCIF_TEXT};
    ti.pszText = str_lable;
    ti.cchTextMax = (int)_tcslen(str_lable);
    return (bool) SendMessage(htab, TCM_INSERTITEM, iid, (LPARAM) &ti);
}

static bool
on_search_init_pages(HWND htab)
{
    LOAD_I18N_RESSTR(IDC_MSG_SEARCH_TIT1, tit_str1);
    LOAD_I18N_RESSTR(IDC_MSG_SEARCH_TIT2, tit_str2);
    LOAD_I18N_RESSTR(IDC_MSG_SEARCH_TIT3, tit_str3);
    on_search_add_page(htab, tit_str1, 0);
    on_search_add_page(htab, tit_str2, 1);
    on_search_add_page(htab, tit_str3, 2);
    orig_tab_proc = SetWindowLongPtr(htab, GWLP_WNDPROC, (LONG_PTR) on_search_tab_proc);
    return orig_tab_proc != 0;
}

static wchar_t *
on_search_regxp_msg(void)
{
    eu_tabpage *pnode = on_tabpage_focus_at();
    if (pnode)
    {
        char sz_msg[512] = {0};
        wchar_t *ll_msg = NULL;
        eu_sci_call(pnode, SCI_GETBOOSTREGEXERRMSG, _countof(sz_msg), (sptr_t)sz_msg);
        if ((ll_msg = eu_utf8_utf16(sz_msg, NULL)) != NULL)
        {
            return ll_msg;
        }
    }
    return NULL;
}

static void
on_search_regxp_error(void)
{
    HWND stc = GetDlgItem(hwnd_search_dlg, IDC_SEARCH_TIPS_STC);
    if (stc)
    {
        HDC dc = GetDC(stc);
        HWND hwnd_re_stc = GetDlgItem(hwnd_search_dlg, IDC_REGXP_TIPS_STC);
        LOAD_I18N_RESSTR(IDS_RE_ERROR, str);
        Static_SetText(stc, str);
        if (hwnd_re_stc && dc)
        {
            RECT rc;
            SIZE sz;
            wchar_t *ll_msg = NULL;
            HFONT hfont_stc = (HFONT)SendMessage(hwnd_search_dlg, WM_GETFONT, 0,0);
            HFONT old = SelectObject(dc, hfont_stc);
            GetTextExtentPoint32(dc, str, eu_int_cast(wcslen(str)), &sz);
            SelectObject(dc, old);
            ReleaseDC(stc, dc);
            GetWindowRect(stc, &rc);
            MapWindowPoints(NULL, hwnd_search_dlg, (LPPOINT)&rc, 2);
            MoveWindow(hwnd_re_stc, rc.left + sz.cx + 8, rc.top, 12, rc.bottom - rc.top, TRUE);
            ShowWindow(hwnd_re_stc, SW_HIDE);
            ShowWindow(hwnd_re_stc, SW_SHOW);
            if (!hwnd_regxp_tips)
            {
                if ((ll_msg = on_search_regxp_msg()) != NULL)
                {
                    hwnd_regxp_tips = util_create_tips(hwnd_re_stc, hwnd_search_dlg, ll_msg);
                }
                if (hwnd_regxp_tips && on_dark_enable())
                {
                    on_dark_set_theme(hwnd_regxp_tips, L"DarkMode_Explorer", NULL);
                }
            }
            else if ((ll_msg = on_search_regxp_msg()) != NULL)
            {
                TOOLINFO toolinfo = {0};
                toolinfo.cbSize = sizeof(TOOLINFO);
                toolinfo.hwnd = hwnd_search_dlg;
                toolinfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
                toolinfo.uId = (LONG_PTR)hwnd_re_stc;
                toolinfo.lpszText = ll_msg;
                SendMessage(hwnd_regxp_tips, TTM_UPDATETIPTEXT, 0, (LPARAM)&toolinfo);
            }
            eu_safe_free(ll_msg);
        }
    }
}

static void
on_search_delete_lword(HWND hedit)
{
    WORD cursor;
    WORD wordstart;
    TCHAR str[MAX_PATH + 1];
    SendMessage(hedit, WM_GETTEXT, MAX_PATH, (LPARAM)str);
    SendMessage(hedit, EM_GETSEL, (WPARAM)&cursor, 0);
    wordstart = cursor;
    while (wordstart > 0)
    {
        TCHAR c = str[wordstart - 1];
        if (c != ' ' && c != '\t')
        {
            break;
        }
        --wordstart;
    }
    while (wordstart > 0)
    {
        TCHAR c = str[wordstart - 1];
        if (c == ' ' || c == '\t')
        {
            break;
        }
        --wordstart;
    }
    if (wordstart < cursor)
    {
        SendMessage(hedit, EM_SETSEL, (WPARAM)wordstart, (LPARAM)cursor);
        SendMessage(hedit, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)(_T("")));
    }
}

static char *
on_search_get_combo_list(HWND hcombo, int index)
{
    TCHAR buf[MAX_PATH+1] = {0};
    ComboBox_GetLBText(hcombo, index, buf);
    if (*buf)
    {
        return eu_utf16_utf8(buf, NULL);
    }
    return NULL;
}

static LRESULT FAR PASCAL
on_search_combo_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // ASCII for "Ctrl+Backspace"
    if (message == WM_CHAR && wParam == 0x7F)
    {
        int i = CB_ERR;
        on_search_delete_lword(hwnd);
        int res_id = (int)(LONG_PTR)GetWindowLongPtr(hwnd_search_dlg, GWLP_USERDATA);
        HWND hwnd_txt = GetDlgItem(hwnd_search_dlg, res_id);
        if ((i = ComboBox_GetCurSel(hwnd_txt)) != CB_ERR)
        {
            char *key = on_search_get_combo_list(hwnd_txt, i);
            if (key)
            {
                if (res_id == IDC_WHAT_FOLDER_CBO)
                {
                    eu_delete_find_history(key);
                }
                else if (res_id == IDC_SEARCH_RP_CBO)
                {
                    eu_delete_replace_history(key);
                }
                else if (res_id == IDC_SEARCH_DIR_CBO)
                {
                    eu_delete_folder_history(key);
                }
                free(key);
            }
            ComboBox_DeleteString(hwnd_txt, i);
        }
        return 0;
    }
    return CallWindowProc((WNDPROC)orig_combo_proc, hwnd, message, wParam, lParam);
}

static LRESULT WINAPI
on_search_combo_wnd(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubClass, DWORD_PTR dwRefData)
{
    switch(msg)
    {
        case WM_PAINT:
        {
            uintptr_t style = GetWindowLongPtr(hwnd, GWL_STYLE);
            if(!(style & (CBS_DROPDOWN | WS_VISIBLE)))
            {
                break;
            }
            RECT rc;
            GetClientRect(hwnd, &rc);
            PAINTSTRUCT ps;
            const HDC hdc = BeginPaint(hwnd, &ps);
            HBRUSH brush = CreateSolidBrush(on_dark_enable() ? rgb_dark_btn_color : GetSysColor(COLOR_BTNFACE));
            HPEN pen = CreatePen(PS_SOLID, 1, RGB(128, 128, 128));
            HGDIOBJ oldbrush = SelectObject(hdc, brush);
            HGDIOBJ oldpen = SelectObject(hdc, pen);
            SelectObject(hdc, (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0));
            set_bk_color(hdc, on_dark_enable());
            set_text_color(hdc, on_dark_enable());
            Rectangle(hdc, 0, 0, rc.right, rc.bottom);
            RECT rcf = rc;
            rcf.left = rc.right - 24;  // rc.right - 24 exclude DROPDOWN_BUTTUON
            TCHAR text[] = {0x2B9F, 0};
            DrawText(hdc, text, (int)_tcslen(text), &rcf, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
            HGDIOBJ newbrush = SelectObject(hdc, oldpen);
            HGDIOBJ newpen = SelectObject(hdc, oldbrush);
            DeleteObject(brush);
            DeleteObject(pen);
            if (newbrush)
            {
                DeleteObject(newbrush);
            }
            if (newpen)
            {
                DeleteObject(newpen);
            }
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_NCDESTROY:
        {
            RemoveWindowSubclass(hwnd, on_search_combo_wnd, uIdSubClass);
            break;
        }
        default:
        {
            break;
        }
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

sptr_t
on_search_process_find(eu_tabpage *pnode, const char *key, size_t from_pos, size_t to_pos, size_t flags)
{
    eu_sci_call(pnode, SCI_SETSEARCHFLAGS, flags, 0);
    eu_sci_call(pnode, SCI_SETTARGETRANGE, from_pos, to_pos);
    size_t len = strlen(key);
    return eu_sci_call(pnode, SCI_SEARCHINTARGET, len, (sptr_t)key);
}

static sptr_t
on_search_process_find2(eu_tabpage *pnode, const char *key)
{
    eu_tabpage *p = pnode;
    size_t flags = on_search_build_flags(hwnd_search_dlg);
    size_t len = strlen(key);
    if (!(p || (p = on_tabpage_focus_at())))
    {
        return 0;
    }
    eu_sci_call(p, SCI_SETSEARCHFLAGS, flags, 0);
    eu_sci_call(p, SCI_TARGETWHOLEDOCUMENT, 0, 0);
    sptr_t pos = eu_sci_call(p, SCI_SEARCHINTARGET, len, (sptr_t)key);
    if (pos > 0)
    {
        eu_sci_call(p, SCI_SETSEL, pos, eu_sci_call(p, SCI_GETTARGETEND, 0, 0));
    }
    return pos;
}

static void
on_search_push_string_listbox(const TCHAR *path, const char *key, int num)
{
    TCHAR *u16 = NULL;
    TCHAR msg_str[MAX_PATH*2+1] = {0};
    int index = ListBox_FindString(hwnd_found_box, -1, path);
    if ((u16 = eu_utf8_utf16(key, NULL)))
    {
        LOAD_I18N_RESSTR(IDC_MSG_SEARCH_STR2, format);
        _sntprintf(msg_str, MAX_PATH*2, format, path, u16, num);
        if (index >= 0)
        {
            ListBox_DeleteString(hwnd_found_box, index);
        }
        ListBox_AddString(hwnd_found_box, msg_str);
        free(u16);
    }
}

static int
on_search_process_count(eu_tabpage *pnode, const char *key)
{
    sptr_t pos = 0;
    size_t flags = on_search_build_flags(hwnd_search_dlg);
    size_t len = strlen(key);
    eu_sci_call(pnode, SCI_SETSEARCHFLAGS, flags, 0);
    eu_sci_call(pnode, SCI_TARGETWHOLEDOCUMENT, 0, 0);
    pnode->match_count = 0;
    // 清除行存储空间
    if (cvector_size(pnode->pvec) > 0)
    {
        cvector_free(pnode->pvec);
        pnode->pvec = NULL;
    }
    while (pos >= 0)
    {
        result_vec ret = {-1,};
        pos = eu_sci_call(pnode, SCI_SEARCHINTARGET, len, (sptr_t)key);
        if (pos >= 0)
        {
            sptr_t start_pos = eu_sci_call(pnode, SCI_GETTARGETEND, 0, 0);
            sptr_t end_pos = eu_sci_call(pnode, SCI_GETTEXTLENGTH, 0, 0);
            sptr_t line = eu_sci_call(pnode, SCI_LINEFROMPOSITION, pos, 0);
            if (line >= 0)
            {
                ret.line = line;
            }
            ++pnode->match_count;
            if (end_pos >= start_pos)
            {
                eu_sci_call(pnode, SCI_SETTARGETRANGE, start_pos, end_pos);
                ret.mark.start = pos;
                ret.mark.end = start_pos;
                if (ret.line >= 0)
                {
                    cvector_push_back(pnode->pvec, ret);
                }
            }
            else
            {
                break;
            }
        }
        else if (pos == -2)
        {
            pnode->match_count = -2;
        }
    }
    return pnode->match_count;
}

static int
on_search_report_result(int err, int resid)
{
    int tabcount = 1;
    int file_count = 1;
    int match_count = 0;
    bool all_file = false;
    char *key = NULL;
    HWND hwnd_re_stc = NULL;
    eu_tabpage *pnode = NULL;
    if (!(pnode = on_tabpage_focus_at()))
    {
        return 0;
    }
    if (pnode->hex_mode)
    {
        return 0;
    }
    if (!(key = on_search_get_combo_str(IDC_WHAT_FOLDER_CBO)))
    {
        return 0;
    }
    if ((hwnd_re_stc = GetDlgItem(hwnd_search_dlg, IDC_REGXP_TIPS_STC)) != NULL)
    {
        ShowWindow(hwnd_re_stc, SW_HIDE);
    }
    if ((all_file = DLG_BTN_CHECK(hwnd_search_dlg, IDC_MATCH_ALL_FILE)))
    {   // 如果在多个打开的文件中搜索, 重新计数
        file_count = 0;
        tabcount = TabCtrl_GetItemCount(g_tabpages);
        for (int index = 0; index < tabcount; ++index)
        {
            int result = 0;
            eu_tabpage *p = NULL;
            TCITEM tci = {TCIF_PARAM,};
            TabCtrl_GetItem(g_tabpages, index, &tci);
            if ((p = (eu_tabpage *) (tci.lParam)) && on_search_process_count(p, key) > 0)
            {
                if (resid == IDC_SEARCH_PRE_BTN || resid == IDC_SEARCH_NEXT_BTN)
                {
                    on_search_push_string_listbox(p->pathfile, key, p->match_count);
                }
                match_count += p->match_count;
                ++file_count;
            }
        }
    }
    else
    {
        match_count = on_search_process_count(pnode, key);
        if (match_count > 0 && (resid == IDC_SEARCH_PRE_BTN || resid == IDC_SEARCH_NEXT_BTN))
        {
            on_search_push_string_listbox(pnode->pathfile, key, match_count);
        }
    }
    if (pnode->match_count == -2)
    {
        on_search_regxp_error();
    }
    else
    {
        switch (resid)
        {
            case IDC_SEARCH_PRE_BTN:
            {
                if (err)
                {
                    if (match_count > 0)
                    {
                        on_search_set_result(IDC_MSG_SEARCH_ERR6, tabcount, match_count);
                    }
                    else
                    {
                        on_search_set_result(IDC_MSG_SEARCH_ERR8, tabcount, match_count);
                    }
                }
                else
                {
                    on_search_set_result(IDC_MSG_SEARCH_ERR7, file_count, match_count);
                }
                break;
            }
            case IDC_SEARCH_NEXT_BTN:
            {
                if (err)
                {
                    if (match_count > 0)
                    {
                        on_search_set_result(IDC_MSG_SEARCH_ERR5, tabcount, match_count);
                    }
                    else
                    {
                        on_search_set_result(IDC_MSG_SEARCH_ERR8, tabcount, match_count);
                    }
                }
                else
                {
                    on_search_set_result(IDC_MSG_SEARCH_ERR7, file_count, match_count);
                }
                break;
            }
            case IDC_SEARCH_RE_BTN:
            {
                if (err)
                {
                    if (match_count > 0)
                    {
                        on_search_set_result(IDC_MSG_SEARCH_ERR9, 1, 0);
                    }
                }
                else if (match_count > 0)
                {
                    on_search_set_result(IDC_MSG_SEARCH_ERR12, file_count, 1);
                }
                else
                {
                    on_search_set_result(IDC_MSG_SEARCH_ERR11, tabcount, match_count);
                }
                break;
            }
            case IDC_SEARCH_REALL_BTN:
            {
                if (match_count > 0)
                {
                    on_search_set_result(IDC_MSG_SEARCH_ERR12, file_count, match_count);
                }
                else
                {
                    on_search_set_result(IDC_MSG_SEARCH_ERR11, tabcount, match_count);
                }
                break;
            }
            default:
                break;
        }
    }
    free(key);
    return match_count;
}

/*************************************************************************************************
 * 功能,  在页面上搜索                                                                           *
 * pnode, 标签句柄                                                                               *
 * key, 关键字                                                                                   *
 * reverse 是否反向搜索                                                                          *
 * this_page 是否当前页面                                                                        *
 *************************************************************************************************/
static bool
on_search_at_page(eu_tabpage *pnode, const char *key, bool reverse, bool this_page)
{
    sptr_t start_pos = 0;
    sptr_t end_pos = 0;
    sptr_t found_pos = 0;
    sptr_t target_end = -1;
    bool m_loop = false;
    sptr_t pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
    sptr_t max_pos = eu_sci_call(pnode, SCI_GETTEXTLENGTH, 0, 0);
    size_t find_flags = on_search_build_flags(hwnd_search_dlg);
    if (this_page)
    {
        m_loop = DLG_BTN_CHECK(hwnd_search_dlg, IDC_MATCH_ALL_FILE) ? false : DLG_BTN_CHECK(hwnd_search_dlg, IDC_MATCH_LOOP);
    }
    if (reverse)
    {
        start_pos = this_page ? pos - 1 : max_pos;
    }
    else
    {
        start_pos = this_page ? pos : 0;
        end_pos = max_pos;
        if (start_pos == eu_sci_call(pnode, SCI_GETSELECTIONSTART, 0, 0))
        {
            start_pos = eu_sci_call(pnode, SCI_GETSELECTIONEND, 0, 0);
        }
    }
    DO_SEARCH_LOOP:
    {
        found_pos = on_search_process_find(pnode, key, start_pos, end_pos, find_flags);
        printf("start_pos = %I64d, end_pos = %I64d, found_pos = %I64d\n", start_pos, end_pos, found_pos);
        if (found_pos >= 0)
        {
            target_end = eu_sci_call(pnode, SCI_GETTARGETEND, 0, 0);
            eu_sci_call(pnode, SCI_SETSEL, found_pos, target_end);
        }
        else if (found_pos == -2)
        {
            printf("re error\n");
            return false;
        }
        else if (m_loop)
        {
            m_loop = false;
            if (reverse)
            {
                start_pos = max_pos;
                end_pos = pos;
            }
            else
            {
                start_pos = 0;
                end_pos = pos;
            }
            goto DO_SEARCH_LOOP;
        }
    }
    eu_push_find_history(key);
    return (found_pos>=0);
}

static int
on_search_hexview_string(eu_tabpage *pnode, const char *pattern, bool reverse)
{
    int match = 0;
    BYTERANGE lpos = {0};
    bool m_loop = false;
    bool m_case = false;
    sptr_t pos = -1;
    sptr_t found_pos = -1;
    size_t sub_len = strlen(pattern);
    m_loop = DLG_BTN_CHECK(hwnd_search_dlg, IDC_MATCH_LOOP);
    m_case = DLG_BTN_CHECK(hwnd_search_dlg, IDC_MATCH_CASE);
    eu_sci_call(pnode, HVM_GETSEL, 0,(sptr_t) &lpos);
    pos = reverse ? lpos.min : lpos.max;
    if (!pos)
    {
        pos = reverse ? pnode->phex->total_items : eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
    }
    if (pos < 0)
    {
        return 0;
    }
DO_STR_LOOP:
    {
        if (reverse)
        {
            match = eu_sunday(pnode->phex->pbase, (const uint8_t *)pattern, pos, sub_len, !m_case, false, reverse, &found_pos);
        }
        else
        {
            match = eu_sunday(&pnode->phex->pbase[pos], (const uint8_t *)pattern, pnode->phex->total_items - pos, sub_len, !m_case, false, reverse, &found_pos);
        }
        if (found_pos >= 0)
        {
            lpos.min = reverse ? found_pos : pos + found_pos;
            lpos.max = lpos.min + sub_len;
            eu_sci_call(pnode, HVM_SETSEL, 0, (sptr_t)&lpos);
        }
        else if (m_loop)
        {
            m_loop = false;
            pos = reverse ? pnode->phex->total_items : 0;
            goto DO_STR_LOOP;
        }
    }
    return match;
}

static int
on_search_hexview(eu_tabpage *pnode, const char *pattern, bool reverse)
{
    int match = 0;
    BYTERANGE lpos = {0};
    bool m_loop = false;
    bool m_str = false;
    sptr_t pos = -1;
    sptr_t found_pos = -1;
    size_t len = strlen(pattern);
    char *pmark = NULL;
    m_str = DLG_BTN_CHECK(hwnd_search_dlg, IDC_SEARCH_HEX_STRINGS);
    if (m_str)
    {
        match = on_search_hexview_string(pnode, pattern, reverse);
    }
    else
    {
        if (!(pmark = (char *) calloc(1, len)))
        {
            return EUE_OUT_OF_MEMORY;
        }
        // 去除所有空格
        for (int c = 0, k = 0; c < len; ++c)
        {
            if (pattern[c] != 0x20)
            {
                pmark[k++] = pattern[c];
            }
        }
        len = strlen(pmark);
        // 十六进制数据长度不能为单数
        if (len % 2 != 0)
        {
            on_search_set_tip(IDC_MSG_SEARCH_ERR15);
            free(pmark);
            return 0;
        }
        m_loop = DLG_BTN_CHECK(hwnd_search_dlg, IDC_MATCH_LOOP);
        eu_sci_call(pnode, HVM_GETSEL, 0,(sptr_t) &lpos);
        pos = reverse ? lpos.min : lpos.max;
        if (!pos)
        {
            pos = reverse ? pnode->phex->total_items : eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
        }
        if (pos < 0)
        {
            free(pmark);
            return 0;
        }
    DO_HEX_LOOP:
        {
            if (reverse)
            {
                match = eu_sunday_hex(pnode->phex->pbase, pmark, pos, reverse, &found_pos);
            }
            else
            {
                match = eu_sunday_hex(&pnode->phex->pbase[pos], pmark, pnode->phex->total_items - pos, reverse, &found_pos);
            }
            if (found_pos >= 0)
            {
                lpos.min = reverse ? found_pos : pos + found_pos;
                lpos.max = lpos.min + strlen(pmark)/2;
                eu_sci_call(pnode, HVM_SETSEL, 0, (sptr_t)&lpos);
            }
            else if (m_loop)
            {
                m_loop = false;
                pos = reverse ? pnode->phex->total_items : 0;
                goto DO_HEX_LOOP;
            }
        }
    }
    if (pmark)
    {
        free(pmark);
    }
    // report results
    if (match > 0)
    {
        eu_push_find_history(pattern);
        eu_safe_free(pattern);
    }
    on_search_set_result(IDC_MSG_SEARCH_ERR14, 1, match);
    return match;
}

static int
on_search_find_button(bool reverse)
{
    bool result = false;
    char *dlg_text = NULL;
    dlg_text = on_search_get_combo_str(IDC_WHAT_FOLDER_CBO);
    eu_tabpage *pnode = on_tabpage_focus_at();
    if (!(dlg_text && *dlg_text && pnode))
    {
        return 1;
    }
    if (pnode->hex_mode)
    {
        return on_search_hexview(pnode, dlg_text, reverse);
    }
    if (!DLG_BTN_CHECK(hwnd_search_dlg, IDC_MATCH_ALL_FILE))
    {
        result = on_search_at_page(pnode, dlg_text, reverse, true);
    }
    else
    {   // 首先在当前页面查找
        result = on_search_at_page(pnode, dlg_text, reverse, true);
        if (!result)
        {
            int index = 0;
            int count = TabCtrl_GetItemCount(g_tabpages);
            int c_index = TabCtrl_GetCurSel(g_tabpages);
            if (c_index + 1 < count)
            {
                index = c_index + 1;
            }
            for (; index < count; ++index)
            {
                eu_tabpage *p = NULL;
                TCITEM tci = {TCIF_PARAM,};
                TabCtrl_GetItem(g_tabpages, index, &tci);
                p = (eu_tabpage *) (tci.lParam);
                if (!p)
                {
                    break;
                }
                if (p == pnode)
                {
                    break;
                }
                if ((result = on_search_at_page(p, dlg_text, reverse, false)))
                {
                    on_tabpage_select_index(index);
                    break;
                }
                else if (index == count - 1)
                {
                    index = -1;
                }
            }
        }
    }
    free(dlg_text);
    return (result ? 0 : 1);
}

static void
on_search_find_next_button(void)
{
    int err = on_search_find_button(false);
    on_search_report_result(err, IDC_SEARCH_NEXT_BTN);
}

static void
on_search_find_pre_button(void)
{
    int err = on_search_find_button(true);
    on_search_report_result(err, IDC_SEARCH_PRE_BTN);
}

static int
on_search_detail_button(void)
{
    int index = -1;
    HWND hc = GetDlgItem(hwnd_search_dlg, IDC_SEARCH_FOUNDLIST);
    if (hc)
    {
        HWND hsub = NULL;
        HWND hwnd_tab = GetDlgItem(hwnd_search_dlg, IDD_SEARCH_TAB_1);
        index = TabCtrl_GetCurSel(hwnd_tab);
        if ((GetWindowLongPtr(hc, GWL_STYLE) & WS_VISIBLE))
        {
            if (index == 2)
            {
                CONTROL_HANDLE(hsub, IDC_SEARCH_SUB_CHK, SW_SHOW)
                CONTROL_HANDLE(hsub, IDC_SEARCH_HIDE_CHK, SW_SHOW)
                CONTROL_HANDLE(hsub, IDC_SEARCH_UTF8_CHK, SW_SHOW)
                if (search_id)
                {
                    CONTROL_HANDLE(hsub, IDC_PGB_STC, SW_SHOW)
                    CONTROL_HANDLE(hsub, IDC_PGB1, SW_SHOW)
                }
            }
            ShowWindow(hc, SW_HIDE);
        }
        else
        {
            if (index == 2)
            {
                CONTROL_HANDLE(hsub, IDC_SEARCH_SUB_CHK, SW_HIDE)
                CONTROL_HANDLE(hsub, IDC_SEARCH_HIDE_CHK, SW_HIDE)
                CONTROL_HANDLE(hsub, IDC_SEARCH_UTF8_CHK, SW_HIDE)
                CONTROL_HANDLE(hsub, IDC_PGB_STC, SW_HIDE)
                CONTROL_HANDLE(hsub, IDC_PGB1, SW_HIDE)
            }
            ShowWindow(hc, SW_SHOW);
        }
    }
    return 0;
}

static int
on_search_active_tab(const TCHAR *path, const TCHAR *key)
{
    int tab_find = EUE_TAB_NULL;
    if (!(path && key))
    {
        return EUE_POINT_NULL;
    }
    for (int index = 0, count = TabCtrl_GetItemCount(g_tabpages); index < count; ++index)
    {
        eu_tabpage *p = on_tabpage_get_ptr(index);
        if (p && _tcscmp(p->pathfile, path) == 0)
        {
            char *u8_key = eu_utf16_utf8(key, NULL);
            on_tabpage_select_index(index);
            on_search_process_find2(p, u8_key);
            free(u8_key);
            tab_find = index;
            break;
        }
    }
    if (tab_find < 0)
    {
        file_backup bak = {0};
        _tcscpy(bak.rel_path, path);
        tab_find = on_file_only_open(&bak, true);
        if (tab_find > SKYLARK_OPENED)
        {
            char *u8_key = eu_utf16_utf8(key, NULL);
            on_search_process_count(on_tabpage_get_ptr(tab_find), u8_key);
            free(u8_key);
        }
    }
    return tab_find;
}

static void
on_search_push_result(eu_tabpage *pnode, LPCTSTR key, LPCTSTR path)
{
    if (pnode && pnode->presult)
    {
        char *ptr_add = NULL;
        eu_tabpage *presult = pnode->presult;
        TCHAR msg[MAX_BUFFER] = {0};
        LOAD_I18N_RESSTR(IDS_RESULT_STRINGS1, path_str);
        LOAD_I18N_RESSTR(IDS_RESULT_STRINGS2, line_str);
        _sntprintf(msg, MAX_BUFFER-1, path_str, key, path, pnode->match_count);
        char *path = eu_utf16_utf8(msg, NULL);
        if (path)
        {
            eu_sci_call(presult, SCI_SETREADONLY, 0, 0);
            eu_sci_call(presult, SCI_ADDTEXT, strlen(path), (LPARAM)path);
            free(path);
        }
        char *pformat = eu_utf16_utf8(line_str, NULL);
        if (pnode->pvec && pformat)
        {
            const int k = util_count_number(pnode->pvec[cvector_size(pnode->pvec) - 1].line);
            for (int i = 0; i < cvector_size(pnode->pvec); ++i)
            {
                char *buf = util_strdup_line(pnode, pnode->pvec[i].line, NULL);
                if (buf)
                {
                    const size_t m_size = strlen(buf) + 24;
                    if ((ptr_add = (char *)calloc(1, m_size)) != NULL)
                    {
                        int len = snprintf(ptr_add, m_size - 1, pformat, k, pnode->pvec[i].line + 1, buf);
                        eu_sci_call(presult, SCI_ADDTEXT, len, (LPARAM)ptr_add);
                        free(ptr_add);
                    }
                    free(buf);
                }
            }
            // 编辑区设置成只读
            eu_sci_call(presult, SCI_SETREADONLY, 1, 0);
            eu_sci_call(presult, SCI_GOTOLINE, 1, 0);
        }
    }
}

static void
on_search_found_list(HWND hwnd)
{
    int listno = (int)SendMessage(hwnd, LB_GETCURSEL ,0 , 0);
    int buf_len = ListBox_GetTextLen(hwnd, listno) + 1;
    TCHAR *buf = (TCHAR *)calloc(sizeof(TCHAR), buf_len);
    if (!buf)
    {
        return;
    }
    ListBox_GetText(hwnd, listno, buf);
    if (*buf)
    {
        TCHAR path[MAX_PATH+1] = {0};
        TCHAR key[MAX_PATH+1] = {0};
        if (_stscanf(buf, _T("%260[^|]"), path) == 1 &&
            _stscanf(buf, _T("%*[^|]|%260[^|]"), key) == 1)
        {
            int tab = on_search_active_tab(path, key);
            eu_tabpage *pnode = on_tabpage_get_ptr(tab);
            if (pnode && on_result_launch(pnode) && pnode->presult)
            {
                pnode->result_show = true;
                eu_sci_call(pnode->presult, SCI_SETREADONLY, 0, 0);
                eu_sci_call(pnode->presult, SCI_CLEARALL, 0, 0);
                eu_window_resize(NULL);
                char *u8_key = eu_utf16_utf8(key, NULL);
                if (u8_key)
                {
                    eu_sci_call(pnode->presult, SCI_SETKEYWORDS, 0, (sptr_t)u8_key);
                    free(u8_key);
                }
                on_search_push_result(pnode, key, path);
                // 窗口并排可能导致主编辑器之前的光标位置被遮挡
                // 滚动视图以使光标可见
                eu_sci_call(pnode, SCI_SCROLLCARET, 0, 0);
                ShowWindow(hwnd_search_dlg, SW_HIDE);
            }
        }
    }
    free(buf);
}

static bool
on_search_replace_target(eu_tabpage *pnode, const char *replace_str)
{
    sptr_t re_len = 0;
    eu_sci_call(pnode, SCI_TARGETFROMSELECTION, 0, 0);
    if (eu_sci_call(pnode, SCI_GETSEARCHFLAGS, 0, 0) & SCFIND_REGEXP)
    {
        printf("replace_str = %s\n", replace_str);
        re_len = eu_sci_call(pnode, SCI_REPLACETARGETRE, (WPARAM)-1, (LPARAM)replace_str);
    }
    else
    {
        re_len = eu_sci_call(pnode, SCI_REPLACETARGET, (WPARAM)-1, (LPARAM)replace_str);
    }
    if (re_len >= 0)
    {
        sptr_t target_end = eu_sci_call(pnode, SCI_GETTARGETEND, 0, 0);
        eu_sci_call(pnode, SCI_GOTOPOS, target_end, 0);
    }
    return (re_len >= 0);
}

static bool
on_search_common(eu_tabpage *pnode, const char *key, int opt)
{
    sptr_t start_pos = 0;
    sptr_t found_pos = 0;
    bool m_loop = false;
    sptr_t pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
    sptr_t end_pos = eu_sci_call(pnode, SCI_GETLENGTH, 0, 0);
    size_t find_flags = on_search_build_flags(hwnd_search_dlg);
    m_loop = DLG_BTN_CHECK(hwnd_search_dlg, IDC_MATCH_LOOP);
    if ((opt & ON_REPLACE_ALL)  || (opt & ON_OTHER_PAGE))
    {
        m_loop = false;
        start_pos = 0;
    }
    else
    {
        start_pos = pos;
        sptr_t sel_start = eu_sci_call(pnode, SCI_GETSELECTIONSTART, 0, 0);
        if (start_pos > sel_start)
        {
            start_pos = sel_start;
        }
    }
    DO_COMMON_LOOP:
    {
        found_pos = on_search_process_find(pnode, key, start_pos, end_pos, find_flags);
        if (found_pos >= 0)
        {
            sptr_t target_end = eu_sci_call(pnode, SCI_GETTARGETEND, 0, 0);
            eu_sci_call(pnode, SCI_SETSELECTION, found_pos, target_end);
        }
        else if (found_pos == -2)
        {
            printf("re error\n");
            return false;
        }
        else if (m_loop)
        {
            m_loop = false;
            start_pos = 0;
            goto DO_COMMON_LOOP;
        }
    }
    return (found_pos>=0);
}

static bool
on_search_first(eu_tabpage *pnode, const char *key, int opt)
{
    return on_search_common(pnode, key, opt);
}

static bool
on_search_next(eu_tabpage *pnode, const char *key, int opt)
{
    sptr_t found_pos = 0;
    sptr_t start_pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
    sptr_t end_pos = eu_sci_call(pnode, SCI_GETLENGTH, 0, 0);
    size_t find_flags = on_search_build_flags(hwnd_search_dlg);
    found_pos = on_search_process_find(pnode, key, start_pos, end_pos, find_flags);
    if (found_pos >= 0)
    {
        sptr_t target_end = eu_sci_call(pnode, SCI_GETTARGETEND, 0, 0);
        eu_sci_call(pnode, SCI_SETSELECTION, found_pos, target_end);
    }
    return (found_pos>=0);
}

static bool
on_search_at_replace_page(eu_tabpage *pnode, int opt)
{
    bool result = false;
    bool next_result = false;
    char *replace_str = on_search_get_combo_str(IDC_SEARCH_RP_CBO);
    char *find_str = on_search_get_combo_str(IDC_WHAT_FOLDER_CBO);
    if (!(pnode && find_str))
    {
        printf("get find_str && replace_str failed\n");
        return false;
    }
    if (!replace_str)
    {
        replace_str = _strdup("");
    }
    if (on_search_first(pnode, find_str, opt))
    {
        eu_sci_call(pnode, SCI_BEGINUNDOACTION, 0, 0);
        do
        {
            result = on_search_replace_target(pnode, replace_str);
            next_result = on_search_next(pnode, find_str, opt);
        } while (!(opt & ON_REPLACE_THIS) && result && next_result);
        eu_sci_call(pnode, SCI_ENDUNDOACTION, 0, 0);
    }
    if (*replace_str != 0)
    {
        eu_push_replace_history(replace_str);
    }
    free(replace_str);
    free(find_str);
    return result;
}

static void
on_search_replace_button(void)
{
    int  c_index = -1;
    int  m_index = -1;
    int  opt = ON_REPLACE_THIS;
    bool result = false;
    eu_tabpage *pnode = NULL;
    if (!(pnode = on_tabpage_focus_at()))
    {
        return;
    }
    if (!on_search_report_result(0, IDC_SEARCH_RE_BTN))
    {
        return;
    }
    if (!DLG_BTN_CHECK(hwnd_search_dlg, IDC_MATCH_ALL_FILE))
    {
        if (!on_search_at_replace_page(pnode, opt))
        {
            on_search_report_result(1, IDC_SEARCH_RE_BTN);
        }
    }
    else
    {   // 首先在当前页面查找
        result = on_search_at_replace_page(pnode, opt);
        if (result)
        {
            return;
        }
        // 标记当前页面
        c_index = TabCtrl_GetCurSel(g_tabpages);
        // 在所有页面查找
        int count = TabCtrl_GetItemCount(g_tabpages);
        if (!(opt & ON_OTHER_PAGE))
        {
            opt |= ON_OTHER_PAGE;
        }
        for (int index = 0; index < count; ++index)
        {
            eu_tabpage *p = NULL;
            TCITEM tci = {TCIF_PARAM};
            if (index == c_index)
            {
                continue;
            }
            TabCtrl_GetItem(g_tabpages, index, &tci);
            p = (eu_tabpage *) (tci.lParam);
            if (p && (result = on_search_at_replace_page(p, opt)))
            {
                on_tabpage_select_index(index);
                break;
            }
        }
    }
}

static void
on_search_replace_all_button(void)
{
    int  opt = ON_REPLACE_ALL;
    eu_tabpage *pnode = NULL;
    if (!(pnode = on_tabpage_focus_at()))
    {
        return;
    }
    if (!on_search_report_result(0, IDC_SEARCH_REALL_BTN))
    {
        return;
    }
    if (!DLG_BTN_CHECK(hwnd_search_dlg, IDC_MATCH_ALL_FILE))
    {
        on_search_at_replace_page(pnode, opt);
    }
    else
    {
        // replaces on all page
        int count = TabCtrl_GetItemCount(g_tabpages);
        for (int index = 0; index < count; ++index)
        {
            eu_tabpage *p = NULL;
            TCITEM tci = {TCIF_PARAM};
            TabCtrl_GetItem(g_tabpages, index, &tci);
            if ((p = (eu_tabpage *) (tci.lParam)) != NULL)
            {   // clear indicator and setletction
                sptr_t total_len = eu_sci_call(p, SCI_GETLENGTH, 0, 0);
                eu_sci_call(p, SCI_INDICATORCLEARRANGE, 0, total_len);
                eu_sci_call(p, SCI_SETEMPTYSELECTION, 0, 0);
                on_search_at_replace_page(p, opt);
            }
        }
    }
}

static int __stdcall
on_search_browse_folder_proc(HWND hwnd, UINT msg, LPARAM lParam, LPARAM pdata)
{   // 选择目录对话框初始化时 选中指定目录
    if (msg == BFFM_INITIALIZED && pdata != 0)
    {
        SendMessage(hwnd, BFFM_SETSELECTION, TRUE, pdata);
    }
    return 0;
}

static void
on_search_set_folder_path(LPCTSTR path)
{
    HWND hwnd_cbo = GetDlgItem(hwnd_search_dlg, IDC_SEARCH_DIR_CBO);
    if (hwnd_cbo)
    {
        ComboBox_SetText(hwnd_cbo, path);
    }
}

static int
on_search_folder_browser(void)
{
    TCHAR *path = NULL;
    LPITEMIDLIST pidl = NULL;
    eu_tabpage *pnode = NULL;
    if (!(pnode = on_tabpage_focus_at()))
    {
        return 1;
    }
    if (!(path = (TCHAR *)calloc(sizeof(TCHAR), MAX_PATH+1)))
    {
        return 1;
    }
    BROWSEINFO bi = {hwnd_search_dlg};
    {
        bi.pidlRoot = NULL;
        bi.ulFlags = BIF_USENEWUI | BIF_NONEWFOLDERBUTTON;
        bi.lpfn = on_search_browse_folder_proc;
        bi.lParam = (LPARAM)(&pnode->pathname);  //选择目录对话框弹出时的默认目录
    }
    if((pidl = SHBrowseForFolder(&bi)) && SHGetPathFromIDList(pidl, path))
    {
        on_search_set_folder_path(path);
    }
    else
    {
        on_search_set_folder_path(pnode->pathname);
    }
    if (pidl)
    {
        CoTaskMemFree(pidl);
    }
    free(path);
    return 0;
}

static void
on_search_make_file_list(const TCHAR *path, const TCHAR *name)
{
    file_trace *pfile = (file_trace *) calloc(1, sizeof(file_trace));
    if (!pfile)
    {
        return;
    }
    if (path[_tcslen(path) - 1] == _T('\\'))
    {
        _sntprintf(pfile->path, MAX_PATH, _T("%s%s"), path, name);
    }
    else
    {
        _sntprintf(pfile->path, MAX_PATH, _T("%s\\%s"), path, name);
    }
    pfile->count = 0;
    list_add_tail(&(pfile->node_file), &list_files);
}

static void
on_search_clean_file_list(void)
{
    int i = 0;
    struct list_head *pos, *n;
    list_for_each_safe(pos, n, &list_files)
    {
        file_trace *tmp = list_entry(pos, file_trace, node_file);
        if (tmp)
        {
            list_del_init(pos);
            free(tmp);
            ++i;
        }
    }
    printf("list_free(%d)\n", i);
}

static void
on_search_clean_folder_list(void)
{
    struct list_head *pos, *n;
    list_for_each_safe(pos, n, &list_folders)
    {
        folder_trace *tmp = list_entry(pos, folder_trace, node_folder);
        if (tmp)
        {
            list_del_init(pos);
            free(tmp);
        }
    }
}

// 非递归遍历文件, 深度优先
static int
on_search_folder_files(const TCHAR *parent, const TCHAR *type, size_t opt)
{
    int index = 0;
    TCHAR *p = NULL;
    HWND hwnd_stc = NULL;
    MSG msg = {0};
    folder_trace *folderlast = NULL;
    folder_trace *folderstart = NULL;
    WIN32_FIND_DATA fd = {0};
    HANDLE hfile = INVALID_HANDLE_VALUE;
    if (!(hwnd_stc = GetDlgItem(hwnd_search_dlg, IDC_PGB_STC)))
    {
        return index;
    }
    if (!(folderstart = (folder_trace *)calloc(1, sizeof(folder_trace))))
    {
        return index;
    }
    _sntprintf(folderstart->dir, MAX_PATH, _T("%s\\*"), parent);
    list_add_tail(&(folderstart->node_folder), &list_folders);
    list_for_each_entry(folderlast, &list_folders, folder_trace, node_folder)
    {
        PeekMessage(&msg, NULL,  0, 0, PM_REMOVE);
        if (msg.message == WM_QUIT)
        {
            index = 0;
            goto search_err;
        }
        if ((hfile = FindFirstFile(folderlast->dir, &fd)) == INVALID_HANDLE_VALUE)
        {
            goto search_err;
        }
        do
        {
            PeekMessage(&msg, NULL,  0, 0, PM_REMOVE);
            if (msg.message == WM_QUIT)
            {
                index = 0;
                goto search_err;
            }
            if (_tcscmp(fd.cFileName, _T(".")) == 0 || _tcscmp(fd.cFileName, _T("..")) == 0)
            {
                continue;
            }
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {   // 默认不搜索隐藏目录
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN && !(opt & INCLUDE_FOLDER_HIDDEN))
                {
                    continue;
                }
                // 包含子目录时, 子目录加入链表
                if ((opt & INCLUDE_FOLDER_SUB))
                {
                    TCHAR sub[MAX_PATH+1] = {0};
                    _sntprintf(sub, MAX_PATH, _T("%s"), folderlast->dir);
                    p = _tcsrchr(sub, _T('\\'));
                    if (p)
                    {
                        *p = 0;
                    }
                    folderstart = (folder_trace *)calloc(1, sizeof(folder_trace));
                    if (!folderstart)
                    {
                        goto search_err;
                    }
                    _sntprintf(folderstart->dir, MAX_PATH, _T("%s\\%s\\*"), sub, fd.cFileName);
                    list_add_tail(&(folderstart->node_folder), &list_folders);
                }
            }   /* 后缀名是否匹配 */
            else
            {
                p = _tcsrchr(fd.cFileName, _T('.'));
                if ((p && _tcsicmp(p, type) == 0) || type[_tcslen(type)-1] == _T('*'))
                {
                    TCHAR sub[MAX_PATH+1] = {0};
                    _sntprintf(sub, MAX_PATH, _T("%s"), folderlast->dir);
                    p = _tcsrchr(sub, _T('\\'));
                    if (p)
                    {
                        *p = 0;
                    }
                    on_search_make_file_list(sub, fd.cFileName);
                    Static_SetText(hwnd_stc, fd.cFileName);
                    ++index;
                }
            }

        } while (FindNextFile(hfile, &fd));
        share_close(hfile);
    }
search_err:
    share_close(hfile);
    on_search_clean_folder_list();
    return index;
}

static int
on_search_pcre_file_callback(pcre_conainer *pcre_info, void *para)
{
    if (pcre_info->rc > 0)
    {
        ++pcre_info->match;
    }
    return 0;
}

static void
on_search_at_search_button(int res_id)
{
    int ret = 0;
    TCHAR lpch[FT_LEN+1] = { 0 };
    HWND h_btn = GetDlgItem(hwnd_search_dlg, IDC_SEARCH_ALL_BTN);
    if (h_btn)
    {
        LOAD_I18N_RESSTR(res_id, on_str);
        Button_SetText(h_btn, on_str);
    }
}

static unsigned __stdcall
on_search_find_all_button(void* lp)
{
    size_t flags = 0;
    int count = 0;
    size_t readlen = 0;
    FILE *fp = NULL;
    char *key = NULL;
    char *u8_key = NULL;
    int new_pos = 0;
    float pos = 0.0;
    float s_progress = 0.0;
    MSG msg = {0};
    HWND hwnd_stc = NULL;
    HWND hwnd_pgb = NULL;
    HWND hwnd_cb = NULL;
    file_trace *pfile = NULL;
    uint8_t *data = NULL;
    TCHAR filetype[FT_LEN+1] = {0};
    TCHAR text[MAX_PATH+1] = {0};
    TCHAR result_str[MAX_PATH+1] = {0};
    uint64_t file_count = 0;
    uint64_t key_match = 0;
    bool whole = DLG_BTN_CHECK(hwnd_search_dlg, IDC_MATCH_WORD);
    bool incase = !DLG_BTN_CHECK(hwnd_search_dlg, IDC_MATCH_CASE);
    bool inpcre = DLG_BTN_CHECK(hwnd_search_dlg, IDC_MODE_REGEXP);
    HWND hwnd_type = GetDlgItem(hwnd_search_dlg, IDC_SEARCH_FY_CBO);
    char *u8_folder = on_search_get_combo_str(IDC_SEARCH_DIR_CBO);
    ComboBox_GetText(hwnd_type, filetype, FT_LEN);
    if (!filetype[0])
    {
        goto res_clean;
    }
    hwnd_cb = GetDlgItem(hwnd_search_dlg, IDC_SEARCH_DIR_CBO);
    ComboBox_GetText(hwnd_cb, text, MAX_PATH);
    if (!text[0])
    {
        goto res_clean;
    }
    flags = on_search_folder_flags(hwnd_search_dlg);
    if (!(u8_key = on_search_get_combo_str(IDC_WHAT_FOLDER_CBO)))
    {
        printf("key not exist!\n");
        goto res_clean;
    }
    if (flags & INCLUDE_FILE_UTF8)
    {
        key = _strdup(u8_key);
    }
    else
    {   // 根据界面语言确定代码页?
        key = eu_utf8_mbcs(CP_THREAD_ACP, u8_key, NULL);
    }
    CONTROL_HANDLE(hwnd_stc, IDC_PGB_STC, SW_SHOW)
    CONTROL_HANDLE(hwnd_pgb, IDC_PGB1, SW_SHOW)
    SendMessage(hwnd_pgb, PBM_SETPOS, 0, 0);
    ListBox_ResetContent(hwnd_found_box);
    count = on_search_folder_files(text, &filetype[1], flags);
    if (!count)
    {
        goto res_clean;
    }
    pos = (float)(100.0/count);
    if ((data = (uint8_t *) calloc(1, BUFF_SIZE+1)) == NULL)
    {
        goto res_clean;
    }
    list_for_each_entry(pfile, &list_files, file_trace, node_file)
    {
        // 窗口关闭时, 保证线程退出
        PeekMessage(&msg, NULL,  0, 0, PM_REMOVE);
        if (msg.message == WM_QUIT)
        {
            printf("we recv WM_QUIT\n");
            break;
        }
        Static_SetText(hwnd_stc, pfile->path);
        s_progress += pos;
        if (s_progress < 1.0)
        {
            new_pos = 1;
        }
        else
        {
            new_pos = (int)(s_progress + 0.5);
        }
        SendMessage(hwnd_pgb, PBM_SETPOS, new_pos, 0L);
        if ((fp = _tfopen(pfile->path, _T("rb"))) == NULL)
        {
            continue;
        }
        while ((readlen = fread((char *) data, 1, BUFF_SIZE, fp)) > 0)
        {
            if (inpcre)
            {
                pcre_conainer *pcre_info = NULL;
                pcre_info = eu_pcre_init((const char *)data, readlen, key, NULL, incase ? PCRE_CASELESS : 0);
                if (pcre_info)
                {
                    eu_pcre_exec_multi(pcre_info, on_search_pcre_file_callback, 0);
                    if (pcre_info->match > 0)
                    {
                        pfile->count += pcre_info->match;
                        on_search_push_string_listbox(pfile->path, u8_key, pfile->count);
                    }
                    eu_pcre_delete(pcre_info);
                }
            }
            else
            {
                pfile->count += eu_sunday(data, (const uint8_t *)key, readlen, strlen(key), incase, whole, false, NULL);
                if (pfile->count > 0)
                {
                    on_search_push_string_listbox(pfile->path, u8_key, pfile->count);
                }
            }
        }
        if (pfile->count > 0)
        {
            key_match += pfile->count;
            ++file_count;
        }
        fclose(fp);
    }
    LOAD_I18N_RESSTR(IDC_MSG_SEARCH_STR4, m_finish);
    _sntprintf(result_str, MAX_PATH, m_finish, file_count, key_match);
    Static_SetText(hwnd_stc, result_str);
res_clean:
    eu_safe_free(data);
    if (u8_key)
    {
        eu_push_find_history(u8_key);
        free(u8_key);
    }
    if (key)
    {
        free(key);
    }
    if (u8_folder)
    {
        eu_push_folder_history(u8_folder);
        free(u8_folder);
    }
    on_search_clean_file_list();
    _InterlockedExchange(&search_id, 0);
    // 有信号时, 代表搜索结束
    SetEvent(search_event_final);
    on_search_at_search_button(IDC_SEARCH_BTN_ON);
    return 0;
}

void
on_search_finish_wait(void)
{
    if (search_id)
    {   // we destroy windows, that it should wait for search thread exit
        PostThreadMessage(search_id, WM_QUIT, 0, 0);
        if (search_event_final)
        {
            WaitForSingleObject(search_event_final, INFINITE);
        }
        share_close(search_event_final);
    }
}

void
on_search_dark_mode_release(void)
{
    // 如果有搜索事件,等待完成
    on_search_finish_wait();
    if (hwnd_search_dlg)
    {
        DestroyWindow(hwnd_search_dlg);
        on_search_create_box();
    }
}

static void
on_search_dark_mode_init(HWND hdlg)
{
    int id;
    HWND btn = NULL;
    on_dark_allow_window(hdlg, true);
    on_dark_refresh_titlebar(hdlg);
    const int cbo_buttons[] = { IDC_WHAT_FOLDER_CBO,
                                IDC_SEARCH_RP_CBO,
                                IDC_SEARCH_FY_CBO,
                                IDC_SEARCH_DIR_CBO
                              };
    for (id = 0; id < _countof(cbo_buttons); ++id)
    {
        if ((btn = GetDlgItem(hdlg, cbo_buttons[id])))
        {
            SetWindowSubclass(btn, on_search_combo_wnd, 0, 0);
        }
    }
    const int bs_buttons[] = { IDC_MATCH_ALL_FILE,
                               IDC_MATCH_LOOP,
                               IDC_MATCH_WDSTART,
                               IDC_MATCH_WORD,
                               IDC_MATCH_CASE,
                               IDC_SEARCH_CD_CHK,
                               IDC_SEARCH_SUB_CHK,
                               IDC_SEARCH_HIDE_CHK,
                               IDC_SEARCH_UTF8_CHK,
                               IDC_SEARCH_HEX_STRINGS,
                               IDC_MODE_STATIC,
                               IDC_MODE_NORMAL,
                               IDC_MODE_REGEXP
                             };
    for (id = 0; id < _countof(bs_buttons); ++id)
    {
        if ((btn = GetDlgItem(hdlg, bs_buttons[id])))
        {
            on_dark_set_theme(btn, L"", L"");
            SendMessage(btn, WM_THEMECHANGED, 0, 0);
        }
    }
    const int buttons[] = { IDC_FILES_BROWSE_BTN,
                            IDC_SEARCH_PRE_BTN,
                            IDC_SEARCH_NEXT_BTN,
                            IDC_SEARCH_CLOSE_BTN,
                            IDC_SEARCH_RE_BTN,
                            IDC_SEARCH_REALL_BTN,
                            IDC_SEARCH_ALL_BTN,
                            IDC_SEARCH_COUNT_BTN,
                            IDC_SEARCH_FOUNDLIST,
                            IDD_SEARCH_TAB_1
                           };
    for (id = 0; id < _countof(buttons); ++id)
    {
        if ((btn = GetDlgItem(hdlg, buttons[id])))
        {
            on_dark_set_theme(btn, L"Explorer", NULL);
            on_dark_allow_window(btn, true);
            SendMessage(btn, WM_THEMECHANGED, 0, 0);
        }
    }
    if (hwnd_regxp_tips)
    {
        on_dark_set_theme(hwnd_regxp_tips, L"DarkMode_Explorer", NULL);
    }
}

static INT_PTR CALLBACK
on_search_orig_find_proc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
        case WM_INITDIALOG:
        {
            RECT rc;
            GetClientRect(hdlg, &rc);
            HWND htab = CreateWindowEx(0,
                                       WC_TABCONTROL,
                                       0,
                                       TCS_FIXEDWIDTH | WS_CHILD | WS_VISIBLE,
                                       rc.left + 2,
                                       rc.top + 2,
                                       rc.right - 4,
                                       rc.top + 24,
                                       hdlg,
                                       (HMENU)IDD_SEARCH_TAB_1,
                                       eu_module_handle(),
                                       0);
            if (htab)
            {
                SendMessage(htab, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0);
                on_search_init_pages(htab);
            }
            HWND hwnd_combo_what = GetDlgItem(hdlg, IDC_WHAT_FOLDER_CBO);
            HWND hwnd_combo_replace = GetDlgItem(hdlg, IDC_SEARCH_RP_CBO);
            HWND hwnd_dir = GetDlgItem(hdlg, IDC_SEARCH_DIR_CBO);
            HWND hwnd_fy = GetDlgItem(hdlg, IDC_SEARCH_FY_CBO);
            HWND hwnd_re_stc = GetDlgItem(hdlg, IDC_REGXP_TIPS_STC);
            if (!(hwnd_combo_what && hwnd_combo_replace && hwnd_dir && hwnd_fy && hwnd_re_stc))
            {
                break;
            }
            ShowWindow(hwnd_re_stc, SW_HIDE);
            COMBOBOXINFO cbinfo = { sizeof(COMBOBOXINFO) };
            GetComboBoxInfo(hwnd_combo_what, &cbinfo);
            orig_combo_proc = SetWindowLongPtr(cbinfo.hwndItem, GWLP_WNDPROC, (LONG_PTR)on_search_combo_proc);
            GetComboBoxInfo(hwnd_combo_replace, &cbinfo);
            SetWindowLongPtr(cbinfo.hwndItem, GWLP_WNDPROC, (LONG_PTR)on_search_combo_proc);
            GetComboBoxInfo(hwnd_dir, &cbinfo);
            SetWindowLongPtr(cbinfo.hwndItem, GWLP_WNDPROC, (LONG_PTR)on_search_combo_proc);
            hwnd_found_box = GetDlgItem(hdlg, IDC_SEARCH_FOUNDLIST);
            SendMessage(hwnd_found_box, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 0);
            SendMessage(hwnd_found_box, LB_SETHORIZONTALEXTENT,HSCRALL_LEN, 0);
            if (on_dark_enable())
            {
                on_dark_set_theme(hdlg, L"Explorer", NULL);
            }
            return 1;
        }
        case WM_COMMAND:
            {
                switch (LOWORD(wParam))
                {
                    case IDCANCEL:
                    case IDC_SEARCH_CLOSE_BTN:
                    {
                        HWND hwnd_re_stc = GetDlgItem(hdlg, IDC_REGXP_TIPS_STC);
                        if (hwnd_re_stc)
                        {
                            ShowWindow(hwnd_re_stc, SW_HIDE);
                        }
                        ShowWindow(hdlg, SW_HIDE);
                        break;
                    }
                    case IDC_SEARCH_NEXT_BTN:
                    {
                        ListBox_ResetContent(hwnd_found_box);
                        on_search_find_next_button();
                        break;
                    }
                    case IDC_SEARCH_PRE_BTN:
                    {
                        ListBox_ResetContent(hwnd_found_box);
                        on_search_find_pre_button();
                        break;
                    }
                    case IDC_SEARCH_RE_BTN:
                    {
                        on_search_replace_button();
                        break;
                    }
                    case IDC_SEARCH_REALL_BTN:
                    {
                        on_search_replace_all_button();
                        break;
                    }
                    case IDC_SEARCH_HEX_STRINGS:
                    {
                        HWND hc = NULL;
                        HWND hwnd_hex = GetDlgItem(hdlg, IDC_SEARCH_HEX_STRINGS);
                        HWND hwnd_case = GetDlgItem(hdlg, IDC_MATCH_CASE);
                        if (!IsWindowVisible(hwnd_hex))
                        {
                            break;
                        }
                        if (Button_GetCheck(hwnd_hex) == BST_CHECKED)
                        {
                            EnableWindow(hwnd_case, true);
                            CONTROL_HANDLE(hc, IDC_SEARCH_HEX_STC, SW_HIDE)
                        }
                        else
                        {
                            EnableWindow(hwnd_case, false);
                            CONTROL_HANDLE(hc, IDC_SEARCH_HEX_STC, SW_SHOW)
                        }
                        break;
                    }
                    case IDC_MODE_NORMAL:
                    {
                        HWND hwnd_normal = GetDlgItem(hdlg, IDC_MODE_NORMAL);
                        if (Button_GetCheck(hwnd_normal) == BST_CHECKED)
                        {
                            HWND hwnd_word = GetDlgItem(hdlg, IDC_MATCH_WORD);
                            HWND hwnd_wdst = GetDlgItem(hdlg, IDC_MATCH_WDSTART);
                            HWND hwnd_case = GetDlgItem(hdlg, IDC_MATCH_CASE);
                            HWND hwnd_re_stc = GetDlgItem(hdlg, IDC_REGXP_TIPS_STC);
                            EnableWindow(hwnd_word, true);
                            EnableWindow(hwnd_wdst, true);
                            EnableWindow(hwnd_case, true);
                            ShowWindow(hwnd_re_stc, SW_HIDE);
                            on_search_set_tip(IDC_MSG_SEARCH_STR1);
                        }
                        break;
                    }
                    case IDC_MODE_REGEXP:
                    {
                        HWND rexp = GetDlgItem(hdlg, IDC_MODE_REGEXP);
                        if (Button_GetCheck(rexp) == BST_CHECKED)
                        {
                            HWND hwnd_word = GetDlgItem(hdlg, IDC_MATCH_WORD);
                            HWND hwnd_wdst = GetDlgItem(hdlg, IDC_MATCH_WDSTART);
                            HWND hwnd_case = GetDlgItem(hdlg, IDC_MATCH_CASE);
                            EnableWindow(hwnd_word, false);
                            EnableWindow(hwnd_wdst, false);
                            EnableWindow(hwnd_case, true);
                        }
                        break;
                    }
                    case IDC_SEARCH_COUNT_BTN:
                    {
                        on_search_detail_button();
                        break;
                    }
                    case IDC_SEARCH_FOUNDLIST:
                    {
                        if (HIWORD(wParam) == LBN_DBLCLK)
                        {
                            on_search_found_list((HWND)lParam);
                            return 1;
                        }
                        break;
                    }
                    case IDC_FILES_BROWSE_BTN:
                    {
                        on_search_folder_browser();
                        break;
                    }
                    case IDC_SEARCH_CD_CHK:
                    {
                        eu_tabpage *pnode = NULL;
                        if (Button_GetCheck((HWND)lParam) == BST_UNCHECKED)
                        {
                            on_search_set_folder_path(_T(""));
                        }
                        else if ((pnode = on_tabpage_focus_at()))
                        {
                            on_search_set_folder_path(pnode->pathname);
                        }
                        break;
                    }
                    case IDC_SEARCH_ALL_BTN:
                    {
                        if (!search_id)
                        {
                            if (!search_event_final)
                            {
                                search_event_final = CreateEvent(NULL, FALSE, FALSE, NULL);
                            }
                            else
                            {
                                ResetEvent(search_event_final);
                            }   // 开始搜索时, 信号量重置
                            CloseHandle((HANDLE) _beginthreadex(NULL, 0, on_search_find_all_button, NULL, 0, (DWORD *)&search_id));
                            on_search_at_search_button(IDC_SEARCH_BTN_OFF);
                        }
                        else
                        {
                            printf("one search thread runing...\n");
                            PostThreadMessage(search_id, WM_QUIT, 0, 0);
                            on_search_at_search_button(IDC_SEARCH_BTN_ON);
                        }
                        break;
                    }
                    case IDC_WHAT_FOLDER_CBO:
                    {
                        if (orig_combo_proc)
                        {
                            SetWindowLongPtr(hdlg, GWLP_USERDATA, (LONG_PTR)IDC_WHAT_FOLDER_CBO);
                            eu_get_find_history(on_search_combo_callback);
                        }
                        break;
                    }
                    case IDC_SEARCH_RP_CBO:
                    {
                        if (orig_combo_proc)
                        {
                            SetWindowLongPtr(hdlg, GWLP_USERDATA, (LONG_PTR)IDC_SEARCH_RP_CBO);
                            eu_get_replace_history(on_search_combo_callback);
                        }
                        break;
                    }
                    case IDC_SEARCH_DIR_CBO:
                    {
                        if (orig_combo_proc)
                        {
                            SetWindowLongPtr(hdlg, GWLP_USERDATA, (LONG_PTR)IDC_SEARCH_DIR_CBO);
                            eu_get_folder_history(on_search_combo_callback);
                        }
                        break;
                    }
                    default:
                        break;
                }
            }
            break;
        case WM_SHOWWINDOW:
            if (!IsWindowVisible(hdlg))
            {
                on_search_set_tip(IDC_MSG_SEARCH_STR1);
            }
            break;
        CASE_WM_CTLCOLOR_SET:
        {
            if (on_dark_enable())
            {
                return on_dark_set_contorl_color(wParam);
            }
            else if ((HWND)lParam == GetDlgItem(hdlg, IDC_SEARCH_TIPS_STC) ||
                     (HWND)lParam == GetDlgItem(hdlg, IDC_SEARCH_HEX_STC)  ||
                     (HWND)lParam == GetDlgItem(hdlg, IDC_REGXP_TIPS_STC))
            {
                    // 绘制静态控件上的文本颜色
                    SetBkMode((HDC)wParam, TRANSPARENT);
                    SetTextColor((HDC)wParam, RGB(255,0,0));
                    // 不要使用GetSolidBrush, 会泄露GDI句柄.
                    return (INT_PTR)GetSysColorBrush(COLOR_MENU);
            }
            break;
        }
        case WM_THEMECHANGED:
        {
            if (on_dark_supports())
            {
                on_search_dark_mode_init(hdlg);
            }
            break;
        }
        case WM_DESTROY:
        {
            printf("Search window WM_DESTROY\n");
            if (hwnd_regxp_tips)
            {
                DestroyWindow(hwnd_regxp_tips);
                hwnd_regxp_tips = NULL;
            }
            break;
        }
    }
    return 0;
}

static void
on_search_do_space(eu_tabpage *pnode, const char *key, const char *str_replace)
{
    sptr_t fpos = -1;
    sptr_t pos = 0;
    sptr_t max_pos = 0;
    const int flags = SCFIND_REGEXP;
    sptr_t cur_pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
    sptr_t anchor_pos = eu_sci_call(pnode, SCI_GETANCHOR, 0, 0);
    if (!(pnode && key && str_replace))
    {
        return;
    }
    if (cur_pos == anchor_pos)
    {
        max_pos = eu_sci_call(pnode, SCI_GETTEXTLENGTH, 0, 0);
    }
    else if (!eu_sci_call(pnode, SCI_SELECTIONISRECTANGLE, 0, 0))
    {
        pos = eu_sci_call(pnode, SCI_GETSELECTIONSTART, 0, 0);
        max_pos = eu_sci_call(pnode, SCI_GETSELECTIONEND, 0, 0);
    }
    else
    {
        MSG_BOX(IDS_SELRECT, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
        return;
    }
    if (max_pos - pos < (sptr_t)strlen(str_replace))
    {
        return;
    }
    eu_sci_call(pnode, SCI_BEGINUNDOACTION, 0, 0);
    while (pos < max_pos && (fpos = on_search_process_find(pnode, key, pos, max_pos, flags)) >= 0)
    {
        sptr_t target_end = eu_sci_call(pnode, SCI_GETTARGETEND, 0, 0);
        eu_sci_call(pnode, SCI_SETSELECTION, fpos, target_end);
        int re_len = (int)eu_sci_call(pnode, SCI_REPLACETARGETRE, (WPARAM)-1, (LPARAM)str_replace);
        pos = fpos + re_len;
        // 替换后, 末尾位置发生了变化, 要减去或加上差值
        max_pos -= ((int)(target_end - fpos) - re_len);
    }
    eu_sci_call(pnode, SCI_ENDUNDOACTION, 0, 0);
    if (pos > 1)
    {
        eu_sci_call(pnode, SCI_GOTOPOS, pos, 0);
    }
}

void
on_search_tab2space(eu_tabpage *pnode)
{
    int flags = SCFIND_REGEXP;
    char str_replace[ACNAME_LEN] = {0};
    const char *key = "\t";
    int number = 0;
    if (!pnode)
    {
        return;
    }
    if (pnode->doc_ptr && pnode->doc_ptr->tab_width > 0)
    {
        number = pnode->doc_ptr->tab_width;
    }
    else
    {
        number = eu_get_config()->tab_width;
    }
    memset(str_replace, 0x20, number);
    on_search_do_space(pnode, key, str_replace);
}

void
on_search_space2tab(eu_tabpage *pnode)
{
    int flags = SCFIND_REGEXP;
    char key[ACNAME_LEN] = {0};
    const char *str_replace = "\t";
    int number = 0;
    if (!pnode)
    {
        return;
    }
    if (pnode->doc_ptr && pnode->doc_ptr->tab_width > 0)
    {
        number = pnode->doc_ptr->tab_width;
    }
    else
    {
        number = eu_get_config()->tab_width;
    }
    memset(key, 0x20, number);
    on_search_do_space(pnode, key, str_replace);
}

bool
on_search_create_box(void)
{
    hwnd_search_dlg = i18n_create_dialog(eu_module_hwnd(), IDD_SEARCH_TAB_DLG, on_search_orig_find_proc);
    if (!hwnd_search_dlg)
    {
        printf("i18n_create_dialog return false in %s\n", __FUNCTION__);
        return false;
    }
    ShowWindow(hwnd_search_dlg, SW_HIDE);
    return (true);
}

HWND
eu_get_search_hwnd(void)
{
    return hwnd_search_dlg;
}
