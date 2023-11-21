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

#include <fcntl.h>
#include "targetver.h"
#include "framework.h"

#define DPI_PERCENT_144  144
#define DPI_PERCENT_192  192
#define DPI_PERCENT_240  240
#define DPI_PERCENT_288  288
#define DPI_PERCENT_336  336
#define DPI_PERCENT_384  384
#define DPI_PERCENT_432  432
#define DPI_PERCENT_480  480
#define HIGHT_2X         2560
#define HIGHT_2Y         1440
#define HIGHT_4X         3840
#define HIGHT_4Y         2160
#define HIGHT_8X         7680
#define HIGHT_8Y         4320

#define DARK_HOTCOLOR    "#D3D3D3"
#define DARK_UNHOTCOLOR  "#4E4E4E"
#define BLUE_UNHOTCOLOR  "#D3D3D3"
#define WINE_UNHOTCOLOR  "#696969"

#define CHECK_IF(a) if ((a)!= 0) return false

typedef struct _toolbar_data
{
    int bmp_wh;
    int bar_wh;
    char hcolor[OVEC_LEN];
    char gcolor[OVEC_LEN];
} toolbar_data;

int g_toolbar_size = 0;

enum {READ_FD, WRITE_FD};
static int g_toolbar_height;
static int fd_stdout;
static int fd_pipe[2];
static int m_index;
static int m_locked;
static bool m_copy;
static HWND m_edit[EDITNUMBS];
static bool m_block[EDITNUMBS];
static HWND m_chain;
static HWND g_clip_hwnd;
static HIMAGELIST img_list1;
static HIMAGELIST img_list2;

static int
on_toolbar_fill_params(toolbar_data *pdata, const int resid)
{
    int cx = GetSystemMetrics(SM_CXSCREEN);
    int cy = GetSystemMetrics(SM_CYSCREEN);
    const bool dark = on_dark_enable();
    if (dark)
    {
        strncpy(pdata->hcolor, DARK_HOTCOLOR, OVEC_LEN - 1);
        strncpy(pdata->gcolor, DARK_UNHOTCOLOR, OVEC_LEN - 1);
    }
    else
    {
        strncpy(pdata->gcolor, util_under_wine() ? WINE_UNHOTCOLOR : BLUE_UNHOTCOLOR, OVEC_LEN - 1);
    }
    if (resid == IDB_SIZE_1)
    {
        const uint32_t dpi = eu_get_dpi(NULL);
        if (dpi >= DPI_PERCENT_480)
        {
            pdata->bmp_wh = 128;
        }
        else if (dpi >= DPI_PERCENT_432)
        {
            pdata->bmp_wh = 112;
        }
        else if (dpi >= DPI_PERCENT_384)
        {
            pdata->bmp_wh = 96;
        }
        else if (dpi >= DPI_PERCENT_336)
        {
            pdata->bmp_wh = 80;
        }
        else if (dpi >= DPI_PERCENT_288)
        {
            pdata->bmp_wh = 64;
        }
        else if (dpi >= DPI_PERCENT_240 || (cx >= HIGHT_8X && cy >= HIGHT_8Y))
        {
            pdata->bmp_wh = 48;
        }
        else if (dpi >= DPI_PERCENT_192 || (cx >= HIGHT_4X && cy >= HIGHT_4Y))
        {
            pdata->bmp_wh = 32;
        }
        else if (dpi >= DPI_PERCENT_144 || (cx >= HIGHT_2X && cy >= HIGHT_2Y))
        {
            pdata->bmp_wh = 24;
        }
        else
        {
            pdata->bmp_wh = 16;
        }
    }
    else if (resid > IDB_SIZE_1)
    {
        switch (resid)
        {
            case IDB_SIZE_16:
                pdata->bmp_wh = 16;
                break;
            case IDB_SIZE_24:
                pdata->bmp_wh = 24;
                break;
            case IDB_SIZE_32:
                pdata->bmp_wh = 32;
                break;
            case IDB_SIZE_48:
                pdata->bmp_wh = 48;
                break;
            case IDB_SIZE_64:
                pdata->bmp_wh = 64;
                break;
            case IDB_SIZE_80:
                pdata->bmp_wh = 80;
                break;
            case IDB_SIZE_96:
                pdata->bmp_wh = 96;
                break;
            case IDB_SIZE_112:
                pdata->bmp_wh = 112;
                break;
            case IDB_SIZE_128:
                pdata->bmp_wh = 128;
                break;
            default:
                break;
        }
    }
    if (pdata->bmp_wh > 0)
    {
        pdata->bar_wh = pdata->bmp_wh + 10;
    }
    return pdata->bar_wh;
}

static bool
on_toolbar_init_params(toolbar_data **pdata, const int resid)
{
    if ((*pdata = (toolbar_data *)calloc(1, sizeof(toolbar_data))) != NULL)
    {
        return on_toolbar_fill_params(*pdata, resid) > 0;
    }
    return false;
}

/**********************************************
 * 设置工具栏按钮的状态,
 * id 为 资源id号
 * flags == 0, 自动翻转
 * flags == 1, 设为禁止状态
 * flags == 2. 设为启用状态
 **********************************************/
void
on_toolbar_setup_button(int id, int flags)
{
    LRESULT status = 0;
    HWND h_tool = GetDlgItem(eu_module_hwnd(), IDC_TOOLBAR);
    if (h_tool)
    {
        if ((status = SendMessage(h_tool, TB_GETSTATE, id, 0)) == -1)
        {
            return;
        }
        switch (flags)
        {
            case 0:
                switch (status)
                {
                    case TBSTATE_INDETERMINATE:
                        SendMessage(h_tool, TB_SETSTATE, id, TBSTATE_ENABLED);
                        break;
                    case TBSTATE_ENABLED:
                        SendMessage(h_tool, TB_SETSTATE, id, TBSTATE_INDETERMINATE);
                        break;
                }
                break;
            case 1:
                if (status == TBSTATE_ENABLED)
                {
                    SendMessage(h_tool, TB_SETSTATE, id, TBSTATE_INDETERMINATE);
                }
                break;
            case 2:
                if (status == TBSTATE_INDETERMINATE)
                {
                    SendMessage(h_tool, TB_SETSTATE, id, TBSTATE_ENABLED);
                }
                break;
            default:
                break;
        }
    }
}

HWND
on_toolbar_hwnd(void)
{
    HWND hwnd = eu_module_hwnd();
    return hwnd ? GetDlgItem(hwnd, IDC_TOOLBAR) : NULL;
}

void
on_toolbar_destroy(HWND hwnd)
{
    HWND htool = hwnd ? GetDlgItem(hwnd, IDC_TOOLBAR) : NULL;
    if (htool)
    {
        DestroyWindow(htool);
    }
}

void
on_toolbar_adjust_box(void)
{
    int m_bar = eu_get_config()->m_toolbar;
    if (m_bar == IDB_SIZE_0)
    {
        g_toolbar_height = 0;
    }
    else
    {
        toolbar_data data = {0};
        g_toolbar_height = on_toolbar_fill_params(&data, m_bar);
    }
}

void
on_toolbar_setpos_clipdlg(HWND hwnd, HWND hparent)
{
    if (hwnd && IsWindow(hwnd) && IsWindow(hparent))
    {
        RECT rc, rcparent;
        int width, height, left, top;
        int border = util_os_version() >= 1000 ? 0 : 7;
        GetWindowRect(hwnd, &rc);
        GetWindowRect(hparent, &rcparent);
        width = rc.right - rc.left;
        height = rc.bottom - rc.top;
        left = rcparent.right - width - border - eu_dpi_scale_xy(0, 20);
        top = rcparent.bottom - height - on_statusbar_height() - border - 2;
        eu_setpos_window(hwnd, HWND_TOPMOST, left, top, width, height, 0);
    }
}

static bool
init_clip_dlg(HWND dialog, bool init)
{
    int w, h;
    HICON hicon = NULL;
    HBITMAP hbmp = NULL;
    const char* svg_icon = \
        "<svg version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\" width=\"32\" height=\"32\" viewBox=\"0 0 32 32\"><g id=\"c01\"><path fill=\"#455f91\" opacity=\"1.00\" d=\" M 9.81 0.00 L 21.87 0.00 C 25.02 1.37 28.18 4.20 28.00 7.92 C 27.91 13.27 28.31 18.65 27.74 23.98 C 26.58 26.98 22.64 25.70 20.17 26.05 C 20.02 28.20 20.04 30.62 18.07 32.00 L 1.67 32.00 C 1.91 30.65 1.35 30.07 0.00 30.27 L 0.00 8.06 C 1.65 5.27 5.12 6.13 7.83 5.95 C 7.94 3.80 8.10 1.54 9.81 0.00 M 11.06 3.07 C 10.94 9.71 11.03 16.36 11.00 23.00 C 15.67 23.00 20.33 23.00 25.00 23.00 C 25.00 18.67 25.00 14.34 25.00 10.00 C 23.00 10.01 20.99 10.10 19.00 9.91 C 16.99 8.32 18.38 5.21 18.00 2.99 C 15.68 3.00 13.37 3.02 11.06 3.07 M 21.04 2.66 C 21.01 4.11 21.00 5.56 20.99 7.01 C 22.44 7.00 23.89 6.99 25.34 6.96 C 24.22 5.24 22.76 3.78 21.04 2.66 M 3.07 9.07 C 2.92 15.71 3.03 22.35 3.00 29.00 C 7.67 29.00 12.33 29.00 17.00 29.00 C 17.00 28.25 17.00 26.75 17.00 26.01 C 14.18 25.62 9.80 27.26 8.26 24.05 C 7.64 19.06 8.17 14.00 8.00 8.98 C 6.36 9.00 4.71 9.04 3.07 9.07 Z\" /></g></svg>";
    HWND hbtn = dialog ? GetDlgItem(dialog, IDC_BUTTON0) : NULL;
    if (!hbtn)
    {
        return false;
    }
    else
    {
        RECT rc;
        GetWindowRect(hbtn, &rc);
        w = rc.right - rc.left;
        h = rc.bottom - rc.top;
    }
    if (!(w > 0 && h > 0))
    {
        return false;
    }
    if (util_under_wine())
    {
        if (!(hicon = LoadIcon(eu_module_handle(), MAKEINTRESOURCE(IDB_TXT))))
        {
            eu_logmsg("%s: LoadIcon failed\n", __FUNCTION__);
            return false;
        }
    }
    else if (!(hbmp = on_pixmap_from_svg(svg_icon, w, w, on_dark_enable() ? DARK_HOTCOLOR : NULL)))
    {   // nsvgRasterize不支持纵横比缩放, 在这里按宽度缩放
        eu_logmsg("%s: on_pixmap_from_svg failed\n", __FUNCTION__);
        return false;
    }
    for (int i = 0; i < _countof(m_edit); ++i)
    {
        hbtn = GetDlgItem(dialog, IDC_BUTTON0 + i);
        if (hicon)
        {   // wine bug 15505
            LONG_PTR style = GetWindowLongPtr(hbtn, GWL_STYLE);
            SetWindowLongPtr(hbtn, GWL_STYLE, (style & ~SS_BITMAP) | SS_ICON);
            Static_SetIcon(hbtn, hicon);
        }
        else if (hbmp)
        {   // 在静态控件上加载位图
            SendMessage(hbtn, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbmp);
        }
        m_edit[i] = GetDlgItem(dialog, IDC_EDIT0 + i);
    }
    if (hicon)
    {
        DeleteObject(hicon);
    }
    if (hbmp)
    {
        DeleteObject(hbmp);
    }
    if (init)
    {
        SetLastError(0);
        m_chain = SetClipboardViewer(dialog);
    }
    return (GetLastError() == 0);
}

static void
clip_copy(uint32_t nid)
{
    int i = nid - IDC_BUTTON0;
    Edit_SetSel(m_edit[i], 0, -1);
    m_copy = false;
    SendMessage(m_edit[i], WM_COPY, 0, 0);
}

static void
clip_lock(uint32_t nid)
{
    int i = nid - IDC_CHECK0;
    m_block[i]  ^= true;
    if (m_block[i])
    {
        ++m_locked;
    }
    else
    {
        --m_locked;
    }
}

static void
draw_clipboard(void)
{
    if (m_copy && m_locked < _countof(m_block) && IsClipboardFormatAvailable(CF_TEXT))
    {
        while (m_block[m_index])
        {
            m_index = (m_index + 1) % _countof(m_block);
        }
        Edit_SetSel(m_edit[m_index], 0, -1);
        SendMessage(m_edit[m_index], WM_PASTE, 0, 0);
        m_index = (m_index + 1) % _countof(m_edit);
    }
    if (m_chain != NULL)
    {
        SendMessage(m_chain, WM_DRAWCLIPBOARD, 0, 0);
    }
    m_copy = true;
}

static void
refresh_clipboard(void)
{
    if (g_clip_hwnd)
    {
        init_clip_dlg(g_clip_hwnd, false);
        if (IsWindowVisible(g_clip_hwnd))
        {
            UpdateWindowEx(g_clip_hwnd);  // 在某些平台上, 可能需要重绘所有界面
        }
    }
}

static intptr_t CALLBACK
clip_proc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
        {
            if (init_clip_dlg(hdlg, true) && on_dark_enable())
            {
                on_dark_set_theme(hdlg, L"Explorer", NULL);
            }
            break;
        }
        case WM_THEMECHANGED:
        {
            if (on_dark_supports())
            {
                bool dark = on_dark_enable();
                on_dark_allow_window(hdlg, dark);
                on_dark_refresh_titlebar(hdlg);
                const int buttons[] = {IDC_BUTTON0,
                                       IDC_BUTTON1,
                                       IDC_BUTTON2,
                                       IDC_BUTTON3,
                                       IDC_BUTTON4,
                                       IDC_BUTTON5,
                                       IDC_BUTTON6,
                                       IDC_BUTTON7,
                                       IDC_BUTTON8,
                                       IDC_BUTTON9,
                                       IDC_BUTTON10};
                for (int id = 0; id < _countof(buttons); ++id)
                {
                    HWND btn = GetDlgItem(hdlg, buttons[id]);
                    on_dark_allow_window(btn, dark);
                    on_dark_set_theme(btn, L"Explorer", NULL);
                }
                refresh_clipboard();
                break;
            }
        }
    CASE_WM_CTLCOLOR_SET:
        {
            return on_dark_set_contorl_color(wParam);
        }
        case WM_COMMAND:
        {
            uint16_t wmid = LOWORD(wParam);
            uint16_t wmnc = HIWORD(wParam);
            switch (wmid)
            {
                case IDC_BUTTON0:
                case IDC_BUTTON1:
                case IDC_BUTTON2:
                case IDC_BUTTON3:
                case IDC_BUTTON4:
                case IDC_BUTTON5:
                case IDC_BUTTON6:
                case IDC_BUTTON7:
                case IDC_BUTTON8:
                case IDC_BUTTON9:
                case IDC_BUTTON10:
                    if (wmnc == STN_CLICKED)
                    {
                        clip_copy(wmid);
                    }
                    break;
                case IDC_CHECK0:
                case IDC_CHECK1:
                case IDC_CHECK2:
                case IDC_CHECK3:
                case IDC_CHECK4:
                case IDC_CHECK5:
                case IDC_CHECK6:
                case IDC_CHECK7:
                case IDC_CHECK8:
                case IDC_CHECK9:
                case IDC_CHECK10:
                    if (wmnc == BN_CLICKED)
                    {
                        clip_lock(wmid);
                    }
                    break;
                default:
                    break;
            }
            break;
        }
        case WM_DRAWCLIPBOARD:
        {
            draw_clipboard();
            HWND hwnd = GetClipboardOwner();
            if (!(hwnd && (hwnd == eu_module_hwnd())))
            {   // 不是16进制编辑器写剪贴板
                hexview_set_area(0);
            }
            break;
        }
        case WM_CHANGECBCHAIN:
        {
            if ((HWND) wParam == m_chain)
            {
                m_chain = (HWND) lParam;
            }
            else
            {
                SendMessage(m_chain, message, wParam, lParam);
            }
            break;
        }
        case WM_CLEAN_CHAIN:
        {
            for (int id = IDC_EDIT0; id <= IDC_EDIT10; ++id)
            {
                HWND edt = GetDlgItem(hdlg, id);
                if (edt)
                {
                    Edit_SetText(edt, _T(""));
                }
            }
            break;
        }
        case WM_SHOWWINDOW:
        {
            return 1;
        }
        case WM_CLOSE:
        {
            ShowWindow(hdlg, SW_HIDE);
            return 1;
        }
        case WM_DESTROY:
        {
            if (g_clip_hwnd)
            {
                ChangeClipboardChain(hdlg, m_chain);
                m_chain = NULL;
                g_clip_hwnd = NULL;
            }
            return 1;
        }
        default:
            break;
    }
    return 0;
}

HWND
on_toolbar_clip_hwnd(void)
{
    return g_clip_hwnd;
}

HWND
on_toolbar_create_clipbox(HWND hwnd)
{
    g_clip_hwnd = i18n_create_dialog(hwnd, IDD_DIALOG_CLIP, clip_proc);
    return g_clip_hwnd;
}

static HIMAGELIST
create_img_list(toolbar_data *pdata, const bool hot)
{
    int num = 0;
    HIMAGELIST himg = NULL;
    HBITMAP hbmp = on_pixmap_icons(pdata->bmp_wh, pdata->bmp_wh, hot ? pdata->hcolor : pdata->gcolor, &num);
    if (hbmp)
    {
        himg = ImageList_Create(pdata->bmp_wh, pdata->bmp_wh, ILC_COLOR32|ILC_MASK, num, 1);
        himg ? ImageList_AddMasked(himg, hbmp, RGB(0xF0, 0xF0, 0xF0)) : (void)0;
        DeleteObject((HGDIOBJ) hbmp);
    }
    return himg;
}

static bool
close_stdout_redirect(FILE *console)
{
    /* restore original standard output handle */
    CHECK_IF(_dup2(fd_stdout, _fileno(stdout)));
    _close(fd_stdout);
    _close(fd_pipe[WRITE_FD]);
    _close(fd_pipe[READ_FD]);
    eu_close_console(console);
    return true;
}

static bool
init_stdout_redirect(int size, FILE **pconsole)
{
    bool ret = false;
    if (*pconsole)
    {
        *pconsole = NULL;
    }
    if (_fileno(stdout) != 1 && AllocConsole())
    {
        *pconsole = freopen("CONOUT$", "w", stdout);
        ShowWindow (FindWindow(_T("ConsoleWindowClass"), NULL), SW_HIDE);
    }
    do
    {
        if ((_pipe(fd_pipe, size, _O_TEXT)) != 0)
        {
            break;
        }
        int fd = _fileno(stdout);
        fd_stdout = _dup(fd);
        fflush(stdout);
        if ((_dup2(fd_pipe[WRITE_FD], fd)) == 0)
        {
            setvbuf(stdout, NULL, _IONBF, 0);
            ret = true;
        }
    }while(0);
    if (!ret)
    {
        eu_close_console(*pconsole);
        fprintf(stderr, "init_stdout_redirect failed\n");
    }
    return ret;
}

static int
get_output_buffer(char *buffer, int size)
{
    int read_len = _read(fd_pipe[READ_FD], buffer, size);
    if (read_len <= 0)
    {
        fprintf(stderr, "read_len = %d\n", read_len);
        *buffer = 0;
    }
    else
    {
        buffer[read_len] = 0;
    }
    return read_len;
}

void
on_toolbar_no_highlight(void *lp)
{
    result_vec *pvec = NULL;
    eu_sci_call((eu_tabpage *)lp, SCI_SETPROPERTY, (sptr_t)result_extra, (sptr_t)&pvec);
}

void
on_toolbar_lua_exec(eu_tabpage *pnode)
{
    char *buffer = NULL;
    FILE *console = NULL;
    if (pnode && pnode->doc_ptr)
    {
        if (!pnode->presult)
        {
            pnode->result_show = on_result_launch(pnode);
        }
        if (RESULT_SHOW(pnode) && (buffer = util_strdup_content(pnode, NULL)) && init_stdout_redirect(MAX_OUTPUT_BUF, &console))
        {
            int read_len = 0;
            char *std_buffer = NULL;
            pnode->presult->pwant = on_toolbar_no_highlight;
            on_proc_resize(NULL);
            do_lua_setting_path(pnode);
            if ((std_buffer = (char *)calloc(1, MAX_OUTPUT_BUF+1)))
            {
                if (do_lua_code((const char *)buffer) == 0)
                {
                    read_len = get_output_buffer(std_buffer, MAX_OUTPUT_BUF);
                }
                else
                {   // 写标准输出设备, 防止_read函数阻塞
                    fprintf(stdout, "Failed to execute Lua script\n");
                }
                close_stdout_redirect(console);
                if (read_len > 0)
                {
                    char *pstr = util_unix_newline(std_buffer, MAX_OUTPUT_BUF);
                    if (pstr)
                    {
                        on_result_append_text_utf8("%s", pstr);
                        free(pstr);
                    }
                }
                free(std_buffer);
            }
            free(buffer);
            do_lua_setting_path(NULL);
        }
    }
}

static void
on_toolbar_create_file(const int *pv, const int size, wchar_t ***plist)
{
    size_t buf_len = 0;
    eu_tabpage *p = NULL;
    for (int i = 0; i < size; ++i)
    {
        wchar_t *pname = NULL;
        p = on_tabpage_get_ptr(pv[i]);
        if (p && p->doc_ptr && (pname = (wchar_t *)calloc(sizeof(wchar_t), MAX_PATH)))
        {
            HANDLE pfile = NULL;
            if ((pfile = util_mk_temp(pname, on_doc_get_ext(p))) != INVALID_HANDLE_VALUE)
            {
                char *pbuffer = NULL;
                if ((pbuffer = util_strdup_content(p, &buf_len)) != NULL)
                {
                    uint32_t written;
                    WriteFile(pfile, pbuffer, (unsigned long)buf_len, &written, NULL);
                    cvector_push_back(*plist, pname);
                    free(pbuffer);
                }
                CloseHandle(pfile);
            }
        }
    }
}

static bool
on_toolbar_mk_temp(wchar_t ***vec_files)
{
    cvector_vector_type(int) v = NULL;
    if (on_tabpage_sel_number(&v, true) > 0 && v)
    {
        const int size = (int)cvector_size(v);
        // 改成函数调用, cvector宏定义在一个函数里, 导致clang瞎优化
        on_toolbar_create_file(v, size, vec_files);
    }
    cvector_freep(&v);
    return (*vec_files != NULL && cvector_size(*vec_files) > 0);
}

static unsigned __stdcall
do_extra_actions(void *lp)
{
    wchar_t *abs_path = NULL;
    cvector_vector_type(wchar_t *) vec_files = NULL;
    char *pactions = eu_get_config()->m_actions[eu_int_cast(lp)];
    if (strlen(pactions) < 1)
    {
        return 1;
    }
    if (!on_toolbar_mk_temp(&vec_files))
    {
        return 1;
    }
    if ((abs_path = util_to_abs(pactions)) != NULL)
    {
        bool wine = util_under_wine();
        int count = eu_int_cast(cvector_size(vec_files));
        const int len = (count + 1) * (MAX_PATH + 1);
        wchar_t *cmd_exec = (wchar_t *)calloc(sizeof(wchar_t), len + 1);
        if (cmd_exec != NULL)
        {
            HANDLE handle = NULL;
            WCHAR *plugin = NULL;
            if (wine && (plugin = util_winexy_get()))
            {
                _snwprintf(cmd_exec, len, L"%s \\\"%s\\\" ", plugin, abs_path);
                free(plugin);
            }
            else
            {
                _snwprintf(cmd_exec, len, L"\"%s\" ", abs_path);
            }
            for (int i = 0; i < count; ++i)
            {
                WCHAR unix_path[MAX_PATH] = {0};
                if (i == count - 1)
                {
                    wcsncat(cmd_exec, wine ? L"\\\"" : L"\"", len);
                    if (wine && util_get_unix_file_name(vec_files[i], unix_path, MAX_PATH))
                    {
                        wcsncat(cmd_exec, unix_path, len);
                    }
                    else
                    {
                        wcsncat(cmd_exec, vec_files[i], len);
                    }
                    wcsncat(cmd_exec, wine ? L"\\\"" : L"\"", len);
                }
                else
                {
                    wcsncat(cmd_exec, wine ? L"\\\"" : L"\"", len);
                    if (wine && util_get_unix_file_name(vec_files[i], unix_path, MAX_PATH))
                    {
                        wcsncat(cmd_exec, unix_path, len);
                    }
                    else
                    {
                        wcsncat(cmd_exec, vec_files[i], len);
                    }
                    wcsncat(cmd_exec, wine ? L"\\\"" : L"\" ", len);
                }
            }
            if ((handle = eu_new_process(cmd_exec, NULL, NULL, 2, NULL)))
            {
                WaitForSingleObject(handle, INFINITE);
            }
            else if (!wine)
            {
                *pactions = 0;
                MSG_BOX(IDC_MSG_EXEC_ERR1, IDC_MSG_ERROR, MB_ICONERROR|MB_OK);
            }
            eu_close_handle(handle);
            if (!wine)
            {
                cvector_for_each(vec_files, DeleteFile);
            }
            free(cmd_exec);
        }
        free(abs_path);
    }
    if (vec_files)
    {
        cvector_free_each_and_free(vec_files, free);
    }
    return 0;
}

void
on_toolbar_execute_script(void)
{
    eu_tabpage *p = on_tabpage_focus_at();
    if (p && !p->hex_mode && p->doc_ptr)
    {
        intptr_t param = (intptr_t)p->doc_ptr->doc_type;
        if (strlen(eu_get_config()->m_actions[param]) > 1)
        {
            util_update_env(p);
            CloseHandle((HANDLE) _beginthreadex(NULL, 0, do_extra_actions, (void *)param, 0, NULL));
        }
        else if (param == DOCTYPE_LUA)
        {   // lua script
            on_toolbar_lua_exec(p);
        }
        else if (param == DOCTYPE_SQL || param == DOCTYPE_REDIS)
        {   // sql query
            on_view_result_show(p, VK_CONTROL);
        }
        else
        {   // 预设动作
            LOAD_I18N_RESSTR(IDS_EXTRA_PATH, m_input);
            TCHAR process[MAX_PATH] = _T("./share/example_build.bat");
            if (util_under_wine())
            {
                _sntprintf(process, MAX_PATH - 1, _T("%s"), _T("./share/example_build.sh"));
            }
            else
            {
                _sntprintf(process, MAX_PATH - 1, _T("%s"), _T("./share/example_build.bat"));
            }
            if (!eu_input(m_input, process, MAX_PATH - 1))
            {
                return;
            }
            int len = (int)_tcslen(process);
            if (len > 1 && len < MAX_PATH && util_make_u8(util_path2unix(process, len), eu_get_config()->m_actions[param], MAX_PATH-1))
            {
                util_update_env(p);
                CloseHandle((HANDLE) _beginthreadex(NULL, 0, do_extra_actions, (void *)param, 0, NULL));
            }
        }
    }
}

static void
on_toolbar_menu_callback(HMENU hpop, void *param)
{
    UNREFERENCED_PARAMETER(param);
    if (hpop)
    {
        util_set_menu_item(hpop, IDM_VIEW_MENUBAR, eu_get_config()->m_menubar);
        util_set_menu_item(hpop, IDM_VIEW_TOOLBAR, eu_get_config()->m_toolbar != IDB_SIZE_0);
        util_set_menu_item(hpop, IDM_VIEW_STATUSBAR, eu_get_config()->m_statusbar);
    }
}

/*****************************************************************
 * 工具栏回调函数, 接受工具栏点击消息, 以及销毁自身资源
 *****************************************************************/
LRESULT CALLBACK
toolbar_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WNDPROC old = (WNDPROC)GetWindowLongPtr(hwnd,GWLP_USERDATA);
    if (!old)
    {
        return 0;
    }
    switch (msg)
    {
        case WM_RBUTTONUP:
        {
            return menu_pop_track(hwnd, IDR_TOOLBAR_POPUPMENU, 0, -1, on_toolbar_menu_callback, NULL);
        }
        case WM_SIZE:
        {
            on_toolbar_setpos_clipdlg(g_clip_hwnd, eu_module_hwnd());
            break;
        }
        case WM_DESTROY:
        {
            if (img_list1)
            {
                ImageList_Destroy(img_list1);
                img_list1 = NULL;
            }
            if (img_list2)
            {
                ImageList_Destroy(img_list2);
                img_list2 = NULL;
            }
            break;
        }
        default:
            break;
    }
    return CallWindowProc(old,hwnd,msg,wParam,lParam);
}

static bool
is_exec_file(eu_tabpage *pnode)
{
    if (!(pnode && pnode->doc_ptr))
    {
        return false;
    }
    return (pnode->doc_ptr->doc_type == DOCTYPE_LUA ||
            pnode->doc_ptr->doc_type == DOCTYPE_SQL ||
            pnode->doc_ptr->doc_type == DOCTYPE_REDIS);
}

void
on_toolbar_cmd_start(eu_tabpage *pnode)
{
    if (pnode && eu_get_config())
    {
        bool wine = false;
        HANDLE handle = NULL;
        WCHAR *plugin = NULL;
        TCHAR pcd[MAX_BUFFER] = {0};
        TCHAR cmd_exec[MAX_BUFFER] = _T("cmd.exe");
        if (pnode->is_blank || url_has_remote(pnode->pathfile))
        {
            GetEnvironmentVariable(_T("USERPROFILE"), pcd, MAX_BUFFER);
        }
        else
        {
            _tcsncpy(pcd, pnode->pathname, MAX_BUFFER);
        }
        if (eu_get_config()->m_path[0])
        {
            *cmd_exec = 0;
            MultiByteToWideChar(CP_UTF8, 0, eu_get_config()->m_path, -1, cmd_exec, MAX_BUFFER);
        }
        if ((wine = util_under_wine()) && (plugin = util_winexy_get()))
        {
            TCHAR unix_path[MAX_PATH] = {0};
            if (strlen(eu_get_config()->m_path) > 1 && (eu_get_config()->m_path[0] == '/') && (eu_get_config()->m_path[1] != '/'))
            {
                util_make_u16(eu_get_config()->m_path, unix_path, MAX_PATH - 1);
            }
            _sntprintf(cmd_exec, MAX_BUFFER, _T("%s \"%s\""), plugin, unix_path[0] ? unix_path : L"x-terminal-emulator");
            free(plugin);
        }
        if ((handle = eu_new_process(cmd_exec, NULL, pcd, 2, NULL)) != NULL)
        {
            CloseHandle(handle);
        }
        else if (!wine)
        {
            MSG_BOX(IDC_MSG_EXEC_ERR1, IDC_MSG_ERROR, MB_ICONERROR|MB_OK);
        }
    }
}

int
on_toolbar_height(void)
{
    return g_toolbar_height;
}

void
on_toolbar_update_button(void)
{
    if (eu_get_config() && eu_get_config()->m_toolbar != IDB_SIZE_0)
    {
        eu_tabpage *pnode = on_tabpage_focus_at();
        if (pnode && pnode->hwnd_sc)
        {
            on_toolbar_setup_button(IDM_FILE_SAVE, on_sci_doc_modified(pnode) && !(pnode->file_attr & FILE_ATTRIBUTE_READONLY) ? 2 : 1);
            on_toolbar_setup_button(IDM_FILE_SAVEAS, 2);
            on_toolbar_setup_button(IDM_FILE_CLOSE, 2);
            on_toolbar_setup_button(IDM_FILE_PRINT, 2);
            on_toolbar_setup_button(IDM_EDIT_CUT, !pnode->pmod && (pnode->hex_mode || util_can_selections(pnode)) ? 2 : 1);
            on_toolbar_setup_button(IDM_EDIT_COPY, !pnode->pmod && TAB_NOT_NUL(pnode) ? 2 : 1);
            on_toolbar_setup_button(IDM_EDIT_PASTE, !pnode->pmod ? 2 : 1);
            on_toolbar_setup_button(IDM_SEARCH_FIND, !pnode->pmod ? 2 : 1);
            on_toolbar_setup_button(IDM_SEARCH_FINDPREV, !pnode->pmod ? 2 : 1);
            on_toolbar_setup_button(IDM_SEARCH_FINDNEXT, !pnode->pmod ? 2 : 1);
            on_toolbar_setup_button(IDM_EDIT_UNDO, !pnode->pmod && eu_sci_call(pnode, SCI_CANUNDO, 0, 0) ? 2 : 1);
            on_toolbar_setup_button(IDM_EDIT_REDO, !pnode->pmod && eu_sci_call(pnode, SCI_CANREDO, 0, 0) ? 2 : 1);
            on_toolbar_setup_button(IDM_SEARCH_TOGGLE_BOOKMARK, !pnode->pmod && !pnode->hex_mode ? 2 : 1);
            on_toolbar_setup_button(IDM_SEARCH_GOTO_PREV_BOOKMARK, !pnode->pmod && !pnode->hex_mode ? 2 : 1);
            on_toolbar_setup_button(IDM_SEARCH_GOTO_NEXT_BOOKMARK, !pnode->pmod && !pnode->hex_mode ? 2 : 1);
            on_toolbar_setup_button(IDM_VIEW_HEXEDIT_MODE, (pnode->codepage != IDM_OTHER_BIN) && TAB_NOT_NUL(pnode) ? 2 : 1);
            on_toolbar_setup_button(IDM_VIEW_SYMTREE, (pnode->hwnd_symlist || pnode->hwnd_symtree) ? 2 : 1);
            on_toolbar_setup_button(IDM_VIEW_FULLSCREEN, 2);
            on_toolbar_setup_button(IDM_FILE_REMOTE_FILESERVERS, util_exist_libcurl() ? 2 : 1);
            on_toolbar_setup_button(IDM_VIEW_ZOOMIN, 2);
            on_toolbar_setup_button(IDM_VIEW_ZOOMOUT, 2);
            on_toolbar_setup_button(IDM_SCRIPT_EXEC, (!pnode->hex_mode && pnode->doc_ptr) ? 2 : 1);
        }
    }
}

void
on_toolbar_redraw(HWND hwnd)
{
    HWND h_tool = NULL;
    if ((h_tool = GetDlgItem(hwnd, IDC_TOOLBAR)))
    {
        UpdateWindowEx(h_tool);
    }
}

bool
on_toolbar_refresh(HWND hwnd)
{
    HWND h_tool = NULL;
    if ((h_tool = GetDlgItem(hwnd, IDC_TOOLBAR)))
    {
        DestroyWindow(h_tool);
    }
    refresh_clipboard();  // dpi改变时重新渲染图标
    return (on_toolbar_create(hwnd) == 0);
}

int
on_toolbar_create(HWND parent)
{
    int ret = 0;
    HWND htool = NULL;
    intptr_t tool_proc = 0;
    TBBUTTON *ptb = NULL;
    toolbar_data *pdata = NULL;
    TCHAR (*str)[BUFFSIZE] = NULL;
    const int tb_size = on_pixmap_vec_count();
    const int m_svg = on_pixmap_svg_count();
    do
    {
        int i = 0, j = 0;
        const int style = WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|TBSTYLE_TOOLTIPS|TBSTYLE_FLAT|TBSTYLE_CUSTOMERASE|CCS_TOP|CCS_NODIVIDER;
        /*********************************************************************
         * iBitmap(0), 第i个位图
         * idCommand(0), WM_COMMAND消息响应的ID
         * fsState(TBSTATE_ENABLED), 按钮状态,可用或不可用
         * fsStyle(TBSTYLE_BUTTON, TBSTYLE_SEP), 按钮风格
         * {0} 保留
         * dwData(0) 应用定义的值
         * iString(0) 鼠标指向时显示的字符串
         *********************************************************************/
        if (eu_get_config()->m_toolbar == IDB_SIZE_0)
        {
            break;
        }
        if (!m_svg)
        {
            break;
        }
        if ((str = (TCHAR (*)[BUFFSIZE])calloc(1, m_svg * sizeof(str[0]))) == NULL)
        {
            ret = 1;
            break;
        }
        if ((ptb = (TBBUTTON *)calloc(1, tb_size * sizeof(TBBUTTON))) == NULL)
        {
            ret = 1;
            break;
        }
        on_toolbar_init_params(&pdata, eu_get_config()->m_toolbar);
        if (!pdata)
        {
            ret = 1;
            break;
        }
        if ((img_list1 = create_img_list(pdata, true)) == NULL)
        {
            ret = 1;
            break;
        }
        if ((img_list2 = create_img_list(pdata, false)) == NULL)
        {
            ret = 1;
            break;
        }
        for (eue_toolbar* p = eu_get_toolbar(); p && p->imsg != -1; ++p, ++i)
        {
            if (p->icmd > 0 && p->isvg[0])
            {
                ptb[i].iBitmap = j;
                ptb[i].idCommand = p->icmd;
                ptb[i].fsState = TBSTATE_ENABLED;
                ptb[i].fsStyle = TBSTYLE_BUTTON;
                if (!LoadString(g_skylark_lang, p->imsg, str[j], BUFFSIZE))
                {
                    *str[j] = 0;
                }
                ptb[i].iString = (intptr_t) str[j];
                ++j;
            }
            else
            {
                ptb[i].iBitmap = 0;
                ptb[i].idCommand = 0;
                ptb[i].fsState = TBSTATE_ENABLED;
                ptb[i].fsStyle = TBSTYLE_SEP;
            }
        }
        htool = CreateWindowEx(WS_EX_PALETTEWINDOW, //TBSTYLE_EX_DOUBLEBUFFER
                               TOOLBARCLASSNAME,
                               _T(""),
                               style,
                               0,
                               0,
                               0,
                               0,
                               parent,
                               (HMENU) IDC_TOOLBAR,
                               eu_module_handle(),
                               NULL);
        if (!htool)
        {
            ret = 1;
            break;
        }
        if (!(tool_proc = (intptr_t)SetWindowLongPtr(htool, GWLP_WNDPROC, (intptr_t)toolbar_proc)))
        {
            eu_logmsg("%s: SetWindowLongPtr(htool) failed\n", __FUNCTION__);
            ret = 1;
            break;
        }
        SetWindowLongPtr(htool, GWLP_USERDATA, tool_proc);
        SendMessage(htool, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0);
        SendMessage(htool, TB_ADDBUTTONS, (WPARAM) tb_size, (LPARAM)ptb);
        SendMessage(htool, TB_SETIMAGELIST, (WPARAM) 0, (LPARAM) img_list1);
        SendMessage(htool, TB_SETDISABLEDIMAGELIST, (WPARAM) 0, (LPARAM) img_list2);
        SendMessage(htool, TB_SETMAXTEXTROWS, 0, 0);
        SendMessage(htool, TB_AUTOSIZE, 0, 0);
        on_dark_tips_theme(htool, TB_GETTOOLTIPS);
    } while(0);
    free(str);
    eu_safe_free(ptb);
    eu_safe_free(pdata);
    return ret;
}
