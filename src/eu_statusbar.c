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

#define BTN_DEFAULT_WIDTH 50
#define CAP_TOGGLED ((GetKeyState(VK_CAPITAL) & 1) != 0)

HWND g_statusbar = NULL;

static HMENU g_menu_1;
static HMENU g_menu_2;
static HMENU g_menu_3;
static HFONT hfont_btn;
static LONG_PTR old_proc;
static int g_status_height;

char iconv_undo_str[64] = {0};

void WINAPI
on_statusbar_btn_case(void)
{
    HWND h_bt_1 = NULL;
    if (util_under_wine())
    {
        return;
    }
    if (!g_statusbar)
    {
        return;
    }
    if (!(h_bt_1 = GetDlgItem(g_statusbar, IDM_BTN_1)))
    {
        return;
    }
    if (CAP_TOGGLED)
    {
        if (!IsWindowEnabled(h_bt_1))
        {
            Button_Enable(h_bt_1, 1);
        }
    }
    else if (IsWindowEnabled(h_bt_1))
    {
        Button_Enable(h_bt_1, 0);
    }
}

/*******************************************
 *m_auto为true则自动翻转,为false则手动翻转
 *返回翻转之后的状态,
 *0, 错误
 *1, 只读,
 *2, 可写.
 *******************************************/
static int
set_btn_rw(eu_tabpage *pnode, bool m_auto)
{
    int ret = 0;
    DWORD m_attr = 0;
    TCHAR lpch[EDITNUMBS] = { 0 };
    if (!g_statusbar)
    {
        return 0;
    }
    HWND h_bt_2 = GetDlgItem(g_statusbar, IDM_BTN_2); 
    LOAD_I18N_RESSTR(IDS_BUTTON_R, rstr);
    LOAD_I18N_RESSTR(IDS_BUTTON_W, wstr);  
    if (!h_bt_2)
    {
        return 0;
    }
    if (!(pnode && *pnode->pathfile))
    {
        return 0;
    }
    if ((m_attr = GetFileAttributes(pnode->pathfile)) == INVALID_FILE_ATTRIBUTES)
    {
        if (pnode->is_blank)
        {
            Button_SetText(h_bt_2, wstr);
            on_toolbar_update_button();
        }
        return 0;
    }
    Button_GetText(h_bt_2, lpch, EDITNUMBS - 1);
    if (m_auto)
    {
        if (m_attr & FILE_ATTRIBUTE_READONLY)
        {
            Button_SetText(h_bt_2, rstr);
            eu_sci_call(pnode, SCI_SETREADONLY, 1, 0);
            ret = 1;
        }
        else
        {
            Button_SetText(h_bt_2, wstr);
            eu_sci_call(pnode, SCI_SETREADONLY, 0, 0);
            ret = 2;
        }
    }
    else if (_tcscmp(lpch, rstr) == 0)
    {
        if (m_attr & FILE_ATTRIBUTE_READONLY)
        {   // 去掉只读属性
            m_attr &= ~FILE_ATTRIBUTE_READONLY;
            SetFileAttributes(pnode->pathfile, m_attr);
        }
        Button_SetText(h_bt_2, wstr);
        eu_sci_call(pnode, SCI_SETREADONLY, 0, 0);
        ret = 2;
    }
    else if (_tcscmp(lpch, wstr) == 0)
    {
        if (!(m_attr & FILE_ATTRIBUTE_READONLY))
        {   // 加上只读属性
            m_attr |= FILE_ATTRIBUTE_READONLY;
            SetFileAttributes(pnode->pathfile, m_attr);
            Button_SetText(h_bt_2, rstr);
            eu_sci_call(pnode, SCI_SETREADONLY, 1, 0);
        }
        ret = 1;
    }
    on_toolbar_update_button();
    return ret;
}

static void
on_statusbar_set_text(HWND hwnd, const uint8_t part, LPCTSTR lpsz)
{
    if (lpsz && part != SB_SIMPLEID)
    {
        WPARAM wParam = (WPARAM)part;
        SendMessage(hwnd, SB_SETTEXT, (WPARAM)(on_dark_supports() ? (wParam |= SBT_OWNERDRAW) : (wParam &= ~SBT_OWNERDRAW)), (LPARAM)lpsz);
    }
}

void WINAPI
on_statusbar_adjust_box(void)
{
    if (!eu_get_config()->m_statusbar)
    {
        g_status_height = 0;
    }
    else if (g_statusbar)
    {
        RECT rc = { 0 };
        GetClientRect(g_statusbar, &rc);
        g_status_height = rc.bottom - rc.top;
    }
}

static void
on_statusbar_adjust_btn(void)
{
    HWND h_bt_1 = GetDlgItem(g_statusbar, IDM_BTN_1);
    HWND h_bt_2 = GetDlgItem(g_statusbar, IDM_BTN_2);
    if (h_bt_1 && h_bt_2)
    {
        int btn_height = 0;
        int btn_width = 0;
        RECT rc_part = {0};
        SendMessage(g_statusbar, SB_GETRECT, STATUSBAR_DOC_BTN, (LPARAM)&rc_part);
        g_status_height = btn_height = rc_part.bottom - rc_part.top;
        btn_width = (rc_part.right - rc_part.left)/2;
        if (btn_width > BTN_DEFAULT_WIDTH)
        {
            btn_width = BTN_DEFAULT_WIDTH;
        }
        HDWP hdwp = BeginDeferWindowPos(2);
        DeferWindowPos(hdwp, h_bt_1, HWND_TOP, rc_part.right - btn_width - 2, rc_part.top, btn_width, btn_height, SWP_NOZORDER | SWP_SHOWWINDOW);
        DeferWindowPos(hdwp, h_bt_2, HWND_TOP, rc_part.right - 2*(btn_width+2), rc_part.top, btn_width, btn_height, SWP_NOZORDER | SWP_SHOWWINDOW);
        EndDeferWindowPos(hdwp);
    }
}

bool check_read_access(void* paddr, size_t nsize)
{
    bool ret = false;
    if (!paddr || !nsize)
    {
        return ret;
    }
    const uint32_t dw_forbidden = PAGE_GUARD | PAGE_NOACCESS;
    const uint32_t dw_read = PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
    MEMORY_BASIC_INFORMATION mbi;
    uintptr_t pcur = (uintptr_t)paddr;
    uintptr_t pend = (uintptr_t)(pcur + (nsize - 1));
    do
    {
        ZeroMemory(&mbi, sizeof(mbi));
        VirtualQuery((LPCVOID)pcur, &mbi, sizeof(mbi));
        ret = (mbi.State & MEM_COMMIT)           // memory allocated and
            && !(mbi.Protect & dw_forbidden)    // access to page allowed and
            && (mbi.Protect & dw_read);        // the required rights
        pcur = ((uintptr_t)(mbi.BaseAddress) + mbi.RegionSize);
    } while (ret && pcur <= pend);
    return ret;
}

void WINAPI
on_statusbar_size(void)
{
    if (g_statusbar)
    {
        if (!eu_get_config()->m_statusbar)
        {
            if ((GetWindowLongPtr(g_statusbar, GWL_STYLE) & WS_VISIBLE))
            {
                ShowWindow(g_statusbar, SW_HIDE);
            } 
            g_status_height = 0;
        }
        else
        {
            HWND hwnd = eu_module_hwnd();
            RECT rc_main = {0};
            GetWindowRect(hwnd, &rc_main);
            int cx = rc_main.right - rc_main.left;
            int n_half = cx / 8;
            int parts[] = { n_half*2, n_half*3, n_half*4, n_half*5+20, n_half*6+20, n_half*7+70, -1 };
            SendMessage(g_statusbar, SB_SETPARTS, STATUSBAR_PART, (LPARAM)&parts);
            if (!(GetWindowLongPtr(g_statusbar, GWL_STYLE) & WS_VISIBLE))
            {
                ShowWindow(g_statusbar, SW_SHOW);
            }
            SendMessage(g_statusbar, WM_SIZE, 0, 0);
            InvalidateRect(g_statusbar, NULL, false);
            on_statusbar_adjust_btn();
            on_statusbar_update();
            UpdateWindow(hwnd);
        }
    }
}

LRESULT WINAPI
on_statusbar_draw_item(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(hwnd);
    if (on_dark_enable())
    { 
        const DRAWITEMSTRUCT* pdis = (const DRAWITEMSTRUCT*)lParam;
        const int part_id = (const int)pdis->itemID;
        if (part_id == -1) 
        {
            return 0;
        }
        const HDC hdc = pdis->hDC;
        const RECT rc = pdis->rcItem;
        set_bk_color(hdc, true);
        set_text_color(hdc, true);
        // overpaint part frames
        const HWND hwnd_item = pdis->hwndItem;
        const int bdh = GetSystemMetrics(SM_CYFRAME);
        const HDC hdc_from = GetWindowDC(hwnd_item);
        RECT rcf = rc;
        for (int i = 1; i < bdh; ++i) 
        {
            FrameRect(hdc_from, &rcf, (HBRUSH)on_dark_get_brush());
            rcf.left -= 1;
            rcf.top -= 1;
            rcf.bottom += 1;
            rcf.right += 1;
        }
        ReleaseDC(hwnd_item, hdc_from);
        TCHAR *text = (TCHAR *)(pdis->itemData);
        // Judge the memory permission, because the memory address may be modified under multithreading
        if (text && check_read_access(text, 2))
        {
            ExtTextOut(hdc, rc.left + 1, rc.top + 1, ETO_OPAQUE | ETO_NUMERICSLOCAL, &rc, text, (UINT)_tcslen(text), NULL);
        }
        return 1;
    }
    return 0;
}

static bool
create_button(HWND hstatus)
{
    RECT rc = { 0 };
    TCHAR lcap[EDITNUMBS] = { 0 };
    TCHAR wstr[EDITNUMBS] = { 0 };
    HWND h_bt_1 = NULL;
    HWND h_bt_2 = NULL;
    if (!eu_i18n_load_str(IDS_BUTTON_CAP, lcap, EDITNUMBS))
    {
        return false;
    }
    if (!eu_i18n_load_str(IDS_BUTTON_W, wstr, EDITNUMBS))
    {
        return false;
    }
    do
    {
        uint32_t style =  WS_CHILD | WS_VISIBLE | WS_BORDER | BS_FLAT;
        h_bt_1 = CreateWindowEx(0, _T("button"), lcap, style, 0, 0, 0, 0, hstatus, (HMENU) IDM_BTN_1, eu_module_handle(), NULL);
        if (!h_bt_1)
        {
            printf("CreateWindowEx g_bt_1 failed\n");
            break;
        }
        Button_Enable(h_bt_1, 0);
        ShowWindow(h_bt_1, SW_HIDE);
        h_bt_2 = CreateWindowEx(0, _T("button"), wstr, style, 0, 0, 0, 0, hstatus, (HMENU) IDM_BTN_2, eu_module_handle(), NULL);
        if (!h_bt_2)
        {
            printf("CreateWindowEx g_bt_2 failed\n");
            break;
        }
        ShowWindow(h_bt_2, SW_HIDE);
    } while(0);
    return (h_bt_1 && h_bt_2);
}

static void
set_menu_check(HMENU hmenu, int first_id, int last_id, int id, int parts)
{
    int len = 0;
    TCHAR buf[FILESIZE + 1] = {0xA554, 0x0020, 0};
    if (id == IDM_UNI_UTF16LE || id == IDM_UNI_UTF16BE)
    {
        ++id;   // IDM_UNI_UTF16LEB or IDM_UNI_UTF16BEB
    }
    if ((len = GetMenuString(hmenu, id, &buf[2], FILESIZE-2, MF_BYCOMMAND)) > 0)
    {
        for (int i = IDM_UNI_UTF8; hmenu == g_menu_2 && i <= IDM_UNKNOWN; ++i)
        {
            CheckMenuItem(hmenu, i, MF_BYCOMMAND | MFS_UNCHECKED);
        }
        if (buf[2] && CheckMenuRadioItem(hmenu, first_id, last_id, id, MF_BYCOMMAND))
        {
            on_statusbar_set_text(g_statusbar, parts, buf);
        }
    }
}

static int 
on_convert_coding(eu_tabpage *pnode, int encoding)
{
    sptr_t file_len = 0;
    char *file_buf = NULL;
    char *src_str = NULL;
    char *dst_str = NULL;
    size_t src_len = 0;
    size_t dst_len = 0;
    size_t res = 0;
    euconv_t evd = {0};
    if (!pnode)
    {
        return 1;
    }
    if (!(file_buf = util_strdup_content(pnode, (size_t *)&file_len)))
    {
        return 1;        
    }  
    evd.warning = WANRING_AUTO;
    evd.src_from = "utf-8";
    evd.dst_to = eu_query_encoding_name(encoding);
    src_str = file_buf;
    src_len = file_len;
    res = on_encoding_do_iconv(&evd, src_str, &src_len, &dst_str, &dst_len);
    if (res == (size_t)-1)
    {
        free(file_buf);
        MSG_BOX(IDC_MSG_CONV_FAIL1, IDC_MSG_ERROR, MB_ICONERROR|MB_OK);
        return 1;
    }
    else if (res == 255)
    {
        free(file_buf);
        return 1;
    }
    else if (res > 0)
    {
        free(file_buf);
        free(dst_str);
        MSG_BOX(IDC_MSG_CONV_FAIL2, IDC_MSG_ERROR, MB_ICONERROR|MB_OK);
        return 1;
    }    
    if (dst_str)
    {
        free(dst_str);
    }
    free(file_buf);
    // we set new codepage
    _snprintf(iconv_undo_str, ACNAME_LEN-1, "%s=%d=%d", "_iconv/?@#$%^&*()`/~", pnode->codepage, encoding);
    pnode->codepage = encoding;
    on_encoding_set_bom_from_cp(pnode);
    return on_tabpage_editor_modify(pnode, iconv_undo_str);
}

void WINAPI
on_statusbar_pop_menu(int parts, LPPOINT pt)
{
    if (!(g_statusbar && pt))
    {
        return;
    }
    switch (parts)
    {
        case STATUSBAR_DOC_EOLS:
            TrackPopupMenu(g_menu_1, TPM_LEFTALIGN | TPM_BOTTOMALIGN, pt->x, pt->y, 0, g_statusbar, NULL);
            break;
        case STATUSBAR_DOC_ENC:
            TrackPopupMenu(g_menu_2, TPM_LEFTALIGN | TPM_BOTTOMALIGN, pt->x, pt->y, 0, g_statusbar, NULL);
            break;
        case STATUSBAR_DOC_TYPE:
            TrackPopupMenu(g_menu_3, TPM_LEFTALIGN | TPM_BOTTOMALIGN, pt->x, pt->y, 0, g_statusbar, NULL);
            break;                        
    }
}

static void
on_statusbar_update_btn(HWND hwnd)
{
    if (!hfont_btn)
    {
        LOGFONT logfont = {eu_dpi_scale_font()};
        logfont.lfWeight = FW_NORMAL;
        logfont.lfOutPrecision = OUT_DEFAULT_PRECIS;
        logfont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
        logfont.lfQuality = CLEARTYPE_QUALITY;
        logfont.lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;
        logfont.lfCharSet = ANSI_CHARSET;
        _tcsncpy(logfont.lfFaceName, _T("MS Shell Dlg"), _countof(logfont.lfFaceName)-1);
        if (!(hfont_btn = CreateFontIndirect(&logfont)))
        {
            return;
        }
    }
    SendMessage(hwnd, WM_SETFONT, (WPARAM)hfont_btn, 0);
}

static intptr_t CALLBACK
stbar_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    eu_tabpage *pnode = NULL;
    switch (message)
    {
        case WM_ERASEBKGND:
        {
            if (!on_dark_enable())
            {
                break;
            }
            RECT rc = { 0 };
            GetClientRect(hwnd, &rc);
            FillRect((HDC)wParam, &rc, (HBRUSH)on_dark_get_brush());
            return 1;
        }     
        case WM_COMMAND:
        {
            if (HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDM_BTN_2)
            {
                if ((pnode = on_tabpage_focus_at()) && pnode->hwnd_sc)
                {
                    set_btn_rw(pnode, false);
                    SetFocus(pnode->hwnd_sc);
                }
                else
                {
                    SetFocus(eu_module_hwnd());
                }
                break;
            }
            uint16_t id_menu = LOWORD(wParam);
            if (id_menu >= IDM_TYPES_0 && id_menu <= IDM_TYPES_0 + VIEW_FILETYPE_MAXCOUNT-1)
            {
                if (on_view_switch_type(id_menu - IDM_TYPES_0 - 1) == 0)
                {
                    set_menu_check(g_menu_3, IDM_TYPES_0, IDM_TYPES_0 + VIEW_FILETYPE_MAXCOUNT-1, id_menu, STATUSBAR_DOC_TYPE);
                }
                break;
            }
            if (!(pnode = on_tabpage_focus_at()))
            {
                break;
            }
            if (id_menu > IDM_UNI_UTF8 && id_menu < IDM_LBREAK_4)
            {
                if (eu_sci_call(pnode, SCI_GETREADONLY, 0, 0))
                {
                    MSG_BOX(IDC_MSG_DO_READONLY, IDC_MSG_WARN, MB_ICONWARNING|MB_OK);
                    break;
                }
            }
            switch (id_menu)
            {   
                case IDM_LBREAK_1:
                case IDM_LBREAK_2:
                case IDM_LBREAK_3:
                {
                    int old_eol = pnode->eol;
                    if (!on_edit_convert_eols(pnode, id_menu-IDM_LBREAK_1))
                    {
                        set_menu_check(g_menu_1, IDM_LBREAK_1, IDM_LBREAK_3, id_menu, STATUSBAR_DOC_EOLS);
                    }
                    break;
                }
                case IDM_UNI_UTF8:
                    pnode->pre_len = 0;
                    memset(pnode->pre_context, 0, sizeof(pnode->pre_context));
                    _snprintf(iconv_undo_str, ACNAME_LEN-1, "%s=%d=%d", "_iconv/?@#$%^&*()`/~", pnode->codepage, IDM_UNI_UTF8);
                    pnode->codepage = IDM_UNI_UTF8;
                    on_tabpage_editor_modify(pnode, iconv_undo_str);
                    set_menu_check(g_menu_2, IDM_UNI_UTF8, IDM_UNI_ASCII, id_menu, STATUSBAR_DOC_ENC);
                    break;
                case IDM_UNI_UTF8B:
                    pnode->pre_len = 3;
                    memset(pnode->pre_context, 0, sizeof(pnode->pre_context));
                    memcpy(pnode->pre_context, "\xEF\xBB\xBF", 3);
                    _snprintf(iconv_undo_str, ACNAME_LEN-1, "%s=%d=%d", "_iconv/?@#$%^&*()`/~", pnode->codepage, IDM_UNI_UTF8B);
                    pnode->codepage = IDM_UNI_UTF8B;
                    on_tabpage_editor_modify(pnode, iconv_undo_str);
                    set_menu_check(g_menu_2, IDM_UNI_UTF8, IDM_UNI_ASCII, id_menu, STATUSBAR_DOC_ENC);
                    break;
                case IDM_UNI_UTF16LEB:
                case IDM_UNI_UTF16BEB:
                case IDM_UNI_UTF32LE:
                case IDM_UNI_UTF32BE:
                case IDM_UNI_ASCII:
                    if (!on_convert_coding(pnode, id_menu))
                    {
                        set_menu_check(g_menu_2, IDM_UNI_UTF8, IDM_UNI_ASCII, id_menu, STATUSBAR_DOC_ENC);
                    }
                    break;
                case IDM_ANSI_1:
                case IDM_ANSI_2:
                case IDM_ANSI_3:
                case IDM_ANSI_4:
                case IDM_ANSI_5:
                case IDM_ANSI_6:
                case IDM_ANSI_7:
                case IDM_ANSI_8:
                case IDM_ANSI_9:
                case IDM_ANSI_10:
                case IDM_ANSI_11:
                case IDM_ANSI_12:
                case IDM_ANSI_13:
                case IDM_ANSI_14:
                    if (!on_convert_coding(pnode, id_menu))
                    {
                        set_menu_check(g_menu_2, IDM_ANSI_1, IDM_ANSI_14, id_menu, STATUSBAR_DOC_ENC);
                    }
                    break;
                case IDM_ISO_1:
                case IDM_ISO_2:
                case IDM_ISO_3:
                case IDM_ISO_4:
                case IDM_ISO_5:
                case IDM_ISO_6:
                case IDM_ISO_7:
                case IDM_ISO_8:
                case IDM_ISO_9:
                case IDM_ISO_10:
                case IDM_ISO_11:
                case IDM_ISO_13:
                case IDM_ISO_15:
                case IDM_ISO_16:
                case IDM_ISO_JP:
                case IDM_ISO_KR:
                case IDM_ISO_CN:
                    if (!on_convert_coding(pnode, id_menu))
                    {
                        set_menu_check(g_menu_2, IDM_ISO_1, IDM_ISO_CN, id_menu, STATUSBAR_DOC_ENC);
                    }
                    break;
                case IDM_IBM_1:
                case IDM_IBM_2:
                case IDM_IBM_3:    
                    if (!on_convert_coding(pnode, id_menu))
                    {
                        set_menu_check(g_menu_2, IDM_IBM_1, IDM_IBM_3, id_menu, STATUSBAR_DOC_ENC);
                    }
                    break;
                case IDM_EUC_1:
                case IDM_EUC_2:
                    if (!on_convert_coding(pnode, id_menu))
                    {
                        set_menu_check(g_menu_2, IDM_EUC_1, IDM_EUC_2, id_menu, STATUSBAR_DOC_ENC);
                    }
                    break;
                case IDM_OTHER_1:
                case IDM_OTHER_2:
                case IDM_OTHER_3:
                case IDM_OTHER_ANSI:
                case IDM_OTHER_BIN:
                case IDM_UNKNOWN:
                    if (!on_convert_coding(pnode, id_menu))
                    {
                        set_menu_check(g_menu_2, IDM_OTHER_1, IDM_UNKNOWN, id_menu, STATUSBAR_DOC_ENC);
                    }
                    break;
                default:
                    break;
            }
            break;
        }
        case WM_CTLCOLORBTN:
        {
            //SendMessage((HWND)lParam, WM_SETFONT, (WPARAM) on_theme_font_hwnd(), 0);
            on_statusbar_update_btn((HWND)lParam);
            return on_dark_set_contorl_color(wParam);
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
            printf("status WM_THEMECHANGED\n");
            if (on_dark_enable())
            {
                on_dark_allow_window(hwnd, true);
                on_statusbar_dark_release(false);
                UpdateWindow(hwnd);
            }
            break;
        }
        case WM_DESTROY:
        {
            if (g_menu_1)
            {
                DestroyMenu(g_menu_1);
            }
            if (g_menu_2)
            {
                DestroyMenu(g_menu_2);
            }
            if (g_menu_3)
            {
                DestroyMenu(g_menu_3);
            } 
            DestroyWindow(GetDlgItem(hwnd, IDM_BTN_1));
            DestroyWindow(GetDlgItem(hwnd, IDM_BTN_2));
            if (hfont_btn)
            {
                DeleteObject(hfont_btn);
                hfont_btn = NULL;
            }            
            g_statusbar = NULL;
            printf("statusbar WM_DESTROY\n");
            break;
        }
        default:
            break;
    }
    return CallWindowProc((WNDPROC) old_proc, hwnd, message, wParam, lParam);
}

static void
set_file_by_info(time_t *ptime)
{
    if (g_statusbar)
    {
        TCHAR s_hp[MAX_PATH+1] = {0};
        TCHAR m_hp[MAX_PATH+1] = {0};
        TCHAR file_time[100+1] = {0};
        eu_i18n_load_str(IDS_STATUS_F1, m_hp, MAX_PATH);
        struct tm *tm = localtime(ptime);
        if (tm)
        {
            sntprintf(file_time, 100, _T("%d-%d-%d %02d:%02d:%02d"), tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);  
        }
        sntprintf(s_hp, MAX_PATH, m_hp, TabCtrl_GetItemCount(g_tabpages), *file_time ? file_time : _T("0"));
        on_statusbar_set_text(g_statusbar, 0, s_hp);
    }
}

static unsigned WINAPI 
set_remotefs_fileinfo(void * lp)
{
    eu_tabpage *pnode = (eu_tabpage *)lp;
    if (util_availed_char(pnode->fs_server.networkaddr[0]))
    {
        CURL *curl = NULL;
        curl_off_t filetime = 0;
        char *url = eu_utf16_utf8(pnode->pathfile, NULL);
        if (!url)
        {
            return 1;
        }
        if ((curl = on_remote_init_socket(url, &pnode->fs_server)) == NULL)
        {
            printf("on_remote_init_socket return false\n");
            free(url);
            return 1;
        }
        eu_curl_easy_setopt(curl, CURLOPT_FILETIME, 1L);
        eu_curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
        eu_curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
        CURLcode res = eu_curl_easy_perform(curl);
        if (res == CURLE_OK)
        {
            res = eu_curl_easy_getinfo(curl, CURLINFO_FILETIME_T, &filetime);
        }
        if (res == CURLE_OK && filetime > 0)
        {
            pnode->st_mtime = (time_t)filetime;
        }
        free(url);
        eu_curl_easy_cleanup(curl);
    }
    if (pnode->st_mtime > 0)
    {
        set_file_by_info(&pnode->st_mtime);
    }
    return 0;
}

void __stdcall
on_statusbar_update_fileinfo(eu_tabpage *pnode, const TCHAR *print_str)
{
    int  count = 0;
    if (!g_statusbar)
    {
        return;
    } 
    if (print_str)
    {
        on_statusbar_set_text(g_statusbar, 0, print_str);
        InvalidateRect(g_statusbar, NULL, TRUE);
        UpdateWindow(g_statusbar);
        return;
    }
    if (!pnode)
    {
        TCHAR s_hp[FILESIZE] = {0};
        LOAD_I18N_RESSTR(IDS_STATUS_F1, m_hp);
        _sntprintf(s_hp, FILESIZE-1, m_hp, count, _T("0"));
        on_statusbar_set_text(g_statusbar, 0, s_hp);
        return;
    }
    if (pnode->fs_server.networkaddr[0])
    {
        CloseHandle((HANDLE)_beginthreadex(NULL,0,&set_remotefs_fileinfo, (void *)pnode, 0, NULL));
    }
    else
    {
        struct _stat buf = {0};
        if (pnode->is_blank && !pnode->st_mtime)
        {
            pnode->st_mtime = time(NULL);
        }
        if (_tstat(pnode->pathfile, &buf) != 0)
        {
            buf.st_mtime = pnode->st_mtime;
        }        
        set_file_by_info(&buf.st_mtime);
    }
}

void WINAPI
on_statusbar_update_line(eu_tabpage *pnode)
{
    int  count = 0;
    TCHAR s_xy[FILESIZE] = {0};
    TCHAR m_xy[MAX_LOADSTRING]  = {0};
    if (!g_statusbar)
    {
        return;
    } 
    if (!pnode)
    {
        eu_i18n_load_str(IDS_STATUS_XY, m_xy, 0);
        _sntprintf(s_xy, FILESIZE-1, m_xy, 0, 0);
    }
    else if (pnode->hex_mode)
    {
        eu_i18n_load_str(IDS_STATUS_HXY, m_xy, 0);
        _sntprintf(s_xy, FILESIZE-1, m_xy, SendMessage(pnode->hwnd_sc, HVM_GETHEXADDR, 0, 0));
    }
    else
    {
        sptr_t pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
        sptr_t lineno = eu_sci_call(pnode, SCI_LINEFROMPOSITION, pos, 0);
        sptr_t row = eu_sci_call(pnode, SCI_POSITIONFROMLINE, lineno, 0);
        eu_i18n_load_str(IDS_STATUS_XY, m_xy, 0);
        _sntprintf(s_xy, FILESIZE - 1, m_xy, lineno+1, pos-row+1);
    }
    on_statusbar_set_text(g_statusbar, 1, s_xy);
}

void WINAPI
on_statusbar_update_filesize(eu_tabpage *pnode)
{
    sptr_t nsize = 0;
    sptr_t line = 0;
    sptr_t ns_start = 0;
    sptr_t ns_end = 0;
    TCHAR file_size[FILESIZE + 1] = {0};
    if (!g_statusbar)
    {
        return;
    } 
    if (pnode)
    {
        nsize = eu_sci_call(pnode, SCI_GETLENGTH, 0, 0) + pnode->pre_len;
        ns_start = eu_sci_call(pnode, SCI_GETSELECTIONSTART, 0, 0);
        ns_end = eu_sci_call(pnode, SCI_GETSELECTIONEND, 0, 0);
        if (ns_end - ns_start > 0)
        {
            sptr_t nfirst = eu_sci_call(pnode, SCI_LINEFROMPOSITION, ns_start, 0);
            sptr_t nlast = eu_sci_call(pnode, SCI_LINEFROMPOSITION, ns_end, 0);
            line = nlast - nfirst + 1;
        }
        else if (nsize >= 0)
        {
            line = eu_sci_call(pnode, SCI_GETLINECOUNT, 0, 0);
        }          
    }
    if (ns_end - ns_start > 0)
    {
        LOAD_I18N_RESSTR(IDS_STATUS_LD, s_ld);
        _sntprintf(file_size, FILESIZE, s_ld, ns_end - ns_start, line);
    }
    else
    {
        LOAD_I18N_RESSTR(IDS_STATUS_LC, s_lc);
        _sntprintf(file_size, FILESIZE, s_lc, nsize, line);
    }
    if (*file_size)
    {
        on_statusbar_set_text(g_statusbar, 5, file_size);
    }
}

void WINAPI
on_statusbar_update_eol(eu_tabpage *pnode)
{
    if(!pnode || pnode->hex_mode)
    {
        set_menu_check(g_menu_1, IDM_LBREAK_1, IDM_LBREAK_4, IDM_LBREAK_4, STATUSBAR_DOC_EOLS);
        return;
    }
    switch (eu_sci_call(pnode, SCI_GETEOLMODE, 0, 0))
    {
        case 0:
            set_menu_check(g_menu_1, IDM_LBREAK_1, IDM_LBREAK_4, IDM_LBREAK_1, STATUSBAR_DOC_EOLS);
            break;
        case 1:
            set_menu_check(g_menu_1, IDM_LBREAK_1, IDM_LBREAK_4, IDM_LBREAK_2, STATUSBAR_DOC_EOLS);
            break;
        case 2:
            set_menu_check(g_menu_1, IDM_LBREAK_1, IDM_LBREAK_4, IDM_LBREAK_3, STATUSBAR_DOC_EOLS);
            break;
        default:
            set_menu_check(g_menu_1, IDM_LBREAK_1, IDM_LBREAK_4, IDM_LBREAK_4, STATUSBAR_DOC_EOLS);
            break;
    }
}

static void
on_statusbar_update_filetype_menu(eu_tabpage *pnode)
{
    bool res = false;
    if (g_statusbar && pnode && pnode->doc_ptr)
    {
        int index;
        doctype_t *doc_ptr;
        for (index = 1, doc_ptr = eu_doc_get_ptr(); doc_ptr && doc_ptr->doc_type; index++, doc_ptr++)
        {
            if (pnode->doc_ptr && (pnode->doc_ptr == doc_ptr))
            {
                set_menu_check(g_menu_3, IDM_TYPES_0, IDM_TYPES_0 + VIEW_FILETYPE_MAXCOUNT-1, IDM_TYPES_0 + index, STATUSBAR_DOC_TYPE);
                res = true;
            }
        }    
    }  
    if (!(pnode && res))
    {
        set_menu_check(g_menu_3, IDM_TYPES_0, IDM_TYPES_0 + VIEW_FILETYPE_MAXCOUNT-1, IDM_TYPES_0, STATUSBAR_DOC_TYPE);
    }
}

void WINAPI
on_statusbar_update_coding(eu_tabpage *pnode, const int res_id)
{
    int type = IDM_UNKNOWN;
    if (!g_statusbar)
    {
        return;
    }
    if (res_id)
    {
        type = res_id;
    }
    else if (pnode && pnode->codepage)
    {
        type = pnode->codepage;
    }
    switch (type)
    {
        case IDM_UNI_UTF8:
        case IDM_UNI_UTF8B:
        case IDM_UNI_UTF16LE:
        case IDM_UNI_UTF16LEB:
        case IDM_UNI_UTF16BE:
        case IDM_UNI_UTF16BEB:
        case IDM_UNI_UTF32LE:
        case IDM_UNI_UTF32BE:
        case IDM_UNI_ASCII:
            set_menu_check(g_menu_2, IDM_UNI_UTF8, IDM_UNI_ASCII, type, STATUSBAR_DOC_ENC);
            break;
        case IDM_ANSI_1:
        case IDM_ANSI_2:
        case IDM_ANSI_3:
        case IDM_ANSI_4:
        case IDM_ANSI_5:
        case IDM_ANSI_6:
        case IDM_ANSI_7:
        case IDM_ANSI_8:
        case IDM_ANSI_9:
        case IDM_ANSI_10:
        case IDM_ANSI_11:
        case IDM_ANSI_12:
        case IDM_ANSI_13:
        case IDM_ANSI_14:
            set_menu_check(g_menu_2, IDM_ANSI_1, IDM_ANSI_14, type, STATUSBAR_DOC_ENC);
            break;
        case IDM_ISO_1:
        case IDM_ISO_2:
        case IDM_ISO_3:
        case IDM_ISO_4:
        case IDM_ISO_5:
        case IDM_ISO_6:
        case IDM_ISO_7:
        case IDM_ISO_8:
        case IDM_ISO_9:
        case IDM_ISO_10:
        case IDM_ISO_11:
        case IDM_ISO_13:
        case IDM_ISO_15:
        case IDM_ISO_16:
        case IDM_ISO_JP:
        case IDM_ISO_KR:
        case IDM_ISO_CN:
            set_menu_check(g_menu_2, IDM_ISO_1, IDM_ISO_CN, type, STATUSBAR_DOC_ENC);
            break;
        case IDM_IBM_1:
        case IDM_IBM_2:
        case IDM_IBM_3:
            set_menu_check(g_menu_2, IDM_IBM_1, IDM_IBM_3, type, STATUSBAR_DOC_ENC);
            break;
        case IDM_EUC_1:
        case IDM_EUC_2:
            set_menu_check(g_menu_2, IDM_EUC_1, IDM_EUC_2, type, STATUSBAR_DOC_ENC);
            break;
        case IDM_OTHER_1:
        case IDM_OTHER_2:
        case IDM_OTHER_3:
        case IDM_OTHER_ANSI:
        case IDM_OTHER_BIN:
        case IDM_UNKNOWN:
            set_menu_check(g_menu_2, IDM_OTHER_1, IDM_UNKNOWN, type, STATUSBAR_DOC_ENC);
            break;
        default:
            break;
    }
}

static void
on_statusbar_create_filetype_menu(void)
{
    if (g_menu_3)
    {
        int index;
        doctype_t *doc_ptr;
        for (index = 1, doc_ptr = eu_doc_get_ptr(); doc_ptr&&doc_ptr->doc_type; index++, doc_ptr++)
        {
            TCHAR *desc = NULL;
            if ((desc = eu_utf8_utf16(doc_ptr->filedesc, NULL)) != NULL)
            {
                AppendMenu(g_menu_3, MF_POPUP | MF_STRING, IDM_TYPES_0 + index, desc);
                free(desc);
            }
        }
    }
}

int WINAPI
on_statusbar_height(void)
{
    return g_status_height;
}

void WINAPI 
on_statusbar_update(void)
{
    eu_tabpage *pnode = on_tabpage_focus_at();
    if (pnode && pnode->hwnd_sc)
    {
        set_btn_rw(pnode, true);
        on_statusbar_update_fileinfo(pnode, NULL);
        on_statusbar_update_line(pnode);
        on_statusbar_update_filesize(pnode);
        on_statusbar_update_eol(pnode);
        on_statusbar_update_filetype_menu(pnode->hex_mode ? NULL : pnode);
        on_statusbar_update_coding(pnode->hex_mode ? NULL : pnode, pnode->hex_mode ? IDM_OTHER_BIN : 0);
    }    
}

void WINAPI 
on_statusbar_dark_mode(void)
{
    if (on_dark_enable())
    {
        const int buttons[] = {IDM_BTN_1, IDM_BTN_2};
        for (int id = 0; id < _countof(buttons); ++id) 
        {
            HWND btn = GetDlgItem(g_statusbar, buttons[id]);
            on_dark_set_theme(btn, L"Explorer", NULL);
        }
        on_dark_set_theme(g_statusbar, L"Explorer", NULL);
    }
}

void WINAPI 
on_statusbar_dark_release(bool off)
{
    const int buttons[] = {IDM_BTN_1, IDM_BTN_2};
    for (int id = 0; id < _countof(buttons); ++id) 
    {
        HWND btn = GetDlgItem(g_statusbar, buttons[id]);
        on_dark_allow_window(btn, !off);
        SendMessage(btn, WM_THEMECHANGED, 0, 0);
    }
}

bool WINAPI
on_statusbar_init(HWND hwnd)
{
    bool ret = false;
    const uint32_t dw_style = SBT_NOBORDERS | WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE;
    if (g_statusbar)
    {
        DestroyWindow(g_statusbar);
    }
    g_statusbar = CreateWindowEx(WS_EX_COMPOSITED, STATUSCLASSNAME, NULL, dw_style, 0, 0, 0, 0, hwnd, (HMENU)IDC_STATUSBAR, eu_module_handle(), 0);
    do
    {
        if (!g_statusbar)
        {
            break;
        }
        // no simple status bar, to allow owner draw for dark mode
        if (SendMessage(g_statusbar, SB_ISSIMPLE, 0, 0)) 
        {
            SendMessage(g_statusbar, SB_SIMPLE, 0, 0);
        }
        if (!(old_proc = SetWindowLongPtr(g_statusbar, GWLP_WNDPROC, (LONG_PTR) stbar_proc)))
        {
            break;
        }
        g_menu_1 = i18n_load_menu(IDR_LBREAK_MENU);
        g_menu_2 = i18n_load_menu(IDR_CODEING_MENU);
        g_menu_3 = i18n_load_menu(IDR_TYPES_MENU);
        if (!(g_menu_1 && g_menu_2 && g_menu_3))
        {
            printf("create menu failed\n");
            break;
        }
        else
        {
            g_menu_1 = GetSubMenu(g_menu_1, 0);
            g_menu_2 = GetSubMenu(g_menu_2, 0);
            g_menu_3 = GetSubMenu(g_menu_3, 0);
        }
        on_statusbar_create_filetype_menu();
        ret = create_button(g_statusbar);
    } while(0);
    if (ret && on_dark_enable())
    {
        on_statusbar_dark_mode();
    }
    return ret;
}
