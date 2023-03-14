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
HWND g_splitter_editbar = NULL;
HWND g_splitter_tablebar = NULL;

static void
on_splitter_drawing(HWND hwnd, HDC hdc)
{
    RECT rc = {0};
    GetClientRect(hwnd, &rc);
    FillRect(hdc, &rc, (HBRUSH)on_dark_get_brush());
}

static void
on_splitter_rect_box(HWND hwnd, LPRECT r, int offset)
{
    RECT rc_tree = {0};
    RECT rc_main = {0};
    RECT rc_client = {0};
    POINT client_top = {0};
    on_treebar_adjust_box(&rc_tree);
    GetWindowRect(hwnd, &rc_main);
    GetClientRect(hwnd, &rc_client);
    ClientToScreen(hwnd, &client_top);
    int toolbar_height = on_toolbar_height();
    int tree_hight = rc_client.bottom - rc_client.top - SPLIT_WIDTH - toolbar_height - on_statusbar_height() - offset;
    r->left = rc_tree.left;
    r->right = rc_tree.right;
    r->top = client_top.y - rc_main.top + toolbar_height + offset;
    r->bottom = r->top + tree_hight;
}

static int
on_splitter_absolute_height(int y)
{
    return y+ menu_height() + on_toolbar_height() + 5;
}

static HDC
on_splitter_drawing_line(HWND parent, LPRECT r, int x, HPEN *ptr_pen)
{
    HDC hdc = GetWindowDC(parent);
    HPEN hpen = CreatePen(PS_SOLID, SPLIT_WIDTH, 0);
    HPEN hold_pen = (HPEN)(SelectObject(hdc, hpen));
    SetROP2(hdc, R2_NOTXORPEN);
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

static LRESULT CALLBACK
on_splitter_callback_treebar(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static int x;
    switch (msg)
    {
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            on_splitter_drawing(hwnd, hdc);
            EndPaint(hwnd, &ps);
            break;
        }
        case WM_LBUTTONDOWN:
        {
            RECT rect_tree;
            HWND parent = GetParent(hwnd);
            on_treebar_adjust_box(&rect_tree);
            x = rect_tree.right - rect_tree.left + 3 * SPLIT_WIDTH;
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
            eu_window_resize(parent);
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
                x = rect_tree.right - rect_tree.left + (short)LOWORD(lParam);
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
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            on_splitter_drawing(hwnd, hdc);
            EndPaint(hwnd, &ps);
            break;
        }
        case WM_LBUTTONDOWN:
        {
            RECT r;
            HWND parent = GetParent(hwnd);
            HDC hdc = GetWindowDC(parent);
            if ((pnode = on_tabpage_focus_at()) != NULL)
            {
                if (pnode->sym_show)
                {
                    cx = pnode->rect_sym.left +  3 * SPLIT_WIDTH;
                }
                else if (pnode->map_show)
                {
                    cx = pnode->rect_map.left +  3 * SPLIT_WIDTH;
                }
                else
                {
                    cx = pnode->rect_sc.right;
                }
                HPEN hpen = CreatePen(PS_SOLID, SPLIT_WIDTH, 0);
                HPEN hold_pen = (HPEN)(SelectObject(hdc, hpen));
                SetROP2(hdc, R2_NOTXORPEN);
                on_splitter_rect_box(parent, &r, on_tabpage_get_height() + SCINTILLA_MARGIN_TOP);
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
            if (!(pnode = on_tabpage_focus_at()))
            {
                break;
            }
            if (pnode->sym_show)
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
            }
            else if (pnode->map_show)
            {
                eu_get_config()->document_map_width = pnode->rect_map.right - cx - SPLIT_WIDTH / 2;
                if (eu_get_config()->document_map_width < DOCUMENTMAP_WIDTH_MIN)
                {
                    eu_get_config()->document_map_width = DOCUMENTMAP_WIDTH_MIN;
                }
            }
            eu_window_resize(NULL);
            break;
        }
        case WM_MOUSEMOVE:
        {
            if ((wParam & MK_LBUTTON) == MK_LBUTTON && GetCapture() == hwnd)
            {
                RECT r;
                HWND parent = GetParent(hwnd);
                HDC hdc = GetWindowDC(parent);
                if ((pnode = on_tabpage_focus_at()) != NULL)
                {
                    HPEN hpen = CreatePen(PS_SOLID, SPLIT_WIDTH, 0);
                    HPEN hold_pen = (HPEN)(SelectObject(hdc, hpen));
                    SetROP2(hdc, R2_NOTXORPEN);
                    on_splitter_rect_box(parent, &r, on_tabpage_get_height() + SCINTILLA_MARGIN_TOP);
                    MoveToEx(hdc, cx, r.top, NULL);
                    LineTo(hdc, cx,  r.bottom);
                    if (pnode->sym_show)
                    {
                        cx = pnode->rect_sym.left + (short)LOWORD(lParam);
                    }
                    else if (pnode->map_show)
                    {
                        cx = pnode->rect_map.left + (short)LOWORD(lParam);
                    }
                    else
                    {
                        cx = pnode->rect_sc.right + (short)LOWORD(lParam);
                    }
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
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            on_splitter_drawing(hwnd, hdc);
            EndPaint(hwnd, &ps);
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
            if (RESULT_SHOW(pnode) && eu_result_hwnd())
            {
                if (pnode->hwnd_qrtable)
                {
                    eu_get_config()->result_edit_height = on_splitter_absolute_height(pnode->rect_result.bottom) - cy - SPLIT_WIDTH/2;
                }
                else
                {
                    eu_get_config()->result_edit_height = on_splitter_absolute_height(pnode->rect_result.bottom) - cy - SPLIT_WIDTH - eu_get_config()->result_list_height;
                }
            }
            if (eu_get_config()->result_edit_height < SQLQUERYRESULT_LISTVIEW_HEIGHT_MIN)
            {
                eu_get_config()->result_edit_height = SQLQUERYRESULT_LISTVIEW_HEIGHT_MIN;
            }
            eu_window_resize(NULL);
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
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            on_splitter_drawing(hwnd, hdc);
            EndPaint(hwnd, &ps);
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
            eu_window_resize(NULL);
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
    wcex.lpfnWndProc = proc;
    wcex.hInstance = eu_module_handle();
    wcex.hCursor = LoadCursor(wcex.hInstance, cur_id ? MAKEINTRESOURCE(cur_id) : IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = classname;
    return (RegisterClassEx(&wcex) > 0);
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
    const TCHAR *splite_class = _T("splitter_scintilla_symbar");
    on_splitter_register(splite_class, on_splitter_callback_symbar, IDC_CURSOR_WE);
    g_splitter_symbar = CreateWindowEx(0, splite_class, _T(""), WS_CHILD | WS_CLIPSIBLINGS, 0, 0, 0, 0, parent, 0, eu_module_handle(), 0);
    return (g_splitter_symbar != NULL);
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
