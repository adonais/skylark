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

#define LEN_TXT 384

#define FILE_KEEP_YES 1
#define FILE_KEEP_NO 2
#define FILE_KEEP_ALL_YES 3
#define FILE_KEEP_ALL_NO 4

typedef struct _THREAD_WINDOWS
{
    uint32_t numHandles;
    uint32_t numAllocs;
    HWND *handles;
}THREAD_WINDOWS;

static volatile long keep_mask = 0;

static void
on_changes_box_vaule(HWND hdlg)
{
    if (SendDlgItemMessage(hdlg, IDC_FC_CHK1, BM_GETCHECK, 0, 0))
    {
        // Checkbox Selected
        eu_get_config()->m_upfile = FILE_CHANGE_SEC_YES;
    }
    else
    {
        // Checkbox Unselected
        eu_get_config()->m_upfile = FILE_CHANGE_SEC_NO;
    }
}

// 文件丢失框的消息处理程序。
static INT_PTR CALLBACK
on_changes_delete_proc(HWND hdlg, uint32_t message, WPARAM wParam, LPARAM lParam)
{
    eu_tabpage *p = NULL;
    switch (message)
    {
        case WM_INITDIALOG:
            HWND hdis = NULL;
            p = (eu_tabpage *) lParam;
            const TCHAR *filename = p->pathfile;
            TCHAR txt[LEN_TXT + 1] = { 0 };
            SetWindowLongPtr(hdlg, GWLP_USERDATA, lParam);
            hdis = GetDlgItem(hdlg, IDC_NOEXIST_STC1);
            if (hdis != NULL)
            {
                Static_GetText(hdis, txt, LEN_TXT);
            }
            if (*txt != 0)
            {
                TCHAR stxt[LEN_TXT] = { 0 };
                _sntprintf(stxt, LEN_TXT, txt, filename);
                Static_SetText(hdis, stxt);
            }
            SendMessage(hdlg, DM_SETDEFID, IDC_NOEXIST_BTN4, 0);
            if (on_dark_enable())
            {
                const int buttons[] = { IDC_NOEXIST_BTN1, IDC_NOEXIST_BTN2, IDC_NOEXIST_BTN3, IDC_NOEXIST_BTN4 };
                for (int id = 0; id < _countof(buttons); ++id)
                {
                    HWND btn = GetDlgItem(hdlg, buttons[id]);
                    on_dark_set_theme(btn, L"Explorer", NULL);
                }
                on_dark_set_theme(hdlg, L"Explorer", NULL);
            }
            return util_creater_window(hdlg, eu_module_hwnd());
        CASE_WM_CTLCOLOR_SET:
        {
            return on_dark_set_contorl_color(wParam);
        }
        case WM_SETTINGCHANGE:
        {
            if (on_dark_enable() && on_dark_color_scheme_change(lParam))
            {
                SendMessage(hdlg, WM_THEMECHANGED, 0, 0);
            }
            break;
        }
        case WM_THEMECHANGED:
        {
            if (on_dark_enable())
            {
                on_dark_allow_window(hdlg, true);
                on_dark_refresh_titlebar(hdlg);
                const int buttons[] = { IDC_NOEXIST_BTN1, IDC_NOEXIST_BTN2, IDC_NOEXIST_BTN3, IDC_NOEXIST_BTN4 };
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
        case WM_COMMAND:
        {
            WORD mid = LOWORD(wParam);
            switch (mid)
            {
                case IDC_NOEXIST_BTN1:
                    _InterlockedExchange(&keep_mask, FILE_KEEP_ALL_YES);
                    SendMessage(hdlg, WM_CLOSE, 0, 0);
                    break;
                case IDC_NOEXIST_BTN2:
                    _InterlockedExchange(&keep_mask, FILE_KEEP_ALL_NO);
                    SendMessage(hdlg, WM_CLOSE, 0, 0);
                    break;
                case IDC_NOEXIST_BTN3:
                    if ((p = (eu_tabpage *) GetWindowLongPtr(hdlg, GWLP_USERDATA)) != NULL)
                    {
                        on_tabpage_reload_file(p, 0);
                        p->st_mtime = 0;
                        _InterlockedExchange(&keep_mask, FILE_KEEP_YES);
                        SendMessage(hdlg, WM_CLOSE, 0, 0);
                    }
                    break;
                case IDC_NOEXIST_BTN4:
                    if ((p = (eu_tabpage *) GetWindowLongPtr(hdlg, GWLP_USERDATA)) != NULL)
                    {
                        on_tabpage_reload_file(p, 1);
                        _InterlockedExchange(&keep_mask, FILE_KEEP_NO);
                        SendMessage(hdlg, WM_CLOSE, 0, 0);
                    }
                    break;
                default:
                    break;
            }
            break;
        }
        case WM_CLOSE:
            return (INT_PTR)EndDialog(hdlg, LOWORD(wParam));
        default:
            break;
    }
    return 0;
}

// 文件更改框的消息处理程序。
static INT_PTR CALLBACK
on_changes_time_proc(HWND hdlg, uint32_t message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
            HWND hdis = NULL;
            const TCHAR *filename = ((eu_tabpage *) lParam)->pathfile;
            TCHAR txt[LEN_TXT + 1] = { 0 };
            SetWindowLongPtr(hdlg, GWLP_USERDATA, lParam);
            hdis = GetDlgItem(hdlg, IDC_FC_STC1);
            if (hdis != NULL)
            {
                Static_GetText(hdis, txt, LEN_TXT);
            }
            if (*txt != 0)
            {
                TCHAR stxt[LEN_TXT] = { 0 };
                _sntprintf(stxt, LEN_TXT, txt, filename);
                Static_SetText(hdis, stxt);
            }
            if (on_dark_enable())
            {
                const int buttons[] = { IDC_FC_BTN1, IDC_FC_BTN2, IDC_FC_BTN3, IDC_FC_BTN4 };
                for (int id = 0; id < _countof(buttons); ++id)
                {
                    HWND btn = GetDlgItem(hdlg, buttons[id]);
                    on_dark_set_theme(btn, L"Explorer", NULL);
                }
                on_dark_set_theme(GetDlgItem(hdlg, IDC_FC_CHK1), L"", L"");
                on_dark_set_theme(hdlg, L"Explorer", NULL);
            }
            return 1;
        CASE_WM_CTLCOLOR_SET:
        {
            return on_dark_set_contorl_color(wParam);
        }
        case WM_SETTINGCHANGE:
        {
            if (on_dark_enable() && on_dark_color_scheme_change(lParam))
            {
                SendMessage(hdlg, WM_THEMECHANGED, 0, 0);
            }
            break;
        }
        case WM_THEMECHANGED:
        {
            if (on_dark_enable())
            {
                on_dark_allow_window(hdlg, true);
                on_dark_refresh_titlebar(hdlg);
                const int buttons[] = { IDC_FC_BTN1, IDC_FC_BTN2, IDC_FC_BTN3, IDC_FC_BTN4 };
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
        case WM_COMMAND:
        {
            WORD mid = LOWORD(wParam);
            eu_tabpage *p = NULL;
            switch (mid)
            {
                case IDC_FC_BTN1:
                    if ((p = (eu_tabpage *) GetWindowLongPtr(hdlg, GWLP_USERDATA)) != NULL)
                    {
                        on_tabpage_reload_file(p, 2);
                        SendMessage(hdlg, WM_CLOSE, 0, 0);
                    }
                    break;
                case IDC_FC_BTN2:
                    if ((p = (eu_tabpage *) GetWindowLongPtr(hdlg, GWLP_USERDATA)) != NULL)
                    {
                        on_tabpage_reload_file(p, 0);
                        p->st_mtime = util_last_time(p->pathfile);
                        SendMessage(hdlg, WM_CLOSE, 0, 0);
                    }
                    break;
                case IDC_FC_BTN3:
                    if ((p = (eu_tabpage *) GetWindowLongPtr(hdlg, GWLP_USERDATA)) != NULL)
                    {
                        on_tabpage_reload_file(p, 2);
                        if (eu_get_config()->m_upfile != FILE_CHANGE_SEC_YES)
                        {
                            eu_get_config()->m_upfile = FILE_CHANGE_ALL_YES;
                        }
                        SendMessage(hdlg, WM_CLOSE, 0, 0);
                    }
                    break;
                case IDC_FC_BTN4:
                    if ((p = (eu_tabpage *) GetWindowLongPtr(hdlg, GWLP_USERDATA)) != NULL)
                    {
                        on_tabpage_reload_file(p, 0);
                        p->st_mtime = util_last_time(p->pathfile);
                        if (eu_get_config()->m_upfile != FILE_CHANGE_SEC_NO)
                        {
                            eu_get_config()->m_upfile = FILE_CHANGE_ALL_NO;
                        }
                        SendMessage(hdlg, WM_CLOSE, 0, 0);
                    }
                    break;
                case IDC_FC_CHK1:
                    on_changes_box_vaule(hdlg);
                    break;
                default:
                    break;
            }
            break;
        }
        case WM_ACTIVATE:
            break;
        case WM_CLOSE:
            return (INT_PTR)EndDialog(hdlg, LOWORD(wParam));
        default:
            break;
    }
    return 0;
}

static void
on_changes_alter_dlg(eu_tabpage *p, bool fc)
{
    HWND hwnd = eu_module_hwnd();
    if (hwnd)
    {
        if (fc)
        {
            i18n_dlgbox(hwnd, IDD_DIALOG_FC, on_changes_time_proc, (LPARAM) p);
        }
        else
        {
            i18n_dlgbox(hwnd, IDD_DIALOG_NOEXIST, on_changes_delete_proc, (LPARAM) p);
        }
    }
}

static bool
on_changes_delete_event(eu_tabpage *p)
{
    switch (keep_mask)
    {
        case 0:
        case FILE_KEEP_YES:
        case FILE_KEEP_NO:
        {
            on_changes_alter_dlg(p, false);
            break;
        }
        case FILE_KEEP_ALL_YES:
        {
            on_tabpage_reload_file(p, 0);
            p->st_mtime = 0;
            break;
        }
        case FILE_KEEP_ALL_NO:
        {
            on_tabpage_reload_file(p, 1);
            break;
        }
        default:
            break;
    }
    return (keep_mask > FILE_KEEP_NO);
}

static bool
on_changes_time_event(eu_tabpage *p)
{
    int opt = eu_get_config()->m_upfile;
    switch (opt)
    {
        case 0:
        case FILE_CHANGE_FILE_YES:
        case FILE_CHANGE_FILE_NO:
        {
            on_changes_alter_dlg(p, true);
            break;
        }
        case FILE_CHANGE_ALL_YES:
        case FILE_CHANGE_SEC_YES:
        {
            on_tabpage_reload_file(p, 2);
            break;
        }
        case FILE_CHANGE_ALL_NO:
        case FILE_CHANGE_SEC_NO:
        {
            on_tabpage_reload_file(p, 0);
            p->st_mtime = util_last_time(p->pathfile);
            break;
        }
        default:
            break;
    }
    return (eu_get_config()->m_upfile > FILE_CHANGE_FILE_NO);
}

static void
on_changes_click_sci(eu_tabpage *p)
{
    if (p && p->hwnd_sc)
    {
        POINT pt;
        GetCursorPos(&pt);
        HWND hwnd_click = WindowFromPoint(pt);
        if (p->hwnd_sc == hwnd_click)
        {
            ScreenToClient(p->hwnd_sc, &pt);
            LPARAM lparam = MAKELONG(pt.x, pt.y);
            SendMessage(p->hwnd_sc, WM_LBUTTONUP, 0, lparam);
        }
    }
}

void WINAPI
on_changes_window(HWND hwnd)
{
    for (int index = 0, count = TabCtrl_GetItemCount(g_tabpages); index < count; ++index)
    {
        TCITEM tci = { TCIF_PARAM };
        TabCtrl_GetItem(g_tabpages, index, &tci);
        eu_tabpage *p = (eu_tabpage *) (tci.lParam);
        if (p && !p->hex_mode && !p->is_blank && !p->fs_server.networkaddr[0] && p->st_mtime != util_last_time(p->pathfile))
        {
            on_changes_click_sci(on_tabpage_focus_at());
            on_tabpage_selection(p, index);
            if (_taccess(p->pathfile, 0) == -1)
            {
                if (!on_changes_delete_event(p))
                {
                    break;
                }
            }
            else if (!on_changes_time_event(p))
            {
                break;
            }
        }
    }
    if (eu_get_config()->m_upfile && eu_get_config()->m_upfile < FILE_CHANGE_SEC_YES)
    { // 复位, 下次会话重新显示窗口
        eu_get_config()->m_upfile = 0;
    }
}

static BOOL CALLBACK
msgbox_enum_proc(HWND hwnd, LPARAM lParam)
{
    THREAD_WINDOWS *thread_wins = (THREAD_WINDOWS *) lParam;
    if (!EnableWindow(hwnd, FALSE))
    {
        if (thread_wins->numHandles >= thread_wins->numAllocs)
        {
            thread_wins->handles =
                HeapReAlloc(GetProcessHeap(), 0, thread_wins->handles, (thread_wins->numAllocs * 2) * sizeof(HWND));
            thread_wins->numAllocs *= 2;
        }
        thread_wins->handles[thread_wins->numHandles++] = hwnd;
    }
    return TRUE;
}

static void
on_msgbox_init(HWND hwnd, LPMSGBOXPARAMSW lpmb)
{
    HFONT hPrevFont;
    RECT rect;
    HWND hItem;
    HDC hdc;
    int i, buttons;
    int bspace, bw, bh, theight, tleft, wwidth, wheight, wleft, wtop, bpos;
    int borheight, borwidth, iheight, ileft, iwidth, twidth, tiheight;
    NONCLIENTMETRICSW nclm;
    HMONITOR monitor;
    MONITORINFO mon_info;
    LPCWSTR lpszText;
    WCHAR *buffer = NULL;
    const WCHAR *ptr;
    /* Index the order the buttons need to appear to an ID* constant */
    static const int buttonOrder[11] = {IDYES, IDNO, IDOK, IDABORT, IDRETRY, IDCANCEL, IDIGNORE, IDTRYAGAIN, IDCONTINUE, IDALWAYS, IDHELP};
    if (!(g_skylark_lang || i18n_reload_lang()))
    {
        return;
    }
    nclm.cbSize = sizeof(nclm);
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, 0, &nclm, 0);

    if (!IS_INTRESOURCE(lpmb->lpszCaption))
    {
        SetWindowText(hwnd, lpmb->lpszCaption);
    }
    else
    {
        uint32_t len = LoadString(lpmb->hInstance, LOWORD(lpmb->lpszCaption), (LPWSTR) &ptr, 0);
        if (!len)
        {
            len = LoadString(g_skylark_lang, IDS_USER32_ERROR, (LPWSTR) &ptr, 0);
        }
        buffer = HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR));
        if (buffer)
        {
            memcpy(buffer, ptr, len * sizeof(WCHAR));
            buffer[len] = 0;
            SetWindowText(hwnd, buffer);
            HeapFree(GetProcessHeap(), 0, buffer);
            buffer = NULL;
        }
    }
    if (IS_INTRESOURCE(lpmb->lpszText))
    {
        uint32_t len = LoadString(lpmb->hInstance, LOWORD(lpmb->lpszText), (LPWSTR) &ptr, 0);
        lpszText = buffer = HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR));
        if (buffer)
        {
            memcpy(buffer, ptr, len * sizeof(WCHAR));
            buffer[len] = 0;
        }
    }
    else
    {
        lpszText = lpmb->lpszText;
    }
    SetWindowText(GetDlgItem(hwnd, MSGBOX_IDTEXT), lpszText);

    /* Remove not selected buttons and assign the WS_GROUP style to the first button */
    hItem = 0;
    switch (lpmb->dwStyle & MB_TYPEMASK)
    {
        case MB_OK:
            DestroyWindow(GetDlgItem(hwnd, IDCANCEL));
        /* fall through */
        case MB_OKCANCEL:
            hItem = GetDlgItem(hwnd, IDOK);
            DestroyWindow(GetDlgItem(hwnd, IDABORT));
            DestroyWindow(GetDlgItem(hwnd, IDRETRY));
            DestroyWindow(GetDlgItem(hwnd, IDIGNORE));
            DestroyWindow(GetDlgItem(hwnd, IDYES));
            DestroyWindow(GetDlgItem(hwnd, IDNO));
            DestroyWindow(GetDlgItem(hwnd, IDTRYAGAIN));
            DestroyWindow(GetDlgItem(hwnd, IDCONTINUE));
            DestroyWindow(GetDlgItem(hwnd, IDALWAYS));
            break;
        case MB_ABORTRETRYIGNORE:
            hItem = GetDlgItem(hwnd, IDABORT);
            DestroyWindow(GetDlgItem(hwnd, IDOK));
            DestroyWindow(GetDlgItem(hwnd, IDCANCEL));
            DestroyWindow(GetDlgItem(hwnd, IDYES));
            DestroyWindow(GetDlgItem(hwnd, IDNO));
            DestroyWindow(GetDlgItem(hwnd, IDCONTINUE));
            DestroyWindow(GetDlgItem(hwnd, IDTRYAGAIN));
            DestroyWindow(GetDlgItem(hwnd, IDALWAYS));
            break;
        case MB_YESNO:
            DestroyWindow(GetDlgItem(hwnd, IDCANCEL));
        /* fall through */
        case MB_YESNOCANCEL:
            hItem = GetDlgItem(hwnd, IDYES);
            DestroyWindow(GetDlgItem(hwnd, IDOK));
            DestroyWindow(GetDlgItem(hwnd, IDABORT));
            DestroyWindow(GetDlgItem(hwnd, IDRETRY));
            DestroyWindow(GetDlgItem(hwnd, IDIGNORE));
            DestroyWindow(GetDlgItem(hwnd, IDCONTINUE));
            DestroyWindow(GetDlgItem(hwnd, IDTRYAGAIN));
            DestroyWindow(GetDlgItem(hwnd, IDALWAYS));
            break;
        case MB_YESNOALWAYS:
            hItem = GetDlgItem(hwnd, IDALWAYS);
            DestroyWindow(GetDlgItem(hwnd, IDOK));
            DestroyWindow(GetDlgItem(hwnd, IDABORT));
            DestroyWindow(GetDlgItem(hwnd, IDRETRY));
            DestroyWindow(GetDlgItem(hwnd, IDIGNORE));
            DestroyWindow(GetDlgItem(hwnd, IDCONTINUE));
            DestroyWindow(GetDlgItem(hwnd, IDTRYAGAIN));
            DestroyWindow(GetDlgItem(hwnd, IDCANCEL));
            break;
        case MB_RETRYCANCEL:
            hItem = GetDlgItem(hwnd, IDRETRY);
            DestroyWindow(GetDlgItem(hwnd, IDOK));
            DestroyWindow(GetDlgItem(hwnd, IDABORT));
            DestroyWindow(GetDlgItem(hwnd, IDIGNORE));
            DestroyWindow(GetDlgItem(hwnd, IDYES));
            DestroyWindow(GetDlgItem(hwnd, IDNO));
            DestroyWindow(GetDlgItem(hwnd, IDCONTINUE));
            DestroyWindow(GetDlgItem(hwnd, IDTRYAGAIN));
            DestroyWindow(GetDlgItem(hwnd, IDALWAYS));
            break;
        case MB_CANCELTRYCONTINUE:
            hItem = GetDlgItem(hwnd, IDCANCEL);
            DestroyWindow(GetDlgItem(hwnd, IDOK));
            DestroyWindow(GetDlgItem(hwnd, IDABORT));
            DestroyWindow(GetDlgItem(hwnd, IDIGNORE));
            DestroyWindow(GetDlgItem(hwnd, IDYES));
            DestroyWindow(GetDlgItem(hwnd, IDNO));
            DestroyWindow(GetDlgItem(hwnd, IDRETRY));
            DestroyWindow(GetDlgItem(hwnd, IDALWAYS));
    }

    if (hItem)
    {
        SetWindowLongPtr(hItem, GWL_STYLE, GetWindowLongPtr(hItem, GWL_STYLE) | WS_GROUP);
    }
    /* Set the icon */
    switch (lpmb->dwStyle & MB_ICONMASK)
    {
        case MB_ICONEXCLAMATION:
            SendDlgItemMessageW(hwnd, MSGBOX_IDICON, STM_SETICON, (WPARAM) LoadIcon(0, (LPWSTR) IDI_EXCLAMATION), 0);
            break;
        case MB_ICONQUESTION:
            SendDlgItemMessageW(hwnd, MSGBOX_IDICON, STM_SETICON, (WPARAM) LoadIcon(0, (LPWSTR) IDI_QUESTION), 0);
            break;
        case MB_ICONASTERISK:
            SendDlgItemMessageW(hwnd, MSGBOX_IDICON, STM_SETICON, (WPARAM) LoadIcon(0, (LPWSTR) IDI_ASTERISK), 0);
            break;
        case MB_ICONHAND:
            SendDlgItemMessageW(hwnd, MSGBOX_IDICON, STM_SETICON, (WPARAM) LoadIcon(0, (LPWSTR) IDI_HAND), 0);
            break;
        case MB_USERICON:
            SendDlgItemMessageW(hwnd, MSGBOX_IDICON, STM_SETICON, (WPARAM) LoadIcon(lpmb->hInstance, lpmb->lpszIcon), 0);
            break;
        default:
            /* By default, Windows 95/98/NT do not associate an icon to message boxes.
             * So wine should do the same.
             */
            break;
    }

    /* Remove Help button unless MB_HELP supplied */
    if (!(lpmb->dwStyle & MB_HELP))
    {
        DestroyWindow(GetDlgItem(hwnd, IDHELP));
    }

    /* Position everything */
    GetWindowRect(hwnd, &rect);
    borheight = rect.bottom - rect.top;
    borwidth = rect.right - rect.left;
    GetClientRect(hwnd, &rect);
    borheight -= rect.bottom - rect.top;
    borwidth -= rect.right - rect.left;

    /* Get the icon height */
    GetWindowRect(GetDlgItem(hwnd, MSGBOX_IDICON), &rect);
    MapWindowPoints(0, hwnd, (LPPOINT) &rect, 2);
    if (!(lpmb->dwStyle & MB_ICONMASK))
    {
        rect.bottom = rect.top;
        rect.right = rect.left;
    }
    iheight = rect.bottom - rect.top;
    ileft = rect.left;
    iwidth = rect.right - ileft;

    hdc = GetDC(hwnd);
    hPrevFont = SelectObject(hdc, (HFONT) SendMessage(hwnd, WM_GETFONT, 0, 0));

    /* Get the number of visible buttons and their size */
    bh = bw = 1; /* Minimum button sizes */
    for (buttons = 0, i = IDOK; i <= IDALWAYS; ++i)
    {
        if (i == IDCLOSE) continue; /* No CLOSE button */
        hItem = GetDlgItem(hwnd, i);
        if (GetWindowLongPtr(hItem, GWL_STYLE) & WS_VISIBLE)
        {
            int w, h;
            WCHAR buttonText[QW_SIZE] = {0};
            buttons++;
            if (GetWindowText(hItem, buttonText, QW_SIZE-1))
            {
                DrawText(hdc, buttonText, -1, &rect, DT_LEFT | DT_EXPANDTABS | DT_CALCRECT);
                h = rect.bottom - rect.top;
                w = rect.right - rect.left;
                if (h > bh) bh = h;
                if (w > bw) bw = w;
            }
        }
    }
    bw = max(bw, bh * 2);
    /* Button white space */
    bh = bh * 2;
    bw = bw * 2;
    bspace = bw / 3; /* Space between buttons */

    /* Get the text size */
    GetClientRect(GetDlgItem(hwnd, MSGBOX_IDTEXT), &rect);
    rect.top = rect.left = rect.bottom = 0;
    DrawText(hdc, lpszText, -1, &rect, DT_LEFT | DT_EXPANDTABS | DT_WORDBREAK | DT_CALCRECT | DT_NOPREFIX);
    /* Min text width corresponds to space for the buttons */
    tleft = ileft;
    if (iwidth) tleft += ileft + iwidth;
    twidth = max((bw + bspace) * buttons + bspace - tleft, rect.right);
    theight = rect.bottom;

    SelectObject(hdc, hPrevFont);
    ReleaseDC(hwnd, hdc);

    tiheight = 16 + max(iheight, theight);
    wwidth = tleft + twidth + ileft + borwidth;
    wheight = 8 + tiheight + bh + borheight;

    /* Message boxes are always desktop centered, so query desktop size and center window */
    monitor = MonitorFromWindow(lpmb->hwndOwner ? lpmb->hwndOwner : GetActiveWindow(), MONITOR_DEFAULTTOPRIMARY);
    mon_info.cbSize = sizeof(mon_info);
    GetMonitorInfoW(monitor, &mon_info);
    wleft = (mon_info.rcWork.left + mon_info.rcWork.right - wwidth) / 2;
    wtop = (mon_info.rcWork.top + mon_info.rcWork.bottom - wheight) / 2;

    /* Resize and center the window */
    SetWindowPos(hwnd, 0, wleft, wtop, wwidth, wheight, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW);

    /* Position the icon */
    SetWindowPos(GetDlgItem(hwnd, MSGBOX_IDICON), 0, ileft, (tiheight - iheight) / 2, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW);

    /* Position the text */
    SetWindowPos(GetDlgItem(hwnd, MSGBOX_IDTEXT), 0, tleft, (tiheight - theight) / 2, twidth, theight, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW);

    /* Position the buttons */
    bpos = (wwidth - (bw + bspace) * buttons + bspace) / 2;
    for (buttons = i = 0; i < _countof(buttonOrder); i++)
    {
        /* Convert the button order to ID* value to order for the buttons */
        hItem = GetDlgItem(hwnd, buttonOrder[i]);
        if (GetWindowLongPtr(hItem, GWL_STYLE) & WS_VISIBLE)
        {
            if (buttons++ == ((lpmb->dwStyle & MB_DEFMASK) >> 8))
            {
                SetFocus(hItem);
                SendMessage(hItem, BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE);
            }
            SetWindowPos(hItem, 0, bpos, tiheight, bw, bh, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW);
            bpos += bw + bspace;
        }
    }
    /*handle modal message boxes*/
    if (((lpmb->dwStyle & MB_TASKMODAL) && (lpmb->hwndOwner == NULL)) || (lpmb->dwStyle & MB_SYSTEMMODAL))
    {
        SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
    }
    HeapFree(GetProcessHeap(), 0, buffer);
}

/***************************************************************************
 *           msgbox_dlg_proc
 *
 * Dialog procedure for message boxes.
 **************************************************************************/
static INT_PTR CALLBACK
msgbox_dlg_proc(HWND hwnd, uint32_t message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
        {
            LPMSGBOXPARAMSW mbp = (LPMSGBOXPARAMSW) lParam;
            SetWindowContextHelpId(hwnd, (DWORD) mbp->dwContextHelpId);
            on_msgbox_init(hwnd, mbp);
            SetPropA(hwnd, "WINE_MSGBOX_HELPCALLBACK", mbp->lpfnMsgBoxCallback);
            if (on_dark_enable())
            {
                const int buttons[] = { IDOK, IDCANCEL, IDABORT, IDRETRY, IDYES, IDNO, IDTRYAGAIN, IDIGNORE, IDCONTINUE, IDALWAYS, IDHELP };
                for (int id = 0; id < _countof(buttons); ++id)
                {
                    HWND btn = GetDlgItem(hwnd, buttons[id]);
                    if (btn)
                    {
                        on_dark_set_theme(btn, L"Explorer", NULL);
                    }
                }
                SendMessage(hwnd, WM_THEMECHANGED, 0, 0);
            }
            if (GetDlgItem(hwnd, IDOK))
            {
                SendMessage(hwnd, DM_SETDEFID, IDOK, 0);
            }
            else if (GetDlgItem(hwnd, IDYES))
            {
                SendMessage(hwnd, DM_SETDEFID, IDYES, 0);
            }
            else if (GetDlgItem(hwnd, IDNO))
            {
                SendMessage(hwnd, DM_SETDEFID, IDNO, 0);
            }
            return (LONG_PTR)util_creater_window(hwnd, mbp->hwndOwner);
        }
        CASE_WM_CTLCOLOR_SET:
        {
            return (BOOL) on_dark_set_contorl_color(wParam);
        }
        case WM_SETTINGCHANGE:
        {
            if (on_dark_enable() && on_dark_color_scheme_change(lParam))
            {
                SendMessage(hwnd, WM_THEMECHANGED, 0, 0);
            }
            break;
        }
        case WM_THEMECHANGED:
        {
            on_dark_allow_window(hwnd, true);
            on_dark_refresh_titlebar(hwnd);
            const int buttons[] = {IDOK, IDCANCEL, IDABORT, IDRETRY, IDYES, IDNO, IDTRYAGAIN, IDIGNORE, IDCONTINUE, IDALWAYS, IDHELP};
            for (int id = 0; id < _countof(buttons); ++id)
            {
                HWND btn = GetDlgItem(hwnd, buttons[id]);
                if (btn)
                {
                    on_dark_allow_window(btn, true);
                    SendMessage(btn, WM_THEMECHANGED, 0, 0);
                }
            }
            break;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                case IDCANCEL:
                case IDABORT:
                case IDRETRY:
                case IDIGNORE:
                case IDYES:
                case IDNO:
                case IDTRYAGAIN:
                case IDCONTINUE:
                case IDALWAYS:
                    EndDialog(hwnd, wParam);
                    break;
                case IDHELP:
                    break;
            }
            break;

        case WM_HELP:
        {
            MSGBOXCALLBACK callback = (MSGBOXCALLBACK) GetPropA(hwnd, "SKYLARK_MSGBOX_HELPCALLBACK");
            HELPINFO hi;

            memcpy(&hi, (void *) lParam, sizeof(hi));
            hi.dwContextId = GetWindowContextHelpId(hwnd);
            if (callback)
            {
                callback(&hi);
            }
            else
            {
                SendMessage(GetWindow(hwnd, GW_OWNER), WM_HELP, 0, (LPARAM) &hi);
            }
            break;
        }
        default:
            /* Ok. Ignore all the other messages */
            break;
    }
    return 0;
}

/****************************************************************************
 *        on_changes_msgbox like MessageBoxIndirectW (USER32.@)
 ***************************************************************************/
static int WINAPI
on_changes_msgbox(LPMSGBOXPARAMSW msgbox)
{
    LPVOID tmplate;
    HRSRC hres;
    int ret = -1;
    uint32_t i;
    THREAD_WINDOWS thread_wins;
    if (!(g_skylark_lang || i18n_reload_lang()))
    {
        return ret;
    }
    if (!(hres = FindResourceEx(g_skylark_lang, (LPWSTR) RT_DIALOG, _T("MSGBOX"), (WORD) msgbox->dwLanguageId)))
    {
        if (!msgbox->dwLanguageId || !(hres = FindResourceEx(g_skylark_lang, (LPWSTR) RT_DIALOG, _T("MSGBOX"), LANG_NEUTRAL)))
        {
            return ret;
        }
    }
    if (!(tmplate = LoadResource(g_skylark_lang, hres)))
    {
        return ret;
    }
    if ((msgbox->dwStyle & MB_TASKMODAL) && (msgbox->hwndOwner == NULL))
    {
        thread_wins.numHandles = 0;
        thread_wins.numAllocs = 10;
        thread_wins.handles = HeapAlloc(GetProcessHeap(), 0, 10 * sizeof(HWND));
        EnumThreadWindows(GetCurrentThreadId(), msgbox_enum_proc, (LPARAM) &thread_wins);
    }
    ret = (int) DialogBoxIndirectParamW(g_skylark_lang, tmplate, msgbox->hwndOwner, msgbox_dlg_proc, (LPARAM) msgbox);
    if ((msgbox->dwStyle & MB_TASKMODAL) && (msgbox->hwndOwner == NULL))
    {
        for (i = 0; i < thread_wins.numHandles; i++)
        {
            EnableWindow(thread_wins.handles[i], TRUE);
        }
        HeapFree(GetProcessHeap(), 0, thread_wins.handles);
    }
    return ret;
}

/****************************************************************************
 *        eu_msgbox 模仿 MessageBoxW, 支持dark mode
 ***************************************************************************/
int
eu_msgbox(HWND hwnd, LPCWSTR text, LPCWSTR title, uint32_t type)
{
    MSGBOXPARAMSW msgbox;
    msgbox.cbSize = sizeof(msgbox);
    msgbox.hwndOwner = hwnd;
    msgbox.hInstance = NULL;
    msgbox.lpszText = text;
    msgbox.lpszCaption = title;
    msgbox.dwStyle = type;
    msgbox.lpszIcon = NULL;
    msgbox.dwContextHelpId = 0;
    msgbox.lpfnMsgBoxCallback = NULL;
    msgbox.dwLanguageId = LANG_NEUTRAL;
    if (IsWindow(hwnd) && IsIconic(hwnd))
    {
        eu_restore_placement(hwnd);
        ShowWindow(hwnd, SW_SHOW);
    }
    return on_changes_msgbox(&msgbox);
}

int WINAPI
eu_i18n_msgbox(HWND hwnd, uint16_t contents_id, uint16_t title_id, uint32_t type)
{
    wchar_t *pstr = NULL;
    LOAD_I18N_RESSTR(title_id, title);
    if ((pstr = (wchar_t *)calloc(sizeof(wchar_t), 2048)) && eu_i18n_load_str(contents_id, pstr, 2048))
    {
        return eu_msgbox(hwnd, pstr, title, type);
    }
    return 0;
}
