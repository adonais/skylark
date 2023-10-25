/******************************************************************************
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

static HFONT g_hfont;
static HWND  hwnd_edit_tips;
static HBRUSH brush_linenumber;
static HBRUSH brush_foldmargin;
static HBRUSH brush_text;
static HBRUSH brush_caretline;
static HBRUSH brush_indicator;
static HBRUSH brush_activetab;
static HBRUSH brush_language;

static TCHAR uni_font[DW_SIZE+1];
static struct styletheme dlg_style;

static HWND hwnd_linenumber_static;
static HFONT font_linenumber_static;

static HWND hwnd_foldmargin_static;
static HFONT font_foldmargin_static;

static HWND hwnd_text_static;
static HFONT font_text_static;

static HWND hwnd_caretline_static;
static HFONT font_caretline_static;

static HWND hwnd_indicator_static;
static HFONT font_indicator_static;

static HWND hwnd_caret_static;
static HFONT font_caret_static;

static HWND hwnd_keyword_static;
static HFONT font_keyword_static;

static HWND hwnd_keyword2_static;
static HFONT font_keyword2_static;

static HWND hwnd_string_static;
static HFONT font_string_static;

static HWND hwnd_character_static;
static HFONT font_character_static;

static HWND hwnd_number_static;
static HFONT font_number_static;

static HWND hwnd_operator_static;
static HFONT font_operator_static;

static HWND hwnd_preprocessor_static;
static HFONT font_preprocessor_static;

static HWND hwnd_comment_static;
static HFONT font_comment_static;

static HWND hwnd_commentline_static;
static HFONT font_commentline_static;

static HWND hwnd_commentdoc_static;
static HFONT font_commentdoc_static;

static HWND hwnd_tags_static;
static HFONT font_tags_static;

static HWND hwnd_unknowtags_static;
static HFONT font_unknowtags_static;

static HWND hwnd_attributes_static;
static HFONT font_attributes_static;

static HWND hwnd_unknowattr_static;
static HFONT font_unknowattr_static;

static HWND hwnd_entities_static;
static HFONT font_entities_static;

static HWND hwnd_tagends_static;
static HFONT font_tagends_static;

static HWND hwnd_cdata_static;
static HFONT font_cdata_static;

static HWND hwnd_phpsection_static;
static HFONT font_phpsection_static;

static HWND hwnd_aspsection_static;
static HFONT font_aspsection_static;

static HWND hwnd_xmlsection_static;
static HFONT font_xmlsection_static;

static HWND hwnd_hyperlink_static;
static HFONT font_hyperlink_static;

static HWND hwnd_activetab_static;
static HFONT font_activetab_static;

static HWND hwnd_symbolic_static;
static HFONT font_symbolic_static;

static HWND hwnd_result_lineno_static;
static HWND hwnd_result_key_static;
static HFONT font_result_lineno_static;
static HFONT font_result_key_static;

static HWND hwnd_bracesection_static;
static HFONT font_bracesection_static;

static HWND hwnd_nchistory_static;
static HWND hwnd_nchistory_static2;
static HFONT font_nchistory_static;
static HFONT font_nchistory_static2;

static HWND hwnd_dochistory_static;
static HWND hwnd_dochistory_static2;
static HFONT font_dochistory_static;
static HFONT font_dochistory_static2;

static theme_query pm_query[] =
{
    {IDS_THEME_DESC_DEFAULT, {0}, _T("default")} ,
    {IDS_THEME_DESC_BLOCK  , {0}, _T("black")}   ,
    {IDS_THEME_DESC_WHITE  , {0}, _T("white")}   ,
    {0, {0}, {0}}
};

TCHAR *
on_theme_query_name(TCHAR *str)
{
    theme_query *iter = NULL;
    if (STR_IS_NUL(str))
    {
        return NULL;
    }
    for (iter = &pm_query[0]; iter->res_id; ++iter)
    {
        if (eu_i18n_load_str(iter->res_id, iter->desc, QW_SIZE-1))
        {
            if (_tcsicmp(iter->desc, str) == 0)
            {
                return iter->name;
            }
        }
    }
    return str;
}

int
on_theme_load_script(const TCHAR *ac_name)
{
    TCHAR script[MAX_BUFFER];
    char script_name[MAX_BUFFER] = {0};
    char name[QW_SIZE+1] = {0};
    _sntprintf(script, MAX_BUFFER, _T("%s\\conf\\conf.d\\eu_main.lua"), eu_module_path);
    if (!WideCharToMultiByte(CP_UTF8, 0, script, -1, script_name, MAX_BUFFER, NULL, NULL))
    {
        return EUE_API_CONV_FAIL;
    }
    if (!WideCharToMultiByte(CP_UTF8, 0, ac_name, -1, name, QW_SIZE, NULL, NULL))
    {
        return EUE_API_CONV_FAIL;
    }
    return do_lua_func(script_name, "switch_theme", name);
}

static bool
search_theme_files(theme_desc *lptheme, int m)
{
    WIN32_FIND_DATA data;
    TCHAR filepath[MAX_BUFFER];
    HANDLE hfile;
    int index = 0;
    _sntprintf(filepath, MAX_BUFFER, _T("%s\\styletheme*.conf"), eu_config_path);
    hfile = FindFirstFile(filepath, &data);
    if (hfile == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    do
    {
        if (_tcscmp(data.cFileName, _T(".")) == 0 || _tcscmp(data.cFileName, _T("..")) == 0)
        {
            continue;
        }
        if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            continue;
        }
        if (_tcsicmp(data.cFileName, _T("styletheme.conf")) == 0)
        {
            eu_i18n_load_str(IDS_THEME_DESC_DEFAULT, lptheme[index].desc, QW_SIZE-1);
            _tcsncpy(lptheme[index].name, _T("default"), QW_SIZE-1);
        }
        else if (_stscanf(data.cFileName + _tcslen(_T("styletheme_")), _T("%[^.]"), lptheme[index].name) == 1)
        {
            if (_tcsicmp(lptheme[index].name, _T("black")) == 0)
            {
                eu_i18n_load_str(IDS_THEME_DESC_BLOCK, lptheme[index].desc, QW_SIZE-1);
            }
            else if (_tcsicmp(lptheme[index].name, _T("white")) == 0)
            {
                eu_i18n_load_str(IDS_THEME_DESC_WHITE, lptheme[index].desc, QW_SIZE-1);
            }
        }
        else
        {   // 不符合命名规则
            continue;
        }
        ++index;
    } while (FindNextFile(hfile, &data) && m > index);
    FindClose(hfile);
    return true;
}

void
on_theme_update_item(void)
{
    HWND hwnd = eu_module_hwnd();
    HMENU root_menu = hwnd ? GetMenu(hwnd) : NULL;
    if (root_menu)
    {
        int index;
        HMENU menu_view;
        HMENU menu_theme;
        theme_desc ptheme[VIEW_STYLETHEME_MAXCOUNT] = { 0 };
        if (!(menu_view = GetSubMenu(root_menu, THEME_MENU)))
        {
            return;
        }
        if (!(menu_theme = GetSubMenu(menu_view, THEME_MENU_SUB)))
        {
            return;
        }
        if (!search_theme_files(ptheme, VIEW_STYLETHEME_MAXCOUNT))
        {
            return;
        }
        int count = GetMenuItemCount(menu_theme);
        for (index = 0; index < count; ++index)
        {
            DeleteMenu(menu_theme, 0, MF_BYPOSITION);
        }
        for (index = 0; index < VIEW_STYLETHEME_MAXCOUNT && ptheme[index].name[0]; ++index)
        {
            AppendMenu(menu_theme, MF_POPUP | MF_STRING, IDM_STYLETHEME_BASE + index, ptheme[index].desc[0]?ptheme[index].desc:ptheme[index].name);
        }
    }
}

HFONT
on_theme_font_hwnd(void)
{
    return g_hfont;
}

bool
on_theme_setup_font(HWND hwnd)
{
    if (g_hfont)
    {
        DeleteObject(g_hfont);
    }
    NONCLIENTMETRICS ncm = {sizeof(NONCLIENTMETRICS)};
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);
    LOGFONT logfont = ncm.lfMessageFont;
    int font_size = logfont.lfHeight < 0 ? -logfont.lfHeight : logfont.lfHeight;
    logfont.lfHeight = -MulDiv(font_size, eu_get_dpi(hwnd), USER_DEFAULT_SCREEN_DPI);
    g_hfont = CreateFontIndirect(&logfont);
    if (g_hfont)
    {
        SendMessage(hwnd, WM_SETFONT, (WPARAM) g_hfont, 0);
    }
    return (g_hfont != NULL);
}

void
eu_font_release(void)
{
    if (g_hfont)
    {
        DeleteObject(g_hfont);
        g_hfont = NULL;
    }
}

int
on_theme_copy_style(TCHAR *ac_theme)
{
    TCHAR pathfile[MAX_BUFFER] = { 0 };
    TCHAR old_path[MAX_BUFFER] = { 0 };
    if (!MultiByteToWideChar(CP_UTF8, 0, eu_get_theme()->pathfile, -1, old_path, MAX_BUFFER))
    {
        return EUE_API_CONV_FAIL;
    }
    _sntprintf(pathfile, MAX_BUFFER, _T("%s\\styletheme_%s.conf"), eu_config_path, ac_theme);
    if (!CopyFile(old_path, pathfile, true))
    {
        MSG_BOX(IDC_MSG_THEME_ERR4, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
        return EUE_COPY_FILE_ERR;
    }
    if (on_theme_load_script(ac_theme))
    {
        eu_logmsg("%s: on_theme_load_script return false\n", __FUNCTION__);
        return EUE_LOAD_SCRIPT_ERR;
    }
    strncpy(eu_get_config()->window_theme, eu_get_theme()->name, QW_SIZE - 1);
    on_theme_update_item();
    menu_switch_theme();
    on_view_modify_theme();
    return SKYLARK_OK;
}

static inline
TCHAR *u16_font(char *font)
{
    MultiByteToWideChar(CP_UTF8, 0, font, -1, uni_font, DW_SIZE);
    return uni_font;
}

static UINT_PTR WINAPI
choose_font_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM unnamedParam4)
{
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            if (on_dark_enable())
            {
                on_dark_set_theme(GetDlgItem(hwnd, IDOK), L"Explorer", NULL);
                on_dark_set_theme(GetDlgItem(hwnd, IDCANCEL), L"Explorer", NULL);
                const int ctl[] = { 0x430, 0x431, 0x410, 0x0411, 0x0470, 0x0471, 0x0472, 0x0473, 0x0474, 0x440, 0x441, 0x442, 0x443, 0x444, 0x445, 0x446 };
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
                const int buttons[] = { IDOK, IDCANCEL };
                for (int id = 0; id < _countof(buttons); ++id)
                {
                    HWND btn = GetDlgItem(hwnd, buttons[id]);
                    on_dark_allow_window(btn, true);
                    SendMessage(btn, WM_THEMECHANGED, 0, 0);
                }
                UpdateWindow(hwnd);
            }
            break;
        default:
            break;
    }
    return 0;
}

static UINT_PTR WINAPI
choose_color_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM unnamedParam4)
{
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            if (on_dark_enable())
            {
                on_dark_set_theme(GetDlgItem(hwnd, IDOK), L"Explorer", NULL);
                on_dark_set_theme(GetDlgItem(hwnd, IDCANCEL), L"Explorer", NULL);
                on_dark_set_theme(GetDlgItem(hwnd, 0x2c8), L"Explorer", NULL);  // COLOR_ADD
                on_dark_set_theme(GetDlgItem(hwnd, 0x2cf), L"Explorer", NULL);  // COLOR_MIX
                const int ctl[] = { 0x2be, 0x2c5, 0x2c6, IDC_STATIC };
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
                const int buttons[] = { IDOK, IDCANCEL, 0x2c8, 0x2cf };
                for (int id = 0; id < _countof(buttons); ++id)
                {
                    HWND btn = GetDlgItem(hwnd, buttons[id]);
                    on_dark_allow_window(btn, true);
                    SendMessage(btn, WM_THEMECHANGED, 0, 0);
                }
                UpdateWindow(hwnd);
            }
            break;
        default:
            break;
    }
    return 0;
}

static int
choose_style_font(char *font, int *fontsize, int *bold)
{
    HDC hdc = NULL;
    HWND hwnd = eu_module_hwnd();
    LOGFONT lf= {0};
    if (!(hwnd && font))
    {
        return EUE_POINT_NULL;
    }
    CHOOSEFONT cf= {sizeof(CHOOSEFONT)};
    hdc = GetDC(hwnd);
    _tcsncpy(lf.lfFaceName, u16_font(font), _countof(lf.lfFaceName)-1);
    lf.lfWeight = (bold && *bold) ? FW_BOLD : FW_NORMAL;
    lf.lfHeight = -MulDiv(*fontsize, eu_get_dpi(hwnd), 72);
    cf.hwndOwner = hwnd;
    cf.hInstance = eu_module_handle();
    cf.lpLogFont = &lf;
    cf.Flags = CF_INITTOLOGFONTSTRUCT;
    if (on_dark_enable())
    {
        cf.Flags |= CF_ENABLEHOOK;
        cf.lpfnHook = choose_font_proc;
    }
    cf.nFontType = SCREEN_FONTTYPE;
    ReleaseDC(hwnd, hdc);
    if (!ChooseFont(&cf))
    {
        return EUE_CHOOSE_FONT_ERR;
    }
    else
    {
        *font = 0;
    }
    if (!WideCharToMultiByte(CP_UTF8, 0, lf.lfFaceName, -1, font, DW_SIZE, NULL, NULL))
    {
        return EUE_API_CONV_FAIL;
    }
    (*fontsize) = cf.iPointSize / 10;
    if (bold)
    {
        (*bold) = (lf.lfWeight == FW_BOLD);
    }
    return SKYLARK_OK;
}

static int
choose_text_color(HWND hwnd, uint32_t *color)
{
    COLORREF cr = {0};
    COLORREF crs[16];
    CHOOSECOLOR cc = {sizeof(CHOOSECOLOR)};
    cr = (*color) & 0x00FFFFFF;
    memset(&crs, 0, sizeof(crs));
    cc.hwndOwner = hwnd;
    cc.hInstance = (HWND)eu_module_handle();
    cc.rgbResult = cr;
    cc.lpCustColors = (LPDWORD) crs;
    cc.Flags = CC_RGBINIT;
    if (on_dark_enable())
    {
        cc.Flags |= CC_ENABLEHOOK;
        cc.lpfnHook = choose_color_proc;
    }
    if (ChooseColor(&cc))
    {   // 保留原有的alpha值
        COLORREF tmp = (*color) & 0xFF000000;
        (*color) = (tmp | cc.rgbResult);
    }
    return SKYLARK_OK;
}

// 建立示例字体
#define CREATE_STYLETHEME_FONT(_st_memb_, _idc_static_, _hwnd_handle_name_, _font_handle_name_)   \
    _hwnd_handle_name_ = GetDlgItem(hdlg, _idc_static_);                                          \
    if (_idc_static_ == IDC_THEME_TAB_STATIC)                                                     \
    {                                                                                             \
        SendMessage(_hwnd_handle_name_, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 0); \
    }                                                                                             \
    else                                                                                          \
    {                                                                                             \
        _font_handle_name_ = CreateFont(FONT_SIZE_DPI(dlg_style._st_memb_.fontsize),              \
                                        0,                                                        \
                                        0,                                                        \
                                        0,                                                        \
                                        _idc_static_ == IDC_CARET_STATIC ? FW_NORMAL :            \
                                        (dlg_style._st_memb_.bold ? FW_BOLD : FW_NORMAL),         \
                                        false,                                                    \
                                        FALSE,                                                    \
                                        0,                                                        \
                                        DEFAULT_CHARSET,                                          \
                                        OUT_DEFAULT_PRECIS,                                       \
                                        CLIP_DEFAULT_PRECIS,                                      \
                                        DEFAULT_QUALITY,                                          \
                                        DEFAULT_PITCH | FF_SWISS,                                 \
                                        u16_font(dlg_style._st_memb_.font));                      \
        SendMessage(_hwnd_handle_name_, WM_SETFONT, (WPARAM) _font_handle_name_, 0);              \
    }

#define SET_STATIC_TEXTCOLOR(_hwnd_static_, _st_memb_)                                            \
    if ((HWND) lParam == _hwnd_static_)                                                           \
    {                                                                                             \
        SetTextColor((HDC) wParam, dlg_style._st_memb_.color & 0x00FFFFFF);                       \
        SetBkColor((HDC) wParam, dlg_style.text.bgcolor & 0x00FFFFFF);                            \
    }

// 选择新字体后更新示例字体
#define SYNC_FONT(_st_memb_, _hwnd_handle_name_, _font_handle_name_)                              \
    strcpy(dlg_style._st_memb_.font, dlg_style.text.font);                                        \
    dlg_style._st_memb_.fontsize = dlg_style.text.fontsize;                                       \
    DeleteObject(_font_handle_name_);                                                             \
    _font_handle_name_ = CreateFont(FONT_SIZE_DPI(dlg_style._st_memb_.fontsize),                  \
                                    0,                                                            \
                                    0,                                                            \
                                    0,                                                            \
                                    _hwnd_handle_name_ == hwnd_caret_static ? FW_NORMAL :         \
                                    (dlg_style._st_memb_.bold ? FW_BOLD : FW_NORMAL),             \
                                    false,                                                        \
                                    FALSE,                                                        \
                                    0,                                                            \
                                    DEFAULT_CHARSET,                                              \
                                    OUT_DEFAULT_PRECIS,                                           \
                                    CLIP_DEFAULT_PRECIS,                                          \
                                    DEFAULT_QUALITY,                                              \
                                    DEFAULT_PITCH | FF_SWISS,                                     \
                                    u16_font(dlg_style._st_memb_.font));                          \
    SendMessage(_hwnd_handle_name_, WM_SETFONT, (WPARAM) _font_handle_name_, FALSE);              \
    InvalidateRect(_hwnd_handle_name_, NULL, TRUE);

// 选择颜色后更新示例字体
#define STYLE_MSG(_st_memb_, _hwnd_handle_name_, _font_handle_name_, _idc_setfont_button_, _idc_settextcolor_button_) \
    if (wm_id == _idc_setfont_button_)                                                                                \
    {                                                                                                                 \
        choose_style_font(dlg_style._st_memb_.font, &(dlg_style._st_memb_.fontsize), &(dlg_style._st_memb_.bold));    \
        DeleteObject(_font_handle_name_);                                                                             \
        _font_handle_name_ = CreateFont(FONT_SIZE_DPI(dlg_style._st_memb_.fontsize),                                  \
                                        0,                                                                            \
                                        0,                                                                            \
                                        0,                                                                            \
                                        _hwnd_handle_name_ == hwnd_caret_static ? FW_NORMAL :                         \
                                        (dlg_style._st_memb_.bold ? FW_BOLD : FW_NORMAL),                             \
                                        false,                                                                        \
                                        FALSE,                                                                        \
                                        0,                                                                            \
                                        DEFAULT_CHARSET,                                                              \
                                        OUT_DEFAULT_PRECIS,                                                           \
                                        CLIP_DEFAULT_PRECIS,                                                          \
                                        DEFAULT_QUALITY,                                                              \
                                        DEFAULT_PITCH | FF_SWISS,                                                     \
                                        u16_font(dlg_style._st_memb_.font));                                          \
        SendMessage(_hwnd_handle_name_, WM_SETFONT, (WPARAM) _font_handle_name_, FALSE);                              \
        InvalidateRect(_hwnd_handle_name_, NULL, TRUE);                                                               \
    }                                                                                                                 \
    else if (wm_id == _idc_settextcolor_button_)                                                                      \
    {                                                                                                                 \
        choose_text_color(hdlg, &(dlg_style._st_memb_.color));                                                        \
        InvalidateRect(_hwnd_handle_name_, NULL, TRUE);                                                               \
    }

static void
theme_release_handle(void)
{
    DeleteObject(font_linenumber_static);
    DeleteObject(font_foldmargin_static);
    DeleteObject(font_text_static);
    DeleteObject(font_caretline_static);
    DeleteObject(font_indicator_static);
    DeleteObject(font_caret_static);
    DeleteObject(font_keyword_static);
    DeleteObject(font_keyword2_static);
    DeleteObject(font_string_static);
    DeleteObject(font_character_static);
    DeleteObject(font_number_static);
    DeleteObject(font_operator_static);
    DeleteObject(font_preprocessor_static);
    DeleteObject(font_comment_static);
    DeleteObject(font_commentline_static);
    DeleteObject(font_commentdoc_static);
    DeleteObject(font_tags_static);
    DeleteObject(font_unknowtags_static);
    DeleteObject(font_attributes_static);
    DeleteObject(font_unknowattr_static);
    DeleteObject(font_entities_static);
    DeleteObject(font_tagends_static);
    DeleteObject(font_cdata_static);
    DeleteObject(font_phpsection_static);
    DeleteObject(font_aspsection_static);
    DeleteObject(font_xmlsection_static);
    DeleteObject(font_hyperlink_static);
    DeleteObject(font_result_lineno_static);
    DeleteObject(font_result_key_static);
    DeleteObject(font_bracesection_static);
    DeleteObject(font_nchistory_static);
    DeleteObject(font_nchistory_static2);
    DeleteObject(font_dochistory_static);
    DeleteObject(font_dochistory_static2);
    DeleteObject(brush_language);
    brush_language = NULL;
    DeleteObject(brush_linenumber);
    brush_linenumber = NULL;
    DeleteObject(brush_foldmargin);
    brush_foldmargin = NULL;
    DeleteObject(brush_text);
    brush_text = NULL;
    DeleteObject(brush_caretline);
    brush_caretline = NULL;
    DeleteObject(brush_indicator);
    brush_indicator = NULL;
    DeleteObject(brush_activetab);
    brush_activetab = NULL;
}

static void
theme_show_balloon_tip(const HWND hdlg, const int resid)
{
    TCHAR ptxt[MAX_PATH] = {0};
    HWND hwnd_edit = GetDlgItem(hdlg, resid);
    if (hwnd_edit)
    {
        if (resid == IDC_THEME_CARET_EDT)
        {
            eu_i18n_load_str(IDS_THEME_CARET_TIPS, ptxt, MAX_PATH);
        }
        else
        {
            eu_i18n_load_str(IDS_THEME_EDIT_TIPS, ptxt, MAX_PATH);
        }
        if (hwnd_edit_tips)
        {
            DestroyWindow(hwnd_edit_tips);
            hwnd_edit_tips = NULL;
        }
        if (!hwnd_edit_tips)
        {
            hwnd_edit_tips = util_create_tips(hwnd_edit, hdlg, ptxt);
            if (hwnd_edit_tips && on_dark_enable())
            {
                on_dark_set_theme(hwnd_edit_tips, L"DarkMode_Explorer", NULL);
            }
        }
    }
}

static void
on_theme_set_tip(const HWND hdlg, const int res_id)
{
    HWND stc = GetDlgItem(hdlg, res_id);
    eu_tabpage *pnode = on_tabpage_focus_at();
    bool fn_font = pnode ? eu_doc_special_font(pnode) : false;
    if (stc && pnode && (fn_font || on_doc_is_customized(pnode, -1)))
    {
        HWND btn = NULL;
        LOAD_I18N_RESSTR(fn_font ? IDS_THEME_TIPS1 : IDS_THEME_TIPS2, str);
        Static_SetText(stc, str);
        if (fn_font && (btn = GetDlgItem(hdlg, IDC_SETFONT_TEXT_BTN)))
        {
            Button_Enable(btn, FALSE);
        }
    }
}

static intptr_t CALLBACK
theme_proc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wm_id;
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
        case WM_INITDIALOG:
        {
            TCHAR alpha[4] = {0};
            memcpy(&dlg_style, &(eu_get_theme()->item), sizeof(struct styletheme));
            CREATE_STYLETHEME_FONT(linenumber, IDC_LINENUMBER_STATIC, hwnd_linenumber_static, font_linenumber_static)
            CREATE_STYLETHEME_FONT(foldmargin, IDC_FOLDMARGIN_STATIC, hwnd_foldmargin_static, font_foldmargin_static)
            CREATE_STYLETHEME_FONT(text, IDC_TEXT_STATIC, hwnd_text_static, font_text_static)
            CREATE_STYLETHEME_FONT(caretline, IDC_CARETLINE_STATIC, hwnd_caretline_static, font_caretline_static)
            CREATE_STYLETHEME_FONT(indicator, IDC_INDICATOR_STATIC, hwnd_indicator_static, font_indicator_static)
            CREATE_STYLETHEME_FONT(caret, IDC_CARET_STATIC, hwnd_caret_static, font_caret_static)
            CREATE_STYLETHEME_FONT(keywords0, IDC_KEYWORDS_STATIC, hwnd_keyword_static, font_keyword_static)
            CREATE_STYLETHEME_FONT(keywords1, IDC_KEYWORDS2_STATIC, hwnd_keyword2_static, font_keyword2_static)
            CREATE_STYLETHEME_FONT(string, IDC_STRING_STATIC, hwnd_string_static, font_string_static)
            CREATE_STYLETHEME_FONT(character, IDC_CHARACTER_STATIC, hwnd_character_static, font_character_static)
            CREATE_STYLETHEME_FONT(number, IDC_NUMBER_STATIC, hwnd_number_static, font_number_static)
            CREATE_STYLETHEME_FONT(operators, IDC_OPERATOR_STATIC, hwnd_operator_static, font_operator_static)
            CREATE_STYLETHEME_FONT(preprocessor, IDC_PREPROCESSOR_STATIC, hwnd_preprocessor_static, font_preprocessor_static)
            CREATE_STYLETHEME_FONT(comment, IDC_COMMENT_STATIC, hwnd_comment_static, font_comment_static)
            CREATE_STYLETHEME_FONT(commentline, IDC_COMMENTLINE_STATIC, hwnd_commentline_static, font_commentline_static)
            CREATE_STYLETHEME_FONT(commentdoc, IDC_COMMENTDOC_STATIC, hwnd_commentdoc_static, font_commentdoc_static)
            CREATE_STYLETHEME_FONT(tags, IDC_TAGS_STATIC, hwnd_tags_static, font_tags_static)
            CREATE_STYLETHEME_FONT(unknowtags, IDC_UNKNOWTAGS_STATIC, hwnd_unknowtags_static, font_unknowtags_static)
            CREATE_STYLETHEME_FONT(attributes, IDC_ATTRIBUTES_STATIC, hwnd_attributes_static, font_attributes_static)
            CREATE_STYLETHEME_FONT(unknowattributes, IDC_UNKNOWATTRIBUTES_STATIC, hwnd_unknowattr_static, font_unknowattr_static)
            CREATE_STYLETHEME_FONT(entities, IDC_ENTITIES_STATIC, hwnd_entities_static, font_entities_static)
            CREATE_STYLETHEME_FONT(tagends, IDC_TAGENDS_STATIC, hwnd_tagends_static, font_tagends_static)
            CREATE_STYLETHEME_FONT(cdata, IDC_CDATA_STATIC, hwnd_cdata_static, font_cdata_static)
            CREATE_STYLETHEME_FONT(phpsection, IDC_PHPSECTION_STATIC, hwnd_phpsection_static, font_phpsection_static)
            CREATE_STYLETHEME_FONT(aspsection, IDC_ASPSECTION_STATIC, hwnd_aspsection_static, font_aspsection_static)
            CREATE_STYLETHEME_FONT(xmlsection, IDC_XMLSECTION_STATIC, hwnd_xmlsection_static, font_xmlsection_static)
            CREATE_STYLETHEME_FONT(hyperlink, IDC_HYPERLINKSECTION_STATIC, hwnd_hyperlink_static, font_hyperlink_static)
            CREATE_STYLETHEME_FONT(activetab, IDC_THEME_TAB_STATIC, hwnd_activetab_static, font_activetab_static)
            CREATE_STYLETHEME_FONT(symbolic, IDC_THEME_SYMBOLIC_STATIC, hwnd_symbolic_static, font_symbolic_static)
            CREATE_STYLETHEME_FONT(results, IDC_THEME_LINENO_STATIC, hwnd_result_lineno_static, font_result_lineno_static)
            CREATE_STYLETHEME_FONT(results, IDC_THEME_LINEKEY_STATIC, hwnd_result_key_static, font_result_key_static)
            CREATE_STYLETHEME_FONT(bracesection, IDC_BRACESECTION_STATIC, hwnd_bracesection_static, font_bracesection_static)
            CREATE_STYLETHEME_FONT(nchistory, IDC_THEME_HISTORY_STATIC, hwnd_nchistory_static, font_nchistory_static)
            CREATE_STYLETHEME_FONT(nchistory, IDC_THEME_HISTORY2_STATIC, hwnd_nchistory_static2, font_nchistory_static2)
            CREATE_STYLETHEME_FONT(dochistory, IDC_THEME_HISTORYDOC_STATIC, hwnd_dochistory_static, font_dochistory_static)
            CREATE_STYLETHEME_FONT(dochistory, IDC_THEME_HISTORYDOC2_STATIC, hwnd_dochistory_static2, font_dochistory_static2)
            HWND hwnd_caretline_edt = GetDlgItem(hdlg, IDC_THEME_CARTETLINE_EDT);
            HWND hwnd_caretline_udn = GetDlgItem(hdlg, IDC_THEME_CARTETLINE_UDN);
            HWND hwnd_indicator_edt = GetDlgItem(hdlg, IDC_THEME_INDICATOR_EDT);
            HWND hwnd_indicator_udn = GetDlgItem(hdlg, IDC_THEME_INDICATOR_UDN);
            HWND hwnd_caret_edt = GetDlgItem(hdlg, IDC_THEME_CARET_EDT);
            HWND hwnd_caret_udn = GetDlgItem(hdlg, IDC_THEME_CARET_UDN);
            if (hwnd_caretline_edt)
            {
                _sntprintf(alpha, 4, _T("%d"), (dlg_style.caretline.bgcolor & 0xff000000) >> 24);
                Edit_SetText(hwnd_caretline_edt, alpha);
            }
            if (hwnd_indicator_edt)
            {
                _sntprintf(alpha, 4, _T("%d"), (dlg_style.indicator.bgcolor & 0xff000000) >> 24);
                Edit_SetText(hwnd_indicator_edt, alpha);
            }
            if (hwnd_caret_edt)
            {
                _sntprintf(alpha, 4, _T("%d"), (dlg_style.caret.color & 0xff000000) >> 24);
                Edit_SetText(hwnd_caret_edt, alpha);
            }
            if (hwnd_caretline_udn)
            {
                SendMessage(hwnd_caretline_udn, UDM_SETRANGE, 0, MAKELPARAM(0, 255));
            }
            if (hwnd_indicator_udn)
            {
                SendMessage(hwnd_indicator_udn, UDM_SETRANGE, 0, MAKELPARAM(0, 255));
            }
            if (hwnd_caret_udn)
            {
                SendMessage(hwnd_caret_udn, UDM_SETRANGE, 0, MAKELPARAM(1, 9));
            }
            if (on_dark_enable())
            {
                on_dark_set_theme(GetDlgItem(hdlg, IDC_THEME_LANGUAGE_STATIC), L"", L"");
                on_dark_set_theme(GetDlgItem(hdlg, IDC_THEME_MARKUP_STATIC), L"", L"");
                on_dark_set_theme(GetDlgItem(hdlg, IDC_THEME_EDIT_STATIC), L"", L"");
                on_dark_set_theme(GetDlgItem(hdlg, IDC_THEME_MARGIN_STATIC), L"", L"");
                on_dark_set_theme(hdlg, L"Explorer", NULL);
            }
            on_theme_set_tip(hdlg, IDC_THEME_TIPS_STC);
            return util_creater_window(hdlg, eu_module_hwnd());
        }
        case WM_THEMECHANGED:
        {
            if (on_dark_supports())
            {
                bool dark = on_dark_enable();
                on_dark_allow_window(hdlg, dark);
                on_dark_refresh_titlebar(hdlg);
                const int buttons[] = {IDOK,
                                       IDCANCEL,
                                       IDC_SETFONT_KEYWORDS_BUTTON,
                                       IDC_SETTEXTCOLOR_KEYWORDS_BTN,
                                       IDC_SETFONT_KEYWORDS2_BTN,
                                       IDC_SETTEXTCOLOR_KEYWORDS2_BTN,
                                       IDC_SETFONT_STRING_BTN,
                                       IDC_SETTEXTCOLOR_STRING_BTN,
                                       IDC_SETFONT_CHARACTER_BTN,
                                       IDC_SETTEXTCOLOR_CHARACTER_BTN,
                                       IDC_SETFONT_NUMBER_BTN,
                                       IDC_SETTEXTCOLOR_NUMBER_BTN,
                                       IDC_SETFONT_PREPRO_BTN,
                                       IDC_SETTEXTCOLOR_PREPRO_BTN,
                                       IDC_SETFONT_COMMENT_BTN,
                                       IDC_SETTEXTCOLOR_COMMENT_BTN,
                                       IDC_SETFONT_COMMENTLINE_BTN,
                                       IDC_SETTEXTCOLOR_COMMENTL_BTN,
                                       IDC_SETFONT_COMMENTDOC_BTN,
                                       IDC_SETTEXTCOLOR_COMMENTDOC_BTN,
                                       IDC_SETFONT_TEXT_BTN,
                                       IDC_SETTEXTCOLOR_TEXT_BTN,
                                       IDC_SETFONT_OPERATOR_BTN,
                                       IDC_SETTEXTCOLOR_OPERATOR_BTN,
                                       IDC_SETFONT_UNKNOWTAGS_BTN,
                                       IDC_SETTEXTCOLOR_UNKNOWTAGS_BTN,
                                       IDC_SETFONT_ATTRIBUTES_BTN,
                                       IDC_SETTEXTCOLOR_ATTRIBUTES_BTN,
                                       IDC_SETFONT_UNATTRIBUTES_BTN,
                                       IDC_SETTEXTCOLOR_UNATTRS_BTN,
                                       IDC_SETFONT_ENTITIES_BTN,
                                       IDC_SETTEXTCOLOR_ENTITIES_BTN,
                                       IDC_SETFONT_TAGENDS_BTN,
                                       IDC_SETTEXTCOLOR_TAGENDS_BTN,
                                       IDC_SETFONT_PHPSECTION_BTN,
                                       IDC_SETTEXTCOLOR_PHPSECTION_BTN,
                                       IDC_SETFONT_ASPSECTION_BTN,
                                       IDC_SETTEXTCOLOR_ASPSECTION_BTN,
                                       IDC_SETFONT_XMLSECTION_BTN,
                                       IDC_SETTEXTCOLOR_XMLSECTION_BTN,
                                       IDC_SETFONT_HYPERLINKSECTION_BTN,
                                       IDC_SETTEXTCOLOR_HYPERLINKSECTION_BTN,
                                       IDC_SETFONT_TAGS_BTN,
                                       IDC_SETTEXTCOLOR_TAGS_BTN,
                                       IDC_SETFONT_CDATA_BTN,
                                       IDC_SETTEXTCOLOR_CDATA_BTN,
                                       IDC_SETBGCOLOR_CARETLINE_BTN,
                                       IDC_SETBGCOLOR_TEXT_BTN,
                                       IDC_SETBGCOLOR_INDICATOR_BTN,
                                       IDC_SETTEXTCOLOR_LINENUMBER_BTN,
                                       IDC_SETBGCOLOR_LINENUMBER_BTN,
                                       IDC_SETBGCOLOR_FOLDMARGIN_BTN,
                                       IDC_SETBGCOLOR_TAB_BTN,
                                       IDC_SETCOLOR_CARET_BTN,
                                       IDC_SETFONT_SYMBOLIC_BTN,
                                       IDC_SETTEXTCOLOR_SYMBOLIC_BTN,
                                       IDC_SETTEXTCOLOR_LINENO_BTN,
                                       IDC_SETTEXTCOLOR_LINEKEY_BTN,
                                       IDC_SETTEXTCOLORBRACESECTION_BTN,
                                       IDC_SETTEXTCOLOR_HISTORY_BTN,
                                       IDC_SETTEXTCOLOR_HISTORY2_BTN,
                                       IDC_SETTEXTCOLOR_HISTORYDOC_BTN,
                                       IDC_SETTEXTCOLOR_HISTORYDOC2_BTN,
                                       };
                for (int id = 0; id < _countof(buttons); ++id)
                {
                    HWND btn = GetDlgItem(hdlg, buttons[id]);
                    on_dark_allow_window(btn, dark);
                    on_dark_set_theme(btn, L"Explorer", NULL);
                }
                UpdateWindowEx(hdlg);
            }
            break;
        }
        case WM_CTLCOLORDLG:
        case WM_CTLCOLOREDIT:
        {
            return on_dark_set_contorl_color(wParam);
        }
        case WM_CTLCOLORSTATIC:
        {
            uint32_t resid = (uint32_t)GetWindowLongPtr((HWND) lParam, GWL_ID);
            if (resid == IDC_THEME_LANGUAGE_STATIC ||
                resid == IDC_THEME_MARKUP_STATIC   ||
                resid == IDC_THEME_EDIT_STATIC     ||
                resid == IDC_THEME_MARGIN_STATIC   ||
                resid == IDC_THEME_TIPS_STC
               )
            {
                return on_dark_set_contorl_color(wParam);
            }
            if (!brush_language)
            {
                brush_language = CreateSolidBrush(dlg_style.text.bgcolor);
            }
            if ((HWND) lParam == hwnd_linenumber_static)
            {
                SetTextColor((HDC) wParam, dlg_style.linenumber.color);
                SetBkColor((HDC) wParam, dlg_style.linenumber.bgcolor);
                if (brush_linenumber)
                {
                    DeleteObject(brush_linenumber);
                }
                brush_linenumber = CreateSolidBrush(dlg_style.linenumber.bgcolor);
                return (INT_PTR) brush_linenumber;
            }
            else if ((HWND) lParam == hwnd_foldmargin_static)
            {
                SetTextColor((HDC) wParam, dlg_style.text.color);
                SetBkColor((HDC) wParam, dlg_style.foldmargin.bgcolor);
                if (brush_foldmargin)
                {
                    DeleteObject(brush_foldmargin);
                }
                brush_foldmargin = CreateSolidBrush(dlg_style.foldmargin.bgcolor);
                return (INT_PTR) brush_foldmargin;
            }
            else if ((HWND) lParam == hwnd_text_static)
            {
                SetTextColor((HDC) wParam, dlg_style.text.color);
                SetBkColor((HDC) wParam, dlg_style.text.bgcolor);
                if (brush_text)
                {
                    DeleteObject(brush_text);
                }
                brush_text = CreateSolidBrush(dlg_style.text.bgcolor);
                return (INT_PTR) brush_text;
            }
            else if ((HWND) lParam == hwnd_caretline_static)
            {
                dlg_style.caretline.bgcolor &= 0x00ffffff;
                SetTextColor((HDC) wParam, dlg_style.text.color);
                SetBkColor((HDC) wParam, dlg_style.caretline.bgcolor);
                if (brush_caretline)
                {
                    DeleteObject(brush_caretline);
                }
                brush_caretline = CreateSolidBrush(dlg_style.caretline.bgcolor);
                return (INT_PTR) brush_caretline;
            }
            else if ((HWND) lParam == hwnd_indicator_static)
            {
                dlg_style.indicator.bgcolor &= 0x00ffffff;
                SetTextColor((HDC) wParam, dlg_style.text.color);
                SetBkColor((HDC) wParam, dlg_style.indicator.bgcolor);
                if (brush_indicator)
                {
                    DeleteObject(brush_indicator);
                }
                brush_indicator = CreateSolidBrush(dlg_style.indicator.bgcolor);
                return (INT_PTR) brush_indicator;
            }
            else if ((HWND) lParam == hwnd_activetab_static)
            {
                dlg_style.activetab.bgcolor &= 0x00ffffff;
                SetTextColor((HDC) wParam, dlg_style.text.color);
                SetBkColor((HDC) wParam, dlg_style.activetab.bgcolor);
                if (brush_activetab)
                {
                    DeleteObject(brush_activetab);
                }
                brush_activetab = CreateSolidBrush(dlg_style.activetab.bgcolor);
                return (INT_PTR) brush_activetab;
            }
            else if ((HWND) lParam == hwnd_result_lineno_static)
            {
                SetTextColor((HDC) wParam, dlg_style.results.color);
                SetBkColor((HDC) wParam, dlg_style.text.bgcolor);
            }
            else if ((HWND) lParam == hwnd_result_key_static)
            {
                SetTextColor((HDC) wParam, dlg_style.results.bgcolor);
                SetBkColor((HDC) wParam, dlg_style.text.bgcolor);
            }
            else if ((HWND) lParam == hwnd_nchistory_static)
            {
                SetTextColor((HDC) wParam, dlg_style.nchistory.color);
                SetBkColor((HDC) wParam, dlg_style.text.bgcolor);
            }
            else if ((HWND) lParam == hwnd_nchistory_static2)
            {
                SetTextColor((HDC) wParam, dlg_style.nchistory.bgcolor);
                SetBkColor((HDC) wParam, dlg_style.text.bgcolor);
            }
            else if ((HWND) lParam == hwnd_dochistory_static)
            {
                SetTextColor((HDC) wParam, dlg_style.dochistory.color);
                SetBkColor((HDC) wParam, dlg_style.text.bgcolor);
            }
            else if ((HWND) lParam == hwnd_dochistory_static2)
            {
                SetTextColor((HDC) wParam, dlg_style.dochistory.bgcolor);
                SetBkColor((HDC) wParam, dlg_style.text.bgcolor);
            }
            else SET_STATIC_TEXTCOLOR(hwnd_keyword_static, keywords0)
            else SET_STATIC_TEXTCOLOR(hwnd_keyword2_static, keywords1)
            else SET_STATIC_TEXTCOLOR(hwnd_string_static, string)
            else SET_STATIC_TEXTCOLOR(hwnd_character_static, character)
            else SET_STATIC_TEXTCOLOR(hwnd_number_static, number)
            else SET_STATIC_TEXTCOLOR(hwnd_operator_static, operators)
            else SET_STATIC_TEXTCOLOR(hwnd_preprocessor_static, preprocessor)
            else SET_STATIC_TEXTCOLOR(hwnd_comment_static, comment)
            else SET_STATIC_TEXTCOLOR(hwnd_commentline_static, commentline)
            else SET_STATIC_TEXTCOLOR(hwnd_commentdoc_static, commentdoc)
            else SET_STATIC_TEXTCOLOR(hwnd_tags_static, tags)
            else SET_STATIC_TEXTCOLOR(hwnd_unknowtags_static, unknowtags)
            else SET_STATIC_TEXTCOLOR(hwnd_attributes_static, attributes)
            else SET_STATIC_TEXTCOLOR(hwnd_unknowattr_static, unknowattributes)
            else SET_STATIC_TEXTCOLOR(hwnd_entities_static, entities)
            else SET_STATIC_TEXTCOLOR(hwnd_tagends_static, tagends)
            else SET_STATIC_TEXTCOLOR(hwnd_cdata_static, cdata)
            else SET_STATIC_TEXTCOLOR(hwnd_phpsection_static, phpsection)
            else SET_STATIC_TEXTCOLOR(hwnd_aspsection_static, aspsection)
            else SET_STATIC_TEXTCOLOR(hwnd_xmlsection_static, xmlsection)
            else SET_STATIC_TEXTCOLOR(hwnd_hyperlink_static, hyperlink)
            else SET_STATIC_TEXTCOLOR(hwnd_bracesection_static, bracesection)
            else SET_STATIC_TEXTCOLOR(hwnd_caret_static, caret)
            else SET_STATIC_TEXTCOLOR(hwnd_symbolic_static, symbolic)
            return (LRESULT) brush_language;
        }
        case WM_COMMAND:
            wm_id = LOWORD(wParam);
            switch (wm_id)
            {
                case IDOK:
                {
                    int value = 0;
                    TCHAR alpha[4] = {0};
                    HWND hwnd_caretline = GetDlgItem(hdlg, IDC_THEME_CARTETLINE_EDT);
                    HWND hwnd_indicator = GetDlgItem(hdlg, IDC_THEME_INDICATOR_EDT);
                    HWND hwnd_caret = GetDlgItem(hdlg, IDC_THEME_CARET_EDT);
                    if (hwnd_caretline)
                    {
                        Edit_GetText(hwnd_caretline, alpha, 4);
                        value = _tstoi(alpha) << 24 & 0xff000000;
                        dlg_style.caretline.bgcolor |= value;
                    }
                    if (hwnd_indicator)
                    {
                        Edit_GetText(hwnd_indicator, alpha, 4);
                        value = _tstoi(alpha) << 24 & 0xff000000;
                        dlg_style.indicator.bgcolor |= value;
                    }
                    if (hwnd_caret)
                    {
                        Edit_GetText(hwnd_caret, alpha, 4);
                        value = _tstoi(alpha) << 24 & 0xff000000;
                        dlg_style.caret.color |= value;
                    }
                    memcpy(&(eu_get_theme()->item), &dlg_style, sizeof(struct styletheme));
                    // 当前主题可能被修改, 立即保存
                    eu_save_theme();
                    EndDialog(hdlg, LOWORD(wParam));
                    break;
                }
                case IDCANCEL:
                    EndDialog(hdlg, -1);
                    break;
                case IDC_THEME_CARTETLINE_EDT:
                case IDC_THEME_INDICATOR_EDT:
                case IDC_THEME_CARET_EDT:
                {
                    if (HIWORD(wParam) == EN_SETFOCUS)
                    {
                        theme_show_balloon_tip(hdlg, wm_id);
                    }
                    break;
                }
                case IDC_SETTEXTCOLOR_LINENUMBER_BTN:
                    choose_text_color(hdlg, &(dlg_style.linenumber.color));
                    InvalidateRect(hwnd_linenumber_static, NULL, TRUE);
                    break;
                case IDC_SETBGCOLOR_LINENUMBER_BTN:
                    choose_text_color(hdlg, &(dlg_style.linenumber.bgcolor));
                    InvalidateRect(hwnd_linenumber_static, NULL, TRUE);
                    break;
                case IDC_SETBGCOLOR_FOLDMARGIN_BTN:
                    choose_text_color(hdlg, &(dlg_style.foldmargin.bgcolor));
                    InvalidateRect(hwnd_foldmargin_static, NULL, TRUE);
                    break;
                case IDC_SETFONT_TEXT_BTN:
                {
                    choose_style_font(dlg_style.text.font, &(dlg_style.text.fontsize), &(dlg_style.text.bold));

                    SYNC_FONT(text, hwnd_text_static, font_text_static)
                    SYNC_FONT(keywords0, hwnd_keyword_static, font_keyword_static)
                    SYNC_FONT(keywords1, hwnd_keyword2_static, font_keyword2_static)
                    SYNC_FONT(string, hwnd_string_static, font_string_static)
                    SYNC_FONT(character, hwnd_character_static, font_character_static)
                    SYNC_FONT(number, hwnd_number_static, font_number_static)
                    SYNC_FONT(operators, hwnd_operator_static, font_operator_static)
                    SYNC_FONT(preprocessor, hwnd_preprocessor_static, font_preprocessor_static)
                    SYNC_FONT(comment, hwnd_comment_static, font_comment_static)
                    SYNC_FONT(commentline, hwnd_commentline_static, font_commentline_static)
                    SYNC_FONT(commentdoc, hwnd_commentdoc_static, font_commentdoc_static)

                    SYNC_FONT(tags, hwnd_tags_static, font_tags_static)
                    SYNC_FONT(unknowtags, hwnd_unknowtags_static, font_unknowtags_static)
                    SYNC_FONT(attributes, hwnd_attributes_static, font_attributes_static)
                    SYNC_FONT(unknowattributes, hwnd_unknowattr_static, font_unknowattr_static)
                    SYNC_FONT(entities, hwnd_entities_static, font_entities_static)
                    SYNC_FONT(tagends, hwnd_tagends_static, font_tagends_static)
                    SYNC_FONT(cdata, hwnd_cdata_static, font_cdata_static)
                    SYNC_FONT(phpsection, hwnd_phpsection_static, font_phpsection_static)
                    SYNC_FONT(aspsection, hwnd_aspsection_static, font_aspsection_static)
                    SYNC_FONT(xmlsection, hwnd_xmlsection_static, font_xmlsection_static)
                    SYNC_FONT(hyperlink, hwnd_hyperlink_static, font_hyperlink_static)
                    break;
                }
                case IDC_SETTEXTCOLOR_TEXT_BTN:
                    choose_text_color(hdlg, &(dlg_style.text.color));
                    InvalidateRect(hwnd_text_static, NULL, TRUE);
                    break;
                case IDC_SETBGCOLOR_TEXT_BTN:
                    choose_text_color(hdlg, &(dlg_style.text.bgcolor));
                    InvalidateRect(hwnd_text_static, NULL, TRUE);
                    break;
                case IDC_SETBGCOLOR_CARETLINE_BTN:
                    choose_text_color(hdlg, &(dlg_style.caretline.bgcolor));
                    InvalidateRect(hwnd_caretline_static, NULL, TRUE);
                    break;
                case IDC_SETBGCOLOR_INDICATOR_BTN:
                {
                    choose_text_color(hdlg, &dlg_style.indicator.bgcolor);
                    InvalidateRect(hwnd_indicator_static, NULL, TRUE);
                    break;
                }
                case IDC_SETBGCOLOR_TAB_BTN:
                {
                    choose_text_color(hdlg, &dlg_style.activetab.bgcolor);
                    InvalidateRect(hwnd_activetab_static, NULL, TRUE);
                    break;
                }
                case IDC_SETCOLOR_CARET_BTN:
                {
                    choose_text_color(hdlg, &(dlg_style.caret.color));
                    InvalidateRect(hwnd_caret_static, NULL, TRUE);
                    break;
                }
                case IDC_SETTEXTCOLOR_LINENO_BTN:
                {
                    choose_text_color(hdlg, &dlg_style.results.color);
                    InvalidateRect(hwnd_result_lineno_static, NULL, FALSE);
                    break;
                }
                case IDC_SETTEXTCOLOR_LINEKEY_BTN:
                {
                    choose_text_color(hdlg, &(dlg_style.results.bgcolor));
                    InvalidateRect(hwnd_result_key_static, NULL, FALSE);
                    break;
                }
                case IDC_SETTEXTCOLORBRACESECTION_BTN:
                {
                    choose_text_color(hdlg, &dlg_style.bracesection.color);
                    InvalidateRect(hwnd_bracesection_static, NULL, FALSE);
                    break;
                }
                case IDC_SETTEXTCOLOR_HISTORY_BTN:
                {
                    choose_text_color(hdlg, &dlg_style.nchistory.color);
                    InvalidateRect(hwnd_nchistory_static, NULL, FALSE);
                    break;
                }
                case IDC_SETTEXTCOLOR_HISTORY2_BTN:
                {
                    choose_text_color(hdlg, &(dlg_style.nchistory.bgcolor));
                    InvalidateRect(hwnd_nchistory_static2, NULL, FALSE);
                    break;
                }
                case IDC_SETTEXTCOLOR_HISTORYDOC_BTN:
                {
                    choose_text_color(hdlg, &dlg_style.dochistory.color);
                    InvalidateRect(hwnd_dochistory_static, NULL, FALSE);
                    break;
                }
                case IDC_SETTEXTCOLOR_HISTORYDOC2_BTN:
                {
                    choose_text_color(hdlg, &(dlg_style.dochistory.bgcolor));
                    InvalidateRect(hwnd_dochistory_static2, NULL, FALSE);
                    break;
                }
            }
            STYLE_MSG(keywords0,hwnd_keyword_static,font_keyword_static,IDC_SETFONT_KEYWORDS_BUTTON,IDC_SETTEXTCOLOR_KEYWORDS_BTN)
            else STYLE_MSG(keywords1,hwnd_keyword2_static,font_keyword2_static,IDC_SETFONT_KEYWORDS2_BTN,IDC_SETTEXTCOLOR_KEYWORDS2_BTN)
            else STYLE_MSG(string,hwnd_string_static,font_string_static,IDC_SETFONT_STRING_BTN,IDC_SETTEXTCOLOR_STRING_BTN)
            else STYLE_MSG(character,hwnd_character_static,font_character_static,IDC_SETFONT_CHARACTER_BTN,IDC_SETTEXTCOLOR_CHARACTER_BTN)
            else STYLE_MSG(number,hwnd_number_static,font_number_static,IDC_SETFONT_NUMBER_BTN,IDC_SETTEXTCOLOR_NUMBER_BTN)
            else STYLE_MSG(operators,hwnd_operator_static,font_operator_static,IDC_SETFONT_OPERATOR_BTN,IDC_SETTEXTCOLOR_OPERATOR_BTN)
            else STYLE_MSG(preprocessor,hwnd_preprocessor_static,font_preprocessor_static,IDC_SETFONT_PREPRO_BTN,IDC_SETTEXTCOLOR_PREPRO_BTN)
            else STYLE_MSG(comment,hwnd_comment_static,font_comment_static,IDC_SETFONT_COMMENT_BTN,IDC_SETTEXTCOLOR_COMMENT_BTN)
            else STYLE_MSG(commentline,hwnd_commentline_static,font_commentline_static,IDC_SETFONT_COMMENTLINE_BTN,IDC_SETTEXTCOLOR_COMMENTL_BTN)
            else STYLE_MSG(commentdoc,hwnd_commentdoc_static,font_commentdoc_static,IDC_SETFONT_COMMENTDOC_BTN,IDC_SETTEXTCOLOR_COMMENTDOC_BTN)
            else STYLE_MSG(tags,hwnd_tags_static,font_tags_static,IDC_SETFONT_TAGS_BTN,IDC_SETTEXTCOLOR_TAGS_BTN)
            else STYLE_MSG(unknowtags,hwnd_unknowtags_static,font_unknowtags_static,IDC_SETFONT_UNKNOWTAGS_BTN,IDC_SETTEXTCOLOR_UNKNOWTAGS_BTN)
            else STYLE_MSG(attributes,hwnd_attributes_static,font_attributes_static,IDC_SETFONT_ATTRIBUTES_BTN,IDC_SETTEXTCOLOR_ATTRIBUTES_BTN)
            else STYLE_MSG(unknowattributes,hwnd_unknowattr_static,font_unknowattr_static,IDC_SETFONT_UNATTRIBUTES_BTN,IDC_SETTEXTCOLOR_UNATTRS_BTN)
            else STYLE_MSG(entities,hwnd_entities_static,font_entities_static,IDC_SETFONT_ENTITIES_BTN,IDC_SETTEXTCOLOR_ENTITIES_BTN)
            else STYLE_MSG(tagends,hwnd_tagends_static,font_tagends_static,IDC_SETFONT_TAGENDS_BTN,IDC_SETTEXTCOLOR_TAGENDS_BTN)
            else STYLE_MSG(cdata,hwnd_cdata_static,font_cdata_static,IDC_SETFONT_CDATA_BTN,IDC_SETTEXTCOLOR_CDATA_BTN)
            else STYLE_MSG(phpsection,hwnd_phpsection_static,font_phpsection_static,IDC_SETFONT_PHPSECTION_BTN,IDC_SETTEXTCOLOR_PHPSECTION_BTN)
            else STYLE_MSG(aspsection,hwnd_aspsection_static,font_aspsection_static,IDC_SETFONT_ASPSECTION_BTN,IDC_SETTEXTCOLOR_ASPSECTION_BTN)
            else STYLE_MSG(xmlsection,hwnd_xmlsection_static,font_xmlsection_static,IDC_SETFONT_XMLSECTION_BTN,IDC_SETTEXTCOLOR_XMLSECTION_BTN)    
            else STYLE_MSG(hyperlink,hwnd_hyperlink_static,font_hyperlink_static,IDC_SETFONT_HYPERLINKSECTION_BTN,IDC_SETTEXTCOLOR_HYPERLINKSECTION_BTN)
            else STYLE_MSG(symbolic,hwnd_symbolic_static,font_symbolic_static,IDC_SETFONT_SYMBOLIC_BTN,IDC_SETTEXTCOLOR_SYMBOLIC_BTN)
            break;
        case WM_DESTROY:
            if (hwnd_edit_tips)
            {
                DestroyWindow(hwnd_edit_tips);
                hwnd_edit_tips = NULL;
            }
            theme_release_handle();
            break;
        default:
            break;
    }
    return 0;
}

bool
on_theme_create_dlg(void)
{
    return i18n_dlgbox(eu_module_hwnd(), IDD_STYLETHEME_DIALOG, theme_proc, 0) > 0;
}
