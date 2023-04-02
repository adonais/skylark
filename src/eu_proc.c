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

#define EU_TIMER_ID 1
#define EU_UPTIMES 600
#define MAYBE100MS 100
#define ERROR_CALLBACK_ABORT 0x3e80

typedef UINT (WINAPI* GetDpiForWindowPtr)(HWND hwnd);
typedef BOOL(WINAPI *AdjustWindowRectExForDpiPtr)(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle, UINT dpi);

static HWND g_hwndmain;                    // 主窗口句柄
static HBRUSH g_control_brush;             // 全局子控件画刷
static volatile long undo_off;             // 状态栏按钮撤销信号量

volatile long g_interval_count = 0;        // 启动自动更新的时间间隔

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
    return SKYLARK_OK;
}

static int CALLBACK
enum_skylark_proc(HWND hwnd, LPARAM lParam)
{
    if (IsWindowVisible(hwnd))
    {
        wchar_t m_class[FILESIZE] = {0};
        GetClassName(hwnd, m_class, FILESIZE - 1);
        if (_tcsnicmp(m_class, APP_CLASS, _tcslen(m_class)) == 0)
        {
            uint32_t m_pid = 0;
            GetWindowThreadProcessId(hwnd, &m_pid);
            HANDLE hprocess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, m_pid);
            if (hprocess)
            {
                wchar_t m_path[MAX_BUFFER] = {0};
                wchar_t m_buffer[MAX_BUFFER] = {0};
                uint32_t buffer_len = MAX_BUFFER;
                GetModuleFileName(NULL , m_path, buffer_len);
                QueryFullProcessImageName(hprocess, 0, m_buffer, &buffer_len);
                if (_tcsnicmp(m_buffer, m_path, _tcslen(m_buffer)) == 0)
                {
                    printf("we get other hwnd = %p\n", (void *)hwnd);
                    share_envent_set_hwnd(hwnd);
                    SetLastError(ERROR_CALLBACK_ABORT);
                    return 0;
                }
            }
        }
    }
    return 1;
}

static void
on_destory_window(HWND hwnd)
{
    // 保存主窗口位置
    util_save_placement(hwnd);
    // 销毁定时器
    KillTimer(hwnd, EU_TIMER_ID);
    // 等待搜索线程完成
    on_search_finish_wait();
    // 等待更新线程完成并响应
    if (on_update_thread_wait())
    {
        Sleep(200);
    }
    // 销毁控件画刷
    on_proc_destory_brush();
    // 清理主题画刷
    on_dark_delete_theme_brush();
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
    if (g_hwndmain == share_envent_get_hwnd())
    {
        if (!EnumWindows(enum_skylark_proc, 0) && GetLastError() == ERROR_CALLBACK_ABORT)
        {
            if (eu_get_config()->upgrade.flags == VERSION_UPDATE_COMPLETED)
            {
                SendMessage(share_envent_get_hwnd(), WM_UPCHECK_STATUS, VERSION_UPDATE_COMPLETED, 0);
            }
        }
    }
    // 文件关闭,销毁信号量
    on_file_finish_wait();
    eu_curl_global_cleanup();
    // 全局变量清零
    _InterlockedExchange(&undo_off, 0);
    _InterlockedExchange(&g_interval_count, 0);
    // 退出消息循环
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
on_proc_undo_off(void)
{
    _InterlockedExchange(&undo_off, 0);
}

void
on_proc_destory_brush(void)
{
    if (g_control_brush)
    {
        DeleteObject(g_control_brush);
        g_control_brush = NULL;
    }
}

HWND
eu_module_hwnd(void)
{
    return (g_hwndmain ? g_hwndmain : share_envent_get_hwnd());
}

HWND
eu_hwnd_self(void)
{
    return (g_hwndmain);
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
        if (fnGetDpiForWindow && (dpi = fnGetDpiForWindow(hwnd ? hwnd : g_hwndmain)) > 0)
        {
            return dpi;
        }
    }
    if (!dpi)
    {   // PMv1或Win7系统, 使用GetDeviceCaps获取dpi
        HDC screen = GetDC(hwnd ? hwnd : g_hwndmain);
        int x = GetDeviceCaps(screen,LOGPIXELSX);
        int y = GetDeviceCaps(screen,LOGPIXELSY);
        ReleaseDC(hwnd ? hwnd : g_hwndmain, screen);
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
        RECT rc = {0};
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

void
eu_create_fullscreen(HWND hwnd)
{
    on_view_setfullscreenimpl(hwnd);
}

static void
on_proc_move_sidebar(eu_tabpage *pnode)
{
    if (pnode->sym_show)
    {
        eu_setpos_window(g_splitter_symbar, HWND_TOP, pnode->rect_sc.right, pnode->rect_sym.top,
                         SPLIT_WIDTH, pnode->rect_sym.bottom - pnode->rect_sym.top, SWP_SHOWWINDOW);
        if (pnode->hwnd_symlist)
        {
            eu_setpos_window(pnode->hwnd_symlist, HWND_TOP, pnode->rect_sym.left, pnode->rect_sym.top,
                             pnode->rect_sym.right - pnode->rect_sym.left, pnode->rect_sym.bottom - pnode->rect_sym.top, SWP_SHOWWINDOW);

        }
        else if (pnode->hwnd_symtree)
        {
            eu_setpos_window(pnode->hwnd_symtree, HWND_TOP, pnode->rect_sym.left, pnode->rect_sym.top,
                             pnode->rect_sym.right - pnode->rect_sym.left, pnode->rect_sym.bottom - pnode->rect_sym.top, SWP_SHOWWINDOW);
        }
        if (document_map_initialized && hwnd_document_map)
        {
            eu_setpos_window(hwnd_document_map, HWND_BOTTOM, 0, 0, 0, 0, SWP_HIDEWINDOW);
        }
    }
    else if (pnode->map_show && document_map_initialized)
    {
        eu_tabpage *pedit = NULL;
        eu_setpos_window(g_splitter_symbar, HWND_TOP, pnode->rect_sc.right, pnode->rect_map.top,
                         SPLIT_WIDTH, pnode->rect_map.bottom - pnode->rect_map.top, SWP_SHOWWINDOW);
        if (hwnd_document_map)
        {
            eu_setpos_window(hwnd_document_map, HWND_TOP, pnode->rect_map.left, pnode->rect_map.top,
                             pnode->rect_map.right - pnode->rect_map.left, pnode->rect_map.bottom - pnode->rect_map.top, SWP_SHOWWINDOW);
        }
        if ((pedit = on_map_edit()) && pedit->hwnd_sc)
        {
            on_map_reload(pedit);
        }
    }
}

/*****************************************************************************
 * 主窗口缩放处理函数
 * 一次性处理所有窗口, 不在每个窗口处理WM_SIZE消息了
******************************************************************************/
static void
on_proc_msg_size(HWND hwnd, eu_tabpage *ptab)
{
    RECT rc = {0};
    RECT rect_treebar = {0};
    RECT rect_tabbar = {0};
    eu_tabpage *pnode = ptab ? ptab : on_tabpage_focus_at();
    if (pnode)
    {
        int number = 10;
        on_toolbar_adjust_box();
        on_statusbar_adjust_box();
        on_tabpage_adjust_box(&rect_tabbar);
        on_treebar_adjust_box(&rect_treebar);
        on_tabpage_adjust_window(pnode);
        GetWindowRect(hwnd ? hwnd : eu_module_hwnd(), &rc);
        if (ptab)
        {
            number -= 5;
        }
        HDWP hdwp = BeginDeferWindowPos(number);
        if (!ptab)
        {
            if (eu_get_config()->m_toolbar != IDB_SIZE_0)
            {
                DeferWindowPos(hdwp, on_toolbar_hwnd(), HWND_TOP, 0, 0, rc.right - rc.left, on_toolbar_height(), SWP_SHOWWINDOW);
            }
            else
            {
                DeferWindowPos(hdwp, on_toolbar_hwnd(), 0, 0, 0, 0, 0, SWP_HIDEWINDOW);
            }
            if (eu_get_config()->m_ftree_show)
            {
                RECT rect_filetree = {0};
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
                DeferWindowPos(hdwp,
                               g_splitter_treebar,
                               HWND_TOP,
                               rect_treebar.right,
                               pnode->rect_sc.top,
                               SPLIT_WIDTH,
                               rect_treebar.bottom - pnode->rect_sc.top,
                               SWP_SHOWWINDOW);
            }
            else
            {
                DeferWindowPos(hdwp, g_treebar, 0, 0, 0, 0, 0, SWP_HIDEWINDOW);
                DeferWindowPos(hdwp, g_filetree, 0, 0, 0, 0, 0, SWP_HIDEWINDOW);
                DeferWindowPos(hdwp, g_splitter_treebar, 0, 0, 0, 0, 0, SWP_HIDEWINDOW);
            }
            DeferWindowPos(hdwp, g_tabpages, HWND_TOP, rect_tabbar.left, rect_tabbar.top,
                           rect_tabbar.right - rect_tabbar.left, rect_tabbar.bottom - rect_tabbar.top, SWP_SHOWWINDOW);
        }
        if (pnode->hwnd_sc)
        {
            DeferWindowPos(hdwp, pnode->hwnd_sc, HWND_TOP, pnode->rect_sc.left, pnode->rect_sc.top,
                           pnode->rect_sc.right - pnode->rect_sc.left, pnode->rect_sc.bottom - pnode->rect_sc.top, SWP_SHOWWINDOW);
            // 先隐藏右边侧边栏
            DeferWindowPos(hdwp, g_splitter_symbar, HWND_BOTTOM, 0, 0, 0, 0, SWP_HIDEWINDOW);
            if (pnode->hwnd_symlist)
            {
                DeferWindowPos(hdwp, pnode->hwnd_symlist, HWND_BOTTOM, 0, 0, 0, 0, SWP_HIDEWINDOW);
            }
            else if (pnode->hwnd_symtree)
            {
                DeferWindowPos(hdwp, pnode->hwnd_symtree, HWND_BOTTOM, 0, 0, 0, 0, SWP_HIDEWINDOW);
            }
            if (document_map_initialized && hwnd_document_map)
            {
                DeferWindowPos(hdwp, hwnd_document_map, 0, 0, 0, 0, 0, SWP_HIDEWINDOW);
            }
        }
        if (eu_get_config()->m_statusbar)
        {
            if (g_statusbar)
            {
                DeferWindowPos(hdwp, g_statusbar, HWND_TOP, rc.left, rc.bottom - on_statusbar_height(),
                               rc.right - rc.left, on_statusbar_height(), SWP_SHOWWINDOW);
                SendMessage(g_statusbar, WM_STATUS_REFRESH, 0, 0);
                on_statusbar_btn_rw(pnode, true);
            }
        }
        else if (g_statusbar)
        {
            DeferWindowPos(hdwp, g_statusbar, HWND_BOTTOM, 0, 0, 0, 0, SWP_HIDEWINDOW);
        }
        if (true)
        {
            EndDeferWindowPos(hdwp);
            if (!ptab)
            {   // 重新计算标签栏大小
                on_tabpage_adjust_box(&rect_tabbar);
                on_tabpage_adjust_window(pnode);
                eu_setpos_window(g_tabpages, HWND_TOP, rect_tabbar.left, rect_tabbar.top,
                                 rect_tabbar.right - rect_tabbar.left, rect_tabbar.bottom - rect_tabbar.top, SWP_SHOWWINDOW);
                eu_setpos_window(pnode->hwnd_sc, HWND_TOP, pnode->rect_sc.left, pnode->rect_sc.top,
                                 pnode->rect_sc.right - pnode->rect_sc.left, pnode->rect_sc.bottom - pnode->rect_sc.top, SWP_SHOWWINDOW);
                UpdateWindowEx(g_tabpages);
                UpdateWindowEx(pnode->hwnd_sc);
            }
            // 重新加上位置后显示
            on_proc_move_sidebar(pnode);
        }
        for (int index = 0, count = TabCtrl_GetItemCount(g_tabpages); index < count; ++index)
        {
            eu_tabpage *p = on_tabpage_get_ptr(index);
            if (p && p != pnode && !p->plugin)
            {
                if (RESULT_SHOW(p))
                {
                    ShowWindow(p->presult->hwnd_sc, SW_HIDE);
                    if (p->hwnd_qrtable)
                    {
                        ShowWindow(p->hwnd_qrtable, SW_HIDE);
                    }
                }
                if (p->hex_mode && p->hwnd_sc)
                {
                    ShowWindow(p->hwnd_sc, SW_HIDE);
                }
            }
        }
        if (RESULT_SHOW(pnode) && eu_result_hwnd())
        {
            RECT r = {0, 0, pnode->rect_sc.right - pnode->rect_sc.left, SPLIT_WIDTH};
            eu_setpos_window(eu_result_hwnd(), HWND_TOP, pnode->rect_result.left, pnode->rect_result.top,
                             pnode->rect_result.right - pnode->rect_result.left, pnode->rect_result.bottom - pnode->rect_result.top, SWP_SHOWWINDOW);
            eu_setpos_window(g_splitter_editbar, HWND_TOP, pnode->rect_sc.left, pnode->rect_sc.bottom,
                             pnode->rect_sc.right - pnode->rect_sc.left, SPLIT_WIDTH, SWP_SHOWWINDOW);
            on_result_move_sci(pnode, pnode->rect_result.right - pnode->rect_result.left, pnode->rect_result.bottom - pnode->rect_result.top);
            on_result_reload(pnode->presult);
            if (pnode->hwnd_qrtable)
            {
                eu_setpos_window(pnode->hwnd_qrtable, HWND_TOP, pnode->rect_qrtable.left, pnode->rect_qrtable.top,
                                 pnode->rect_qrtable.right - pnode->rect_qrtable.left, pnode->rect_qrtable.bottom - pnode->rect_qrtable.top, SWP_SHOWWINDOW);
                eu_setpos_window(g_splitter_tablebar, HWND_TOP, pnode->rect_sc.left, pnode->rect_result.bottom,
                                 pnode->rect_sc.right - pnode->rect_sc.left, SPLIT_WIDTH, SWP_SHOWWINDOW);
                UpdateWindowEx(pnode->hwnd_qrtable);
                InvalidateRect(g_splitter_tablebar, &r, false);
                UpdateWindow(g_splitter_tablebar);
            }
        }
        PostMessage(hwnd, WM_ACTIVATE, MAKEWPARAM(WA_CLICKACTIVE, 0), 0);
    }
}

void
on_proc_resize(HWND hwnd)
{   // 当在线程需要刷新界面时, 使用消息让主线程刷新
    SendMessage(hwnd ? hwnd : eu_module_hwnd(), WM_SIZE, 0, 0);
}

void
eu_window_resize(HWND hwnd)
{
    on_proc_msg_size(hwnd ? hwnd : eu_module_hwnd(), NULL);
}

static void
on_proc_tab_click(HWND hwnd, eu_tabpage *pnode)
{
    on_proc_msg_size(hwnd, pnode);
    if (pnode && pnode->nc_pos >= 0 && eu_get_config() && eu_get_config()->scroll_to_cursor)
    {
        if (pnode->hex_mode)
        {
            eu_sci_call(pnode, SCI_GOTOPOS, pnode->nc_pos, 0);
        }
        else
        {
            eu_sci_call(pnode, SCI_SCROLLCARET, 0, 0);
        }
    }
}

int
eu_before_proc(MSG *p_msg)
{
    eu_tabpage *pnode = NULL;
    if (p_msg->message == WM_SYSKEYDOWN && p_msg->wParam == VK_MENU && !(p_msg->lParam & 0xff00))
    {
        return 1;
    }
    if (p_msg->message == WM_SYSKEYDOWN && 0x31 <= p_msg->wParam && p_msg->wParam <= 0x39 && (p_msg->lParam & (1 << 29)))
    {
        on_tabpage_active_one((int) (p_msg->wParam) - 0x31);
        return 1;
    }
    if((pnode = on_tabpage_focus_at()) && pnode && pnode->doc_ptr && !pnode->hex_mode && p_msg->message == WM_KEYDOWN && p_msg->hwnd == pnode->hwnd_sc)
    {
        bool main_up = KEY_UP(VK_CONTROL) && KEY_UP(VK_MENU) && KEY_UP(VK_INSERT);
        bool main_down = KEY_DOWN(VK_CONTROL) && KEY_DOWN(VK_MENU) && KEY_DOWN(VK_INSERT) && KEY_DOWN(VK_SHIFT);
        if (p_msg->wParam == VK_TAB && main_up && eu_get_config() && eu_get_config()->eu_complete.snippet)
        {
            if (main_up)
            {
                eu_sci_call(pnode, SCI_CANCEL, 0, 0);
                if (KEY_DOWN(VK_SHIFT))
                {
                    return on_complete_snippet_back(pnode);
                }
                else if (on_complete_snippet(pnode))
                {
                    return 1;
                }
            }
        }
        else if (main_down && pnode->doc_ptr->doc_type == DOCTYPE_CPP)
        {
            on_sci_insert_egg(pnode);
            return 1;
        }
    }
    return 0;
}

static int
on_proc_save_remote(eu_tabpage *pnode)
{
    int ret = EUE_CURL_NETWORK_ERR;
    if (pnode && pnode->plugin)
    {   // 传送修改到远程服务器
        pf_stream pstream = NULL;
        wchar_t msg[PERROR_LEN+1] = {0};
        if (!np_plugins_getvalue(&pnode->plugin->funcs, &pnode->plugin->npp, NV_STREAM, (void **)&pstream) && pstream)
        {
            pnode->write_buffer = (uint8_t *)pstream->base;
            pnode->bytes_remaining = pstream->size;
            if ((ret = on_file_stream_upload(pnode, msg)) != SKYLARK_OK)
            {
                print_err_msg(IDC_MSG_ATTACH_ERRORS, msg);
                pnode->st_mtime = 0;
            }
            else
            {
                on_file_update_time(pnode, time(NULL));
            }
            pstream->close(pstream);
            eu_safe_free(pstream);
            pnode->write_buffer = NULL;
            pnode->bytes_remaining = 0;
        }
    }
    return ret;
}

static void
on_proc_save_status(WPARAM flags, npn_nmhdr *lpnmhdr)
{
    eu_tabpage *pnode = NULL;
    if (!lpnmhdr)
    {
        return;
    }
    if (lpnmhdr->pnode)
    {
        pnode = (eu_tabpage *)lpnmhdr->pnode;
    }
    else
    {
        pnode = on_tabpage_focus_at();
    }
    if (!pnode)
    {
        return;
    }
    if (lpnmhdr->modified)
    {
        if (!pnode->be_modify)
        {
            pnode->be_modify = true;
            on_toolbar_update_button();
            InvalidateRect(g_tabpages, NULL, false);
            printf("skylark: doc has been modified\n");
        }
    }
    if (flags && !lpnmhdr->modified && pnode->plugin)
    {
        bool remote = false;
        bool backup = false;
        wchar_t *full_path = NULL;
        pnode->be_modify = false;
        printf("skylark: doc has been saved\n");
        if (!np_plugins_getvalue(&pnode->plugin->funcs, &pnode->plugin->npp, NV_TABTITLE, (void **)&full_path) && STR_NOT_NUL(full_path))
        {
            printf("full_path = %ls, pnode->pathfile = %ls, pnode->bakpath = %ls\n", full_path, pnode->pathfile, pnode->bakpath);
            if (url_has_remote(pnode->pathfile))
            {
                remote = true;
            }
            else if (pnode->bakpath[0] && (wcsicmp(full_path, pnode->bakpath) == 0))
            {   // 获取流保存在本地的临时文件名
                backup = true;
            }
            if (flags == OPERATE_SAVE)
            {
                if (remote)
                {
                    on_proc_save_remote(pnode);
                }
                else if (backup)
                {
                    np_plugins_savefileas(&pnode->plugin->funcs, &pnode->plugin->npp, pnode->pathfile);
                    util_delete_file(pnode->bakpath);
                    pnode->bakpath[0] = 0;
                }
                else
                {
                    np_plugins_savefile(&pnode->plugin->funcs, &pnode->plugin->npp);
                }
                on_file_update_time(pnode, 0);
            }
            else if (flags == OPERATE_SAVEAS)
            {
                if (remote)
                {
                    pnode->fs_server.networkaddr[0] = 0;
                }
                else if (backup)
                {
                    util_delete_file(pnode->bakpath);
                    pnode->bakpath[0] = 0;
                }
                on_sql_delete_backup_row(pnode);
                _tcsncpy(pnode->pathfile, full_path, MAX_BUFFER);
                _wsplitpath(full_path, NULL, NULL, pnode->filename, pnode->extname);
                if (wcslen(pnode->extname) > 0)
                {
                    wcsncat(pnode->filename, pnode->extname, MAX_PATH-1);
                }
                on_file_update_time(pnode, 0);
                util_set_title(pnode);
                np_plugins_setvalue(&pnode->plugin->funcs, &pnode->plugin->npp, NV_PATH_CHANGE, pnode->pathfile);
            }
            InvalidateRect(g_tabpages, NULL, false);
            eu_safe_free(full_path);
        }
    }
}

LRESULT CALLBACK
eu_main_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    NMHDR *lpnmhdr = NULL;
    ptr_notify lpnotify = NULL;
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
            if (eu_get_config()->m_menubar)
            {
                SetMenu(hwnd, i18n_load_menu(IDC_SKYLARK));
            }
            if (eu_get_config()->m_fullscreen)
            {
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
                    eu_dark_theme_release(false);
                    on_proc_msg_size(hwnd, NULL);
                }
            }
            else
            {
                HDC hdc = GetWindowDC(hwnd);
                RECT r = {0};
                get_menu_border_rect(hwnd, &r);
                FillRect(hdc, &r, (HBRUSH)on_dark_get_brush());
                ReleaseDC(hwnd, hdc);
            }
            return result;
        }
        case WM_NCACTIVATE:
            return 1;
        case WM_SIZE:
            if (wParam != SIZE_MINIMIZED)
            {
                on_proc_msg_size(hwnd, NULL);
            }
            break;
        case WM_RBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
        {
            if (eu_get_config() && eu_get_config()->m_new_way > 0)
            {
                POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                if (g_tabpages)
                {
                    RECT rect = {0};
                    MapWindowPoints(hwnd, g_tabpages, &pt, 1);
                    GetClientRect(g_tabpages, &rect);
                    if (PtInRect(&rect, pt) && (int)SendMessage(g_tabpages, WM_NCHITTEST, 0, lParam) == HTTRANSPARENT)
                    {
                        PostMessage(g_tabpages, WM_TAB_NCCLICK, wParam, 0);
                    }
                }
            }
            break;
        }
        case WM_TAB_CLICK:
            on_proc_tab_click(hwnd, (void *)wParam);
            return 1;
        case WM_UPCHECK_STATUS:
        {
            if ((intptr_t)wParam < 0)
            {
                eu_get_config()->upgrade.flags = VERSION_LATEST;
            }
            else
            {
                eu_get_config()->upgrade.flags = (int)wParam;
            }
            return 1;
        }
        case WM_UPCHECK_LAST:
        {
            if ((intptr_t)wParam < 0)
            {
                eu_get_config()->upgrade.last_check = (uint64_t)time(NULL);
            }
            else
            {
                eu_get_config()->upgrade.last_check = (uint64_t)wParam;
            }
            return 1;
        }
        case WM_TIMER:
            if (KEY_DOWN(VK_ESCAPE))
            {
                if (on_qrgen_hwnd())
                {
                    EndDialog(on_qrgen_hwnd(), 0);
                }
                else if (eu_get_config()->m_fullscreen)
                {
                    on_view_full_sreen(eu_hwnd_self());
                }
            }
            if (g_hwndmain == GetForegroundWindow())
            {
                ONCE_RUN(on_changes_window(hwnd));
            }
            if (eu_get_config()->upgrade.enable)
            {
                if (g_interval_count == EU_UPTIMES)
                {   // 启动更新进程
                    _InterlockedIncrement(&g_interval_count);
                    on_update_check(UPCHECK_INDENT_MAIN);
                    printf("g_interval_count = %ld\n", g_interval_count);
                }
                else if (g_interval_count < EU_UPTIMES)
                {
                    _InterlockedIncrement(&g_interval_count);
                }
            }
            break;
        case WM_INITMENUPOPUP:
            // 展开时, 显示菜单状态
            menu_update_item((HMENU)wParam, false);
            break;
        case WM_UNINITMENUPOPUP:
            // 合拢时, 启用所有菜单项
            menu_update_item((HMENU)wParam, true);
            break;
        case WM_SKYLARK_DESC:
            return WM_SKYLARK_DESC;
        case WM_ABOUT_RE:
            on_search_regxp_error();
            break;
        case WM_DPICHANGED:
        {
            on_theme_setup_font(hwnd);
            on_tabpage_foreach(hexview_update_theme);
            on_toolbar_refresh(hwnd);
            on_statusbar_init(hwnd);
            SendMessage(g_treebar, WM_DPICHANGED, 0, 0);
            break;
        }
        case WM_CTLCOLORLISTBOX:
        {   // 为list控件创建画刷,用来绘制背景色
            HDC hdc = (HDC)wParam;
            if (!(pnode = on_tabpage_focus_at()))
            {
                break;
            }
            if (!g_control_brush)
            {
                if (NULL == (g_control_brush = CreateSolidBrush(eu_get_theme()->item.text.bgcolor)))
                    break;
            }
            if (pnode->hwnd_symlist == (HWND)lParam)
            {
                SetTextColor(hdc, eu_get_theme()->item.symbolic.color & 0xFFFFFF);
            }
            else
            {
                SetTextColor(hdc, eu_get_theme()->item.text.color);
            }
            SetBkColor(hdc, eu_get_theme()->item.text.bgcolor);
            SetBkMode(hdc, TRANSPARENT);
            return (LRESULT)g_control_brush;
        }
        case WM_CTLCOLOREDIT:
        {   // 为edit控件创建画刷,用来绘制背景色
            HDC hdc = (HDC)wParam;
            if (!g_control_brush)
            {
                if (NULL == (g_control_brush = CreateSolidBrush(eu_get_theme()->item.text.bgcolor)))
                    break;
            }
            SetTextColor(hdc, eu_get_theme()->item.text.color);
            SetBkColor(hdc, eu_get_theme()->item.text.bgcolor);
            SetBkMode(hdc, TRANSPARENT);
            return (LRESULT)g_control_brush;
        }
        case WM_DRAWITEM:
        {
            switch (((LPDRAWITEMSTRUCT)lParam)->CtlID)
            {
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
                HWND snippet = NULL;
                on_dark_allow_window(hwnd, on_dark_enable());
                on_dark_refresh_titlebar(hwnd);
                on_dark_tips_theme(g_tabpages, TCM_GETTOOLTIPS);
                on_tabpage_foreach(on_tabpage_theme_changed);
                if (g_statusbar)
                {
                    on_statusbar_init(hwnd);
                }
                on_toolbar_refresh(hwnd);
                on_dark_set_theme(g_treebar, L"Explorer", NULL);
                if (g_filetree)
                {
                    SendMessage(g_filetree, WM_THEMECHANGED, 0, 0);
                }
                if ((snippet = eu_snippet_hwnd()) && IsWindowVisible(snippet))
                {
                    on_dark_set_theme(snippet, L"", L"");
                    on_dark_set_theme(snippet, L"Explorer", NULL);
                }
            }
            break;
        case WM_SYSCOMMAND:
        {
            if (wParam == SC_RESTORE)
            {
                const LRESULT rv = DefWindowProc(hwnd, message, wParam, lParam);
                if ((pnode = on_tabpage_focus_at()) && !pnode->hex_mode && pnode->nc_pos >= 0)
                {
                    eu_sci_call(pnode, SCI_SCROLLCARET, 0, 0);
                }
                return rv;
            }
            return DefWindowProc(hwnd, message, wParam, lParam);
        }
        case WM_COMMAND:
        {
            int wm_id = LOWORD(wParam);
            pnode = on_tabpage_focus_at();
            if (IDM_HISTORY_BASE <= wm_id && wm_id <= IDM_HISTORY_BASE + PATH_MAX_RECENTLY - 1)
            {
                int len = 0;
                HMENU file_menu = NULL;
                HMENU hpop = NULL;
                HMENU root_menu = GetMenu(hwnd);
                file_backup bak = {0};
                file_menu = root_menu ? GetSubMenu(root_menu, 0) : NULL;
                hpop = file_menu ? GetSubMenu(file_menu, 2) : NULL;
                len = hpop ? GetMenuString(hpop, wm_id, bak.rel_path, MAX_BUFFER, MF_BYCOMMAND) : 0;
                if (len > 0)
                {
                    if (_tcsrchr(bak.rel_path, _T('&')))
                    {
                        eu_wstr_replace(bak.rel_path, MAX_BUFFER, _T("&&"), _T("&"));
                    }
                    if (!url_has_remote(bak.rel_path))
                    {
                        if (_tcsrchr(bak.rel_path, _T('/')))
                        {
                            eu_wstr_replace(bak.rel_path, MAX_BUFFER, _T("/"), _T("\\"));
                        }
                        on_file_only_open(&bak, true);
                    }
                    else
                    {
                        on_file_open_remote(NULL, &bak, true);
                    }
                }
                break;
            }
            if (IDM_STYLETHEME_BASE <= wm_id && wm_id <= IDM_STYLETHEME_BASE + VIEW_STYLETHEME_MAXCOUNT - 1)
            {
                on_view_switch_theme(g_hwndmain, wm_id);
                break;
            }
            if (IDM_LOCALES_BASE <= wm_id && wm_id <= IDM_LOCALES_BASE + MAX_MULTI_LANG - 1)
            {
                i18n_switch_locale(g_hwndmain, wm_id);
                break;
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
                case IDM_TABPAGE_SAVE:
                    on_tabpage_do_file(on_tabpage_save_files);
                    break;
                case IDM_FILE_SAVEAS:
                    on_file_save_as(pnode);
                    break;
                case IDM_FILE_SAVEALL:
                    on_file_all_save();
                    break;
                case IDM_FILE_CLOSE:
                    on_tabpage_do_file(on_tabpage_close_tabs);
                    break;
                case IDM_FILE_CLOSEALL:
                    on_file_all_close();
                    break;
                case IDM_FILE_CLOSEALL_EXCLUDE:
                    on_file_exclude_close(pnode);
                    break;
                case IDM_FILE_RESTORE_RECENT:
                    on_file_restore_recent();
                    break;
                case IDM_FILE_RELOAD_CURRENT:
                    on_file_reload_current(pnode);
                    break;
                case IDM_FILE_WRITE_COPY:
                    on_file_backup_menu();
                    break;
                case IDM_FILE_SESSION:
                    on_file_session_menu();
                    break;
                case IDM_FILE_EXIT_WHEN_LAST_TAB:
                    on_file_close_last_tab();
                    break;
                case IDM_FILE_RESTART_ADMIN:
                    on_file_edit_restart(hwnd, true);
                    break;
                case IDM_FILE_PAGESETUP:
                    on_print_setup(g_hwndmain);
                    break;
                case IDM_FILE_PRINT:
                    on_print_file(pnode);
                    break;
                case IDM_EDIT_CLIP:
                {
                    HWND hcip = on_toolbar_clip_hwnd();
                    if (!hcip && (hcip = on_toolbar_create_clipbox(hwnd)))
                    {
                        on_toolbar_setpos_clipdlg(hcip, hwnd);
                        ShowWindow(hcip, SW_SHOW);
                    }
                    else if (!IsWindowVisible(hcip))
                    {
                        on_toolbar_setpos_clipdlg(hcip, hwnd);
                        ShowWindow(hcip, SW_SHOW);
                    }
                    else
                    {
                        ShowWindow(hcip, SW_HIDE);
                    }
                    break;
                }
                case IDM_SCRIPT_EXEC:
                {
                    on_toolbar_execute_script();
                    break;
                }
                case IDM_CMD_TAB:
                {
                    on_toolbar_cmd_start(pnode);
                    break;
                }
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
                case IDM_EDIT_COPYLINE:
                    on_edit_copy_line(pnode);
                    break;
                case IDM_EDIT_COPY_FILENAME:
                    if (pnode && *pnode->filename)
                    {
                        on_edit_push_clipboard(pnode->filename);
                    }
                    break;
                case IDM_EDIT_COPY_PATHNAME:
                    if (pnode && *pnode->pathname)
                    {
                        TCHAR unix_path[MAX_BUFFER] = {0};
                        if (util_under_wine() && util_get_unix_file_name(pnode->pathname, unix_path, MAX_BUFFER))
                        {
                            on_edit_push_clipboard(unix_path);
                            break;
                        }
                        on_edit_push_clipboard(pnode->pathname);
                    }
                    break;
                case IDM_EDIT_COPY_PATHFILENAME:
                    if (pnode && *pnode->pathfile)
                    {
                        TCHAR unix_path[MAX_BUFFER] = {0};
                        if (util_under_wine() && util_get_unix_file_name(pnode->pathfile, unix_path, MAX_BUFFER))
                        {
                            on_edit_push_clipboard(unix_path);
                            break;
                        }
                        on_edit_push_clipboard(pnode->pathfile);
                    }
                    break;
                case IDM_EDIT_COPY_INCREMENTAL:
                    on_edit_incremental_clipborad(pnode);
                    break;
                case IDM_EDIT_COPY_RTF:
                    on_edit_rtf_clipborad(hwnd, pnode);
                    break;
                case IDM_EDIT_SWAP_CLIPBOARD:
                    on_edit_swap_clipborad(pnode);
                    break;
                case IDM_EDIT_CLEAR_CLIPBOARD:
                    on_edit_clear_clipborad(hwnd);
                    break;
                case IDM_EDIT_OTHER_EDITOR:
                    {
                        on_tabpage_do_file(on_tabpage_push_editor);
                    }
                    break;
                case IDM_EDIT_OTHER_BCOMPARE:
                    {
                        on_edit_push_compare();
                    }
                    break;
                case IDM_FILE_WORKSPACE:
                    if (pnode && *pnode->pathfile && !pnode->is_blank)
                    {
                        on_treebar_locate_path(pnode->pathfile);
                    }
                    break;
                case IDM_FILE_EXPLORER:
                    if (pnode && *pnode->pathname && !pnode->is_blank && !url_has_remote(pnode->pathfile))
                    {
                        util_explorer_open(pnode);
                    }
                    break;
                case IDM_EDIT_DELETELINE:
                    on_edit_delete_line(pnode);
                    break;
                case IDM_EDIT_REMOVE_DUP_LINES:
                    on_edit_delete_dups(pnode);
                    break;
                case IDM_DELETE_SPACE_LINEHEAD:
                    on_edit_delete_line_header_white(pnode);
                    break;
                case IDM_DELETE_SPACE_LINETAIL:
                    on_edit_delete_line_tail_white(pnode);
                    break;
                case IDM_DELETE_ALL_SPACE_LINEHEAD:
                    on_edit_delete_line_header_all(pnode);
                    break;
                case IDM_DELETE_ALL_SPACE_LINETAIL:
                    on_edit_delete_line_tail_all(pnode);
                    break;
                case IDM_DELETE_ALL_SPACE_LINE:
                    on_edit_delete_all_empty_lines(pnode);
                    break;
                case IDM_EDIT_LINETRANSPOSE:
                    on_edit_line_transpose(pnode);
                    break;
                case IDM_EDIT_JOINLINE:
                    on_edit_join_line(pnode);
                    break;
                case IDM_EDIT_MOVE_LINEUP:
                    on_edit_line_up(pnode);
                    break;
                case IDM_EDIT_MOVE_LINEDOWN:
                    on_edit_line_down(pnode);
                    break;
                case IDM_EDIT_LINECOMMENT:
                    on_edit_comment_line(pnode);
                    break;
                case IDM_EDIT_STREAMCOMMENT:
                    on_edit_comment_stream(pnode);
                    break;
                case IDM_EDIT_ASCENDING_SORT:
                case IDM_EDIT_ASCENDING_SORT_IGNORECASE:
                case IDM_EDIT_DESCENDING_SORT:
                case IDM_EDIT_DESCENDING_SORT_IGNORECASE:
                    on_edit_sorting(pnode, wm_id);
                    break;
                case IDM_EDIT_LOWERCASE:
                    on_edit_lower(pnode);
                    break;
                case IDM_EDIT_UPPERCASE:
                    on_edit_upper(pnode);
                    break;
                case IDM_EDIT_WORD_UPPERCASE:
                    on_edit_sentence_upper(pnode, false);
                    break;
                case IDM_EDIT_SENTENCE_UPPERCASE:
                    on_edit_sentence_upper(pnode, true);
                    break;
                case IDM_EDIT_TAB_SPACE:
                    on_search_repalce_event(pnode, TAB_SPACE);
                    break;
                case IDM_EDIT_SPACE_TAB:
                    on_search_repalce_event(pnode, SPACE_TAB);
                    break;
                case IDM_EDIT_SLASH_BACKSLASH:
                    on_edit_convert_slash(pnode, true);
                    break;
                case IDM_EDIT_BACKSLASH_SLASH:
                    on_edit_convert_slash(pnode, false);
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
                case IDM_FORMAT_FULL_HALF:
                    on_search_repalce_event(pnode, FULL_HALF);
                    break;
                case IDM_FORMAT_HALF_FULL:
                    on_search_repalce_event(pnode, HALF_FULL);
                    break;
                case IDM_EDIT_AUTO_CLOSECHAR:
                    eu_get_config()->eu_brace.autoc ^= true;
                    break;
                case IDM_EDIT_AUTO_INDENTATION:
                    on_view_identation();
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
                    on_search_create_box();
                    on_search_find_thread(pnode);
                    break;
                case IDM_SEARCH_FINDPREV:
                    on_search_create_box();
                    on_search_find_pre(pnode);
                    break;
                case IDM_SEARCH_FINDNEXT:
                    on_search_create_box();
                    on_search_find_next(pnode);
                    break;
                case IDM_SEARCH_REPLACE:
                    on_search_create_box();
                    on_search_replace_thread(pnode);
                    break;
                case IDM_SEARCH_FILES:
                    on_search_create_box();
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
                case IDM_SEARCH_SELECT_HEAD:
                case IDM_SEARCH_SELECT_END:
                    on_search_select_se(pnode, wm_id);
                    break;
                case IDM_SEARCH_ADDSELECT_LEFT_WORD:
                    on_search_select_left_word(pnode);
                    break;
                case IDM_SEARCH_ADDSELECT_RIGHT_WORD:
                    on_search_select_right_word(pnode);
                    break;
                case IDM_SEARCH_ADDSELECT_LEFT_WORDGROUP:
                    on_search_select_left_group(pnode);
                    break;
                case IDM_SEARCH_ADDSELECT_RIGHT_WORDGROUP:
                    on_search_select_right_group(pnode);
                    break;
                case IDM_SEARCH_SELECTTOP_FIRSTLINE:
                    on_search_cumulative_previous_block(pnode);
                    break;
                case IDM_SEARCH_SELECTBOTTOM_FIRSTLINE:
                    on_search_cumulative_next_block(pnode);
                    break;
                case IDM_SEARCH_MOVE_LEFT_WORD:
                    on_search_move_to_lgroup(pnode);
                    break;
                case IDM_SEARCH_MOVE_RIGHT_WORD:
                    on_search_move_to_rgroup(pnode);
                    break;
                case IDM_SEARCH_MOVE_LEFT_WORDGROUP:
                    on_search_move_to_lword(pnode);
                    break;
                case IDM_SEARCH_MOVE_RIGHT_WORDGROUP:
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
                case IDM_SEARCH_REMOVE_ALL_BOOKMARKS:
                    on_search_remove_marks_all(pnode);
                    break;
                case IDM_SEARCH_GOTO_PREV_BOOKMARK:
                    on_search_jmp_premark_this(pnode);
                    break;
                case IDM_SEARCH_GOTO_NEXT_BOOKMARK:
                    on_search_jmp_next_mark_this(pnode);
                    break;
                case IDM_SEARCH_GOTO_PREV_BOOKMARK_INALL:
                    on_search_jmp_premark_all(pnode);
                    break;
                case IDM_SEARCH_GOTO_NEXT_BOOKMARK_INALL:
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
                case IDM_SEARCH_MATCHING_BRACE:
                case IDM_SEARCH_MATCHING_BRACE_SELECT:
                    on_search_jmp_matching_brace(pnode, &wm_id);
                    break;
                case IDM_SEARCH_NAVIGATE_PREV_THIS:
                    on_search_back_navigate_this(pnode);
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
                case IDM_VIEW_DOCUMENT_MAP:
                    on_view_document_map(pnode);
                    break;
                case IDM_VIEW_MODIFY_STYLETHEME:
                    on_view_modify_theme();
                    break;
                case IDM_VIEW_COPYNEW_STYLETHEME:
                    on_view_copy_theme();
                    break;
                case IDM_VIEW_HEXEDIT_MODE:
                    hexview_switch_item(pnode);
                    break;
                case IDM_VIEW_HIGHLIGHT_BRACE:
                    on_view_light_brace();
                    break;
                case IDM_VIEW_HIGHLIGHT_STR:
                    on_view_light_str();
                    break;
                case IDM_VIEW_HIGHLIGHT_FOLD:
                    on_view_light_fold();
                    break;
                case IDM_FORMAT_REFORMAT_JSON:
                    if (pnode->doc_ptr && !pnode->hex_mode && pnode->doc_ptr->doc_type == DOCTYPE_JSON)
                    {
                        on_format_file_style(pnode);
                        on_symtree_json(pnode);
                        on_sci_refresh_ui(pnode);
                    }
                    break;
                case IDM_FORMAT_COMPRESS_JSON:
                    if (pnode->doc_ptr && !pnode->hex_mode && pnode->doc_ptr->doc_type == DOCTYPE_JSON)
                    {
                        on_format_do_compress(pnode, on_format_json_callback);
                        on_symtree_json(pnode);
                        on_sci_refresh_ui(pnode);
                    }
                    break;
                case IDM_FORMAT_REFORMAT_JS:
                    if (pnode->doc_ptr && !pnode->hex_mode && pnode->doc_ptr->doc_type == DOCTYPE_JAVASCRIPT)
                    {
                        on_format_file_style(pnode);
                        on_symlist_reqular(pnode);
                        on_sci_refresh_ui(pnode);
                    }
                    break;
                case IDM_FORMAT_COMPRESS_JS:
                    if (pnode->doc_ptr && !pnode->hex_mode && pnode->doc_ptr->doc_type == DOCTYPE_JAVASCRIPT)
                    {
                        on_format_do_compress(pnode, on_format_js_callback);
                        on_symlist_reqular(pnode);
                        on_sci_refresh_ui(pnode);
                    }
                    break;
                case IDM_FORMAT_WHOLE_FILE:
                    on_format_clang_file(pnode, true);
                    on_sci_refresh_ui(pnode);
                    break;
                case IDM_FORMAT_RANGLE_STR:
                    on_format_clang_file(pnode, false);
                    on_sci_refresh_ui(pnode);
                    break;
                case IDM_FORMAT_RUN_SCRIPT:
                    if (pnode->doc_ptr && !pnode->hex_mode && pnode->doc_ptr->doc_type == DOCTYPE_LUA)
                    {
                        on_toolbar_lua_exec(pnode);
                    }
                    break;
                case IDM_FORMAT_BYTE_CODE:
                    if (pnode->doc_ptr && !pnode->hex_mode && pnode->doc_ptr->doc_type == DOCTYPE_LUA)
                    {
                        do_byte_code(pnode);
                    }
                    break;
                case IDM_FORMAT_HYPERLINKHOTSPOTS:
                {
                    eu_get_config()->m_hyperlink ^= true;
                    break;
                }
                case IDM_FORMAT_CHECK_INDENTATION:
                {
                    on_format_check_indentation(pnode);
                    break;
                }
                case IDM_SKYLAR_AUTOMATIC_UPDATE:
                {
                    eu_get_config()->upgrade.enable ^= true;
                    break;
                }
                case IDM_VIEW_WRAPLINE_MODE:
                {
                    on_view_wrap_line();
                    break;
                }
                case IDM_VIEW_TAB_WIDTH:
                    on_view_tab_width(hwnd, pnode);
                    break;
                case IDM_TAB_CONVERT_SPACES:
                    on_view_space_converter(hwnd, pnode);
                    break;
                case IDM_VIEW_LINENUMBER_VISIABLE:
                    on_view_line_num();
                    break;
                case IDM_VIEW_BOOKMARK_VISIABLE:
                    on_view_bookmark();
                    break;
                case IDM_VIEW_WHITESPACE_VISIABLE:
                    on_view_white_space();
                    break;
                case IDM_VIEW_NEWLINE_VISIABLE:
                    on_view_line_visiable();
                    break;
                case IDM_VIEW_INDENTGUIDES_VISIABLE:
                    on_view_indent_visiable();
                    break;
                case IDM_VIEW_TIPS_ONTAB:
                    eu_get_config()->m_tab_tip ^= true;
                    break;
                case IDM_VIEW_LEFT_TAB:
                case IDM_VIEW_RIGHT_TAB:
                case IDM_VIEW_FAR_LEFT_TAB:
                case IDM_VIEW_FAR_RIGHT_TAB:
                    eu_get_config()->m_tab_active = wm_id;
                    break;
                case IDM_VIEW_TAB_RIGHT_CLICK:
                case IDM_VIEW_TAB_LEFT_DBCLICK:
                {
                    if (eu_get_config()->m_close_way == wm_id)
                    {
                        eu_get_config()->m_close_way = 0;
                    }
                    else
                    {
                        eu_get_config()->m_close_way = wm_id;
                    }
                    break;
                }
                case IDM_VIEW_TAB_RIGHT_NEW:
                case IDM_VIEW_TAB_DBCLICK_NEW:
                {
                    if (eu_get_config()->m_new_way == wm_id)
                    {
                        eu_get_config()->m_new_way = 0;
                    }
                    else
                    {
                        eu_get_config()->m_new_way = wm_id;
                    }
                    break;
                }
                case IDM_TABCLOSE_FOLLOW:
                case IDM_TABCLOSE_ALWAYS:
                case IDM_TABCLOSE_NONE:
                    eu_get_config()->m_close_draw = wm_id;
                    if (g_tabpages)
                    {
                        UpdateWindowEx(g_tabpages);
                    }
                    break;
                case IDM_VIEW_SCROLLCURSOR:
                    eu_get_config()->scroll_to_cursor ^= true;
                    break;
                case IDM_VIEW_SWITCH_TAB:
                    on_tabpage_switch_next(hwnd);
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
                    on_view_show_fold_lines();
                    break;
                case IDM_SOURCE_BLOCKFOLD_TOGGLE:
                    on_code_switch_fold(pnode, -1);
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
                    eu_get_config()->eu_complete.enable ^= true;
                    break;
                case IDM_SOURCEE_ACSHOW_CHARS:
                    on_code_set_complete_chars(pnode);
                    break;
                case IDM_SOURCE_ENABLE_CTSHOW:
                    eu_get_config()->eu_calltip.enable ^= true;
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
                    on_code_insert_config(pnode);
                    break;
                case IDM_SOURCE_SNIPPET_ENABLE:
                {
                    if (eu_get_config()->eu_complete.snippet == wm_id)
                    {
                        eu_get_config()->eu_complete.snippet = 0;
                    }
                    else
                    {
                        eu_get_config()->eu_complete.snippet = wm_id;
                    }
                    break;
                }
                case IDM_SOURCE_SNIPPET_CONFIGURE:
                    on_snippet_create_dlg(hwnd);
                    break;
                case IDM_DATABASE_EXECUTE_SQL:  // 执行选定sql,redis
                    on_view_result_show(pnode, 0);
                    break;
                case IDM_PROGRAM_EXECUTE_ACTION:  // 执行预置动作
                    on_toolbar_execute_script();
                    break;
                case IDM_ENV_FILE_POPUPMENU:
                    eu_reg_file_popup_menu();
                    break;
                case IDM_ENV_DIRECTORY_POPUPMENU:
                    eu_reg_dir_popup_menu();
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
                    _sntprintf(bak.rel_path, MAX_BUFFER, _T("%s\\README_CN.MD"), eu_module_path);
                    on_file_only_open(&bak, true);
                    break;
                }
                case IDM_CHANGELOG:
                {
                    file_backup bak = {0};
                    _sntprintf(bak.rel_path, MAX_BUFFER, _T("%s\\share\\changelog"), eu_module_path);
                    on_file_only_open(&bak, true);
                    break;
                }
                case IDM_HELP_COMMAND:
                    eu_about_command();
                    break;
                case IDM_VIEW_FULLSCREEN:
                {
                    on_view_full_sreen(hwnd);
                    break;
                }
                case IDM_TABPAGE_LOCKED:
                {
                    eu_get_config()->inter_reserved_1 ^= true;
                    break;
                }
                case IDM_VIEW_MENUBAR:
                {
                    eu_get_config()->m_menubar ^= true;
                    eu_get_config()->m_menubar?(GetMenu(hwnd)?(void)0:SetMenu(hwnd, i18n_load_menu(IDC_SKYLARK))):SetMenu(hwnd, NULL);
                    on_proc_msg_size(hwnd, NULL);
                    break;
                }
                case IDB_SIZE_1:
                case IDB_SIZE_16:
                case IDB_SIZE_24:
                case IDB_SIZE_32:
                case IDB_SIZE_48:
                case IDB_SIZE_64:
                case IDB_SIZE_80:
                case IDB_SIZE_96:
                case IDB_SIZE_112:
                case IDB_SIZE_128:
                {
                    if (eu_get_config()->m_toolbar != wm_id)
                    {
                        eu_get_config()->m_toolbar = wm_id;
                        g_toolbar_size = wm_id;
                        if (on_toolbar_refresh(hwnd))
                        {
                            on_proc_msg_size(hwnd, NULL);
                        }
                    }
                    break;
                }
                case IDM_VIEW_TOOLBAR:
                case IDB_SIZE_0:
                {
                    if (eu_get_config()->m_toolbar != IDB_SIZE_0)
                    {
                        g_toolbar_size = eu_get_config()->m_toolbar;
                        eu_get_config()->m_toolbar = IDB_SIZE_0;
                    }
                    else
                    {
                        eu_get_config()->m_toolbar = g_toolbar_size ? g_toolbar_size : IDB_SIZE_1;
                    }
                    if (on_toolbar_refresh(hwnd))
                    {
                        on_proc_msg_size(hwnd, NULL);
                    }
                    break;
                }
                case IDM_VIEW_STATUSBAR:
                {
                    if (eu_get_config() && !(eu_get_config()->m_fullscreen))
                    {
                        eu_get_config()->m_statusbar ^= true;
                        on_proc_msg_size(hwnd, NULL);
                    }
                    break;
                }
                case IDM_TAB_CLOSE_LEFT:
                    on_file_left_close();
                    break;
                case IDM_TAB_CLOSE_RIGHT:
                    on_file_right_close();
                    break;
                case IDM_ABOUT:
                    on_about_dialog();
                    break;
                default:
                    return DefWindowProc(hwnd, message, wParam, lParam);
            }
            break;
        }
        case WM_NOTIFY:
        {
            lpnmhdr = (NMHDR *) lParam;
            lpnotify = (ptr_notify) lParam;
            p_tips = (TOOLTIPTEXT *) lParam;
            eu_tabpage *pview = NULL;
            if (!lpnmhdr || !lpnotify || !p_tips)
            {
                break;
            }
            if (lpnmhdr->hwndFrom == g_filetree)
            {
                SendMessage(g_filetree, WM_NOTIFY, wParam, lParam);
                break;
            }
            if (!(pnode = on_tabpage_focus_at()))
            {
                break;
            }
            if (pnode->presult && pnode->presult->hwnd_sc && lpnmhdr->hwndFrom == pnode->presult->hwnd_sc)
            {
                break;
            }
            if ((pview = on_map_edit()) && pview->hwnd_sc && lpnmhdr->hwndFrom == pview->hwnd_sc)
            {
                break;
            }
            switch (lpnmhdr->code)
            {
                case EU_SAVE_CONFIG:
                {
                    eu_save_config();
                    eu_save_theme();
                    break;
                }
                case NM_CLICK:
                    if (!pnode->hex_mode && g_statusbar && lpnmhdr->hwndFrom == g_statusbar)
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
                // 16进制编辑器视图消息响应
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
                    on_sci_point_left(pnode);
                    break;
                }
                case NM_SETFOCUS:
                {
                    DrawMenuBar(hwnd);
                    break;
                }
                // scintilla控件响应消息, 其他消息见eu_scintill.c
                case SCN_CHARADDED:
                    on_sci_character(on_tabpage_get_handle(lpnotify->nmhdr.hwndFrom), lpnotify);
                    break;
                case SCN_AUTOCCHARDELETED:
                    on_sci_character(on_tabpage_focus_at(), 0);
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
                                if (!eu_sci_call(pnode, SCI_CANUNDO, 0, 0))
                                {
                                    eu_sci_call(pnode, SCI_EMPTYUNDOBUFFER, 0, 0);
                                }
                            }
                        }
                    }
                    break;
                case TCN_TABDROPPED_OUT:
                    on_file_out_open(eu_int_cast(lpnotify->nmhdr.hwndFrom), NULL);
                    break;
                case SCN_SAVEPOINTREACHED:
                    on_sci_point_reached(on_tabpage_get_handle(lpnotify->nmhdr.hwndFrom));
                    break;
                case SCN_SAVEPOINTLEFT:
                    on_sci_point_left(on_tabpage_get_handle(lpnotify->nmhdr.hwndFrom));
                    break;
                case SCN_MARGINCLICK:
                {
                    sptr_t lineno = eu_sci_call(pnode, SCI_LINEFROMPOSITION, lpnotify->position, 0);
                    if (lpnotify->margin == 1)
                    {
                        on_search_toggle_mark(pnode, lineno);
                    }
                    else if (lpnotify->margin == 2)
                    {
                        on_code_switch_fold(pnode, lineno);
                    }
                    break;
                }
                case SCN_INDICATORRELEASE:
                {
                    bool up = KEY_UP(VK_MENU) && KEY_UP(VK_INSERT) && KEY_UP(VK_SHIFT);
                    if (up)
                    {
                        on_hyper_click(pnode, hwnd, lpnotify->position, KEY_DOWN(VK_CONTROL));
                    }
                    break;
                }
                case SCN_PAINTED:
                {
                    if ((lpnotify->nmhdr.hwndFrom == pnode->hwnd_sc) && pnode->map_show && document_map_initialized)
                    {
                        eu_tabpage *map_edit = hwnd_document_map ? (eu_tabpage *)GetWindowLongPtr(hwnd_document_map, GWLP_USERDATA) : NULL;
                        if (map_edit)
                        {
                            on_map_scroll(pnode, map_edit);
                        }
                    }
                    break;
                }
                case SCN_UPDATEUI:
                {
                    if ((lpnotify->updated))
                    {
                        if (eu_get_config()->m_hyperlink && pnode && (!(pnode->hex_mode || pnode->plugin)))
                        {
                            on_hyper_update_style(pnode);
                        }
                        if (lpnotify->updated & SC_UPDATE_SELECTION)
                        {
                            if (eu_get_config()->m_light_str || KEY_DOWN(VK_SHIFT))
                            {
                                on_view_editor_selection(pnode);
                            }
                            if (eu_get_config()->m_toolbar != IDB_SIZE_0)
                            {
                                on_toolbar_setup_button(IDM_EDIT_CUT, !pnode->pmod && util_can_selections(pnode) ? 2 : 1);
                                on_toolbar_setup_button(IDM_EDIT_COPY, !pnode->pmod && TAB_NOT_NUL(pnode) ? 2 : 1);
                            }
                            on_search_turn_select(pnode);
                        }
                        else if ((lpnotify->updated & SC_UPDATE_CONTENT) && pnode->map_show && document_map_initialized)
                        {
                            if (hwnd_document_map)
                            {
                                eu_tabpage *map_edit = (eu_tabpage *)GetWindowLongPtr(hwnd_document_map, GWLP_USERDATA);
                                if (map_edit)
                                {
                                    on_map_reload(map_edit);
                                }
                            }
                        }
                        on_statusbar_update_filesize(pnode);
                    }
                    break;
                }
                // Scintilla 5.3.2, SCN_AUTOCCOMPLETED supports SCI_AUTOCSETCHOOSESINGLE mode
                // So, we replace SCN_AUTOCSELECTION with SCN_AUTOCCOMPLETED
                case SCN_AUTOCCOMPLETED:
                {
                    int opt = (int)eu_sci_call(pnode, SCI_AUTOCGETOPTIONS, 0, 0);
                    if (((opt & SC_AUTOCOMPLETE_SNIPPET) && pnode->ac_mode != AUTO_CODE) || on_complete_auto_expand(pnode, lpnotify->text, lpnotify->position))
                    {
                        on_complete_reset_focus(pnode);
                        on_complete_snippet(pnode);
                    }
                    return 1;
                }
                case TTN_NEEDTEXT:
                {
                    if (!eu_get_config()->m_tab_tip)
                    {
                        break;
                    }
                    if (p_tips->hdr.hwndFrom != TabCtrl_GetToolTips(g_tabpages))
                    {
                        break;
                    }
                    if ((pnode = on_tabpage_get_ptr((int) (p_tips->hdr.idFrom))))
                    {   // 显示标签的快捷键提示
                        memset(p_tips->szText, 0, sizeof(p_tips->szText));
                        if ((int) (p_tips->hdr.idFrom) <= 8)
                        {
                            _sntprintf(p_tips->szText, _countof(p_tips->szText) - 1, _T("%.68s - (Alt+%d)"), pnode->pathfile, (int) (p_tips->hdr.idFrom) + 1);
                        }
                        else
                        {
                            _sntprintf(p_tips->szText, _countof(p_tips->szText) - 1, _T("%.68s"), pnode->pathfile);
                        }
                    }
                    break;
                }
                case NPP_DOC_MODIFY:
                {
                    on_proc_save_status(wParam, (npn_nmhdr *)lParam);
                    break;
                }
                case NPP_DOC_STATUS:
                {
                    if (pnode && lParam && ((npn_nmhdr *)lpnmhdr)->nm.idFrom > 0)
                    {
                        if (GetParent((HWND)(((npn_nmhdr *)lpnmhdr)->nm.idFrom)) == pnode->hwnd_sc)
                        {
                            return (intptr_t)pnode->be_modify;
                        }
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case WM_COPYDATA:
        {
            COPYDATASTRUCT *cpd = (COPYDATASTRUCT *) lParam;
            if (cpd)
            {
                file_backup *pm = (file_backup *) (cpd->lpData);
                size_t rel_len = (pm && pm->rel_path) ? _tcslen(pm->rel_path) : 0;
                if (rel_len > 0 && pm->rel_path[rel_len - 1] == _T('#'))
                {
                    pm->rel_path[rel_len - 1] = 0;
                    // 先打开空白标签, 然后打开文件管理器
                    if (g_tabpages && TabCtrl_GetItemCount(g_tabpages) < 1)
                    {
                        on_file_redirect(hwnd, NULL);
                    }
                    on_treebar_locate_path(pm->rel_path);
                }
                else
                {   // 文件可能被重定向
                    on_file_redirect(hwnd, pm);
                }
            }
            break;
        }
        case WM_ACTIVATE:
        {
            if (LOWORD(wParam) != WA_INACTIVE && (pnode = on_tabpage_focus_at()))
            {
                if (pnode->hwnd_sc && GetWindowLongPtr(pnode->hwnd_sc, GWL_STYLE) & WS_VISIBLE)
                {
                    SetFocus(pnode->hwnd_sc);
                }
            }
            break;
        }
        case WM_DROPFILES:
            if (wParam)
            {
                on_file_drop((HDROP) wParam);
            }
            break;
        case WM_MOVE:
        {
            HWND hwnd_clip = on_toolbar_clip_hwnd();
            if (hwnd_clip && IsWindow(hwnd_clip) && IsWindowVisible(hwnd_clip))
            {
                on_toolbar_setpos_clipdlg(hwnd_clip, hwnd);
            }
            if (document_map_initialized && hwnd_document_map)
            {
                PostMessage(hwnd_document_map, WM_MOVE, 0, 0);
            }
            if (g_statusbar)
            {
                PostMessage(g_statusbar, WM_MOVE, 0, 0);
            }
            break;
        }
        case WM_CLOSE:
            if (hwnd == g_hwndmain)
            {
                on_file_edit_exit(hwnd);
            }
            break;
        case WM_BACKUP_OVER:
            if (hwnd == g_hwndmain)
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
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SKYLARK));
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

void
eu_close_edit(void)
{
    SendMessage(eu_module_hwnd(), WM_CLOSE, 0, 0);
}

HWND
eu_create_main_window(HINSTANCE instance)
{
    CloseHandle((HANDLE) _beginthreadex(NULL, 0, do_calss_drop, NULL, 0, NULL));
    if (class_register(instance))
    {
        INITCOMMONCONTROLSEX icex;
        uint32_t ac_flags = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
        icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icex.dwICC = ICC_TAB_CLASSES | ICC_COOL_CLASSES | ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES | ICC_USEREX_CLASSES;
        if (InitCommonControlsEx(&icex))
        {
            LOAD_APP_RESSTR(IDS_APP_TITLE, app_title);
            g_hwndmain = CreateWindowEx(WS_EX_ACCEPTFILES, APP_CLASS, app_title, ac_flags, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, instance, NULL);
            on_theme_setup_font(g_hwndmain);
        }
    }
    return g_hwndmain;
}
