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

HWND g_splitter_treebar = NULL;
HWND g_splitter_symbar = NULL;
HWND g_splitter_tabbar = NULL;
HWND g_splitter_editbar = NULL;
HWND g_splitter_tablebar = NULL;
HWND g_splitter_minmap = NULL;

static inline int
on_splitter_tree_line(void)
{
    return (3 * SPLIT_WIDTH);
}

static inline int
on_splitter_tabbar_line(void)
{
    return (4 * SPLIT_WIDTH);
}

static inline void
on_splitter_drawing(const HWND hwnd, const HDC hdc)
{
    RECT rc = {0};
    GetClientRect(hwnd, &rc);
    FillRect(hdc, &rc, (HBRUSH)on_splitter_brush());
}

static inline int
on_splitter_min_width(void)
{
    int min_width = TABBAR_WIDTH_MIN;
    if (eu_get_config())
    {
        min_width = MAX(eu_get_config()->sym_tree_width, eu_get_config()->document_map_width);
        min_width = MAX(TABBAR_WIDTH_MIN, min_width) + TABS_SPLIT;
    }
    return min_width;
}

static void
on_splitter_rect_box(HWND hwnd, LPRECT r, const int offset)
{
    RECT rc_tree = {0};
    RECT rc_main = {0};
    RECT rc_client = {0};
    POINT client_top = {0};
    GetClientRect(hwnd, &rc_client);
    on_treebar_adjust_box(&rc_client, &rc_tree);
    GetWindowRect(hwnd, &rc_main);
    ClientToScreen(hwnd, &client_top);
    int toolbar_height = on_toolbar_get_height();
    int tree_hight = rc_client.bottom - rc_client.top - SPLIT_WIDTH - toolbar_height - on_statusbar_height() - offset;
    r->left = rc_tree.left;
    r->right = rc_tree.right;
    r->top = client_top.y - rc_main.top + toolbar_height + offset;
    r->bottom = r->top + tree_hight;
}

static inline int
on_splitter_absolute_height(const int y)
{
    return y + menu_height() + on_toolbar_get_height() + 5;
}

static HDC
on_splitter_drawing_line(HWND parent, LPRECT r, const int x, HPEN *ptr_pen)
{
    RECT rc = {0};
    HDC hdc = GetWindowDC(parent);
    HPEN hpen = CreatePen(PS_SOLID, SPLIT_WIDTH, 0);
    HPEN hold_pen = (HPEN)(SelectObject(hdc, hpen));
    SetROP2(hdc, R2_NOTXORPEN);
    if (!r)
    {
        r = &rc;
    }
    on_splitter_rect_box(parent, r, 0);
    MoveToEx(hdc, x, r->top, NULL);
    LineTo(hdc, x,  r->bottom);
    if (ptr_pen)
    {
        *ptr_pen = hold_pen;
    }
    else
    {
        SelectObject(hdc, hold_pen);
        DeleteObject(hpen);
    }
    return hdc;
}

static inline void
on_splitter_paint(const HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    on_splitter_drawing(hwnd, hdc);
    EndPaint(hwnd, &ps);
}

static LRESULT CALLBACK
on_splitter_callback_tabbar(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static int x = 0;
    switch (msg)
    {
        case WM_PAINT:
        {
            //on_splitter_paint(hwnd);
            RECT rc = {0};
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            GetClientRect(hwnd, &rc);
            FillRect(hdc, &rc, CreateSolidBrush(0x0000ff));
            EndPaint(hwnd, &ps);
            break;
        }
        case WM_THEMECHANGED:
        {
            SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, on_splitter_brush());
            break;
        }
        case WM_LBUTTONDOWN:
        {
            RECT rc;
            HDC hdc;
            HWND parent = GetParent(hwnd);
            GetWindowRect(hwnd, &rc);
            MapWindowPoints(HWND_DESKTOP, parent, (POINT*)(&rc), 2);
            x = rc.right + TABS_SPLIT/2;
            hdc = on_splitter_drawing_line(parent, NULL, x, NULL);
            ReleaseDC(parent, hdc);
            SetCapture(hwnd);
            break;
        }
        case WM_LBUTTONUP:
        {
            RECT rc;
            RECT rect_tree;
            HWND parent = GetParent(hwnd);
            HDC hdc = on_splitter_drawing_line(parent, &rect_tree, x, NULL);
            const int min_width = on_splitter_min_width();
            ReleaseDC(parent, hdc);
            ReleaseCapture();
            GetClientRect(parent, &rc);
            eu_get_config()->eu_tab.slave_size = rc.right - x + 2 * SPLIT_WIDTH;
            if (eu_get_config()->eu_tab.slave_size < min_width)
            {
                eu_get_config()->eu_tab.slave_size = min_width;
            }
            eu_logmsg("x = %d, lParam = %hd, slave_size = %d\n", x, (short)LOWORD(lParam), eu_get_config()->eu_tab.slave_size);
            on_proc_redraw(NULL);
            break;
        }
        case WM_MOUSEMOVE:
        {
            if ((wParam & MK_LBUTTON) == MK_LBUTTON && GetCapture() == hwnd)
            {
                RECT rect_tree;
                HWND parent = GetParent(hwnd);
                HPEN hpen = NULL;
                HDC hdc = on_splitter_drawing_line(parent, &rect_tree, x, &hpen);
                const int tcx = rect_tree.right - rect_tree.left;
                const int min_width = on_splitter_min_width();
                x = rect_tree.right - rect_tree.left + SPLIT_WIDTH;
                x += eu_get_config()->eu_tab.main_size + on_splitter_tabbar_line() + (short)LOWORD(lParam);
                if (tcx && x < tcx + min_width)
                {
                    x = tcx + min_width;
                }
                else if (x < min_width)
                {
                    x = min_width;
                }
                MoveToEx(hdc, x, rect_tree.top, NULL);
                LineTo(hdc, x,  rect_tree.bottom);
                if (hpen)
                {
                    hpen = (HPEN)SelectObject(hdc, hpen);
                    DeleteObject(hpen);
                }
                ReleaseDC(parent, hdc);
            }
            break;
        }
        case WM_DESTROY:
        {
            if (g_splitter_tabbar)
            {
                g_splitter_tabbar = NULL;
                x = 0;
                eu_logmsg("g_splitter_tabbar WM_DESTROY\n");
            }
            break;
        }
        default:
        {
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }
    }
    return 0;
}

static LRESULT CALLBACK
on_splitter_callback_treebar(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static int x;
    switch (msg)
    {
        case WM_THEMECHANGED:
        {
            SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, on_splitter_brush());
            break;
        }
        case WM_LBUTTONDOWN:
        {
            RECT rect_tree;
            HWND parent = GetParent(hwnd);
            on_treebar_adjust_box(NULL, &rect_tree);
            x = rect_tree.right - rect_tree.left + on_splitter_tree_line();
            HDC hdc = on_splitter_drawing_line(parent, &rect_tree, x, NULL);
            ReleaseDC(parent, hdc);
            SetCapture(hwnd);
            break;
        }
        case WM_LBUTTONUP:
        {
            RECT rect_tree;
            HWND parent = GetParent(hwnd);
            HDC hdc = on_splitter_drawing_line(parent, &rect_tree, x, NULL);
            ReleaseDC(parent, hdc);
            ReleaseCapture();
            eu_get_config()->file_tree_width = x - rect_tree.left - SPLIT_WIDTH / 2;
            if (eu_get_config()->file_tree_width < FILETREEBAR_WIDTH_MIN)
            {
                eu_get_config()->file_tree_width = FILETREEBAR_WIDTH_MIN;
            }
            on_proc_redraw(NULL);
            break;
        }
        case WM_MOUSEMOVE:
        {
            if ((wParam & MK_LBUTTON) == MK_LBUTTON && GetCapture() == hwnd)
            {
                RECT rect_tree;
                HWND parent = GetParent(hwnd);
                HPEN hpen = NULL;
                HDC hdc = on_splitter_drawing_line(parent, &rect_tree, x, &hpen);
                x = rect_tree.right - rect_tree.left + on_splitter_tree_line() + (short)LOWORD(lParam);
                MoveToEx(hdc, x, rect_tree.top, NULL);
                LineTo(hdc, x,  rect_tree.bottom);
                if (hpen)
                {
                    hpen = (HPEN)SelectObject(hdc, hpen);
                    DeleteObject(hpen);
                }
                ReleaseDC(parent, hdc);
            }
            break;
        }
        case WM_DESTROY:
        {
            if (g_splitter_treebar)
            {
                g_splitter_treebar = NULL;
                x = 0;
                eu_logmsg("g_splitter_treebar WM_DESTROY\n");
            }
            break;
        }
        default:
        {
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }
    }
    return 0;
}

static LRESULT CALLBACK
on_splitter_callback_symbar(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static int cx;
    eu_tabpage *pnode = NULL;
    switch (msg)
    {
        case WM_THEMECHANGED:
        {
            SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, on_splitter_brush());
            break;
        }
        case WM_LBUTTONDOWN:
        {
            RECT r;
            HWND parent = GetParent(hwnd);
            HDC hdc = GetWindowDC(parent);
            if ((pnode = on_tabpage_focus_at()) != NULL)
            {
                const int offset = on_tabpage_get_height(HSLAVE_SHOW ? 1 : 0) + 1;
                if (pnode->sym_show)
                {
                    cx = pnode->rect_sym.left +  on_splitter_tree_line();
                }
                else
                {
                    cx = pnode->rect_sc.right;
                }
                HPEN hpen = CreatePen(PS_SOLID, SPLIT_WIDTH, 0);
                HPEN hold_pen = (HPEN)(SelectObject(hdc, hpen));
                SetROP2(hdc, R2_NOTXORPEN);
                on_splitter_rect_box(parent, &r, offset);
                MoveToEx(hdc, cx, r.top, NULL);
                LineTo(hdc, cx,  r.bottom);
                SelectObject(hdc, hold_pen);
                DeleteObject(hpen);
            }
            ReleaseDC(parent, hdc);
            SetCapture(hwnd);
            break;
        }
        case WM_LBUTTONUP:
        {
            ReleaseCapture();
            if ((pnode = on_tabpage_focus_at()) && pnode->sym_show)
            {
                if (pnode->hwnd_symlist)
                {
                    eu_get_config()->sym_list_width = pnode->rect_sym.right - cx - SPLIT_WIDTH / 2;
                    if (eu_get_config()->sym_list_width < SYMBOLLIST_WIDTH_MIN)
                    {
                        eu_get_config()->sym_list_width = SYMBOLLIST_WIDTH_MIN;
                    }
                }
                else if (pnode->hwnd_symtree)
                {
                    eu_get_config()->sym_tree_width = pnode->rect_sym.right - cx - SPLIT_WIDTH / 2;
                    if (eu_get_config()->sym_tree_width < TREEVIEW_WIDTH_MIN)
                    {
                        eu_get_config()->sym_tree_width = TREEVIEW_WIDTH_MIN;
                    }
                }
                eu_window_resize();
            }
            break;
        }
        case WM_MOUSEMOVE:
        {
            if ((wParam & MK_LBUTTON) == MK_LBUTTON && GetCapture() == hwnd)
            {
                RECT r;
                HWND parent = GetParent(hwnd);
                HDC hdc = GetWindowDC(parent);
                const int offset = on_tabpage_get_height(HSLAVE_SHOW ? 1 : 0) + 1;
                if ((pnode = on_tabpage_focus_at()) != NULL && pnode->sym_show)
                {
                    HPEN hpen = CreatePen(PS_SOLID, SPLIT_WIDTH, 0);
                    HPEN hold_pen = (HPEN)(SelectObject(hdc, hpen));
                    SetROP2(hdc, R2_NOTXORPEN);
                    on_splitter_rect_box(parent, &r, offset);
                    MoveToEx(hdc, cx, r.top, NULL);
                    LineTo(hdc, cx,  r.bottom);
                    cx = pnode->rect_sym.left + (short)LOWORD(lParam) + on_splitter_tree_line();
                    /* 这里应该限制cx, 不要覆盖treebar */
                    MoveToEx(hdc, cx, r.top, NULL);
                    LineTo(hdc, cx,  r.bottom);
                    SelectObject(hdc, hold_pen);
                    DeleteObject(hpen);
                }
                ReleaseDC(parent, hdc);
            }
            break;
        }
        default:
        {
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }
    }
    return 0;
}

static LRESULT CALLBACK
on_splitter_callback_minmap(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static int cx;
    eu_tabpage *pnode = NULL;
    switch (msg)
    {
        case WM_THEMECHANGED:
        {
            SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, on_splitter_brush());
            break;
        }
        case WM_LBUTTONDOWN:
        {
            RECT r;
            HWND parent = GetParent(hwnd);
            HDC hdc = GetWindowDC(parent);
            if ((pnode = on_tabpage_focus_at()) != NULL && pnode->map_show)
            {
                cx = pnode->rect_map.left +  on_splitter_tree_line();
                HPEN hpen = CreatePen(PS_SOLID, SPLIT_WIDTH, 0);
                HPEN hold_pen = (HPEN)(SelectObject(hdc, hpen));
                SetROP2(hdc, R2_NOTXORPEN);
                on_splitter_rect_box(parent, &r, 0);
                MoveToEx(hdc, cx, r.top, NULL);
                LineTo(hdc, cx,  r.bottom);
                SelectObject(hdc, hold_pen);
                DeleteObject(hpen);
            }
            ReleaseDC(parent, hdc);
            SetCapture(hwnd);
            break;
        }
        case WM_LBUTTONUP:
        {
            ReleaseCapture();
            if ((pnode = on_tabpage_focus_at()) && pnode->map_show)
            {
                eu_get_config()->document_map_width = pnode->rect_map.right - cx - SPLIT_WIDTH / 2;
                if (eu_get_config()->document_map_width < DOCUMENTMAP_WIDTH_MIN)
                {
                    eu_get_config()->document_map_width = DOCUMENTMAP_WIDTH_MIN;
                }
                on_proc_redraw(NULL);
            }
            break;
        }
        case WM_MOUSEMOVE:
        {
            if ((wParam & MK_LBUTTON) == MK_LBUTTON && GetCapture() == hwnd)
            {
                RECT r;
                HWND parent = GetParent(hwnd);
                HDC hdc = GetWindowDC(parent);
                if ((pnode = on_tabpage_focus_at()) != NULL && pnode->map_show)
                {
                    HPEN hpen = CreatePen(PS_SOLID, SPLIT_WIDTH, 0);
                    HPEN hold_pen = (HPEN)(SelectObject(hdc, hpen));
                    SetROP2(hdc, R2_NOTXORPEN);
                    on_splitter_rect_box(parent, &r, 0);
                    MoveToEx(hdc, cx, r.top, NULL);
                    LineTo(hdc, cx,  r.bottom);
                    cx = pnode->rect_map.left + (short)LOWORD(lParam) + on_splitter_tree_line();
                    /* 这里应该限制cx, 不要覆盖treebar */
                    MoveToEx(hdc, cx, r.top, NULL);
                    LineTo(hdc, cx,  r.bottom);
                    SelectObject(hdc, hold_pen);
                    DeleteObject(hpen);
                }
                ReleaseDC(parent, hdc);
            }
            break;
        }
        default:
        {
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }
    }
    return 0;
}

static LRESULT CALLBACK
on_splitter_callback_editbar(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static int cy;
    HWND parent = NULL;
    eu_tabpage *pnode = NULL;
    switch (msg)
    {
        case WM_THEMECHANGED:
        {
            SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, on_splitter_brush());
            break;
        }
        case WM_LBUTTONDOWN:
        {
            parent = GetParent(hwnd);
            pnode = on_tabpage_focus_at();
            cy = on_splitter_absolute_height(pnode->rect_sc.bottom);
            HDC hdc = GetWindowDC(parent);
            HPEN hpen = CreatePen(PS_SOLID, SPLIT_WIDTH, 0);
            HPEN hold_pen = (HPEN)SelectObject(hdc, hpen);
            SetROP2(hdc, R2_NOTXORPEN);
            MoveToEx(hdc, pnode->rect_sc.left + SPLIT_WIDTH, cy, NULL);
            LineTo(hdc, pnode->rect_sc.right, cy);
            SelectObject(hdc, hold_pen);
            DeleteObject(hpen);
            ReleaseDC(parent, hdc);
            SetCapture(hwnd);
            break;
        }
        case WM_LBUTTONUP:
        {
            parent = GetParent(hwnd);
            pnode = on_tabpage_focus_at();
            HDC hdc = GetWindowDC(parent);
            HPEN hpen = CreatePen(PS_SOLID, SPLIT_WIDTH, 0);
            HPEN hold_pen = (HPEN)SelectObject(hdc, hpen);
            SetROP2(hdc, R2_NOTXORPEN);
            MoveToEx(hdc, pnode->rect_sc.left + SPLIT_WIDTH, cy, NULL);
            LineTo(hdc, pnode->rect_sc.right,  cy);
            SelectObject(hdc, hold_pen);
            ReleaseDC(parent, hdc);
            DeleteObject(hpen);
            ReleaseCapture();
            if (RESULT_SHOW(pnode) && eu_result_hwnd(pnode))
            {
                eu_get_config()->result_edit_height = on_splitter_absolute_height(pnode->rect_result.bottom) - cy - SPLIT_WIDTH;
            }
            if (eu_get_config()->result_edit_height < SQLQUERYRESULT_LISTVIEW_HEIGHT_MIN)
            {
                eu_get_config()->result_edit_height = SQLQUERYRESULT_LISTVIEW_HEIGHT_MIN;
            }
            eu_window_resize();
            break;
        }
        case WM_MOUSEMOVE:
        {
            if ((wParam & MK_LBUTTON) == MK_LBUTTON && GetCapture() == hwnd)
            {
                parent = GetParent(hwnd);
                pnode = on_tabpage_focus_at();
                HDC hdc = GetWindowDC(parent);
                HPEN hpen = CreatePen(PS_SOLID, SPLIT_WIDTH, 0);
                HPEN hold_pen = (HPEN)(SelectObject(hdc, hpen));
                SetROP2(hdc, R2_NOTXORPEN);
                MoveToEx(hdc, pnode->rect_sc.left + SPLIT_WIDTH, cy, NULL);
                LineTo(hdc, pnode->rect_sc.right,  cy);
                cy = on_splitter_absolute_height(pnode->rect_sc.bottom) + (short)HIWORD(lParam);
                MoveToEx(hdc, pnode->rect_sc.left + SPLIT_WIDTH, cy, NULL);
                LineTo(hdc, pnode->rect_sc.right,  cy);
                SelectObject(hdc, hold_pen);
                DeleteObject(hpen);
                ReleaseDC(parent, hdc);
            }
            break;
        }
        default:
        {
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }
    }
    return 0;
}

static LRESULT CALLBACK
on_splitter_callback_tablebar(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static int y;
    eu_tabpage *pnode = NULL;
    switch (msg)
    {
        case WM_THEMECHANGED:
        {
            SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, on_splitter_brush());
            break;
        }
        case WM_LBUTTONDOWN:
        {
            HWND parent = GetParent(hwnd);
            pnode = on_tabpage_focus_at();
            y = on_splitter_absolute_height(pnode->rect_result.bottom);
            HDC hdc = GetWindowDC(parent);
            HPEN hpen = CreatePen(PS_SOLID, SPLIT_WIDTH, 0);
            HPEN hold_pen = (HPEN)(SelectObject(hdc, hpen));
            SetROP2(hdc, R2_NOTXORPEN);
            MoveToEx(hdc, pnode->rect_sc.left + SPLIT_WIDTH, y, NULL);
            LineTo(hdc, pnode->rect_sc.right,  y);
            SelectObject(hdc, hold_pen);
            DeleteObject(hpen);
            ReleaseDC(parent, hdc);
            SetCapture(hwnd);
            break;
        }
        case WM_LBUTTONUP:
        {
            HWND parent = GetParent(hwnd);
            pnode = on_tabpage_focus_at();
            HDC hdc = GetWindowDC(parent);
            HPEN hpen = CreatePen(PS_SOLID, SPLIT_WIDTH, 0);
            HPEN hold_pen = (HPEN)(SelectObject(hdc, hpen));
            SetROP2(hdc, R2_NOTXORPEN);
            MoveToEx(hdc, pnode->rect_sc.left + SPLIT_WIDTH, y, NULL);
            LineTo(hdc, pnode->rect_sc.right,  y);
            SelectObject(hdc, hold_pen);
            DeleteObject(hpen);
            ReleaseDC(parent, hdc);
            ReleaseCapture();
            int m_height = on_splitter_absolute_height(pnode->rect_qrtable.bottom) - y - SPLIT_WIDTH/2;
            if (m_height >= SQLQUERYRESULT_LISTVIEW_HEIGHT_MIN)
            {
                eu_get_config()->result_list_height = m_height;
            }
            eu_window_resize();
            break;
        }
        case WM_MOUSEMOVE:
        {
            if ((wParam & MK_LBUTTON) == MK_LBUTTON && GetCapture() == hwnd)
            {
                HWND parent = GetParent(hwnd);
                pnode = on_tabpage_focus_at();
                HDC hdc = GetWindowDC(parent);
                HPEN hpen = CreatePen(PS_SOLID, SPLIT_WIDTH, 0);
                HPEN hold_pen = (HPEN)(SelectObject(hdc, hpen));
                SetROP2(hdc, R2_NOTXORPEN);
                MoveToEx(hdc, pnode->rect_sc.left + SPLIT_WIDTH, y, NULL);
                LineTo(hdc, pnode->rect_sc.right,  y);
                y = on_splitter_absolute_height(pnode->rect_result.bottom) + (short)HIWORD(lParam);
                MoveToEx(hdc, pnode->rect_sc.left + SPLIT_WIDTH, y, NULL);
                LineTo(hdc, pnode->rect_sc.right,  y);
                SelectObject(hdc, hold_pen);
                DeleteObject(hpen);
                ReleaseDC(parent, hdc);
            }
            break;
        }
        default:
        {
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }
    }
    return 0;
}

static bool
on_splitter_register(const TCHAR *classname, WNDPROC proc, int cur_id)
{
    WNDCLASSEX wcex = {sizeof(WNDCLASSEX)};
    wcex.style			= CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = proc;
    wcex.hInstance = eu_module_handle();
    wcex.hCursor = LoadCursor(wcex.hInstance, cur_id ? MAKEINTRESOURCE(cur_id) : IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)on_splitter_brush();
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = classname;
    return (RegisterClassEx(&wcex) > 0);
}

intptr_t
on_splitter_brush(void)
{
    return (eu_theme_index() == THEME_WHITE ? (intptr_t)GetSysColorBrush(COLOR_MENU) : on_dark_get_bgbrush());
}

void
on_splitter_redraw(void)
{
    HWND hrst = NULL;
    if (g_splitter_treebar)
    {
        SendMessage(g_splitter_treebar, WM_THEMECHANGED, 0, 0);
        UpdateWindowEx(g_splitter_treebar);
    }
    if (g_splitter_symbar)
    {
        SendMessage(g_splitter_symbar, WM_THEMECHANGED, 0, 0);
        UpdateWindowEx(g_splitter_symbar);
    }
    if (g_splitter_tabbar)
    {
        SendMessage(g_splitter_tabbar, WM_THEMECHANGED, 0, 0);
        UpdateWindowEx(g_splitter_tabbar);
    }
    if (g_splitter_editbar)
    {
        SendMessage(g_splitter_editbar, WM_THEMECHANGED, 0, 0);
        UpdateWindowEx(g_splitter_editbar);
    }
    if (g_splitter_tablebar)
    {
        SendMessage(g_splitter_tablebar, WM_THEMECHANGED, 0, 0);
        UpdateWindowEx(g_splitter_tablebar);
    }
    if (g_splitter_minmap)
    {
        SendMessage(g_splitter_minmap, WM_THEMECHANGED, 0, 0);
        UpdateWindowEx(g_splitter_minmap);
    }
}

bool
on_splitter_init_treebar(HWND parent)
{
    const TCHAR *splite_class = _T("splitter_treebar_scintilla");
    on_splitter_register(splite_class, on_splitter_callback_treebar, IDC_CURSOR_WE);
    g_splitter_treebar = CreateWindowEx(0, splite_class, _T(""), WS_CHILD | WS_CLIPSIBLINGS, 0, 0, 0, 0, parent, 0, eu_module_handle(), 0);
    return (g_splitter_treebar != NULL);
}

bool
on_splitter_init_symbar(HWND parent)
{
    const TCHAR *splite_class = _T("splitter_symbar_scintilla");
    on_splitter_register(splite_class, on_splitter_callback_symbar, IDC_CURSOR_WE);
    g_splitter_symbar = CreateWindowEx(0, splite_class, _T(""), WS_CHILD | WS_CLIPSIBLINGS, 0, 0, 0, 0, parent, 0, eu_module_handle(), 0);
    return (g_splitter_symbar != NULL);
}

bool
on_splitter_init_minmap(HWND parent)
{
    const TCHAR *splite_class = _T("splitter_minmap_scintilla");
    on_splitter_register(splite_class, on_splitter_callback_minmap, IDC_CURSOR_WE);
    g_splitter_minmap = CreateWindowEx(0, splite_class, _T(""), WS_CHILD | WS_CLIPSIBLINGS, 0, 0, 0, 0, parent, 0, eu_module_handle(), 0);
    return (g_splitter_minmap != NULL);
}

bool
on_splitter_init_tabbar(HWND parent)
{
    const TCHAR *splite_class = _T("splitter_tabbar_scintilla");
    on_splitter_register(splite_class, on_splitter_callback_tabbar, IDC_CURSOR_WE);
    g_splitter_tabbar = CreateWindowEx(0, splite_class, _T(""), WS_CHILD | WS_CLIPSIBLINGS, 0, 0, 0, 0, parent, 0, eu_module_handle(), 0);
    return (g_splitter_tabbar != NULL);
}

bool
on_splitter_init_editbar(HWND parent)
{
    const TCHAR *splite_class = _T("splitter_scintilla_editbar");
    on_splitter_register(splite_class, on_splitter_callback_editbar, IDC_CURSOR_NS);
    g_splitter_editbar = CreateWindowEx(0, splite_class, _T(""), WS_CHILD | WS_CLIPSIBLINGS, 0, 0, 0, 0, parent, 0, eu_module_handle(), 0);
    return (g_splitter_editbar != NULL);
}

bool
on_splitter_init_tablebar(HWND parent)
{
    const TCHAR *splite_class = _T("splitter_editbar_tablebar");
    on_splitter_register(splite_class, on_splitter_callback_tablebar, IDC_CURSOR_NS);
    g_splitter_tablebar = CreateWindowEx(0, splite_class, _T(""), WS_CHILD | WS_CLIPSIBLINGS, 0, 0, 0, 0, parent, 0, eu_module_handle(), 0);
    return (g_splitter_tablebar != NULL);
}

HWND
on_splitter_init_window(HWND parent, const TCHAR *class_name, const int flags, HMENU hmenu, WNDPROC proc, void *lp)
{
    on_splitter_register(class_name, proc, 0);
    return CreateWindowEx(0, class_name, _T(""), flags, 0, 0, 0, 0, parent, hmenu, eu_module_handle(), lp);
}

HWND
on_splitter_static_control(HWND parent, WNDPROC proc, void *lp)
{
    const uint32_t flags = WS_CHILD | WS_CLIPSIBLINGS;
    on_splitter_register(_T("minmap_tab"), proc, 0);
    return CreateWindowEx(0, _T("minmap_tab"), _T(""), flags, 0, 0, 0, 0, parent, NULL, eu_module_handle(), lp);
}
