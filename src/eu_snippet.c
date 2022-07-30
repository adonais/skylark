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
        // 强制使用unix回车符
        eu_sci_call(pedit, SCI_SETEOLMODE, SC_EOL_LF, 0);
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
    }
}

static void
on_snippet_init_edt(HWND hwnd_edt)
{
    if (hwnd_edt)
    {
        RECT rect_edt;
        LOAD_I18N_RESSTR(IDS_SNIPPET_EDT_DEFAULT, edt_str);
        SendMessage(hwnd_edt, EM_GETRECT, 0, (LPARAM)&rect_edt);
        rect_edt.top += 2;
        rect_edt.bottom -= 2;
        SendMessage(hwnd_edt, EM_SETRECT, 0, (intptr_t)&rect_edt);
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
on_snippet_do_sci(const char *txt)
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
on_snippet_init_parser(const TCHAR *path)
{
    ;
}

static void
on_snippet_do_combo(HWND hself)
{
    char *pstr = NULL;
    doctype_t *doc_ptr = NULL;
    TCHAR str[ACNAME_LEN] = {0};
    TCHAR psn[MAX_PATH+1] = {0};
    TCHAR first[ACNAME_LEN] = {0};
    ComboBox_GetText(hself, str, ACNAME_LEN-1);
    ComboBox_GetLBText(hself, 0, first);
    if (!_tcscmp(str, first))
    {
        on_snippet_do_edt("");
        on_snippet_do_sci("");
        on_snippet_do_listbox(NULL);
    }
    else if ((pstr = eu_utf16_utf8(str, NULL)) != NULL)
    {
        for (doc_ptr = eu_doc_get_ptr(); doc_ptr&&doc_ptr->doc_type; ++doc_ptr)
        {
            if (doc_ptr->filedesc && doc_ptr->snippet && !strcmp(pstr, doc_ptr->filedesc))
            {
                TCHAR name[MAX_PATH] = {0};
                _sntprintf(psn, MAX_PATH, _T("%s\\conf\\snippets\\%s"), eu_module_path, util_make_u16(doc_ptr->snippet, name, MAX_PATH-1));
                break;
            }
        }
        free(pstr);
        if (eu_exist_file(psn))
        {
            on_snippet_init_parser(psn);
        }
        else
        {   // snippet file does not exist
            on_snippet_do_edt(NULL);
            on_snippet_do_sci(NULL);
            on_snippet_do_listbox(NULL);
        }
    }
}

static INT_PTR CALLBACK
on_snippet_proc(HWND hdlg, uint32_t msg, WPARAM wParam, LPARAM lParam)
{
    eu_tabpage *pview = NULL;
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
            if (hwnd_edt && hwnd_cmb)
            {
                SetWindowSubclass(hwnd_edt, on_snippet_edt_proc, SNIPPET_EDT_SUBID, 0);
                SetWindowSubclass(hwnd_cmb, on_snippet_cmb_proc, SNIPPET_CMB_SUBID, 0);
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
            return 1;
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
                        on_snippet_do_combo((HWND)lParam);
                    }
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
