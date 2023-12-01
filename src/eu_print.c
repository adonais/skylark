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

// 保存设备对象...
static HGLOBAL dev_mode;
static HGLOBAL dev_names;

static print_set *
on_print_init(void)
{
    print_set *ptr_print = (print_set *) ((uintptr_t) eu_get_config() + (uintptr_t) offsetof(struct eu_config, eu_print));
    if (!ptr_print)
    {
        return NULL;
    }
    if (ptr_print->rect.left == -1 || ptr_print->rect.top == -1 || ptr_print->rect.right == -1 || ptr_print->rect.bottom == -1)
    {
        TCHAR localeInfo[3];
        GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IMEASURE, localeInfo, 3);
        if (localeInfo[0] == _T('0')) // Metric system. '1' is US System
        {
            ptr_print->rect.left = 2000;
            ptr_print->rect.top = 2000;
            ptr_print->rect.right = 2000;
            ptr_print->rect.bottom = 2000;
        }
        else
        {
            ptr_print->rect.left = 1000;
            ptr_print->rect.top = 1000;
            ptr_print->rect.right = 1000;
            ptr_print->rect.bottom = 1000;
        }
    }
    return ptr_print;
}

static void
on_print_file_info(LPCTSTR path, DWORD attr, SHFILEINFO *psfi, uint32_t cb_info, uint32_t flags)
{
    CoInitialize(NULL);
    if (_taccess(path, 0) != -1)
    {
        TCHAR filename[MAX_PATH] = {0};
        TCHAR extname[_MAX_EXT] = {0};
        _tsplitpath(path, NULL, NULL, filename, extname);
        if (_tcslen(extname) > 0)
        {
            _tcsncat(filename, extname, MAX_PATH-1);
        }
        DWORD_PTR dw = SHGetFileInfo(path, attr, psfi, cb_info, flags);
        if (_tcslen(psfi->szDisplayName) < _tcslen(filename))
        {
            _tcsncat(psfi->szDisplayName, extname, _countof(psfi->szDisplayName));
        }
    }
    else
    {
        _sntprintf(psfi->szDisplayName, MAX_PATH - 1, _T("%s"), path);
    }
    CoUninitialize();
}

static bool
on_print_get_fonts(LPWSTR pface_name, uint16_t *psize)
{
    HDC hdc;
    int pix_y;
    HMODULE ux_theme;
    HTHEME h_theme;
    LOGFONT lf;
    bool m_succeed = false;
    hdc = GetDC(NULL);
    pix_y = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(NULL, hdc);
    if ((ux_theme = LoadLibrary(_T("uxtheme.dll"))))
    {
        if ((bool)(GetProcAddress(ux_theme, "IsAppThemed"))())
        {
            h_theme = (HTHEME)(INT_PTR)((OpenThemeDataPtr)GetProcAddress(ux_theme, "OpenThemeData"))(NULL, _T("WINDOWSTYLE;WINDOW"));
            if (h_theme)
            {
                if (S_OK == (HRESULT)((GetThemeSysFontPtr)GetProcAddress(ux_theme, "GetThemeSysFont"))(h_theme, /*TMT_MSGBOXFONT*/ 805, &lf))
                {
                    if (lf.lfHeight < 0)
                    {
                        lf.lfHeight = -lf.lfHeight;
                    }
                    *psize = (uint16_t) MulDiv(lf.lfHeight, 72, pix_y);
                    if (*psize == 0)
                    {
                        *psize = 8;
                    }
                    _tcsncpy(pface_name, lf.lfFaceName, LF_FACESIZE);
                    m_succeed = true;
                }
                ((CloseThemeDataPtr)GetProcAddress(ux_theme, "CloseThemeData"))(h_theme);
            }
        }
        FreeLibrary(ux_theme);
    }
    return (m_succeed);
}

inline bool
template_is_dialogex(const DLGTEMPLATE *ptemplate)
{

    return ((DLGTEMPLATEEX *) ptemplate)->signature == 0xFFFF;
}

inline bool
template_has_font(const DLGTEMPLATE *ptemplate)
{

    return (DS_SETFONT & (template_is_dialogex(ptemplate) ? ((DLGTEMPLATEEX *) ptemplate)->style : ptemplate->style));
}

inline int
template_attr_size(bool m_dlg)
{

    return (int) sizeof(uint16_t) * (m_dlg ? 3 : 1);
}

static uint8_t *
get_template_field(const DLGTEMPLATE *ptemplate)
{
    uint16_t *pw;
    bool m_dlg = template_is_dialogex(ptemplate);
    if (m_dlg)
    {
        pw = (uint16_t *) ((DLGTEMPLATEEX *) ptemplate + 1);
    }
    else
    {
        pw = (uint16_t *) (ptemplate + 1);
    }
    if (*pw == (uint16_t) -1)
    {
        pw += 2;
    }
    else
    {
        while (*pw++);
    }
    if (*pw == (uint16_t) -1)
    {
        pw += 2;
    }
    else
    {
        while (*pw++);
    }
    while (*pw++);
    return (uint8_t *) pw;
}

static DLGTEMPLATE *
load_template_theme(LPCTSTR dlg_id, HINSTANCE hinst)
{
    HRSRC hr_src;
    HGLOBAL hr_mem;
    DLGTEMPLATE *pr_mem;
    DLGTEMPLATE *ptemplate;
    uint32_t template_size = 0;
    TCHAR face_name[LF_FACESIZE];
    uint16_t font_size;
    bool m_dlg;
    bool has_font;
    int font_attr;
    int cb_new;
    int cb_old;
    uint8_t *pb_new;
    uint8_t *pb;
    uint8_t *old_controls;
    uint8_t *new_controls;
    uint16_t m_ctl;
    hr_src = FindResource(hinst, dlg_id, RT_DIALOG);
    if (hr_src == NULL) 
    {
        return NULL;
    }
    hr_mem = LoadResource(hinst, hr_src);
    pr_mem = (DLGTEMPLATE *) LockResource(hr_mem);
    template_size = (uint32_t) SizeofResource(hinst, hr_src);
    if ((template_size == 0) || (ptemplate = LocalAlloc(LPTR, template_size + LF_FACESIZE * 2)) == NULL)
    {
        UnlockResource(hr_mem);
        FreeResource(hr_mem);
        return NULL;
    }
    memcpy((uint8_t *) ptemplate, pr_mem, (size_t) template_size);
    UnlockResource(hr_mem);
    FreeResource(hr_mem);
    if (!on_print_get_fonts(face_name, &font_size))
    {
        return ptemplate;
    }
    m_dlg = template_is_dialogex(ptemplate);
    has_font = template_has_font(ptemplate);
    font_attr = template_attr_size(m_dlg);
    if (m_dlg)
    {
        ((DLGTEMPLATEEX *) ptemplate)->style |= DS_SHELLFONT;
    }
    else
    {
        ptemplate->style |= DS_SHELLFONT;
    }
    cb_new = font_attr + ((lstrlen(face_name) + 1) * sizeof(TCHAR));
    pb_new = (uint8_t *) face_name;
    pb = get_template_field(ptemplate);
    cb_old = (int) (has_font ? font_attr + 2 * (_tcslen((TCHAR *) (pb + font_attr)) + 1) : 0);
    old_controls = (uint8_t *) (((DWORD_PTR) pb + cb_old + 3) & ~(DWORD_PTR) 3);
    new_controls = (uint8_t *) (((DWORD_PTR) pb + cb_new + 3) & ~(DWORD_PTR) 3);
    m_ctl = m_dlg ? (uint16_t)((DLGTEMPLATEEX *) ptemplate)->cDlgItems : (uint16_t) ptemplate->cdit;
    if (cb_new != cb_old && m_ctl > 0)
    {
        memmove(new_controls, old_controls, (size_t)(template_size - (old_controls - (uint8_t *) ptemplate)));
    }
    *(uint16_t *) pb = font_size;
    memmove(pb + font_attr, pb_new, (size_t)(cb_new - font_attr));
    return (ptemplate);
}

//=============================================================================
//
//  edit_print_setup() - Code from SciTE
//
//  Custom controls: Zoom IDC_PAGESETUP_ZOOMLEVEL_EDIT
//                   spin IDC_PAGESETUP_ZOOMLEVEL_CTLS
//                   Header  IDC_PAGESETUP_HEADER_LIST
//                   Footer  IDC_PAGESETUP_FOOTER_LIST
//                   Colors  IDC_PAGESETUP_COLOR_MODE_LIST
//
static UINT_PTR CALLBACK
on_print_proc(HWND hwnd, uint32_t msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            TCHAR *p1, *p2;
            TCHAR tch[ENV_LEN] = {0};
            SendDlgItemMessage(hwnd, IDC_PAGESETUP_ZOOMLEVEL_EDIT, EM_LIMITTEXT, 32, 0);
            SendDlgItemMessage(hwnd, IDC_PAGESETUP_ZOOMLEVEL_CTLS, UDM_SETRANGE, 0, MAKELONG((short) 20, (short) -10));
            SendDlgItemMessage(hwnd, IDC_PAGESETUP_ZOOMLEVEL_CTLS, UDM_SETPOS, 0, MAKELONG((short) eu_get_config()->eu_print.zoom, 0));

            // Set header options
            eu_i18n_load_str(IDS_PRINT_HEADER, tch, ENV_LEN - 1);
            _tcsncat(tch, _T("|"), ENV_LEN - 1);
            p1 = tch;
            while ((p2 = _tcschr(p1, _T('|'))))
            {
                *p2++ = _T('\0');
                if (*p1)
                {
                    SendDlgItemMessage(hwnd, IDC_PAGESETUP_HEADER_LIST, CB_ADDSTRING, 0, (LPARAM) p1);
                }
                p1 = p2;
            }
            SendDlgItemMessage(hwnd, IDC_PAGESETUP_HEADER_LIST, CB_SETCURSEL, (WPARAM) eu_get_config()->eu_print.header, 0);

            // Set footer options
            eu_i18n_load_str(IDS_PRINT_FOOTER, tch, _countof(tch));
            _tcsncat(tch, _T("|"), ENV_LEN - 1);
            p1 = tch;
            while ((p2 = _tcschr(p1, _T('|'))))
            {
                *p2++ = _T('\0');
                if (*p1)
                {
                    SendDlgItemMessage(hwnd, IDC_PAGESETUP_FOOTER_LIST, CB_ADDSTRING, 0, (LPARAM) p1);
                }
                p1 = p2;
            }
            SendDlgItemMessage(hwnd, IDC_PAGESETUP_FOOTER_LIST, CB_SETCURSEL, (WPARAM) eu_get_config()->eu_print.footer, 0);

            // Set color options
            eu_i18n_load_str(IDS_PRINT_COLOR, tch, _countof(tch));
            _tcsncat(tch, _T("|"), ENV_LEN - 1);
            p1 = tch;
            while ((p2 = _tcschr(p1, _T('|'))))
            {
                *p2++ = _T('\0');
                if (*p1)
                {
                    SendDlgItemMessage(hwnd, IDC_PAGESETUP_COLOR_MODE_LIST, CB_ADDSTRING, 0, (LPARAM) p1);
                }
                p1 = p2;
            }
            SendDlgItemMessage(hwnd, IDC_PAGESETUP_COLOR_MODE_LIST, CB_SETCURSEL, (WPARAM) eu_get_config()->eu_print.color_mode, 0);

            // Make combos handier
            SendDlgItemMessage(hwnd, IDC_PAGESETUP_HEADER_LIST, CB_SETEXTENDEDUI, true, 0);
            SendDlgItemMessage(hwnd, IDC_PAGESETUP_FOOTER_LIST, CB_SETEXTENDEDUI, true, 0);
            SendDlgItemMessage(hwnd, IDC_PAGESETUP_COLOR_MODE_LIST, CB_SETEXTENDEDUI, true, 0);
            SendDlgItemMessage(hwnd, IDC_PAGESETUP_SOURCE_LIST, CB_SETEXTENDEDUI, true, 0);
            SendDlgItemMessage(hwnd, IDC_PAGESETUP_ORIENTATION_LIST, CB_SETEXTENDEDUI, true, 0);
            if (on_dark_enable())
            {
                on_dark_set_theme(GetDlgItem(hwnd, IDOK), L"Explorer", NULL);
                on_dark_set_theme(GetDlgItem(hwnd, IDCANCEL), L"Explorer", NULL);
                on_dark_set_theme(GetDlgItem(hwnd, IDC_PAGESETUP_PRINTER), L"Explorer", NULL); 
                const int buttons[] = {IDC_PAGESETUP_PAGER_BOX,
                                       IDC_PAGESETUP_ORIENTATION_BOX,
                                       IDC_PAGESETUP_PORTRAIT,
                                       IDC_PAGESETUP_LANDSCAPE,
                                       IDC_PAGESETUP_MARGIN_BOX,
                                       IDC_PAGESETUP_HEADER_FOOTER_BOX,
                                       IDC_PAGESETUP_PRINT_COLOR_BOX,
                                       IDC_ZOOM_STATIC,
                                       IDC_PREVIEW_STATIC,
                                       IDC_PAGESETUP_SOURCE_LIST,
                                       IDC_PAGESETUP_ORIENTATION_LIST,
                                       IDC_PAGESETUP_HEADER_LIST,
                                       IDC_PAGESETUP_FOOTER_LIST,
                                       IDC_PAGESETUP_COLOR_MODE_LIST
                                       };
                for (int id = 0; id < _countof(buttons); ++id) 
                {
                    HWND btn = GetDlgItem(hwnd, buttons[id]);
                    // remove theme for BS_AUTORADIOBUTTON
                    on_dark_set_theme(btn, L"", L"");
                }
                on_dark_set_theme(hwnd, L"Explorer", NULL);
            }            
            return 1;
        }
        case WM_DPICHANGED:
            eu_window_layout_dpi(hwnd, (RECT*)lParam, 0);
            break;        
        case WM_THEMECHANGED:
        {
            if (on_dark_enable())
            {
                on_dark_allow_window(hwnd, true);
                on_dark_refresh_titlebar(hwnd);
                const int buttons[] = {IDOK, 
                                       IDCANCEL, 
                                       IDC_PAGESETUP_PRINTER};
                for (int id = 0; id < _countof(buttons); ++id) 
                {
                    HWND btn = GetDlgItem(hwnd, buttons[id]);
                    on_dark_allow_window(btn, true);
                    SendMessage(btn, WM_THEMECHANGED, 0, 0);
                }
                UpdateWindow(hwnd);
            }
            break;
        }
        CASE_WM_CTLCOLOR_SET:
        {
            return on_dark_set_contorl_color(wParam);
        }        
        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK)
            {
                int iPos = (int) SendDlgItemMessage(hwnd, IDC_PAGESETUP_ZOOMLEVEL_CTLS, UDM_GETPOS, 0, 0);
                if (HIWORD(iPos) == 0)
                {
                    eu_get_config()->eu_print.zoom = (int) (short) LOWORD(iPos);
                }
                else
                {
                    eu_get_config()->eu_print.zoom = 0;
                }
                eu_get_config()->eu_print.header = (int) SendDlgItemMessage(hwnd, IDC_PAGESETUP_HEADER_LIST, CB_GETCURSEL, 0, 0);
                eu_get_config()->eu_print.footer = (int) SendDlgItemMessage(hwnd, IDC_PAGESETUP_FOOTER_LIST, CB_GETCURSEL, 0, 0);
                eu_get_config()->eu_print.color_mode = (int) SendDlgItemMessage(hwnd, IDC_PAGESETUP_COLOR_MODE_LIST, CB_GETCURSEL, 0, 0);
            }
            else if (LOWORD(wParam) == IDC_PAGESETUP_PRINTER) 
            {
                PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(0x0402, 1), 0);
            }
            break;
        default:
            break;
    }
    return (0);
}

void
on_print_setup(HWND hwnd)
{
    PAGESETUPDLG pdlg = {0};
    print_set *ptr_print = on_print_init();
    if (!ptr_print)
    {
        return;
    }
    DLGTEMPLATE *ptemplate = load_template_theme(MAKEINTRESOURCE(IDD_PAGESETUP), g_skylark_lang);
    pdlg.lStructSize = sizeof(PAGESETUPDLG);
    pdlg.Flags = PSD_ENABLEPAGESETUPHOOK | PSD_ENABLEPAGESETUPTEMPLATEHANDLE;
    pdlg.lpfnPageSetupHook = on_print_proc;
    pdlg.hPageSetupTemplate = ptemplate;
    pdlg.hwndOwner = eu_module_hwnd();
    pdlg.hInstance = eu_module_handle();
    if (ptr_print->rect.left != 0 || ptr_print->rect.right != 0 || ptr_print->rect.top != 0 || ptr_print->rect.bottom != 0)
    {
        pdlg.Flags |= PSD_MARGINS;
        pdlg.rtMargin.left = ptr_print->rect.left;
        pdlg.rtMargin.top = ptr_print->rect.top;
        pdlg.rtMargin.right = ptr_print->rect.right;
        pdlg.rtMargin.bottom = ptr_print->rect.bottom;
    }
    if (PageSetupDlg(&pdlg))
    {
        ptr_print->rect.left = pdlg.rtMargin.left;
        ptr_print->rect.top = pdlg.rtMargin.top;
        ptr_print->rect.right = pdlg.rtMargin.right;
        ptr_print->rect.bottom = pdlg.rtMargin.bottom;
        dev_mode = pdlg.hDevMode;
        dev_names = pdlg.hDevNames;
    }
    LocalFree(ptemplate);
}

static UINT_PTR CALLBACK 
on_print_hook_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) 
    {
        case WM_INITDIALOG: 
        {
            if (on_dark_enable())
            {
                on_dark_set_theme(GetDlgItem(hwnd, IDOK), L"Explorer", NULL);
                on_dark_set_theme(GetDlgItem(hwnd, IDCANCEL), L"Explorer", NULL);
                on_dark_set_theme(GetDlgItem(hwnd, 0x401), L"Explorer", NULL);
                const int ctl[] = { 0x410, 0x420, 0x421, 0x422, 0x430, 0x431, 0x433, 0x473 };
                for (int i = 0; i < _countof(ctl); ++i)
                {
                    on_dark_set_theme(GetDlgItem(hwnd, ctl[i]), L"", L"");
                }
                on_dark_set_theme(hwnd, L"Explorer", NULL);
            }
            break;
        }
        CASE_WM_CTLCOLOR_SET:
            return on_dark_set_contorl_color(wParam);
        case WM_THEMECHANGED:
            if (on_dark_enable())
            {
                on_dark_allow_window(hwnd, true);
                on_dark_refresh_titlebar(hwnd);
                const int buttons[] = { IDOK, IDCANCEL, 0x401 };
                for (int id = 0; id < _countof(buttons); ++id) 
                {
                    HWND btn = GetDlgItem(hwnd, buttons[id]);
                    on_dark_allow_window(btn, true);
                    SendMessage(btn, WM_THEMECHANGED, 0, 0);
                }
                UpdateWindow(hwnd);
            }
            break;
        case WM_DPICHANGED:
            eu_window_layout_dpi(hwnd, (RECT*)lParam, 0);
            break;
        default:
            break;
    }
    return 0;
}

static void
on_print_draw_line(const HDC hdc, const uint32_t ta, const struct Sci_RangeToFormat *pfr_print, const int height, bool header)
{
    HPEN pen;
    HPEN pen_old;
    SetTextAlign(hdc, ta);
    pen = CreatePen(0, 1, RGB(0, 0, 0));
    pen_old = (HPEN) SelectObject(hdc, pen);
    MoveToEx(hdc, pfr_print->rc.left, (header ? pfr_print->rc.top : pfr_print->rc.bottom) - height / 4, NULL);
    LineTo(hdc, pfr_print->rc.right, (header ? pfr_print->rc.top : pfr_print->rc.bottom) - height / 4);
    SelectObject(hdc, pen_old);
    DeleteObject(pen);    
}

static void
on_print_footer(const HDC hdc, const HFONT font_footer, const TCHAR *str, const struct Sci_RangeToFormat *pfr_print, const int height)
{
    uint32_t ta = SetTextAlign(hdc, TA_TOP);
    RECT rcw = {pfr_print->rc.left, pfr_print->rc.bottom + height / 2, pfr_print->rc.right, pfr_print->rc.bottom + height + height / 2};
    SIZE size_footer;
    SetTextColor(hdc, RGB(0, 0, 0));
    SetBkColor(hdc, RGB(255, 255, 255));
    SelectObject(hdc, font_footer);    
    GetTextExtentPoint32(hdc, str, (int)_tcslen(str), &size_footer);
    ExtTextOut(hdc, pfr_print->rc.right - 5 - size_footer.cx, pfr_print->rc.bottom + height / 2, 0, &rcw, str, (uint32_t)_tcslen(str), NULL);
    on_print_draw_line(hdc, ta, pfr_print, height, false);
    
}

static void
on_print_header(const HDC hdc, const HFONT font_header, const TCHAR *title, const struct Sci_RangeToFormat *pfr_print, const int height, const int id)
{
    uint32_t ta = SetTextAlign(hdc, TA_BOTTOM);
    RECT rcw = {pfr_print->rc.left, pfr_print->rc.top - height - height / 2, pfr_print->rc.right, pfr_print->rc.top - height / 2};
    SetTextColor(hdc, RGB(0, 0, 0));
    SetBkColor(hdc, RGB(255, 255, 255));
    SelectObject(hdc, font_header);
    rcw.bottom = rcw.top + height;
    ExtTextOut(hdc, pfr_print->rc.left + 5, pfr_print->rc.top - height / 2, 0, &rcw, title, (uint32_t)_tcslen(title), NULL);
    // Print date in header
    if (id == 0 || id == 1)
    {
        SIZE size_info;
        TCHAR date_str[MAX_SIZE];
        // Get current date...
        SYSTEMTIME st;
        GetLocalTime(&st);
        GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, date_str, MAX_SIZE);
        // Get current time...
        if (id == 0)
        {
            TCHAR time_str[128];
            GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st, NULL, time_str, 128);
            _tcscat(date_str, _T(" "));
            _tcscat(date_str, time_str);
        }
        SelectObject(hdc, font_header);
        GetTextExtentPoint32(hdc, date_str, (int)_tcslen(date_str), &size_info);
        ExtTextOut(hdc, pfr_print->rc.right - 5 - size_info.cx, pfr_print->rc.top - height / 2, 0, &rcw, date_str, (uint32_t)_tcslen(date_str), NULL);
    }
    on_print_draw_line(hdc, ta, pfr_print, height, true);
}

static bool
on_print_layout(eu_tabpage *pnode, LPCTSTR doc_title, LPCTSTR page_fmt)
{
    HDC hdc;
    RECT rect_margins;
    RECT rect_phys_margins;
    RECT rect_setup;
    POINT pt_page;
    POINT pt_dpi;
    TEXTMETRIC tm;
    HFONT hold_font = NULL;
    HFONT font_header = NULL;
    HFONT font_footer = NULL;
    int start_pos;
    int end_pos;    
    int header_line_height;
    int footer_line_height;
    int doc_len;
    int doc_max;
    int len_printed;
    int page_num;
    bool print_page;
    TCHAR page_str[32];
    DOCINFO di = {sizeof(DOCINFO)};
    struct Sci_RangeToFormat fr_print;
    PRINTDLG pdlg = {sizeof(PRINTDLG)};
    print_set *ptr_print = on_print_init();
    if (!ptr_print)
    {
        return false;
    }
    if ((doc_len = (int)eu_sci_call(pnode, SCI_GETLENGTH, 0, 0)) <= 0)
    {   // 不打印空文档
        MSG_BOX(IDS_PRINT_EMPTY, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
        return true;
    }
    pdlg.hwndOwner = eu_module_hwnd();
    pdlg.hInstance = eu_module_handle();
    pdlg.Flags = PD_ENABLEPRINTHOOK | PD_USEDEVMODECOPIES | PD_ALLPAGES | PD_RETURNDC;
    pdlg.nFromPage = 1;
    pdlg.nToPage = 1;
    pdlg.nMinPage = 1;
    pdlg.nMaxPage = 0xffffU;
    pdlg.nCopies = 1;
    pdlg.hDC = 0;
    pdlg.hDevMode = dev_mode;
    pdlg.hDevNames = dev_names;
    pdlg.lpfnPrintHook = on_print_hook_proc;
    start_pos = (int) eu_sci_call(pnode, SCI_GETSELECTIONSTART, 0, 0);
    end_pos = (int) eu_sci_call(pnode, SCI_GETSELECTIONEND, 0, 0);
    if (start_pos == end_pos)
    {
        pdlg.Flags |= PD_NOSELECTION;
    }
    else
    {
        pdlg.Flags |= PD_SELECTION;
    }
    // 如果不显示对话框, 而使用标准设置, 使用下面的标志
    // pdlg.Flags |= PD_RETURNDEFAULT;
    if (!PrintDlg(&pdlg))
    {   // false 意味着错误 .....
        return true;
    }
    dev_mode = pdlg.hDevMode;
    dev_names = pdlg.hDevNames;
    hdc = pdlg.hDC;
    // 获取打印机分辨率
    pt_dpi.x = GetDeviceCaps(hdc, LOGPIXELSX); // dpi in X direction
    pt_dpi.y = GetDeviceCaps(hdc, LOGPIXELSY); // dpi in Y direction

    // 首先获取物理页面大小
    pt_page.x = GetDeviceCaps(hdc, PHYSICALWIDTH);  // device units
    pt_page.y = GetDeviceCaps(hdc, PHYSICALHEIGHT); // device units

    // Get the dimensions of the unprintable
    // part of the page (in device units).
    rect_phys_margins.left = GetDeviceCaps(hdc, PHYSICALOFFSETX);
    rect_phys_margins.top = GetDeviceCaps(hdc, PHYSICALOFFSETY);

    // To get the right and lower unprintable area,
    // we take the entire width and height of the paper and
    // subtract everything else.
    rect_phys_margins.right = pt_page.x                      // total paper width
                            - GetDeviceCaps(hdc, HORZRES)   // printable width
                            - rect_phys_margins.left;       // left unprintable margin

    rect_phys_margins.bottom = pt_page.y                      // total paper height
                             - GetDeviceCaps(hdc, VERTRES)   // printable height
                             - rect_phys_margins.top;        // right unprintable margin

    // At this point, rect_phys_margins contains the widths of the
    // unprintable regions on all four sides of the page in device units.

    // Take in account the page setup given by the user (if one value is not null)
    if (ptr_print->rect.left != 0 || ptr_print->rect.right != 0 || ptr_print->rect.top != 0 || ptr_print->rect.bottom != 0)
    {

        // Convert the hundredths of millimeters (HiMetric) or
        // thousandths of inches (HiEnglish) margin values
        // from the Page Setup dialog to device units.
        // (There are 2540 hundredths of a mm in an inch.)

        TCHAR localeInfo[3];
        GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IMEASURE, localeInfo, 3);

        if (localeInfo[0] == L'0')
        { // Metric system. L'1' is US System
            rect_setup.left = MulDiv(ptr_print->rect.left, pt_dpi.x, 2540);
            rect_setup.top = MulDiv(ptr_print->rect.top, pt_dpi.y, 2540);
            rect_setup.right = MulDiv(ptr_print->rect.right, pt_dpi.x, 2540);
            rect_setup.bottom = MulDiv(ptr_print->rect.bottom, pt_dpi.y, 2540);
        }
        else
        {
            rect_setup.left = MulDiv(ptr_print->rect.left, pt_dpi.x, 1000);
            rect_setup.top = MulDiv(ptr_print->rect.top, pt_dpi.y, 1000);
            rect_setup.right = MulDiv(ptr_print->rect.right, pt_dpi.x, 1000);
            rect_setup.bottom = MulDiv(ptr_print->rect.bottom, pt_dpi.y, 1000);
        }

        // Dont reduce margins below the minimum printable area
        rect_margins.left = max(rect_phys_margins.left, rect_setup.left);
        rect_margins.top = max(rect_phys_margins.top, rect_setup.top);
        rect_margins.right = max(rect_phys_margins.right, rect_setup.right);
        rect_margins.bottom = max(rect_phys_margins.bottom, rect_setup.bottom);
    }
    else
    {
        rect_margins.left = rect_phys_margins.left;
        rect_margins.top = rect_phys_margins.top;
        rect_margins.right = rect_phys_margins.right;
        rect_margins.bottom = rect_phys_margins.bottom;
    }
    // rect_margins now contains the values used to shrink the printable
    // area of the page.
    // Convert device coordinates into logical coordinates
    DPtoLP(hdc, (LPPOINT) &rect_margins, 2);
    DPtoLP(hdc, (LPPOINT) &rect_phys_margins, 2);
    // Convert page size to logical units and we're done!
    DPtoLP(hdc, (LPPOINT) &pt_page, 1);
    header_line_height = MulDiv(8, pt_dpi.y, 72);
    font_header = CreateFont(header_line_height, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 0, 0, _T("Arial"));
    hold_font = SelectObject(hdc, font_header);
    GetTextMetrics(hdc, &tm);
    header_line_height = tm.tmHeight + tm.tmExternalLeading;
    if (ptr_print->header == 3) 
    {
        header_line_height = 0;
    }
    footer_line_height = MulDiv(7, pt_dpi.y, 72);
    font_footer = CreateFont(footer_line_height, 0, 0, 0, FW_NORMAL, 0, 0, 0, 0, 0, 0, 0, 0, _T("Arial"));
    SelectObject(hdc, font_footer);
    GetTextMetrics(hdc, &tm);
    footer_line_height = tm.tmHeight + tm.tmExternalLeading;
    if (ptr_print->footer == 1)
    { 
        footer_line_height = 0;
    }
    di.lpszDocName = doc_title;
    di.lpszOutput = 0;
    di.lpszDatatype = 0;
    di.fwType = 0;
    if (StartDoc(hdc, &di) > 0)
    {   // 开始执行一个打印作业, 设置打印颜色
        int pmodes[5] = {SC_PRINT_NORMAL, SC_PRINT_INVERTLIGHT, SC_PRINT_BLACKONWHITE, SC_PRINT_COLOURONWHITE, SC_PRINT_COLOURONWHITEDEFAULTBG};
        eu_sci_call(pnode, SCI_SETPRINTCOLOURMODE, pmodes[ptr_print->color_mode], 0);
        // 打印放大设置
        eu_sci_call(pnode, SCI_SETPRINTMAGNIFICATION, (WPARAM) ptr_print->zoom, 0);
        doc_max = doc_len;
        len_printed = 0;
        // 打印所选内容
        if (pdlg.Flags & PD_SELECTION)
        {
            if (start_pos > end_pos)
            {
                len_printed = end_pos;
                doc_len = start_pos;
            }
            else
            {
                len_printed = start_pos;
                doc_len = end_pos;
            }
            if (len_printed < 0)
            {
                len_printed = 0;
            }
            if (doc_len > doc_max)
            {
                doc_len = doc_max;
            }
        }
        // 从可打印区域减去物理页边距
        fr_print.hdc = hdc;
        fr_print.hdcTarget = hdc;
        fr_print.rc.left = rect_margins.left - rect_phys_margins.left;
        fr_print.rc.top = rect_margins.top - rect_phys_margins.top;
        fr_print.rc.right = pt_page.x - rect_margins.right - rect_phys_margins.left;
        fr_print.rc.bottom = pt_page.y - rect_margins.bottom - rect_phys_margins.top;
        fr_print.rcPage.left = 0;
        fr_print.rcPage.top = 0;
        fr_print.rcPage.right = pt_page.x - rect_phys_margins.left - rect_phys_margins.right - 1;
        fr_print.rcPage.bottom = pt_page.y - rect_phys_margins.top - rect_phys_margins.bottom - 1;
        fr_print.rc.top += header_line_height + header_line_height / 2;
        fr_print.rc.bottom -= footer_line_height + footer_line_height / 2;
        // 从第1页开始, 打印每页
        page_num = 1;
        // Show wait cursor...
        util_wait_cursor(pnode);
        while (len_printed < doc_len)
        {
            print_page = (!(pdlg.Flags & PD_PAGENUMS) || ((page_num >= pdlg.nFromPage) && (page_num <= pdlg.nToPage)));
            _stprintf(page_str, page_fmt, page_num);
            if (!print_page)
            {
                break;
            }
            on_statusbar_update_fileinfo(pnode, page_str);
            // 打印机走纸, 打印内容
            StartPage(hdc);
            if (ptr_print->header < 3)
            {   // 打印页眉
                on_print_header(hdc, font_header, doc_title, &fr_print, header_line_height, ptr_print->header);
            }            
            fr_print.chrg.cpMin = len_printed;
            fr_print.chrg.cpMax = doc_len;
            len_printed = (int)eu_sci_call(pnode, SCI_FORMATRANGE, print_page, (LPARAM) &fr_print);
            if (ptr_print->footer == 0)
            {   // 打印页尾
                on_print_footer(hdc, font_footer, page_str, &fr_print, footer_line_height);
            }
            // 打印机停纸, 停止打印内容
            EndPage(hdc);
            ++page_num;
            if ((pdlg.Flags & PD_PAGENUMS) && (page_num > pdlg.nToPage))
            {
                break;
            }
        }
        eu_sci_call(pnode, SCI_FORMATRANGE, false, 0);
        // 结束打印作业
        EndDoc(hdc);        
    }
    if (hold_font)
    {
        SelectObject(hdc, hold_font);
    }
    if (hdc)
    {
        DeleteDC(hdc);
    }
    if (font_header)
    {
        DeleteObject(font_header);
    }
    if (font_footer)
    {
        DeleteObject(font_footer);
    }
    // Reset Statusbar to default mode
    on_statusbar_update_fileinfo(pnode, NULL);
    // Remove wait cursor...
    util_restore_cursor(pnode);
    return true;
}

int
on_print_file(eu_tabpage *pnode)
{
    if (pnode && pnode->pathfile[0])
    {
        if (pnode->pmod && pnode->plugin)
        {
            np_plugins_print(&pnode->plugin->funcs, &pnode->plugin->npp, NULL);
        }
        else if (!TAB_HEX_MODE(pnode))
        {
            SHFILEINFO shfi = {0};
            on_print_file_info(pnode->pathfile, 0, &shfi, sizeof(SHFILEINFO), SHGFI_DISPLAYNAME);
            if (_tcslen(shfi.szDisplayName) > 0)
            {
                TCHAR page_fmt[32];
                eu_i18n_load_str(IDS_PRINT_PAGENUM, page_fmt, _countof(page_fmt));
                if (!on_print_layout(pnode, shfi.szDisplayName, page_fmt))
                {
                    print_err_msg(IDS_PRINT_ERROR, shfi.szDisplayName);
                }
            }
        }
        else
        {
            MSG_BOX(IDS_PRINT_HEX_WARNS, IDC_MSG_ERROR, MB_ICONSTOP | MB_OK);
        }
    }
    return 0;
}
