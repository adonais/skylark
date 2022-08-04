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

static HWND hwnd_snippet = NULL;

static LRESULT CALLBACK
on_snippet_edit_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    return CallWindowProc((WNDPROC)eu_edit_wnd, hwnd, message, wParam, lParam);
}

void WINAPI
on_snippet_reload(eu_tabpage *pedit)
{
    if (pedit)
    {
        on_sci_init_style(pedit);
        // disable margin
        eu_sci_call(pedit, SCI_SETMARGINWIDTHN, MARGIN_LINENUMBER_INDEX, 0);
        eu_sci_call(pedit, SCI_SETMARGINWIDTHN, MARGIN_BOOKMARK_INDEX, 0);
        eu_sci_call(pedit, SCI_SETMARGINWIDTHN, MARGIN_FOLD_INDEX, MARGIN_FOLD_WIDTH);
        // 强制启用自动换行
        eu_sci_call(pedit, SCI_SETWRAPMODE, 2, 0);
        // 启用语法解析与配色方案
        on_doc_init_after_scilexer(pedit, "eu_demo");
        on_doc_default_light(pedit, SCE_DEMO_CARETSTART, 0xFF8000, -1, true);
        on_doc_default_light(pedit, SCE_DEMO_MARKNUMBER, 0x00FF8000, -1, true);
        on_doc_default_light(pedit, SCE_DEMO_MARK0, 0x0000FF, -1, true);
    }
}

static LRESULT CALLBACK 
on_snippet_edt_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR sub_id, DWORD_PTR dwRefData)
{
    switch(msg) 
    {
        case WM_LBUTTONDOWN:
        {
            TCHAR str[MAX_PATH] = {0};
            LOAD_I18N_RESSTR(IDS_SNIPPET_EDT_DEFAULT, edt_str);
            Edit_GetText(hwnd, str, MAX_PATH-1);
            if (_tcscmp(edt_str, str) == 0)
            {
                SendMessage(hwnd, EM_SETSEL, 0, -1);
                SendMessage(hwnd, WM_CLEAR, 0, 0);
            }
            break;
        }
        case WM_NCDESTROY:
            RemoveWindowSubclass(hwnd, on_snippet_edt_proc, sub_id);
            break;
        default:
            break;
    }
    return DefSubclassProc(hwnd, msg, wp, lp);
}

static LRESULT CALLBACK 
on_snippet_cmb_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR sub_id, DWORD_PTR dwRefData)
{
    switch(msg) 
    {
        case WM_PAINT:
        {
            RECT rc;
            if (!on_dark_enable())
            {
                break;
            }
            GetClientRect(hwnd, &rc);
            PAINTSTRUCT ps;
            const HDC hdc = BeginPaint(hwnd, &ps);
            on_remotefs_draw_combo(hwnd, hdc, rc);
            EndPaint(hwnd, &ps);
            return 0;
        }        
        case WM_NCDESTROY:
            RemoveWindowSubclass(hwnd, on_snippet_cmb_proc, sub_id);
            break;
        default:
            break;
    }
    return DefSubclassProc(hwnd, msg, wp, lp);
}

static void
on_snippet_init_sci(eu_tabpage *pview)
{
    if (pview)
    {
        char *u8_str = NULL;
        LOAD_I18N_RESSTR(IDS_SNIPPET_EXAMPLE_DEC, sc_str);
        if ((u8_str = eu_utf16_utf8(sc_str, NULL)))
        {
            eu_sci_call(pview, SCI_CLEARALL, 0, 0);
            eu_sci_call(pview, SCI_ADDTEXT, strlen(u8_str), (sptr_t)u8_str);
            free(u8_str);
        }
        eu_sci_call(pview, SCI_SETSAVEPOINT, 0, 0);
    }
}

static void
on_snippet_init_edt(HWND hwnd_edt)
{
    if (hwnd_edt)
    {
        LOAD_I18N_RESSTR(IDS_SNIPPET_EDT_DEFAULT, edt_str);
        Edit_SetText(hwnd_edt, edt_str);
    }   
}

static void
on_snippet_init_cmb(HWND hwnd_cmb)
{
    if (hwnd_cmb)
    {
        int index = 1;
        doctype_t *doc_ptr = NULL;
        LOAD_I18N_RESSTR(IDS_SNIPPET_COMBO_DEC, cmb_str);
        ComboBox_InsertString(hwnd_cmb, 0, cmb_str);
        for (doc_ptr = eu_doc_get_ptr(); doc_ptr&&doc_ptr->doc_type; index++, doc_ptr++)
        {
            TCHAR *desc = eu_utf8_utf16(doc_ptr->filedesc, NULL);
            if (desc && doc_ptr->snippet && _tcscmp(desc, _T("Snippet File")))
            {
                ComboBox_InsertString(hwnd_cmb, index, desc);
            }
            eu_safe_free(desc);
        }
        ComboBox_SetCurSel(hwnd_cmb, 0);
    }
}

static void
on_snippet_init_control(HWND hdlg, eu_tabpage *pview)
{
    if (pview)
    {
        HWND hwnd_edt = GetDlgItem(hdlg, IDC_SNIPPET_EDT1);
        HWND hwnd_cmb = GetDlgItem(hdlg, IDC_SNIPPET_CBO1);
        if (hwnd_edt && hwnd_cmb)
        {
            on_snippet_init_edt(hwnd_edt);
            on_snippet_init_cmb(hwnd_cmb);
            on_snippet_init_sci(pview);
        }
    }
}

static void
on_snippet_move_edit(HWND hdlg)
{
    HWND hwnd_lst = GetDlgItem(hdlg, IDC_SNIPPET_LST);
    HWND hwnd_stc = GetDlgItem(hdlg, IDC_SNIPPET_STC2);
    eu_tabpage *pview = (eu_tabpage *)GetWindowLongPtr(hdlg, GWLP_USERDATA);
    if (hwnd_lst && hwnd_stc && pview && pview->hwnd_sc)
    {
        RECT rc;
        RECT rc_lst;
        RECT rc_stc;
        GetClientRect(hdlg, &rc);
        GetClientRect(hwnd_lst, &rc_lst);
        GetClientRect(hwnd_stc, &rc_stc);
        MapWindowPoints(hwnd_lst, hdlg, (LPPOINT)&rc_lst, 2);
        MapWindowPoints(hwnd_stc, hdlg, (LPPOINT)&rc_stc, 2);
        MoveWindow(pview->hwnd_sc, rc_stc.left, rc_lst.top, (rc.right - rc_stc.left - 5), (rc_lst.bottom - rc_lst.top), TRUE);
        ShowWindow(pview->hwnd_sc, SW_SHOW);
        on_snippet_reload(pview);
        on_snippet_init_control(hdlg, pview);
    }
}

static void
on_snippet_do_edt(const char *txt)
{
    HWND hwnd_edt = GetDlgItem(hwnd_snippet, IDC_SNIPPET_EDT1);
    if (hwnd_edt)
    {
        TCHAR ptxt[MAX_PATH] = {0};
        LOAD_I18N_RESSTR(IDS_SNIPPET_EDT_DEFAULT, edt_str);
        if (txt)
        {
            Edit_SetText(hwnd_edt, strlen(txt) > 0 ? util_make_u16(txt, ptxt, MAX_PATH-1) : edt_str);
        }
        else
        {
            Edit_SetText(hwnd_edt, _T(""));
        }
    }
}

static void
on_snippet_do_sci(const char *txt, bool sel)
{
    eu_tabpage *pview = NULL;
    if ((pview = (eu_tabpage *)GetWindowLongPtr(hwnd_snippet, GWLP_USERDATA)))
    {
        if (txt)
        {
            if (strlen(txt) > 0)
            {
                eu_sci_call(pview, SCI_CLEARALL, 0, 0);
                eu_sci_call(pview, SCI_ADDTEXT, strlen(txt), (sptr_t)txt);
                if (sel)
                {
                    SetFocus(pview->hwnd_sc);
                    eu_sci_call(pview, SCI_GOTOPOS, 0, 0);
                }
            }
            else
            {
                on_snippet_init_sci(pview);
            }
        }
        else
        {
            eu_sci_call(pview, SCI_CLEARALL, 0, 0);
        }
        eu_sci_call(pview, SCI_SETSAVEPOINT, 0, 0);
    }
}

static void
on_snippet_do_listbox(const char *txt)
{
    HWND hwnd_lst = GetDlgItem(hwnd_snippet, IDC_SNIPPET_LST);
    if (hwnd_lst)
    {
        if (!txt)
        {
            ListBox_ResetContent(hwnd_lst);
        }
        else if (txt[0])
        {
            TCHAR ptxt[MAX_PATH] = {0};
            ListBox_AddString(hwnd_lst, util_make_u16(txt, ptxt, MAX_PATH-1));
        }
    }
}

static void
on_snippet_write_control(snippet_t *pv)
{
    if (pv) 
    {
        snippet_t *it;
        for (it = cvector_begin(pv); it != cvector_end(pv); ++it)
        {
            on_snippet_do_listbox(it->name);
        }
    }
}

static void
on_snippet_init_parser(const TCHAR *path, snippet_t **ptr_vec)
{
    int eol = 0;
    cvector_vector_type(snippet_t) vec = NULL;
    if (on_parser_init(path, &vec, &eol))
    {   // 启用snippets文件里的回车符
        eu_tabpage *pview = (eu_tabpage *)GetWindowLongPtr(hwnd_snippet, GWLP_USERDATA);
        if (pview)
        {
            eu_sci_call(pview, SCI_SETEOLMODE, eol, 0);
        }
        on_snippet_write_control(vec);
        *ptr_vec = vec;
    }
}

static bool
on_snippet_get_file(HWND hwnd_cmb, TCHAR *path, int len)
{
    bool ret = false;
    char *pname = NULL;
    doctype_t *doc_ptr = NULL;
    TCHAR name[ACNAME_LEN] = {0};
    ComboBox_GetText(hwnd_cmb, name, ACNAME_LEN - 1);
    if (_tcslen(name) > 0 && (pname = eu_utf16_utf8(name, NULL)) != NULL)
    {
        for (doc_ptr = eu_doc_get_ptr(); doc_ptr&&doc_ptr->doc_type; ++doc_ptr)
        {
            if (doc_ptr->filedesc && doc_ptr->snippet && !strcmp(pname, doc_ptr->filedesc))
            {
                TCHAR fname[MAX_PATH] = {0};
                _sntprintf(path, len - 1, _T("%s\\conf\\snippets\\%s"), eu_module_path, util_make_u16(doc_ptr->snippet, fname, MAX_PATH-1));
                ret = true;
                break;
            }
        }
        free(pname);
    }
    return ret;
}

static void
on_snippet_do_combo(HWND hself, snippet_t **ptr_vec)
{
    TCHAR snippet_file[MAX_PATH] = {0};
    TCHAR first[ACNAME_LEN] = {0};
    TCHAR str[ACNAME_LEN] = {0};
    ComboBox_GetText(hself, str, ACNAME_LEN - 1);
    ComboBox_GetLBText(hself, 0, first);
    if (!_tcscmp(str, first))
    {
        on_snippet_do_edt("");
        on_snippet_do_sci("", false);
        on_snippet_do_listbox(NULL);
    }
    else if (on_snippet_get_file(hself, snippet_file, MAX_PATH))
    {
        cvector_vector_type(snippet_t) vec = NULL;
        if (eu_exist_file(snippet_file))
        {
            on_snippet_init_parser(snippet_file, &vec);
            *ptr_vec = vec;
        }
        if (!vec)
        {
            on_snippet_do_listbox(NULL);
        }
        on_snippet_do_edt(NULL);
        on_snippet_do_sci(NULL, false);
    }
}

static void
on_snippet_lst_click(HWND hwnd_lst, const char *ptxt, int index)
{
    snippet_t *vec = NULL;
    HWND hwnd_cbo = GetDlgItem(hwnd_snippet, IDC_SNIPPET_CBO1);
    if ((vec = (snippet_t *)GetWindowLongPtr(hwnd_cbo, GWLP_USERDATA)) != NULL)
    {
        snippet_t *it;
        for (it = cvector_begin(vec); it != cvector_end(vec); ++it) 
        {
            if (!strcmp(it->name, ptxt))
            {
                char edt_str[MAX_PATH] = {0};
                if (it->parameter[0])
                {
                    _snprintf(edt_str, MAX_PATH - 1, "%s/%s/%s", it->name, it->comment, it->parameter);
                }
                else if (it->comment[0])
                {
                    _snprintf(edt_str, MAX_PATH - 1, "%s/%s", it->name, it->comment);
                }
                else
                {
                    _snprintf(edt_str, MAX_PATH - 1, "%s", it->name);
                }
                on_snippet_do_edt(edt_str);
                on_snippet_do_sci(it->body, true);
            }
        }
    }
}

static void
on_snippet_do_modify(HWND hdlg)
{
    do
    {
        int index = -1;
        bool edt_modify = false;
        TCHAR snippet_file[MAX_PATH] = {0};
        HWND hwnd_edt = GetDlgItem(hdlg, IDC_SNIPPET_EDT1);
        HWND hwnd_cmb = GetDlgItem(hdlg, IDC_SNIPPET_CBO1);
        HWND hwnd_lst = GetDlgItem(hdlg, IDC_SNIPPET_LST);
        snippet_t *vec = (snippet_t *)GetWindowLongPtr(hwnd_cmb, GWLP_USERDATA);
        eu_tabpage *pview = (eu_tabpage *)GetWindowLongPtr(hdlg, GWLP_USERDATA);
        if (!(hwnd_edt && hwnd_cmb && hwnd_lst && pview && vec))
        {
            break;
        }
        if (ComboBox_GetCurSel(hwnd_cmb) == 0)
        {
            break;
        }
        index = ListBox_GetCurSel(hwnd_lst);
        if (Edit_GetModify(hwnd_edt))
        {
            int c = 0;
            TCHAR *p = NULL;
            TCHAR str[MAX_PATH] = {0};
            TCHAR name[ACNAME_LEN] = {0};
            Edit_GetText(hwnd_edt, str, MAX_PATH);
            p = _tcstok(str, _T("/"));
            while (p && _tcslen(p) < ACNAME_LEN)
            {
                switch (c)
                {
                    case 0:
                    {
                        memset(&vec[index].name, 0, ACNAME_LEN);
                        _sntprintf(name, ACNAME_LEN - 1, _T("%s"), p);
                        util_make_u8(p, vec[index].name, ACNAME_LEN);
                        break;
                    }
                    case 1:
                    {
                        memset(&vec[index].comment, 0, ACNAME_LEN);
                        util_make_u8(p, vec[index].comment, ACNAME_LEN);
                        break;
                    }
                    case 2:
                    {
                        memset(&vec[index].parameter, 0, 2);
                        util_make_u8(p, vec[index].parameter, 2);
                        break;
                    }
                    default:
                        break;
                }
                if ((p = _tcstok(NULL, _T("/"))))
                {
                    ++c;
                }
            }
            if (name[0])
            {
                int len = 0;
                TCHAR *ptxt = NULL;
                if ((len = ListBox_GetTextLen(hwnd_lst, index)) > 0 && (ptxt = calloc(sizeof(TCHAR), len + 1)))
                {
                    ListBox_GetText(hwnd_lst, index, ptxt);
                    if (_tcslen(ptxt) > 0 && (_tcscmp(name, ptxt)))
                    {
                        ListBox_DeleteString(hwnd_lst, index);
                        ListBox_InsertString(hwnd_lst, index, name);
                        ListBox_SetCurSel(hwnd_lst, index);
                    }
                    free(ptxt);
                }
            }
            Edit_SetModify(hwnd_edt, FALSE);
            edt_modify = true;
        }
        if (eu_sci_call(pview, SCI_GETMODIFY, 0, 0))
        {
            char *txt = util_strdup_content(pview, NULL);
            if (txt)
            {
                _snprintf(vec[index].body, LARGER_LEN - 1, "%s", txt);
                eu_sci_call(pview, SCI_SETSAVEPOINT, 0, 0);
                edt_modify = true;
                free(txt);
            }
        }
        if (edt_modify && index >= 0 && on_snippet_get_file(hwnd_cmb, snippet_file, MAX_PATH))
        {
            on_parser_vector_modify(snippet_file, &vec, index);
        }        
    } while (0);
}

static INT_PTR CALLBACK
on_snippet_proc(HWND hdlg, uint32_t msg, WPARAM wParam, LPARAM lParam)
{
    eu_tabpage *pview = NULL;
    static int last_index = 0;
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            if (!(pview = (eu_tabpage *)calloc(1, sizeof(eu_tabpage))))
            {
                SendMessage(hdlg, WM_CLOSE, 0, 0);
                break;
            }
            const int flags = WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_CLIPCHILDREN | WS_EX_RTLREADING;
            if (on_sci_create(pview, hdlg, flags, on_snippet_edit_proc) != SKYLARK_OK)
            {
                SendMessage(hdlg, WM_CLOSE, 0, 0);
                break;
            }  
            SetWindowLongPtr(hdlg, GWLP_USERDATA, (LONG_PTR)pview);
            HWND hwnd_edt = GetDlgItem(hdlg, IDC_SNIPPET_EDT1);
            HWND hwnd_cmb = GetDlgItem(hdlg, IDC_SNIPPET_CBO1);
            HWND hwnd_lst = GetDlgItem(hdlg, IDC_SNIPPET_LST);
            if (hwnd_edt && hwnd_cmb && hwnd_lst)
            {
                SetWindowSubclass(hwnd_edt, on_snippet_edt_proc, SNIPPET_EDT_SUBID, 0);
                SetWindowSubclass(hwnd_cmb, on_snippet_cmb_proc, SNIPPET_CMB_SUBID, 0);
                SendMessage(hwnd_edt, WM_SETFONT, (WPARAM) on_theme_font_hwnd(), 0);
                SendMessage(hwnd_cmb, WM_SETFONT, (WPARAM) on_theme_font_hwnd(), 0);
                SendMessage(hwnd_lst, WM_SETFONT, (WPARAM) on_theme_font_hwnd(), 0);
            }
            hwnd_snippet = hdlg;
            util_creater_window(hdlg, eu_module_hwnd());
            on_snippet_move_edit(hdlg);
            if (on_dark_enable())
            {
                const int buttons[] = {IDC_SNIPPET_BTN1,
                                       IDC_SNIPPET_BTN2,
                                       IDC_SNIPPET_BTN3};
                for (int id = 0; id < _countof(buttons); ++id)
                {
                    HWND btn = GetDlgItem(hdlg, buttons[id]);
                    on_dark_set_theme(btn, L"Explorer", NULL);
                }
                on_dark_set_theme(hdlg, L"Explorer", NULL);
            }
            return (INT_PTR)SendMessage(hdlg, WM_SETFONT, (WPARAM) on_theme_font_hwnd(), 0);
        }
        case WM_THEMECHANGED:
        {
            if (on_dark_enable())
            {
                on_dark_allow_window(hdlg, true);
                on_dark_refresh_titlebar(hdlg);
                const int buttons[] = {IDC_SNIPPET_BTN1,
                                       IDC_SNIPPET_BTN2,
                                       IDC_SNIPPET_BTN3};
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
        CASE_WM_CTLCOLOR_SET:
        {
            return on_dark_set_contorl_color(wParam);
        }
        case WM_COMMAND:
        {
            WORD mid = LOWORD(wParam);
            switch (mid)
            {
                case IDCANCEL:
                case IDC_SNIPPET_BTN3:
                    SendMessage(hdlg, WM_CLOSE, 0, 0);
                    return 1;
                case IDC_SNIPPET_CBO1:
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        HWND cbo_self = (HWND)lParam;
                        int focus = ComboBox_GetCurSel(cbo_self);
                        if (focus != last_index)
                        {
                            snippet_t *before_vec = (snippet_t *)GetWindowLongPtr(cbo_self, GWLP_USERDATA);
                            cvector_vector_type(snippet_t) vec = NULL;
                            on_snippet_do_combo(cbo_self, &vec);
                            if (vec && vec != before_vec)
                            {
                                cvector_freep(&before_vec);
                                SetWindowLongPtr(cbo_self, GWLP_USERDATA, (LONG_PTR)vec);
                            }
                            last_index = ComboBox_GetCurSel(cbo_self);            
                        }
                    }
                    break;
                }
                case IDC_SNIPPET_LST:
                {
                    if (HIWORD(wParam) == LBN_DBLCLK || HIWORD(wParam) == LBN_SELCHANGE)
                    {
                        TCHAR *txt = NULL;
                        HWND hwnd_lst = (HWND)lParam;
                        int index = ListBox_GetCurSel(hwnd_lst);
                        int len = ListBox_GetTextLen(hwnd_lst, index);
                        if (len > 0 && (txt = calloc(sizeof(TCHAR), len + 1)))
                        {
                            char *ptxt = NULL;
                            ListBox_GetText(hwnd_lst, index, txt);
                            if (_tcslen(txt) > 0 && (ptxt = eu_utf16_utf8(txt, NULL)))
                            {
                                on_snippet_lst_click(hwnd_lst, ptxt, index);
                                free(ptxt);
                            }
                            free(txt);
                        }
                    }
                    break;
                }
                case IDC_SNIPPET_BTN1:
                {
                    int i = 0;
                    snippet_t *vec = NULL;
                    TCHAR snippet_file[MAX_PATH] = {0};
                    HWND hwnd_lst = GetDlgItem(hdlg, IDC_SNIPPET_LST);
                    HWND hwnd_cmb = GetDlgItem(hdlg, IDC_SNIPPET_CBO1);
                    int index = ListBox_GetCurSel(hwnd_lst);
                    ListBox_DeleteString(hwnd_lst, index);
                    if (index == 0)
                    {
                        i = ListBox_GetTopIndex(hwnd_lst);
                    }
                    else if (index > 0)
                    {
                        i = index - 1;
                    }
                    ListBox_SetCurSel(hwnd_lst, i);
                    SendMessage(hdlg, WM_COMMAND, MAKEWPARAM(IDC_SNIPPET_LST, LBN_SELCHANGE), (LPARAM)hwnd_lst);
                    if (on_snippet_get_file(hwnd_cmb, snippet_file, MAX_PATH) && (vec = (snippet_t *)GetWindowLongPtr(hwnd_cmb, GWLP_USERDATA)) != NULL)
                    {
                        on_parser_vector_erase(snippet_file, &vec, index);
                    }
                    break;
                }
                case IDC_SNIPPET_BTN2:
                {
                    on_snippet_do_modify(hdlg);
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case WM_DESTROY:
        {
            if (hwnd_snippet)
            {
                snippet_t *vec = NULL;
                HWND hwnd_cbo = GetDlgItem(hdlg, IDC_SNIPPET_CBO1);
                if ((pview = (eu_tabpage *)GetWindowLongPtr(hdlg, GWLP_USERDATA)))
                {
                    if (pview->hwnd_sc)
                    {
                        DestroyWindow(pview->hwnd_sc);
                        pview->hwnd_sc = NULL;
                    }
                    eu_safe_free(pview);
                    SetWindowLongPtr(hdlg, GWLP_USERDATA, 0);
                }
                if ((vec = (snippet_t *)GetWindowLongPtr(hwnd_cbo, GWLP_USERDATA)) != NULL)
                {
                    cvector_freep(&vec);
                    HWND hwnd_cbo = GetDlgItem(hdlg, IDC_SNIPPET_CBO1);
                    SetWindowLongPtr(hwnd_cbo, GWLP_USERDATA, 0);
                }
                last_index = 0;
                hwnd_snippet = NULL;
                printf("hwnd_snippet WM_DESTROY\n");
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

void WINAPI
on_snippet_create_dlg(HWND parent)
{
    i18n_dlgbox(parent, IDD_SNIPPET_DLG, on_snippet_proc, 0);
}
