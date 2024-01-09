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

#define DLG_FOCUS (RGB(0xFF, 0x80, 0x00))
#define DLG_FROST (RGB(0xFF, 0xFF, 0xFF))
#define WINE_BACK_COLOR 0x1E1E1E

static volatile sptr_t canvas_default_proc = 0;
static volatile long higher_y = 0;
static volatile long lower_y = 0;
static HWND hwnd_document_static = NULL;
static HWND hwnd_static_control = NULL;

typedef enum _move_mode
{
    per_line,
    per_page
} move_mode;

static const int
on_map_viewer_height(void)
{
    return (lower_y - higher_y);
}

static const int
on_map_center_pos(void)
{
    return (lower_y - higher_y)/2 + higher_y;
}

static void
on_map_draw_zone(long h, long l)
{
    higher_y = h;
    lower_y = l;
    if (NULL != hwnd_document_static)
    {
        InvalidateRect(hwnd_document_static, NULL, TRUE);
    }
}

static void
on_map_get_fold(eu_tabpage *pnode, sptr_t **plines)
{
    sptr_t header_line = 0;
    do
    {   // SCI_CONTRACTEDFOLDNEXT比检查文档的每一行快10%-50%
        header_line = eu_sci_call(pnode, SCI_CONTRACTEDFOLDNEXT, header_line, 0);
        if (header_line != -1)
        {
            cvector_push_back(*plines, header_line);
            ++header_line;
        }
    } while (header_line != -1);
}

static void
on_map_contract_fold(eu_tabpage *pedit, sptr_t *plines)
{
    if (pedit && plines)
    {
        int i = 0;
        sptr_t *it;
        for (it = cvector_begin(plines); it != cvector_end(plines); ++it)
        {
            on_code_do_fold(pedit, SC_FOLDACTION_CONTRACT, *it, true);
            ++i;
        }
    }
}

static void
on_map_sync_fold(eu_tabpage *pnode, eu_tabpage *pedit)
{
    if (pnode && pedit)
    {
        cvector_vector_type(sptr_t) v = NULL;
        on_map_get_fold(pnode, &v);
        if (cvector_size(v) > 0)
        {
            on_map_contract_fold(pedit, v);
        }
        cvector_free(v);
    }
}

#ifdef APP_DEBUG
static void
on_map_print(sptr_t *plines)
{
    if (plines)
    {
        int i = 0;
        sptr_t *it;
        for (it = cvector_begin(plines); it != cvector_end(plines); ++it)
        {
            printf("plines[%d] = %zd\n", i, *it);
            ++i;
        }
    }
}
#endif

void
on_map_scroll(eu_tabpage *pnode, eu_tabpage *ptr_map)
{
    if (pnode && ptr_map)
    {
        // 从主编辑器视图中获取第一个和最后一个显示字符的位置
        sptr_t higher_pos = eu_sci_call(pnode, SCI_POSITIONFROMPOINT, 0, 0);
        sptr_t lower_pos = eu_sci_call(pnode, SCI_POSITIONFROMPOINT, pnode->rect_sc.right - pnode->rect_sc.left, pnode->rect_sc.bottom - pnode->rect_sc.top);

        eu_sci_call(ptr_map, SCI_GOTOPOS, higher_pos, 0);
        eu_sci_call(ptr_map, SCI_GOTOPOS, lower_pos, 0);

        RECT rc_map;
        GetClientRect(ptr_map->hwnd_sc, &rc_map);
        sptr_t hy = eu_sci_call(ptr_map, SCI_POINTYFROMPOSITION, 0, higher_pos);

        sptr_t map_line_height  = eu_sci_call(ptr_map, SCI_TEXTHEIGHT, 0, 0);
        sptr_t ly = eu_sci_call(ptr_map, SCI_POINTYFROMPOSITION, 0, lower_pos) + map_line_height;
        // 标记文档视图中的显示区域
        on_map_draw_zone((long)hy, (long)ly);
    }
}

static void
on_map_model_scroll(eu_tabpage *pnode, eu_tabpage *ptr_map, bool direction, move_mode mode)
{
    sptr_t nb_line = eu_sci_call(pnode, SCI_LINESONSCREEN, 0, 0);
    sptr_t line2go = (mode == per_line ? 1 : nb_line);
    eu_sci_call(pnode, SCI_LINESCROLL, 0, (direction == (bool)MOVE_DOWN) ? line2go : -line2go);
    on_map_scroll(pnode, ptr_map);
}

static void
on_map_draw_static_zone(DRAWITEMSTRUCT *pdis)
{
    HDC hdc = pdis->hDC;
    RECT rc = pdis->rcItem;
    HBRUSH hbrush_bg = CreateSolidBrush(DLG_FROST);
    HBRUSH hbrush_fg = CreateSolidBrush(DLG_FOCUS);
    FillRect(hdc, &rc, hbrush_bg);
    rc.top = higher_y;
    rc.bottom = lower_y;
    FillRect(hdc, &rc, hbrush_fg);
    DeleteObject(hbrush_bg);
    DeleteObject(hbrush_fg);
}

static LRESULT CALLBACK
on_map_canvas_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_DESTROY:
        {
            return 1;
        }
        case WM_KEYDOWN:
        {
            eu_tabpage *p = on_map_edit();
            if (!p || !p->hwnd_sc)
            {
                break;
            }
            if (wParam == VK_UP)
            {
                SendMessage(p->hwnd_sc, DOCUMENTMAP_SCROLL, (WPARAM)MOVE_UP, 0);
            }
            if (wParam == VK_DOWN)
            {
                SendMessage(p->hwnd_sc, DOCUMENTMAP_SCROLL, (WPARAM)MOVE_DOWN, 0);
            }
            if (wParam == VK_PRIOR)
            {
                SendMessage(p->hwnd_sc, DOCUMENTMAP_SCROLL, (WPARAM)MOVE_UP, 1);
            }
            if (wParam == VK_NEXT)
            {
                SendMessage(p->hwnd_sc, DOCUMENTMAP_SCROLL, (WPARAM)MOVE_DOWN, 1);
            }
            break;
        }
        case WM_SIZE:
        {
            break;
        }
        case WM_NOTIFY:
        {
            return 1;
        }
        default :
            break;
    }
    return ((WNDPROC)canvas_default_proc)(hwnd, message, wParam, lParam);
}

static intptr_t CALLBACK
on_map_static_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            HWND hwnd_canvas = GetDlgItem(hwnd, IDC_VIEWZONE_CANVAS);
            if (NULL != hwnd_canvas)
            {
                if (inter_atom_compare_exchange(&canvas_default_proc, SetWindowLongPtr(hwnd_canvas, GWLP_WNDPROC, (LONG_PTR)on_map_canvas_proc), 0))
                {
                    SetWindowLongPtr(hwnd_canvas, GWLP_WNDPROC, (LONG_PTR)on_map_canvas_proc);
                }
                if (canvas_default_proc)
                {
                    BringWindowToTop(hwnd);
                    util_transparent(hwnd, 12);
                }
                return 1;
            }
            break;
        }
        case WM_DRAWITEM :
        {
            on_map_draw_static_zone((DRAWITEMSTRUCT *)lParam);
            return 1;
        }
        case WM_THEMECHANGED:
        {
            SendMessage(hwnd_static_control, WM_THEMECHANGED, 0, 0);
            break;
        }
        case WM_SIZE:
        {
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
            HWND hwnd_canvas = GetDlgItem(hwnd, IDC_VIEWZONE_CANVAS);
            if (NULL != hwnd_canvas)
            {
                MoveWindow(hwnd_canvas, 0, 0, width, height, FALSE);
            }
            break;
        }
        case WM_MOVE:
        {
            if (!(GetWindowLongPtr(hwnd, GWL_STYLE) & WS_CHILD))
            {
                RECT rc;
                eu_tabpage *pmap = NULL;
                GetClientRect(hwnd, & rc);
                if ((rc.right - rc.left) && (pmap = on_map_edit()))
                {
                    GetWindowRect(pmap->hwnd_sc, &rc);
                    MoveWindow(hwnd, rc.left, rc.top, (rc.right - rc.left), (rc.bottom - rc.top), FALSE);
                }
            }
            break;
        }
        case WM_LBUTTONDOWN:
        {
            eu_tabpage *pmap = on_map_edit();
            if (pmap && pmap->hwnd_sc)
            {
                SendMessage(pmap->hwnd_sc, DOCUMENTMAP_MOUSECLICKED, wParam, lParam);
            }
            break;
        }
        case WM_MOUSEMOVE:
        {
            eu_tabpage *pmap = on_map_edit();
            if ((wParam & MK_LBUTTON) && pmap && pmap->hwnd_sc)
            {
                SendMessage(pmap->hwnd_sc, DOCUMENTMAP_MOUSECLICKED, wParam, lParam);
            }
            break;
        }
        case WM_MOUSEWHEEL:
        {
            int fw_keys = GET_KEYSTATE_WPARAM(wParam);
            eu_tabpage *pmap = on_map_edit();
            if (!fw_keys && pmap && pmap->hwnd_sc)
            {
                SendMessage(pmap->hwnd_sc, DOCUMENTMAP_MOUSEWHEEL, wParam, lParam);
            }
            break;
        }
        case WM_DESTROY:
        {
            if (hwnd_document_static)
            {
                eu_tabpage *pmap = (eu_tabpage *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
                if (pmap && pmap->hwnd_sc)
                {
                    DestroyWindow(pmap->hwnd_sc);
                    pmap->hwnd_sc = NULL;
                    SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
                    eu_safe_free(pmap);
                }
                if (g_splitter_minmap)
                {
                    DestroyWindow(g_splitter_minmap);
                    g_splitter_minmap = NULL;
                }
                hwnd_document_static = NULL;
                eu_logmsg("on_map_static_proc recv WM_DESTROY\n");
            }
            break;
        }
        default:
            break;
    }
    return 0;
}

void
on_map_reload(eu_tabpage *pedit)
{
    eu_tabpage *pnode = on_tabpage_focused();
    if (pedit && pnode)
    {
        sptr_t pdoc = eu_sci_call(pnode, SCI_GETDOCPOINTER, 0, 0);
        if (strcmp(eu_get_config()->window_theme, "default") == 0)
        {
            on_sci_default_theme(pedit, util_under_wine() ? WINE_BACK_COLOR : -1);
        }
        else
        {
            on_sci_default_theme(pedit, -1);
        }
        eu_sci_call(pedit, SCI_SETZOOM, -10, 0);
        eu_sci_call(pedit, SCI_SETVSCROLLBAR, 0, 0);
        eu_sci_call(pedit, SCI_SETHSCROLLBAR, 0, 0);
        // disable margin
        eu_sci_call(pedit, SCI_SETMARGINS, 0, 0);
        // receive doc pointer
        eu_sci_call(pedit, SCI_SETDOCPOINTER, 0, pdoc);
        // 强制启用自动换行, 不然folding会出现问题
        eu_sci_call(pedit, SCI_SETWRAPMODE, 2, 0);
        // folding
        eu_sci_call(pedit, SCI_SETPROPERTY, (sptr_t)"fold", (sptr_t)"1");
        eu_sci_call(pedit, SCI_SETPROPERTY, (sptr_t)"fold.comment", (sptr_t)"1");
        eu_sci_call(pedit, SCI_SETPROPERTY, (sptr_t)"fold.preprocessor", (sptr_t)"1");
        // 折叠时在下面画一条横线
        eu_sci_call(pedit, SCI_SETFOLDFLAGS, SC_FOLDFLAG_LINEAFTER_CONTRACTED, 0);
        // 行变更时展开
        eu_sci_call(pedit, SCI_SETAUTOMATICFOLD, SC_AUTOMATICFOLD_SHOW | SC_AUTOMATICFOLD_CHANGE, 0);
        on_map_sync_fold(pnode, pedit);
        on_map_scroll(pnode, pedit);
    }
}

static LRESULT CALLBACK
on_map_edit_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case DOCUMENTMAP_SCROLL:
        {
            eu_tabpage *pnode = on_tabpage_focused();
            eu_tabpage *map_edit = on_map_edit();
            if (pnode && map_edit)
            {
                bool dir = (wParam != 0);
                move_mode mode = (lParam == 0) ? per_line : per_page;
                on_map_model_scroll(pnode, map_edit, dir, mode);
            }
            break;
        }
        case DOCUMENTMAP_MOUSECLICKED:
        {
            eu_tabpage *pnode = on_tabpage_focused();
            eu_tabpage *map_edit = on_map_edit();
            if (pnode && map_edit)
            {
                int y = GET_Y_LPARAM(lParam);
                int center_y = on_map_center_pos();
                sptr_t pixel_line = eu_sci_call(map_edit, SCI_TEXTHEIGHT, 0, 0);
                sptr_t jump_distance = y- center_y;
                sptr_t jump_line = jump_distance/pixel_line;
                eu_sci_call(pnode, SCI_LINESCROLL, 0, jump_line);
                on_map_scroll(pnode, map_edit);
            }
            break;
        }
        case DOCUMENTMAP_MOUSEWHEEL:
        {
            eu_tabpage *pnode = on_tabpage_focused();
            if (pnode)
            {
                on_sic_mousewheel(pnode, wParam, lParam);
            }
            break;
        }
        default:
            break;
    }
    return CallWindowProc((WNDPROC)eu_edit_wnd, hwnd, message, wParam, lParam);
}

static void
on_map_rect(const RECT *psrc, RECT *prc)
{
    const int height = psrc->bottom - psrc->top - 4;
    prc->top = psrc->top + SPLIT_WIDTH;
    prc->bottom = psrc->bottom - SPLIT_WIDTH;
    prc->left = (psrc->right > height) ? psrc->right - height : psrc->left;
    prc->right = psrc->right;
}

static LRESULT CALLBACK
on_map_control_callback(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_PAINT:
        {
            RECT rc = {0};
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            const bool dark = on_dark_enable();
            const TCHAR *text  = util_os_version() < 603 || util_under_wine() ? _T("x") : _T("✕");
            LOAD_I18N_RESSTR(IDS_MINMAP_DESC, map_str);
            GetClientRect(hwnd, &rc);
            set_tabface_color(hdc, dark);
            set_text_color(hdc, dark);
            HGDIOBJ oldj = SelectObject(hdc, on_theme_font_hwnd());
            if (oldj)
            {
                RECT rc_close = {0};
                int height = rc.bottom - rc.top - 4;
                int right = rc.right;
                int left = rc.left;
                FrameRect(hdc, &rc, dark ? GetSysColorBrush(COLOR_3DDKSHADOW) : GetSysColorBrush(COLOR_BTNSHADOW));
                if (rc.right > height)
                {
                    rc.right -= height;
                }
                rc.left += SPLIT_WIDTH;
                DrawText(hdc, map_str, (int)_tcslen(map_str), &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
                rc.left = left;
                rc.right = right;
                on_map_rect(&rc, &rc_close);
                DrawText(hdc, text, (int)_tcslen(text), &rc_close, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                SelectObject(hdc, oldj);
            }
            EndPaint(hwnd, &ps);
            break;
        }
        case WM_THEMECHANGED:
        {
            SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, on_splitter_brush());
            InvalidateRect(hwnd, NULL, false);
            break;
        }
        case WM_LBUTTONUP:
        {
            RECT rc;
            RECT rc_close;
            eu_tabpage *p = NULL;
            POINT point = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            GetClientRect(hwnd, &rc);
            on_map_rect(&rc, &rc_close);
            if (PtInRect(&rc_close, point) && (p = on_tabpage_focused()))
            {
                p->map_show = false;
                on_proc_redraw(NULL);
            }
            return 1;
        }
        case WM_DESTROY:
        {
            if (hwnd_static_control)
            {
                hwnd_static_control = NULL;
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

static inline bool
on_map_create_static_dlg(HWND parent)
{
    if (!hwnd_document_static)
    {
        bool ver = !util_under_wine() && util_os_version() >= 603;
        if (ver)
        {   // Windows 8.1 or above
            hwnd_document_static = CreateDialogParam(eu_module_handle(), MAKEINTRESOURCE(IDD_VIEWZONE), parent, on_map_static_proc, 0);
        }
        else
        {
            hwnd_document_static = CreateDialogParam(eu_module_handle(), MAKEINTRESOURCE(IDD_VIEWZONE_CLASSIC), parent, on_map_static_proc, 0);
        }
    }
    return (hwnd_document_static != NULL);
}

static void
on_map_rect_screen(const int left, const int top, const int right, const int bottom, RECT *prc)
{
    if (prc)
    {
        prc->left = left;
        prc->top = top;
        prc->right = right;
        prc->bottom = bottom;
        MapWindowPoints(eu_hwnd_self(), HWND_DESKTOP, (POINT*)(prc), 2);
    }
}

void
on_map_size(const eu_tabpage *pnode, const int flags)
{
    eu_tabpage *pmap = on_map_edit();
    const HWND htab = on_tabpage_hwnd(pnode);
    if (pmap && htab)
    {
        RECT rc = {0};
        HDWP hdwp = NULL;
        const bool child = GetWindowLongPtr(hwnd_document_static, GWL_STYLE) & WS_CHILD;
        if (flags == SW_HIDE)
        {
            GetClientRect(hwnd_document_static, &rc);
            if (rc.right - rc.left != 0)
            {
                if (!child)
                {
                    eu_setpos_window(g_splitter_minmap, HWND_BOTTOM, 0, 0, 0, 0, SWP_HIDEWINDOW);
                    eu_setpos_window(pmap->hwnd_sc, HWND_BOTTOM, 0, 0, 0, 0, SWP_HIDEWINDOW);
                    eu_setpos_window(hwnd_document_static, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW);
                    eu_setpos_window(hwnd_static_control, HWND_BOTTOM, 0, 0, 0, 0, SWP_HIDEWINDOW);
                }
                else
                {
                    hdwp = BeginDeferWindowPos(4);
                    DeferWindowPos(hdwp, g_splitter_minmap, HWND_BOTTOM, 0, 0, 0, 0, SWP_HIDEWINDOW);
                    DeferWindowPos(hdwp, pmap->hwnd_sc, HWND_BOTTOM, 0, 0, 0, 0, SWP_HIDEWINDOW);
                    DeferWindowPos(hdwp, hwnd_document_static, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW);
                    DeferWindowPos(hdwp, hwnd_static_control, HWND_BOTTOM, 0, 0, 0, 0, SWP_HIDEWINDOW);
                    EndDeferWindowPos(hdwp);
                }
                if (on_dark_enable() || !child)
                {
                    UpdateWindowEx(htab);
                }
            }
        }
        else if (flags == SW_SHOW)
        {
            if (pnode)
            {
                int number = 3;
                const int height = on_treebar_tab_height();
                const int top = pnode->rect_map.top + height;
                if (child)
                {
                    ++number;
                }
                else 
                {
                    on_map_rect_screen(pnode->rect_map.left, top, pnode->rect_map.right, pnode->rect_map.bottom, &rc);
                }
                hdwp = BeginDeferWindowPos(number);
                DeferWindowPos(hdwp, g_splitter_minmap, HWND_TOP, pnode->rect_map.left - SPLIT_WIDTH, pnode->rect_map.top,
                               SPLIT_WIDTH, pnode->rect_map.bottom - pnode->rect_map.top, SWP_SHOWWINDOW);
                DeferWindowPos(hdwp, pmap->hwnd_sc, HWND_TOP, pnode->rect_map.left, top,
                               pnode->rect_map.right - pnode->rect_map.left, pnode->rect_map.bottom - top, SWP_SHOWWINDOW);
                if (child)
                {
                    DeferWindowPos(hdwp, hwnd_document_static, HWND_TOP, pnode->rect_map.left, top,
                                   pnode->rect_map.right - pnode->rect_map.left, pnode->rect_map.bottom - top, SWP_SHOWWINDOW);
                }
                DeferWindowPos(hdwp, hwnd_static_control, HWND_TOP, pnode->rect_map.left, pnode->rect_map.top,
                               pnode->rect_map.right - pnode->rect_map.left, height - 1, SWP_SHOWWINDOW);
                EndDeferWindowPos(hdwp);
                if (!child)
                {
                    eu_setpos_window(hwnd_document_static, HWND_TOP, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_SHOWWINDOW);
                }
                on_map_reload(pmap);
            }
        }
    }
}

eu_tabpage*
on_map_edit(void)
{
    return (hwnd_document_static ? (eu_tabpage *)GetWindowLongPtr(hwnd_document_static, GWLP_USERDATA) : NULL);
}

HWND
on_map_hwnd(void)
{
    return hwnd_document_static;
}

eu_tabpage*
on_map_launch(void)
{
    eu_tabpage *pmap = NULL;
    if (!hwnd_document_static)
    {
        HWND hwnd = eu_hwnd_self();
        const int flags = WS_CHILD | WS_CLIPCHILDREN | WS_EX_RTLREADING;
        pmap = (eu_tabpage *)calloc(1, sizeof(eu_tabpage));
        if (pmap && (g_splitter_minmap || on_splitter_init_minmap(hwnd)))
        {
            if (!hwnd_static_control)
            {
                hwnd_static_control = on_splitter_static_control(hwnd, on_map_control_callback, NULL);
            }
            if (!hwnd_static_control || (on_sci_create(pmap, hwnd, flags, on_map_edit_proc) != SKYLARK_OK) || !on_map_create_static_dlg(hwnd))
            {
                eu_safe_free(pmap);
                eu_logmsg("create map window failed\n");
            }
            else if (hwnd_document_static && pmap->hwnd_sc)
            {
                SetWindowLongPtr(hwnd_document_static, GWLP_USERDATA, (LONG_PTR)pmap);
                on_dark_border(pmap->hwnd_sc, true);
            }
        }
    }
    else
    {
        pmap = (eu_tabpage *)GetWindowLongPtr(hwnd_document_static, GWLP_USERDATA);
    }
    return pmap;
}
