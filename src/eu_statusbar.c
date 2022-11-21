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
#include <uxtheme.h>
#include <vssym32.h>

#define BTN_DEFAULT_WIDTH 60
#define STATUS_STATIC_FOCUS (RGB(0x48, 0x27, 0x30))
#define CAP_TOGGLED ((GetKeyState(VK_CAPITAL) & 1) != 0)
#define FILE_READONLY_COLOR 0x00800000

typedef struct _sb_borders
{
    int horizontal;
    int vertical;
    int between;
} sb_borders;

HWND g_statusbar = NULL;
static HMENU g_menu_break;
static HMENU g_menu_code;
static HMENU g_menu_type;
static HFONT hfont_btn;
static int g_status_height;

char iconv_undo_str[QW_SIZE] = {0};

static void
on_statusbar_btn(eu_tabpage *pnode, bool only_read)
{
    if (pnode && !pnode->plugin)
    {
        if (!only_read)
        {
            if (pnode->hex_mode)
            {
                SendMessage(pnode->hwnd_sc, HVM_SETBKCOLOR, 0, (LPARAM)eu_get_theme()->item.text.bgcolor);
            }
            else
            {
                on_sci_init_style(pnode);
                on_sci_after_file(pnode);
            }
            eu_sci_call(pnode, SCI_SETREADONLY, 0, 0);
            pnode->file_attr &= ~FILE_ATTRIBUTE_READONLY;
            pnode->file_attr &= ~FILE_READONLY_COLOR;
            InvalidateRect(g_tabpages, NULL, 0);
        }
        else
        {
            if (pnode->hex_mode)
            {
                SendMessage(pnode->hwnd_sc, HVM_SETBKCOLOR, 0, (LPARAM)STATUS_STATIC_FOCUS);
            }
            else
            {
                on_sci_init_default(pnode, STATUS_STATIC_FOCUS);
                on_sci_after_file(pnode);
            }
            eu_sci_call(pnode, SCI_SETREADONLY, 1, 0);
            pnode->file_attr |= (FILE_ATTRIBUTE_READONLY | FILE_READONLY_COLOR);
            InvalidateRect(g_tabpages, NULL, 0);
        }
    }
}

static void
on_statusbar_stream(eu_tabpage *pnode, bool only_read)
{
    if (pnode && !pnode->plugin)
    {
        bool r = (bool)eu_sci_call(pnode, SCI_GETREADONLY, 0, 0);
        if (!only_read)
        {
            if (r)
            {
                eu_sci_call(pnode, SCI_SETREADONLY, 0, 0);
            }
        }
        else if (!r)
        {
            if (pnode->file_attr & FILE_READONLY_COLOR)
            {
                eu_sci_call(pnode, SCI_SETREADONLY, 1, 0);
            }
            else
            {
                on_statusbar_btn(pnode, only_read);
            }
        }
    }
}

/*******************************************
 *m_auto为true则自动翻转,为false则手动翻转
 *返回翻转之后的状态,
 *0, 错误
 *1, 只读,
 *2, 可写.
 *******************************************/
int WINAPI
on_statusbar_btn_rw(eu_tabpage *pnode, bool m_auto)
{
    int ret = 0;
    uint32_t attr = 0;
    uint32_t grant = 0;
    TCHAR lpch[EDITNUMBS] = {0};
    if (!g_statusbar)
    {
        return 0;
    }
    HWND hrw = GetDlgItem(g_statusbar, IDM_BTN_RW);
    LOAD_I18N_RESSTR(IDS_BUTTON_R, rstr);
    LOAD_I18N_RESSTR(IDS_BUTTON_W, wstr);
    if (!hrw)
    {
        return 0;
    }
    if (!(pnode && *pnode->pathfile))
    {
        return 0;
    }
    if ((attr = GetFileAttributes(pnode->pathfile)) == INVALID_FILE_ATTRIBUTES)
    {
        if (pnode->is_blank)
        {
            Button_SetText(hrw, wstr);
            on_toolbar_update_button();
        }
        return 0;
    }
    Button_GetText(hrw, lpch, EDITNUMBS - 1);
    if (m_auto)
    {
        if (attr & FILE_ATTRIBUTE_READONLY)
        {
            Button_SetText(hrw, rstr);
            on_statusbar_stream(pnode, true);
            ret = 1;
        }
        else if (util_file_access(pnode->pathfile, &grant) && ((grant & FILE_GENERIC_WRITE) != FILE_GENERIC_WRITE))
        {
            Button_SetText(hrw, rstr);
            on_statusbar_stream(pnode, true);
            ret = 2;
        }
        else
        {
            Button_SetText(hrw, wstr);
            on_statusbar_stream(pnode, false);
            ret = 2;
        }
    }
    else if (util_file_access(pnode->pathfile, &grant) && ((grant & FILE_GENERIC_WRITE) != FILE_GENERIC_WRITE))
    {
        MSG_BOX(IDS_BUTTON_RW_TIPS, IDC_MSG_TIPS, MB_OK);
        ret = 0;
    }
    else if (_tcscmp(lpch, rstr) == 0)
    {
        if (attr & FILE_ATTRIBUTE_READONLY)
        {   // 去掉只读属性
            attr &= ~FILE_ATTRIBUTE_READONLY;
            SetFileAttributes(pnode->pathfile, attr);
        }
        Button_SetText(hrw, wstr);
        on_statusbar_btn(pnode, false);
        ret = 2;
    }
    else if (_tcscmp(lpch, wstr) == 0)
    {
        if (!(attr & FILE_ATTRIBUTE_READONLY))
        {   // 加上只读属性
            attr |= FILE_ATTRIBUTE_READONLY;
            SetFileAttributes(pnode->pathfile, attr);
            Button_SetText(hrw, rstr);
            on_statusbar_btn(pnode, true);
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
        WPARAM wParam = MAKEWPARAM(part, 0);
        SendMessage(hwnd, SB_SETTEXT, wParam, (LPARAM)lpsz);
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
        RECT rc = {0};
        GetClientRect(g_statusbar, &rc);
        g_status_height = rc.bottom - rc.top;
    }
}

static void
on_statusbar_adjust_btn(int left, int right)
{
    HWND hrw = GetDlgItem(g_statusbar, IDM_BTN_RW);
    if (hrw)
    {
        int btn_height = 0;
        int dpi = eu_get_dpi(g_statusbar);
        int btn_width = eu_dpi_scale_xy(dpi > 96 ? dpi - dpi/4 : dpi, BTN_DEFAULT_WIDTH);
        RECT rc_part = {0};
        SendMessage(g_statusbar, SB_GETRECT, STATUSBAR_DOC_BTN, (LPARAM)&rc_part);
        btn_height = rc_part.bottom - SPLIT_WIDTH;
        left = right - btn_width - 12;
        MoveWindow(hrw, left, SPLIT_WIDTH, btn_width, btn_height, TRUE);
        ShowWindow(hrw, SW_SHOW);
    }
}

void WINAPI
on_statusbar_refresh(void)
{
    if (g_statusbar && eu_get_config()->m_statusbar)
    {
        HWND hwnd = eu_module_hwnd();
        RECT rc_main = {0};
        GetWindowRect(hwnd, &rc_main);
        int cx = rc_main.right - rc_main.left;
        int n_half = cx / 8;
        int btn_half = n_half*7+70;
        int parts[] = {n_half*2, n_half*3, n_half*4, n_half*5+20, n_half*6+20, btn_half, -1};
        SendMessage(g_statusbar, SB_SETPARTS, STATUSBAR_PART, (LPARAM)&parts);
        on_statusbar_adjust_btn(btn_half, cx);
    }
}

static bool
on_statusbar_create_button(HWND hstatus)
{
    RECT rc = {0};
    TCHAR wstr[EDITNUMBS] = {0};
    HWND hrw = NULL;
    if (!eu_i18n_load_str(IDS_BUTTON_W, wstr, EDITNUMBS))
    {
        return false;
    }
    hrw = CreateWindowEx(0, _T("button"), wstr, WS_CHILD | WS_CLIPSIBLINGS | BS_FLAT, 0, 0, 0, 0, hstatus, (HMENU) IDM_BTN_RW, eu_module_handle(), NULL);
    if (!hrw)
    {
        printf("CreateWindowEx IDM_BTN_RW failed\n");
    }
    return (hrw != NULL);
}

static void
on_statusbar_menu_check(HMENU hmenu, int first_id, int last_id, int id, int parts)
{
    int len = 0;
    TCHAR buf[FILESIZE + 1] = {0xA554, 0x0020, 0};
    if (id == IDM_UNI_UTF16LE || id == IDM_UNI_UTF16BE)
    {
        ++id;   // IDM_UNI_UTF16LEB or IDM_UNI_UTF16BEB
    }
    if ((len = GetMenuString(hmenu, id, &buf[2], FILESIZE-2, MF_BYCOMMAND)) > 0)
    {
        for (int i = IDM_UNI_UTF8; hmenu == g_menu_code && i <= IDM_UNKNOWN; ++i)
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
on_statusbar_convert_coding(eu_tabpage *pnode, int encoding)
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
    _snprintf(iconv_undo_str, QW_SIZE-1, "%s=%d=%d", "_iconv/?@#$%^&*()`/~", pnode->codepage, encoding);
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
            TrackPopupMenu(g_menu_break, TPM_LEFTALIGN | TPM_BOTTOMALIGN, pt->x, pt->y, 0, g_statusbar, NULL);
            break;
        case STATUSBAR_DOC_ENC:
            TrackPopupMenu(g_menu_code, TPM_LEFTALIGN | TPM_BOTTOMALIGN, pt->x, pt->y, 0, g_statusbar, NULL);
            break;
        case STATUSBAR_DOC_TYPE:
            TrackPopupMenu(g_menu_type, TPM_LEFTALIGN | TPM_BOTTOMALIGN, pt->x, pt->y, 0, g_statusbar, NULL);
            break;
    }
}

static HFONT
on_statusbar_default_font(void)
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
        hfont_btn = CreateFontIndirect(&logfont);
    }
    return hfont_btn;
}

static void
on_statusbar_update_btn(HWND hwnd)
{
    if (hfont_btn || (on_statusbar_default_font()))
    {
        SendMessage(hwnd, WM_SETFONT, (WPARAM)hfont_btn, 0);
    }
}

static LRESULT CALLBACK
on_statusbar_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR sub_id, DWORD_PTR dwRefData)
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
            RECT rc = {0};
            GetClientRect(hwnd, &rc);
            FillRect((HDC)wParam, &rc, (HBRUSH)on_dark_get_brush());
            return 1;
        }
        case WM_PAINT:
        {
            if (!on_dark_enable())
            {
                break;
            }
            PAINTSTRUCT ps;
            sb_borders borders = {0};
            const COLORREF edge_color = 0x646464;
            SendMessage(hwnd, SB_GETBORDERS, 0, (LPARAM)&borders);
            intptr_t style = GetWindowLongPtr(hwnd, GWL_STYLE);
            bool is_grip = style & SBARS_SIZEGRIP;
            HDC hdc = BeginPaint(hwnd, &ps);
            HPEN hpen = CreatePen(PS_SOLID, 1, edge_color);
            HPEN hold_pen = (HPEN)(SelectObject(hdc, hpen));
            HFONT hold_font = (HFONT)SelectObject(hdc, on_statusbar_default_font());
            FillRect(hdc, &ps.rcPaint, (HBRUSH)on_dark_get_brush());
            wchar_t str[MAX_PATH] = {0};
            int nparts = (int)SendMessage(hwnd, SB_GETPARTS, 0, 0);
            for (int i = 0; i < nparts; ++i)
            {
                RECT rc_part = {0};
                SendMessage(hwnd, SB_GETRECT, i, (LPARAM)&rc_part);
                RECT rc_intersect = {0};
                if (!IntersectRect(&rc_intersect, &rc_part, &ps.rcPaint))
                {
                    continue;
                }
                POINT edges[] =
                {
                    {rc_part.right - 2, rc_part.top + 1},
                    {rc_part.right - 2, rc_part.bottom - 3}
                };
                Polyline(hdc, edges, _countof(edges));
                RECT rc_divider = {rc_part.right - borders.vertical, rc_part.top, rc_part.right, rc_part.bottom };
                DWORD text_len = LOWORD(SendMessage(hwnd, SB_GETTEXTLENGTH, i, 0));
                if (text_len < 1 || text_len > MAX_PATH - 1)
                {
                    continue;
                }
                SendMessage(hwnd, SB_GETTEXT, i, (LPARAM)str);
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, rgb_dark_txt_color);
                rc_part.left += borders.between;
                rc_part.right -= borders.vertical;
                DrawText(hdc, str, eu_int_cast(wcslen(str)), &rc_part, DT_SINGLELINE | DT_VCENTER | DT_LEFT);
                if (!is_grip && i < (nparts - 1))
                {
                    FillRect(hdc, &rc_divider, (HBRUSH)on_dark_get_hot_brush());
                }
            }
            SelectObject(hdc, hold_font);
            SelectObject(hdc, hold_pen);
            DeleteObject(hpen);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_STATUS_REFRESH:
        {
            on_statusbar_refresh();
            return 1;
        }
        case WM_MOVE:
        {
            on_statusbar_refresh();
            on_statusbar_update();
            break;
        }
        case WM_SIZE:
        {
            break;
        }
        case WM_COMMAND:
        {
            if (HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDM_BTN_RW)
            {
                if ((pnode = on_tabpage_focus_at()) && pnode->hwnd_sc)
                {
                    on_statusbar_btn_rw(pnode, false);
                    // Maybe affect this part, refresh it
                    on_statusbar_update_filesize(pnode);
                    PostMessage(eu_module_hwnd(), WM_ACTIVATE, MAKEWPARAM(WA_CLICKACTIVE, 0), 0);
                }
                return 1;
            }
            uint16_t id_menu = LOWORD(wParam);
            if (id_menu >= IDM_TYPES_0 && id_menu <= IDM_TYPES_0 + VIEW_FILETYPE_MAXCOUNT-1)
            {
                if (on_view_switch_type(id_menu - IDM_TYPES_0 - 1) == 0)
                {
                    on_statusbar_menu_check(g_menu_type, IDM_TYPES_0, IDM_TYPES_0 + VIEW_FILETYPE_MAXCOUNT-1, id_menu, STATUSBAR_DOC_TYPE);
                }
                break;
            }
            if (!(pnode = on_tabpage_focus_at()))
            {
                break;
            }
            if (id_menu > IDM_UNI_UTF8 && id_menu < IDM_LBREAK_3)
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
                    if (!on_edit_convert_eols(pnode, id_menu-IDM_LBREAK_1))
                    {
                        on_statusbar_menu_check(g_menu_break, IDM_LBREAK_1, IDM_LBREAK_3, id_menu, STATUSBAR_DOC_EOLS);
                    }
                    break;
                }
                case IDM_UNI_UTF8:
                    pnode->pre_len = 0;
                    memset(pnode->pre_context, 0, sizeof(pnode->pre_context));
                    _snprintf(iconv_undo_str, QW_SIZE-1, "%s=%d=%d", "_iconv/?@#$%^&*()`/~", pnode->codepage, IDM_UNI_UTF8);
                    pnode->codepage = IDM_UNI_UTF8;
                    on_tabpage_editor_modify(pnode, iconv_undo_str);
                    on_statusbar_menu_check(g_menu_code, IDM_UNI_UTF8, IDM_UNI_UTF32BE, id_menu, STATUSBAR_DOC_ENC);
                    break;
                case IDM_UNI_UTF8B:
                    pnode->pre_len = 3;
                    memset(pnode->pre_context, 0, sizeof(pnode->pre_context));
                    memcpy(pnode->pre_context, "\xEF\xBB\xBF", 3);
                    _snprintf(iconv_undo_str, QW_SIZE-1, "%s=%d=%d", "_iconv/?@#$%^&*()`/~", pnode->codepage, IDM_UNI_UTF8B);
                    pnode->codepage = IDM_UNI_UTF8B;
                    on_tabpage_editor_modify(pnode, iconv_undo_str);
                    on_statusbar_menu_check(g_menu_code, IDM_UNI_UTF8, IDM_UNI_UTF32BE, id_menu, STATUSBAR_DOC_ENC);
                    break;
                case IDM_UNI_UTF16LE:
                case IDM_UNI_UTF16LEB:
                case IDM_UNI_UTF16BEB:
                case IDM_UNI_UTF32LE:
                case IDM_UNI_UTF32BE:
                    if (!on_statusbar_convert_coding(pnode, id_menu))
                    {
                        on_statusbar_menu_check(g_menu_code, IDM_UNI_UTF8, IDM_UNI_UTF32BE, id_menu, STATUSBAR_DOC_ENC);
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
                    if (!on_statusbar_convert_coding(pnode, id_menu))
                    {
                        on_statusbar_menu_check(g_menu_code, IDM_ANSI_1, IDM_ANSI_14, id_menu, STATUSBAR_DOC_ENC);
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
                case IDM_ISO_KR:
                case IDM_ISO_CN:
                case IDM_ISO_JP_2:
                case IDM_ISO_JP_2004:
                case IDM_ISO_JP_MS:
                    if (!on_statusbar_convert_coding(pnode, id_menu))
                    {
                        on_statusbar_menu_check(g_menu_code, IDM_ISO_1, IDM_ISO_JP_MS, id_menu, STATUSBAR_DOC_ENC);
                    }
                    break;
                case IDM_IBM_1:
                case IDM_IBM_2:
                case IDM_IBM_3:
                    if (!on_statusbar_convert_coding(pnode, id_menu))
                    {
                        on_statusbar_menu_check(g_menu_code, IDM_IBM_1, IDM_IBM_3, id_menu, STATUSBAR_DOC_ENC);
                    }
                    break;
                case IDM_EUC_1:
                case IDM_EUC_2:
                    if (!on_statusbar_convert_coding(pnode, id_menu))
                    {
                        on_statusbar_menu_check(g_menu_code, IDM_EUC_1, IDM_EUC_2, id_menu, STATUSBAR_DOC_ENC);
                    }
                    break;
                case IDM_OTHER_HZ:
                case IDM_OTHER_1:
                case IDM_OTHER_2:
                case IDM_OTHER_3:
                case IDM_OTHER_ANSI:
                case IDM_OTHER_BIN:
                case IDM_UNKNOWN:
                    if (!on_statusbar_convert_coding(pnode, id_menu))
                    {
                        on_statusbar_menu_check(g_menu_code, IDM_OTHER_HZ, IDM_UNKNOWN, id_menu, STATUSBAR_DOC_ENC);
                    }
                    break;
                default:
                    break;
            }
            break;
        }
        case WM_CTLCOLORBTN:
        {
            on_statusbar_update_btn((HWND)lParam);
            return 1;
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
        case WM_NCDESTROY:
        {
            RemoveWindowSubclass(hwnd, on_statusbar_proc, sub_id);
            break;
        }
        case WM_DESTROY:
        {
            if (g_statusbar)
            {
                if (g_menu_break)
                {
                    DestroyMenu(g_menu_break);
                }
                if (g_menu_code)
                {
                    DestroyMenu(g_menu_code);
                }
                if (g_menu_type)
                {
                    DestroyMenu(g_menu_type);
                }
                DestroyWindow(GetDlgItem(hwnd, IDM_BTN_RW));
                if (hfont_btn)
                {
                    DeleteObject(hfont_btn);
                    hfont_btn = NULL;
                }
                g_statusbar = NULL;
                printf("statusbar WM_DESTROY\n");
            }
            break;
        }
        default:
            break;
    }
    return DefSubclassProc(hwnd, message, wParam, lParam);
}

static void
on_statusbar_file_info(time_t filetime)
{
    if (g_statusbar)
    {
        TCHAR s_hp[MAX_PATH+1] = {0};
        TCHAR m_hp[MAX_PATH+1] = {0};
        TCHAR file_time[100+1] = {0};
        eu_i18n_load_str(IDS_STATUS_F1, m_hp, MAX_PATH);
        struct tm *tm = localtime(&filetime);
        if (tm)
        {
            sntprintf(file_time, 100, _T("%d-%d-%d %02d:%02d:%02d"), tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
        }
        sntprintf(s_hp, MAX_PATH, m_hp, TabCtrl_GetItemCount(g_tabpages), *file_time ? file_time : _T("0"));
        on_statusbar_set_text(g_statusbar, 0, s_hp);
    }
}

void __stdcall
on_statusbar_update_fileinfo(eu_tabpage *pnode, const TCHAR *print_str)
{
    if (g_statusbar && pnode && eu_get_config()->m_statusbar)
    {
        print_str ? on_statusbar_set_text(g_statusbar, 0, print_str) : on_statusbar_file_info(pnode->st_mtime);
    }
}

void WINAPI
on_statusbar_update_line(eu_tabpage *pnode)
{
    int  count = 0;
    TCHAR s_xy[FILESIZE] = {0};
    TCHAR m_xy[MAX_LOADSTRING]  = {0};
    if (!(g_statusbar && pnode && eu_get_config()->m_statusbar))
    {
        return;
    }
    if (pnode->hex_mode)
    {
        eu_i18n_load_str(IDS_STATUS_HXY, m_xy, 0);
        _sntprintf(s_xy, FILESIZE-1, m_xy, SendMessage(pnode->hwnd_sc, HVM_GETHEXADDR, 0, 0));
    }
    else if (pnode->ac_mode == AUTO_CODE)
    {
        eu_i18n_load_str(IDS_SNIPPETS_STR, s_xy, 0);
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
    if (!(g_statusbar && pnode && eu_get_config()->m_statusbar))
    {
        return;
    }
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
    if (ns_end - ns_start > 0)
    {
        LOAD_I18N_RESSTR(IDS_STATUS_LD, s_ld);
        _sntprintf(file_size, FILESIZE, s_ld, ns_end - ns_start, line);
    }
    else
    {
        LOAD_I18N_RESSTR(pnode->hex_mode? IDS_STATUS_HLC : IDS_STATUS_LC, s_lc);
        if (!pnode->hex_mode)
        {
            _sntprintf(file_size, FILESIZE, s_lc, nsize, eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0));
        }
        else
        {
            _sntprintf(file_size, FILESIZE, s_lc, pnode->pmod ? (sptr_t)pnode->raw_size : nsize);
        }
    }
    if (*file_size)
    {
        on_statusbar_set_text(g_statusbar, STATUSBAR_DOC_SIZE, file_size);
    }
}

void WINAPI
on_statusbar_update_eol(eu_tabpage *pnode)
{
    if (!(g_statusbar && pnode && eu_get_config()->m_statusbar))
    {
        return;
    }
    if(pnode->hex_mode)
    {
        TCHAR buf[] = {0xA554, 0x0020, _T('N'), _T('a'), _T('N'), 0};
        on_statusbar_set_text(g_statusbar, STATUSBAR_DOC_EOLS, buf);
        return;
    }
    switch (eu_sci_call(pnode, SCI_GETEOLMODE, 0, 0))
    {
        case 0:
            on_statusbar_menu_check(g_menu_break, IDM_LBREAK_1, IDM_LBREAK_3, IDM_LBREAK_1, STATUSBAR_DOC_EOLS);
            break;
        case 1:
            on_statusbar_menu_check(g_menu_break, IDM_LBREAK_1, IDM_LBREAK_3, IDM_LBREAK_2, STATUSBAR_DOC_EOLS);
            break;
        case 2:
            on_statusbar_menu_check(g_menu_break, IDM_LBREAK_1, IDM_LBREAK_3, IDM_LBREAK_3, STATUSBAR_DOC_EOLS);
            break;
        default:
            break;
    }
}

static void
on_statusbar_update_filetype_menu(eu_tabpage *pnode)
{
    if (!(g_statusbar && pnode && eu_get_config()->m_statusbar))
    {
        return;
    }
    bool res = false;
    if (g_statusbar && pnode && pnode->doc_ptr)
    {
        int index;
        doctype_t *doc_ptr;
        for (index = 1, doc_ptr = eu_doc_get_ptr(); doc_ptr && doc_ptr->doc_type; index++, doc_ptr++)
        {
            if (pnode->doc_ptr && (pnode->doc_ptr == doc_ptr))
            {
                on_statusbar_menu_check(g_menu_type, IDM_TYPES_0, IDM_TYPES_0 + VIEW_FILETYPE_MAXCOUNT-1, IDM_TYPES_0 + index, STATUSBAR_DOC_TYPE);
                res = true;
            }
        }
    }
    if (!(pnode && res))
    {
        on_statusbar_menu_check(g_menu_type, IDM_TYPES_0, IDM_TYPES_0 + VIEW_FILETYPE_MAXCOUNT-1, IDM_TYPES_0, STATUSBAR_DOC_TYPE);
    }
}

void WINAPI
on_statusbar_update_coding(eu_tabpage *pnode, const int res_id)
{
    int type = IDM_UNKNOWN;
    if (!(g_statusbar && pnode && eu_get_config()->m_statusbar))
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
            on_statusbar_menu_check(g_menu_code, IDM_UNI_UTF8, IDM_UNI_UTF32BE, type, STATUSBAR_DOC_ENC);
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
            on_statusbar_menu_check(g_menu_code, IDM_ANSI_1, IDM_ANSI_14, type, STATUSBAR_DOC_ENC);
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
        case IDM_ISO_KR:
        case IDM_ISO_CN:
        case IDM_ISO_JP_2:
        case IDM_ISO_JP_2004:
        case IDM_ISO_JP_MS:
            on_statusbar_menu_check(g_menu_code, IDM_ISO_1, IDM_ISO_JP_MS, type, STATUSBAR_DOC_ENC);
            break;
        case IDM_IBM_1:
        case IDM_IBM_2:
        case IDM_IBM_3:
            on_statusbar_menu_check(g_menu_code, IDM_IBM_1, IDM_IBM_3, type, STATUSBAR_DOC_ENC);
            break;
        case IDM_EUC_1:
        case IDM_EUC_2:
            on_statusbar_menu_check(g_menu_code, IDM_EUC_1, IDM_EUC_2, type, STATUSBAR_DOC_ENC);
            break;
        case IDM_OTHER_HZ:
        case IDM_OTHER_1:
        case IDM_OTHER_2:
        case IDM_OTHER_3:
        case IDM_OTHER_ANSI:
        case IDM_OTHER_BIN:
        case IDM_UNKNOWN:
            on_statusbar_menu_check(g_menu_code, IDM_OTHER_HZ, IDM_UNKNOWN, type, STATUSBAR_DOC_ENC);
            break;
        default:
            break;
    }
}

static void
on_statusbar_create_filetype_menu(void)
{
    if (g_menu_type)
    {
        int index;
        doctype_t *doc_ptr;
        for (index = 1, doc_ptr = eu_doc_get_ptr(); doc_ptr&&doc_ptr->doc_type; index++, doc_ptr++)
        {
            TCHAR *desc = NULL;
            if ((desc = eu_utf8_utf16(doc_ptr->filedesc, NULL)) != NULL)
            {
                AppendMenu(g_menu_type, MF_POPUP | MF_STRING, IDM_TYPES_0 + index, desc);
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
    if (g_statusbar && eu_get_config()->m_statusbar)
    {
        eu_tabpage *pnode = on_tabpage_focus_at();
        if (pnode && pnode->hwnd_sc)
        {
            SendMessage(g_statusbar, WM_SETREDRAW, FALSE, 0);
            on_statusbar_update_fileinfo(pnode, NULL);
            on_statusbar_update_line(pnode);
            on_statusbar_update_filesize(pnode);
            on_statusbar_update_eol(pnode);
            on_statusbar_update_filetype_menu(pnode);
            on_statusbar_update_coding(pnode, pnode->hex_mode ? IDM_OTHER_BIN : 0);
            SendMessage(g_statusbar, WM_SETREDRAW, TRUE, 0);
            RedrawWindow(g_statusbar, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
        }
    }
}

void WINAPI
on_statusbar_dark_mode(void)
{
    if (g_statusbar && on_dark_enable())
    {
        const int buttons[] = {IDM_BTN_RW};
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
    const int buttons[] = {IDM_BTN_RW};
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
    const uint32_t dw_style = SBT_NOBORDERS | WS_CHILD | SBARS_SIZEGRIP;
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
        if (!(SetWindowSubclass(g_statusbar, on_statusbar_proc, STATUSBAR_SUBID, 0)))
        {
            break;
        }
        g_menu_break = menu_load(IDR_LBREAK_MENU);
        g_menu_code = menu_load(IDR_CODEING_MENU);
        g_menu_type = menu_load(IDR_TYPES_MENU);
        if (!(g_menu_break && g_menu_code && g_menu_type))
        {
            printf("create menu failed\n");
            break;
        }
        on_statusbar_create_filetype_menu();
        ret = on_statusbar_create_button(g_statusbar);
    } while(0);
    if (ret && on_dark_enable())
    {
        on_statusbar_dark_mode();
        SendMessage(g_statusbar, WM_STATUS_REFRESH, 0, 0);
        on_statusbar_update();
    }
    return ret;
}
