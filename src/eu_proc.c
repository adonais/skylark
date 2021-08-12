/*******************************************************************************
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

#ifndef WM_COPYGLOBALDATA
#define WM_COPYGLOBALDATA 0x0049
#endif

#define EU_TIMER_ID 1
#define MAYBE100MS 100
#define APP_CLASS _T("__eu_skylark__")

typedef UINT (WINAPI* GetDpiForWindowPtr)(HWND hwnd);
typedef BOOL(WINAPI *AdjustWindowRectExForDpiPtr)(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle, UINT dpi);

static bool filebar_hover_resize;
static bool is_filebar_resize;
static bool function_hover_resize;
static bool is_function_resize;
static bool view_hover_resize;
static bool is_view_resize;
static bool sql_hover_resize;
static bool is_sql_resize;
static bool result_hover_resize;
static bool is_result_resize;
static HBRUSH g_control_brush;
static HWND eu_hwndmain;  // 主窗口句柄
static volatile long undo_off;

static int
on_create_window(HWND hwnd)
{
    if (on_treebar_create_dlg(hwnd))
    {
        printf("on_treebar_create_dlg return false\n");
        return 1;
    }
    if (on_tabpage_create_dlg(hwnd))
    {
        printf("on_tabpage_create_dlg return false\n");
        return 1;
    }
    return 0;
}

static void
on_destory_window(HWND hwnd)
{
    // 保存主窗口位置
    WINDOWPLACEMENT wp;
    if (GetWindowPlacement(hwnd, &wp))
    {
        char *placement = util_struct_to_string(&wp, sizeof(wp));
        if (placement)
        {
            if (strlen(placement) < MAX_BUFFER)
            {
                sprintf(eu_get_config()->m_placement, "%s", placement);
            }
            free(placement);
        }
    }
    // 销毁定时器
    KillTimer(hwnd, EU_TIMER_ID);
    // 等待搜索完成
    on_search_finish_wait();
    // 销毁全局画刷
    if (g_control_brush)
    {
        DeleteObject(g_control_brush);
        g_control_brush = NULL;
    }
    // 销毁工具栏
    HWND h_tool = GetDlgItem(hwnd, IDC_TOOLBAR);
    if (h_tool)
    {
        DestroyWindow(h_tool);
    }
    // 销毁状态栏
    if (g_statusbar)
    {
        DestroyWindow(g_statusbar);
    }
    // 文件关闭,销毁信号量
    on_file_finish_wait();
    // 结束进程
    PostQuitMessage(0);
}

static bool
adjust_window_rect_dpi(LPRECT lpRect, DWORD dwStyle, DWORD dwExStyle, UINT dpi)
{
    AdjustWindowRectExForDpiPtr fnAdjustWindowRectExForDpi = NULL;
    HMODULE user32 = GetModuleHandle(_T("user32.dll"));
    fnAdjustWindowRectExForDpi = user32 ? (AdjustWindowRectExForDpiPtr)GetProcAddress(user32, "AdjustWindowRectExForDpi") : NULL;
    if (fnAdjustWindowRectExForDpi) 
    {
        return fnAdjustWindowRectExForDpi(lpRect, dwStyle, FALSE, dwExStyle, dpi);
    }
    return AdjustWindowRectEx(lpRect, dwStyle, FALSE, dwExStyle);
}

/*****************************************************************************
 * 在admin模式下启用拖放
 ****************************************************************************/
static void
do_drop_fix(void)
{
    typedef BOOL(WINAPI *ChangeWindowMessageFilterPtr)(UINT message, DWORD flag);
    ChangeWindowMessageFilterPtr fnChangeWindowMessageFilter = NULL;
    HMODULE usr32 = LoadLibrary(_T("user32.dll"));
    if (usr32)
    {
        fnChangeWindowMessageFilter = (ChangeWindowMessageFilterPtr) GetProcAddress(usr32, "ChangeWindowMessageFilter");
        if (fnChangeWindowMessageFilter)
        {
            fnChangeWindowMessageFilter(WM_DROPFILES, MSGFLT_ADD);
            fnChangeWindowMessageFilter(WM_COPYDATA, MSGFLT_ADD);
            fnChangeWindowMessageFilter(WM_COPYGLOBALDATA, MSGFLT_ADD);
        }
        FreeLibrary(usr32);
    }
}

void
eu_reset_drag_line(void)
{
    is_filebar_resize = false;
    is_function_resize = false;
    is_view_resize = false;
    is_sql_resize = false;
    is_result_resize = false;
}

void
eu_clear_undo_off(void)
{
    _InterlockedExchange(&undo_off, 0);
}

HWND
eu_module_hwnd(void)
{
    return eu_hwndmain;
}

uint32_t
eu_get_dpi(HWND hwnd)
{
    uint32_t dpi = 0;
    GetDpiForWindowPtr fnGetDpiForWindow = NULL;
    HMODULE user32 = GetModuleHandle(_T("user32.dll"));
    if (user32)
    {   // PMv2, 使用GetDpiForWindow获取dpi
        fnGetDpiForWindow = (GetDpiForWindowPtr)GetProcAddress(user32, "GetDpiForWindow");
        if (fnGetDpiForWindow && (dpi = fnGetDpiForWindow(hwnd ? hwnd : eu_hwndmain)) > 0)
        {
            return dpi;
        }
    }
    if (!dpi)
    {   // PMv1或Win7系统, 使用GetDeviceCaps获取dpi
        HDC screen = GetDC(hwnd ? hwnd : eu_hwndmain);
        int x = GetDeviceCaps(screen,LOGPIXELSX);
        int y = GetDeviceCaps(screen,LOGPIXELSY);
        ReleaseDC(hwnd ? hwnd : eu_hwndmain, screen);
        dpi = (uint32_t)((x + y)/2);
    }
    return dpi;
}

void 
eu_window_layout_dpi(HWND hwnd, const RECT *pnew_rect, const uint32_t adpi)
{
    const uint32_t flags = SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED;
    if (pnew_rect) 
    {
        SetWindowPos(hwnd, NULL, pnew_rect->left, pnew_rect->top,
                    (pnew_rect->right - pnew_rect->left), (pnew_rect->bottom - pnew_rect->top), flags);
    } 
    else 
    {
        RECT rc = { 0 };
        GetWindowRect(hwnd, &rc);
        const uint32_t dpi = adpi ? adpi : eu_get_dpi(hwnd);
        adjust_window_rect_dpi((LPRECT)&rc, flags, 0, dpi);
        SetWindowPos(hwnd, NULL, rc.left, rc.top, (rc.right - rc.left), (rc.bottom - rc.top), flags);
    }
    RedrawWindow(hwnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_ERASE | RDW_INTERNALPAINT | RDW_ALLCHILDREN | RDW_UPDATENOW);
}

int
eu_dpi_scale_font(void)
{
    return eu_get_dpi(NULL) > 96 ? 0 : -11;
}

int
eu_dpi_scale_xy(int adpi, int m)
{
    int dpx = adpi ? adpi : eu_get_dpi(NULL);
    if (dpx)
    {
        return MulDiv(m, dpx, 96);
    }
    return m;
}

/*****************************************************************************
 * 菜单栏下面 1px 分割线位置
 ****************************************************************************/
static void
get_menu_border_rect(HWND hwnd, LPRECT r)
{
    RECT rc_window;
    POINT client_top = {0};
    GetWindowRect(hwnd, &rc_window);
    ClientToScreen(hwnd, &client_top);
    r->left = 0;
    r->right = rc_window.right - rc_window.left;
    r->top = client_top.y - rc_window.top - 1;
    r->bottom = client_top.y - rc_window.top;
}

/*****************************************************************************
 * 资源管理器与编辑器区分割线的位置
 ****************************************************************************/
static bool
get_treebar_border_rect(HWND hwnd, LPRECT r)
{
    RECT rc_tree = {0};
    RECT rc_main = {0};
    RECT rc_client = {0};
    POINT client_top = {0};
    on_treebar_adjust_box(&rc_tree);
    int scale = eu_dpi_scale_xy(0, SPLIT_WIDTH);
    if (rc_tree.right - rc_tree.left < scale)
    {
        return false;
    }
    GetWindowRect(hwnd, &rc_main);
    GetClientRect(hwnd, &rc_client);
    ClientToScreen(hwnd, &client_top);
    int toolbar_height = on_toolbar_height();
    int tree_hight = rc_client.bottom - rc_client.top - toolbar_height - on_statusbar_height();
    r->left = rc_tree.right - rc_tree.left + SPLIT_WIDTH + scale;
    r->right = r->left + scale;
    r->top = client_top.y - rc_main.top + toolbar_height;
    r->bottom = r->top + tree_hight;
    return true;
}

bool
eu_create_toolbar(HWND hwnd)
{
    return (on_toolbar_create(hwnd) == 0);
}

bool
eu_create_statusbar(HWND hwnd)
{
    return on_statusbar_init(hwnd);
}

bool
eu_create_search_dlg(void)
{
    return on_search_create_box();
}

void
eu_create_fullscreen(HWND hwnd)
{
    on_view_setfullscreenimpl(hwnd);
}

/*****************************************************************************
 * 窗口缩放处理函数
 ****************************************************************************/
void
eu_window_resize(HWND hwnd)
{
    int count = 0;
    eu_tabpage *pnode = NULL;
    RECT rect_treebar = {0};
    if (hwnd)
    {
        eu_get_config()->m_menubar?(GetMenu(hwnd)?(void)0:SetMenu(hwnd, i18n_load_menu(IDC_SKYLARK))):SetMenu(hwnd, NULL);
    }
    on_toolbar_adjust_box();
    on_statusbar_adjust_box();
    on_treebar_adjust_box(&rect_treebar);
    HDWP hdwp = BeginDeferWindowPos(3);
    if (eu_get_config()->m_ftree_show)
    {
        RECT rect_filetree = { 0 };
        on_treebar_adjust_filetree(&rect_treebar, &rect_filetree);
        DeferWindowPos(hdwp,
                       g_treebar,
                       HWND_TOP,
                       rect_treebar.left,
                       rect_treebar.top,
                       rect_treebar.right - rect_treebar.left,
                       rect_treebar.bottom - rect_treebar.top,
                       SWP_SHOWWINDOW);
        DeferWindowPos(hdwp,
                       g_filetree,
                       HWND_TOP,
                       rect_filetree.left,
                       rect_filetree.top,
                       rect_filetree.right - rect_filetree.left,
                       rect_filetree.bottom - rect_filetree.top,
                       SWP_SHOWWINDOW);                       
        ShowWindow(g_treebar, SW_SHOW);
        ShowWindow(g_filetree, SW_SHOW);
        UpdateWindow(g_treebar);
        // on wine, we use RedrawWindow refresh client area
        RedrawWindow(g_filetree, NULL, NULL,RDW_INVALIDATE | RDW_FRAME | RDW_ERASE | RDW_ALLCHILDREN);
    }
    else
    {
        DeferWindowPos(hdwp, g_treebar, 0, 0, 0, 0, 0, SWP_HIDEWINDOW);
        DeferWindowPos(hdwp, g_filetree, 0, 0, 0, 0, 0, SWP_HIDEWINDOW);
    }
    if (true)
    {
        RECT rect_tabbar = { 0 };
        on_tabpage_adjust_box(&rect_tabbar);
        EndDeferWindowPos(hdwp);
        eu_setpos_window(g_tabpages,
                         HWND_TOP,
                         rect_tabbar.left,
                         rect_tabbar.top,
                         rect_tabbar.right - rect_tabbar.left,
                         rect_tabbar.bottom - rect_tabbar.top,
                         SWP_SHOWWINDOW);
    }
    if ((pnode = on_tabpage_focus_at()) != NULL)
    {
        count = TabCtrl_GetItemCount(g_tabpages);
    }
    for (int index = 0; index < count; ++index)
    {
        TCITEM tci = {TCIF_PARAM};
        TabCtrl_GetItem(g_tabpages, index, &tci);
        eu_tabpage *p = (eu_tabpage *) (tci.lParam);
        if (p != pnode)
        {
            if (p->hwnd_symlist && (GetWindowLongPtr(p->hwnd_symlist, GWL_STYLE) & WS_VISIBLE))
            {
                eu_setpos_window(p->hwnd_symlist, HWND_BOTTOM, 0, 0, 0, 0, SWP_HIDEWINDOW);
                ShowWindow(p->hwnd_symlist, SW_HIDE);
            }
            if (p->hwnd_symtree && (GetWindowLongPtr(p->hwnd_symtree, GWL_STYLE) & WS_VISIBLE))
            {
                eu_setpos_window(p->hwnd_symtree, HWND_BOTTOM, 0, 0, 0, 0, SWP_HIDEWINDOW);
                ShowWindow(p->hwnd_symtree, SW_HIDE);
            }
            if (p->hwnd_qredit && (GetWindowLongPtr(p->hwnd_qredit, GWL_STYLE) & WS_VISIBLE))
            {
                eu_setpos_window(pnode->hwnd_qredit, HWND_BOTTOM, 0, 0, 0, 0, SWP_HIDEWINDOW);
                ShowWindow(p->hwnd_qredit, SW_HIDE);
            }
            if (p->hwnd_qrtable && (GetWindowLongPtr(p->hwnd_qrtable, GWL_STYLE) & WS_VISIBLE))
            {
                eu_setpos_window(pnode->hwnd_qrtable, HWND_BOTTOM, 0, 0, 0, 0, SWP_HIDEWINDOW);
                ShowWindow(p->hwnd_qrtable, SW_HIDE);
            }
            if (p->hwnd_sc && (GetWindowLongPtr(p->hwnd_sc, GWL_STYLE) & WS_VISIBLE))
            {
                eu_setpos_window(p->hwnd_sc, HWND_BOTTOM, 0, 0, 0, 0, SWP_HIDEWINDOW);
                ShowWindow(p->hwnd_sc, SW_HIDE);
            }
        }
    }
    if (pnode)
    {
        on_tabpage_adjust_window(pnode);
        if (eu_get_config()->m_sym_show)
        {
            if (pnode->hwnd_symlist)
            {
                if (ListBox_GetCount(pnode->hwnd_symlist) > 0)
                {
                    SetWindowPos(pnode->hwnd_symlist,
                                 HWND_TOP,
                                 pnode->rect_symlist.left,
                                 pnode->rect_symlist.top,
                                 pnode->rect_symlist.right - pnode->rect_symlist.left,
                                 pnode->rect_symlist.bottom - pnode->rect_symlist.top,
                                 SWP_SHOWWINDOW);
                    UpdateWindowEx(pnode->hwnd_symlist);
                }
                else
                {
                    ShowWindow(pnode->hwnd_symlist, SW_HIDE);
                    util_set_menu_item(hwnd, IDM_VIEW_SYMTREE, false);
                }
            }
            else if (pnode->hwnd_symtree)
            {
                if (TreeView_GetCount(pnode->hwnd_symtree) > 0)
                {
                    SetWindowPos(pnode->hwnd_symtree,
                                 HWND_TOP,
                                 pnode->rect_symtree.left,
                                 pnode->rect_symtree.top,
                                 pnode->rect_symtree.right - pnode->rect_symtree.left,
                                 pnode->rect_symtree.bottom - pnode->rect_symtree.top,
                                 SWP_SHOWWINDOW);
                    UpdateWindowEx(pnode->hwnd_symtree);
                }
                else
                {
                    ShowWindow(pnode->hwnd_symtree, SW_HIDE);
                    util_set_menu_item(hwnd, IDM_VIEW_SYMTREE, false);
                }
            }
        }
        else
        {
            if (pnode->hwnd_symlist)
            {
                eu_setpos_window(pnode->hwnd_symlist, HWND_BOTTOM, 0, 0, 0, 0, SWP_HIDEWINDOW);
                ShowWindow(pnode->hwnd_symlist, SW_HIDE);
            }
            if (pnode->hwnd_symtree)
            {
                eu_setpos_window(pnode->hwnd_symtree, HWND_BOTTOM, 0, 0, 0, 0, SWP_HIDEWINDOW);
                ShowWindow(pnode->hwnd_symtree, SW_HIDE);
            }
        }
        if (pnode->edit_show)
        {
            if (pnode->hwnd_qredit)
            {
                SetWindowPos(pnode->hwnd_qredit,
                             HWND_TOP,
                             pnode->rect_qredit.left,
                             pnode->rect_qredit.top,
                             pnode->rect_qredit.right - pnode->rect_qredit.left,
                             pnode->rect_qredit.bottom - pnode->rect_qredit.top,
                             SWP_SHOWWINDOW);
                UpdateWindowEx(pnode->hwnd_qredit);
            }
            if (pnode->hwnd_qrtable)
            {
                SetWindowPos(pnode->hwnd_qrtable,
                             HWND_TOP,
                             pnode->rect_qrtable.left,
                             pnode->rect_qrtable.top,
                             pnode->rect_qrtable.right - pnode->rect_qrtable.left,
                             pnode->rect_qrtable.bottom - pnode->rect_qrtable.top,
                             SWP_SHOWWINDOW);
                UpdateWindowEx(pnode->hwnd_qrtable);
            }
        }
        if (pnode->hwnd_sc)
        {
            SetWindowPos(pnode->hwnd_sc,
                         HWND_TOP,
                         pnode->rect_sc.left,
                         pnode->rect_sc.top,
                         pnode->rect_sc.right - pnode->rect_sc.left,
                         pnode->rect_sc.bottom - pnode->rect_sc.top,
                         SWP_SHOWWINDOW);
            
        }
    }
    on_toolbar_size();
    on_statusbar_size();
    if (g_tabpages)
    {
        InvalidateRect(g_tabpages, NULL, true);
        UpdateWindow(g_tabpages);
    }
    if (pnode && pnode->hwnd_sc)
    {
        SetFocus(pnode->hwnd_sc);
        UpdateWindowEx(pnode->hwnd_sc);
    }
}

int
eu_before_proc(MSG *p_msg)
{
    eu_tabpage *pnode = NULL;
    if (p_msg->message == WM_SYSKEYUP && p_msg->wParam == VK_MENU)
    {
        bool extended = KEY_DOWN(VK_CONTROL) || KEY_DOWN(VK_SHIFT) || KEY_DOWN(VK_LWIN) || KEY_DOWN(VK_TAB) || KEY_DOWN(VK_LBUTTON);
        if (!extended && !(p_msg->lParam & 0xff00))
        {  // only left alt press
            eu_get_config()->m_menubar = !eu_get_config()->m_menubar;
            eu_window_resize(eu_hwndmain);
            menu_update_all(eu_hwndmain, NULL);
            return 1;
        }
    } 
    if (p_msg->message == WM_SYSKEYDOWN && 49 <= p_msg->wParam && p_msg->wParam <= 57 && (p_msg->lParam & (1 << 29)))
    {
        if ((pnode = on_tabpage_select_index((uint32_t) (p_msg->wParam) - 49)))
        {
            return 1;
        }
    }
    if((pnode = on_tabpage_focus_at()) && !pnode->hex_mode && pnode->doc_ptr && p_msg->message == WM_KEYDOWN && p_msg->hwnd == pnode->hwnd_sc)
    {
        bool main_key = KEY_DOWN(VK_CONTROL) && KEY_DOWN(VK_SHIFT) && KEY_DOWN(VK_MENU) && KEY_DOWN(VK_INSERT);
        if (main_key && pnode->doc_ptr->doc_type == DOCTYPE_CPP)
        {
            on_sci_insert_egg(pnode);
            return 1;
        } 
    }
    if (p_msg->message == WM_MOUSEMOVE)
    {
        WPARAM wParam = p_msg->wParam;
        LPARAM lParam = p_msg->lParam;
        if (p_msg->hwnd != eu_hwndmain)
        {
            POINT pt = {LOWORD(lParam), HIWORD(lParam)};
            ClientToScreen(p_msg->hwnd, &pt);
            ScreenToClient(eu_hwndmain, &pt);
            lParam = MAKELONG(pt.x, pt.y);
        }
        wParam = MAKELONG(IDM_MOUSEMOVE, 0);
        PostMessage(eu_hwndmain, WM_COMMAND, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK
eu_main_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int width, height;
    int wm_id, wm_nc;
    HWND wm_hwnd;
    NMHDR *lpnmhdr = NULL;
    SCNotification *lpnotify = NULL;
    NMTREEVIEW *lpnmtv = NULL;
    TOOLTIPTEXT *p_tips = NULL;
    eu_tabpage *pnode = NULL;
    LRESULT result = 0;
    if (eu_get_config()->m_menubar && on_dark_enable() && on_theme_menu_proc(hwnd, message, wParam, lParam, &result))
    {
        return result;
    }
    switch (message)
    {
        case WM_MOUSEACTIVATE:
        case WM_NCHITTEST:
        case WM_NCCALCSIZE:
        case WM_PAINT:
        case WM_NCMOUSEMOVE:
        case WM_NCLBUTTONDOWN:
        case WM_WINDOWPOSCHANGING:
        case WM_WINDOWPOSCHANGED:
            return DefWindowProc(hwnd, message, wParam, lParam);
        case WM_ERASEBKGND:
            if (!on_dark_enable())
            {
                return DefWindowProc(hwnd, message, wParam, lParam);
            }
            return 1;    
        case WM_CREATE:
            if (on_create_window(hwnd))
            {
                PostQuitMessage(0);
            }
            if (!SetTimer(hwnd, EU_TIMER_ID, MAYBE100MS, NULL))
            {
                PostQuitMessage(0);
            }
            if (eu_get_config()->m_fullscreen)
            {
                eu_get_config()->m_menubar = false;
                eu_get_config()->m_toolbar = false;
                eu_get_config()->m_statusbar = false;
                on_view_setfullscreenimpl(hwnd);
            }
            break;
        case WM_NCPAINT:
        {
            LRESULT result = DefWindowProc(hwnd, WM_NCPAINT, wParam, lParam);
            if (!on_dark_enable())
            {   // 系统dark模式关闭时, 动态刷新主题
                if (strcmp(eu_get_config()->window_theme, "black") == 0 && on_dark_supports())
                {
                    eu_on_dark_release(false);
                    eu_window_resize(hwnd);
                }
            }
            else
            {
                HDC hdc = GetWindowDC(hwnd);
                RECT r = {0};
                RECT rect_tree = {0};
                get_menu_border_rect(hwnd, &r);
                FillRect(hdc, &r, (HBRUSH)on_dark_get_brush());
                if (get_treebar_border_rect(hwnd, &rect_tree))
                {   // 重新绘制分割线
                    FillRect(hdc, &rect_tree, (HBRUSH)on_dark_get_brush());
                }
                ReleaseDC(hwnd, hdc);                
            }
            return result;
        }
        case WM_NCACTIVATE:
            return 1;
        case WM_SIZE:
            if (wParam == SIZE_MINIMIZED)
            {
                break;
            }
            eu_window_resize(hwnd);
            break;
        case WM_TIMER:
            if (on_qrgen_hwnd() && KEY_DOWN(VK_ESCAPE))
            {
                EndDialog(on_qrgen_hwnd(), 0);
            }
            if (true)
            {   // 是否按下大写键
                on_statusbar_btn_case();
            }
            if (eu_hwndmain == GetForegroundWindow())
            {
                ONCE_RUN(on_changes_window(hwnd));
            }
            return 0;
        case WM_DPICHANGED:
        {
            on_theme_setup_font(hwnd);
            on_tabpage_foreach(hexview_update_theme);
            on_statusbar_init(hwnd);
            SendMessage(g_treebar, WM_DPICHANGED, 0, 0);
            break;
        }
        case WM_CTLCOLORLISTBOX:
        case WM_CTLCOLOREDIT:
        {  // 为控件创建单独的画刷,用来绘制背景色
            HDC hdc = (HDC) wParam;
            if (g_control_brush)
            {
                DeleteObject(g_control_brush);
            }
            g_control_brush = CreateSolidBrush(eu_get_theme()->item.text.bgcolor);
            SetTextColor(hdc, eu_get_theme()->item.text.color);
            SetBkColor(hdc, eu_get_theme()->item.text.bgcolor);
            SetBkMode(hdc, TRANSPARENT);
            return (LRESULT)g_control_brush;
        }
        case WM_DRAWITEM:
        {
            uint16_t wp = LOWORD(wParam);
            switch (wp)
            {
                case IDC_STATUSBAR:
                    if (g_statusbar)
                    {
                        return on_statusbar_draw_item(hwnd, wParam, lParam);
                    }
                case IDM_TREE_BAR:
                case IDM_TABPAGE_BAR:
                    if (g_tabpages)
                    {
                        return on_tabpage_draw_item(hwnd, wParam, lParam);
                    }
                default:
                    break;
            }
            break;
        }
        case WM_THEMECHANGED:
            if (on_dark_supports())
            {
                on_dark_allow_window(hwnd, true);
                on_dark_refresh_titlebar(hwnd);
                on_tabpage_foreach(on_tabpage_theme_changed);
                if (g_statusbar && on_statusbar_init(hwnd))
                {
                    on_statusbar_size();
                }
                if (on_toolbar_refresh(hwnd))
                {
                    on_toolbar_size();
                }
                on_dark_set_theme(g_treebar, L"Explorer", NULL);
                on_dark_set_theme(g_tabpages, L"Explorer", NULL);
                if (g_filetree)
                {
                    SendMessage(g_filetree, WM_THEMECHANGED, 0, 0);
                }
                on_dark_set_theme(eu_get_search_hwnd(), L"Explorer", NULL);
            }
            break;
        case WM_COMMAND:
        {
            wm_id = LOWORD(wParam);
            wm_nc = HIWORD(wParam);
            wm_hwnd = (HWND) lParam;
            pnode = on_tabpage_focus_at();
            if (IDM_HISTORY_BASE <= wm_id && wm_id <= IDM_HISTORY_BASE + PATH_MAX_RECENTLY - 1)
            {
                int len = 0;
                HMENU file_menu = NULL;
                HMENU hpop = NULL;
                file_backup bak = {0};
                HMENU root_menu = GetMenu(eu_hwndmain);
                if (root_menu)
                {
                    file_menu = GetSubMenu(root_menu, 0);
                    hpop = GetSubMenu(file_menu, 2);
                }
                if (root_menu && file_menu && hpop)
                {
                    len = GetMenuString(hpop, wm_id, bak.rel_path, MAX_PATH, MF_BYCOMMAND);
                }
                if (len > 0)
                {
                    if (_tcsnicmp(bak.rel_path, _T("sftp://"), 7) != 0 && _tcsrchr(bak.rel_path, _T('/')))
                    {
                        eu_wstr_replace(bak.rel_path, MAX_PATH, _T("/"), _T("\\"));
                    }
                    on_file_only_open(&bak);
                }
                break;
            }
            if (IDM_STYLETHEME_BASE <= wm_id && wm_id <= IDM_STYLETHEME_BASE + VIEW_STYLETHEME_MAXCOUNT - 1)
            {
                on_view_switch_theme(eu_hwndmain, wm_id);
                break;
            }
            if (IDM_LOCALES_BASE <= wm_id && wm_id <= IDM_LOCALES_BASE + MAX_MULTI_LANG - 1)
            {
                i18n_switch_locale(eu_hwndmain, wm_id);
                break;
            }
            // analysis menu
            if (wm_nc == LBN_DBLCLK)
            {
                width = 0;
                height = 0;
            }
            switch (wm_id)
            {
                case IDM_FILE_NEW:
                    on_file_new();
                    break;
                case IDM_FILE_OPEN:
                    on_file_open();
                    break;
                case IDM_HISTORY_CLEAN:
                    on_file_clear_recent();
                    break;
                case IDM_FILE_SAVE:
                    on_file_save(pnode, false);
                    break;
                case IDM_FILE_SAVEAS:
                    on_file_save_as(pnode);
                    break;
                case IDM_FILE_SAVEALL:
                    on_file_all_save();
                    break;
                case IDM_FILE_CLOSE:
                    on_file_close(pnode, FILE_ONLY_CLOSE);
                    break;
                case IDM_FILE_CLOSEALL:
                    on_file_all_close();
                    break;
                case IDM_FILE_CLOSEALL_EXCLUDE:
                    on_file_exclude_close(pnode);
                    break;
                case IDM_FILE_WRITE_COPY:
                    on_file_backup_menu();
                    break;                    
                case IDM_FILE_SESSION:
                    on_file_session_menu();
                    break;
                case IDM_FILE_PAGESETUP:
                    on_print_setup(eu_hwndmain);
                    break;
                case IDM_FILE_PRINT:
                    on_print_file(pnode);
                    break;
                case IDM_FILE_REMOTE_FILESERVERS:
                    on_remote_manager();
                    break;
                case IDM_FILE_NEWFILE_WINDOWS_EOLS:
                    on_file_new_eols(pnode, 0);
                    break;
                case IDM_FILE_NEWFILE_MAC_EOLS:
                    on_file_new_eols(pnode, 1);
                    break;
                case IDM_FILE_NEWFILE_UNIX_EOLS:
                    on_file_new_eols(pnode, 2);
                    break;
                case IDM_FILE_NEWFILE_ENCODING_UTF8:
                    on_file_new_encoding(pnode, IDM_UNI_UTF8);
                    break;
                case IDM_FILE_NEWFILE_ENCODING_UTF8B:
                    on_file_new_encoding(pnode, IDM_UNI_UTF8B);
                    break;
                case IDM_FILE_NEWFILE_ENCODING_UTF16LE:
                    on_file_new_encoding(pnode, IDM_UNI_UTF16LEB);
                    break;
                case IDM_FILE_NEWFILE_ENCODING_UTF16BE:
                    on_file_new_encoding(pnode, IDM_UNI_UTF16BEB);
                    break;
                case IDM_FILE_NEWFILE_ENCODING_ANSI:
                    on_file_new_encoding(pnode, IDM_OTHER_ANSI);
                    break;
                case IDM_EXIT:
                    on_file_edit_exit(hwnd);
                    break;
                case IDM_EDIT_UNDO:
                    on_edit_undo(pnode);
                    break;
                case IDM_EDIT_REDO:
                    on_edit_redo(pnode);
                    break;
                case IDM_EDIT_CUT:
                    on_edit_cut(pnode);
                    break;
                case IDM_EDIT_COPY:
                    on_edit_copy_text(pnode);
                    break;
                case IDM_EDIT_PASTE:
                    on_edit_paste_text(pnode);
                    break;
                case IDM_EDIT_DELETE:
                    on_edit_delete_text(pnode);
                    break;
                case IDM_EDIT_CUTLINE:
                    on_edit_cut_line(pnode);
                    break;
                case IDM_EDIT_CUTLINE_AND_PASTELINE:
                    on_edit_cut_line_paste(pnode);
                    break;
                case IDM_EDIT_COPYLINE:
                    on_edit_copy_line(pnode);
                    break;
                case IDM_EDIT_COPYLINE_AND_PASTELINE:
                    on_edit_copy_line_paste(pnode);
                    break;
                case IDM_EDIT_COPY_FILENAME:
                    if (pnode && *pnode->filename)
                    {
                        on_edit_copy_filename(pnode->filename);
                    }
                    break;
                case IDM_EDIT_COPY_PATHNAME:
                    if (pnode && *pnode->pathname)
                    {
                        on_edit_push_clipboard(pnode->pathname);
                    }
                    break;
                case IDM_EDIT_COPY_PATHFILENAME:
                    if (pnode && *pnode->pathfile)
                    {
                        on_edit_push_clipboard(pnode->pathfile);
                    }
                    break;
                case IDM_FILE_WORKSPACE:
                    if (pnode && *pnode->pathfile)
                    {
                        if (pnode->is_blank)
                        {
                            break;
                        }
                        on_treebar_locate_path(pnode->pathfile);
                    }
                    break;
                case IDM_EDIT_PASTELINE:
                    on_edit_paste_line(pnode);
                    break;
                case IDM_EDIT_PASTELINE_UPSTAIRS:
                    on_edit_paste_line_up(pnode);
                    break;
                case IDM_EDIT_DELETELINE:
                    on_edit_delete_line(pnode);
                    break;
                case IDM_DELETE_SPACE_LINEHEAD:
                    on_edit_delete_line_header_white(pnode);
                    break;
                case IDM_DELETE_SPACE_LINETAIL:
                    on_edit_delete_line_tail_white(pnode);
                    break;
                case IDM_EDIT_DELETEBLANKLINE:
                    on_edit_delete_blank_line(pnode);
                    break;
                case IDM_DELETE_ALL_SPACE_LINE:
                    on_edit_delete_all_empty_lines(pnode);
                    break;
                case IDM_EDIT_JOINLINE:
                    on_edit_join_line(pnode);
                    break;
                case IDM_EDIT_LINECOMMENT:
                    on_edit_comment_line(pnode);
                    break;
                case IDM_EDIT_STREAMCOMMENT:
                    on_edit_comment_stream(pnode);
                    break;
                case IDM_EDIT_LOWERCASE:
                    on_edit_lower(pnode);
                    break;
                case IDM_EDIT_UPPERCASE:
                    on_edit_upper(pnode);
                    break;
                case IDM_EDIT_QRCODE:
                    on_qrgen_create_dialog();
                    break;
                case IDM_EDIT_GB_BIG5:
                    on_encoding_convert_internal_code(pnode, on_encoding_gb_big5);
                    break;
                case IDM_EDIT_BIG5_GB:
                    on_encoding_convert_internal_code(pnode, on_encoding_big5_gb);
                    break;
                case IDM_EDIT_AUTO_CLOSECHAR:
                    on_edit_close_char(pnode);
                    break;
                case IDM_EDIT_AUTO_INDENTATION:
                    on_edit_identation(pnode);
                    break;
                case IDM_OPEN_FILE_PATH:
                {
                    on_edit_selection(pnode, 0);
                    break;
                }
                case IDM_OPEN_CONTAINING_FOLDER:
                {
                    on_edit_selection(pnode, 1);
                    break;
                }
                case IDM_ONLINE_SEARCH_GOOGLE:
                {
                    on_edit_selection(pnode, 2);
                    break;
                }
                case IDM_ONLINE_SEARCH_BAIDU:
                {
                    on_edit_selection(pnode, 3);
                    break;
                }
                case IDM_ONLINE_SEARCH_BING:
                {
                    on_edit_selection(pnode, 4);
                    break;
                }
                case IDM_EDIT_BASE64_ENCODING:
                    on_edit_base64_enc(pnode);
                    break;
                case IDM_EDIT_BASE64_DECODING:
                    on_edit_base64_dec(pnode);
                    break;
                case IDM_EDIT_MD5:
                    on_edit_md5(pnode);
                    break;
                case IDM_EDIT_SHA1:
                    on_edit_sha1(pnode);
                    break;
                case IDM_EDIT_SHA256:
                    on_edit_sha256(pnode);
                    break;
                case IDM_EDIT_3DES_CBC_ENCRYPTO:
                    on_edit_descbc_enc(pnode);
                    break;
                case IDM_EDIT_3DES_CBC_DECRYPTO:
                    on_edit_descbc_dec(pnode);
                    break;
                case IDM_SEARCH_FIND:
                    on_search_find_thread(pnode);
                    break;
                case IDM_SEARCH_FINDPREV:
                    on_search_find_pre(pnode);
                    break;
                case IDM_SEARCH_FINDNEXT:
                    on_search_find_next(pnode);
                    break;
                case IDM_SEARCH_REPLACE:
                    on_search_replace_thread(pnode);
                    break;
                case IDM_SEARCH_FILES:
                    on_search_file_thread(NULL);
                    break;
                case IDM_UPDATE_SELECTION:
                    on_search_set_selection(pnode);
                    break;
                case IDM_SELECTION_RECTANGLE:
                    on_search_set_rectangle(pnode);
                    break;
                case IDM_SEARCH_SELECTALL:
                    on_search_select_all(pnode);
                    break;
                case IDM_SEARCH_SELECTWORD:
                    on_search_select_word(pnode);
                    break;
                case IDM_SEARCH_SELECTLINE:
                    on_search_select_line(pnode);
                    break;
                case IDM_SEARCH_SELECTGROUP_LEFT:
                    on_search_left_group(pnode);
                    break;
                case IDM_SEARCH_SELECTGROUP_RIGHT:
                    on_search_right_group(pnode);
                    break;
                case IDM_SEARCH_ADDSELECT_LEFT_WORD:
                    on_search_left_word(pnode);
                    break;
                case IDM_SEARCH_ADDSELECT_RIGHT_WORD:
                    on_search_right_word(pnode);
                    break;
                case IDM_SEARCH_SELECTTOP_FIRSTLINE:
                    on_search_cumulative_previous_block(pnode);
                    break;
                case IDM_SEARCH_SELECTBOTTOM_FIRSTLINE:
                    on_search_cumulative_next_block(pnode);
                    break;
                case IDM_SEARCH_MOVE_LEFT_CHARGROUP:
                    on_search_move_to_lgroup(pnode);
                    break;
                case IDM_SEARCH_MOVE_RIGHT_CHARGROUP:
                    on_search_move_to_rgroup(pnode);
                    break;
                case IDM_SEARCH_MOVE_LEFT_WORD:
                    on_search_move_to_lword(pnode);
                    break;
                case IDM_SEARCH_MOVE_RIGHT_WORD:
                    on_search_move_to_rword(pnode);
                    break;
                case IDM_SEARCH_MOVETOP_FIRSTLINE:
                    on_search_move_to_top_block(pnode);
                    break;
                case IDM_SEARCH_MOVEBOTTOM_FIRSTLINE:
                    on_search_move_to_bottom_block(pnode);
                    break;
                case IDM_SEARCH_TOGGLE_BOOKMARK:
                    on_search_toggle_mark(pnode, -1);
                    break;
                case IDM_SEARCH_ADD_BOOKMARK:
                    on_search_add_mark(pnode, -1);
                    break;
                case IDM_SEARCH_REMOVE_BOOKMARK:
                    on_search_remove_marks_this(pnode);
                    break;
                case IDM_SEARCH_REMOVE_ALL_BOOKMARKS:
                    on_search_remove_marks_all(pnode);
                    break;
                case IDM_SEARCH_GOTO_PREV_BOOKMARK:
                    on_search_jmp_premark_this(pnode);
                    break;
                case IDM_SEARCH_GOTO_NEXT_BOOKMARK:
                    on_search_jmp_next_mark_this(pnode);
                    break;
                case IDM_SEARCHPRE_BOOKMARK_INALL:
                    on_search_jmp_premark_all(pnode);
                    break;
                case IDM_SEARCH_BOOKMARK_INALL:
                    on_search_jmp_next_mark_all(pnode);
                    break;
                case IDM_SEARCH_GOTOHOME:
                    on_search_jmp_home(pnode);
                    break;
                case IDM_SEARCH_GOTOEND:
                    on_search_jmp_end(pnode);
                    break;
                case IDM_SEARCH_GOTOLINE:
                    on_search_jmp_specified_line(pnode);
                    break;
                case IDM_SEARCH_NAVIGATE_PREV_THIS:
                    on_search_back_navigate_this();
                    break;
                case IDM_SEARCH_NAVIGATE_PREV_INALL:
                    on_search_back_navigate_all();
                    break;
                case IDM_SEARCH_MULTISELECT_README:
                    MSG_BOX(IDC_MSG_HELP_INF1, IDC_MSG_JUST_HELP, MB_OK);
                    break;
                case IDM_VIEW_FILETREE:
                    on_view_filetree();
                    break;
                case IDM_VIEW_SYMTREE:
                    on_view_symtree(pnode);
                    break;
                case IDM_VIEW_MODIFY_STYLETHEME:
                    on_view_modify_theme();
                    break;
                case IDM_VIEW_COPYNEW_STYLETHEME:
                    on_view_copy_theme();
                    break;
                case IDM_VIEW_HEXEDIT_MODE:
                    hexview_switch_mode(pnode);
                    break;
                case IDM_VIEW_HIGHLIGHT_STR:
                    on_view_light_str(hwnd);
                    break;
                case IDM_VIEW_HIGHLIGHT_FOLD:
                    on_view_light_fold(hwnd);
                    break; 
                case IDM_FORMAT_REFORMAT:
                    format_do_json_file(pnode, format_do_json_string);
                    on_symtree_json(pnode);
                    break;
                case IDM_FORMAT_COMPRESS:
                    format_do_json_file(pnode, format_undo_json_string);
                    on_symtree_json(pnode);
                    break; 
                case IDM_FORMAT_WHOLE_FILE:
                    format_file_with_clang(pnode);
                    on_symlist_reqular(pnode);
                    break;
                case IDM_FORMAT_RANGLE_STR:
                    format_str_with_clang(pnode);
                    on_symlist_reqular(pnode);
                    break;
                case IDM_FORMAT_RUN_SCRIPT:
                    on_toolbar_lua_exec(pnode);
                    break;
                case IDM_FORMAT_BYTE_CODE:
                    do_byte_code(pnode);
                    break;
                case IDM_VIEW_WRAPLINE_MODE:
                    on_view_wrap_line(hwnd);
                    break;
                case IDM_VIEW_TAB_WIDTH:
                    on_view_tab_width(hwnd, pnode);
                    break;
                case IDM_TAB_CONVERT_SPACES:
                    on_view_space_converter(hwnd, pnode);
                    break;
                case IDM_VIEW_LINENUMBER_VISIABLE:
                    on_view_line_num(hwnd);
                    break;
                case IDM_VIEW_BOOKMARK_VISIABLE:
                    on_view_bookmark(hwnd);
                    break;
                case IDM_VIEW_WHITESPACE_VISIABLE:
                    on_view_white_space(hwnd);
                    break;
                case IDM_VIEW_NEWLINE_VISIABLE:
                    on_view_line_visiable(hwnd);
                    break;
                case IDM_VIEW_INDENTGUIDES_VISIABLE:
                    on_view_indent_visiable(hwnd);
                    break;
                case IDM_VIEW_ZOOMOUT:
                    on_view_zoom_out(pnode);
                    break;
                case IDM_VIEW_ZOOMIN:
                    on_view_zoom_in(pnode);
                    break;
                case IDM_VIEW_ZOOMRESET:
                    on_view_zoom_reset(pnode);
                    break;
                case IDM_SOURCE_BLOCKFOLD_VISIABLE:
                    on_view_show_fold_lines(hwnd);
                    break;
                case IDM_SOURCE_BLOCKFOLD_TOGGLE:
                    on_code_switch_fold(pnode);
                    break;
                case IDM_SOURCE_BLOCKFOLD_CONTRACT:
                    on_code_block_contract(pnode);
                    break;
                case IDM_SOURCE_BLOCKFOLD_EXPAND:
                    on_code_block_expand(pnode);
                    break;
                case IDM_SOURCE_BLOCKFOLD_CONTRACTALL:
                    on_code_block_contract_all(pnode);
                    break;
                case IDM_SOURCE_BLOCKFOLD_EXPANDALL:
                    on_code_block_expand_all(pnode);
                    break;
                case IDM_SOURCECODE_GOTODEF:
                    if (pnode && pnode->doc_ptr && pnode->doc_ptr->fn_keydown)
                    {
                        pnode->doc_ptr->fn_keydown(pnode, VK_F12, lParam);
                    }
                    break;
                case IDM_SOURCEE_ENABLE_ACSHOW:
                    on_code_block_complete();
                    break;
                case IDM_SOURCEE_ACSHOW_CHARS:
                    on_code_set_complete_chars(pnode);
                    break;
                case IDM_SOURCE_ENABLE_CTSHOW:
                    on_code_block_calltip();
                    break;
                case IDM_VIEW_FONTQUALITY_NONE:
                case IDM_VIEW_FONTQUALITY_STANDARD:
                case IDM_VIEW_FONTQUALITY_CLEARTYPE:
                    on_view_font_quality(hwnd, wm_id);
                    break;
                case IDM_SET_RENDER_TECH_GDI:
                case IDM_SET_RENDER_TECH_D2D:
                case IDM_SET_RENDER_TECH_D2DRETAIN:
                    on_view_enable_rendering(hwnd, wm_id);
                    break;
                case IDM_DATABASE_INSERT_CONFIG:  // 插入sql头
                case IDM_REDIS_INSERT_CONFIG:
                    on_code_insert_config(pnode);
                    break;                  
                case IDM_DATABASE_EXECUTE_SQL:  // 执行选定sql
                case IDM_REDIS_EXECUTE_COMMAND:
                    on_view_result_show(pnode, 0);
                    break;
                case IDM_PROGRAM_EXECUTE_ACTION:  // 执行预置动作
                    on_toolbar_execute_script();
                    break;
                case IDM_ENV_FILE_POPUPMENU:
                    on_reg_file_popup_menu();
                    break;
                case IDM_ENV_DIRECTORY_POPUPMENU:
                    on_reg_dir_popup_menu();
                    break;
                case IDM_ENV_SET_ASSOCIATED_WITH:
                    on_reg_files_association();
                    break;
                case IDM_DONATION:
                    on_about_donation();
                    break;
                case IDM_INTRODUTION:
                {
                    file_backup bak = {0};
                    _sntprintf(bak.rel_path, MAX_PATH - 1, _T("%s\\README_CN.MD"), eu_module_path);
                    on_file_only_open(&bak);
                    break;
                }
                case IDM_CHANGELOG:
                {
                    file_backup bak = {0};
                    _sntprintf(bak.rel_path, MAX_PATH - 1, _T("%s\\share\\changelog"), eu_module_path);
                    on_file_only_open(&bak);
                    break;
                }
                case IDM_VIEW_FULLSCREEN:
                {
                    on_view_full_sreen(hwnd);
                    break;
                }
                case IDM_VIEW_MENUBAR:
                    eu_get_config()->m_menubar = !eu_get_config()->m_menubar;
                    menu_update_all(hwnd, NULL);
                    eu_window_resize(hwnd);
                    break;
                case IDM_VIEW_TOOLBAR:
                    eu_get_config()->m_toolbar = !eu_get_config()->m_toolbar;
                    menu_update_all(hwnd, NULL);
                    eu_window_resize(hwnd);
                    break;
                case IDM_VIEW_STATUSBAR:
                    eu_get_config()->m_statusbar = !eu_get_config()->m_statusbar;
                    menu_update_all(hwnd, NULL);
                    eu_window_resize(hwnd);
                    break;
                case IDM_ABOUT:
                    on_about_dialog();
                    break;
                case IDM_MOUSEMOVE:
                    if (eu_get_config()->m_ftree_show)
                    {
                        RECT rect_treebar = { 0 };
                        RECT rect_tabbar = { 0 };
                        int x = GET_X_LPARAM(lParam);
                        int y = GET_Y_LPARAM(lParam);
                        on_treebar_adjust_box(&rect_treebar);
                        on_tabpage_adjust_box(&rect_tabbar);
                        if (is_filebar_resize)
                        {
                            SetCursor(LoadCursor(NULL, IDC_SIZEWE));
                        }
                        else if (rect_treebar.top <= y && y <= rect_treebar.bottom && rect_treebar.right < x && x < rect_tabbar.left)
                        {
                            SetCursor(LoadCursor(NULL, IDC_SIZEWE));
                            if (!filebar_hover_resize)
                            {
                                filebar_hover_resize = true;
                            }
                        }
                        else
                        {
                            if (filebar_hover_resize)
                            {
                                filebar_hover_resize = false;
                            }
                        }
                        if (is_filebar_resize)
                        {
                            eu_get_config()->file_tree_width = x - rect_treebar.left - SPLIT_WIDTH / 2;
                            if (eu_get_config()->file_tree_width < FILETREEBAR_WIDTH_MIN)
                            {
                                eu_get_config()->file_tree_width = FILETREEBAR_WIDTH_MIN;
                            }
                            eu_window_resize(eu_hwndmain);
                        }
                    }
                    if (!pnode)
                    {
                        break;
                    }
                    if (pnode->hwnd_symlist)
                    {
                        int x = GET_X_LPARAM(lParam);
                        int y = GET_Y_LPARAM(lParam);

                        if (is_function_resize)
                        {
                            SetCursor(LoadCursor(NULL, IDC_SIZEWE));
                        }
                        else if (pnode->rect_symlist.top <= y && y <= pnode->rect_symlist.bottom && pnode->rect_sc.right < x && x < pnode->rect_symlist.left)
                        {
                            SetCursor(LoadCursor(NULL, IDC_SIZEWE));
                            function_hover_resize = true;
                        }
                        else
                        {
                            if (function_hover_resize)
                            {
                                function_hover_resize = false;
                            }
                        }
                        if (is_function_resize)
                        {
                            eu_get_config()->sym_list_width = pnode->rect_symlist.right - x - SPLIT_WIDTH / 2;
                            if (eu_get_config()->sym_list_width < SYMBOLLIST_WIDTH_MIN)
                            {
                                eu_get_config()->sym_list_width = SYMBOLLIST_WIDTH_MIN;
                            }
                            eu_window_resize(eu_hwndmain);
                        }
                    }
                    if (pnode->hwnd_symtree)
                    {
                        int x = GET_X_LPARAM(lParam);
                        int y = GET_Y_LPARAM(lParam);

                        if (is_view_resize)
                        {
                            SetCursor(LoadCursor(NULL, IDC_SIZEWE));
                        }
                        else if (pnode->rect_symtree.top <= y && y <= pnode->rect_symtree.bottom && pnode->rect_sc.right < x && x < pnode->rect_symtree.left)
                        {
                            SetCursor(LoadCursor(NULL, IDC_SIZEWE));
                            view_hover_resize = true;
                        }
                        else
                        {
                            if (view_hover_resize)
                            {
                                view_hover_resize = false;
                            }
                        }

                        if (is_view_resize)
                        {
                            eu_get_config()->sym_tree_width = pnode->rect_symtree.right - x - SPLIT_WIDTH / 2;
                            if (eu_get_config()->sym_tree_width < TREEVIEW_WIDTH_MIN)
                            {
                                eu_get_config()->sym_tree_width = TREEVIEW_WIDTH_MIN;
                            }
                            eu_window_resize(eu_hwndmain);
                        }
                    }
                    if (pnode->hwnd_qredit)
                    {
                        int x = GET_X_LPARAM(lParam);
                        int y = GET_Y_LPARAM(lParam);

                        if (is_sql_resize)
                        {
                            SetCursor(LoadCursor(NULL, IDC_SIZENS));
                            if (pnode->hwnd_qrtable)
                            {
                                eu_get_config()->result_edit_height = pnode->rect_qredit.bottom - y - SPLIT_WIDTH/2;
                            }
                            else
                            {
                                eu_get_config()->result_edit_height = 
                                    pnode->rect_qredit.bottom - y - SPLIT_WIDTH - eu_get_config()->result_list_height;
                            }
                            if (eu_get_config()->result_edit_height < SQLQUERYRESULT_LISTVIEW_HEIGHT_MIN)
                            {
                                eu_get_config()->result_edit_height = SQLQUERYRESULT_LISTVIEW_HEIGHT_MIN;
                            }
                            eu_window_resize(eu_hwndmain);
                        }
                        else if (pnode->rect_qredit.left <= x && x <= pnode->rect_qredit.right && pnode->rect_sc.bottom < y &&
                                 y < pnode->rect_qredit.top)
                        {
                            SetCursor(LoadCursor(NULL, IDC_SIZENS));
                            sql_hover_resize = true;
                        }
                        else
                        {
                            if (sql_hover_resize)
                            {
                                sql_hover_resize = false;
                            }
                        }
                    }
                    if (pnode->hwnd_qrtable)
                    {
                        int x = GET_X_LPARAM(lParam);
                        int y = GET_Y_LPARAM(lParam);

                        if (is_result_resize)
                        {
                            SetCursor(LoadCursor(NULL, IDC_SIZENS));
                            int nListViewHeight = pnode->rect_qrtable.bottom - y - SPLIT_WIDTH / 2;
                            if (nListViewHeight >= SQLQUERYRESULT_LISTVIEW_HEIGHT_MIN)
                            {
                                eu_get_config()->result_list_height = nListViewHeight;
                            }
                            eu_window_resize(eu_hwndmain);
                        }
                        else if (pnode->rect_qrtable.left <= x && x <= pnode->rect_qrtable.right &&
                                 pnode->rect_qredit.bottom < y && y < pnode->rect_qrtable.top)
                        {
                            SetCursor(LoadCursor(NULL, IDC_SIZENS));
                            result_hover_resize = true;
                        }
                        else
                        {
                            if (result_hover_resize)
                            {
                                result_hover_resize = false;
                            }
                        }
                    }
                    break;
                default:
                    return DefWindowProc(hwnd, message, wParam, lParam);
            }
            break;
        }
        case WM_NOTIFY:
            lpnmhdr = (NMHDR *) lParam;
            lpnotify = (SCNotification *) lParam;
            lpnmtv = (NMTREEVIEW *) lParam;
            p_tips = (TOOLTIPTEXT *) lParam;
            if (lpnmhdr->hwndFrom == g_filetree)
            {
                SendMessage(g_filetree, WM_NOTIFY, wParam, lParam);
                break;
            }
            if (!(pnode = on_tabpage_focus_at()))
            {
                break;
            }
            switch (lpnmhdr->code)
            {
                case NM_CLICK:
                    if (g_statusbar && lpnmhdr->hwndFrom == g_statusbar)
                    {
                        POINT pt;
                        LPNMMOUSE lpnmm = (LPNMMOUSE)lParam;
                        GetCursorPos(&pt);
                        on_statusbar_pop_menu((int)lpnmm->dwItemSpec, &pt);
                    }
                    break;
                case NM_CUSTOMDRAW:
                {
                    if (on_dark_enable())
                    {
                        if (GetDlgItem(hwnd, IDC_TOOLBAR) == lpnmhdr->hwndFrom)
                        {
                            LPNMTBCUSTOMDRAW lptoolbar = (LPNMTBCUSTOMDRAW)lParam;
                            if (lptoolbar)
                            {
                                FillRect(lptoolbar->nmcd.hdc, &lptoolbar->nmcd.rc, (HBRUSH)on_dark_get_brush());
                            }
                        }
                        else if (GetDlgItem(hwnd, IDM_TABLE_BAR) == lpnmhdr->hwndFrom)
                        {
                            LPNMLVCUSTOMDRAW lpvcd = (LPNMLVCUSTOMDRAW)lParam;
                            if (lpvcd)
                            {           
                                if (lpvcd->nmcd.dwDrawStage == CDDS_PREPAINT)
                                {
                                    return CDRF_NOTIFYITEMDRAW;
                                }
                                if (lpvcd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT)
                                {
                                    return CDRF_NOTIFYSUBITEMDRAW;
                                }
                                else if (lpvcd->nmcd.dwDrawStage == (CDDS_ITEMPREPAINT|CDDS_SUBITEM))
                                {
                                    return CDRF_DODEFAULT;
                                }
                            }
                        }
                    }
                    break;
                }
                // 编辑器区输入时的消息响应, 其他消息见eu_scintill.c
                case HVN_GETDISPINFO:
                {
                    PNMHVDISPINFO dispinfo = (PNMHVDISPINFO)lParam;
                    if (!(pnode->phex && pnode->phex->pbase))
                    {
                        break;
                    }
                    if (dispinfo->item.mask & HVIF_ADDRESS) 
                    {
                        dispinfo->item.address = dispinfo->item.number_items;
                    }
                    else if (dispinfo->item.mask & HVIF_BYTE) 
                    {
                        uint8_t *base = (uint8_t *)(pnode->phex->pbase + dispinfo->item.number_items);
                        dispinfo->item.value = *base;
                        // Set state of the item.
                        if (dispinfo->item.number_items >= 0 && dispinfo->item.number_items <= 255)
                        {
                            dispinfo->item.state = HVIS_MODIFIED;
                        }
                    }
                    break;
                }
                case HVN_ITEMCHANGING:
                {
                    uint8_t *base = NULL;
                    PNMHEXVIEW phexview = (PNMHEXVIEW)lParam;
                    if (!(pnode->phex && pnode->phex->pbase))
                    {
                        break;
                    }
                    base = (uint8_t *)(pnode->phex->pbase + phexview->item.number_items);
                    *base = phexview->item.value;
                    if (pnode->phex->hex_point)
                    {
                        hexview_updata(pnode->phex->hex_point, phexview->item.number_items);
                    }
                    on_sci_point_left(pnode);
                    break;
                }
                case NM_SETFOCUS:
                {
                    DrawMenuBar(hwnd);
                    break;
                }
                case SCN_CHARADDED:
                    on_sci_character(on_tabpage_get_handle(lpnotify->nmhdr.hwndFrom), lpnotify);
                    break;
                case SCN_MODIFIED:
                    if (lpnotify->modificationType & SC_PERFORMED_UNDO)
                    {
                        if (lpnotify->text)
                        {
                            if (strcmp(lpnotify->text, eols_undo_str) == 0)
                            {
                                if (!_InterlockedCompareExchange(&undo_off, 1, 0))
                                {
                                    on_edit_undo_eol(pnode);
                                }
                            }
                            else if ((strlen(lpnotify->text) <= 2) && (lpnotify->text[0] == 0x0d || lpnotify->text[0] == 0x0a))
                            {
                                if (!eu_sci_call(pnode,SCI_CANUNDO, 0, 0))
                                {
                                    eu_sci_call(pnode, SCI_EMPTYUNDOBUFFER, 0, 0);
                                }
                            }
                            else if (strcmp(lpnotify->text, iconv_undo_str) == 0)
                            {
                                if (!_InterlockedCompareExchange(&undo_off, 1, 0))
                                {
                                    on_edit_undo_iconv(pnode);
                                }
                                if (!eu_sci_call(pnode,SCI_CANUNDO, 0, 0))
                                {
                                    eu_sci_call(pnode, SCI_EMPTYUNDOBUFFER, 0, 0);
                                }
                            }
                        }
                    }
                    break;
                case TCN_SELCHANGE:
                    on_tabpage_changing();
                    break;
                case SCN_SAVEPOINTREACHED:
                    on_sci_point_reached(on_tabpage_get_handle(lpnotify->nmhdr.hwndFrom));
                    break;
                case SCN_SAVEPOINTLEFT:
                    on_sci_point_left(on_tabpage_get_handle(lpnotify->nmhdr.hwndFrom));
                    break;
                case SCN_MARGINCLICK:
                {
                    sptr_t lineno = -1;
                    if ((pnode = on_tabpage_focus_at()) == NULL)
                    {
                        break;
                    }
                    lineno = eu_sci_call(pnode, SCI_LINEFROMPOSITION, lpnotify->position, 0);
                    if (lpnotify->margin == 1)
                    {
                        on_search_toggle_mark(pnode, lineno);
                    }
                    else if (lpnotify->margin == 2)
                    {
                        eu_sci_call(pnode, SCI_TOGGLEFOLD, lineno, 0);
                    }
                    break;
                }
                case SCN_UPDATEUI:
                {
                    if ((lpnotify->updated))
                    {
                        if (!(pnode = on_tabpage_focus_at()))
                        {
                            break;
                        }
                        if (lpnotify->updated & SC_UPDATE_SELECTION)
                        {
                            if (eu_get_config()->m_light_str)
                            {
                                on_view_editor_selection(pnode);
                            }
                            menu_update_all(hwnd, pnode);
                        }
                        on_statusbar_update_filesize(pnode);
                    }
                    break;
                }
                case TTN_NEEDTEXT:
                {
                    if (p_tips->hdr.hwndFrom != TabCtrl_GetToolTips(g_tabpages))
                    {
                        break;
                    }
                    eu_tabpage *p = on_tabpage_get_ptr((int) (p_tips->hdr.idFrom));
                    if (p)
                    {   // 显示标签的快捷键提示
                        memset(p_tips->szText, 0, sizeof(p_tips->szText));
                        if ((int) (p_tips->hdr.idFrom) <= 8)
                        {
                            _sntprintf(p_tips->szText, _countof(p_tips->szText) - 1, _T("%.68s - (Alt+%d)"), p->pathfile, (int) (p_tips->hdr.idFrom) + 1);
                        }
                        else
                        {
                            _sntprintf(p_tips->szText, _countof(p_tips->szText) - 1, _T("%.68s"), p->pathfile);
                        }
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        case WM_COPYDATA:
        {
            size_t rel_len = 0;
            file_backup *pm = NULL;
            COPYDATASTRUCT *cpd = (COPYDATASTRUCT *) lParam;
            if (!cpd)
            {
                break;
            }
            pm = (file_backup *) (cpd->lpData);
            rel_len = _tcslen(pm->rel_path);
            if (_tcsncmp(pm->rel_path, _T("-reg"), 4) == 0)
            {
                on_reg_update_menu(hwnd);
            }
            else if (rel_len > 0 && pm->rel_path[rel_len - 1] == _T('\\'))
            {
                on_treebar_locate_path(pm->rel_path);
            }
            else
            {   // 文件可能被重定向
                on_file_redirect(hwnd, pm);
            }
            break;
        }
        case WM_ACTIVATE:
        {
            if (LOWORD(wParam) != WA_INACTIVE && (pnode = on_tabpage_focus_at()))
            {
                SetFocus(pnode->hwnd_sc);
            }
            break;
        }
        case WM_LBUTTONDOWN:
            if (filebar_hover_resize)
            {
                is_filebar_resize = true;
            }
            if (function_hover_resize)
            {
                is_function_resize = true;
            }
            if (view_hover_resize)
            {
                is_view_resize = true;
            }
            if (sql_hover_resize)
            {
                is_sql_resize = true;
            }
            if (result_hover_resize)
            {
                is_result_resize = true;
            }
            break;
        case WM_LBUTTONUP:
            eu_reset_drag_line();
            break;
        case WM_DROPFILES:
            if (wParam)
            {
                on_file_drop((HDROP) wParam);
            }
            break;
        case WM_MOVE:
        {
            HWND hwnd_clip = on_toolbar_clip_hwnd();
            if (hwnd_clip && IsWindow(hwnd_clip))
            {
                on_toolbar_setpos_clipdlg(hwnd_clip, hwnd);
            }
            break;
        }
        case WM_CLOSE:
            if (hwnd == eu_hwndmain)
            {
                on_file_edit_exit(hwnd);
            }
            break;
        case WM_BACKUP_OVER:
            if (hwnd == eu_hwndmain)
            {
                DestroyWindow(hwnd);
            }
            break;
        case WM_DESTROY:
            {   
                on_destory_window(hwnd);
                printf("main window WM_DESTROY\n");
                break;
            }
        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}

/*****************************************************************************
 * 注册主窗口类
 ****************************************************************************/
static ATOM
class_register(HINSTANCE instance)
{
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_BYTEALIGNWINDOW | CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = eu_main_proc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = instance;
    wcex.hIcon = LoadIcon(instance, MAKEINTRESOURCE(IDI_SKYLARK));
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = APP_CLASS;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
    return RegisterClassEx(&wcex);
}

static unsigned __stdcall
do_calss_drop(void* lp)
{
    if (on_reg_admin())
    {
        do_drop_fix();
    }
    return 0;
}

HWND
eu_create_main_window(HINSTANCE instance)
{
    HWND hwnd = NULL;
    CloseHandle((HANDLE) _beginthreadex(NULL, 0, do_calss_drop, NULL, 0, NULL));
    if (class_register(instance) && i18n_load_menu(IDC_SKYLARK))
    {
        INITCOMMONCONTROLSEX icex;
        uint32_t ac_flags = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
        icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icex.dwICC = ICC_TAB_CLASSES | ICC_COOL_CLASSES | ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES | ICC_USEREX_CLASSES;
        if (InitCommonControlsEx(&icex))
        {            
            LOAD_APP_RESSTR(IDS_APP_TITLE, app_title);
            eu_hwndmain = hwnd = CreateWindowEx(WS_EX_ACCEPTFILES, APP_CLASS, app_title, ac_flags, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, instance, NULL);
            HMENU menu = hwnd && eu_get_config()->m_menubar ? i18n_load_menu(IDC_SKYLARK) : NULL;
            menu ? SetMenu(hwnd, menu) : (void)0;
            on_theme_setup_font(hwnd);
        }
    }
    return hwnd;
}
