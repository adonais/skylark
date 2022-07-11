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

#define USE_DWMAPI 1  // 启用DWM绘制标题栏

typedef enum tagIMMERSIVE_HC_CACHE_MODE
{
    IHCM_USE_CACHED_VALUE,
    IHCM_REFRESH
}IMMERSIVE_HC_CACHE_MODE;

// 1903 18362
typedef enum tagPreferredAppMode
{
    Default,
    AllowDark,
    ForceDark,
    ForceLight,
    Max
}PreferredAppMode;

typedef enum tagWINDOWCOMPOSITIONATTRIB
{
    WCA_UNDEFINED = 0,
    WCA_NCRENDERING_ENABLED = 1,
    WCA_NCRENDERING_POLICY = 2,
    WCA_TRANSITIONS_FORCEDISABLED = 3,
    WCA_ALLOW_NCPAINT = 4,
    WCA_CAPTION_BUTTON_BOUNDS = 5,
    WCA_NONCLIENT_RTL_LAYOUT = 6,
    WCA_FORCE_ICONIC_REPRESENTATION = 7,
    WCA_EXTENDED_FRAME_BOUNDS = 8,
    WCA_HAS_ICONIC_BITMAP = 9,
    WCA_THEME_ATTRIBUTES = 10,
    WCA_NCRENDERING_EXILED = 11,
    WCA_NCADORNMENTINFO = 12,
    WCA_EXCLUDED_FROM_LIVEPREVIEW = 13,
    WCA_VIDEO_OVERLAY_ACTIVE = 14,
    WCA_FORCE_ACTIVEWINDOW_APPEARANCE = 15,
    WCA_DISALLOW_PEEK = 16,
    WCA_CLOAK = 17,
    WCA_CLOAKED = 18,
    WCA_ACCENT_POLICY = 19,
    WCA_FREEZE_REPRESENTATION = 20,
    WCA_EVER_UNCLOAKED = 21,
    WCA_VISUAL_OWNER = 22,
    WCA_HOLOGRAPHIC = 23,
    WCA_EXCLUDED_FROM_DDA = 24,
    WCA_PASSIVEUPDATEMODE = 25,
    WCA_USEDARKMODECOLORS = 26,
    WCA_LAST = 27
}WINDOWCOMPOSITIONATTRIB;

typedef struct tagWINDOWCOMPOSITIONATTRIBDATA
{
    WINDOWCOMPOSITIONATTRIB Attrib;
    PVOID pvData;
    SIZE_T cbData;
}WINDOWCOMPOSITIONATTRIBDATA;

typedef void (WINAPI *RtlGetNtVersionNumbersPtr)(LPDWORD major, LPDWORD minor, LPDWORD build);
typedef BOOL (WINAPI *SetWindowCompositionAttributePtr)(HWND hwnd, WINDOWCOMPOSITIONATTRIBDATA*);
// 1809 17763
typedef bool (WINAPI *ShouldAppsUseDarkModePtr)(void); // ordinal 132
typedef bool (WINAPI *AllowDarkModeForWindowPtr)(HWND hwnd, bool allow); // ordinal 133
typedef bool (WINAPI *AllowDarkModeForAppPtr)(bool allow); // ordinal 135, in 1809
typedef void (WINAPI *FlushMenuThemesPtr)(void); // ordinal 136
typedef void (WINAPI *RefreshImmersiveColorPolicyStatePtr)(void); // ordinal 104
typedef bool (WINAPI *IsDarkModeAllowedForWindowPtr)(HWND hwnd); // ordinal 137
typedef bool (WINAPI *GetIsImmersiveColorUsingHighContrastPtr)(IMMERSIVE_HC_CACHE_MODE mode); // ordinal 106
typedef HTHEME(WINAPI *OpenNcThemeDataPtr)(HWND hwnd, LPCWSTR pszClassList); // ordinal 49
// 1903 18362
typedef bool (WINAPI *ShouldSystemUseDarkModePtr)(void); // ordinal 138
typedef PreferredAppMode (WINAPI *SetPreferredAppModePtr)(PreferredAppMode appMode); // ordinal 135, in 1903
typedef bool (WINAPI *IsDarkModeAllowedForAppPtr)(void); // ordinal 139

typedef HRESULT (WINAPI *SetWindowThemePtr)(HWND, const wchar_t *, const wchar_t *);
typedef HTHEME (WINAPI *OpenThemeDataPtr)(HWND, LPCWSTR pszClassList);
typedef HRESULT (WINAPI *CloseThemeDataPtr)(HTHEME);
typedef COLORREF (WINAPI *GetThemeSysColorPtr)(HTHEME hth, int colid);
typedef HRESULT (WINAPI *GetThemePartSizePtr)(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, LPCRECT prc, int eSize, SIZE *psz);
typedef HRESULT (WINAPI *DrawThemeBackgroundPtr)(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, LPCRECT pRect, LPCRECT pClipRect);

// dwm function
typedef HRESULT (WINAPI *DwmSetWindowAttributePtr)(HWND, DWORD, LPCVOID, DWORD);
typedef HRESULT (WINAPI *DwmGetColorizationColorPtr)(DWORD *pcrColorization, BOOL  *pfOpaqueBlend);

static SetWindowCompositionAttributePtr fnSetWindowCompositionAttribute;
static ShouldAppsUseDarkModePtr fnShouldAppsUseDarkMode;
static AllowDarkModeForWindowPtr fnAllowDarkModeForWindow;
static AllowDarkModeForAppPtr fnAllowDarkModeForApp;
static FlushMenuThemesPtr fnFlushMenuThemes;
static RefreshImmersiveColorPolicyStatePtr fnRefreshImmersiveColorPolicyState;
static IsDarkModeAllowedForWindowPtr fnIsDarkModeAllowedForWindow;
static GetIsImmersiveColorUsingHighContrastPtr fnGetIsImmersiveColorUsingHighContrast;
static OpenNcThemeDataPtr fnOpenNcThemeData;
// 1903 18362
static ShouldSystemUseDarkModePtr fnShouldSystemUseDarkMode;
static SetPreferredAppModePtr fnSetPreferredAppMode;

static bool g_dark_supported;
static bool g_dark_enabled;
static uint32_t g_build_number;
static HMODULE g_uxtheme;
static HBRUSH g_dark_bkgnd;
static HBRUSH g_dark_hot_bkgnd;
static HBRUSH g_theme_bkgnd;

bool
on_dark_supports(void)
{
    return g_dark_supported;
}

bool on_dark_apps_use(void)
{
    return fnShouldAppsUseDarkMode ? fnShouldAppsUseDarkMode() : false;
}

bool
on_dark_allow_window(HWND hwnd, bool allow)
{
    return (g_dark_supported && fnAllowDarkModeForWindow) ? fnAllowDarkModeForWindow(hwnd, allow) : false;
}

static bool
on_dark_high_contrast(void)
{
    HIGHCONTRASTW highContrast = { sizeof(highContrast) };
    return SystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof(HIGHCONTRASTW), &highContrast, 0) ? (highContrast.dwFlags & HCF_HIGHCONTRASTON) : false;
}

bool
on_dark_enable(void)
{
    return g_dark_enabled && on_dark_apps_use() && !on_dark_high_contrast();
}

HRESULT
on_dark_draw_background(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, LPCRECT pRect, LPCRECT pClipRect)
{
    HRESULT ret = 1;
    HMODULE uxtheme = LoadLibraryEx(_T("uxtheme.dll"), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    DrawThemeBackgroundPtr fnDrawThemeBackground = uxtheme ? (DrawThemeBackgroundPtr)GetProcAddress(uxtheme, "DrawThemeBackground") : NULL;
    if (hTheme && fnDrawThemeBackground)
    {
        ret = fnDrawThemeBackground(hTheme, hdc, iPartId, iStateId, pRect, pClipRect);
    }
    safe_close_dll(uxtheme);
    return ret;
}

HRESULT
on_dark_get_partsize(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, LPCRECT prc, int eSize, SIZE *psz)
{
    HRESULT ret = 1;
    HMODULE uxtheme = LoadLibraryEx(_T("uxtheme.dll"), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    GetThemePartSizePtr fnGetThemePartSize = uxtheme ? (GetThemePartSizePtr)GetProcAddress(uxtheme, "GetThemePartSize") : NULL;
    if (hTheme && fnGetThemePartSize)
    {
        ret = fnGetThemePartSize(hTheme, hdc, iPartId, iStateId, prc, eSize, psz);
    }
    safe_close_dll(uxtheme);
    return ret;
}

intptr_t
on_dark_open_data(HWND hwnd, LPCWSTR class_list)
{
    intptr_t hth = 0;
    HMODULE uxtheme = LoadLibraryEx(_T("uxtheme.dll"), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    OpenThemeDataPtr fnOpenThemeData = uxtheme ? (OpenThemeDataPtr)GetProcAddress(uxtheme, "OpenThemeData") : NULL;
    if (class_list && fnOpenThemeData)
    {
        hth = (intptr_t)fnOpenThemeData(hwnd, class_list);
    }
    safe_close_dll(uxtheme);
    return hth;
}

HRESULT
on_dark_close_data(void *hth)
{
    HRESULT ret = 1;
    HMODULE uxtheme = LoadLibraryEx(_T("uxtheme.dll"), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    CloseThemeDataPtr fnCloseThemeData = uxtheme ? (CloseThemeDataPtr)GetProcAddress(uxtheme, "CloseThemeData") : NULL;
    if ((HTHEME)hth && fnCloseThemeData)
    {
        ret = fnCloseThemeData(hth);
    }
    safe_close_dll(uxtheme);
    return ret;
}

colour
on_dark_get_sys_colour(HWND hwnd, int colid)
{
    colour col = 0;
    HTHEME hth = NULL;
    HMODULE uxtheme = LoadLibraryEx(_T("uxtheme.dll"), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    OpenThemeDataPtr fnOpenThemeData = uxtheme ? (OpenThemeDataPtr)GetProcAddress(uxtheme, "OpenThemeData") : NULL;
    CloseThemeDataPtr fnCloseThemeData = uxtheme ? (CloseThemeDataPtr)GetProcAddress(uxtheme, "CloseThemeData") : NULL;
    GetThemeSysColorPtr fnGetThemeSysColor = uxtheme ? (GetThemeSysColorPtr)GetProcAddress(uxtheme, "GetThemeSysColor") : NULL;
    if (fnOpenThemeData && fnCloseThemeData && fnGetThemeSysColor)
    {
        HTHEME hth = fnOpenThemeData(hwnd, _T("TAB;HEADER;WINDOW"));
        if (hth)
        {
            col = fnGetThemeSysColor(hth, colid);
            fnCloseThemeData(hth);
        }
    }
    else
    {
        col = GetSysColor(colid);
    }
    safe_close_dll(uxtheme);
    return col;
}

void
on_dark_set_theme(HWND hwnd, const wchar_t *psz_name, const wchar_t *psz_list)
{
    HMODULE uxtheme = LoadLibraryEx(_T("uxtheme.dll"), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    SetWindowThemePtr fnSetWindowTheme = uxtheme ? (SetWindowThemePtr)GetProcAddress(uxtheme, "SetWindowTheme") : NULL;
    if (fnSetWindowTheme)
    {
        if (fnSetWindowTheme(hwnd, psz_name, psz_list) != S_OK)
        {
            printf("fnSetWindowTheme failed\n");
        }
    }
    safe_close_dll(uxtheme);
}

void
on_dark_set_titlebar(HWND hwnd, BOOL dark)
{
    if (g_build_number < 18362)
    {
    #if USE_DWMAPI
        HMODULE dwm = LoadLibraryEx(_T("dwmapi.dll"), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
        DwmSetWindowAttributePtr fnDwmSetWindowAttribute = dwm ? (DwmSetWindowAttributePtr)GetProcAddress(dwm, "DwmSetWindowAttribute") : NULL;
        if (fnDwmSetWindowAttribute)
        {
            if (S_OK != fnDwmSetWindowAttribute(hwnd, 20, &dark, sizeof dark))
            {   // this would be the call before Windows build 18362
                fnDwmSetWindowAttribute(hwnd, 19, &dark, sizeof dark);
            }
        }
        safe_close_dll(dwm);
    #else
        SetProp(hwnd, _T("UseImmersiveDarkModeColors"), (HANDLE)(intptr_t)(dark));
    #endif // USE_DWMAPI
    }
    else if (fnSetWindowCompositionAttribute)
    {
        WINDOWCOMPOSITIONATTRIBDATA data = { WCA_USEDARKMODECOLORS, &dark, sizeof(dark) };
        fnSetWindowCompositionAttribute(hwnd, &data);
    }
}

colour
on_dark_get_colorization_color(void)
{
    colour theme_color = 0;
    HMODULE dwm = LoadLibraryEx(_T("dwmapi.dll"), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    DwmGetColorizationColorPtr fnDwmGetColorizationColor = dwm ? (DwmGetColorizationColorPtr)GetProcAddress(dwm, "DwmGetColorizationColor") : NULL;
    if (fnDwmGetColorizationColor)
    {
        BOOL fopaqueblend;
        uint32_t colorization = 0;
        if (S_OK == fnDwmGetColorizationColor(&colorization, &fopaqueblend))
        {
            uint8_t r, g, b;
            r = (colorization >> 16) % 256;
            g = (colorization >> 8) % 256;
            b = colorization % 256;
            theme_color = RGB(r, g, b);
        }
    }
    safe_close_dll(dwm);
    return theme_color;
}

void
on_dark_refresh_titlebar(HWND hwnd)
{
    BOOL dark = FALSE;
    if (fnIsDarkModeAllowedForWindow(hwnd) && fnShouldAppsUseDarkMode() &&!on_dark_high_contrast())
    {
        dark = TRUE;
    }
    on_dark_set_titlebar(hwnd, dark);
}

bool
on_dark_color_scheme_change(LPARAM lParam)
{
    bool is = false;
    if (lParam && (0 == _tcsicmp((const TCHAR *)lParam, _T("ImmersiveColorSet"))))
    {
        fnRefreshImmersiveColorPolicyState();
        is = true;
    }
    fnGetIsImmersiveColorUsingHighContrast(IHCM_REFRESH);
    return is;
}

bool
on_dark_color_scheme_change_msg(UINT message, LPARAM lParam)
{
    return (message == WM_SETTINGCHANGE) ? on_dark_color_scheme_change(lParam) : false;
}

void
on_dark_allow_app(bool allow)
{
    if (fnAllowDarkModeForApp)
    {
        fnAllowDarkModeForApp(allow);
    }
    else if (fnSetPreferredAppMode)
    {
        fnSetPreferredAppMode(allow ? ForceDark : Default);
    }
}

static HTHEME WINAPI
OpenNcThemeDataStub(HWND hwnd, LPCTSTR classList)
{
    if (_tcscmp(classList, _T("ScrollBar")) == 0)
    {
        hwnd = NULL;
        classList = _T("Explorer::ScrollBar");
    }
    return fnOpenNcThemeData(hwnd, classList);
};

static void
on_dark_fix_scrollbar(bool fixed)
{
    HMODULE comctl = LoadLibraryEx(_T("comctl32.dll"), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (comctl)
    {
        PIMAGE_THUNK_DATA addr = find_delayload_thunk_by_ordinal(comctl, "uxtheme.dll", 49); // OpenNcThemeData
        if (addr)
        {
            DWORD old_protect;
            if (VirtualProtect(addr, sizeof(IMAGE_THUNK_DATA), PAGE_READWRITE, &old_protect))
            {
                if (fixed)
                {
                    addr->u1.Function = (uintptr_t)OpenNcThemeDataStub;
                }
                else
                {
                    addr->u1.Function = (uintptr_t)fnOpenNcThemeData;
                }
                VirtualProtect(addr, sizeof(IMAGE_THUNK_DATA), old_protect, &old_protect);
            }
        }
        safe_close_dll(comctl);
    }
}

static bool
check_system_build_number(uint32_t build_number)
{
    switch (build_number)
    {
        case 17763: // Win10 v1809
        case 18362: // Win10 v1903
        case 18363: // Win10 v1909
        case 19041: // Win10 v2004
        case 19042: // Win10 v20H2
        case 19043: // Win10 v21H1 Insider Beta and Release Preview Channels [2021-04-28]
        case 21337: // Win10 v21H2 Insider Dev Channel [2021-05-11]
            return true;
        default:
            // not supported
            break;
    }
    return false;
}

static bool
on_dark_create_brush(void)
{
    if (g_dark_bkgnd)
    {
        DeleteObject(g_dark_bkgnd);
    }
    return ((g_dark_bkgnd = CreateSolidBrush(rgb_dark_bk_color)) != NULL);
}

static void
on_dark_delete_brush(void)
{
    if (g_dark_bkgnd)
    {
        DeleteObject(g_dark_bkgnd);
        g_dark_bkgnd = NULL;
    }
}

intptr_t
on_dark_get_brush(void)
{
    if (!g_dark_bkgnd)
    {   // not dark mode
        g_dark_bkgnd = GetSysColorBrush(COLOR_MENU);
    }
    return (intptr_t)g_dark_bkgnd;
}

intptr_t
on_dark_theme_brush(void)
{
    if (!g_theme_bkgnd)
    {
        g_theme_bkgnd = CreateSolidBrush(eu_get_theme()->item.text.bgcolor);
    }
    return (intptr_t)g_theme_bkgnd;
}

void
on_dark_delete_theme_brush(void)
{
    if (g_theme_bkgnd)
    {
        DeleteObject(g_theme_bkgnd);
        g_theme_bkgnd = NULL;
    }
}

void
on_dark_border(HWND hwnd, bool border)
{
    intptr_t style = (intptr_t)GetWindowLongPtr(hwnd, GWL_STYLE);
    bool has_border = (style & WS_BORDER) == WS_BORDER;
    bool change = false;

    if (!has_border && border)
    {
        style |= WS_BORDER;
        change = true;
    }
    else if (has_border && !border)
    {
        style &= ~WS_BORDER;
        change = true;
    }
    if (change)
    {
        SetWindowLongPtr(hwnd, GWL_STYLE, style);
        SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }
}

static bool
on_dark_create_hot_brush(void)
{
    if (!g_dark_hot_bkgnd)
    {
        g_dark_hot_bkgnd = CreateSolidBrush(rgb_dark_hot_color);
    }
    return (g_dark_hot_bkgnd != NULL);
}

static void
on_dark_delete_hot_brush(void)
{
    if (g_dark_hot_bkgnd)
    {
        DeleteObject(g_dark_hot_bkgnd);
        g_dark_hot_bkgnd = NULL;
    }
}

intptr_t
on_dark_get_hot_brush(void)
{
    return (intptr_t)g_dark_hot_bkgnd;
}

intptr_t
on_dark_set_contorl_color(WPARAM wParam)
{
    if (on_dark_supports())
    {
        HDC hdc = (HDC)wParam;
        set_text_color(hdc, true);
        set_bk_color(hdc, true);
        return (intptr_t)g_dark_bkgnd;
    }
    return 0;
}

colour
on_dark_light_color(colour cr_base, float factor)
{
    factor = max(factor, 1.0f);
    uint8_t red, blue, green;
    uint8_t red_hilite, blue_hilite, green_hilite;
    red = GetRValue(cr_base);
    blue = GetBValue(cr_base);
    green = GetGValue(cr_base);
    red_hilite = (uint8_t)min((int)(red * factor), 255);
    blue_hilite = (uint8_t)min((int)(blue * factor), 255);
    green_hilite = (uint8_t)min((int)(green * factor), 255);
    return RGB(red_hilite, green_hilite, blue_hilite);
}

void
eu_on_dark_release(bool shutdown)
{
    if (shutdown)
    {
        safe_close_dll(g_uxtheme);
    }
    else if (g_dark_enabled)
    {
        HWND hwnd = eu_module_hwnd();
        on_dark_set_titlebar(hwnd, false);
        on_dark_allow_app(false);
        on_dark_allow_window(hwnd, false);
        on_statusbar_dark_release(true);
        g_dark_supported = false;
        g_dark_enabled = false;
        on_toolbar_refresh(hwnd);
        fnFlushMenuThemes();
        safe_close_dll(g_uxtheme);
        SendMessageTimeout(HWND_BROADCAST, WM_THEMECHANGED, 0, 0, SMTO_NORMAL, 10, 0);
        on_tabpage_foreach(on_tabpage_theme_changed);
        if (g_treebar)
        {
            SendMessage(g_treebar, WM_THEMECHANGED, 0, 0);
        }
        if (g_filetree)
        {
            SendMessage(g_filetree, WM_THEMECHANGED, 0, 0);
        }
        if (g_tabpages)
        {
            SendMessage(g_tabpages, WM_THEMECHANGED, 0, 0);
        }
        on_dark_delete_hot_brush();
        on_dark_delete_brush();
        on_theme_menu_release();
        on_search_dark_mode_release();
    }
}

bool
eu_on_dark_init(bool fix_scroll, bool dark)
{
    uint32_t major, minor;
    HMODULE huser32 = NULL;
    RtlGetNtVersionNumbersPtr fnRtlGetNtVersionNumbers = NULL;
    if (util_under_wine())
    {
        return false;
    }
    if (!(fnRtlGetNtVersionNumbers = (RtlGetNtVersionNumbersPtr)GetProcAddress(GetModuleHandle(_T("ntdll.dll")), "RtlGetNtVersionNumbers")))
    {
        return false;
    }
    if (!(huser32 = GetModuleHandle(_T("user32.dll"))))
    {
        return false;
    }
    if (!(fnSetWindowCompositionAttribute = (SetWindowCompositionAttributePtr)GetProcAddress(huser32, "SetWindowCompositionAttribute")))
    {
        return false;
    }
    fnRtlGetNtVersionNumbers(&major, &minor, &g_build_number);
    g_build_number &= ~0xf0000000;
    if (major == 10 && minor == 0 && check_system_build_number(g_build_number))
    {
        if ((g_uxtheme = LoadLibraryEx(_T("uxtheme.dll"), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32)) != NULL)
        {
            fnOpenNcThemeData = (OpenNcThemeDataPtr)GetProcAddress(g_uxtheme, MAKEINTRESOURCEA(49));
            fnRefreshImmersiveColorPolicyState = (RefreshImmersiveColorPolicyStatePtr)GetProcAddress(g_uxtheme, MAKEINTRESOURCEA(104));
            fnGetIsImmersiveColorUsingHighContrast = (GetIsImmersiveColorUsingHighContrastPtr)GetProcAddress(g_uxtheme, MAKEINTRESOURCEA(106));
            fnShouldAppsUseDarkMode = (ShouldAppsUseDarkModePtr)GetProcAddress(g_uxtheme, MAKEINTRESOURCEA(132));
            fnAllowDarkModeForWindow = (AllowDarkModeForWindowPtr)GetProcAddress(g_uxtheme, MAKEINTRESOURCEA(133));
            uintptr_t ord135 = (uintptr_t)GetProcAddress(g_uxtheme, MAKEINTRESOURCEA(135));
            if (g_build_number < 18362)
            {
                fnAllowDarkModeForApp = (AllowDarkModeForAppPtr)(ord135);
            }
            else
            {
                fnSetPreferredAppMode = (SetPreferredAppModePtr)(ord135);
            }
            fnFlushMenuThemes = (FlushMenuThemesPtr)GetProcAddress(g_uxtheme, MAKEINTRESOURCEA(136));
            fnIsDarkModeAllowedForWindow = (IsDarkModeAllowedForWindowPtr)GetProcAddress(g_uxtheme, MAKEINTRESOURCEA(137));
            if (fnOpenNcThemeData &&
                fnRefreshImmersiveColorPolicyState &&
                fnShouldAppsUseDarkMode &&
                fnAllowDarkModeForWindow &&
                (fnAllowDarkModeForApp || fnSetPreferredAppMode) &&
                fnFlushMenuThemes &&
                fnIsDarkModeAllowedForWindow)
            {
                g_dark_supported = true;
                on_dark_allow_app(dark);
                fnRefreshImmersiveColorPolicyState();
                g_dark_enabled = fnShouldAppsUseDarkMode() && !on_dark_high_contrast();
                if (fix_scroll)
                {
                    on_dark_fix_scrollbar(fix_scroll);
                }
            }
            if (g_dark_enabled && on_dark_create_hot_brush())
            {
                return on_dark_create_brush();
            }
            else
            {
                g_dark_supported = false;
                safe_close_dll(g_uxtheme);
            }
        }
    }
    return false;
}
