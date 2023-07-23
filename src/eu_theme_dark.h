#ifndef _H_SKYLARK_THEME_DARK_
#define _H_SKYLARK_THEME_DARK_

#include <uxtheme.h>
#include <vssym32.h>

typedef uint32_t colour;

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
typedef HRESULT (WINAPI *GetThemeSysFontPtr)(HTHEME hTheme, int iFontId, LOGFONTW *plf);
typedef HRESULT (WINAPI *GetThemePartSizePtr)(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, LPCRECT prc, int eSize, SIZE *psz);
typedef HRESULT (WINAPI *DrawThemeBackgroundPtr)(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, LPCRECT pRect, LPCRECT pClipRect);

// dwm function
typedef HRESULT (WINAPI *DwmSetWindowAttributePtr)(HWND, DWORD, LPCVOID, DWORD);
typedef HRESULT (WINAPI *DwmGetColorizationColorPtr)(DWORD *pcrColorization, BOOL  *pfOpaqueBlend);

#define  CASE_WM_CTLCOLOR_SET         \
           case WM_CTLCOLORDLG:       \
           case WM_CTLCOLORBTN:       \
           case WM_CTLCOLOREDIT:      \
           case WM_CTLCOLORSCROLLBAR: \
           case WM_CTLCOLORLISTBOX:   \
           case WM_CTLCOLORSTATIC
           
#define UpdateWindowEx(hwnd)                                                               \
  RedrawWindow((hwnd), NULL, NULL, RDW_ERASE|RDW_FRAME|RDW_INVALIDATE|RDW_INTERNALPAINT)   \

#define rgb_alpha(rgb, a)                                                                  \
  ((int)(((colour)((rgb)&0xffffff)) | (((colour)(uint8_t)((a)&0xff)) << 24)))              \

#define rgb_dark_bk11_color  (0x202020)
#define rgb_dark_bk_color    (0x2B2B2B)
#define rgb_dark_btn_color   (0x333333)
#define rgb_dark_txt_color   (0xFFFFFF)
#define rgb_dark_hot_color   (0x404040)
#define rgb_bmp_close_color  (0xd77800)
#define rgb_memu_hot1_color  (0xF7C991)
#define rgb_memu_hot2_color  (0xFFF3E5)

#ifdef __cplusplus
extern "C" {
#endif

bool on_dark_supports(void);
bool on_dark_enable(void);
bool on_dark_apps_use(void);
bool on_dark_allow_window(HWND hwnd, bool allow);
bool on_dark_high_contrast(void);
void on_dark_refresh_titlebar(HWND hwnd);
void on_dark_set_titlebar(HWND hwnd, BOOL dark);
bool on_dark_color_scheme_change(LPARAM lParam);
bool on_dark_color_scheme_change_msg(UINT message, LPARAM lParam);
void on_dark_allow_app(bool allow);
void on_dark_set_theme(HWND hwnd, const wchar_t *psz_name, const wchar_t *psz_list);
void on_dark_delete_theme_brush(void);
void on_dark_delete_hot_brush(void);
void on_dark_delete_bgbrush(void);
void on_dark_delete_brush(void);
void on_dark_border(HWND hwnd, bool border);
void on_dark_tips_theme(HWND hwnd, int msg);
colour on_dark_get_sys_colour(HWND hwnd, int colid);
colour on_dark_get_colorization_color(void);
colour on_dark_light_color(colour cr_base, float factor);
HRESULT on_dark_close_data(void *hth);
intptr_t on_dark_open_data(HWND hwnd, LPCWSTR class_list);
HRESULT on_dark_draw_background(void *hTheme, HDC hdc, int iPartId, int iStateId, LPCRECT pRect, LPCRECT pClipRect);
HRESULT on_dark_get_partsize(void *hTheme, HDC hdc, int iPartId, int iStateId, LPCRECT prc, int eSize, SIZE *psz);
intptr_t on_dark_get_hot_brush(void);
intptr_t on_dark_set_contorl_color(WPARAM wParam);
intptr_t on_dark_get_bgbrush(void);
intptr_t on_dark_theme_brush(void);

static inline uint32_t
set_bk_color(const HDC hdc, const bool use_dark)
{
    return use_dark ? SetBkColor(hdc, rgb_dark_bk_color) : SetBkColor(hdc, GetSysColor(COLOR_WINDOW));
}

static inline uint32_t
set_btnface_color(const HDC hdc, const bool use_dark)
{
    return use_dark ? SetBkColor(hdc, rgb_dark_btn_color) : SetBkColor(hdc, GetSysColor(COLOR_BTNFACE));
}

static inline uint32_t
set_text_color(const HDC hdc, const bool use_dark)
{
    return use_dark ? SetTextColor(hdc, rgb_dark_txt_color) : SetTextColor(hdc, GetSysColor(COLOR_BTNTEXT));
}

#ifdef __cplusplus
}
#endif

#endif  // _H_SKYLARK_THEME_DARK_