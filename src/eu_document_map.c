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
#define DLG_FROST (RGB(0xFF, 0xFF, 0xFF))

HWND hwnd_document_map = NULL;
HWND hwnd_document_static = NULL;
volatile long document_map_initialized = 0;
volatile long higher_y = 0;
volatile long lower_y = 0;
static WNDPROC canvas_default_proc;

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
            printf("plines[%d] = %zd\n", i, *it);
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

static void
on_map_move_static(HWND hwnd)
{
    RECT rc;
    GetClientRect (hwnd, & rc);
    if ((GetWindowLongPtr (hwnd_document_static, GWL_STYLE) & WS_CHILD) == 0)
    {
        MapWindowPoints(hwnd, HWND_DESKTOP, (POINT*)(&rc), 2);
        MoveWindow(hwnd_document_static, rc.left, rc.top, (rc.right - rc.left), (rc.bottom - rc.top), TRUE);
    }
    else
    {
        MoveWindow(hwnd_document_static, 0, 0, (rc.right - rc.left), (rc.bottom - rc.top), TRUE);
    }
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
		    if (!hwnd_document_map)
		    {
		        break;
		    }
            if (wParam == VK_UP)
            {
                SendMessage(hwnd_document_map, DOCUMENTMAP_SCROLL, (WPARAM)move_up, 0);
            }
            if (wParam == VK_DOWN)
            {
                SendMessage(hwnd_document_map, DOCUMENTMAP_SCROLL, (WPARAM)move_down, 0);
            }
            if (wParam == VK_PRIOR)
            {
                SendMessage(hwnd_document_map, DOCUMENTMAP_SCROLL, (WPARAM)move_up, 1);
            }
            if (wParam == VK_NEXT)
            {
                SendMessage(hwnd_document_map, DOCUMENTMAP_SCROLL, (WPARAM)move_down, 1);
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
	return canvas_default_proc(hwnd, message, wParam, lParam);
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
				canvas_default_proc = (WNDPROC)SetWindowLongPtr(hwnd_canvas, GWLP_WNDPROC, (LONG_PTR)on_map_canvas_proc);
				BringWindowToTop(hwnd);
				util_transparent(hwnd, 50);
				return 1;
			}
			break;
		}
        case WM_DRAWITEM :
        {
            on_map_draw_static_zone((DRAWITEMSTRUCT *)lParam);
            return 1;
        }
        case WM_SIZE:
        {
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
			HWND hwnd_canvas = GetDlgItem(hwnd, IDC_VIEWZONE_CANVAS);
			if (NULL != hwnd_canvas)
			{
				MoveWindow(hwnd_canvas, 0, 0, width, height, TRUE);
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
            if (hwnd_document_static)
            {
                hwnd_document_static = NULL;
                printf("on_map_static_proc recv WM_DESTROY\n");
            }
            break;
        }
        default:
            break;
    }
    return 0;
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

static bool
on_map_create_static_dlg(HWND parent)
{
	if (!hwnd_document_static)
	{
		bool win10 = util_os_version() >= 1000;
		if (win10)
		{
		    hwnd_document_static = CreateDialogParam(eu_module_handle(), MAKEINTRESOURCE(IDD_VIEWZONE), parent, on_map_static_proc, 0);
		}
		else
		{
		    hwnd_document_static = CreateDialogParam(eu_module_handle(), MAKEINTRESOURCE(IDD_VIEWZONE_CLASSIC), parent, on_map_static_proc, 0);
		}
	}
	return (hwnd_document_static != NULL);
}

static intptr_t CALLBACK
on_map_callback(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
        {
            eu_tabpage *map_edit = (eu_tabpage *)lParam;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)map_edit);
            const int flags = WS_CHILD | WS_CLIPCHILDREN | WS_EX_RTLREADING;
            if (on_sci_create(map_edit, hwnd, flags, on_map_edit_proc) != SKYLARK_OK)
            {
                (intptr_t)EndDialog(hwnd, 1);
            }
            if (!on_map_create_static_dlg(hwnd))
            {
                (intptr_t)EndDialog(hwnd, 1);
            }
            on_dark_border(map_edit->hwnd_sc, true);
            return 1;
        }
        case WM_SIZE:
        {
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
            eu_tabpage *pview = on_map_edit();
            if ((!pview && pview->hwnd_sc && hwnd_document_static))
            {
                break;
            }
            MoveWindow(pview->hwnd_sc, 0, 0, width, height, TRUE);
            ShowWindow(pview->hwnd_sc, SW_SHOW);
            on_map_move_static(hwnd);
            ShowWindow(hwnd_document_static, SW_SHOW);
            break;
        }
        case WM_MOVE:
        {
            on_map_move_static(hwnd);
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
            break;
        }
        case WM_DESTROY:
        {
            if (hwnd_document_map)
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
                hwnd_document_map = NULL;
                printf("on_map_callback WM_DESTROY\n");
            }
            break;
        }
        default:
            break;
    }
    return 0;
}

static bool
on_map_create_dlg(LPARAM ptr)
{
	if (!hwnd_document_map)
	{
		hwnd_document_map = CreateDialogParam(eu_module_handle(), MAKEINTRESOURCE(IDD_DOCUMENTMAP), eu_module_hwnd(), on_map_callback, ptr);
	}
	return (hwnd_document_map != NULL);
}

eu_tabpage *WINAPI
on_map_edit(void)
{
    if (document_map_initialized && hwnd_document_map)
    {
        return (eu_tabpage *)GetWindowLongPtr(hwnd_document_map, GWLP_USERDATA);
    }
    return NULL;
}

eu_tabpage *WINAPI
on_map_launch(void)
{
    eu_tabpage *map_edit = NULL;
    if (!_InterlockedCompareExchange(&document_map_initialized, 1, 0))
    {
        map_edit = (eu_tabpage *)calloc(1, sizeof(eu_tabpage));
        if (!map_edit || !on_map_create_dlg((LPARAM)map_edit))
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
