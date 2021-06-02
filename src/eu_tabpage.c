/******************************************************************************
 * This file is part of Skylark project
 * Copyright ©2021 Hua andy <hua.andy@gmail.com>

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

HWND g_tabpages = NULL;
HMENU pop_editor_menu = NULL;
HMENU pop_symlist_menu = NULL;
HMENU pop_tab_menu = NULL;
HMENU pop_symtree_refresh_menu = NULL;
HMENU pop_symtree_table_menu = NULL;
HMENU pop_symtree_row_menu = NULL;

#define TAB_MIN_TOP 4
#define TAB_MIN_LEFT 40
#define TAB_MIN_WIDTH 140
 
static bool is_moving;
static int move_from;
static HWND hwnd_close;
static POINT pt_down;
static WNDPROC old_tabproc;

static int
on_tabpage_get_height(void)
{
    EU_VERIFY(g_tabpages != NULL);
    RECT rectTabPage = { 0 };
    int tab_height = TABS_HEIGHT_DEFAULT;
    int count = TabCtrl_GetItemCount(g_tabpages);
    if (count <= 0)
    {
        return tab_height;
    }
    TabCtrl_GetItemRect(g_tabpages, 0, &rectTabPage);
    if (rectTabPage.top > 0 && rectTabPage.bottom > 0)
    {
        int row = TabCtrl_GetRowCount(g_tabpages);
        return (rectTabPage.bottom - rectTabPage.top + 1) * row;
    }
    return tab_height;
}

void 
on_tabpage_destroy_rclick(void)
{
    if (pop_tab_menu)
    {
        DestroyMenu(pop_tab_menu);
        pop_tab_menu = NULL;
    }
    if (pop_editor_menu)
    {
        DestroyMenu(pop_editor_menu);
        pop_editor_menu = NULL;
    }
    if (pop_symlist_menu)
    {
        DestroyMenu(pop_symlist_menu);
        pop_symlist_menu = NULL;
    }
    if (pop_symtree_refresh_menu)
    {
        DestroyMenu(pop_symtree_refresh_menu);
        pop_symtree_refresh_menu = NULL;
    }
    if (pop_symtree_table_menu)
    {
        DestroyMenu(pop_symtree_table_menu);
        pop_symtree_table_menu = NULL;
    }
    if (pop_symtree_row_menu)
    {
        DestroyMenu(pop_symtree_row_menu);
        pop_symtree_row_menu = NULL;
    }
    on_treebar_destroy_pop_menu();
    hexview_destroy_pop_menu();      
}

static void
on_tabpage_destroy_tabbar(void)
{
    on_tabpage_destroy_rclick();
    if (hwnd_close)
    {
        DestroyWindow(hwnd_close);
        hwnd_close = NULL;
    }
    if (g_tabpages)
    {
        g_tabpages = NULL;
    }
    printf("tabbar WM_DESTROY\n");
}

static void
on_tabpage_update_close(bool hover)
{
    POINT point;
    RECT rect = {0};
    int count = -1;
    GetCursorPos(&point);
    ScreenToClient(g_tabpages, &point);
    if ((count = TabCtrl_GetItemCount(g_tabpages)) <= 0)
    {
        eu_setpos_window(hwnd_close, HWND_BOTTOM, 0, 0, 0, 0, SWP_HIDEWINDOW | SWP_NOCOPYBITS);
        return;
    }
    if (!hover)
    {
        eu_setpos_window(hwnd_close, HWND_BOTTOM, 0, 0, 0, 0, SWP_HIDEWINDOW | SWP_NOCOPYBITS);
        return;
    }
    for (int index = 0; index < count; ++index)
    {
        TabCtrl_GetItemRect(g_tabpages, index, &rect);
        if (PtInRect(&rect, point))
        {
            eu_setpos_window(hwnd_close,
                          HWND_TOP,
                          rect.right - TABCLOSEBUTTON_WIDTH - 2,
                          rect.top + 4,
                          TABCLOSEBUTTON_WIDTH,
                          TABCLOSEBUTTON_HEIGHT,
                          SWP_SHOWWINDOW | SWP_NOCOPYBITS);
            break;
        }
        else
        {
            eu_setpos_window(hwnd_close, HWND_BOTTOM, 0, 0, 0, 0, SWP_HIDEWINDOW | SWP_NOCOPYBITS);
        }
    }    
}

LRESULT 
on_tabpage_draw_item(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(hwnd);
    if (on_dark_enable())
    {  
        return 1;
    }
    return 0;
}

static void
on_tabpage_paint_draw(HWND hwnd, HDC hdc)
{
    HBRUSH hbr_bkgnd = (HBRUSH)on_dark_get_brush();
    HGDIOBJ old_font = SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));
    if (old_font)
    {
        set_text_color(hdc, true);
        int count = TabCtrl_GetItemCount(g_tabpages);
        int sel_tab = TabCtrl_GetCurSel(hwnd);
        for (int index = 0; index < count; ++index)
        {
            TCITEM tci = {TCIF_PARAM};
            TabCtrl_GetItem(g_tabpages, index, &tci);
            eu_tabpage *p = (eu_tabpage *) (tci.lParam);
            if (p)
            {
                RECT rc;
                TabCtrl_GetItemRect(hwnd, index, &rc);
                FrameRect(hdc, &rc, GetSysColorBrush(COLOR_3DDKSHADOW));
                if (sel_tab == index)
                {   // example: cr = 0xFF8000;
                    colour cr = on_dark_light_color(rgb_dark_bk_color, 1.5f);
                    SetBkColor(hdc, cr);
                    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
                }
                else
                {
                    set_btnface_color(hdc, true);
                }
                if (STR_NOT_NUL(p->filename))
                {
                    DrawText(hdc, p->filename, (int)_tcslen(p->filename), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                }
            }
        }
        HGDIOBJ hfont = SelectObject(hdc, old_font);
        if (hfont)
        {
            DeleteObject(hfont);
        }
    }
}

LRESULT CALLBACK
tabs_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int count = 0;
    int index = 0;
    eu_tabpage *p = NULL;
    switch (message)
    {
        case WM_ERASEBKGND:
            if (!on_dark_enable())
            {
                break;
            }
            RECT rc = { 0 };
            GetClientRect(hwnd, &rc);
            FillRect((HDC)wParam, &rc, (HBRUSH)on_dark_get_brush());
            return 1;
        case WM_PAINT:
        {
            if (GetWindowLongPtr(hwnd, GWL_STYLE) & TCS_OWNERDRAWFIXED)
            {
                PAINTSTRUCT    ps;
                HDC hdc = BeginPaint(hwnd, & ps);
                 on_tabpage_paint_draw(hwnd, hdc);
                EndPaint(hwnd, &ps);
            }
            else
            {
                on_tabpage_update_close(true);
            }
            break;
        }
        case WM_THEMECHANGED:
        {
            uintptr_t style = GetWindowLongPtr(hwnd, GWL_STYLE);
            if (on_dark_enable())
            {
                SetWindowLongPtr(hwnd, GWL_STYLE, style | TCS_OWNERDRAWFIXED);
            }
            else
            {
                SetWindowLongPtr(hwnd, GWL_STYLE, style & ~TCS_OWNERDRAWFIXED);
            }
            break;
        }
        case WM_DRAWITEM:
        {
            RECT rect = ((DRAWITEMSTRUCT *) lParam)->rcItem;
            HDC hdc = ((DRAWITEMSTRUCT *) lParam)->hDC;
            SetBkMode(hdc, TRANSPARENT);
            colour close_colour = RGB(250, 170, 60);
            HBRUSH brush = CreateSolidBrush(close_colour);
            FillRect(hdc, &rect, brush);
            DeleteObject((HGDIOBJ) brush);
            SetTextColor(hdc, GetSysColor(COLOR_BTNFACE));
            DrawText(hdc, _T("X"), 1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            break;
        }
        case WM_COMMAND:
        {
            if (HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDM_TAB_CLOSE)
            {
                POINT pt;
                GetCursorPos(&pt);
                ScreenToClient(g_tabpages, &pt);
                LPARAM lparam = MAKELONG(pt.x, pt.y);
                PostMessage(g_tabpages, WM_MBUTTONUP, 0, lparam);
            }
            break;
        }
        case WM_MOUSEMOVE:
        {
            TRACKMOUSEEVENT MouseEvent = { sizeof(TRACKMOUSEEVENT), TME_HOVER | TME_LEAVE, hwnd, HOVER_DEFAULT };
            TrackMouseEvent(&MouseEvent);
            break;
        }
        case WM_MOUSEHOVER:
        {   // 鼠标悬停激活
            break;
        }
        case WM_MOUSELEAVE:
        {
            on_tabpage_update_close(false);
            break;
        }
        case WM_LBUTTONDOWN:
            if (!is_moving)
            {
                int x = GET_X_LPARAM(lParam);
                int y = GET_Y_LPARAM(lParam);

                int count = TabCtrl_GetItemCount(g_tabpages);
                RECT rectTabPage;
                move_from = -1;
                for (index = 0; index < count; ++index)
                {
                    TabCtrl_GetItemRect(g_tabpages, index, &rectTabPage);
                    if (rectTabPage.left < x && x < rectTabPage.right && rectTabPage.top < y && y < rectTabPage.bottom)
                    {
                        move_from = index;
                        break;
                    }
                }
                if (move_from >= 0)
                {
                    is_moving = true;
                    pt_down.x = x;
                    pt_down.y = y;
                }
            }
            break;
        case WM_LBUTTONUP:
        {
            if (is_moving)
            {
                int x = GET_X_LPARAM(lParam);
                int y = GET_Y_LPARAM(lParam);

                if (abs(x - pt_down.x) > MOVE_SPLITE_LINE || abs(y - pt_down.y) > MOVE_SPLITE_LINE)
                {
                    int count = TabCtrl_GetItemCount(g_tabpages);
                    RECT rectTabPage;
                    int m_to = -1;
                    for (index = 0; index < count; ++index)
                    {
                        TabCtrl_GetItemRect(g_tabpages, index, &rectTabPage);
                        if (rectTabPage.left < x && x < rectTabPage.right && rectTabPage.top < y && y < rectTabPage.bottom)
                        {
                            m_to = index;
                            break;
                        }
                    }
                    if (m_to >= 0)
                    {
                        TCITEM tci = {0};
                        eu_tabpage *pnode = on_tabpage_focus_at();
                        if (!pnode)
                        {
                            break;
                        }
                        TabCtrl_DeleteItem(g_tabpages, move_from);
                        tci.mask = TCIF_TEXT | TCIF_PARAM;
                        tci.pszText = pnode->filename;
                        tci.lParam = (LPARAM) pnode;
                        if (TabCtrl_InsertItem(g_tabpages, m_to, &tci) != -1)
                        {
                            on_tabpage_select_index(m_to);
                        }
                    }
                }
                is_moving = false;
                PostMessage(eu_module_hwnd(), WM_LBUTTONUP, wParam, lParam);
            }
            break;
        }
        case WM_RBUTTONDOWN:
            if (hwnd == g_tabpages)
            {
                POINT point;
                RECT rect = { 0 };
                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);
                count = TabCtrl_GetItemCount(g_tabpages);
                for (index = 0; index < count; ++index)
                {
                    TabCtrl_GetItemRect(g_tabpages, index, &rect);
                    if (PtInRect(&rect, point))
                    {
                        on_tabpage_select_index(index);
                        break;
                    }
                }
            }
            break;
        case WM_RBUTTONUP:
        {
            POINT pt;
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);
            ClientToScreen(hwnd, &pt);
            TrackPopupMenu(pop_tab_menu, 0, pt.x, pt.y, 0, eu_module_hwnd(), NULL);
            break;
        }
        case WM_MBUTTONUP:
        {
            POINT point;
            RECT rect = { 0 };
            point.x = GET_X_LPARAM(lParam);
            point.y = GET_Y_LPARAM(lParam);
            count = TabCtrl_GetItemCount(g_tabpages);
            for (index = 0; index < count; ++index)
            {
                TabCtrl_GetItemRect(g_tabpages, index, &rect);
                if (PtInRect(&rect, point))
                {
                    eu_tabpage *tmp = on_tabpage_get_ptr(index);
                    if (tmp)
                    {
                        on_file_close(tmp, FILE_ONLY_CLOSE);
                        eu_setpos_window(hwnd_close, HWND_BOTTOM, 0, 0, 0, 0, SWP_HIDEWINDOW);
                    }
                    break;
                }
            }
            break;
        }
        case WM_DESTROY:
        {
            on_tabpage_destroy_tabbar();
            break;
        }
        default:
            break;
    }
    return CallWindowProc(old_tabproc, hwnd, message, wParam, lParam);
}

int
on_tabpage_create_rclick(void)
{
    int err = 1;
    do
    {
        if ((pop_tab_menu = i18n_load_menu(IDR_TABPAGE_POPUPMENU)) == NULL)
        {
            break;
        }
        pop_tab_menu = GetSubMenu(pop_tab_menu, 0);
        if ((pop_editor_menu = i18n_load_menu(IDR_EDITOR_POPUPMENU)) == NULL)
        {
            break;
        }
        pop_editor_menu = GetSubMenu(pop_editor_menu, 0);
        if ((pop_symlist_menu = i18n_load_menu(IDR_SYMBOLLIST_POPUPMENU)) == NULL)
        {
            break;
        }
        pop_symlist_menu = GetSubMenu(pop_symlist_menu, 0);
        if ((pop_symtree_refresh_menu = i18n_load_menu(IDR_SYMBOLTREE_REFRESH_POPUPMENU)) == NULL)
        {
            break;
        }
        pop_symtree_refresh_menu = GetSubMenu(pop_symtree_refresh_menu, 0);
        if ((pop_symtree_table_menu = i18n_load_menu(IDR_SYMBOLTREE_TABLE_POPUPMENU)) == NULL)
        {
            break;
        }
        pop_symtree_table_menu = GetSubMenu(pop_symtree_table_menu, 0);
        if ((pop_symtree_row_menu = i18n_load_menu(IDR_SYMBOLTREE_ROW_POPUPMENU)) == NULL)
        {
            break;
        }
        pop_symtree_row_menu = GetSubMenu(pop_symtree_row_menu, 0);
        if (!on_treebar_create_pop_menu())
        {
            break;
        }
        err = hexview_create_pop_menu();
    } while(0);
    return err;
}

int
on_tabpage_create_dlg(HWND hwnd)
{
    int err = 0;
    uint32_t flags = WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TCS_TOOLTIPS;
#ifdef _M_X64    
    if (!util_under_wine())
    {
        flags &= ~TCS_SINGLELINE;
        flags |= TCS_MULTILINE;
    }
#endif    
    g_tabpages =
        CreateWindow(WC_TABCONTROL, NULL, flags, 0, 0, 0, 0, hwnd, (HMENU)IDM_TABPAGE_BAR, eu_module_handle(), NULL);
    do
    {
        if (g_tabpages == NULL)
        {
            MSG_BOX(IDC_MSG_TABCONTROL_FAIL, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
            err = 1;
            break;
        } 
        SendMessage(g_tabpages, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0);
        TabCtrl_SetPadding(g_tabpages, TAB_MIN_LEFT, TAB_MIN_TOP);
        TabCtrl_SetMinTabWidth(g_tabpages, TAB_MIN_WIDTH);
        ShowWindow(g_tabpages, SW_SHOW);
        UpdateWindow(g_tabpages);
        if ((err = on_tabpage_create_rclick()) != 0)
        {
            break;
        }
        hwnd_close =
            CreateWindowEx(0, _T("BUTTON"), _T("X"), WS_CHILD | BS_FLAT | BS_OWNERDRAW, 0, 0, 0, 0, g_tabpages, (HMENU) IDM_TAB_CLOSE, eu_module_handle(), NULL);
        if (hwnd_close == NULL)
        {
            err = 1;
            break;
        }
        if (!(old_tabproc = (WNDPROC) SetWindowLongPtr(g_tabpages, GWLP_WNDPROC, (LONG_PTR) tabs_proc)))
        {
            err = 1;
            break;
        }
        ShowWindow(hwnd_close, SW_HIDE);
        UpdateWindow(hwnd_close);
    } while(0);
    if (err)
    {
        on_tabpage_destroy_rclick();
        if (hwnd_close)
        {
            DestroyWindow(hwnd_close);
            hwnd_close = NULL;
        }
        if (g_tabpages)
        {
            DestroyWindow(g_tabpages);
            g_tabpages = NULL;
        }
    }
    return err;
}

void
on_tabpage_adjust_box(RECT *ptp)
{
    RECT rect_main;
    RECT rectFileTree = { 0 };
    GetClientRect(eu_module_hwnd(), &rect_main);
    if (!eu_get_config()->m_ftree_show)
    {
        ptp->left = rect_main.left;
        ptp->right = rect_main.right;
        ptp->top = rect_main.top + on_toolbar_height();
        ptp->bottom = rect_main.bottom - on_statusbar_height();
    }
    else
    {
        on_treebar_adjust_box(&rectFileTree);
        ptp->left = rectFileTree.right + SPLIT_WIDTH;
        ptp->right = rect_main.right;
        ptp->top = rect_main.top + on_toolbar_height();
        ptp->bottom = rect_main.bottom - on_statusbar_height();
    }
}

void
on_tabpage_adjust_window(eu_tabpage *pnode)
{
    int tab_height = 0;
    RECT rect_tabpages = {0};
    EU_VERIFY(pnode != NULL);
    if (true)
    {
        tab_height = on_tabpage_get_height();
        on_tabpage_adjust_box(&rect_tabpages);
        pnode->rect_sc.left = rect_tabpages.left + SCINTILLA_MARGIN_LEFT;
        pnode->rect_sc.right = rect_tabpages.right - SCINTILLA_MARGIN_RIGHT;
        pnode->rect_sc.top = rect_tabpages.top + tab_height + SCINTILLA_MARGIN_TOP;
        pnode->rect_sc.bottom = rect_tabpages.bottom - SCINTILLA_MARGIN_BOTTOM;

        pnode->rect_symlist.left = rect_tabpages.right;
        pnode->rect_symlist.right = rect_tabpages.right;
        pnode->rect_symlist.top = rect_tabpages.top + tab_height;
        pnode->rect_symlist.bottom = rect_tabpages.bottom;

        pnode->rect_symtree.left = rect_tabpages.right;
        pnode->rect_symtree.right = rect_tabpages.right;
        pnode->rect_symtree.top = rect_tabpages.top + tab_height;
        pnode->rect_symtree.bottom = rect_tabpages.bottom;
    }    
    if (eu_get_config()->m_sym_show)
    {
        if (pnode->hwnd_symlist && ListBox_GetCount(pnode->hwnd_symlist) > 0)
        {
            pnode->rect_sc.left = rect_tabpages.left + SCINTILLA_MARGIN_LEFT;
            pnode->rect_sc.right = rect_tabpages.right - SYMBOLLIST_MARGIN_LEFT - eu_get_config()->sym_list_width -
                                   SYMBOLLIST_MARGIN_RIGHT - SCINTILLA_MARGIN_RIGHT;
            pnode->rect_sc.top = rect_tabpages.top + tab_height + SCINTILLA_MARGIN_TOP;
            pnode->rect_sc.bottom = rect_tabpages.bottom - SCINTILLA_MARGIN_BOTTOM;

            pnode->rect_symlist.left = pnode->rect_sc.right + SPLIT_WIDTH + SYMBOLLIST_MARGIN_LEFT;
            pnode->rect_symlist.right = rect_tabpages.right - SYMBOLLIST_MARGIN_RIGHT;
            pnode->rect_symlist.top = rect_tabpages.top + tab_height + SYMBOLTREE_MARGIN_TOP;
            pnode->rect_symlist.bottom = rect_tabpages.bottom;
            
            pnode->rect_symtree.left = rect_tabpages.right;
            pnode->rect_symtree.right = rect_tabpages.right;
            pnode->rect_symtree.top = rect_tabpages.top + tab_height;
            pnode->rect_symtree.bottom = rect_tabpages.bottom;
        }
        else if (pnode->hwnd_symtree && TreeView_GetCount(pnode->hwnd_symtree) > 0)
        {
            pnode->rect_sc.left = rect_tabpages.left + SCINTILLA_MARGIN_LEFT;
            pnode->rect_sc.right = rect_tabpages.right - SYMBOLTREE_MARGIN_LEFT - eu_get_config()->sym_tree_width -
                                   SYMBOLTREE_MARGIN_RIGHT - SCINTILLA_MARGIN_RIGHT;
            pnode->rect_sc.top = rect_tabpages.top + tab_height + SCINTILLA_MARGIN_TOP;
            pnode->rect_sc.bottom = rect_tabpages.bottom - SCINTILLA_MARGIN_BOTTOM;

            pnode->rect_symlist.left = rect_tabpages.right;
            pnode->rect_symlist.right = rect_tabpages.right;
            pnode->rect_symlist.top = rect_tabpages.top + tab_height;
            pnode->rect_symlist.bottom = rect_tabpages.bottom;

            pnode->rect_symtree.left = pnode->rect_sc.right + SPLIT_WIDTH + SYMBOLTREE_MARGIN_LEFT;
            pnode->rect_symtree.right = rect_tabpages.right - SYMBOLTREE_MARGIN_RIGHT;
            pnode->rect_symtree.top = rect_tabpages.top + tab_height + SYMBOLTREE_MARGIN_TOP;
            pnode->rect_symtree.bottom = rect_tabpages.bottom - SYMBOLTREE_MARGIN_BOTTOM;
        }
    }
    if (pnode->edit_show)
    {
        if (pnode->hwnd_qredit)
        {
            pnode->rect_sc.bottom -= SPLIT_WIDTH + eu_get_config()->result_edit_height + SPLIT_WIDTH +
                                     eu_get_config()->result_list_height;
            pnode->rect_symlist.bottom -= SPLIT_WIDTH + eu_get_config()->result_edit_height + SPLIT_WIDTH +
                                          eu_get_config()->result_list_height;
            pnode->rect_symtree.bottom -= SPLIT_WIDTH + eu_get_config()->result_edit_height + SPLIT_WIDTH +
                                          eu_get_config()->result_list_height;

            pnode->rect_qredit.left = rect_tabpages.left + SCINTILLA_MARGIN_LEFT;
            pnode->rect_qredit.right = rect_tabpages.right - SCINTILLA_MARGIN_RIGHT;
            pnode->rect_qredit.top = pnode->rect_sc.bottom + SPLIT_WIDTH;
            pnode->rect_qredit.bottom = pnode->rect_qredit.top + eu_get_config()->result_edit_height
                                        + SPLIT_WIDTH + eu_get_config()->result_list_height;
            if (pnode->hwnd_qrtable)
            {
                pnode->rect_qredit.bottom = pnode->rect_qredit.top + eu_get_config()->result_edit_height;
                pnode->rect_qrtable.left = rect_tabpages.left + SCINTILLA_MARGIN_LEFT;
                pnode->rect_qrtable.right = rect_tabpages.right - SCINTILLA_MARGIN_RIGHT;
                pnode->rect_qrtable.top = pnode->rect_qredit.bottom + SPLIT_WIDTH;
                pnode->rect_qrtable.bottom = pnode->rect_qrtable.top + eu_get_config()->result_list_height;
            }
        }
    }
}

int
on_tabpage_remove(eu_tabpage **ppnode)
{
    int index = 0;
    eu_tabpage *p = NULL;
    EU_VERIFY(ppnode != NULL && *ppnode != NULL && g_tabpages != NULL);
    int count = TabCtrl_GetItemCount(g_tabpages);
    for (index = 0; index < count; ++index)
    {
        TCITEM tci = {TCIF_PARAM};
        TabCtrl_GetItem(g_tabpages, index, &tci);
        p = (eu_tabpage *) (tci.lParam);
        if (p && p == *ppnode)
        {
            /* 删除控件句柄与释放资源 */
            TabCtrl_DeleteItem(g_tabpages, index);
            on_sci_free_tab(ppnode);
            break;
        }
    }
    return index;
}

static int
on_tabpage_remove_empty(void)
{
    int count;
    int ret = 0;
    EU_VERIFY(g_tabpages != NULL);
    if ((count = TabCtrl_GetItemCount(g_tabpages)) < 2)
    {
        return 0;
    }
    for (int index = 0; index < count; ++index)
    {
        TCITEM tci = {TCIF_PARAM};
        TabCtrl_GetItem(g_tabpages, index, &tci);
        eu_tabpage *p = (eu_tabpage *) (tci.lParam);
        if (p && p->is_blank && !eu_sci_call(p, SCI_GETLENGTH, 0, 0))
        {
            if (!on_sci_doc_modified(p))
            {
                printf("we ready remove empty tab!\n");
                ret = 1;
                TabCtrl_DeleteItem(g_tabpages, index);
                on_sci_free_tab(&p);
                break;
            }
        }
    }
    return ret;
}

TCHAR *
on_tabpage_newdoc_name(TCHAR *filename, int len)
{
    EU_VERIFY(g_tabpages != NULL);
    LOAD_APP_RESSTR(IDC_MSG_NEW_FILE, m_file);
    if (_stscanf(m_file, _T("%100s"), filename) == 1)
    {
        int ret = 1;
        int count = TabCtrl_GetItemCount(g_tabpages);
        for (int index = 0; index < count; ++index)
        {
            TCITEM tci = {TCIF_PARAM,};
            TabCtrl_GetItem(g_tabpages, index, &tci);
            eu_tabpage *p = (eu_tabpage *) (tci.lParam);
            if (p && p->is_blank)
            {
                if (_tcsncmp(p->filename, filename, _tcslen(filename)) == 0)
                {
                    ++ret;
                }
            }
        }
        _sntprintf(filename, len, m_file, ret);
    }
    return filename;
}

int
on_tabpage_add(eu_tabpage *pnode)
{
    EU_VERIFY(pnode != NULL && g_tabpages != NULL);
    TCITEM tci = { 0 };
    if (pnode->codepage != IDM_OTHER_BIN && !pnode->hex_mode)
    {
        pnode->doc_ptr = on_doc_get_type(pnode->filename);
    }
    tci.mask = TCIF_TEXT | TCIF_PARAM;
    tci.pszText = pnode->filename;
    tci.lParam = (LPARAM) pnode;
    pnode->tab_id = TabCtrl_GetItemCount(g_tabpages);
    if (TabCtrl_InsertItem(g_tabpages, pnode->tab_id, &tci) == -1)
    {
        printf("TabCtrl_InsertItem return failed on %s:%d\n", __FILE__, __LINE__);
        return 1;
    }
    if (!pnode->is_blank)
    {
        pnode->tab_id -= on_tabpage_remove_empty();
    }
    if ((pnode->fs_server.networkaddr[0] == 0 && pnode->codepage == IDM_OTHER_BIN) || pnode->hex_mode)
    {
        pnode->hex_mode = true;
        pnode->bytes_remaining = pnode->raw_size;
        pnode->tab_id = -1;
        printf("hexview_init, pnode = %p, pnode->bytes_remaining = %I64d\n", pnode, pnode->bytes_remaining);
        return !hexview_init(pnode);
    }
    return on_sci_init_dlg(pnode);
}

void
on_tabpage_set_title(int ontab, TCHAR *title)
{
    EU_VERIFY(g_tabpages != NULL);
    TCITEM tci = {0};
    tci.mask = TCIF_TEXT;
    tci.pszText = title;
    TabCtrl_SetItem(g_tabpages, ontab, &tci);
    InvalidateRect(g_tabpages, NULL, true);
}

int
on_tabpage_editor_modify(eu_tabpage *pnode, const char *str)
{
    EU_VERIFY(pnode != NULL);
    if (pnode->hex_mode)
    {
        return (int)eu_sci_call(pnode, SCN_SAVEPOINTLEFT, 0, 0);
    }
    eu_sci_call(pnode, SCI_BEGINUNDOACTION, 0, 0);
    eu_sci_call(pnode, SCI_INSERTTEXT, 0, (sptr_t) str);
    eu_sci_call(pnode, SCI_DELETERANGE, 0, strlen(str));
    eu_sci_call(pnode, SCI_ENDUNDOACTION, 0, 0);
    return 0;
}

int
on_tabpage_reload_file(eu_tabpage *pnode, int flags)
{
    EU_VERIFY(pnode != NULL);
    switch (flags)
    {
        case 0: // 保留
            on_tabpage_editor_modify(pnode, "X");
            break;
        case 1: // 丢弃
            eu_sci_call(pnode, SCI_SETSAVEPOINT, 0, 0);
            on_file_close(pnode, FILE_ONLY_CLOSE);
            break;
        case 2: // 重载
        {
            sptr_t max_line;
            sptr_t pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
            sptr_t current_line = eu_sci_call(pnode, SCI_LINEFROMPOSITION, pos, 0);
            eu_sci_call(pnode, SCI_CLEARALL, 0, 0);
            if (on_file_to_tab(pnode, NULL))
            {
                return 1;
            }
            max_line = eu_sci_call(pnode, SCI_GETLINECOUNT, 0, 0);
            if (current_line > max_line - 1)
            {
                current_line = max_line - 1;
            }
            eu_sci_call(pnode, SCI_SETUNDOCOLLECTION, 1, 0);
            eu_sci_call(pnode, SCI_EMPTYUNDOBUFFER, 0, 0);
            eu_sci_call(pnode, SCI_SETSAVEPOINT, 0, 0);
            on_search_jmp_line(pnode, current_line, 0);
            pnode->st_mtime = util_last_time(pnode->pathfile);
            break;
        }
        default:
            return 1;
    }
    return 0;
}

int 
on_tabpage_theme_changed(eu_tabpage *p)
{
    if (p && p->hwnd_sc)
    {
        PostMessage(p->hwnd_sc, WM_THEMECHANGED, 0, 0);
    }
    if (eu_get_config()->m_sym_show)
    {
        if (p->hwnd_symlist)
        {
            PostMessage(p->hwnd_symlist, WM_THEMECHANGED, 0, 0);
        }
        else if (p->hwnd_symtree)
        {
            SendMessage(p->hwnd_symtree, WM_THEMECHANGED, 0, 0);
        }
    }
    if (p->edit_show)
    {
        if (p->hwnd_qredit)
        {
            SendMessage(p->hwnd_qredit, WM_THEMECHANGED, 0, 0);
        }
        if (p->hwnd_qrtable)
        {
            SendMessage(p->hwnd_qrtable, WM_THEMECHANGED, 0, 0);
        }
    } 
    return 0;
}

void
on_tabpage_foreach(tab_ptr fntab)
{
    int count = TabCtrl_GetItemCount(g_tabpages);
    for (int index = 0; index < count; ++index)
    {
        TCITEM tci = {TCIF_PARAM};
        TabCtrl_GetItem(g_tabpages, index, &tci);
        eu_tabpage *p = (eu_tabpage *) (tci.lParam);
        if (p)
        {
            fntab(p);
        }
    }
}

eu_tabpage *
on_tabpage_focus_at(void)
{
    int index = -1;
    eu_tabpage *p = NULL;
    if (g_tabpages && (index = TabCtrl_GetCurSel(g_tabpages)) >= 0)
    {
        TCITEM tci = {TCIF_PARAM};
        if (TabCtrl_GetItem(g_tabpages, index, &tci))
        {
            p = (eu_tabpage *) (tci.lParam);
        }
    }
    return p;
}

static void
on_tabpage_set_light(const int m_index)
{
    EU_VERIFY(g_tabpages != NULL);
    int count = TabCtrl_GetItemCount(g_tabpages);
    for (int index = 0; index < count; ++index)
    {
        if (index == m_index)
        {
            TabCtrl_HighlightItem(g_tabpages, index, 1.00);
        }
        else
        {
            TabCtrl_HighlightItem(g_tabpages, index, 0.00);
        }
    }
}

void
on_tabpage_selection(eu_tabpage *pnode, int index)
{
    EU_VERIFY(pnode != NULL && g_tabpages != NULL);
    int count = TabCtrl_GetItemCount(g_tabpages);
    if (index < 0)
    {
        for (index = 0; index < count; ++index)
        {
            TCITEM tci = {TCIF_PARAM};
            TabCtrl_GetItem(g_tabpages, index, &tci);
            eu_tabpage *p = (eu_tabpage *) (tci.lParam);
            if (p == pnode)
            {
                break;
            }
        }
    }
    if(index >= 0 && index < count)
    {
        HWND hwnd = eu_module_hwnd();
        TabCtrl_SetCurSel(g_tabpages, index);
        util_set_title(pnode->pathfile);
        // 切换工作目录
        util_set_working_dir(pnode->pathname);
        on_tabpage_set_light(index);
        menu_update_all(hwnd, pnode);
        eu_window_resize(hwnd);
        if (pnode->hwnd_sc)
        {
            SendMessage(pnode->hwnd_sc, WM_SETFOCUS, 0, 0);
        }
    }
}

eu_tabpage *
on_tabpage_get_handle(void *hwnd_sc)
{
    eu_tabpage *p = NULL;
    EU_VERIFY(g_tabpages != NULL);
    int count = TabCtrl_GetItemCount(g_tabpages);
    for (int index = 0; index < count; ++index)
    {
        p = on_tabpage_get_ptr(index);
        if (p->hwnd_sc == hwnd_sc) 
        {
            return p;
        }
    }
    return NULL;
}

eu_tabpage *
on_tabpage_get_ptr(int index)
{
    EU_VERIFY(g_tabpages != NULL);
    TCITEM tci = {TCIF_PARAM};
    int count = TabCtrl_GetItemCount(g_tabpages);
    if (index < 0 || index >= count) 
    {
        return NULL;
    }
    TabCtrl_GetItem(g_tabpages, index, &tci);
    return (eu_tabpage *) (tci.lParam);
}

static int
on_tabpage_get_index(eu_tabpage *pnode)
{
    EU_VERIFY(pnode != NULL && g_tabpages != NULL);
    eu_tabpage *p = NULL;
    int count = TabCtrl_GetItemCount(g_tabpages);
    for (int index = 0; index < count; ++index)
    {
        TCITEM tci = {TCIF_PARAM};
        TabCtrl_GetItem(g_tabpages, index, &tci);
        p = (eu_tabpage *) (tci.lParam);
        if (p == pnode) 
        {
            return index;
        }
    }
    return -1;
}

eu_tabpage *
on_tabpage_select_index(int index)
{
    eu_tabpage *p = on_tabpage_get_ptr(index);
    if (p)
    {
        on_tabpage_selection(p, index);
        return p;
    }
    return NULL;
}

void
on_tabpage_changing(void)
{
    EU_VERIFY(g_tabpages != NULL);
    int pageno = TabCtrl_GetCurSel(g_tabpages);
    on_tabpage_select_index(pageno);
}

void
on_tabpage_symlist_click(eu_tabpage *pnode)
{
    if (pnode && pnode->doc_ptr && pnode->doc_ptr->fn_click_symlist)
    {
        pnode->doc_ptr->fn_click_symlist(pnode);
    }
}
