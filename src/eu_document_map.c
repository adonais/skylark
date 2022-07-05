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

#define DLG_FOCUS (RGB(0xFF, 0x80, 0x00))

HWND hwnd_document_map = NULL;
HWND hwnd_document_static = NULL;
volatile long document_map_initialized = 0;
volatile long higher_y = 0;
volatile long lower_y = 0;

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

#ifdef _DEBUG
static void
on_map_print(sptr_t *plines)
{
    if (plines)
    {
        int i = 0;
        sptr_t *it;
        for (it = cvector_begin(plines); it != cvector_end(plines); ++it)
        {
            printf("plines[%d] = %I64d\n", i, *it);
            ++i;
        }
    }
}
#endif

void WINAPI
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
    eu_sci_call(pnode, SCI_LINESCROLL, 0, (direction == move_down) ? line2go : -line2go);
    on_map_scroll(pnode, ptr_map);
}

static LRESULT CALLBACK
on_map_edit_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    return CallWindowProc((WNDPROC)eu_edit_wnd, hwnd, message, wParam, lParam);
}

static LRESULT CALLBACK
on_map_callback(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_ERASEBKGND:
        {
            RECT rc = {0};
            GetClientRect(hwnd, &rc);
            FillRect((HDC)wParam, &rc, (HBRUSH)on_dark_theme_brush());
            return 1;
        }
        case WM_CREATE:
        {
            eu_tabpage *map_edit = (eu_tabpage *)((LPCREATESTRUCTW)lParam)->lpCreateParams;
            if (map_edit)
            {
                SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)map_edit);
                const int flags = WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_CLIPCHILDREN | WS_EX_RTLREADING;
                if (!on_sci_create(map_edit, flags, on_map_edit_proc))
                {
                    on_dark_border(map_edit->hwnd_sc, true);
                }
            }
            break;
        }
        case WM_SIZE:
        {
            break;
        }
        case DOCUMENTMAP_SCROLL:
        {
            eu_tabpage *pnode = on_tabpage_focus_at();
            eu_tabpage *map_edit = (eu_tabpage *)GetWindowLongPtr(hwnd_document_map, GWLP_USERDATA);
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
            eu_tabpage *pnode = on_tabpage_focus_at();
            eu_tabpage *map_edit = (eu_tabpage *)GetWindowLongPtr(hwnd_document_map, GWLP_USERDATA);
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
            eu_tabpage *pnode = on_tabpage_focus_at();
            if (pnode)
            {
                on_sic_mousewheel(pnode, wParam, lParam);
            }
            break;
        }
        case WM_THEMECHANGED:
        {
            on_dark_delete_theme_brush();
            break;
        }
        case WM_DESTROY:
        {
            eu_tabpage *map_edit = (eu_tabpage *)GetWindowLongPtr(hwnd_document_map, GWLP_USERDATA);
            if (map_edit)
            {
                if (map_edit->hwnd_sc)
                {
                    DestroyWindow(map_edit->hwnd_sc);
                    map_edit->hwnd_sc = NULL;
                }
                eu_safe_free(map_edit);
                SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
            }
            _InterlockedExchange(&document_map_initialized, 0);
            on_dark_delete_theme_brush();
            printf("on_map_callback WM_DESTROY\n");
            break;
        }
        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}

static void
on_map_draw_static_zone(HDC hdc, RECT *pdis)
{
    RECT rc = *pdis;
    HBRUSH hbrush_fg = CreateSolidBrush(DLG_FOCUS);
    rc.top = higher_y;
    rc.bottom = lower_y;
    FillRect(hdc, &rc, hbrush_fg);
    DeleteObject(hbrush_fg);
}

static LRESULT CALLBACK
on_map_static_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CREATE:
        {
            util_transparent(hwnd, 50);
            break;
        }
        case WM_PAINT:
        {
            RECT rc;
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, & ps);
            GetClientRect(hwnd, &rc);
            on_map_draw_static_zone(hdc, &rc);
            EndPaint(hwnd, &ps);
            break;
        }
        case WM_SIZE:
        {
            break;
        }
        case WM_SETCURSOR:
        {
            CURSORINFO hcur = {sizeof (CURSORINFO)};
            GetCursorInfo(&hcur);
            HCURSOR hcrs = LoadCursor(NULL, IDC_ARROW);
            if ( hcur.hCursor != hcrs)
            {
                SetCursor(hcrs);
            }
            break;
        }
        case WM_LBUTTONDOWN:
        {
            SendMessage(hwnd_document_map, DOCUMENTMAP_MOUSECLICKED, wParam, lParam);
            break;
        }
        case WM_MOUSEMOVE:
        {
            if (wParam & MK_LBUTTON)
            {
                SendMessage(hwnd_document_map, DOCUMENTMAP_MOUSECLICKED, wParam, lParam);
            }
            break;
        }
        case WM_MOUSEWHEEL:
        {
            int fw_keys = GET_KEYSTATE_WPARAM(wParam);
            if (!fw_keys)
            {
                SendMessage(hwnd_document_map, DOCUMENTMAP_MOUSEWHEEL, wParam, lParam);
            }
            break;
        }
        case WM_DESTROY:
        {
            printf("on_map_static_proc recv WM_DESTROY\n");
            break;
        }
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void WINAPI
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

void WINAPI
on_map_reload(eu_tabpage *pedit)
{
    eu_tabpage *pnode = on_tabpage_focus_at();
    if (pedit && pnode)
    {
        sptr_t pdoc = eu_sci_call(pnode, SCI_GETDOCPOINTER, 0, 0);
        on_sci_init_style(pedit);
        eu_sci_call(pedit, SCI_SETZOOM, -10, 0);
        eu_sci_call(pedit, SCI_SETVSCROLLBAR, 0, 0);
        eu_sci_call(pedit, SCI_SETHSCROLLBAR, 0, 0);
        // disable margin
        eu_sci_call(pedit, SCI_SETMARGINWIDTHN, MARGIN_LINENUMBER_INDEX, 0);
        eu_sci_call(pedit, SCI_SETMARGINWIDTHN, MARGIN_BOOKMARK_INDEX, 0);
        eu_sci_call(pedit, SCI_SETMARGINWIDTHN, MARGIN_FOLD_INDEX, 0);
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

eu_tabpage *WINAPI
on_map_launch(void)
{
    eu_tabpage *map_edit = NULL;
    if (!_InterlockedCompareExchange(&document_map_initialized, 1, 0))
    {
        const TCHAR *class_name = _T("Document Map");
        const int flags = WS_CHILD | WS_CLIPSIBLINGS;
        HWND parent = eu_module_hwnd();
        map_edit = (eu_tabpage *)calloc(1, sizeof(eu_tabpage));
        if (!map_edit ||
            !(hwnd_document_map = on_splitter_init_window(parent, class_name, flags, NULL, on_map_callback, (void *)map_edit)) ||
            !(hwnd_document_static = on_splitter_init_window(parent, _T("Static"), flags, (HMENU)IDC_VIEWZONE, on_map_static_proc, NULL)))
        {
            _InterlockedExchange(&document_map_initialized, 0);
            eu_safe_free(map_edit);
        }
    }
    else if (hwnd_document_map)
    {
        map_edit = (eu_tabpage *)GetWindowLongPtr(hwnd_document_map, GWLP_USERDATA);
    }
    return map_edit;
}
