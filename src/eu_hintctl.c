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

static eu_tabpage *phint = NULL;
static volatile long code_hint_initialized = 0;

static void
on_hint_reload(const char **pbuf)
{
    if (phint)
    {
        on_sci_default_fonts(phint, -1);
        // 设置一个页边缩进
        on_sci_set_margin(phint);
        // 回车符
        eu_sci_call(phint, SCI_SETEOLMODE, phint->eol, 0);
        // 不显示插入符
        eu_sci_call(phint, SCI_SETCARETSTYLE, CARETSTYLE_INVISIBLE, 0);
        // 插入缓冲区代码
        for (size_t i = 0; i < cvector_size(pbuf); ++i)
        {
            eu_sci_call(phint, SCI_ADDTEXT, strlen(pbuf[i]), (LPARAM)(pbuf[i]));
        }
    }
}

static void
on_hint_move_sci(eu_tabpage *p, int width, int height)
{
    if (p && p->hwnd_sc)
    {
        MoveWindow(p->hwnd_sc, 0, 0, width, height, 0);
        ShowWindow(p->hwnd_sc, SW_SHOW);
    }
}

static LRESULT CALLBACK
on_hint_code_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_DESTROY:
        {
            eu_logmsg("%s: destroy\n", __FUNCTION__);
            break;
        }
    }
    return CallWindowProc((WNDPROC)eu_edit_wnd, hwnd, message, wParam, lParam);
}

static LRESULT CALLBACK
on_hint_callback(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_CREATE:
        {
            return ShowWindow(hwnd, SW_HIDE);
        }
        case WM_SIZE:
        {
            on_hint_move_sci(phint, LOWORD(lParam), HIWORD(lParam));
            break;
        }
        case WM_DESTROY:
        {
            eu_logmsg("%s: destroy\n", __FUNCTION__);
            eu_safe_free(phint);
            break;
        }
        default:
        {
            return DefWindowProc(hwnd, message, wParam, lParam);
        }
    }
    return 0;
}

static bool
on_hint_create(const HWND parent)
{
    eu_safe_free(phint);
    if (!phint && (phint = (eu_tabpage *)calloc(1, sizeof(eu_tabpage))))
    {
        const TCHAR *class_name = _T("Code Hint");
        phint->reserved0 = (intptr_t)on_splitter_init_window(parent, class_name, WS_CHILD | WS_CLIPSIBLINGS, NULL, on_hint_callback, NULL);
        return phint->reserved0 != 0;
    }
    return false;
}

HWND
on_hint_hwnd(void)
{
    if (phint && phint->reserved0)
    {
        return ((HWND)phint->reserved0);
    }
    return NULL;
}

void
on_hint_hide(const POINT *pt)
{
    if (pt && phint && phint->reserved0 && phint->hwnd_sc && code_hint_initialized && !PtInRect(&phint->rect_sc, *pt))
    {
        DestroyWindow((HWND)phint->reserved0);
        _InterlockedExchange(&code_hint_initialized, 0);
    }
}

bool
on_hint_initialized(void)
{
    return !_InterlockedCompareExchange(&code_hint_initialized, 1, 0);
}

bool
on_hint_launch(eu_tabpage *pnode, const RECT *prc, const char **pbuf, const int line_count, const int line_max, const bool downward)
{
    if (pnode && prc && on_hint_create(pnode->hwnd_sc))
    {
        RECT rc = {0};
        long r1 = 0;
        long r2 = 0;
        int flags = WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_EX_RTLREADING;
        const sptr_t font_width = eu_sci_call(pnode, SCI_TEXTWIDTH, STYLE_DEFAULT, (sptr_t)"X");
        const sptr_t font_hight = eu_sci_call(pnode, SCI_TEXTHEIGHT, 0, 0);
        memcpy(&phint->rect_sc, prc, sizeof(RECT));
        phint->eol = pnode->eol;
        if (downward)
        {
            rc.left = prc->left;
            rc.top = prc->top + (long)font_hight * 2;
            rc.right = (long)MIN((r1 = pnode->rect_sc.right - pnode->rect_sc.left - 28), (r2 = (long)(rc.left + (line_max + 3) * font_width)));
            rc.bottom = rc.top + (long)(line_count * font_hight);
        }
        else
        {
            rc.left = prc->left;
            rc.right = (long)MIN((r1 = pnode->rect_sc.right - pnode->rect_sc.left - 28), (r2 = (long)(rc.left + (line_max + 3) * font_width)));
            rc.bottom = prc->bottom - (long)font_hight * 2;
            rc.top = rc.bottom - (long)(line_count * font_hight);
        }
        if (r1 < r2)
        {
            rc.left = (long)(pnode->reserved1 > 0 ? pnode->reserved1 : rc.left);
        }
        if (on_sci_create(phint, (HWND)phint->reserved0, flags, on_hint_code_proc) == SKYLARK_OK)
        {
            on_dark_border(phint->hwnd_sc, true);
            on_hint_reload(pbuf);
        }
        return SetWindowPos((HWND)phint->reserved0, HWND_TOP, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_SHOWWINDOW);
    }
    return false;
}
