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

#include <fcntl.h>
#include "targetver.h"
#include "framework.h"

#define DPI_PERCENT_LARGE 168
#define CHECK_IF(a) if ((a)!= 0) return false

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

/**********************************************
 * 设置工具栏按钮的状态,
 * id 为 资源id号
 * flags == 0, 自动翻转
 * flags == 1, 设为禁止状态
 * flags == 2. 设为启用状态
 **********************************************/
void WINAPI
on_toolbar_setup_button(int id, int flags)
{
    LRESULT status = 0;
    HWND h_tool = GetDlgItem(eu_module_hwnd(), IDC_TOOLBAR);
    if (h_tool)
    {
        status = SendMessage(h_tool, TB_GETSTATE, id, 0);
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

/***********************************************************
 * 获得系统剪贴板数据, 调用后需要释放指针
 ***********************************************************/
bool WINAPI
on_toolbar_get_clipboard(char **ppstr)
{
    bool ret = false;
    if (!ppstr)
    {
        return ret;
    }
    if (OpenClipboard(NULL))
    {
        HGLOBAL hmem = GetClipboardData(CF_UNICODETEXT);
        if (NULL != hmem)
        {
            wchar_t *lpstr = (wchar_t *) GlobalLock(hmem);
            if (NULL != lpstr)
            {
                if ((*ppstr = eu_utf16_utf8(lpstr, NULL)) != NULL)
                {
                    ret = true;
                }
                GlobalUnlock(hmem);
            }
        }
        CloseClipboard();
    }
    return ret;
}

HWND WINAPI
on_toolbar_hwnd(void)
{
    HWND hwnd = eu_module_hwnd();
    HWND htool = hwnd ? GetDlgItem(hwnd, IDC_TOOLBAR) : NULL;
    return htool;
}

void WINAPI
on_toolbar_adjust_box(void)
{
    if (!eu_get_config()->m_toolbar)
    {
        g_toolbar_height = 0;
    }
    else
    {
        g_toolbar_height = eu_get_dpi(NULL) >= DPI_PERCENT_LARGE ? TB_LARGE_SIZE : TB_NORMAL_SIZE;
    }
}

bool WINAPI
on_toolbar_setpos_clipdlg(HWND hwnd, HWND hparent)
{
    RECT rc, rcparent;
    int width;
    int height;
    if (!IsWindow(hwnd) || !IsWindow(hparent))
    {
        return false;
    }
    GetWindowRect(hwnd, &rc);
    GetWindowRect(hparent, &rcparent);
    width = rc.right - rc.left;
    height = rc.bottom - rc.top;
    eu_setpos_window(hwnd, HWND_TOPMOST, rcparent.right - width - 20, rcparent.bottom - height - on_statusbar_height() - 2, width, height, 0);
    return true;
}

static bool
init_clip_dlg(HWND dialog)
{
    HICON hicon = NULL;
    LPCTSTR name = NULL;
    if (!on_dark_supports())
    {
        name = MAKEINTRESOURCE(IDB_COPY);
    }
    else
    {
        name = MAKEINTRESOURCE(IDB_COPY_DARK);
    }
    if (!(hicon = (HICON) LoadImage(eu_module_handle(), name, IMAGE_ICON, 16, 16, 0)))
    {
        return false;
    }
    // 在静态控件上加载图标.
    for (int i = 0; i < _countof(m_edit); ++i)
    {
        Static_SetIcon(GetDlgItem(dialog, IDC_BUTTON0 + i), hicon);
        m_edit[i] = GetDlgItem(dialog, IDC_EDIT0 + i);
    }
    DeleteObject(hicon);
    SetLastError(0);
    m_chain = SetClipboardViewer(dialog);
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
    m_block[i] = !m_block[i];
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

static intptr_t CALLBACK
clip_proc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
        {
            HICON m_icon = LoadIcon(eu_module_handle(), MAKEINTRESOURCE(IDI_SMALL));
            if (m_icon)
            {
                SetClassLongPtr(hdlg, GCLP_HICONSM, (LONG_PTR)m_icon);
            }
            if (!init_clip_dlg(hdlg))
            {
                return EndDialog(hdlg, 0);
            }
            if (on_dark_enable())
            {
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
                    on_dark_set_theme(btn, L"Explorer", NULL);
                }
                on_dark_set_theme(hdlg, L"Explorer", NULL);
            }
            return 1;
        }
        case WM_THEMECHANGED:
        {
            if (on_dark_enable())
            {
                on_dark_allow_window(hdlg, true);
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
                    on_dark_allow_window(btn, true);
                    SendMessage(btn, WM_THEMECHANGED, 0, 0);
                }
                UpdateWindow(hdlg);
            }
            break;
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
            if ((HWND) wParam == m_chain)
            {
                m_chain = (HWND) lParam;
            }
            else
            {
                SendMessage(m_chain, message, wParam, lParam);
            }
            break;
        case WM_NOTIFY:
            break;
        case WM_SHOWWINDOW:
        {
            if (!IsWindowVisible(hdlg))
            {
                printf("we get WM_SHOWWINDOW\n");
            }
            break;
        }
        case WM_CLOSE:
        {
            ShowWindow(hdlg, SW_HIDE);
            return 1;
        }
        case WM_DESTROY:
        {
            ChangeClipboardChain(hdlg, m_chain);
            m_chain = NULL;
            g_clip_hwnd = NULL;
            EndDialog(hdlg, LOWORD(wParam));
            printf("clip window WM_DESTROY\n");
            return 1;
        }
        default:
            break;
    }
    return 0;
}

HWND WINAPI
on_toolbar_clip_hwnd(void)
{
    return g_clip_hwnd;
}

static bool
create_clipbox(void)
{
    g_clip_hwnd = i18n_create_dialog(eu_module_hwnd(), IDD_DIALOG_CLIP, clip_proc);
    if (!g_clip_hwnd)
    {
        printf("creator DialogBox failed, cause: %lu\n", GetLastError());
        return false;
    }
    return EnableWindow(g_clip_hwnd, true);
}

static HIMAGELIST
create_img_list(INT res_id, UINT mask, bool zoom)
{
    HIMAGELIST himg = NULL;
    HBITMAP hbmp = NULL;
    BITMAP bm = { 0 };
    int width = zoom ? IMAGE_LARGE_WIDTH : IMAGE_NORMAL_WIDTH;
    int height = zoom ? IMAGE_LARGE_HEIGHT : IMAGE_NORMAL_HEIGHT;
    hbmp = (HBITMAP) LoadImage(eu_module_handle(), MAKEINTRESOURCE(res_id), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_DEFAULTSIZE);
    GetObject(hbmp, sizeof(BITMAP), &bm);
    himg = ImageList_Create(width, height, bm.bmBitsPixel | ILC_MASK, bm.bmWidth / width, 1);
    ImageList_AddMasked(himg, hbmp, mask);
    if (hbmp != NULL)
    {
        DeleteObject((HGDIOBJ) hbmp);
    }
    return himg;
}

static uint32_t
where_mouse(HWND hwnd, POINT *lpt)
{
    RECT rect;
    uint32_t ret = 0;
    for (uint32_t i = IDM_EDIT_CLIP; i < IDB_TOOLBAR1; ++i)
    {
        if (!SendMessage(hwnd, TB_GETRECT, (WPARAM)i, (LPARAM)&rect))
        {
            break;
        }
        if (PtInRect(&rect, *lpt))
        {
            ret = i;
            break;
        }
    }
    return ret;
}

static void
on_cmd_start(void)
{
    HANDLE handle = NULL;
    TCHAR cmd_exec[MAX_PATH +1] = _T("cmd.exe");
    if (eu_get_config()->m_path[0])
    {
        *cmd_exec = 0;
        MultiByteToWideChar(CP_UTF8, 0, eu_get_config()->m_path, -1, cmd_exec, MAX_PATH);
    }
    if ((handle = eu_new_process(cmd_exec, NULL, NULL, 2, NULL)) != NULL)
    {
        CloseHandle(handle);
    }
    else
    {
        MSG_BOX(IDC_MSG_EXEC_ERR1, IDC_MSG_ERROR, MB_ICONERROR|MB_OK);
    }
}

static bool
close_stdout_redirect(FILE *console)
{
    CHECK_IF(_dup2(fd_stdout, _fileno(stdout)));
    _close(fd_stdout);
    _close(fd_pipe[WRITE_FD]);
    _close(fd_pipe[READ_FD]);
    safe_close_console(console);
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
    if ((_fileno(stdout) != 1) && AllocConsole())
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
        fd_stdout = _dup(_fileno(stdout));
        fflush(stdout);
        if ((_dup2(fd_pipe[WRITE_FD], _fileno(stdout))) == 0)
        {
            setvbuf(stdout, NULL, _IONBF, 0);
            ret = true;
        }
    }while(0);
    if (!ret)
    {
        safe_close_console(*pconsole);
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

void WINAPI
on_toolbar_lua_exec(eu_tabpage *pnode)
{
    char *buffer = NULL;
    FILE *console = NULL;
    if (pnode && pnode->doc_ptr && pnode->hwnd_qredit)
    {
        if ((buffer = util_strdup_content(pnode, NULL)) && init_stdout_redirect(MAX_OUTPUT_BUF, &console))
        {
            int read_len = 0;
            char *std_buffer = (char *)calloc(1, MAX_OUTPUT_BUF+1);
            if (std_buffer)
            {
                if (do_lua_code((const char *)buffer) == 0)
                {
                    read_len = get_output_buffer(std_buffer, MAX_OUTPUT_BUF);
                }
                else
                {
                    // 写标准输出设备, 防止_read函数阻塞
                    fprintf(stdout, "Failed to execute Lua script\n");
                }
                close_stdout_redirect(console);
                if (read_len > 0)
                {
                    char *pstr = util_unix_newline(std_buffer, MAX_OUTPUT_BUF);
                    TCHAR *result = pstr? eu_utf8_utf16(pstr,  NULL) : NULL;
                    if (result)
                    {
                        on_result_append_text(NULL, _T("%s"), result);
                        free(result);
                    }
                    if (pstr)
                    {
                        free(pstr);
                    }
                }
                free(std_buffer);
                pnode->edit_show = true;
                pnode->result_show = false;
                eu_window_resize(NULL);
            }
            free(buffer);
        }
    }
}

static unsigned __stdcall
do_extra_actions(void *lp)
{
    eu_tabpage *p = on_tabpage_focus_at();
    if (p && p->doc_ptr)
    {
        char *pactions = eu_get_config()->m_actions[p->doc_ptr->doc_type];
        if (strlen(pactions) > 0)
        {
            WCHAR *abs_path = util_to_abs(pactions);
            if (abs_path)
            {
                DWORD written;
                HANDLE pfile = NULL;
                char *pbuffer = NULL;
                uint32_t buf_len = 0;
                TCHAR cmd_exec[MAX_BUFFER] = {0};
                TCHAR pname[MAX_PATH+1] = {0};
                if ((pfile = util_mk_temp(pname, on_doc_get_ext(p))) == INVALID_HANDLE_VALUE)
                {
                    printf("util_mk_temp return failed, cause:%lu\n", GetLastError());
                    free(abs_path);
                    return 1;
                }
                if (!(pbuffer = util_strdup_content(p, (size_t *)&buf_len)))
                {
                    CloseHandle(pfile);
                    free(abs_path);
                    return 1;
                }
                WriteFile(pfile, pbuffer, buf_len, &written, NULL);
                CloseHandle(pfile);
                _sntprintf(cmd_exec, MAX_BUFFER - 1, _T("%s %s"), abs_path, pname);
                free(abs_path);
                HANDLE handle = eu_new_process(cmd_exec, NULL, NULL, 2, NULL);
                if (handle)
                {
                    WaitForSingleObject(handle, INFINITE);
                    CloseHandle(handle);
                }
                else
                {
                    *pactions = 0;
                    MSG_BOX(IDC_MSG_EXEC_ERR1, IDC_MSG_ERROR, MB_ICONERROR|MB_OK);
                }
                DeleteFile(pname);
            }
        }
    }
    return 0;
}

void WINAPI
on_toolbar_execute_script(void)
{
    eu_tabpage *p = on_tabpage_focus_at();
    if (p && !p->hex_mode && p->doc_ptr)
    {
        if (strlen(eu_get_config()->m_actions[p->doc_ptr->doc_type]) > 1)
        {
            CloseHandle((HANDLE) _beginthreadex(NULL, 0, do_extra_actions, NULL, 0, NULL));
        }
        else if (p->doc_ptr->doc_type == DOCTYPE_LUA)
        {   // lua script
            on_toolbar_lua_exec(p);
        }
        else if (p->doc_ptr->doc_type == DOCTYPE_SQL || p->doc_ptr->doc_type == DOCTYPE_REDIS)
        {   // sql query
            on_view_result_show(p, VK_CONTROL);
        }
        else
        {   // 预设动作
            LOAD_I18N_RESSTR(IDS_EXTRA_PATH, m_input);
            TCHAR process[MAX_PATH] = _T("./share/example_build.bat");
            if (!eu_input(m_input, process, MAX_PATH - 1))
            {
                return;
            }
            int len = (int)_tcslen(process);
            if (len > 1 && len < MAX_PATH)
            {
                WideCharToMultiByte(CP_UTF8, 0, process, -1, eu_get_config()->m_actions[p->doc_ptr->doc_type], MAX_PATH-1, NULL, NULL);
                CloseHandle((HANDLE) _beginthreadex(NULL, 0, do_extra_actions, NULL, 0, NULL));
            }
        }
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
        case WM_LBUTTONUP:
        {
            POINT pt;
            uint32_t resid = 0;
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);
            if (!(resid = where_mouse(hwnd, &pt)))
            {
                break;
            }
            switch (resid)
            {
                case IDM_EDIT_CLIP:
                    if (!IsWindowVisible(g_clip_hwnd))
                    {
                        ShowWindow(g_clip_hwnd, SW_SHOW);
                        on_toolbar_setpos_clipdlg(g_clip_hwnd, eu_module_hwnd());
                    }
                    else
                    {
                        ShowWindow(g_clip_hwnd, SW_HIDE);
                    }
                    break;
                case IDM_FILE_PRINT:
                {
                    eu_tabpage *p = on_tabpage_focus_at();
                    if (p)
                    {
                        on_print_file(p);
                    }
                    break;
                }
                case IDM_SCRIPT_EXEC:
                {
                    on_toolbar_execute_script();
                    break;
                }
                case IDM_CMD_TAB:
                {   // 当前目录打开shell
                    TCHAR *pold = NULL;
                    eu_tabpage *p = on_tabpage_focus_at();
                    if (p)
                    {
                        util_set_working_dir(p->pathname, &pold);
                    }                    
                    on_cmd_start();
                    if (pold)
                    {
                        SetCurrentDirectory(pold);
                        free(pold);
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case WM_RBUTTONUP:
        {
            POINT pt;
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);
            ClientToScreen(hwnd, &pt);
            HMENU hpop = menu_load(IDR_TOOLBAR_POPUPMENU);
            if (hpop)
            {
                util_set_menu_item(hpop, IDM_VIEW_MENUBAR, eu_get_config()->m_menubar);
                util_set_menu_item(hpop, IDM_VIEW_TOOLBAR, eu_get_config()->m_toolbar);
                util_set_menu_item(hpop, IDM_VIEW_STATUSBAR, eu_get_config()->m_statusbar);
                TrackPopupMenu(hpop, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
                DestroyMenu(hpop);
            }
            return 1;
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
            printf("toolbar WM_DESTROY\n");
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

static void
on_toolbar_dark_tips(HWND hwnd)
{
    if (on_dark_enable())
    {
        HWND htips = (HWND) SendMessage(hwnd, TB_GETTOOLTIPS, 0, 0);
        if (NULL != htips)
        {
            on_dark_set_theme(htips, L"DarkMode_Explorer", NULL);
        }
    }
}

int WINAPI
on_toolbar_height(void)
{
    return g_toolbar_height;
}

void WINAPI
on_toolbar_update_button(void)
{
    if (eu_get_config() && eu_get_config()->m_toolbar)
    {
        eu_tabpage *pnode = on_tabpage_focus_at();
        if (pnode && pnode->hwnd_sc)
        {
            on_toolbar_setup_button(IDM_FILE_SAVE, on_sci_doc_modified(pnode) && !eu_sci_call(pnode, SCI_GETREADONLY, 0, 0)?2:1);
            on_toolbar_setup_button(IDM_FILE_SAVEAS, 2);
            on_toolbar_setup_button(IDM_FILE_CLOSE, 2);
            on_toolbar_setup_button(IDM_FILE_PRINT, 2);
            on_toolbar_setup_button(IDM_EDIT_PASTE, eu_sci_call(pnode,SCI_CANPASTE, 0, 0)?2:1);
            on_toolbar_setup_button(IDM_SEARCH_FIND, 2);
            on_toolbar_setup_button(IDM_SEARCH_FINDPREV, 2);
            on_toolbar_setup_button(IDM_SEARCH_FINDNEXT, 2);
            on_toolbar_setup_button(IDM_EDIT_UNDO, eu_sci_call(pnode,SCI_CANUNDO, 0, 0)?2:1);
            on_toolbar_setup_button(IDM_EDIT_REDO, eu_sci_call(pnode,SCI_CANREDO, 0, 0)?2:1);
            on_toolbar_setup_button(IDM_SEARCH_TOGGLE_BOOKMARK, !pnode->hex_mode?2:1);
            on_toolbar_setup_button(IDM_SEARCH_GOTO_PREV_BOOKMARK, !pnode->hex_mode?2:1);
            on_toolbar_setup_button(IDM_SEARCH_GOTO_NEXT_BOOKMARK, !pnode->hex_mode?2:1);
            on_toolbar_setup_button(IDM_VIEW_HEXEDIT_MODE, (pnode->codepage != IDM_OTHER_BIN)?2:1);
            on_toolbar_setup_button(IDM_VIEW_SYMTREE, (pnode->hwnd_symlist || pnode->hwnd_symtree)?2:1);
            on_toolbar_setup_button(IDM_VIEW_FULLSCREEN, 2);
            on_toolbar_setup_button(IDM_SCRIPT_EXEC, (!pnode->hex_mode && pnode->doc_ptr)?2:1);
            on_toolbar_setup_button(IDM_FILE_REMOTE_FILESERVERS, util_exist_libcurl()?2:1);
        }
    }
}

bool WINAPI
on_toolbar_refresh(HWND hwnd)
{
    HWND h_tool = NULL;
    if (g_clip_hwnd)
    {
        DestroyWindow(g_clip_hwnd);
    }
    if ((h_tool = GetDlgItem(hwnd, IDC_TOOLBAR)))
    {
        DestroyWindow(h_tool);
        return on_toolbar_create(hwnd);
    }
    return false;
}

int WINAPI
on_toolbar_create(HWND parent)
{
    int tb_size = 0;
    HWND htool = NULL;
    intptr_t tool_proc = 0;
    TCHAR str[28][BUFFSIZE] = { 0 };
    /*********************************************************************
     * iBitmap(0), 第i个位图
     * idCommand(0), WM_COMMAND消息响应的ID
     * TBSTATE_ENABLED, 按钮状态,可用或不可用
     * TBSTYLE_SEP, 按钮风格
     * {0} 保留
     * dwData(0) 应用定义的值
     * iString(0) 鼠标指向时显示的字符串
     *********************************************************************/
    TBBUTTON tb_cmd[] = {
        { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, { 0 }, 0, 0 },
        { 0, IDM_FILE_NEW, TBSTATE_ENABLED, TBSTYLE_BUTTON, { 0 }, 0, 0 },
        { 1, IDM_FILE_OPEN, TBSTATE_ENABLED, TBSTYLE_BUTTON, { 0 }, 0, 0 },
        { 2, IDM_FILE_SAVE, TBSTATE_ENABLED, TBSTYLE_BUTTON, { 0 }, 0, 0 },
        { 3, IDM_FILE_SAVEAS, TBSTATE_ENABLED, TBSTYLE_BUTTON, { 0 }, 0, 0 },
        { 4, IDM_FILE_CLOSE, TBSTATE_ENABLED, TBSTYLE_BUTTON, { 0 }, 0, 0 },
        { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, { 0 }, 0, 0 },
        { 5, IDM_FILE_PRINT, TBSTATE_ENABLED, TBSTYLE_BUTTON, { 0 }, 0, 0 },
        { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, { 0 }, 0, 0 },
        { 6, IDM_FILE_REMOTE_FILESERVERS, TBSTATE_ENABLED, TBSTYLE_BUTTON, { 0 }, 0, 0 },
        { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, { 0 }, 0, 0 },
        { 7, IDM_EDIT_CUT, TBSTATE_ENABLED, TBSTYLE_BUTTON, { 0 }, 0, 0 },
        { 8, IDM_EDIT_COPY, TBSTATE_ENABLED, TBSTYLE_BUTTON, { 0 }, 0, 0 },
        { 9, IDM_EDIT_PASTE, TBSTATE_ENABLED, TBSTYLE_BUTTON, { 0 }, 0, 0 },
        { 10, IDM_EDIT_CLIP, TBSTATE_ENABLED, TBSTYLE_BUTTON, { 0 }, 0, 0 },
        { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, { 0 }, 0, 0 },
        { 11, IDM_SEARCH_FIND, TBSTATE_ENABLED, TBSTYLE_BUTTON, { 0 }, 0, 0 },
        { 12, IDM_SEARCH_FINDPREV, TBSTATE_ENABLED, TBSTYLE_BUTTON, { 0 }, 0, 0 },
        { 13, IDM_SEARCH_FINDNEXT, TBSTATE_ENABLED, TBSTYLE_BUTTON, { 0 }, 0, 0 },
        { 14, IDM_EDIT_UNDO, TBSTATE_ENABLED, TBSTYLE_BUTTON, { 0 }, 0, 0 },
        { 15, IDM_EDIT_REDO, TBSTATE_ENABLED, TBSTYLE_BUTTON, { 0 }, 0, 0 },
        { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, { 0 }, 0, 0 },
        { 16, IDM_SEARCH_TOGGLE_BOOKMARK, TBSTATE_ENABLED, TBSTYLE_BUTTON, { 0 }, 0, 0 },
        { 17, IDM_SEARCH_GOTO_PREV_BOOKMARK, TBSTATE_ENABLED, TBSTYLE_BUTTON, { 0 }, 0, 0 },
        { 18, IDM_SEARCH_GOTO_NEXT_BOOKMARK, TBSTATE_ENABLED, TBSTYLE_BUTTON, { 0 }, 0, 0 },
        { 19, IDM_VIEW_HEXEDIT_MODE, TBSTATE_ENABLED, TBSTYLE_BUTTON, { 0 }, 0, 0 },
        { 20, IDM_VIEW_FILETREE, TBSTATE_ENABLED, TBSTYLE_BUTTON, { 0 }, 0, 0 },
        { 21, IDM_VIEW_SYMTREE, TBSTATE_ENABLED, TBSTYLE_BUTTON, { 0 }, 0, 0 },
        { 22, IDM_VIEW_MODIFY_STYLETHEME, TBSTATE_ENABLED, TBSTYLE_BUTTON, { 0 }, 0, 0 },
        { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, { 0 }, 0, 0 },
        { 23, IDM_VIEW_ZOOMOUT, TBSTATE_ENABLED, TBSTYLE_BUTTON, { 0 }, 0, 0 },
        { 24, IDM_VIEW_ZOOMIN, TBSTATE_ENABLED, TBSTYLE_BUTTON, { 0 }, 0, 0 },
        { 25, IDM_VIEW_FULLSCREEN, TBSTATE_ENABLED, TBSTYLE_BUTTON, { 0 }, 0, 0 },
        { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, { 0 }, 0, 0 },
        { 26, IDM_SCRIPT_EXEC, TBSTATE_ENABLED, TBSTYLE_BUTTON, { 0 }, 0, 0 },
        { 27, IDM_CMD_TAB, TBSTATE_ENABLED, TBSTYLE_BUTTON, { 0 }, 0, 0 },
        { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, { 0 }, 0, 0 },
    };
    const bool zoom = eu_get_dpi(parent) >= DPI_PERCENT_LARGE ? true : false;
    if (!on_dark_enable())
    {
        if ((img_list1 = create_img_list(zoom ? IDB_TOOLBAR_LARGE32 : IDB_TOOLBAR16, RGB(0xF0, 0xF0, 0xF0), zoom)) == NULL)
        {
            return 1;
        }
        if ((img_list2 = create_img_list(zoom ? IDB_TOOLBAR_LARGE1 : IDB_TOOLBAR1, RGB(0xF0, 0xF0, 0xF0), zoom)) == NULL)
        {
            return 1;
        }
    }
    else
    {
        if ((img_list1 = create_img_list(zoom ? IDB_DARK_LARGE32 : IDB_DARK16, RGB(0xF0, 0xF0, 0xF0), zoom)) == NULL)
        {
            return 1;
        }
        if ((img_list2 = create_img_list(zoom ? IDB_DARK_LARGE1 : IDB_DARK1, RGB(0xF0, 0xF0, 0xF0), zoom)) == NULL)
        {
            return 1;
        }
    }
    tb_size = sizeof(tb_cmd) / sizeof(TBBUTTON);
    for (int i = 0, j = 0; i < tb_size; ++i)
    {
        if (tb_cmd[i].idCommand)
        {
            if (!LoadString(g_skylark_lang, IDS_TOOLBAR_0 + j, str[j], BUFFSIZE))
            {
                continue;
            }
            tb_cmd[i].iString = (intptr_t) str[j];
            ++j;
        }
    }
    const int style = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TBSTYLE_TOOLTIPS |TBSTYLE_FLAT |TBSTYLE_CUSTOMERASE | CCS_TOP | CCS_NODIVIDER;
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
        return 1;
    }
    if (!(tool_proc = (intptr_t)SetWindowLongPtr(htool, GWLP_WNDPROC, (intptr_t)toolbar_proc)))
    {
        printf("SetWindowLongPtr(htool) failed\n");
        return 1;
    }
    SetWindowLongPtr(htool, GWLP_USERDATA, tool_proc);
    SendMessage(htool, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0);
    SendMessage(htool, TB_ADDBUTTONS, (WPARAM) tb_size, (LPARAM) &tb_cmd);
    SendMessage(htool, TB_SETIMAGELIST, (WPARAM) 0, (LPARAM) img_list1);
    SendMessage(htool, TB_SETDISABLEDIMAGELIST, (WPARAM) 0, (LPARAM) img_list2);
    SendMessage(htool, TB_SETMAXTEXTROWS, 0, 0);
    SendMessage(htool, TB_AUTOSIZE, 0, 0);
    on_toolbar_dark_tips(htool);
    // 剪贴板窗口
    return !create_clipbox();
}
