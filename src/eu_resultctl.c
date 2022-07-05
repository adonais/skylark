/******************************************************************************
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

HWND hwnd_rst = NULL;
volatile long result_dlg_initialized = 0;
static WNDPROC orig_result_proc;

LRESULT CALLBACK
ptr_result_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_SIZE:
            break;
        case WM_RBUTTONUP:
        {
            return menu_pop_track(hwnd, IDR_RESULT_MENU, 0);
        }
        case WM_COMMAND:
        {
            WORD high = HIWORD(wParam);
            WORD low = LOWORD(wParam);
            if (high != 0)
            {
                break;
            }
            switch(low)
            {
                case IDM_RESULT_1:
                    SendMessage(hwnd, WM_COPY, 0, 0);
                    break;
                case IDM_RESULT_2:
                    SendMessage(hwnd, WM_PASTE, 0, 0);
                    break;
                case IDM_RESULT_3:
                    SendMessage(hwnd, EM_SETSEL, 0, -1);
                    break;
                case IDM_RESULT_4:
                    SendMessage(hwnd, EM_SETSEL, 0, 0);
                    break;
                case IDM_RESULT_5:
                    SendMessage(hwnd, EM_SETSEL, 0, -1);
                    SendMessage(hwnd, WM_CLEAR, 0, 0);
                    break;
                case IDM_RESULT_CLOSE:
                {
                    eu_tabpage *p = on_tabpage_focus_at();
                    if (p)
                    {
                        p->edit_show = false;
                        eu_window_resize(NULL);
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case WM_DPICHANGED:
        {
            SendMessage(hwnd, WM_SETFONT, (WPARAM) on_theme_font_hwnd(), 0);
            break;
        }
        case WM_DESTROY:
        {
            printf("result_edit WM_DESTROY\n");
            break;
        }
        default:
            break;
    }
    return CallWindowProc(orig_result_proc, hwnd, message, wParam, lParam);
}

static int
update_redit_theme(eu_tabpage *pnode)
{
    if (pnode && pnode->hwnd_qredit)
    {
        SendMessage(pnode->hwnd_qredit, WM_SETFONT, (WPARAM) on_theme_font_hwnd(), 0);
    }
    return 0;
}

int
on_result_create_dlg(eu_tabpage *pnode)
{
    if (pnode->hwnd_qredit)
    {
        DestroyWindow(pnode->hwnd_qredit);
    }
    /* 创建结果消息控件 */
    const int style = WS_CHILD | WS_CLIPSIBLINGS | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL;
    pnode->hwnd_qredit = CreateWindow(_T("EDIT"), NULL, style, 0, 0, 0, 0, eu_module_hwnd(), NULL, eu_module_handle(), NULL);
    if (pnode->hwnd_qredit == NULL)
    {
        MSG_BOX(IDC_MSG_EDIT_ERR1, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
        return 1;
    }
    if (!(orig_result_proc = (WNDPROC) SetWindowLongPtr(pnode->hwnd_qredit, GWLP_WNDPROC, (LONG_PTR) ptr_result_proc)))
    {
        printf("SetWindowLongPtr failed\n");
        return 1;
    }
    return update_redit_theme(pnode);
}

int
on_result_append_text(HWND hwnd, TCHAR *format, ...)
{
    va_list valist;
    SYSTEMTIME systime;
    TCHAR *buf = NULL;
    int l, len = 0;
    bool pt = true;
    eu_tabpage *pnode = NULL;
    if (hwnd == NULL)
    {
        if ((pnode = on_tabpage_focus_at()) == NULL)
        {
            return 1;
        }
        else
        {
            pt = false;
            hwnd = pnode->hwnd_qredit;
        }
    }
    va_start(valist, format);
    int buf_len = _vsctprintf(format, valist);
    buf = buf_len > 0 ? (TCHAR *)calloc(sizeof(TCHAR), buf_len += 68) : NULL;
    if (!buf)
    {
        va_end(valist);
        return 1;

    }
    if (pt)
    {
        GetLocalTime(&systime);
        l = _sntprintf(buf + len,
                       buf_len - len,
                       _T("%04d-%02d-%02d %02d:%02d:%02d.%06d | "),
                       systime.wYear,
                       systime.wMonth,
                       systime.wDay,
                       systime.wHour,
                       systime.wMinute,
                       systime.wSecond,
                       systime.wMilliseconds * 1000);
        if (l > 0)
        {
            len += l;
        }
    }
    {
        l = _vsntprintf(buf + len, buf_len - 1 - len, format, valist);
        if (l > 0)
        {
            len += l;
        }
    }
    va_end(valist);
    {
        l = _sntprintf(buf + len, buf_len - 1 - len, _T("\r\n"));
        if (l > 0)
        {
            len += l;
        }
    }
    SendMessage(hwnd, EM_SETSEL, -2, -1);
    SendMessage(hwnd, EM_REPLACESEL, true, (LPARAM) (LPCTSTR) buf);
    free(buf);
    return 0;
}

static LRESULT CALLBACK
on_result_edit_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_LBUTTONDBLCLK:
        {
            eu_tabpage *pnode = on_tabpage_focus_at();
            eu_tabpage *prst = (eu_tabpage *)SendMessage(hwnd_rst, WM_RESULT_PTR, 0, 0);
            if (prst && pnode && pnode->pvec)
            {
                sptr_t line = eu_sci_call(prst, SCI_LINEFROMPOSITION, eu_sci_call(prst, SCI_GETCURRENTPOS, 0, 0), 0);
                if (line >= 0 && cvector_size(pnode->pvec) > 0)
                {
                    sptr_t pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
                    sptr_t current_line = eu_sci_call(pnode, SCI_LINEFROMPOSITION, pos, 0);
                    on_search_jmp_line(pnode, pnode->pvec[line - 1].line, current_line);
                    eu_sci_call(pnode, SCI_SETSEL, pnode->pvec[line - 1].mark.start, pnode->pvec[line - 1].mark.end);
                }
            }
            return 1;
        }
        case WM_THEMECHANGED:
        {
            break;
        }
        case WM_DESTROY:
        {
            printf("on_result_edit_proc WM_DESTROY\n");
            break;
        }
    }
    return CallWindowProc((WNDPROC)eu_edit_wnd, hwnd, message, wParam, lParam);
}

static LRESULT CALLBACK
on_result_callback(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_CREATE:
        {
            eu_tabpage *rst_scintilla = (eu_tabpage *)((LPCREATESTRUCTW)lParam)->lpCreateParams;
            if (rst_scintilla)
            {
                SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)rst_scintilla);
                const int flags = WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_CLIPCHILDREN | WS_EX_RTLREADING;
                if (!on_sci_create(rst_scintilla, flags, on_result_edit_proc))
                {
                    on_dark_border(rst_scintilla->hwnd_sc, true);
                }
            }
            break;
        }
        case WM_SIZE:
        {
            break;
        }
        case WM_RESULT_PTR:
        {
            return GetWindowLongPtr(hwnd, GWLP_USERDATA);
        }
        case WM_DESTROY:
        {
            eu_tabpage *rst_scintilla = (eu_tabpage *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
            if (rst_scintilla)
            {
                if (rst_scintilla->hwnd_sc)
                {
                    DestroyWindow(rst_scintilla->hwnd_sc);
                    rst_scintilla->hwnd_sc = NULL;
                }
                eu_safe_free(rst_scintilla);
                SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
            }
            _InterlockedExchange(&result_dlg_initialized, 0);
            printf("on_result_callback WM_DESTROY\n");
            break;
        }
        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}

void
on_result_reload(eu_tabpage *pedit)
{
    eu_tabpage *pnode = on_tabpage_focus_at();
    if (pedit && pnode)
    {
        on_sci_init_style(pedit);
        // disable margin
        eu_sci_call(pedit, SCI_SETMARGINWIDTHN, MARGIN_LINENUMBER_INDEX, 0);
        eu_sci_call(pedit, SCI_SETMARGINWIDTHN, MARGIN_BOOKMARK_INDEX, 0);
        eu_sci_call(pedit, SCI_SETMARGINWIDTHN, MARGIN_FOLD_INDEX, 0);
        // 同步主编辑器换行符
        eu_sci_call(pedit, SCI_SETEOLMODE, pnode->eol, 0);
        // 不启用自动换行
        eu_sci_call(pedit, SCI_SETWRAPMODE, 0, 0);
        // 不显示对齐线
        eu_sci_call(pedit, SCI_SETINDENTATIONGUIDES, SC_IV_NONE, 0);
        // 去除只读
        eu_sci_call(pedit, SCI_SETREADONLY, 0, 0);
        // 加载词语解析器
        on_doc_init_after_scilexer(pedit, "result");
        on_doc_default_light(pedit, SCE_RESULT_COMMENT, eu_get_theme()->item.keywords0.color, -1, true);
        on_doc_default_light(pedit, SCE_RESULT_KEYWORD, eu_get_theme()->item.number.color, -1, true);
    }
}

eu_tabpage *
on_result_launch(void)
{
    eu_tabpage *rst_edit = NULL;
    if (!_InterlockedCompareExchange(&result_dlg_initialized, 1, 0))
    {
        const TCHAR *class_name = _T("Result List");
        const int flags = WS_CHILD | WS_CLIPSIBLINGS;
        HWND parent = eu_module_hwnd();
        rst_edit = (eu_tabpage *)calloc(1, sizeof(eu_tabpage));
        if (!rst_edit ||
            !(hwnd_rst = on_splitter_init_window(parent, class_name, flags, NULL, on_result_callback, (void *)rst_edit)))
        {
            _InterlockedExchange(&result_dlg_initialized, 0);
            eu_safe_free(rst_edit);
        }
    }
    else if (hwnd_rst)
    {
        rst_edit = (eu_tabpage *)GetWindowLongPtr(hwnd_rst, GWLP_USERDATA);
    }
    return rst_edit;
}
