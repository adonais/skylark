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
#include <shlobj.h>

#define MAX_TRACE_COUNT 8192
#define RESULAT_MAX_MATCH (UINT16_MAX * 2 + 1)
#define INVISIBLE_BITMASK()      on_sci_bitmask_get(0, 1)
#define MARKERS_BITMASK()        on_sci_bitmask_get(0, MARGIN_BOOKMARK_VALUE + 1)
#define BOOKMARK_BITMASK()       on_sci_bitmask_get(MARGIN_BOOKMARK_VALUE, 1)
#define CHANGE_HISTORY_BITMASK() on_sci_bitmask_get(SC_MARKNUM_HISTORY_REVERTED_TO_ORIGIN, \
    (SC_MARKNUM_HISTORY_REVERTED_TO_MODIFIED - SC_MARKNUM_HISTORY_REVERTED_TO_ORIGIN + 1))

static int max_nav_count;
static int result_line_width;
static HWND hwnd_found_box;
static HWND hwnd_search_dlg;
static LONG_PTR orig_combo_proc;
static LONG_PTR orig_tab_proc;
static LIST_HEAD(list_trace);
static LIST_HEAD(list_files);
static LIST_HEAD(list_folders);
static volatile long search_btn_id = 0;
static HANDLE search_event_final = NULL;

typedef struct _report_data
{
    eu_tabpage *p;
    int code;
    int button;
    bool thr;
}report_data;

#define HSCRALL_LEN 768
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
on_search_set_result(const int res_id, int64_t num, int64_t count)
{
    HWND stc = GetDlgItem(hwnd_search_dlg, IDC_SEARCH_TIPS_STC);
    if (stc)
    {
        TCHAR msg[MAX_LOADSTRING] = {0};
        LOAD_I18N_RESSTR(res_id, str);
        if (num < 0)
        {
            if (count > 0)
            {
                _sntprintf(msg, MAX_LOADSTRING-1, str, count);
            }
            else
            {
                _sntprintf(msg, MAX_LOADSTRING-1, str, count);
            }
        }
        else if (count >= 0)
        {
            _sntprintf(msg, MAX_LOADSTRING-1, str, num, count);
        }
        else
        {
            _sntprintf(msg, MAX_LOADSTRING-1, str, num);
        }
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
    HWND hc = NULL;
    RECT rc = {0};
    eu_tabpage *pnode = on_tabpage_focused();
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
        CONTROL_HANDLE(hc, IDC_SEARCH_START_ENGINE, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_CD_CHK, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_SUB_CHK, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_HIDE_CHK, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_UTF8_CHK, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_FOUNDLIST, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_PGB_STC, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_PGB1, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_SELRE_BTN, SW_HIDE)
        hc = GetDlgItem(hwnd_search_dlg, IDC_WHAT_FOLDER_CBO);
        EnableWindow(hc, true);
        hc = GetDlgItem(hwnd_search_dlg, IDC_MATCH_CASE);
        if (!TAB_HEX_MODE(pnode))
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
            CONTROL_HANDLE(hc, IDC_SEARCH_ALL_BTN, SW_SHOW)
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
            CONTROL_HANDLE(hc, IDC_SEARCH_HEX_STRINGS, SW_SHOW)
            CONTROL_HANDLE(hc, IDC_SEARCH_ALL_BTN, SW_HIDE)
            CONTROL_HANDLE(hc, IDC_SEARCH_CLOSE_BTN, SW_SHOW)
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
        CONTROL_HANDLE(hc, IDC_SEARCH_START_ENGINE, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_COUNT_BTN, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_CD_CHK, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_SUB_CHK, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_HIDE_CHK, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_UTF8_CHK, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_FOUNDLIST, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_PGB_STC, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_PGB1, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_ALL_BTN, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_HEX_STRINGS, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_HEX_STC, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_MATCH_ALL_FILE, !TAB_HEX_MODE(pnode) ? SW_SHOW : SW_HIDE)
        CONTROL_HANDLE(hc, IDC_MATCH_LOOP, !TAB_HEX_MODE(pnode) ? SW_SHOW : SW_HIDE)
        CONTROL_HANDLE(hc, IDC_MATCH_WDSTART, !TAB_HEX_MODE(pnode) ? SW_SHOW : SW_HIDE)
        CONTROL_HANDLE(hc, IDC_MATCH_WORD, !TAB_HEX_MODE(pnode) ? SW_SHOW : SW_HIDE)
        hc = GetDlgItem(hwnd_search_dlg, IDC_MATCH_CASE);
        EnableWindow(hc, true);
        hc = GetDlgItem(hwnd_search_dlg, IDC_MODE_REGEXP);
        EnableWindow(hc, true);
        CONTROL_HANDLE(hc, IDC_SEARCH_PRE_BTN, !TAB_HEX_MODE(pnode) ? SW_SHOW : SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_NEXT_BTN, !TAB_HEX_MODE(pnode) ? SW_SHOW : SW_HIDE)
        GetWindowRect(hc, &rc);
        MapWindowPoints(NULL, hwnd_search_dlg, (LPPOINT)&rc, 2);
        CONTROL_HANDLE(hc, IDC_SEARCH_RE_BTN, !TAB_HEX_MODE(pnode) ? SW_SHOW : SW_HIDE)
        MoveWindow(hc, rc.left, rc.bottom + 4, rc.right - rc.left, rc.bottom - rc.top, TRUE);
        CONTROL_HANDLE(hc, IDC_SEARCH_REALL_BTN, !TAB_HEX_MODE(pnode) ? SW_SHOW : SW_HIDE)
        MoveWindow(hc, rc.left, rc.bottom + (rc.bottom - rc.top) + 6, rc.right - rc.left, rc.bottom - rc.top, TRUE);
        hc = GetDlgItem(hwnd_search_dlg, IDC_WHAT_FOLDER_CBO);
        if (TAB_HEX_MODE(pnode))
        {
            EnableWindow(hc, false);
            CONTROL_HANDLE(hc, IDC_SEARCH_SELRE_BTN, SW_HIDE)
        }
        else
        {
            EnableWindow(hc, true);
            CONTROL_HANDLE(hc, IDC_SEARCH_SELRE_BTN, SW_SHOW)
            MoveWindow(hc, rc.left, rc.bottom + (rc.bottom - rc.top) * 2 + 8, rc.right - rc.left, rc.bottom - rc.top, TRUE);
            util_can_selections(pnode) ? EnableWindow(hc, true) : EnableWindow(hc, false);

        }
        CONTROL_HANDLE(hc, IDC_SEARCH_RP_CBO, !TAB_HEX_MODE(pnode) ? SW_SHOW : SW_HIDE)
        TAB_HEX_MODE(pnode) ? EnableWindow(hc, false) : EnableWindow(hc, true);
        CONTROL_HANDLE(hc, IDC_SEARCH_CLOSE_BTN, SW_SHOW)
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
        CONTROL_HANDLE(hc, IDC_SEARCH_ALL_BTN, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_SELRE_BTN, SW_HIDE)
        hc = GetDlgItem(hwnd_search_dlg, IDC_MODE_REGEXP);
        EnableWindow(hc, true);
        hc = GetDlgItem(hwnd_search_dlg, IDC_MATCH_CASE);
        EnableWindow(hc, true);
        CONTROL_HANDLE(hc, IDC_MATCH_WORD, SW_SHOW)
        CONTROL_HANDLE(hc, IDC_SEARCH_START_ENGINE, SW_SHOW)
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
        CONTROL_HANDLE(hc, IDC_SEARCH_FOUNDLIST, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_HEX_STRINGS, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_SEARCH_HEX_STC, SW_HIDE)
        CONTROL_HANDLE(hc, IDC_REGXP_TIPS_STC, SW_HIDE)
        if (search_btn_id)
        {
            CONTROL_HANDLE(hc, IDC_PGB_STC, SW_SHOW)
            CONTROL_HANDLE(hc, IDC_PGB1, SW_SHOW)
        }
        else
        {
            CONTROL_HANDLE(hc, IDC_PGB_STC, SW_HIDE)
            CONTROL_HANDLE(hc, IDC_PGB1, SW_HIDE)
        }
        on_search_fill_combo();
    }
    if (hwnd_search_dlg)
    {
        on_search_set_tip(IDC_MSG_SEARCH_STR1);
        UpdateWindow(hwnd_search_dlg);
    }
}

static void
on_search_init_option(void)
{
    const btn_state bs[] =
    {
        {IDC_MATCH_ALL_FILE, ON_REPLACE_ALL},
        {IDC_MATCH_LOOP, ON_LOOP_FLAGS},
        {IDC_MATCH_WDSTART, SCFIND_WORDSTART},
        {IDC_MATCH_WORD, SCFIND_WHOLEWORD},
        {IDC_MATCH_CASE, SCFIND_MATCHCASE},
        {IDC_SEARCH_CD_CHK, INCLUDE_CURRENT_FOLDER},
        {IDC_SEARCH_SUB_CHK, INCLUDE_FOLDER_SUB},
        {IDC_SEARCH_HIDE_CHK, INCLUDE_FOLDER_HIDDEN},
        {IDC_SEARCH_UTF8_CHK, INCLUDE_FILE_UTF8},
        {IDC_SEARCH_HEX_STRINGS, ON_HEX_STRINGS},
        {IDC_MODE_NORMAL, NO_REGXP_FLAGS},
        {IDC_MODE_REGEXP, SCFIND_REGEXP}
    };
    for (int i = 0; i < _countof(bs); ++i)
    {
        HWND btn = GetDlgItem(hwnd_search_dlg, bs[i].id);
        if (eu_get_config()->last_flags & bs[i].mask)
        {
            Button_SetCheck(btn, BST_CHECKED);
        }
        else
        {
            Button_SetCheck(btn, BST_UNCHECKED);
        }
    }
    if (eu_get_config()->last_flags & IDC_MODE_REGEXP)
    {
        PostMessage(hwnd_search_dlg, WM_COMMAND, MAKEWPARAM(IDC_MODE_REGEXP, 0), 0);
    }
    else
    {
        PostMessage(hwnd_search_dlg, WM_COMMAND, MAKEWPARAM(IDC_MODE_NORMAL, 0), 0);
    }
    PostMessage(hwnd_search_dlg, WM_COMMAND, MAKEWPARAM(IDC_SEARCH_HEX_STRINGS, 0), 0);
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
on_search_jmp_pos(eu_tabpage *pnode)
{
    if (pnode)
    {
        if (!pnode->nc_pos || TAB_HEX_MODE(pnode))
        {
            eu_sci_call(pnode, SCI_GOTOPOS, pnode->nc_pos, 0);
        }
        else if (pnode->nc_pos > 0)
        {
            sptr_t line = eu_sci_call(pnode, SCI_LINEFROMPOSITION, pnode->nc_pos, 0);
            on_search_jmp_line(pnode, line, 0);
            eu_sci_call(pnode, SCI_GOTOPOS, pnode->nc_pos, 0);
        }
    }
}

void
on_search_jmp_matching_brace(eu_tabpage *pnode, int *pres)
{
    if (pnode && pres && !TAB_HEX_MODE(pnode) && !pnode->plugin)
    {
        int char_before = 0;
        sptr_t brace_caret = -1;
        sptr_t brace_pos = -1;
        sptr_t caret_pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
        sptr_t length = eu_sci_call(pnode, SCI_GETLENGTH, 0, 0);
        if ((length > 0) && (caret_pos > 0))
        {
            char_before = (int)eu_sci_call(pnode, SCI_GETCHARAT, caret_pos - 1, 0);
        }
        if (char_before && strchr("[](){}", char_before))
        {
            brace_caret = caret_pos - 1;
        }
        if (length > 0  && (brace_caret < 0))
        {   // 没找到, 向另一侧匹配
            int char_after = (int)eu_sci_call(pnode, SCI_GETCHARAT, caret_pos, 0);
            if (char_after && strchr("[](){}", char_after))
            {
                brace_caret = caret_pos;
            }
        }
        if (brace_caret >= 0)
        {
            brace_pos = eu_sci_call(pnode, SCI_BRACEMATCH, brace_caret, 0);
        }
        if (brace_pos != -1)
        {
            if (*pres == IDM_SEARCH_MATCHING_BRACE)
            {
                eu_sci_call(pnode, SCI_GOTOPOS, brace_pos, 0);
            }
            else if (*pres == IDM_SEARCH_MATCHING_BRACE_SELECT)
            {
                eu_sci_call(pnode, SCI_SETSEL, min(brace_caret, brace_pos), max(brace_caret, brace_pos) + 1);
            }
            else
            {
                *pres = 1;
            }
        }
        else
        {
            *pres = 0;
        }
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
    if (!(flags & SCFIND_REGEXP))
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
    TCHAR text[MAX_BUFFER]= {0};
    hwnd_cb = GetDlgItem(hwnd_search_dlg, resid);
    ComboBox_GetText(hwnd_cb, text, MAX_BUFFER);
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

static void
on_search_set_folder_path(LPCTSTR path)
{
    HWND hwnd_cbo = GetDlgItem(hwnd_search_dlg, IDC_SEARCH_DIR_CBO);
    if (hwnd_cbo)
    {
        ComboBox_SetText(hwnd_cbo, path);
    }
}

void
on_search_turn_select(eu_tabpage *pnode)
{
    if (hwnd_search_dlg && pnode && !TAB_HEX_MODE(pnode))
    {
        HWND hwnd_tab = GetDlgItem(hwnd_search_dlg, IDD_SEARCH_TAB_1);
        if (hwnd_tab && TabCtrl_GetCurSel(hwnd_tab) == 1)
        {
            HWND hc = GetDlgItem(hwnd_search_dlg, IDC_SEARCH_SELRE_BTN);
            if (hc && (GetWindowLongPtr(hc, GWL_STYLE) & WS_VISIBLE))
            {
                util_can_selections(pnode) ? EnableWindow(hc, true) : EnableWindow(hc, false);
            }
        }
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
    if (pnode && !TAB_HEX_MODE(pnode) && !pnode->pmod)
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
    eu_tabpage *pnode = on_tabpage_focused();
    if (hwnd_what && hwnd_tab && pnode)
    {
        HWND chk = GetDlgItem(hwnd_search_dlg, IDC_SEARCH_CD_CHK);
        util_push_text_dlg(pnode, hwnd_what);
        TabCtrl_SetCurSel(hwnd_tab, 2);
        on_search_tab_ui(2);
        on_search_init_option();
        util_creater_window(hwnd_search_dlg, eu_module_hwnd());
        ShowWindow(hwnd_search_dlg, SW_SHOW);
        if (STR_NOT_NUL(path))
        {
            Button_SetCheck(chk, BST_UNCHECKED);
            on_search_set_folder_path(path);
        }
        else
        {
            SendMessage(hwnd_search_dlg, WM_COMMAND, MAKEWPARAM(IDC_SEARCH_CD_CHK, 0), (LPARAM)chk);
        }
        SendMessage(hwnd_search_dlg, DM_SETDEFID, IDC_SEARCH_START_ENGINE, 0);
        eu_get_find_history(on_search_combo_callback);
        eu_get_folder_history(on_search_combo_callback);
        on_search_set_focus(pnode, hwnd_what);
    }
    return 0;
}

void
on_search_set_selection(eu_tabpage *pnode)
{
    if (pnode && !TAB_HEX_MODE(pnode) && !pnode->pmod)
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
    if (pnode && !TAB_HEX_MODE(pnode) && !pnode->pmod)
    {
        if (eu_sci_call(pnode, SCI_GETSELECTIONMODE, 0, 0) == SC_SEL_STREAM)
        {
            if (eu_sci_call(pnode, SCI_GETZOOM, 0, 0) == 0)
            {
                on_view_zoom_out(pnode);
                on_view_zoom_out(pnode);
                pnode->zoom_level = SELECTION_ZOOM_LEVEEL;
                eu_logmsg("%s: pnode->zoom_level = %d\n", __FUNCTION__, pnode->zoom_level);
            }
            eu_sci_call(pnode, SCI_SETSELECTIONMODE, SC_SEL_RECTANGLE, 0);
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
    if (pnode && !TAB_HEX_MODE(pnode) && !pnode->pmod)
    {
        eu_sci_call(pnode, SCI_SELECTALL, 0, 0);
    }
}

void
on_search_select_word(eu_tabpage *pnode)
{
    if (pnode && !TAB_HEX_MODE(pnode) && !pnode->pmod)
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
    if (pnode && !TAB_HEX_MODE(pnode) && !pnode->pmod)
    {
        // 支持类似visual studio的单行操作
        // TAB键增加缩进, Shift+TAB减少缩进
        eu_sci_call(pnode, SCI_SETSELECTIONMODE, SC_SEL_LINES, 0);
    }
}

void
on_search_select_se(eu_tabpage *pnode, uint16_t id)
{
    if (pnode && !TAB_HEX_MODE(pnode) && !pnode->pmod)
    {
        sptr_t sel_start = 0;
        sptr_t sel_end = 0;
        if (id == IDM_SEARCH_SELECT_END)
        {
            sel_start = eu_sci_call(pnode, SCI_GETSELECTIONSTART, 0, 0);
            sel_end = eu_sci_call(pnode, SCI_GETLENGTH, 0, 0);
        }
        else if (id == IDM_SEARCH_SELECT_HEAD)
        {
            sel_start = 0;
            sel_end =  eu_sci_call(pnode, SCI_GETSELECTIONEND, 0, 0);
        }
        eu_sci_call(pnode, SCI_SETSELECTIONSTART, sel_start, 0);
        eu_sci_call(pnode, SCI_SETSELECTIONEND, sel_end, 0);
    }
}

void
on_search_select_left_word(eu_tabpage *pnode)
{
    if (pnode && !TAB_HEX_MODE(pnode) && !pnode->pmod)
    {
        eu_sci_call(pnode, SCI_WORDPARTLEFTEXTEND, 0, 0);
    }
}

void
on_search_select_right_word(eu_tabpage *pnode)
{
    if (pnode && !TAB_HEX_MODE(pnode) && !pnode->pmod)
    {
        eu_sci_call(pnode, SCI_WORDPARTRIGHTEXTEND, 0, 0);
    }
}

void
on_search_select_left_group(eu_tabpage *pnode)
{
    if (pnode && !TAB_HEX_MODE(pnode) && !pnode->pmod)
    {
        eu_sci_call(pnode, SCI_WORDLEFTEXTEND, 0, 0);
    }
}

void
on_search_select_right_group(eu_tabpage *pnode)
{
    if (pnode && !TAB_HEX_MODE(pnode) && !pnode->pmod)
    {
        eu_sci_call(pnode, SCI_WORDRIGHTEXTEND, 0, 0);
    }
}

void
on_search_cumulative_previous_block(eu_tabpage *pnode)
{
    if (pnode && !TAB_HEX_MODE(pnode) && !pnode->pmod)
    {
        eu_sci_call(pnode, SCI_PARAUPEXTEND, 0, 0);
    }
}

void
on_search_cumulative_next_block(eu_tabpage *pnode)
{
    if (pnode && !TAB_HEX_MODE(pnode) && !pnode->pmod)
    {
        eu_sci_call(pnode, SCI_PARADOWNEXTEND, 0, 0);
    }
}

void
on_search_move_to_lgroup(eu_tabpage *pnode)
{
    if (pnode && !TAB_HEX_MODE(pnode) && !pnode->pmod)
    {
        eu_sci_call(pnode, SCI_WORDPARTLEFT, 0, 0);
    }
}

void
on_search_move_to_rgroup(eu_tabpage *pnode)
{
    if (pnode && !TAB_HEX_MODE(pnode) && !pnode->pmod)
    {
        eu_sci_call(pnode, SCI_WORDPARTRIGHT, 0, 0);
    }
}

void
on_search_move_to_lword(eu_tabpage *pnode)
{
    if (pnode && !TAB_HEX_MODE(pnode) && !pnode->pmod)
    {
        eu_sci_call(pnode, SCI_WORDLEFT, 0, 0);
    }
}

void
on_search_move_to_rword(eu_tabpage *pnode)
{
    if (pnode && !TAB_HEX_MODE(pnode) && !pnode->pmod)
    {
        eu_sci_call(pnode, SCI_WORDRIGHT, 0, 0);
    }
}

void
on_search_move_to_top_block(eu_tabpage *pnode)
{
    if (pnode && !TAB_HEX_MODE(pnode) && !pnode->pmod)
    {
        eu_sci_call(pnode, SCI_PARAUP, 0, 0);
    }
}

void
on_search_move_to_bottom_block(eu_tabpage *pnode)
{
    if (pnode && !TAB_HEX_MODE(pnode) && !pnode->pmod)
    {
        eu_sci_call(pnode, SCI_PARADOWN, 0, 0);
    }
}

void
on_search_jmp_home(eu_tabpage *pnode)
{
    if (pnode && !pnode->pmod)
    {
        if (TAB_HEX_MODE(pnode))
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
    if (pnode && !pnode->pmod)
    {
        if (TAB_HEX_MODE(pnode))
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
    if (pnode && !pnode->pmod)
    {
        if (TAB_HEX_MODE(pnode))
        {
            eu_i18n_load_str(IDC_MSG_SEARCH_STR5, tip_str, 0);
        }
        else
        {
            eu_i18n_load_str(IDC_MSG_SEARCH_ERR2, tip_str, 0);
        }
        if (eu_input(tip_str, lineno, _countof(lineno)))
        {
            if (TAB_HEX_MODE(pnode))
            {

                if (_stscanf(lineno, _T("%zx"), &line) == 1)
                {
                    SendMessage(pnode->hwnd_sc, HVM_GOPOS, line, 0);
                }

            }
            else
            {
                line = _tstoz(lineno);
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
    if (pnode && !TAB_HEX_MODE(pnode) && !pnode->pmod)
    {
        int bitmask = 0;
        bool bookmark = false;
        sptr_t mark_num;
        sptr_t current_line;
        const int  all = MARKERS_BITMASK();
        if (lineno < 0)
        {
            sptr_t pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
            current_line = eu_sci_call(pnode, SCI_LINEFROMPOSITION, pos, 0);
        }
        else
        {
            current_line = lineno;
        }
        if (true)
        {
            mark_num = eu_sci_call(pnode, SCI_MARKERGET, current_line, 0);
            bitmask = (int)mark_num & all;
            bookmark = (bool)(bitmask & BOOKMARK_BITMASK());
        }
        if (bookmark)
        {
            eu_sci_call(pnode, SCI_MARKERDELETE, current_line, MARGIN_BOOKMARK_VALUE);
        }
        if (!bitmask || (bitmask == INVISIBLE_BITMASK()))
        {
            eu_sci_call(pnode, SCI_MARKERADD, current_line, MARGIN_BOOKMARK_VALUE);
        }
    }
}

static void
on_search_add_mark(eu_tabpage *pnode, sptr_t lineno)
{
    sptr_t current_line = lineno;
    if (pnode && !TAB_HEX_MODE(pnode) && !pnode->pmod)
    {
        if (current_line < 0)
        {
            sptr_t pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
            current_line = eu_sci_call(pnode, SCI_LINEFROMPOSITION, pos, 0);
        }
        eu_sci_call(pnode, SCI_MARKERADD, current_line, MARGIN_BOOKMARK_VALUE);
    }
}

static void
on_search_remove_marks_this(eu_tabpage *pnode)
{
    if (pnode && !TAB_HEX_MODE(pnode) && !pnode->pmod)
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
    const HWND htab= on_tabpage_hwnd(pnode);
    for (int index = 0, count = TabCtrl_GetItemCount(htab); index < count; ++index)
    {
        if ((p = on_tabpage_get_ptr(htab, index)) && !TAB_HEX_MODE(p) && !p->pmod)
        {
            eu_sci_call(p, SCI_MARKERDELETEALL, MARGIN_BOOKMARK_VALUE, 0);
        }
    }
}

sptr_t
on_search_marker_next(eu_tabpage *pnode, const sptr_t line, sptr_t last, const int bitmask)
{
    if (bitmask & CHANGE_HISTORY_BITMASK())
    {
        if (last <= 0)
        {
            last = eu_sci_call(pnode, SCI_GETLINECOUNT, 0, 0);
        }
        for (sptr_t i = line; i <= last; ++i)
        {
            if (eu_sci_call(pnode, SCI_MARKERGET, i, 0) & bitmask)
            {
                return i;
            }
        }
        return LINE_NOT_FOUND;
    }
    return eu_sci_call(pnode, SCI_MARKERNEXT, line, bitmask);
}

static inline sptr_t
on_search_marker_previous(eu_tabpage *pnode, const sptr_t line, const int bitmask)
{
    if (bitmask & CHANGE_HISTORY_BITMASK())
    {
        for (sptr_t i = line; i >= 0; --i)
        {
            if (eu_sci_call(pnode, SCI_MARKERGET, i, 0) & bitmask)
            {
                return i;
            }
        }
        return LINE_NOT_FOUND;
    }
    return eu_sci_call(pnode, SCI_MARKERPREVIOUS, line, bitmask);
}

void
on_search_jmp_premark_this(eu_tabpage *pnode, const int mask)
{
    if (pnode && !TAB_HEX_MODE(pnode) && !pnode->pmod)
    {
        sptr_t pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
        sptr_t current_line = eu_sci_call(pnode, SCI_LINEFROMPOSITION, pos, 0);
        sptr_t find_line = on_search_marker_previous(pnode, current_line - 1, mask);
        if (find_line != LINE_NOT_FOUND)
        {
            on_search_jmp_line(pnode, find_line, current_line);
        }
    }
}

void
on_search_jmp_next_mark_this(eu_tabpage *pnode, const int mask)
{
    if (pnode && !TAB_HEX_MODE(pnode) && !pnode->pmod)
    {
        sptr_t pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
        sptr_t current_line = eu_sci_call(pnode, SCI_LINEFROMPOSITION, pos, 0);
        sptr_t find_line = on_search_marker_next(pnode, current_line + 1, -1, mask);
        if (find_line != LINE_NOT_FOUND)
        {
            on_search_jmp_line(pnode, find_line, current_line);
        }
    }
}

void
on_search_jmp_previous_history(eu_tabpage *pnode)
{
    if (pnode && !TAB_HEX_MODE(pnode) && !pnode->pmod)
    {
        const int maskn = eu_get_config()->history_mask - IDM_VIEW_HISTORY_PLACEHOLDE;
        if (maskn > 1)
        {
            const bool non_mark = !(maskn & SC_CHANGE_HISTORY_MARKERS);
            if (non_mark)
            {
                eu_sci_call(pnode, SCI_SETCHANGEHISTORY, SC_CHANGE_HISTORY_MARKERS | eu_sci_call(pnode, SCI_GETCHANGEHISTORY, 0, 0), 0);
            }
            on_search_jmp_premark_this(pnode, MARGIN_HISTORY_MASKN);
            if (non_mark)
            {
                eu_sci_call(pnode, SCI_SETCHANGEHISTORY, maskn, 0);
            }
        }
    }
}

void
on_search_jmp_next_history(eu_tabpage *pnode)
{
    if (pnode && !TAB_HEX_MODE(pnode) && !pnode->pmod)
    {
        const int maskn = eu_get_config()->history_mask - IDM_VIEW_HISTORY_PLACEHOLDE;
        if (maskn > 1)
        {
            const bool non_mark = !(maskn & SC_CHANGE_HISTORY_MARKERS);
            if (non_mark)
            {
                eu_sci_call(pnode, SCI_SETCHANGEHISTORY, SC_CHANGE_HISTORY_MARKERS | eu_sci_call(pnode, SCI_GETCHANGEHISTORY, 0, 0), 0);
            }
            on_search_jmp_next_mark_this(pnode, MARGIN_HISTORY_MASKN);
            if (non_mark)
            {
                eu_sci_call(pnode, SCI_SETCHANGEHISTORY, maskn, 0);
            }
        }
    }
}

int
on_search_jmp_premark_all(eu_tabpage *pnode)
{
    sptr_t pos = -1, find_line = -1;
    sptr_t max_line = 0, current_line = 0;
    eu_tabpage *p = NULL;
    const HWND htab = on_tabpage_hwnd(pnode);
    if (!htab)
    {
        return EUE_UNKOWN_ERR;
    }
    for (int index = 0, count = TabCtrl_GetItemCount(htab); index < count; ++index)
    {
        if ((p = on_tabpage_get_ptr(htab, index)) && (p == pnode))
        {
            pos = eu_sci_call(p, SCI_GETCURRENTPOS, 0, 0);
            current_line = eu_sci_call(p, SCI_LINEFROMPOSITION, pos, 0);
            find_line = eu_sci_call(p, SCI_MARKERPREVIOUS, current_line - 1, MARGIN_BOOKMARK_MASKN);
            if (find_line != LINE_NOT_FOUND)
            {
                on_search_jmp_line(p, find_line, current_line);
                break;
            }
            else
            {
                for (index--; index >= 0; index--)
                {
                    if ((p = on_tabpage_get_ptr(htab, index)))
                    {
                        max_line = eu_sci_call(p, SCI_GETLINECOUNT, 0, 0);
                        find_line = eu_sci_call(p, SCI_MARKERPREVIOUS, max_line - 1, MARGIN_BOOKMARK_MASKN);
                        if (find_line != LINE_NOT_FOUND)
                        {
                            on_tabpage_select_index(htab, index);
                            on_search_jmp_line(p, find_line, max_line);
                            return SKYLARK_OK;
                        }
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
    sptr_t find_line = -1;
    sptr_t pos = 0, current_line = 0;
    eu_tabpage *p = NULL;
    const HWND htab = on_tabpage_hwnd(pnode);
    if (!htab)
    {
        return EUE_UNKOWN_ERR;
    }
    for (int index = 0, count = TabCtrl_GetItemCount(htab); index < count; ++index)
    {
        if ((p = on_tabpage_get_ptr(htab, index)) && (p == pnode))
        {
            pos = eu_sci_call(p, SCI_GETCURRENTPOS, 0, 0);
            current_line = eu_sci_call(p, SCI_LINEFROMPOSITION, pos, 0);
            find_line = eu_sci_call(p, SCI_MARKERNEXT, current_line + 1, MARGIN_BOOKMARK_MASKN);
            if (find_line != LINE_NOT_FOUND)
            {
                on_search_jmp_line(p, find_line, current_line);
                break;
            }
            else
            {
                for (index++; index < count; index++)
                {
                    if ((p = on_tabpage_get_ptr(htab, index)) && !TAB_HEX_MODE(p) && !p->pmod)
                    {
                        find_line = eu_sci_call(p, SCI_MARKERNEXT, 0, MARGIN_BOOKMARK_MASKN);
                        if (find_line != LINE_NOT_FOUND)
                        {
                            on_tabpage_select_index(htab, index);
                            on_search_jmp_line(p, find_line, 0);
                            return SKYLARK_OK;
                        }
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
    if (pnode && !TAB_HEX_MODE(pnode) && !pnode->pmod)
    {
        const sptr_t line = eu_sci_call(pnode, SCI_GETLINECOUNT, 0, 0);
        *szmark = 0;
        while (find_line != LINE_NOT_FOUND && find_line <= line)
        {
            find_line = eu_sci_call(pnode, SCI_MARKERNEXT, current_line, MARGIN_BOOKMARK_MASKN);
            if (find_line >= 0)
            {
                offset = eu_int_cast(strlen(szmark));
                if (offset >= size)
                {
                    break;
                }
                _snprintf(szmark+offset, size-offset, "%zd;", find_line);
                current_line = find_line + 1;
            }
        }
    }
}

void
on_search_update_mark(eu_tabpage *pnode, char *szmark)
{
    if (pnode && !TAB_HEX_MODE(pnode) && !pnode->pmod)
    {
        char *p = strtok(szmark, ";");
        while (p)
        {
            on_search_add_mark(pnode, _atoz(p));
            p = strtok(NULL, ";");
        }
    }
}

void
on_search_fold_kept(eu_tabpage *pnode, char *szfold, int size)
{
    if (pnode && !TAB_HEX_MODE(pnode) && !pnode->pmod)
    {
        int offset = 0;
        sptr_t header_line = 0;
        *szfold = 0;
        do
        {
            header_line = eu_sci_call(pnode, SCI_CONTRACTEDFOLDNEXT, header_line, 0);
            if (header_line != LINE_NOT_FOUND)
            {
                offset = eu_int_cast(strlen(szfold));
                if (offset >= size)
                {
                    break;
                }
                _snprintf(szfold+offset, size-offset, "%zd;", header_line);
                ++header_line;
            }
        } while (header_line != LINE_NOT_FOUND);
    }
}

void
on_search_update_fold(eu_tabpage *pnode, char *szfold)
{
    if (pnode && !TAB_HEX_MODE(pnode) && !pnode->pmod)
    {
        char *p = strtok(szfold, ";");
        while (p)
        {
            sptr_t line = _atoz(p);
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

int
on_search_add_navigate_list(eu_tabpage *pnode, int64_t pos)
{
    struct navigate_trace *curr = NULL;
    if (!pnode || pnode->pmod)
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
        eu_logmsg("%s: memory allocation failed\n", __FUNCTION__);
        return EUE_OUT_OF_MEMORY;
    }
    curr->pnode = pnode;
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

static int
on_search_list_lenght(const eu_tabpage *pnode)
{
    int i = 0;
    if (pnode)
    {
        struct list_head *pos, *n;
        struct navigate_trace *curr = NULL;
        list_for_each_safe(pos, n, &list_trace)
        {
            curr = list_entry(pos, struct navigate_trace, ng_node);
            if (curr && curr->pnode == pnode)
            {
                ++i;
            }
        }
    }
    return i;
}

int
on_search_back_navigate_this(const eu_tabpage *pnode)
{
    int64_t pos = -1;
    eu_tabpage *p = NULL;
    struct navigate_trace *curr = NULL;
    struct navigate_trace *prev = NULL;
    if (pnode && !list_empty(&list_trace))
    {
        list_for_each_entry_safe_reverse(curr, prev, &list_trace, struct navigate_trace, ng_node)
        {
            if (curr->pnode != (eu_tabpage *)pnode)
            {
                continue;
            }
            if (curr->last_pos == (int64_t)eu_sci_call((eu_tabpage *)pnode, SCI_GETCURRENTPOS, 0, 0))
            {
                continue;
            }
            pos = curr->last_pos;
            p = curr->pnode;
            if (on_search_list_lenght(pnode) > 1)
            {
                list_del(&(curr->ng_node));
                free(curr);
                --max_nav_count;
            }
            if (p == pnode)
            {
                sptr_t text_len = eu_sci_call((eu_tabpage *)pnode, SCI_GETLENGTH, 0, 0);
                sptr_t go_pos = pos > text_len - 1 ? text_len - 1 : (sptr_t)(pos > 0 ? pos : 0);
                eu_sci_call((eu_tabpage *)pnode, SCI_GOTOPOS, go_pos, 0);
            }
            return SKYLARK_OK;
        }
    }
    return EUE_UNKOWN_ERR;
}

int
on_search_back_navigate_all(void)
{
    eu_tabpage *pnode = NULL;
    struct navigate_trace *curr = NULL;
    struct navigate_trace *prev = NULL;
    list_for_each_entry_safe_reverse(curr, prev, &list_trace, struct navigate_trace, ng_node)
    {
        if (!(pnode = on_tabpage_focused()))
        {
            return EUE_TAB_NULL;
        }
        if (curr->pnode != pnode)
        {
            on_tabpage_selection(curr->pnode);
            return SKYLARK_OK;
        }
        list_del(&(curr->ng_node));
        free(curr);
        --max_nav_count;
        if (&(prev->ng_node) != &list_trace)
        {
            if (prev->pnode != pnode)
            {
                on_tabpage_selection(prev->pnode);
            }
            sptr_t text_len = eu_sci_call(pnode, SCI_GETLENGTH, 0, 0);
            sptr_t go_pos = prev->last_pos > text_len - 1 ? text_len - 1 : (sptr_t)prev->last_pos;
            eu_sci_call(pnode, SCI_GOTOPOS, go_pos, 0);
        }
        return SKYLARK_OK;
    }
    return EUE_UNKOWN_ERR;
}

void
on_search_clean_navigate_this(eu_tabpage *pnode)
{
    if (pnode)
    {
        struct navigate_trace *curr = NULL;
        struct navigate_trace *next = NULL;
        list_for_each_entry_safe(curr, next, &list_trace, struct navigate_trace, ng_node)
        {
            if (curr->pnode == pnode)
            {
                list_del(&(curr->ng_node));
                free(curr);
                --max_nav_count;
            }
        }
    }
}

void
on_search_select_matching_all(eu_tabpage *pnode)
{
    if (pnode)
    {
        sptr_t i = 0;
        sptr_t len = 0;
        sptr_t total_len = 0;
        sptr_t current_pos = 0;
        cvector_vector_type(sci_range_t) v = NULL;
        sci_range_t pos = {eu_sci_call(pnode, SCI_GETSELECTIONSTART, 0, 0), eu_sci_call(pnode, SCI_GETSELECTIONEND, 0, 0)};
        if (pos.cpMax - pos.cpMin > 0)
        {
            sptr_t ins = 0;
            sptr_t diff = 0;
            current_pos = pos.cpMax;
            cvector_push_back(v, pos);
            total_len = eu_sci_call(pnode, SCI_GETTEXTLENGTH, 0, 0);
            eu_sci_call(pnode, SCI_SETINDICATORCURRENT, INDIC_SKYLARK_SELECT, 0);
            for (i = 1; i < total_len; ++i)
            {
                if ((ins = eu_sci_call(pnode, SCI_INDICATORVALUEAT, INDIC_SKYLARK_SELECT, i)) > 0)
                {
                    pos.cpMin = eu_sci_call(pnode, SCI_INDICATORSTART, INDIC_SKYLARK_SELECT, i);
                    pos.cpMax = eu_sci_call(pnode, SCI_INDICATOREND, INDIC_SKYLARK_SELECT, i);
                    if ((diff = pos.cpMax - pos.cpMin) > 0)
                    {
                        i += diff;
                        cvector_push_back(v, pos);
                    }
                }
            }
        }
        if ((len = (sptr_t)cvector_size(v)) > 1)
        {
            eu_sci_call(pnode, SCI_SETEMPTYSELECTION, current_pos, 0);
            eu_sci_call(pnode, SCI_INDICATORCLEARRANGE, 0, total_len);
            for (i = 1; i < len; ++i)
            {
                eu_sci_call(pnode, SCI_ADDSELECTION, v[i].cpMin, v[i].cpMax);
            }
            eu_sci_call(pnode, SCI_ADDSELECTION, v[0].cpMin, v[0].cpMax);
        }
        cvector_free(v);
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
            FillRect((HDC)wParam, &rc, (HBRUSH)on_dark_get_bgbrush());
            return 1;
        case WM_PAINT:
        {
            if (GetWindowLongPtr(hwnd, GWL_STYLE) & TCS_OWNERDRAWFIXED)
            {
                PAINTSTRUCT    ps;
                HDC hdc = BeginPaint(hwnd, & ps);
                HBRUSH hbr_bkgnd = (HBRUSH)on_dark_get_bgbrush();
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
                    SendMessage(hwnd_search_dlg, WM_COMMAND, MAKEWPARAM(IDC_SEARCH_CD_CHK, 0), (LPARAM)GetDlgItem(hwnd_search_dlg, IDC_SEARCH_CD_CHK));
                    SendMessage(hwnd_search_dlg, DM_SETDEFID, IDC_SEARCH_START_ENGINE, 0);
                    break;
                default:
                    break;
            }
            if (hwnd_what)
            {
                on_search_set_focus(on_tabpage_focused(), hwnd_what);
            }
            break;
        }
        case WM_THEMECHANGED:
        {
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
    eu_tabpage *pnode = on_tabpage_focused();
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

void
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
            if (util_os_version() < 603)
            {
                Static_SetText(hwnd_re_stc, _T(" ?"));
            }
            GetTextExtentPoint32(dc, str, eu_int_cast(wcslen(str)), &sz);
            SelectObject(dc, old);
            ReleaseDC(stc, dc);
            GetWindowRect(stc, &rc);
            MapWindowPoints(NULL, hwnd_search_dlg, (LPPOINT)&rc, 2);
            MoveWindow(hwnd_re_stc, rc.left + sz.cx + 8, rc.top, 12, rc.bottom - rc.top, TRUE);
            ShowWindow(hwnd_re_stc, SW_HIDE);
            ShowWindow(hwnd_re_stc, SW_SHOW);
            if ((ll_msg = on_search_regxp_msg()) != NULL)
            {
                util_create_tips(hwnd_re_stc, ll_msg, NULL);
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
    TCHAR str[MAX_BUFFER];
    SendMessage(hedit, WM_GETTEXT, MAX_BUFFER, (LPARAM)str);
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
on_search_get_combo_list(const HWND hcombo, const int index)
{
    char *text = NULL;
    TCHAR *buf = NULL;
    const int len = (const int)ComboBox_GetLBTextLen(hcombo, index);
    buf = len > 0 ? (TCHAR *)calloc(sizeof(TCHAR), len + 1) : NULL;
    if (buf)
    {
        ComboBox_GetLBText(hcombo, index, buf);
        text = eu_utf16_utf8(buf, NULL);
        free(buf);
    }
    return text;
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
on_search_combo_wnd(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR sub_id, DWORD_PTR dw)
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
            SelectObject(hdc, oldpen);
            SelectObject(hdc, oldbrush);
            DeleteObject(brush);
            DeleteObject(pen);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_NCDESTROY:
        {
            RemoveWindowSubclass(hwnd, on_search_combo_wnd, sub_id);
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
    size_t len = STR_NOT_NUL(key) ? strlen(key) : 0;
    return eu_sci_call(pnode, SCI_SEARCHINTARGET, len, (sptr_t)key);
}

static sptr_t
on_search_internal_find(eu_tabpage *p, const char *key, const bool sel)
{
    sptr_t pos = -1;
    size_t flags = on_search_build_flags(hwnd_search_dlg);
    size_t len = STR_NOT_NUL(key) ? strlen(key) : 0;
    if (p && len > 0)
    {
        eu_sci_call(p, SCI_SETSEARCHFLAGS, flags, 0);
        eu_sci_call(p, SCI_TARGETWHOLEDOCUMENT, 0, 0);
        if ((pos = eu_sci_call(p, SCI_SEARCHINTARGET, len, (sptr_t)key)) >= 0 && sel)
        {
            eu_sci_call(p, SCI_SETSEL, pos, eu_sci_call(p, SCI_GETTARGETEND, 0, 0));
        }
    }
    return pos;
}

static void
on_search_push_string_listbox(const TCHAR *path, const char *key, const intptr_t num)
{
    TCHAR *u16 = NULL;
    TCHAR msg_str[MAX_BUFFER] = {0};
    int index = ListBox_FindString(hwnd_found_box, -1, path);
    if ((u16 = eu_utf8_utf16(key, NULL)))
    {
        LOAD_I18N_RESSTR(IDC_MSG_SEARCH_STR2, format);
        _sntprintf(msg_str, MAX_BUFFER, format, path, u16, num);
        if (index >= 0)
        {
            ListBox_DeleteString(hwnd_found_box, index);
        }
        ListBox_AddString(hwnd_found_box, msg_str);
        free(u16);
    }
}

static void
on_search_node_init(eu_tabpage *pnode, const bool vec_clean)
{
    if (pnode)
    {   // 清空计算器
        pnode->match_count = 0;
        // 清空高亮标记数组
        if (vec_clean && pnode->ret_vec)
        {
            cvector_clear(pnode->ret_vec);
        }
    }
}

static int
on_search_msg_tips(const int redis)
{
    int msg = IDOK;
    MSG_BOX_SEL(redis, IDC_MSG_TIPS, MB_ICONSTOP | MB_OKCANCEL, msg);
    return msg;
}

static intptr_t
on_search_process_count(eu_tabpage *pnode, const char *key, const bool sel, const bool need_vec)
{
    sptr_t pos = 0;
    sptr_t end_pos = 0;
    size_t flags = on_search_build_flags(hwnd_search_dlg);
    size_t len = STR_NOT_NUL(key) ? strlen(key) : 0;
    if (!len)
    {
        return 0;
    }
    eu_sci_call(pnode, SCI_SETSEARCHFLAGS, flags, 0);
    if (sel)
    {
        eu_sci_call(pnode, SCI_TARGETFROMSELECTION, 0, 0);
        end_pos = eu_sci_call(pnode, SCI_GETSELECTIONEND, 0, 0);
    }
    else
    {
        eu_sci_call(pnode, SCI_TARGETWHOLEDOCUMENT, 0, 0);
        end_pos = eu_sci_call(pnode, SCI_GETTEXTLENGTH, 0, 0);
    }
    while (pos >= 0)
    {
        result_vec ret = {-1,};
        pos = eu_sci_call(pnode, SCI_SEARCHINTARGET, len, (sptr_t)key);
        if (pos >= 0)
        {
            sptr_t start_pos = eu_sci_call(pnode, SCI_GETTARGETEND, 0, 0);
            sptr_t line = eu_sci_call(pnode, SCI_LINEFROMPOSITION, pos, 0);
            if (need_vec && line >= 0)
            {
                ret.line = line;
            }
            ++pnode->match_count;
            if (pnode->match_count == RESULAT_MAX_MATCH)
            {
                if (need_vec)
                {
                    eu_logmsg("We only show %d results\n", RESULAT_MAX_MATCH);
                    break;
                }
                else if (on_search_msg_tips(IDS_RESULT_STR_TIPS) == IDOK)
                {
                    break;
                }
            }
            if (end_pos >= start_pos)
            {
                eu_sci_call(pnode, SCI_SETTARGETRANGE, start_pos, end_pos);
                if (need_vec)
                {
                    ret.mark.start = pos;
                    ret.mark.end = start_pos;
                    ret.mark._start = pos - eu_sci_call(pnode, SCI_POSITIONFROMLINE, line, 0);
                    if (ret.line >= 0)
                    {
                        cvector_push_back(pnode->ret_vec, ret);
                    }
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
            PostMessage(eu_module_hwnd(), WM_ABOUT_RE, 0, 0);
        }
    }
    return pnode->match_count;
}

static void
on_search_report_result(eu_tabpage *pnode, const int button)
{
    const HWND htab = on_tabpage_hwnd(pnode);
    if ((pnode || !TAB_HEX_MODE(pnode)) && htab)
    {
        int file_count = 0;
        sptr_t match_count = 0;
        eu_tabpage *p = NULL;
        bool all_file = DLG_BTN_CHECK(hwnd_search_dlg, IDC_MATCH_ALL_FILE);
        HWND hwnd_re_stc = GetDlgItem(hwnd_search_dlg, IDC_REGXP_TIPS_STC);
        if (hwnd_re_stc != NULL)
        {
            ShowWindow(hwnd_re_stc, SW_HIDE);
        }
        for (int index = 0, tabcount = TabCtrl_GetItemCount(htab); index < tabcount; ++index)
        {
            if ((p = on_tabpage_get_ptr(htab, index)) && p->match_count > 0)
            {
                match_count += p->match_count;
                ++file_count;
            }
        }
        switch (button)
        {
            case IDC_SEARCH_REALL_BTN:
            case IDC_SEARCH_SELRE_BTN:
            {
                if (!all_file)
                {
                    if (match_count > 0)
                    {
                        on_search_set_result(IDC_MSG_SEARCH_ERR11, -1, pnode->match_count);
                    }
                    else
                    {
                        on_search_set_result(IDC_MSG_SEARCH_ERR12, -1, -1);
                    }
                }
                else if (match_count > 0)
                {
                    on_search_set_result(IDC_MSG_SEARCH_ERR9, file_count, match_count);
                }
                else
                {
                    on_search_set_result(IDC_MSG_SEARCH_ERR10, file_count, -1);
                }
                break;
            }
            default:
                break;
        }
    }
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
    sptr_t found_pos = -1;
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
        if (start_pos > eu_sci_call(pnode, SCI_GETSELECTIONSTART, 0, 0))
        {
            start_pos = eu_sci_call(pnode, SCI_GETSELECTIONSTART, 0, 0);
        }
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
        if (found_pos >= 0)
        {
            target_end = eu_sci_call(pnode, SCI_GETTARGETEND, 0, 0);
            eu_sci_call(pnode, SCI_SETSEL, found_pos, target_end);
        }
        else if (found_pos == -2)
        {
            eu_logmsg("%s: re error\n", __FUNCTION__);
            PostMessage(eu_module_hwnd(), WM_ABOUT_RE, 0, 0);
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
        if (!(pmark = (char *) calloc(1, len + 1)))
        {
            return EUE_OUT_OF_MEMORY;
        }
        // 去除所有空格
        for (size_t c = 0, k = 0; c < len; ++c)
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
            on_search_set_tip(IDC_MSG_HEX_BYTE);
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
    }
    on_search_set_result(IDC_MSG_SEARCH_ERR14, 1, match);
    return match;
}

static bool
on_search_find_button(eu_tabpage *pnode, const char *dlg_text, const int button)
{
    bool result = false;
    bool reverse = false;
    HWND htab = on_tabpage_hwnd(pnode);
    if (STR_IS_NUL(dlg_text) || !htab)
    {
        return false;
    }
    if (button == IDC_SEARCH_PRE_BTN)
    {
        reverse = true;
    }
    if (TAB_HEX_MODE(pnode))
    {
        return on_search_hexview(pnode, dlg_text, reverse) > 0;
    }
    // 首先在当前页面查找
    result = on_search_at_page(pnode, dlg_text, reverse, button != IDC_SEARCH_ALL_BTN);
    if (button != IDC_SEARCH_ALL_BTN && DLG_BTN_CHECK(hwnd_search_dlg, IDC_MATCH_ALL_FILE) && !result)
    {
        int index = 0;
        int count = TabCtrl_GetItemCount(htab);
        int c_index = TabCtrl_GetCurSel(htab);
        if (c_index + 1 < count)
        {
            index = c_index + 1;
        }
        for (; index < count; ++index)
        {
            eu_tabpage *p = on_tabpage_get_ptr(htab, index);
            if (!p || p == pnode)
            {
                break;
            }
            if ((result = on_search_at_page(p, dlg_text, reverse, false)))
            {
                on_tabpage_select_index(htab, index);
                break;
            }
            else if (index == count - 1)
            {
                index = -1;
            }
        }
    }
    return result;
}

static int
on_search_max_line(eu_tabpage *pnode)
{
    result_line_width = 0;
    eu_tabpage *p = NULL;
    const HWND htab = on_tabpage_hwnd(pnode);
    for (int index = 0, count = TabCtrl_GetItemCount(htab); index < count; ++index)
    {
        if ((p = on_tabpage_get_ptr(htab, index)) && !TAB_HEX_MODE(p) && p->match_count > 0)
        {
            const int max_line = (const int)cvector_size(p->ret_vec);
            const int k = max_line > 0 ? util_count_number(p->ret_vec[max_line - 1].line + 1) : 0;
            if (k > result_line_width)
            {
                result_line_width = k;
            }
        }
    }
    return result_line_width;
}

static int
on_search_prefix_width(int line_width)
{
    int ret = 0;
    char *pformat = NULL;
    LOAD_I18N_RESSTR(IDS_RESULT_STRINGS2, line_str);
    if ((pformat = eu_utf16_utf8(line_str, NULL)))
    {
        ret = (int)(strlen(pformat) - strlen("%*zd %s") + line_width);
        free(pformat);
    }
    return ret;
}

static void
on_search_push_result(eu_tabpage *p, LPCTSTR key, LPCTSTR path, int orig_size)
{
    int k = 0;
    int max_line = 0;
    eu_tabpage *presult = NULL;
    eu_tabpage *pnode = NULL;
    if (!p || !key || !path)
    {
        return;
    }
    if (orig_size > 0)
    {
        pnode = on_tabpage_focused();
        presult = pnode ? pnode->presult : NULL;
        max_line = orig_size;
        k = result_line_width;
    }
    else if (orig_size == 0)
    {   // 按照最大行数右对齐
        max_line = (int)cvector_size(p->ret_vec);
        k = util_count_number(p->ret_vec[max_line - 1].line + 1);
        presult = p->presult;
    }
    else
    {
        pnode = on_tabpage_focused();
        presult = pnode ? pnode->presult : NULL;
        max_line = (int)cvector_size(p->ret_vec);
        k = result_line_width;
    }
    if (presult && k > 0)
    {
        char *ptr_add = NULL;
        char *pformat = NULL;
        char *u8_path = NULL;
        TCHAR msg[MAX_BUFFER] = {0};
        TCHAR path_str[ENV_LEN] = {0};
        if (p->match_count == RESULAT_MAX_MATCH)
        {
            eu_i18n_load_str(IDS_RESULT_STR_WARN, path_str, ENV_LEN);
        }
        else
        {
            eu_i18n_load_str(IDS_RESULT_STRINGS1, path_str, ENV_LEN);
        }
        LOAD_I18N_RESSTR(IDS_RESULT_STRINGS2, line_str);
        _sntprintf(msg, MAX_BUFFER-1, path_str, key, path, p->match_count);
        eu_sci_call(presult, SCI_SETREADONLY, 0, 0);
        if ((u8_path = eu_utf16_utf8(msg, NULL)))
        {   // 文件名称前面加换行符
            if (orig_size < 0)
            {
                sptr_t len = eu_sci_call(presult, SCI_GETLENGTH, 0, 0);
                int ch = (int)eu_sci_call(presult, SCI_GETCHARAT, len - 1, 0);
                if (ch != 0x0A && ch != 0x0D)
                {
                    eu_sci_call(presult, SCI_ADDTEXT, 1, (LPARAM)"\n");
                }
            }
            // 加入文件名
            eu_sci_call(presult, SCI_ADDTEXT, strlen(u8_path), (LPARAM)u8_path);
            free(u8_path);
        }
        if (p->ret_vec && (pformat = eu_utf16_utf8(line_str, NULL)))
        {
            for (int i = 0; i < max_line; ++i)
            {
                char *buf = util_strdup_line(p, p->ret_vec[i].line, NULL);
                if (buf)
                {
                    const size_t m_size = strlen(buf) + 24;
                    if ((ptr_add = (char *)calloc(1, m_size)) != NULL)
                    {
                        int len = snprintf(ptr_add, m_size - 1, pformat, k, p->ret_vec[i].line + 1, buf);
                        if (orig_size >= 0)
                        {   // 附加的行前缀长度
                            p->ret_vec[i].mark._no = len - eu_int_cast(strlen(buf)) - 1;
                        }
                        eu_sci_call(presult, SCI_ADDTEXT, len, (LPARAM)ptr_add);
                        free(ptr_add);
                    }
                    free(buf);
                }
            }
            if (!orig_size)
            {   // 编辑区设置成只读
               eu_sci_call(presult, SCI_SETREADONLY, 1, 0);
               eu_sci_call(presult, SCI_GOTOLINE, 1, 0);
            }
        }
    }
}

static void
on_search_launch_result_dlg(eu_tabpage *pnode, LPCTSTR path, LPCTSTR key, int orig_size)
{
    if (pnode && on_result_launch(pnode) && pnode->presult)
    {
        char ptr_style[16 + 1];
        // 显示底部窗口
        pnode->result_show = true;
        pnode->qrtable_show = false;
        // 关键字不高亮的回调函数
        pnode->presult->pwant = NULL;
        on_result_reload(pnode->presult);
        eu_window_resize();
        eu_sci_call(pnode->presult, SCI_SETREADONLY, 0, 0);
        eu_sci_call(pnode->presult, SCI_CLEARALL, 0, 0);
        sprintf(ptr_style, "%p", &pnode->ret_vec);
        eu_sci_call(pnode->presult, SCI_SETPROPERTY, (sptr_t)result_extra, (sptr_t)ptr_style);
        on_search_push_result(pnode, key, path, orig_size);
        // 窗口并排可能导致主编辑器之前的光标位置被遮挡
        // 滚动视图以使光标可见
        eu_sci_call(pnode, SCI_SCROLLCARET, 0, 0);
        ShowWindow(hwnd_search_dlg, SW_HIDE);
    }
}

static void
on_search_lookup_result(eu_tabpage *pnode, const bool all_file, const int count)
{
    int size = 0;
    int index = 0;
    char *ptr_add = NULL;
    TCHAR msg[MAX_BUFFER] = {0};
    TCHAR text[MAX_PATH+1]= {0};
    eu_tabpage *p = NULL;
    const HWND htab = on_tabpage_hwnd(pnode);
    HWND hwnd_cb = GetDlgItem(hwnd_search_dlg, IDC_WHAT_FOLDER_CBO);
    if (all_file)
    {
        int prefix_width = on_search_prefix_width(on_search_max_line(pnode));
        size = (int)cvector_size(pnode->ret_vec);
        for (; index < count; ++index)
        {
            p = on_tabpage_get_ptr(htab, index);
            if (p && !TAB_HEX_MODE(p) && p->match_count > 0 && p->ret_vec)
            {
                if (p != pnode)
                {
                    result_vec vec_t = {-1};
                    vec_t.mark._no = (intptr_t)p;
                    // 占位, 对应词法解析里的文件名
                    cvector_push_back(pnode->ret_vec, vec_t);
                    // 拷贝其他标签页的数据到当前页面
                    for (size_t j = 0; j < cvector_size(p->ret_vec); ++j)
                    {
                        p->ret_vec[j].mark._no = prefix_width;
                        memcpy(&vec_t, &p->ret_vec[j], sizeof(result_vec));
                        cvector_push_back(pnode->ret_vec, vec_t);
                    }
                }
            }
        }
    }
    ComboBox_GetText(hwnd_cb, text, MAX_PATH);
    on_search_launch_result_dlg(pnode, pnode->pathfile, text, size);
    if (all_file && pnode->presult)
    {
        for (index = 0; index < count; ++index)
        {
            eu_tabpage *p = on_tabpage_get_ptr(htab, index);
            if (p && !TAB_HEX_MODE(p) && p->match_count > 0)
            {
                if (p != pnode)
                {
                    on_search_push_result(p, text, p->pathfile, -1);
                }
            }
        }
        eu_sci_call(pnode->presult, SCI_SETREADONLY, 1, 0);
        eu_sci_call(pnode->presult, SCI_GOTOLINE, 1, 0);
    }
}

static void
on_search_find_error(eu_tabpage *pnode, const int button)
{
    sptr_t pos = -1;
    const char *key = on_search_get_combo_str(IDC_WHAT_FOLDER_CBO);
    if (pnode && STR_NOT_NUL(key))
    {
        pos = on_search_internal_find(pnode, key, false);
    }
    switch (button)
    {
        case IDC_SEARCH_PRE_BTN:
        {
            if (pos == -1)
            {
                on_search_set_result(IDC_MSG_SEARCH_ERR6, -1, -1);
            }
            else if (pos >= 0)
            {
                on_search_set_result(IDC_MSG_SEARCH_TOP, -1, -1);
            }
            break;
        }
        case IDC_SEARCH_NEXT_BTN:
        {
            if (pos == -1)
            {
                on_search_set_result(IDC_MSG_SEARCH_ERR6, -1, -1);
            }
            else if (pos >= 0)
            {
                on_search_set_result(IDC_MSG_SEARCH_BOTTOM, -1, -1);
            }
            break;
        }
        default:
            break;
    }
}

static void
on_search_find_next_button(const int button)
{
    eu_tabpage *pnode = on_tabpage_focused();
    const HWND htab = on_tabpage_hwnd(pnode);
    if (pnode && htab)
    {
        const bool all_file = DLG_BTN_CHECK(hwnd_search_dlg, IDC_MATCH_ALL_FILE);
        const int count = TabCtrl_GetItemCount(htab);
        const char *key = on_search_get_combo_str(IDC_WHAT_FOLDER_CBO);
        if (STR_NOT_NUL(key))
        {
            if (button != IDC_SEARCH_ALL_BTN)
            {
                if (on_search_find_button(pnode, key, button))
                {
                    if (!TAB_HEX_MODE(pnode))
                    {
                        sptr_t lineno = 0;
                        sptr_t row = 0;
                        sptr_t pos = eu_sci_call(pnode, SCI_GETSELECTIONSTART, 0, 0);
                        if (pos >= 0)
                        {
                            util_postion_xy(pnode, pos, &lineno, &row);
                            on_search_set_result(IDC_MSG_SEARCH_ERR5, lineno, row);
                        }
                    }
                }
                else if (!TAB_HEX_MODE(pnode))
                {
                    on_search_find_error(pnode, button);
                }
            }
            else
            {
                intptr_t match = 0;
                if (!all_file)
                {
                    on_search_node_init(pnode, true);
                    match = on_search_process_count(pnode, key, false, true);
                }
                else
                {
                    eu_tabpage *p = NULL;
                    for (int i = 0; i < count; ++i)
                    {
                        if ((p = on_tabpage_get_ptr(htab, i)))
                        {
                            on_search_node_init(p, true);
                            sptr_t c = on_search_process_count(p, key, false, true);
                            if (c > 0)
                            {
                                match += c;
                            }
                            else if (c == -2)
                            {
                                match = -2;
                                break;
                            }
                        }
                    }
                }
                if (match > 0)
                {
                    on_search_lookup_result(pnode, all_file, count);
                }
                else if (!match)
                {
                    on_search_set_result(IDC_MSG_SEARCH_ERR15, -1, -1);
                }
                eu_push_find_history(key);
            }
        }
        eu_safe_free(key);
    }
}

static void
on_search_show_listbox(const HWND hc, const bool visible, const int index)
{
    if (hc)
    {
        HWND hsub = NULL;
        if (visible)
        {
            if (index == 2)
            {
                CONTROL_HANDLE(hsub, IDC_SEARCH_SUB_CHK, SW_SHOW)
                CONTROL_HANDLE(hsub, IDC_SEARCH_HIDE_CHK, SW_SHOW)
                CONTROL_HANDLE(hsub, IDC_SEARCH_UTF8_CHK, SW_SHOW)
                if (search_btn_id)
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
}

static intptr_t
on_search_detail_listbox(eu_tabpage *pnode, const char *key)
{
    intptr_t match = 0;
    if (pnode && key)
    {
        on_search_node_init(pnode, false);
        if ((match = on_search_process_count(pnode, key, false, false)) > 0)
        {
            on_search_push_string_listbox(pnode->pathfile, key, match);
        }
    }
    return match;
}

static void
on_search_detail_button(void)
{
    eu_tabpage *pnode = on_tabpage_focused();
    const HWND htab = on_tabpage_hwnd(pnode);
    const HWND hc = GetDlgItem(hwnd_search_dlg, IDC_SEARCH_FOUNDLIST);
    if (pnode && htab && hc)
    {
        intptr_t match = 0;
        const char *find_str = on_search_get_combo_str(IDC_WHAT_FOLDER_CBO);
        const bool all_file = DLG_BTN_CHECK(hwnd_search_dlg, IDC_MATCH_ALL_FILE);
        const bool visible = (GetWindowLongPtr(hc, GWL_STYLE) & WS_VISIBLE);
        const HWND hwnd_tab = GetDlgItem(hwnd_search_dlg, IDD_SEARCH_TAB_1);
        const int index = TabCtrl_GetCurSel(hwnd_tab);
        if (!index && !visible && STR_NOT_NUL(find_str))
        {
            ListBox_ResetContent(hwnd_found_box);
            if (all_file)
            {
                eu_tabpage *p = NULL;
                for (int i = 0, tabcount = TabCtrl_GetItemCount(htab); i < tabcount; ++i)
                {
                    if ((p = on_tabpage_get_ptr(htab, i)))
                    {
                        sptr_t c = on_search_detail_listbox(p, find_str);
                        if (c > 0)
                        {
                            match += c;
                        }
                        else if (c == -2)
                        {
                            match = -2;
                            break;
                        }
                    }
                }
            }
            else
            {
                match = on_search_detail_listbox(pnode, find_str);
            }
        }
        if (match > 0 || visible || index)
        {
            on_search_show_listbox(hc, visible, index);
        }
        else if (!match)
        {
            on_search_set_result(IDC_MSG_SEARCH_ERR15, -1, -1);
        }
    }
}

static int
on_search_active_tab(const TCHAR *path, const TCHAR *key)
{
    int tab_find = EUE_TAB_NULL;
    eu_tabpage *p = NULL;
    HWND htab[2] = {HMAIN_GET, HMAIN_SHOW ? HSLAVE_GET : NULL};
    if (!(path && key))
    {
        return EUE_POINT_NULL;
    }
    for (int k = 0; k < 2 && htab[k]; ++k)
    {
        for (int index = 0, count = TabCtrl_GetItemCount(htab[k]); index < count; ++index)
        {
            if ((p = on_tabpage_get_ptr(htab[k], index)) && _tcscmp(p->pathfile, path) == 0)
            {
                char *u8_key = eu_utf16_utf8(key, NULL);
                on_tabpage_select_index(htab[k], index);
                on_search_internal_find(p, u8_key, true);
                free(u8_key);
                tab_find = index;
                break;
            }
        }
    }
    if (tab_find < 0)
    {
        file_backup bak = {0};
        tab_find = TabCtrl_GetItemCount(htab[0]);
        bak.tab_id = tab_find < 0 ? 0 : tab_find;
        _tcsncpy(bak.rel_path, path, _countof(bak.rel_path));
        tab_find = bak.tab_id;
        if (on_file_redirect(&bak, 1) == SKYLARK_OK && (p = on_tabpage_get_ptr(htab[0], tab_find)))
        {
            char *u8_key = eu_utf16_utf8(key, NULL);
            if (u8_key)
            {
                on_search_internal_find(p, u8_key, true);
                free(u8_key);
            }
        }
    }
    return tab_find;
}

static void
on_search_found_list(HWND hwnd)
{
    int listno = (int)SendMessage(hwnd, LB_GETCURSEL ,0 , 0);
    int buf_len = ListBox_GetTextLen(hwnd, listno);
    TCHAR *buf = (TCHAR *)calloc(sizeof(TCHAR), buf_len + 1);
    if (buf)
    {
        ListBox_GetText(hwnd, listno, buf);
        if (*buf)
        {
            TCHAR path[MAX_PATH+1] = {0};
            TCHAR key[MAX_PATH+1] = {0};
            if (_stscanf(buf, _T("%260[^|]"), path) == 1 &&
                _stscanf(buf, _T("%*[^|]|%260[^|]"), key) == 1)
            {
                int tab = on_search_active_tab(path, key);
            }
        }
        free(buf);
    }
}

static bool
on_search_replace_target(eu_tabpage *pnode, const char *replace_str, sptr_t *poffset)
{
    sptr_t re_len = 0;
    eu_sci_call(pnode, SCI_TARGETFROMSELECTION, 0, 0);
    sptr_t target_start = eu_sci_call(pnode, SCI_GETTARGETSTART, 0, 0);
    sptr_t target_end = eu_sci_call(pnode, SCI_GETTARGETEND, 0, 0);
    if (eu_sci_call(pnode, SCI_GETSEARCHFLAGS, 0, 0) & SCFIND_REGEXP)
    {
        re_len = eu_sci_call(pnode, SCI_REPLACETARGETRE, (WPARAM)-1, (LPARAM)replace_str);
    }
    else
    {
        re_len = eu_sci_call(pnode, SCI_REPLACETARGET, (WPARAM)-1, (LPARAM)replace_str);
    }
    if (re_len >= 0)
    {
        if (poffset)
        {
            *poffset = re_len - target_end + target_start;
            target_end += *poffset;
        }
        ++pnode->match_count;
        eu_sci_call(pnode, SCI_GOTOPOS, target_end, 0);
    }
    return (re_len >= 0);
}

static bool
on_search_common(eu_tabpage *pnode, const char *key, int opt)
{
    sptr_t found_pos = -1;
    bool m_loop = false;
    sptr_t pos = 0;
    sptr_t end_pos = 0;
    sptr_t start_pos = 0;
    size_t find_flags = on_search_build_flags(hwnd_search_dlg);
    if (opt & ON_REPLACE_SELECTION)
    {
        pos = eu_sci_call(pnode, SCI_GETSELECTIONSTART, 0, 0);
        end_pos = eu_sci_call(pnode, SCI_GETSELECTIONEND, 0, 0);
    }
    else
    {
        pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
        end_pos = eu_sci_call(pnode, SCI_GETLENGTH, 0, 0);
    }
    m_loop = (opt & ON_REPLACE_SELECTION) ? false : DLG_BTN_CHECK(hwnd_search_dlg, IDC_MATCH_LOOP);
    if ((opt & ON_REPLACE_ALL)  || (opt & ON_OTHER_PAGE))
    {
        m_loop = false;
        start_pos = 0;
    }
    else
    {
        start_pos = pos;
        if (!(opt & ON_REPLACE_SELECTION))
        {
            pos = eu_sci_call(pnode, SCI_GETSELECTIONSTART, 0, 0);
            if (start_pos > pos)
            {
                start_pos = pos;
            }
        }
    }
DO_COMMON_LOOP:
    if (end_pos > start_pos)
    {
        found_pos = on_search_process_find(pnode, key, start_pos, end_pos, find_flags);
        if (found_pos >= 0)
        {
            sptr_t target_end = eu_sci_call(pnode, SCI_GETTARGETEND, 0, 0);
            eu_sci_call(pnode, SCI_SETSELECTION, found_pos, target_end);
        }
        else if (found_pos == -2)
        {
            eu_logmsg("%s: re error\n", __FUNCTION__);
            PostMessage(eu_module_hwnd(), WM_ABOUT_RE, 0, 0);
            return false;
        }
        else if (m_loop)
        {
            m_loop = false;
            start_pos = 0;
            goto DO_COMMON_LOOP;
        }
    }
    return (found_pos >= 0);
}

static bool
on_search_first(eu_tabpage *pnode, const char *key, int opt)
{
    return on_search_common(pnode, key, opt);
}

static bool
on_search_next(eu_tabpage *pnode, const char *key, const sptr_t end_pos)
{
    sptr_t found_pos = -1;
    sptr_t start_pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
    size_t find_flags = on_search_build_flags(hwnd_search_dlg);
    if ((found_pos = on_search_process_find(pnode, key, start_pos, end_pos, find_flags)) >= 0)
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
    sptr_t offset = 0;
    sptr_t end_pos = 0;
    char *replace_str = on_search_get_combo_str(IDC_SEARCH_RP_CBO);
    char *find_str = on_search_get_combo_str(IDC_WHAT_FOLDER_CBO);
    if (!(pnode && find_str))
    {
        eu_logmsg("%s: point error\n", __FUNCTION__);
        return false;
    }
    if (!replace_str && !(replace_str = _strdup("")))
    {
        free(find_str);
        return false;
    }
    if (opt & ON_REPLACE_SELECTION)
    {
        end_pos = eu_sci_call(pnode, SCI_GETSELECTIONEND, 0, 0);
    }
    on_search_node_init(pnode, false);
    if (on_search_first(pnode, find_str, opt))
    {
        eu_sci_call(pnode, SCI_BEGINUNDOACTION, 0, 0);
        do
        {
            result = on_search_replace_target(pnode, replace_str, &offset);
            if (result)
            {
                if (opt & ON_REPLACE_SELECTION)
                {
                    end_pos += offset;
                }
                else
                {
                    end_pos = eu_sci_call(pnode, SCI_GETLENGTH, 0, 0);
                }
                next_result = on_search_next(pnode, find_str, end_pos);
            }
        } while (!(opt & ON_REPLACE_THIS) && result && next_result);
        eu_sci_call(pnode, SCI_ENDUNDOACTION, 0, 0);
    }
    if (*find_str)
    {
        eu_push_find_history(find_str);
    }
    if (*replace_str)
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
    eu_tabpage *p = NULL;
    eu_tabpage *pnode = on_tabpage_focused();
    const HWND htab = on_tabpage_hwnd(pnode);
    if (!htab)
    {
        return;
    }
    if (!DLG_BTN_CHECK(hwnd_search_dlg, IDC_MATCH_ALL_FILE))
    {
        result = on_search_at_replace_page(pnode, opt);
    }
    else
    {   // 首先在当前页面查找
        result = on_search_at_replace_page(pnode, opt);
        if (result)
        {
            return;
        }
        // 标记当前页面
        c_index = TabCtrl_GetCurSel(htab);
        // 在所有页面查找
        if (!(opt & ON_OTHER_PAGE))
        {
            opt |= ON_OTHER_PAGE;
        }
        for (int index = 0, count = TabCtrl_GetItemCount(htab); index < count; ++index)
        {
            if (index == c_index)
            {
                continue;
            }
            if ((p = on_tabpage_get_ptr(htab, index)) && (result = on_search_at_replace_page(p, opt)))
            {
                on_tabpage_select_index(htab, index);
                break;
            }
        }
    }
    if (result)
    {
        on_search_set_result(IDC_MSG_REPLACE_ONECE, -1, -1);
    }
    else
    {
        on_search_set_result(IDC_MSG_REPLACE_NONE, -1, -1);
    }
}

static void
on_search_replace_all_button(const int button)
{
    int opt = 0;
    eu_tabpage *pnode = on_tabpage_focused();
    const HWND htab = on_tabpage_hwnd(pnode);
    if (!htab)
    {
        return;
    }
    if (button == IDC_SEARCH_REALL_BTN)
    {
        opt |= ON_REPLACE_ALL;
    }
    else if (button == IDC_SEARCH_SELRE_BTN)
    {
        opt |= ON_REPLACE_SELECTION;
    }
    if (!DLG_BTN_CHECK(hwnd_search_dlg, IDC_MATCH_ALL_FILE))
    {
        on_search_at_replace_page(pnode, opt);
    }
    else // replaces on all page
    {
        eu_tabpage *p = NULL;
        for (int index = 0, count = TabCtrl_GetItemCount(htab); index < count; ++index)
        {
            if ((p = on_tabpage_get_ptr(htab, index)) != NULL)
            {
                if (button != IDC_SEARCH_SELRE_BTN)
                {   // clear all setletction
                    eu_sci_call(p, SCI_SETEMPTYSELECTION, 0, 0);
                }
                on_search_at_replace_page(p, opt);
            }
        }
    }
    on_search_report_result(pnode, button);
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

static int
on_search_folder_browser(void)
{
    TCHAR *path = NULL;
    LPITEMIDLIST pidl = NULL;
    eu_tabpage *pnode = on_tabpage_focused();
    if (!pnode)
    {
        return 1;
    }
    if (!(path = (TCHAR *)calloc(sizeof(TCHAR), MAX_BUFFER)))
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
        _sntprintf(pfile->path, MAX_BUFFER, _T("%s%s"), path, name);
    }
    else
    {
        _sntprintf(pfile->path, MAX_BUFFER, _T("%s\\%s"), path, name);
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
    _sntprintf(folderstart->dir, MAX_BUFFER, _T("%s\\*"), parent);
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
                    TCHAR sub[MAX_BUFFER] = {0};
                    _sntprintf(sub, MAX_BUFFER, _T("%s"), folderlast->dir);
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
                    _sntprintf(folderstart->dir, MAX_BUFFER, _T("%s\\%s\\*"), sub, fd.cFileName);
                    list_add_tail(&(folderstart->node_folder), &list_folders);
                }
            }   /* 后缀名是否匹配 */
            else
            {
                p = _tcsrchr(fd.cFileName, _T('.'));
                if ((p && _tcsicmp(p, type) == 0) || type[_tcslen(type)-1] == _T('*'))
                {
                    TCHAR sub[MAX_BUFFER] = {0};
                    _sntprintf(sub, MAX_BUFFER, _T("%s"), folderlast->dir);
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
on_search_change_button(int res_id)
{
    int ret = 0;
    TCHAR lpch[DW_SIZE+1] = { 0 };
    HWND h_btn = GetDlgItem(hwnd_search_dlg, IDC_SEARCH_START_ENGINE);
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
    TCHAR filetype[DW_SIZE+1] = {0};
    TCHAR text[MAX_BUFFER] = {0};
    TCHAR result_str[MAX_PATH+1] = {0};
    uint64_t file_count = 0;
    uint64_t key_match = 0;
    bool whole = DLG_BTN_CHECK(hwnd_search_dlg, IDC_MATCH_WORD);
    bool incase = !DLG_BTN_CHECK(hwnd_search_dlg, IDC_MATCH_CASE);
    bool inpcre = DLG_BTN_CHECK(hwnd_search_dlg, IDC_MODE_REGEXP);
    HWND hwnd_type = GetDlgItem(hwnd_search_dlg, IDC_SEARCH_FY_CBO);
    char *u8_folder = on_search_get_combo_str(IDC_SEARCH_DIR_CBO);
    ComboBox_GetText(hwnd_type, filetype, DW_SIZE);
    if (!filetype[0])
    {
        goto res_clean;
    }
    hwnd_cb = GetDlgItem(hwnd_search_dlg, IDC_SEARCH_DIR_CBO);
    ComboBox_GetText(hwnd_cb, text, MAX_BUFFER);
    if (!text[0])
    {
        goto res_clean;
    }
    flags = on_search_folder_flags(hwnd_search_dlg);
    if (!(u8_key = on_search_get_combo_str(IDC_WHAT_FOLDER_CBO)))
    {
        eu_logmsg("%s: key not exist\n", __FUNCTION__);
        goto res_clean;
    }
    if (flags & INCLUDE_FILE_UTF8)
    {
        key = _strdup(u8_key);
    }
    else
    {   // 使用本地代码页
        key = eu_utf8_mbcs(CP_ACP, u8_key, NULL);
    }
    CONTROL_HANDLE(hwnd_stc, IDC_PGB_STC, SW_SHOW)
    CONTROL_HANDLE(hwnd_pgb, IDC_PGB1, SW_SHOW)
    SendMessage(hwnd_pgb, PBM_SETPOS, 0, 0);
    count = on_search_folder_files(text, &filetype[1], flags);
    if (!count)
    {
        goto res_clean;
    }
    pos = (float)(100.0/count);
    if ((data = (uint8_t *) calloc(1, BUFF_8M+1)) == NULL)
    {
        goto res_clean;
    }
    list_for_each_entry(pfile, &list_files, file_trace, node_file)
    {
        // 窗口关闭时, 保证线程退出
        PeekMessage(&msg, NULL,  0, 0, PM_REMOVE);
        if (msg.message == WM_QUIT)
        {
            eu_logmsg("%s: we recv WM_QUIT\n", __FUNCTION__);
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
        while ((readlen = fread((char *) data, 1, BUFF_8M, fp)) > 0)
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
    _InterlockedExchange(&search_btn_id, 0);
    // 有信号时, 代表搜索结束
    SetEvent(search_event_final);
    on_search_change_button(IDC_SEARCH_BTN_ON);
    eu_logmsg("%s: on_search_file thread exit\n", __FUNCTION__);
    return 0;
}

void
on_search_finish_wait(void)
{
    if (search_btn_id)
    {   // we destroy windows, that it should wait for search thread exit
        PostThreadMessage(search_btn_id, WM_QUIT, 0, 0);
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
        hwnd_search_dlg = NULL;
        on_search_create_box();
    }
}

static void
on_search_dark_mode_init(HWND hdlg, bool dark)
{
    int id;
    HWND btn = NULL;
    on_dark_allow_window(hdlg, dark);
    on_dark_refresh_titlebar(hdlg);
    const int cbo_buttons[] = { IDC_WHAT_FOLDER_CBO,
                                IDC_SEARCH_RP_CBO,
                                IDC_SEARCH_FY_CBO,
                                IDC_SEARCH_DIR_CBO
                              };
    if (dark)
    {
        for (id = 0; id < _countof(cbo_buttons); ++id)
        {
            if ((btn = GetDlgItem(hdlg, cbo_buttons[id])))
            {
                SetWindowSubclass(btn, on_search_combo_wnd, SEARCH_COMBO_SUBID, 0);
            }
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
                            IDC_SEARCH_ALL_BTN,
                            IDC_SEARCH_SELRE_BTN,
                            IDC_SEARCH_CLOSE_BTN,
                            IDC_SEARCH_RE_BTN,
                            IDC_SEARCH_REALL_BTN,
                            IDC_SEARCH_START_ENGINE,
                            IDC_SEARCH_COUNT_BTN,
                            IDC_SEARCH_FOUNDLIST,
                            IDD_SEARCH_TAB_1
                           };
    for (id = 0; id < _countof(buttons); ++id)
    {
        if ((btn = GetDlgItem(hdlg, buttons[id])))
        {
            on_dark_allow_window(btn, dark);
            on_dark_set_theme(btn, L"Explorer", NULL);
        }
    }
    UpdateWindowEx(hdlg);
}

static void
on_search_save_state(HWND hdlg)
{
    const btn_state bs[] =
    {
        {IDC_MATCH_ALL_FILE, ON_REPLACE_ALL},
        {IDC_MATCH_LOOP, ON_LOOP_FLAGS},
        {IDC_MATCH_WDSTART, SCFIND_WORDSTART},
        {IDC_MATCH_WORD, SCFIND_WHOLEWORD},
        {IDC_MATCH_CASE, SCFIND_MATCHCASE},
        {IDC_SEARCH_CD_CHK, INCLUDE_CURRENT_FOLDER},
        {IDC_SEARCH_SUB_CHK, INCLUDE_FOLDER_SUB},
        {IDC_SEARCH_HIDE_CHK, INCLUDE_FOLDER_HIDDEN},
        {IDC_SEARCH_UTF8_CHK, INCLUDE_FILE_UTF8},
        {IDC_SEARCH_HEX_STRINGS, ON_HEX_STRINGS},
        {IDC_MODE_NORMAL, NO_REGXP_FLAGS},
        {IDC_MODE_REGEXP, SCFIND_REGEXP}
    };
    if (eu_get_config()->last_flags == (uint32_t)-1)
    {
        eu_get_config()->last_flags = 0x44;
    }
    for (int i = 0; i < _countof(bs); ++i)
    {
        if (DLG_BTN_CHECK(hwnd_search_dlg, bs[i].id))
        {
            eu_get_config()->last_flags |= bs[i].mask;
        }
        else
        {
            eu_get_config()->last_flags &= ~bs[i].mask;
        }
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
                        result_line_width = 0;
                        ShowWindow(hdlg, SW_HIDE);
                        break;
                    }
                    case IDC_SEARCH_NEXT_BTN:
                    case IDC_SEARCH_PRE_BTN:
                    case IDC_SEARCH_ALL_BTN:
                    {
                        on_search_set_tip(IDC_MSG_SEARCH_STR1);
                        on_search_find_next_button((const int)(LOWORD(wParam)));
                        break;
                    }
                    case IDC_SEARCH_RE_BTN:
                    {
                        on_search_replace_button();
                        break;
                    }
                    case IDC_SEARCH_REALL_BTN:
                    {
                        on_search_replace_all_button(IDC_SEARCH_REALL_BTN);
                        break;
                    }
                    case IDC_SEARCH_SELRE_BTN:
                    {
                        on_search_replace_all_button(IDC_SEARCH_SELRE_BTN);
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
                            HWND rexp = GetDlgItem(hdlg, IDC_MODE_REGEXP);
                            HWND hwnd_re_stc = GetDlgItem(hdlg, IDC_REGXP_TIPS_STC);
                            EnableWindow(hwnd_word, true);
                            EnableWindow(hwnd_wdst, true);
                            EnableWindow(hwnd_case, true);
                            EnableWindow(rexp, true);
                            ShowWindow(hwnd_re_stc, SW_HIDE);
                            on_search_set_tip(IDC_MSG_SEARCH_STR1);
                            eu_tabpage *pnode = on_tabpage_focused();
                            if (pnode && TAB_HEX_MODE(pnode))
                            {
                                Button_SetCheck(rexp, BST_UNCHECKED);
                                EnableWindow(rexp, false);
                                EnableWindow(hwnd_case, false);
                            }
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
                        else if ((pnode = on_tabpage_focused()))
                        {
                            if (pnode->pathname[0] && eu_exist_dir(pnode->pathname))
                            {
                                on_search_set_folder_path(pnode->pathname);
                            }
                            else
                            {
                                TCHAR home_path[MAX_PATH+1] = {0};
                                uint32_t len = GetEnvironmentVariable(_T("USERPROFILE"), home_path, MAX_PATH);
                                if (len > 0 && len < MAX_PATH)
                                {
                                    on_search_set_folder_path(home_path);
                                }
                            }
                        }
                        break;
                    }
                    case IDC_SEARCH_START_ENGINE:
                    {
                        if (!search_btn_id)
                        {
                            if (!search_event_final)
                            {
                                search_event_final = CreateEvent(NULL, FALSE, FALSE, NULL);
                            }
                            else
                            {
                                ResetEvent(search_event_final);
                            }   // 开始搜索时, 信号量重置
                            ListBox_ResetContent(hwnd_found_box);
                            CloseHandle((HANDLE) _beginthreadex(NULL, 0, on_search_find_all_button, NULL, 0, (DWORD *)&search_btn_id));
                            on_search_change_button(IDC_SEARCH_BTN_OFF);
                        }
                        else
                        {
                            eu_logmsg("one search thread runing...\n");
                            PostThreadMessage(search_btn_id, WM_QUIT, 0, 0);
                            on_search_change_button(IDC_SEARCH_BTN_ON);
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
            else
            {
                on_search_save_state(hdlg);
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
                on_search_dark_mode_init(hdlg, on_dark_enable());
            }
            break;
        }
    }
    return 0;
}

void
on_search_do_space(eu_tabpage *pnode, const char *key, const char *str_replace, replace_event docase)
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
        int re_len = 0;
        char str[8] = {0};
        sptr_t target_end = eu_sci_call(pnode, SCI_GETTARGETEND, 0, 0);
        eu_sci_call(pnode, SCI_SETSELECTION, fpos, target_end);
        if (*str_replace != 0)
        {
            re_len = (int)eu_sci_call(pnode, SCI_REPLACETARGETRE, (WPARAM)-1, (LPARAM)str_replace);
        }
        else if (docase == FULL_HALF)
        {
            eu_sci_call(pnode, SCI_GETTAG, 1, (sptr_t)str);
            if (strlen(str) != 3)
            {
                break;
            }
            int old = (str[0] & 0xF) << 12 | ((str[1] & 0x3F) << 6 | (str[2] & 0x3F));
            if (old == 0x3000)
            {   // 全角空格转半角
                _snprintf(str, QW_SIZE, "\\x{%d}", 20);
            }
            else
            {   // 全角英文字符或标点符号转半角
                _snprintf(str, QW_SIZE, "\\x{%x}", old - 0xFEE0);
            }
            re_len = (int)eu_sci_call(pnode, SCI_REPLACETARGETRE, (WPARAM)-1, (LPARAM)str);
        }
        else if (docase == HALF_FULL)
        {
            eu_sci_call(pnode, SCI_GETTAG, 1, (sptr_t)str);
            if (*str == 0)
            {
                break;
            }
            if (str[0] == 0x20)
            {   // 半角空格转全角
                _snprintf(str, QW_SIZE, "\\x{%d}", 3000);
            }
            else
            {   // 半角标点符号转全角, 不包含英文字符
                _snprintf(str, QW_SIZE, "\\x{%x}", str[0] + 0xFEE0);
            }
            re_len = (int)eu_sci_call(pnode, SCI_REPLACETARGETRE, (WPARAM)-1, (LPARAM)str);
        }
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
on_search_repalce_event(eu_tabpage *p, replace_event docase)
{
    int number = 0;
    char key[QW_SIZE] = {0};
    char str_replace[QW_SIZE] = {0};
    cvector_vector_type(int) v = NULL;
    HWND htab = on_tabpage_hwnd(p);
    if (htab && (on_tabpage_sel_number(htab, &v, false)) > 0)
    {
        int count = eu_int_cast(cvector_size(v));
        for (int k = 0; k < count; ++k)
        {
            eu_tabpage *pnode = on_tabpage_get_ptr(htab, v[k]);
            if (pnode && !TAB_HEX_MODE(pnode) && !pnode->plugin)
            {
                if (pnode->doc_ptr && pnode->doc_ptr->tab_width > 0)
                {
                    number = pnode->doc_ptr->tab_width;
                }
                else
                {
                    number = eu_get_config()->tab_width;
                }
                if (number < QW_SIZE)
                {
                    switch (docase)
                    {
                        case FULL_HALF:
                        {
                            _snprintf(key, QW_SIZE, "%s", "([\\x{FF01}-\\x{FF5E}]|\\x{3000})");
                            eu_logmsg("key = /%s/\n", key);
                            break;
                        }
                        case HALF_FULL:
                        {
                            _snprintf(key, QW_SIZE, "%s", "([\\x{20}-\\x{2F}|\\x{3A}-\\x{40}|\\x{5B}-\\x{60}|\\x{7B}-\\x{7E}])");
                            eu_logmsg("key = /%s/\n", key);
                            break;
                        }
                        case TAB_SPACE:
                        {
                            key[0] = '\t';
                            key[1] = 0;
                            memset(str_replace, 0x20, number);
                            str_replace[number] = 0;
                            break;
                        }
                        case SPACE_TAB:
                        {
                            memset(key, 0x20, number);
                            key[number] = 0;
                            str_replace[0] = '\t';
                            str_replace[1] = 0;
                            break;
                        }
                        default:
                            break;
                    }
                    if (strlen(key) > 0)
                    {
                        on_search_do_space(pnode, key, str_replace, docase);
                    }
                }
            }
        }
    }
    cvector_freep(&v);
}

bool
on_search_create_box(void)
{
    if (!hwnd_search_dlg)
    {
        if (!(hwnd_search_dlg = i18n_create_dialog(eu_module_hwnd(), IDD_SEARCH_TAB_DLG, on_search_orig_find_proc)))
        {
            eu_logmsg("%s: i18n_create_dialog return false\n", __FUNCTION__);
            return false;
        }
    }
    ShowWindow(hwnd_search_dlg, SW_HIDE);
    return (true);
}

HWND
eu_get_search_hwnd(void)
{
    return hwnd_search_dlg;
}
