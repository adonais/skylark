#ifndef _H_SKYLARK_THEME_DARK_
#define _H_SKYLARK_THEME_DARK_

#define  CASE_WM_CTLCOLOR_SET         \
           case WM_CTLCOLORDLG:       \
           case WM_CTLCOLORBTN:       \
           case WM_CTLCOLOREDIT:      \
           case WM_CTLCOLORSCROLLBAR: \
           case WM_CTLCOLORLISTBOX:   \
           case WM_CTLCOLORSTATIC
           
#define UpdateWindowEx(hwnd)                                                               \
  RedrawWindow((hwnd), NULL, NULL, RDW_ERASE|RDW_FRAME|RDW_INVALIDATE|RDW_INTERNALPAINT)   \

#define rgb_dark_bk_color    (0x383838)
#define rgb_dark_btn_color   (0x333333)
#define rgb_dark_txt_color   (0xFFFFFF)
#define rgb_dark_hot_color   (0x404040)
#define rgb_high_light_color (0xd77800)

typedef uint32_t colour;
  
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
void on_dark_border(HWND hwnd, bool border);
colour on_dark_get_sys_colour(HWND hwnd, int colid);
colour on_dark_get_colorization_color(void);
colour on_dark_light_color(colour cr_base, float factor);
HRESULT on_dark_close_data(void *hth);
intptr_t on_dark_open_data(HWND hwnd, LPCWSTR class_list);
intptr_t on_dark_get_hot_brush(void);
intptr_t on_dark_set_contorl_color(WPARAM wParam);
intptr_t on_dark_get_brush(void);
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