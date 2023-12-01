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
#include <uxtheme.h>
#include <vssym32.h>

#define BTN_DEFAULT_WIDTH 60
#define STATUS_STATIC_FOCUS (RGB(0x48, 0x27, 0x30))
#define CAP_TOGGLED ((GetKeyState(VK_CAPITAL) & 1) != 0)

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

void
on_statusbar_btn_colour(eu_tabpage *pnode, bool only_read)
{
    if (g_tabpages && pnode)
    {
        if (!only_read)
        {
            if (pnode->file_attr & FILE_READONLY_COLOR)
            {
                if (pnode->plugin)
                {
                    instance_theme theme = {0, (uint32_t)eu_get_theme()->item.text.bgcolor};
                    np_plugins_setvalue(&pnode->plugin->funcs, &pnode->plugin->npp, NV_ATTRIB_CHANGE, &theme);
                }
                else if (TAB_HEX_MODE(pnode))
                {
                    SendMessage(pnode->hwnd_sc, HVM_SETBKCOLOR, 0, (LPARAM)eu_get_theme()->item.text.bgcolor);
                }
                else
                {
                    on_sci_init_style(pnode);
                    on_sci_after_file(pnode, false);
                }
                eu_sci_call(pnode, SCI_SETREADONLY, 0, 0);
                pnode->file_attr &= ~FILE_ATTRIBUTE_READONLY;
                pnode->file_attr &= ~FILE_READONLY_COLOR;
                InvalidateRect(g_tabpages, NULL, 0);
            }
        }
        else if (!(pnode->file_attr & FILE_READONLY_COLOR))
        {
            if (pnode->plugin)
            {
                instance_theme theme = {1, 0x482730  /* STATUS_STATIC_FOCUS convert BGR */};
                np_plugins_setvalue(&pnode->plugin->funcs, &pnode->plugin->npp, NV_ATTRIB_CHANGE, &theme);
            }
            else if (TAB_HEX_MODE(pnode))
            {
                SendMessage(pnode->hwnd_sc, HVM_SETBKCOLOR, 0, (LPARAM)STATUS_STATIC_FOCUS);
            }
            else
            {
                on_sci_init_default(pnode, STATUS_STATIC_FOCUS);
                on_sci_after_file(pnode, false);
            }
            eu_sci_call(pnode, SCI_SETREADONLY, 1, 0);
            pnode->file_attr &= ~FILE_ATTRIBUTE_READONLY;
            pnode->file_attr |= (FILE_ATTRIBUTE_READONLY | FILE_READONLY_COLOR);
            InvalidateRect(g_tabpages, NULL, 0);
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
int
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
    if (!hrw || !pnode || !pnode->pathfile[0])
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
            on_statusbar_btn_colour(pnode, true);
            ret = 1;
        }
        else if (!url_has_samba(pnode->pathfile) && util_file_access(pnode->pathfile, &grant) && ((grant & FILE_GENERIC_WRITE) != FILE_GENERIC_WRITE))
        {
            Button_SetText(hrw, rstr);
            on_statusbar_btn_colour(pnode, true);
            ret = 2;
        }
        else
        {
            Button_SetText(hrw, wstr);
            on_statusbar_btn_colour(pnode, false);
            ret = 2;
        }
    }
    else if (!url_has_samba(pnode->pathfile) && util_file_access(pnode->pathfile, &grant) && ((grant & FILE_GENERIC_WRITE) != FILE_GENERIC_WRITE))
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
        on_statusbar_btn_colour(pnode, false);
        ret = 2;
    }
    else if (_tcscmp(lpch, wstr) == 0)
    {
        if (!(attr & FILE_ATTRIBUTE_READONLY))
        {   // 加上只读属性
            attr |= FILE_ATTRIBUTE_READONLY;
            SetFileAttributes(pnode->pathfile, attr);
            Button_SetText(hrw, rstr);
            on_statusbar_btn_colour(pnode, true);
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

int
on_statusbar_height(void)
{
    int status_height = 0;
    if (g_statusbar && eu_get_config()->m_statusbar)
    {
        status_height = eu_dpi_scale_xy(0, STATUSBAR_DEFHIGHT);
    }
    return status_height;
}

static void
on_statusbar_adjust_btn(int left, int right)
{
    HWND hrw = GetDlgItem(g_statusbar, IDM_BTN_RW);
    if (hrw)
    {
        if (left || right)
        {
            int btn_height = 0;
            int dpi = eu_get_dpi(g_statusbar);
            int btn_width = eu_dpi_scale_xy(dpi > 96 ? dpi - dpi/4 : dpi, BTN_DEFAULT_WIDTH);
            RECT rc_part = {0};
            SendMessage(g_statusbar, SB_GETRECT, STATUSBAR_DOC_BTN, (LPARAM)&rc_part);
            btn_height = rc_part.bottom - SPLIT_WIDTH;
            left = right - btn_width - 1;
            eu_setpos_window(hrw, HWND_TOP, left, SPLIT_WIDTH, btn_width, btn_height, SWP_SHOWWINDOW);
        }
        else 
        {
            eu_setpos_window(hrw, HWND_BOTTOM, 0, 0, 0, 0, SWP_HIDEWINDOW);
        }
    }
}

void
on_statusbar_size(eu_tabpage *pnode)
{
    if (g_statusbar)
    {
        if (!eu_get_config()->m_statusbar)
        {
            on_statusbar_adjust_btn(0, 0);
            eu_setpos_window(g_statusbar, HWND_BOTTOM, 0, 0, 0, 0, SWP_HIDEWINDOW);
        }
        else if (pnode)
        {
            on_statusbar_btn_rw(pnode, true);
        }
        else
        {
            RECT rc = {0};
            GetClientRect(eu_hwnd_self(), &rc);
            const int height = on_statusbar_height();
            int cx = rc.right - rc.left;
            int n_half = cx / 8;
            int btn_half = n_half*7+70;
            int parts[] = {n_half*2, n_half*3, n_half*4, n_half*5+20, n_half*6+20, btn_half, -1};
            SendMessage(g_statusbar, SB_SETPARTS, STATUSBAR_PART, (LPARAM)&parts);
            on_statusbar_adjust_btn(btn_half, cx);
            MoveWindow(g_statusbar, 0, rc.bottom - height, cx, height, TRUE);
            ShowWindow(g_statusbar, SW_SHOW);
        }
    }
}

static bool
on_statusbar_create_button(const HWND hstatus)
{
    RECT rc = {0};
    TCHAR wstr[EDITNUMBS] = {0};
    HWND hrw = NULL;
    if (eu_i18n_load_str(IDS_BUTTON_W, wstr, EDITNUMBS))
    {
        hrw = CreateWindowEx(0, _T("button"), wstr, WS_CHILD | WS_CLIPSIBLINGS | BS_FLAT, 0, 0, 0, 0, hstatus, (HMENU) IDM_BTN_RW, eu_module_handle(), NULL);
        if (hrw)
        {
            on_theme_update_font(btn_id);
        }
    }
    return (hrw != NULL);
}

static void
on_statusbar_menu_check(HMENU hmenu, const int first_id, const int last_id, int id, const int group_id, const int parts)
{
    int len = 0;
    TCHAR buf[FILESIZE + 1] = {0xA554, 0x0020, 0};
    if (id == IDM_UNI_UTF16LE || id == IDM_UNI_UTF16BE)
    {
        ++id;   // IDM_UNI_UTF16LEB or IDM_UNI_UTF16BEB
    }
    if ((len = GetMenuString(hmenu, id, &buf[2], FILESIZE-2, MF_BYCOMMAND)) > 0)
    {
        if (STATUSBAR_DOC_ENC == parts)
        {
            for (int i = IDM_UNI_UTF8; hmenu == g_menu_code && i <= IDM_UNKNOWN; ++i)
            {
                CheckMenuItem(hmenu, i, MF_BYCOMMAND | MFS_UNCHECKED);
            }
            if (group_id >= 0 && group_id <= 5)
            {
                CheckMenuRadioItem(hmenu, 0, 5, group_id, MF_BYPOSITION);
            }
        }
        if (STATUSBAR_DOC_TYPE == parts)
        {
            int ch = 0;
            HMENU htype = NULL;
            for (int i = IDM_TYPES_0; hmenu == g_menu_type && i <= IDM_TYPES_Z; ++i)
            {
                CheckMenuItem(hmenu, i, MF_BYCOMMAND | MFS_UNCHECKED);
            }
            if (IDM_TYPES_0 == id && CheckMenuRadioItem(hmenu, IDM_TYPES_0, IDM_TYPES_0, IDM_TYPES_0, MF_BYCOMMAND))
            {
                on_statusbar_set_text(g_statusbar, parts, buf);
            }
            else if (((ch = towupper(buf[2])) > 0x40 && ch < 0x5B) && (htype = GetSubMenu(hmenu, ch - 0x40)))
            {   // A_Z, 通过首字母获取父菜单位置索引
                CheckMenuRadioItem(hmenu, IDM_TYPES_A, IDM_TYPES_Z, IDM_TYPES_ZERO + ch - 0x40, MF_BYCOMMAND);
                CheckMenuRadioItem(htype, first_id, last_id, id, MF_BYCOMMAND);
                on_statusbar_set_text(g_statusbar, parts, buf);
            }
        }
        else if (buf[2] && CheckMenuRadioItem(hmenu, first_id, last_id, id, MF_BYCOMMAND))
        {
            on_statusbar_set_text(g_statusbar, parts, buf);
        }
    }
}

static int
on_statusbar_convert_coding(eu_tabpage *pnode, int encoding)
{
    char *file_buf = NULL;
    char *src_str = NULL;
    char *dst_str = NULL;
    size_t src_len = 0;
    size_t dst_len = 0;
    size_t file_len = 0;
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
    on_edit_convert_coding(pnode, encoding);
    on_encoding_set_bom_from_cp(pnode);
    return 0;
}

void
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

static LRESULT CALLBACK
on_statusbar_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR sub_id, DWORD_PTR dw)
{
    eu_tabpage *pnode = NULL;
    switch (message)
    {
        case WM_CREATE:
        {
            if (on_dark_enable())
            {
                on_statusbar_size(NULL);
                on_dark_set_theme(g_statusbar, L"Explorer", NULL);
            }
            else 
            {
                SendMessage(hwnd, WM_SETFONT, (WPARAM)on_theme_font_hwnd(), 0);
            }
            break;
        }
        case WM_ERASEBKGND:
        {
            if (!on_dark_enable())
            {
                break;
            }
            RECT rc = {0};
            GetClientRect(hwnd, &rc);
            FillRect((HDC)wParam, &rc, (HBRUSH)on_dark_get_bgbrush());
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
            HDC hdc = BeginPaint(hwnd, &ps);
            HPEN hpen = CreatePen(PS_SOLID, 1, edge_color);
            HPEN hold_pen = (HPEN)(SelectObject(hdc, hpen));
            HFONT hold_font = (HFONT)SelectObject(hdc, on_theme_font_hwnd());
            FillRect(hdc, &ps.rcPaint, (HBRUSH)on_dark_get_bgbrush());
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
                if (i < nparts)
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
        case WM_COMMAND:
        {
            if (HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDM_BTN_RW)
            {
                if ((pnode = on_tabpage_focus_at()) && pnode->hwnd_sc)
                {
                    on_statusbar_btn_rw(pnode, false);
                    // Maybe affect this part, refresh it
                    on_proc_msg_active(pnode);
                }
                return 1;
            }
            uint16_t id_menu = LOWORD(wParam);
            if (id_menu >= IDM_TYPES_0 && id_menu <= IDM_TYPES_0 + VIEW_FILETYPE_MAXCOUNT-1)
            {
                if (on_view_switch_type(id_menu - IDM_TYPES_0 - 1) == 0)
                {
                    on_statusbar_menu_check(g_menu_type, IDM_TYPES_0, IDM_TYPES_0 + VIEW_FILETYPE_MAXCOUNT-1, id_menu, -1, STATUSBAR_DOC_TYPE);
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
                        on_statusbar_menu_check(g_menu_break, IDM_LBREAK_1, IDM_LBREAK_3, id_menu, -1, STATUSBAR_DOC_EOLS);
                    }
                    break;
                }
                case IDM_UNI_UTF8:
                    pnode->pre_len = 0;
                    memset(pnode->pre_context, 0, sizeof(pnode->pre_context));
                    on_edit_convert_coding(pnode, IDM_UNI_UTF8);
                    on_statusbar_menu_check(g_menu_code, IDM_UNI_UTF8, IDM_UNI_UTF32BE, id_menu, 0, STATUSBAR_DOC_ENC);
                    break;
                case IDM_UNI_UTF8B:
                    pnode->pre_len = 3;
                    memset(pnode->pre_context, 0, sizeof(pnode->pre_context));
                    memcpy(pnode->pre_context, "\xEF\xBB\xBF", 3);
                    on_edit_convert_coding(pnode, IDM_UNI_UTF8B);
                    on_statusbar_menu_check(g_menu_code, IDM_UNI_UTF8, IDM_UNI_UTF32BE, id_menu, 0, STATUSBAR_DOC_ENC);
                    break;
                case IDM_UNI_UTF16LE:
                case IDM_UNI_UTF16LEB:
                case IDM_UNI_UTF16BEB:
                case IDM_UNI_UTF32LE:
                case IDM_UNI_UTF32BE:
                    if (!on_statusbar_convert_coding(pnode, id_menu))
                    {
                        on_statusbar_menu_check(g_menu_code, IDM_UNI_UTF8, IDM_UNI_UTF32BE, id_menu, 0, STATUSBAR_DOC_ENC);
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
                        on_statusbar_menu_check(g_menu_code, IDM_ANSI_1, IDM_ANSI_14, id_menu, 1, STATUSBAR_DOC_ENC);
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
                        on_statusbar_menu_check(g_menu_code, IDM_ISO_1, IDM_ISO_JP_MS, id_menu, 2, STATUSBAR_DOC_ENC);
                    }
                    break;
                case IDM_IBM_1:
                case IDM_IBM_2:
                case IDM_IBM_3:
                    if (!on_statusbar_convert_coding(pnode, id_menu))
                    {
                        on_statusbar_menu_check(g_menu_code, IDM_IBM_1, IDM_IBM_3, id_menu, 3, STATUSBAR_DOC_ENC);
                    }
                    break;
                case IDM_EUC_1:
                case IDM_EUC_2:
                    if (!on_statusbar_convert_coding(pnode, id_menu))
                    {
                        on_statusbar_menu_check(g_menu_code, IDM_EUC_1, IDM_EUC_2, id_menu, 4, STATUSBAR_DOC_ENC);
                    }
                    break;
                case IDM_OTHER_HZ:
                case IDM_OTHER_1:
                case IDM_OTHER_2:
                case IDM_OTHER_3:
                case IDM_OTHER_ANSI:
                case IDM_OTHER_BIN:
                case IDM_OTHER_PLUGIN:
                case IDM_UNKNOWN:
                    if (!on_statusbar_convert_coding(pnode, id_menu))
                    {
                        on_statusbar_menu_check(g_menu_code, IDM_OTHER_HZ, IDM_UNKNOWN, id_menu, 5, STATUSBAR_DOC_ENC);
                    }
                    break;
                default:
                    break;
            }
            break;
        }
        case WM_SETTINGCHANGE:
        {
            SendMessage(hwnd, WM_THEMECHANGED, 0, 0);
            break;
        }
        case WM_THEMECHANGED:
        {
            on_statusbar_dark_mode(on_dark_enable());
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
                    g_menu_break = NULL;
                }
                if (g_menu_code)
                {
                    DestroyMenu(g_menu_code);
                    g_menu_code = NULL;
                }
                if (g_menu_type)
                {
                    DestroyMenu(g_menu_type);
                    g_menu_type = NULL;
                }
                DestroyWindow(GetDlgItem(hwnd, IDM_BTN_RW));
                g_statusbar = NULL;
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

void
on_statusbar_update_fileinfo(eu_tabpage *pnode, const TCHAR *print_str)
{
    if (g_statusbar && pnode && eu_get_config()->m_statusbar)
    {
        print_str ? on_statusbar_set_text(g_statusbar, 0, print_str) : on_statusbar_file_info(pnode->st_mtime);
    }
}

void
on_statusbar_update_line(eu_tabpage *pnode)
{
    int  count = 0;
    TCHAR s_xy[FILESIZE] = {0};
    TCHAR m_xy[MAX_LOADSTRING]  = {0};
    if (!(g_statusbar && pnode && eu_get_config()->m_statusbar))
    {
        return;
    }
    if (TAB_HEX_MODE(pnode))
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
        sptr_t lineno = 0;
        sptr_t row = 0;
        util_postion_xy(pnode, -1, &lineno, &row);
        eu_i18n_load_str(IDS_STATUS_XY, m_xy, 0);
        _sntprintf(s_xy, FILESIZE - 1, m_xy, lineno, row);
    }
    on_statusbar_set_text(g_statusbar, 1, s_xy);
}

void
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
        _sntprintf(file_size, FILESIZE, s_ld, util_select_characters(pnode, ns_start, ns_end), line);
    }
    else
    {
        LOAD_I18N_RESSTR(TAB_HAS_TXT(pnode) ? IDS_STATUS_LC : IDS_STATUS_HLC, s_lc);
        if (TAB_HAS_TXT(pnode))
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

void
on_statusbar_update_eol(eu_tabpage *pnode, const int eol)
{
    if (!(g_statusbar && pnode && eu_get_config()->m_statusbar))
    {
        return;
    }
    if(TAB_HEX_MODE(pnode) || pnode->plugin)
    {
        TCHAR buf[] = {0xA554, 0x0020, _T('N'), _T('a'), _T('N'), 0};
        on_statusbar_set_text(g_statusbar, STATUSBAR_DOC_EOLS, buf);
        return;
    }
    switch (eol >= 0 ? eol : (pnode->eol >= 0 ? pnode->eol : eu_sci_call(pnode, SCI_GETEOLMODE, 0, 0)))
    {
        case SC_EOL_CRLF:
            on_statusbar_menu_check(g_menu_break, IDM_LBREAK_1, IDM_LBREAK_3, IDM_LBREAK_1, -1, STATUSBAR_DOC_EOLS);
            break;
        case SC_EOL_CR:
            on_statusbar_menu_check(g_menu_break, IDM_LBREAK_1, IDM_LBREAK_3, IDM_LBREAK_2, -1, STATUSBAR_DOC_EOLS);
            break;
        case SC_EOL_LF:
            on_statusbar_menu_check(g_menu_break, IDM_LBREAK_1, IDM_LBREAK_3, IDM_LBREAK_3, -1, STATUSBAR_DOC_EOLS);
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
                on_statusbar_menu_check(g_menu_type, IDM_TYPES_0, IDM_TYPES_0 + VIEW_FILETYPE_MAXCOUNT-1, IDM_TYPES_0 + index, -1, STATUSBAR_DOC_TYPE);
                res = true;
            }
        }
    }
    if (!(pnode && res))
    {
        on_statusbar_menu_check(g_menu_type, IDM_TYPES_0, IDM_TYPES_0 + VIEW_FILETYPE_MAXCOUNT-1, IDM_TYPES_0, -1, STATUSBAR_DOC_TYPE);
    }
}

void
on_statusbar_update_coding(eu_tabpage *pnode)
{
    int type = IDM_UNKNOWN;
    if (!(g_statusbar && pnode && eu_get_config()->m_statusbar))
    {
        return;
    }
    if (pnode)
    {
        type = TAB_HEX_MODE(pnode) ? IDM_OTHER_BIN : pnode->codepage;
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
            on_statusbar_menu_check(g_menu_code, IDM_UNI_UTF8, IDM_UNI_UTF32BE, type, 0, STATUSBAR_DOC_ENC);
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
            on_statusbar_menu_check(g_menu_code, IDM_ANSI_1, IDM_ANSI_14, type, 1, STATUSBAR_DOC_ENC);
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
            on_statusbar_menu_check(g_menu_code, IDM_ISO_1, IDM_ISO_JP_MS, type, 2, STATUSBAR_DOC_ENC);
            break;
        case IDM_IBM_1:
        case IDM_IBM_2:
        case IDM_IBM_3:
            on_statusbar_menu_check(g_menu_code, IDM_IBM_1, IDM_IBM_3, type, 3, STATUSBAR_DOC_ENC);
            break;
        case IDM_EUC_1:
        case IDM_EUC_2:
            on_statusbar_menu_check(g_menu_code, IDM_EUC_1, IDM_EUC_2, type, 4, STATUSBAR_DOC_ENC);
            break;
        case IDM_OTHER_HZ:
        case IDM_OTHER_1:
        case IDM_OTHER_2:
        case IDM_OTHER_3:
        case IDM_OTHER_ANSI:
        case IDM_OTHER_BIN:
        case IDM_OTHER_PLUGIN:
        case IDM_UNKNOWN:
            on_statusbar_menu_check(g_menu_code, IDM_OTHER_HZ, IDM_UNKNOWN, type, 5, STATUSBAR_DOC_ENC);
            break;
        default:
            break;
    }
}

static bool
on_statusbar_create_filetype_menu(void)
{
    if (g_menu_type)
    {
        int index;
        int ch = 0;
        doctype_t *doc_ptr;
        TCHAR *desc = NULL;
        HMENU htype = NULL;
        for (index = 1, doc_ptr = eu_doc_get_ptr(); doc_ptr&&doc_ptr->doc_type; index++, doc_ptr++)
        {
            if ((desc = eu_utf8_utf16(doc_ptr->filedesc, NULL)) != NULL)
            {
                if (((ch = towupper(*desc)) > 0x40 && ch < 0x5B) && (htype = GetSubMenu(g_menu_type, ch - 0x40)))
                {
                    if (GetMenuItemID(htype, 0) == IDM_TYPES_ZERO)
                    {
                        DeleteMenu(htype, 0, MF_BYPOSITION);
                    }
                    AppendMenu(htype, MF_POPUP | MF_STRING, IDM_TYPES_0 + index, desc);
                }
                free(desc);
            }
        }
        return true;
    }
    return false;
}

void
on_statusbar_update(eu_tabpage *psrc)
{
    eu_tabpage *pnode = psrc;
    if (g_statusbar && eu_get_config()->m_statusbar)
    {
        if ((pnode || (pnode = on_tabpage_focus_at())) && pnode->hwnd_sc)
        {
            SendMessage(g_statusbar, WM_SETREDRAW, FALSE, 0);
            on_statusbar_update_fileinfo(pnode, NULL);
            on_statusbar_update_line(pnode);
            on_statusbar_update_filesize(pnode);
            on_statusbar_update_eol(pnode, -1);
            on_statusbar_update_filetype_menu(pnode);
            on_statusbar_update_coding(pnode);
            SendMessage(g_statusbar, WM_SETREDRAW, TRUE, 0);
            RedrawWindow(g_statusbar, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
        }
    }
}

void
on_statusbar_destroy(void)
{
    if (g_statusbar)
    {
        DestroyWindow(g_statusbar);
        g_statusbar = NULL;
    }
}

void
on_statusbar_dark_mode(const bool dark)
{
    HWND btn = GetDlgItem(g_statusbar, IDM_BTN_RW);
    on_dark_allow_window(g_statusbar, dark);
    on_dark_allow_window(btn, dark);
    on_dark_set_theme(btn, L"Explorer", NULL);
}

int
on_statusbar_create_dlg(HWND hwnd)
{
    int ret = 1;
    const uint32_t style = SBT_NOBORDERS | WS_CHILD;
    if (g_statusbar)
    {
        DestroyWindow(g_statusbar);
    }
    g_statusbar = CreateWindowEx(WS_EX_COMPOSITED, STATUSCLASSNAME, NULL, style, 0, 0, 0, 0, hwnd, (HMENU)IDC_STATUSBAR, eu_module_handle(), 0);
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
            eu_logmsg("%s: create menu failed\n", __FUNCTION__);
            break;
        }
        if (!(on_statusbar_create_filetype_menu()))
        {
            eu_logmsg("%s: on_statusbar_create_filetype_menu failed\n", __FUNCTION__);
            break;
        }
        if (!on_statusbar_create_button(g_statusbar))
        {
            eu_logmsg("%s: on_statusbar_create_button failed\n", __FUNCTION__);
            break;
        }
        ret = 0;
    } while(0);
    return ret;
}
