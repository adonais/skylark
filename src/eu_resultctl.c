/******************************************************************************
 * This file is part of Skylark project
 * Copyright ©2025 Hua andy <hua.andy@gmail.com>

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

static HWND hwnd_rst = NULL;

int
on_result_append_text(TCHAR *format, ...)
{
    va_list valist;
    SYSTEMTIME systime;
    int l, len = 0;
    TCHAR *buf = NULL;
    char *utf_buf = NULL;
    eu_tabpage *pnode = on_tabpage_focus_at();
    if (!RESULT_SHOW(pnode))
    {
        return 1;
    }
    va_start(valist, format);
    int buf_len = _vsctprintf(format, valist);
    buf = buf_len > 0 ? (TCHAR *)calloc(sizeof(TCHAR), buf_len += 68) : NULL;
    if (!buf)
    {
        va_end(valist);
        return 1;

    }
    if (on_sci_call(pnode->presult, SCI_GETLENGTH, 0, 0) < 1)
    {
        char *u8 = NULL;
        LOAD_I18N_RESSTR(IDS_LOADLIBRARY_SQL, mlib);
        if ((u8 = eu_utf16_utf8(mlib, NULL)) != NULL)
        {
            on_sci_call(pnode->presult, SCI_ADDTEXT, strlen(u8), (sptr_t)u8);
            free(u8);
        }
    }
    else
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
    l = _vsntprintf(buf + len, buf_len - 1 - len, format, valist);
    if (l > 0)
    {
        len += l;
    }
    va_end(valist);
    l = _sntprintf(buf + len, buf_len - 1 - len, _T("\n"));
    if (l > 0)
    {
        len += l;
    }
    if ((utf_buf = eu_utf16_utf8(buf, NULL)) != NULL)
    {
        on_sci_call(pnode->presult, SCI_ADDTEXT, (sptr_t)strlen(utf_buf), (sptr_t)utf_buf);
        free(utf_buf);
    }
    free(buf);
    return 0;
}

int
on_result_append_text_utf8(char *format, ...)
{
    va_list valist;
    char *buf = NULL;
    int l, len = 0;
    bool cmds = false;
    eu_tabpage *p = NULL;
    if ((p = on_tabpage_focus_at()) == NULL || !RESULT_SHOW(p))
    {
        return 1;
    }
    va_start(valist, format);
    int buf_len = _vscprintf(format, valist);
    buf = buf_len > 0 ? (char *)calloc(1, buf_len += 68) : NULL;
    if (!buf)
    {
        va_end(valist);
        return 1;
    }
    l = _vsnprintf(buf + len, buf_len - 1 - len, format, valist);
    if (l > 0)
    {
        len += l;
    }
    va_end(valist);
    l = _snprintf(buf + len, buf_len - 1 - len, "\n");
    if (l > 0)
    {
        len += l;
    }
    if ((on_sci_call(p->presult, SCI_GETCARETSTYLE, 0, 0) & CARETSTYLE_LINE))
    {
        cmds = true;
    }
    if (!cmds)
    {
        on_sci_call(p->presult, SCI_SETREADONLY, 0, 0);
        on_sci_call(p->presult, SCI_CLEARALL, 0, 0);
        on_sci_call(p->presult, SCI_ADDTEXT, (sptr_t)strlen(buf), (sptr_t)buf);
        on_sci_call(p->presult, SCI_SETREADONLY, 1, 0);
        on_sci_call(p->presult, SCI_GOTOLINE, 1, 0);
    }
    else
    {
        on_sci_call(p->presult, SCI_ADDTEXT, (sptr_t)1, (sptr_t)"\n");
        on_sci_call(p->presult, SCI_ADDTEXT, (sptr_t)(strlen(buf) - 1), (sptr_t)buf);
    }
    free(buf);
    return 0;
}

static void
on_result_menu_callback(HMENU hpop, void *param)
{
    eu_tabpage *p = (eu_tabpage *)param;
    if (RESULT_SHOW(p) && hpop)
    {
        util_set_menu_item(hpop, IDM_RESULT_WRAPLINE, on_sci_call(p->presult, SCI_GETWRAPMODE, 0, 0));
    }
}

static eu_tabpage *
on_result_other_tab(int line, result_vec *vec_strings)
{
    if ((int)cvector_size(vec_strings) >= line)
    {
        for (int i = line - 1; i >= 0; --i)
        {
            if (vec_strings[i].line == -1)
            {
                return (eu_tabpage *)(vec_strings[i].mark._no);
            }
        }
    }
    return NULL;
}

static int
on_result_dbclick(void)
{
    eu_tabpage *pnode = on_tabpage_focus_at();
    if (pnode && pnode->presult && pnode->ret_vec)
    {
        const sptr_t pos = on_sci_call(pnode->presult, SCI_GETCURRENTPOS, 0, 0);
        const sptr_t line = on_sci_call(pnode->presult, SCI_LINEFROMPOSITION, pos, 0);
        on_sci_call(pnode->presult, SCI_SETEMPTYSELECTION, pos, 0);
        if (line > 0 && cvector_size(pnode->ret_vec) > 0)
        {
            const sptr_t current_line = on_sci_call(pnode, SCI_LINEFROMPOSITION, on_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0), 0);
            eu_tabpage *p = on_result_other_tab((int)line, pnode->ret_vec);
            if (!p)
            {
                on_search_jmp_line(pnode, pnode->ret_vec[line - 1].line, current_line);
                on_sci_call(pnode, SCI_SETSELECTION, pnode->ret_vec[line - 1].mark.start, pnode->ret_vec[line - 1].mark.end);
                SetFocus(pnode->hwnd_sc);
            }
            else if (on_tabpage_selection(p) >= 0)
            {
                on_search_jmp_line(p, pnode->ret_vec[line - 1].line, current_line);
                on_sci_call(p, SCI_SETSEL, pnode->ret_vec[line - 1].mark.start, pnode->ret_vec[line - 1].mark.end);
            }
        }
    }
    return 1;
}

static void
on_result_command(const HWND hwnd, const WORD low)
{
    switch(low)
    {
        case IDM_RESULT_COPY:
            SendMessage(hwnd, WM_COPY, 0, 0);
            break;
        case IDM_RESULT_SETSEL:
            SendMessage(hwnd, EM_SETSEL, 0, -1);
            break;
        case IDM_RESULT_UNSETSEL:
            SendMessage(hwnd, EM_SETSEL, 0, 0);
            break;
        case IDM_RESULT_WRAPLINE:
        {
            eu_tabpage *p = on_tabpage_focus_at();
            if (RESULT_SHOW(p))
            {
                int mode = (int)on_sci_call(p->presult, SCI_GETWRAPMODE, 0, 0);
                !mode ? on_sci_call(p->presult, SCI_SETWRAPMODE, 2, 0) : on_sci_call(p->presult, SCI_SETWRAPMODE, 0, 0);
            }
            break;
        }
        case IDM_RESULT_CLEARALL:
        {
            eu_tabpage *p = on_tabpage_focus_at();
            if (p && p->presult && p->presult->hwnd_sc)
            {
                on_sci_call(p->presult, SCI_SETREADONLY, 0, 0);
                on_sci_call(p->presult, SCI_CLEARALL, 0, 0);
                if (p->presult->pwant != on_command_light)
                {
                    on_sci_call(p->presult, SCI_SETREADONLY, 1, 0);
                }
            }
            break;
        }
        case IDM_RESULT_CLOSE:
        {
            eu_tabpage *p = on_tabpage_focus_at();
            on_result_destroy(p);
            break;
        }
        default:
        {
            break;
        }
    }
}

static LRESULT CALLBACK
on_result_edit_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_KEYDOWN:
        {
            if ((wParam == VK_ESCAPE || wParam == VK_RETURN) && KEY_UP(VK_SHIFT) && KEY_UP(VK_CONTROL))
            {
                eu_tabpage *pnode = on_tabpage_focus_at();
                if (RESULT_SHOW(pnode) && pnode->presult->pwant == on_command_light)
                {
                    if (wParam == VK_ESCAPE)
                    {
                        on_sci_call(pnode->presult, SCI_CLEARALL, 0, 0);
                    }
                    else
                    {
                        on_script_loader_event(SKYLARK_COMMANDS, pnode->presult);
                    }
                }
            }
            break;
        }
        case WM_LBUTTONDBLCLK:
        {
            return on_result_dbclick();
        }
        case WM_RBUTTONUP:
        {
            eu_tabpage *p = on_tabpage_focus_at();
            if (RESULT_SHOW(p))
            {
                return menu_pop_track(hwnd, IDR_RESULT_MENU, 0, -1, on_result_menu_callback, p);
            }
            return 1;
        }
        case WM_COMMAND:
        {
            WORD low = LOWORD(wParam);
            if (HIWORD(wParam) == 0)
            {
                on_result_command(hwnd, low);
            }
            break;
        }
        case WM_THEMECHANGED:
        {
            break;
        }
        case WM_DESTROY:
        {
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
        case WM_SIZE:
        {
            break;
        }
        case WM_DESTROY:
        {
            if (hwnd_rst)
            {
                hwnd_rst = NULL;
                eu_logmsg("Result: window destroy\n");
            }
            break;
        }
        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}

void
on_result_destroy(eu_tabpage *p)
{
    if (RESULT_SHOW(p))
    {
        SendMessage(p->presult->hwnd_sc, WM_CLOSE, 0, 0);
        p->presult->hwnd_sc = NULL;
        p->result_show = false;
        eu_safe_free(p->presult);
        eu_window_resize();
    }
}

void
on_result_lexer(eu_tabpage *pedit)
{
    if (pedit)
    {
        on_sci_default_theme(pedit, DEFAULTSPACE);
        // disable margin
        on_sci_call(pedit, SCI_SETMARGINS, 0, 0);
        // 强制使用unix回车符
        on_sci_call(pedit, SCI_SETEOLMODE, SC_EOL_LF, 0);
        // 不显示插入符
        on_sci_call(pedit, SCI_SETCARETSTYLE, CARETSTYLE_INVISIBLE, 0);
        // 需要时显示水平滚动条
        on_sci_call(pedit, SCI_SETSCROLLWIDTH, 1, 0);
        on_sci_call(pedit, SCI_SETSCROLLWIDTHTRACKING, 1, 0);
        // 加载词语解析器
        on_doc_init_after_scilexer(pedit, "result");
        on_doc_default_light(pedit, SCE_RESULT_COMMENT, 0x768465, -1, true);
        on_doc_default_light(pedit, SCE_RESULT_HEADER , eu_get_theme()->item.results.color, -1, true);
        on_doc_default_light(pedit, SCE_RESULT_KEYWORD, eu_get_theme()->item.results.bgcolor, -1, true);
        if (pedit->pwant)
        {
            pedit->pwant(pedit);
        }
    }
}

bool
on_result_launch(eu_tabpage *pnode)
{
    if (pnode)
    {
        if (!pnode->presult)
        {
            const TCHAR *class_name = _T("Result List");
            const int flags = WS_CHILD | WS_CLIPSIBLINGS | WS_VSCROLL | WS_CLIPCHILDREN | WS_EX_RTLREADING;
            pnode->presult = (eu_tabpage *)calloc(1, sizeof(eu_tabpage));
            if (!hwnd_rst)
            {
                hwnd_rst = on_splitter_init_window(eu_module_hwnd(), class_name, WS_CHILD | WS_CLIPSIBLINGS, NULL, on_result_callback, NULL);
            }
            if (pnode->presult && hwnd_rst && !on_sci_create(pnode->presult, hwnd_rst, flags, on_result_edit_proc))
            {
                on_dark_border(pnode->presult->hwnd_sc, true);
            }
        }
        return true;
    }
    return false;
}

void
on_result_move_sci(eu_tabpage *p, int width, int height)
{
    if (RESULT_SHOW(p))
    {
        MoveWindow(p->presult->hwnd_sc, 0, 0, width, height, TRUE);
        ShowWindow(p->presult->hwnd_sc, SW_SHOW);
    }
}

HWND
eu_result_hwnd(void)
{
    return hwnd_rst;
}
