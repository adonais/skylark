/******************************************************************************
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

int
on_result_append_text(eu_tabpage *pnode, TCHAR *format, ...)
{
    va_list valist;
    SYSTEMTIME systime;
    int l, len = 0;
    TCHAR *buf = NULL;
    char *utf_buf = NULL;
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
    if (eu_sci_call(pnode->presult, SCI_GETLENGTH, 0, 0) < 1)
    {
        char *u8 = NULL;
        LOAD_I18N_RESSTR(IDS_LOADLIBRARY_SQL, mlib);
        if ((u8 = eu_utf16_utf8(mlib, NULL)) != NULL)
        {
            eu_sci_call(pnode->presult, SCI_ADDTEXT, strlen(u8), (sptr_t)u8);
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
        eu_sci_call(pnode->presult, SCI_ADDTEXT, strlen(utf_buf), (LPARAM)utf_buf);
        free(utf_buf);
    }
    free(buf);
    return 0;
}

int
on_result_append_text_utf8(eu_tabpage *pnode, char *format, ...)
{
    va_list valist;
    char *buf = NULL;
    int l, len = 0;
    if (!RESULT_SHOW(pnode))
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
    eu_sci_call(pnode->presult, SCI_SETREADONLY, 0, 0);
    eu_sci_call(pnode->presult, SCI_CLEARALL, 0, 0);
    eu_sci_call(pnode->presult, SCI_ADDTEXT, strlen(buf), (LPARAM)buf);
    eu_sci_call(pnode->presult, SCI_SETREADONLY, 1, 0);
    eu_sci_call(pnode->presult, SCI_GOTOLINE, 1, 0);
    free(buf);
    return 0;
}

static void
on_result_menu_callback(HMENU hpop, void *param)
{
    eu_tabpage *p = (eu_tabpage *)param;
    if (RESULT_SHOW(p) && hpop)
    {
        util_set_menu_item(hpop, IDM_RESULT_WRAPLINE, eu_sci_call(p->presult, SCI_GETWRAPMODE, 0, 0));
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

static LRESULT CALLBACK
on_result_edit_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static bool ctrl_down = false;
    switch (message)
    {   // 不在主消息循环, 支持CTRL+C
        case WM_KEYDOWN:
        {
            if (wParam == VK_CONTROL)
            {
                ctrl_down = true;
            }
            else
            {
                ctrl_down = false;
            }
            return 1;
        }
        case WM_KEYUP:
        {
            if (wParam == VK_CONTROL)
            {
                ctrl_down = false;
            }
            if (wParam == 'C'  && ctrl_down)
            {
                SendMessage(hwnd, WM_COPY, 0, 0);
            }
            return 1;
        }
        case WM_LBUTTONDBLCLK:
        {
            eu_tabpage *pnode = on_tabpage_from_handle(hwnd, on_tabpage_resultctl);
            if (pnode && pnode->presult && pnode->ret_vec)
            {
                sptr_t line = eu_sci_call(pnode->presult, SCI_LINEFROMPOSITION, pnode->presult->nc_pos, 0);
                eu_sci_call(pnode->presult, SCI_SETEMPTYSELECTION, pnode->presult->nc_pos, 0);
                if (line > 0 && cvector_size(pnode->ret_vec) > 0)
                {
                    sptr_t pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
                    sptr_t current_line = eu_sci_call(pnode, SCI_LINEFROMPOSITION, pos, 0);
                    eu_tabpage *p = on_result_other_tab((int)line, pnode->ret_vec);
                    if (!p)
                    {
                        on_search_jmp_line(pnode, pnode->ret_vec[line - 1].line, current_line);
                        eu_sci_call(pnode, SCI_SETSELECTION, pnode->ret_vec[line - 1].mark.start, pnode->ret_vec[line - 1].mark.end);
                    }
                    else if (on_tabpage_selection(p) >= 0)
                    {
                        on_search_jmp_line(p, pnode->ret_vec[line - 1].line, current_line);
                        eu_sci_call(p, SCI_SETSEL, pnode->ret_vec[line - 1].mark.start, pnode->ret_vec[line - 1].mark.end);
                    }
                }
            }
            return 1;
        }
        case WM_LBUTTONUP:
        {
            eu_tabpage *p = on_tabpage_from_handle(hwnd, on_tabpage_resultctl);
            if (RESULT_SHOW(p))
            {
                p->presult->nc_pos = eu_sci_call(p->presult, SCI_GETCURRENTPOS, 0, 0);
            }
            break;
        }
        case WM_RBUTTONUP:
        {
            eu_tabpage *p = on_tabpage_from_handle(hwnd, on_tabpage_resultctl);
            if (RESULT_SHOW(p))
            {
                p->presult->nc_pos = eu_sci_call(p->presult, SCI_GETCURRENTPOS, 0, 0);
                return menu_pop_track(hwnd, IDR_RESULT_MENU, 0, -1, on_result_menu_callback, p);
            }
            return 1;
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
                    eu_tabpage *p = on_tabpage_from_handle(hwnd, on_tabpage_resultctl);
                    if (RESULT_SHOW(p))
                    {
                        int mode = (int)eu_sci_call(p->presult, SCI_GETWRAPMODE, 0, 0);
                        !mode ? eu_sci_call(p->presult, SCI_SETWRAPMODE, 2, 0) : eu_sci_call(p->presult, SCI_SETWRAPMODE, 0, 0);
                    }
                    break;
                }
                case IDM_RESULT_CLEARALL:
                {
                    eu_tabpage *p = on_tabpage_from_handle(hwnd, on_tabpage_resultctl);
                    if (RESULT_SHOW(p))
                    {
                        eu_sci_call(p->presult, SCI_SETREADONLY, 0, 0);
                        eu_sci_call(p->presult, SCI_CLEARALL, 0, 0);
                        eu_sci_call(p->presult, SCI_SETREADONLY, 1, 0);
                    }
                    break;
                }
                case IDM_RESULT_CLOSE:
                {
                    eu_tabpage *p = on_tabpage_from_handle(hwnd, on_tabpage_resultctl);
                    if (p && p->presult)
                    {
                        on_result_free(&p->presult);
                        p->result_show = false;
                        eu_window_resize();
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case WM_THEMECHANGED:
        {
            eu_tabpage *p = on_tabpage_from_handle(hwnd, on_tabpage_resultctl);
            if (p && p->presult)
            {
                on_sci_default_theme(p->presult, -1);
            }
            break;
        }
        case WM_DESTROY:
        {
            ctrl_down = false;
            eu_logmsg("on_result_callback WM_DESTROY\n");
            break;
        }
    }
    return CallWindowProc((WNDPROC)eu_edit_wnd, hwnd, message, wParam, lParam);
}

void
on_result_free(eu_tabpage **ptr_result)
{
    if (ptr_result && *ptr_result)
    {
        if ((*ptr_result)->hwnd_sc)
        {
            SendMessage((*ptr_result)->hwnd_sc, WM_CLOSE, 0, 0);
            (*ptr_result)->hwnd_sc = NULL;
        }
        eu_safe_free((*ptr_result));
    }
}

void
on_result_reload(eu_tabpage *pedit)
{
    if (pedit)
    {   // disable margin
        eu_sci_call(pedit, SCI_SETMARGINS, 0, 0);
        // 强制使用unix回车符
        eu_sci_call(pedit, SCI_SETEOLMODE, SC_EOL_LF, 0);
        // 不显示插入符
        eu_sci_call(pedit, SCI_SETCARETSTYLE, CARETSTYLE_INVISIBLE, 0);
        // 需要时显示水平滚动条
        eu_sci_call(pedit, SCI_SETSCROLLWIDTH, 1, 0);
        eu_sci_call(pedit, SCI_SETSCROLLWIDTHTRACKING, 1, 0);
        // 加载词语解析器
        on_doc_key_scilexer(pedit, "result");
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
    bool ret = false;
    if (pnode)
    {
        bool split = true;
        const HWND h = eu_hwnd_self();
        const HWND htab = on_tabpage_hwnd(pnode);
        if (h && htab)
        {
            if (!g_splitter_editbar)
            {
                split = on_splitter_init_editbar(h);
            }
            if (!g_splitter_editbar2)
            {
                split = on_splitter_editbar_slave(h);
            }
        }
        if (split && !(ret ^= (pnode->presult != NULL)))
        {
            const int flags = WS_CHILD | WS_CLIPSIBLINGS | WS_VSCROLL | WS_HSCROLL | WS_CLIPCHILDREN | WS_EX_RTLREADING;
            if ((pnode->presult = (eu_tabpage *)calloc(1, sizeof(eu_tabpage))))
            {
                if (on_sci_create(pnode->presult, h, flags, on_result_edit_proc) == SKYLARK_OK)
                {
                    on_sci_default_theme(pnode->presult, -1);
                    ret = true;
                }
            }
        }
    }
    return ret;
}

HWND
eu_result_hwnd(eu_tabpage *pnode)
{
    if (pnode && pnode->presult)
    {
        return (pnode->presult->hwnd_sc);
    }
    return NULL;
}
