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

#define TAB_MIN_TOP 4
#define TAB_MIN_LEFT 40
#define TAB_MIN_WIDTH 140
#define CLOSEBUTTON_WIDTH 11
#define CLOSEBUTTON_HEIGHT 11
#define CX_ICON  16
#define CY_ICON  16
#define COLORREF2RGB(c) ((c & 0xff00) | ((c >> 16) & 0xff) | ((c << 16) & 0xff0000))

HWND g_tabpages = NULL;
static POINT g_point;
static WNDPROC old_tabproc = NULL;
static bool tab_drag = false;
static bool tab_mutil_select = false;
static volatile bool tab_do_drag;
static volatile int tab_move_from = -1;

int
on_tabpage_get_height(void)
{
    RECT rect_treebar;
    GetClientRect(g_tabpages, &rect_treebar);
    return (rect_treebar.bottom - rect_treebar.top);
}

static int
on_tabpage_internal_height(void)
{
    RECT rect_tabbar = {0};
    int tab_height = TABS_HEIGHT_DEFAULT;
    TabCtrl_GetItemRect(g_tabpages, 0, &rect_tabbar);
    tab_height = (rect_tabbar.bottom - rect_tabbar.top) * TabCtrl_GetRowCount(g_tabpages);
    return tab_height;
}

static bool
on_tabpage_has_drag(POINT *pt)
{
    int x = pt->x - g_point.x;
    int y = pt->y - g_point.y;
    if (abs(x) > 3 || abs(y) > 3)
    {
        return true;
    }
    return false;
}

static void
on_tabpage_destroy_tabbar(void)
{
    HIMAGELIST himg = TabCtrl_GetImageList(g_tabpages);
    if (himg)
    {
        ImageList_Destroy(himg);
    }
    if (g_tabpages)
    {
        g_tabpages = NULL;
    }
}

LRESULT
on_tabpage_draw_item(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(hwnd);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    return 1;
}

bool
init_icon_img_list(HWND htab)
{
    bool res = false;
    HBITMAP hbmp = NULL;
    HINSTANCE hinst = eu_module_handle();
    if ((hbmp = (HBITMAP) LoadImage(hinst, MAKEINTRESOURCE(IDT_BIRD), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_DEFAULTSIZE)) != NULL)
    {
        BITMAP bm = {0};
        GetObject(hbmp, sizeof(BITMAP), &bm);
        HIMAGELIST himg = ImageList_Create(IMAGEWIDTH, IMAGEHEIGHT, bm.bmBitsPixel | ILC_MASK, bm.bmWidth / IMAGEWIDTH, 1);
        if (himg)
        {
            ImageList_AddMasked(himg, hbmp, 0xF0F0F0);
            TabCtrl_SetImageList(htab, himg);
            res = true;
        }
        DeleteObject((HGDIOBJ) hbmp);
    }
    return res;
}

static inline void
on_tabpage_set_active(int index)
{
    if (g_tabpages && index >= 0)
    {
        TabCtrl_DeselectAll(g_tabpages, true);
        TabCtrl_SetCurSel(g_tabpages, index);
        TabCtrl_SetCurFocus(g_tabpages, index);
    }
}

static inline void
on_tabpage_setpos(eu_tabpage *p)
{
    if (p && p->hwnd_sc)
    {
        eu_setpos_window(p->hwnd_sc, HWND_TOP, p->rect_sc.left, p->rect_sc.top, p->rect_sc.right - p->rect_sc.left,
                         p->rect_sc.bottom - p->rect_sc.top, SWP_SHOWWINDOW);
    }
}

static void
on_tabpage_changing(int index)
{
    eu_tabpage *p = NULL;
    if((p = on_tabpage_get_ptr(index)) != NULL)
    {
        util_set_undo(p);
        util_set_title(p);
        on_toolbar_update_button();
        SendMessage(eu_module_hwnd(), WM_TAB_CLICK, (WPARAM)p, 0);
        if (p->pmod)
        {
            on_tabpage_setpos(p);
        }
    }
}

static void
on_tabpage_deselect(int index)
{
    if (g_tabpages && index >= 0)
    {
        RECT rc;
        TCITEM tci = {TCIF_STATE, 0, TCIS_BUTTONPRESSED};
        SendMessage(g_tabpages, TCM_SETITEM, index, (LPARAM)&tci);
        TabCtrl_GetItemRect(g_tabpages, index, &rc);
        InvalidateRect(g_tabpages, &rc, true);
    }
}

static bool
on_tabpage_nfocus(int index)
{
    if (g_tabpages && index >= 0)
    {
        TCITEM tci = {TCIF_STATE};
        tci.dwStateMask = TCIS_BUTTONPRESSED;
        if (SendMessage(g_tabpages, TCM_GETITEM, index, (LPARAM)&tci))
        {
            return ((tci.dwState & TCIS_BUTTONPRESSED) || (TabCtrl_GetCurSel(g_tabpages) == index));
        }
    }
    return false;
}

int
on_tabpage_sel_number(int **pvec, const bool ascending)
{
    int num = 0;
    int count = TabCtrl_GetItemCount(g_tabpages);
    int index = ascending ? 0 : count - 1;
    for (; ascending ? index < count : index >= 0; ascending ? ++index : --index)
    {
        if (on_tabpage_nfocus(index))
        {
            if (pvec)
            {
                cvector_push_back(*pvec, index);
            }
            ++num;
        }
    }
    return num;
}

int
on_tabpage_sel_path(wchar_t ***pvec, bool *hex)
{
    int num = 0;
    eu_tabpage *p = NULL;
    for (int i = 0, count = TabCtrl_GetItemCount(g_tabpages); i < count; ++i)
    {
        if (on_tabpage_nfocus(i) && (p = on_tabpage_get_ptr(i)) && !p->is_blank && !url_has_remote(p->pathfile))
        {
            cvector_push_back(*pvec, _wcsdup(p->pathfile));
            if (hex && p->hex_mode)
            {
                *hex = p->hex_mode;
            }
            ++num;
        }
    }
    return num;
}

void
on_tabpage_active_one(int index)
{
    int num = 0;
    cvector_vector_type(int) v = NULL;
    num = on_tabpage_sel_number(&v, false);
    if (num > 1 && eu_cvector_at(v, index) >= 0)
    {
        on_tabpage_changing(index);
        tab_mutil_select = true;
    }
    else if (!on_tabpage_nfocus(index))
    {
        cvector_for_each(v, on_tabpage_deselect);
        TabCtrl_SetCurSel(g_tabpages, index);
        TabCtrl_SetCurFocus(g_tabpages, index);
        on_tabpage_changing(index);
    }
    cvector_freep(&v);
}

static void
on_tabpage_draw_sign(const HDC hdc, const LPRECT lprect)
{
    const TCHAR *text  = util_os_version() < 603 ? _T("x") : _T("✕");
    HFONT hfont = CreateFont(-14, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                             CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, _T("Arial"));
    HGDIOBJ oldj = SelectObject(hdc, hfont);
    RECT rc = {lprect->right - CLOSEBUTTON_WIDTH - TAB_MIN_TOP,
               lprect->top + 1,
               lprect->right,
               lprect->bottom
              };
    DrawText(hdc, text, (int)_tcslen(text), &rc, DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    SelectObject(hdc, oldj);
    DeleteObject(hfont);
}

static void
on_tabpage_draw_close(HWND hwnd, const LPRECT lprect, bool sel)
{
    HDC hdc = GetDC(hwnd);
    colour cr = 0;
    if (sel)
    {
        cr = eu_get_theme()->item.activetab.bgcolor;
        SetBkColor(hdc, cr);
    }
    else
    {
        cr = set_btnface_color(hdc, on_dark_enable());
    }
    on_tabpage_draw_sign(hdc, lprect);
    ReleaseDC(hwnd, hdc);
}

static void
on_tabpage_flush_close(HWND hwnd, const LPRECT lprect)
{
    RECT rc = {lprect->right - CLOSEBUTTON_WIDTH - TAB_MIN_TOP,
               lprect->top + 1,
               lprect->right,
               lprect->bottom
              };
    InvalidateRect(hwnd, &rc, true);
}

static bool
on_tabpage_hit_button(const LPRECT lprect, const LPPOINT pt)
{
    RECT rc = {lprect->right - CLOSEBUTTON_WIDTH - TAB_MIN_TOP,
               lprect->top + 1,
               lprect->right,
               lprect->bottom
              };
    return PtInRect(&rc, *pt);
}

static int
on_tabpage_hit_index(const LPPOINT pt)
{
    TCHITTESTINFO hit_info = {{pt->x, pt->y},};
    return (int)(SendMessage(g_tabpages, TCM_HITTEST, 0, (LPARAM)(&hit_info)));
}

static void
on_tabpage_paint_draw(HWND hwnd, HDC hdc)
{
    bool dark_mode = on_dark_enable();
    HGDIOBJ old_font = SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));
    while (old_font)
    {
        set_text_color(hdc, dark_mode);
        for (int index = 0, count = TabCtrl_GetItemCount(hwnd); index < count; ++index)
        {
            RECT rc;
            colour cr = 0;
            HIMAGELIST himg = TabCtrl_GetImageList(hwnd);
            TCITEM tci = {TCIF_PARAM | TCIF_TEXT | TCIF_IMAGE};
            TabCtrl_GetItem(hwnd, index, &tci);
            eu_tabpage *p = (eu_tabpage *) (tci.lParam);
            if (!p || !himg)
            {
                break;
            }
            TabCtrl_GetItemRect(hwnd, index, &rc);
            rc.right -= 1;  // 多行TCS_BUTTONS样式下有些标签没有隔开?
            FrameRect(hdc, &rc, dark_mode ? GetSysColorBrush(COLOR_3DDKSHADOW) : GetSysColorBrush(COLOR_BTNSHADOW));
            if (on_tabpage_nfocus(index))
            {   // 从主题获取COLOR_HIGHLIGHT值
                cr = eu_get_theme()->item.activetab.bgcolor;
                SetBkColor(hdc, cr);
                ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
            }
            else
            {
                cr = set_btnface_color(hdc, dark_mode);
            }
            if (p->file_attr & FILE_ATTRIBUTE_READONLY)
            {
                ImageList_Draw(himg, 1, hdc, rc.left + 6, rc.top + (rc.bottom - rc.top - CY_ICON)/2, ILD_TRANSPARENT);
            }
            else if (p->be_modify)
            {
                ImageList_Draw(himg, 0, hdc, rc.left + 6, rc.top + (rc.bottom - rc.top - CY_ICON)/2, ILD_TRANSPARENT);
            }
            if (STR_NOT_NUL(p->filename))
            {
                DrawText(hdc, p->filename, (int)_tcslen(p->filename), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
                if (eu_get_config()->m_close_draw == IDM_TABCLOSE_ALWAYS)
                {
                    on_tabpage_draw_sign(hdc, &rc);
                }
            }
            if (eu_get_config()->m_close_draw == IDM_TABCLOSE_FOLLOW)
            {
                POINT pt;
                GetCursorPos(&pt);
                ScreenToClient(hwnd, &pt);
                if (on_tabpage_hit_index(&pt) == index)
                {
                    rc.right += 1;
                    on_tabpage_draw_sign(hdc, &rc);
                }
            }
        }
        SelectObject(hdc, old_font);
        break;
    }
}

static void
on_tabpage_exchange_item(const int old_index, const int new_index, const bool active)
{
    int i = 0;
    TCITEM drag_item = {0};
    TCITEM shift_item = {0};
    TCHAR str1[MAX_PATH] = {0};
    TCHAR str2[MAX_PATH] = {0};
    drag_item.mask = shift_item.mask = TCIF_TEXT | TCIF_PARAM;
    drag_item.cchTextMax = shift_item.cchTextMax = MAX_PATH;
    drag_item.pszText = str1;
    shift_item.pszText = str2;
    TabCtrl_GetItem(g_tabpages, old_index, &drag_item);
    if (old_index == new_index || old_index < 0 || new_index < 0)
    {
        return;
    }
    if (active)
    {
        if (old_index > new_index)
        {
            for (i = old_index; i > new_index; --i)
            {
                SendMessage(g_tabpages, TCM_GETITEM, i - 1, (LPARAM)(&shift_item));
                SendMessage(g_tabpages, TCM_SETITEM, i, (LPARAM)(&shift_item));
            }
        }
        else
        {
            for (i = old_index; i < new_index; ++i)
            {
                SendMessage(g_tabpages, TCM_GETITEM, i + 1, (LPARAM)(&shift_item));
                SendMessage(g_tabpages, TCM_SETITEM, i, (LPARAM)(&shift_item));
            }
        }
        SendMessage(g_tabpages, TCM_SETITEM, new_index, (LPARAM)(&drag_item));
        on_tabpage_select_index(new_index);
    }
    else
    {
        SendMessage(g_tabpages, TCM_GETITEM, new_index, (LPARAM)(&shift_item));
        SendMessage(g_tabpages, TCM_SETITEM, old_index, (LPARAM)(&shift_item));
        SendMessage(g_tabpages, TCM_SETITEM, new_index, (LPARAM)(&drag_item));
    }
}

static int
on_tabpage_parser_bakup(void *data, int count, char **column, char **names)
{
    wchar_t path[MAX_BUFFER] = {0};
    file_backup *pbak = (file_backup *)data;
    for (int i = 0; i < count; ++i)
    {
        if (STRCMP(names[i], ==, "szTabId"))
        {
            pbak->tab_id = (short)atoi(column[i]);
        }
        else if (STRCMP(names[i], ==, "szRealPath"))
        {
            MultiByteToWideChar(CP_UTF8, 0, column[i], -1, path, MAX_BUFFER);
        }
        else if (STRCMP(names[i], ==, "szBakPath"))
        {
            MultiByteToWideChar(CP_UTF8, 0, column[i], -1, pbak->bak_path, _countof(pbak->bak_path));
        }
        else if (STRCMP(names[i], ==, "szMark"))
        {
            strncpy(pbak->mark_id, column[i], MAX_BUFFER-1);
        }
        else if (STRCMP(names[i], ==, "szFold"))
        {
            strncpy(pbak->fold_id, column[i], MAX_BUFFER-1);
        }
        else if (STRCMP(names[i], ==, "szLine"))
        {
            pbak->postion = _atoz(column[i]);
        }
        else if (STRCMP(names[i], ==, "szCp"))
        {
            pbak->cp = atoi(column[i]);
        }
        else if (STRCMP(names[i], ==, "szBakCp"))
        {
            pbak->bakcp = atoi(column[i]);
        }
        else if (STRCMP(names[i], ==, "szEol"))
        {
            pbak->eol = atoi(column[i]);
        }
        else if (STRCMP(names[i], ==, "szBlank"))
        {
            pbak->blank = atoi(column[i]);
        }
        else if (STRCMP(names[i], ==, "szHex"))
        {
            pbak->hex = atoi(column[i]);
        }
        else if (STRCMP(names[i], ==, "szFocus"))
        {
            pbak->focus = atoi(column[i]);
        }
        else if (STRCMP(names[i], ==, "szZoom"))
        {
            pbak->zoom = atoi(column[i]);
        }
        else if (STRCMP(names[i], ==, "szStatus"))
        {
            pbak->status = atoi(column[i]);
        }
    }
    if (_tcscmp(path, pbak->rel_path) == 0)
    {
        return SKYLARK_SQL_END;
    }
    return 0;
}

static void
on_tabpage_send_file(const HWND hwin, const int index)
{
    eu_tabpage *p = on_tabpage_get_ptr(index);
    if (p && (!p->is_blank  || TAB_NOT_NUL(p)))
    {
        file_backup bak = {0};
        int err = SKYLARK_NOT_OPENED;
        _tcsncpy(bak.rel_path, p->pathfile, _countof(bak.rel_path));
        if (!eu_get_config()->m_session)
        {
            if (!p->is_blank)
            {
                sptr_t pos = eu_sci_call(p, SCI_GETCURRENTPOS, 0, 0);
                if (pos > 0)
                {
                    sptr_t lineno = eu_sci_call(p, SCI_LINEFROMPOSITION, pos, 0);
                    sptr_t row = eu_sci_call(p, SCI_POSITIONFROMLINE, lineno, 0);
                    bak.x = lineno + 1;
                    bak.y = eu_int_cast(pos - row + 1);
                    bak.hex = p->hex_mode;
                }
                err = on_file_close(p, FILE_ONLY_CLOSE);
            }
        }
        else
        {
            if (!(err = on_file_close(p, FILE_REMOTE_CLOSE)))
            {
                const char *sql = "SELECT * FROM skylark_session;";
                err = on_sql_post(sql, on_tabpage_parser_bakup, &bak);
                if (err != SKYLARK_OK && err != SQLITE_ABORT)
                {
                    printf("on_sql_post failed in %s, cause: %d\n", __FUNCTION__, err);
                }
                else
                {
                    err = SKYLARK_OK;
                }
            }
        }
        if (err == SKYLARK_OK)
        {
            COPYDATASTRUCT cpd = { 0 };
            cpd.lpData = (PVOID) &bak;
            cpd.cbData = (DWORD) sizeof(file_backup);
            SendMessageW(hwin, WM_COPYDATA, 0, (LPARAM) &cpd);
            SwitchToThisWindow(hwin, true);
        }
    }
}

static void
on_tabpage_send_group(const HWND hwin)
{
    cvector_vector_type(int) v = NULL;
    if (on_tabpage_sel_number(&v, false) > 0 && eu_cvector_at(v, tab_move_from) >= 0)
    {
        for (size_t i = 0; i < cvector_size(v); ++i)
        {
            on_tabpage_send_file(hwin, v[i]);
        }
    }
    cvector_freep(&v);
}

static void
on_tabpage_new_hinst(void)
{
    int num = 0;
    int index = -1;
    cvector_vector_type(int) v = NULL;
    if ((num = on_tabpage_sel_number(&v, false)) > 0 && (index = eu_cvector_at(v, tab_move_from)) >= 0)
    {
        uint32_t pid = 0;
        HWND hwin = NULL;
        on_file_out_open(v[0], &pid);
        if (num > 1 && pid > 0)
        {   // 等待新进程窗口初始化
            Sleep(600);
            // 发送其他文件到新进程
            if ((hwin = util_get_hwnd(pid)) != NULL)
            {
                for (size_t i = 1; i < cvector_size(v); ++i)
                {
                    on_tabpage_send_file(hwin, v[i]);
                }
            }
        }
    }
    cvector_freep(&v);
}

static void
on_tabpage_drag_mouse(const HWND hwin)
{
    if (hwin && !eu_get_config()->inter_reserved_1)
    {
        int fn = 0;
        TCHAR name[FILESIZE] = {0};
        HWND parent = GetParent(hwin);
        GetClassName(hwin, name, FILESIZE - 1);
        NMHDR nmhdr = {0};
        eu_send_notify(g_tabpages, EU_SAVE_CONFIG, &nmhdr);
        if (parent == eu_hwnd_self() || hwin == eu_hwnd_self())
        {   // 拖放在skylark编辑器本身界面上, 启动新实例
            on_tabpage_new_hinst();
        }
        else if (!(fn = _tcscmp(name, APP_CLASS)) || (!_tcscmp(name, TEXT("Scintilla"))) || (!_tcscmp(name, HEX_CLASS)) || (!_tcscmp(name, WC_TABCONTROL)))
        {
            int code = fn ? (int)SendMessage(parent, WM_SKYLARK_DESC, 0, 0) : (int)SendMessage(hwin, WM_SKYLARK_DESC, 0, 0);
            if (code != eu_int_cast(WM_SKYLARK_DESC))
            {   // 确认不是skylark编辑器, 启动新实例
                on_tabpage_new_hinst();
            }
            else
            {   // 拖放在另一个skylark编辑器上, 发送文件到窗口句柄
                if (!_tcscmp(name, APP_CLASS))
                {
                    on_tabpage_send_group(hwin);
                }
                else
                {
                    on_tabpage_send_group(parent);
                }
            }
        }
        else
        {   // 拖放在空白处, 启动新实例
            on_tabpage_new_hinst();
        }
    }
}

static void
on_tabpage_menu_callback(HMENU hpop, void *param)
{
    eu_tabpage *p = (eu_tabpage *)param;
    if (p && hpop)
    {
        int num = on_tabpage_sel_number(NULL, false);
        if (num > 1)
        {
            TCHAR sub_str[MAX_PATH] = {0};
            LOAD_I18N_RESSTR(IDS_TABPAGE_CLOSE_NUM, mstr);
            _sntprintf(sub_str, MAX_PATH - 1, mstr, num);
            ModifyMenu(hpop, 0, MF_BYPOSITION | MF_STRING, IDM_FILE_CLOSE, sub_str);
        }
        util_enable_menu_item(hpop, IDM_TABPAGE_SAVE, on_sci_doc_modified(p) && !eu_sci_call(p,SCI_GETREADONLY, 0, 0));
        util_set_menu_item(hpop, IDM_TABPAGE_LOCKED, eu_get_config()->inter_reserved_1);
        util_enable_menu_item(hpop, IDM_EDIT_OTHER_EDITOR, !p->is_blank);
        util_enable_menu_item(hpop, IDM_EDIT_OTHER_BCOMPARE, num > 1 && num < 4);
        util_enable_menu_item(hpop, IDM_FILE_WORKSPACE, !p->is_blank);
        util_enable_menu_item(hpop, IDM_FILE_EXPLORER, !p->is_blank);
    }
}

static void
on_tabpage_pressed(int index)
{
    if (g_tabpages && index >= 0)
    {
        TCITEM tci = {TCIF_STATE, 0, TCIS_BUTTONPRESSED};
        SendMessage(g_tabpages, TCM_GETITEM, index, (LPARAM)&tci);
        if (!(tci.dwState & TCIS_BUTTONPRESSED))
        {
            tci.dwState |= TCIS_BUTTONPRESSED;
            SendMessage(g_tabpages, TCM_SETITEM, index, (LPARAM)&tci);
        }
    }
}

static void
on_tabpage_set_cursor(const int index)
{
    RECT rc;
    POINT point;
    GetCursorPos(&point);
    MapWindowPoints(HWND_DESKTOP, g_tabpages, &point, 2);
    TabCtrl_GetItemRect(g_tabpages, index, &rc);
    if (!PtInRect(&rc, point))
    {
        int cx = (rc.right - rc.left)/2;
        int cy = (rc.bottom - rc.top)/2;
        rc.left += cx;
        rc.top += cy;
        MapWindowPoints(g_tabpages, HWND_DESKTOP, (POINT*)(&rc), 2);
        SetCursorPos(rc.left, rc.top);
    }
}

static void
on_tabpage_arr_drag(const int index)
{
    int num = 0;
    cvector_vector_type(int) v = NULL;
    cvector_vector_type(int) out = NULL;
    if ((num = on_tabpage_sel_number(&v, true)) > 1)
    {
        int i, c = 0;
        int end = eu_int_cast(cvector_size(v));
        num = eu_cvector_at(v, index);
        for (i = num - 1; i >= 0; --i)
        {
            ++c;
            cvector_push_back(out, index - c);
            if (v[i] != index - c)
            {
                on_tabpage_exchange_item(v[i], index - c, false);
            }
        }
        for (i = num + 1, c = 0; i < end; ++i)
        {
            ++c;
            cvector_push_back(out, index + c);
            if (v[i] != index + c)
            {
                on_tabpage_exchange_item(v[i], index + c, false);
            }
        }
        on_tabpage_select_index(index);
        cvector_for_each(out, on_tabpage_pressed);
        on_tabpage_set_cursor(index);
    }
    cvector_freep(&v);
    cvector_freep(&out);
}

static void
on_tabpage_arr_drag2(const int from, const int dest)
{
    if (eu_get_config() && !eu_get_config()->inter_reserved_1)
    {
        int num = 0;
        cvector_vector_type(int) v = NULL;
        cvector_vector_type(int) out = NULL;
        if ((num = on_tabpage_sel_number(&v, true)) > 1)
        {
            int i, c = 0;
            int end = eu_int_cast(cvector_size(v));
            if (eu_cvector_at(v, dest) < 0)
            {
                if (from > dest)
                {
                    for (i = 0, c = 0; i < end; ++i, ++c)
                    {
                        cvector_push_back(out, dest + c);
                        on_tabpage_exchange_item(v[i], dest + c, false);
                    }
                }
                else
                {
                    for (i = end - 1, c = 0; i >= 0; --i, ++c)
                    {
                        cvector_push_back(out, dest - c);
                        on_tabpage_exchange_item(v[i], dest - c, false);
                    }
                }
                on_tabpage_select_index(dest);
                cvector_for_each(out, on_tabpage_pressed);
                on_tabpage_set_cursor(dest);
            }
        }
        else if (eu_cvector_at(v, dest) < 0)
        {
            on_tabpage_exchange_item(from, dest, true);
        }
        cvector_freep(&v);
        cvector_freep(&out);
    }
}

LRESULT CALLBACK
on_tabpage_proc_callback(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int count = 0;
    int index = 0;
    eu_tabpage *p = NULL;
    switch (message)
    {
        case WM_ERASEBKGND:
        {
            RECT rc = {0};
            GetClientRect(hwnd, &rc);
            FillRect((HDC)wParam, &rc, eu_theme_index() == THEME_WHITE ? GetSysColorBrush(COLOR_MENU) : (HBRUSH)on_dark_get_bgbrush());
            return 1;
        }
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            on_tabpage_paint_draw(hwnd, hdc);
            EndPaint(hwnd, &ps);
            break;
        }
        case WM_SIZE:
        {
            break;
        }
        case WM_COMMAND:
        {
            if (HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDM_TAB_CLOSE)
            {
                POINT pt;
                GetCursorPos(&pt);
                ScreenToClient(hwnd, &pt);
                LPARAM lparam = MAKELONG(pt.x, pt.y);
                PostMessage(hwnd, WM_MBUTTONUP, 0, lparam);
            }
            break;
        }
        case WM_LBUTTONDBLCLK:
        {
            if (eu_get_config()->m_close_way == IDM_VIEW_TAB_LEFT_DBCLICK)
            {
                PostMessage(hwnd, WM_MBUTTONUP, 0, lParam);
                return 1;
            }
            break;
        }
        case WM_MOUSEMOVE:
        {
            RECT rect;
            POINT point = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            TRACKMOUSEEVENT MouseEvent = {sizeof(TRACKMOUSEEVENT), TME_HOVER | TME_LEAVE, hwnd, HOVER_DEFAULT};
            TrackMouseEvent(&MouseEvent);
            GetClientRect(hwnd, &rect);
            if (!PtInRect(&rect, point))
            {
                SetCursor(LoadCursor(eu_module_handle(), MAKEINTRESOURCE(IDC_CURSOR_DRAG)));
                break;
            }
            count = TabCtrl_GetItemCount(hwnd);
            for (index = 0; index < count; ++index)
            {
                if (!(p = on_tabpage_get_ptr(index)))
                {
                    break;
                }
                TabCtrl_GetItemRect(hwnd, index, &rect);
                if (PtInRect(&rect, point))
                {
                    if (!tab_do_drag && tab_drag)
                    {
                        tab_do_drag = true;
                        on_tabpage_arr_drag(index);
                        break;
                    }
                    if (on_tabpage_has_drag(&point) && KEY_DOWN(VK_LBUTTON))
                    {
                        tab_drag = true;
                    }
                    else if (eu_get_config()->m_close_draw == IDM_TABCLOSE_FOLLOW && !tab_drag && !p->at_close)
                    {
                        on_tabpage_draw_close(hwnd, &rect, on_tabpage_nfocus(index));
                        p->at_close = true;
                    }
                }
                else if (eu_get_config()->m_close_draw == IDM_TABCLOSE_FOLLOW && p->at_close)
                {
                    on_tabpage_flush_close(hwnd, &rect);
                    p->at_close = false;
                }
            }
            break;
        }
        case WM_LBUTTONDOWN:
        {
            g_point.x = GET_X_LPARAM(lParam);
            g_point.y = GET_Y_LPARAM(lParam);
            count = TabCtrl_GetItemCount(hwnd);
            tab_move_from = -1;
            tab_drag = false;
            tab_mutil_select = false;
            tab_do_drag = false;
            if ((tab_move_from = on_tabpage_hit_index(&g_point)) != -1)
            {
                RECT rc;
                TabCtrl_GetItemRect(hwnd, tab_move_from, &rc);
                if (eu_get_config()->m_close_draw != IDM_TABCLOSE_NONE && on_tabpage_hit_button(&rc, &g_point))
                {
                    PostMessage(hwnd, WM_MBUTTONUP, 0, lParam);
                    return 1;
                }
                if (KEY_UP(VK_CONTROL))
                {
                    on_tabpage_active_one(tab_move_from);
                }
                else if (on_tabpage_nfocus(tab_move_from))
                {
                    if (TabCtrl_GetCurSel(hwnd) != tab_move_from)
                    {
                        on_tabpage_deselect(tab_move_from);
                    }
                    return 1;
                }
                // TLS_BUTTONS is already captured on Windows but WINE/ReactOS must SetCapture
                if (GetCapture() != hwnd && util_under_wine())
                {
                    SetCapture(hwnd);
                }
            }
            break;
        }
        case WM_LBUTTONUP:
        {
            RECT rect = {0};
            POINT point = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            GetClientRect(hwnd, &rect);
            if (util_under_wine() && GetCapture() == hwnd)
            {
                ReleaseCapture();
            }
            if (tab_move_from >= 0)
            {
                int count = TabCtrl_GetItemCount(hwnd);
                if (!PtInRect(&rect, point) && !eu_get_config()->inter_reserved_1)
                {   // Get cursor position of "Screen"
                    GetCursorPos(&point);
                    on_tabpage_drag_mouse(WindowFromPoint(point));
                    return 1;
                }
                if ((index = on_tabpage_hit_index(&point)) != -1)
                {
                    if (tab_drag && index != tab_move_from)
                    {
                        on_tabpage_arr_drag2(tab_move_from, index);
                        break;
                    }
                    if (tab_mutil_select && !tab_drag)
                    {
                        tab_mutil_select = false;
                        on_tabpage_select_index(index);
                    }
                }
                tab_drag = false;
            }
            break;
        }
        case WM_RBUTTONDOWN:
        {
            POINT point = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            if ((index = on_tabpage_hit_index(&point)) != -1 && !on_tabpage_nfocus(index))
            {
                on_tabpage_select_index(index);
                break;
            }
            return 1;
        }
        case WM_RBUTTONUP:
        {
            if (eu_get_config()->m_close_way == IDM_VIEW_TAB_RIGHT_CLICK && KEY_UP(VK_SHIFT))
            {
                PostMessage(hwnd, WM_MBUTTONUP, 0, lParam);
            }
            else if ((p = on_tabpage_get_ptr(TabCtrl_GetCurSel(hwnd))))
            {
                return menu_pop_track(eu_module_hwnd(), IDR_TABPAGE_POPUPMENU, 0, 0, on_tabpage_menu_callback, p);
            }
            break;
        }
        case WM_MBUTTONUP:
        {
            POINT point = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            if ((index = on_tabpage_hit_index(&point)) != -1 && (p = on_tabpage_get_ptr(index)) != NULL)
            {
                on_file_close(p, FILE_ONLY_CLOSE);
                return 1;
            }
            break;
        }
        case WM_TAB_NCCLICK:
        {
            if (eu_get_config()->m_new_way == IDM_VIEW_TAB_RIGHT_NEW && wParam == MK_RBUTTON)
            {
                on_file_new();
            }
            if (eu_get_config()->m_new_way == IDM_VIEW_TAB_DBCLICK_NEW && wParam == MK_LBUTTON)
            {
                on_file_new();
            }
            break;
        }
        case WM_MOUSEHOVER:
        {   // 鼠标悬停激活
            break;
        }
        case WM_MOUSELEAVE:
        {
            if (eu_get_config()->m_close_draw == IDM_TABCLOSE_FOLLOW)
            {
                RECT rect = {0};
                count = TabCtrl_GetItemCount(hwnd);
                for (index = 0; index < count; ++index)
                {
                    if (!(p = on_tabpage_get_ptr(index)))
                    {
                        break;
                    }
                    TabCtrl_GetItemRect(hwnd, index, &rect);
                    if (p->at_close)
                    {
                        on_tabpage_flush_close(hwnd, &rect);
                        p->at_close = false;
                    }
                }
            }
            break;
        }
        case WM_DESTROY:
        {
            on_tabpage_destroy_tabbar();
            break;
        }
        default:
            break;
    }
    return CallWindowProc(old_tabproc, hwnd, message, wParam, lParam);
}

int
on_tabpage_create_dlg(HWND hwnd)
{
    int err = 0;
    const uint32_t flags = \
          WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TCS_TOOLTIPS | TCS_BUTTONS | TCS_MULTISELECT | TCS_MULTILINE | TCS_FOCUSNEVER;
    g_tabpages = CreateWindow(WC_TABCONTROL, NULL, flags, 0, 0, 0, 0, hwnd, (HMENU)IDM_TABPAGE_BAR, eu_module_handle(), NULL);
    do
    {
        if (g_tabpages == NULL)
        {
            MSG_BOX(IDC_MSG_TABCONTROL_FAIL, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
            err = 1;
            break;
        }
        if (!init_icon_img_list(g_tabpages))
        {
            printf("init_icon_img_list return false\n");
            err = 1;
            break;
        }
        SendMessage(g_tabpages, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0);
        TabCtrl_SetPadding(g_tabpages, TAB_MIN_LEFT, TAB_MIN_TOP);
        TabCtrl_SetMinTabWidth(g_tabpages, TAB_MIN_WIDTH);
        ShowWindow(g_tabpages, SW_SHOW);
        if (!(old_tabproc = (WNDPROC) SetWindowLongPtr(g_tabpages, GWLP_WNDPROC, (LONG_PTR) on_tabpage_proc_callback)))
        {
            err = 1;
            break;
        }
    } while(0);
    if (err && g_tabpages)
    {
        DestroyWindow(g_tabpages);
        g_tabpages = NULL;
    }
    return err;
}

void
on_tabpage_close_tabs(int it)
{
    on_file_close(on_tabpage_get_ptr(it), FILE_ONLY_CLOSE);
}

void
on_tabpage_save_files(int it)
{
    on_file_save(on_tabpage_get_ptr(it), false);
}

void
on_tabpage_push_editor(int it)
{
    eu_tabpage *p = on_tabpage_get_ptr(it);
    if (p && !p->is_blank && *p->pathfile)
    {
        on_edit_push_editor(p);
    }
}

void
on_tabpage_do_file(tab_callback func)
{
    int num = 0;
    cvector_vector_type(int) v = NULL;
    num = on_tabpage_sel_number(&v, false);
    if (num > 0)
    {
        cvector_for_each(v, func);
    }
    cvector_freep(&v);
}

void
on_tabpage_adjust_box(RECT *ptp)
{
    RECT rect_main;
    RECT rect_treebar = { 0 };
    int tab_height = on_tabpage_internal_height();
    int row = TabCtrl_GetRowCount(g_tabpages);
    tab_height += row > 1 ? 3 * (row - 1) : 0;
    GetClientRect(eu_module_hwnd(), &rect_main);
    if (!eu_get_config()->m_ftree_show)
    {
        ptp->left = rect_main.left;
    }
    else
    {
        on_treebar_adjust_box(&rect_treebar);
        ptp->left = rect_treebar.right;
    }
    ptp->top = rect_main.top + on_toolbar_height();
    ptp->right = rect_main.right;
    ptp->bottom = ptp->top + tab_height + SCINTILLA_MARGIN_TOP;
}

void
on_tabpage_adjust_window(eu_tabpage *pnode)
{
    int tab_height = 0;
    RECT rect_tabpages = {0};
    if (true)
    {
        RECT rect_main;
        GetClientRect(eu_module_hwnd(), &rect_main);
        on_tabpage_adjust_box(&rect_tabpages);
        pnode->rect_sc.left = rect_tabpages.left;
        if (eu_get_config()->m_ftree_show)
        {
            pnode->rect_sc.left += SPLIT_WIDTH;
        }
        pnode->rect_sc.right = rect_tabpages.right;
        pnode->rect_sc.top = rect_tabpages.bottom;
        pnode->rect_sc.bottom = rect_main.bottom - on_statusbar_height();
    }
    if (pnode->sym_show)
    {
        if (pnode->hwnd_symlist || pnode->hwnd_symtree)
        {
            pnode->rect_sc.right -= (pnode->hwnd_symlist ? eu_get_config()->sym_list_width : eu_get_config()->sym_tree_width)
                                     + SPLIT_WIDTH;
            pnode->rect_sym.left = pnode->rect_sc.right + SPLIT_WIDTH;
            pnode->rect_sym.right = rect_tabpages.right;
            pnode->rect_sym.top = pnode->rect_sc.top;
            pnode->rect_sym.bottom = pnode->rect_sc.bottom;
        }
    }
    else if (pnode->map_show)
    {
        if (document_map_initialized)
        {
            pnode->rect_sc.right -= eu_get_config()->document_map_width + SPLIT_WIDTH;
            pnode->rect_map.left = pnode->rect_sc.right + SPLIT_WIDTH;
            pnode->rect_map.right = rect_tabpages.right;
            pnode->rect_map.top = pnode->rect_sc.top;
            pnode->rect_map.bottom = pnode->rect_sc.bottom;
        }
    }
    if (RESULT_SHOW(pnode))
    {
        int rect_bottom = pnode->rect_sc.bottom;
        pnode->rect_sc.bottom -= SPLIT_WIDTH + eu_get_config()->result_edit_height + eu_get_config()->result_list_height;
        pnode->rect_result.left = pnode->rect_sc.left;
        pnode->rect_result.right = pnode->rect_sc.right;
        pnode->rect_result.top = pnode->rect_sc.bottom + SPLIT_WIDTH;
        pnode->rect_result.bottom = rect_bottom;
        if (pnode->hwnd_qrtable)
        {
            pnode->rect_result.bottom -= SPLIT_WIDTH + eu_get_config()->result_list_height;
            pnode->rect_qrtable.left = pnode->rect_sc.left;
            pnode->rect_qrtable.right = pnode->rect_sc.right;
            pnode->rect_qrtable.top = pnode->rect_result.bottom + SPLIT_WIDTH;
            pnode->rect_qrtable.bottom = rect_bottom;
        }
    }
}

int
on_tabpage_remove(eu_tabpage **ppnode)
{
    int index = 0;
    eu_tabpage *p = NULL;
    EU_VERIFY(ppnode != NULL && *ppnode != NULL && g_tabpages != NULL);
    for (int count = TabCtrl_GetItemCount(g_tabpages); index < count; ++index)
    {
        TCITEM tci = {TCIF_PARAM};
        TabCtrl_GetItem(g_tabpages, index, &tci);
        p = (eu_tabpage *) (tci.lParam);
        if (p && p == *ppnode)
        {   /* 删除控件句柄与释放资源 */
            TabCtrl_DeleteItem(g_tabpages, index);
            on_sci_free_tab(ppnode, NULL);
            break;
        }
    }
    return index;
}

static int
on_tabpage_remove_empty(eu_tabpage *pre)
{
    int count;
    int ret = 0;
    EU_VERIFY(g_tabpages != NULL);
    if ((count = TabCtrl_GetItemCount(g_tabpages)) < 1)
    {
        return 0;
    }
    for (int index = 0; index < count; ++index)
    {
        eu_tabpage *p = on_tabpage_get_ptr(index);
        if (p && p->is_blank && !TAB_NOT_NUL(p))
        {
            if (!on_sci_doc_modified(p))
            {
                ret = 1;
                TabCtrl_DeleteItem(g_tabpages, index);
                on_sci_free_tab(&p, pre);
                break;
            }
        }
    }
    return ret;
}

TCHAR *
on_tabpage_generator(TCHAR *filename, int len)
{
    EU_VERIFY(g_tabpages != NULL);
    LOAD_I18N_RESSTR(IDC_MSG_NEW_FILE, m_file);
    const TCHAR ch = _T(' ');
    const TCHAR *pstr = NULL;
    if ((pstr = _tcsrchr(m_file, ch)) != NULL && (pstr - m_file) > 0)
    {
        int ret = 1;
        int vlen = 0;
        cvector_vector_type(int) v = NULL;
        int count = TabCtrl_GetItemCount(g_tabpages);
        _tcsncpy(filename, m_file, pstr - m_file);
        for (int index = 0; index < count; ++index)
        {
            eu_tabpage *p = on_tabpage_get_ptr(index);
            if (p && p->is_blank)
            {
                size_t mlen = _tcslen(filename);
                if (_tcsncmp(p->filename, filename, mlen) == 0 && _tcslen(p->filename) > mlen + 1)
                {
                    int num = 0;
                    if (_stscanf(&p->filename[mlen + 1], _T("%d"), &num) == 1 && num > 0)
                    {
                        cvector_push_back(v, num);
                    }
                }
            }
        }
        if ((vlen = eu_int_cast(cvector_size(v))) > 0)
        {
            int i = 0;
            for (; i < vlen; ++i)
            {
                if (eu_cvector_at(v, ret) >= 0)
                {
                    i = 0;
                    ++ret;
                    continue;
                }
            }
            cvector_free(v);
        }
        _sntprintf(filename, len, m_file, ret);
    }
    return filename;
}

void
on_tabpage_newdoc_reload(void)
{
    EU_VERIFY(g_tabpages != NULL);
    LOAD_I18N_RESSTR(IDC_MSG_NEW_FILE, m_file);
    const TCHAR ch = _T(' ');
    const TCHAR *pstr = NULL;
    TCHAR filename[MAX_PATH] = {0};
    if ((pstr = _tcsrchr(m_file, ch)) != NULL && (pstr - m_file) > 0)
    {
        int count = TabCtrl_GetItemCount(g_tabpages);
        _tcsncpy(filename, m_file, pstr - m_file);
        pstr = NULL;
        for (int index = 0; index < count; ++index)
        {
            eu_tabpage *p = on_tabpage_get_ptr(index);
            if (p && p->is_blank)
            {
                TCHAR old[MAX_BUFFER] = {0};
                if ((pstr = _tcsrchr(p->pathfile, ch)) != NULL && (pstr - p->pathfile) > 0 && _tcslen(pstr) > 0 &&
                    _tcsspn(pstr + 1, _T("0123456789")) == _tcslen(pstr + 1))
                {
                    _tcsncpy(old, p->pathfile, pstr - p->pathfile);
                    if (_tcscmp(filename, old) != 0)
                    {
                        _sntprintf(filename, MAX_PATH-1, m_file, _tstoi(pstr + 1));
                        _tcscpy(p->pathfile, filename);
                        _tcscpy(p->filename, filename);
                        util_set_title(p);
                    }
                }
            }
        }
    }
}

int
on_tabpage_add(eu_tabpage *pnode)
{
    EU_VERIFY(pnode != NULL && g_tabpages != NULL);
    TCITEM tci = {TCIF_TEXT | TCIF_PARAM,};
    if (TAB_NOT_BIN(pnode) && !pnode->hex_mode && !pnode->pmod)
    {
        pnode->doc_ptr = on_doc_get_type(pnode->filename);
    }
    if (!pnode->is_blank)
    {
        on_tabpage_remove_empty(pnode);
    }
    {
        tci.pszText = pnode->filename;
        tci.lParam = (LPARAM) pnode;
        pnode->tab_id = TabCtrl_GetItemCount(g_tabpages);
    }
    if (TabCtrl_InsertItem(g_tabpages, pnode->tab_id, &tci) == -1)
    {
        printf("TabCtrl_InsertItem return failed on %s:%d\n", __FILE__, __LINE__);
        return SKYLARK_TABCTRL_ERR;
    }
    if ((pnode->fs_server.networkaddr[0] == 0 || pnode->bakpath[0]) && pnode->hex_mode)
    {
        pnode->bytes_remaining = (size_t)pnode->raw_size;
        if (!hexview_init(pnode))
        {
            TabCtrl_DeleteItem(g_tabpages, pnode->tab_id);
            return EUE_INSERT_TAB_FAIL;
        }
        return SKYLARK_OK;
    }
    // 当复用scintilla窗口时, 不重复创建
    if (!pnode->hwnd_sc && on_sci_init_dlg(pnode))
    {
        TabCtrl_DeleteItem(g_tabpages, pnode->tab_id);
        return EUE_INSERT_TAB_FAIL;
    }
    return SKYLARK_OK;
}

void
on_tabpage_set_title(int ontab, TCHAR *title)
{
    EU_VERIFY(g_tabpages != NULL);
    TCITEM tci = {0};
    tci.mask = TCIF_TEXT;
    tci.pszText = title;
    TabCtrl_SetItem(g_tabpages, ontab, &tci);
    InvalidateRect(g_tabpages, NULL, true);
}

int
on_tabpage_editor_modify(eu_tabpage *pnode, const char *str)
{
    EU_VERIFY(pnode != NULL);
    if (pnode->hex_mode)
    {
        return (int)eu_sci_call(pnode, SCN_SAVEPOINTLEFT, 0, 0);
    }
    if (!pnode->plugin)
    {
        eu_sci_call(pnode, SCI_BEGINUNDOACTION, 0, 0);
        eu_sci_call(pnode, SCI_INSERTTEXT, 0, (sptr_t) str);
        eu_sci_call(pnode, SCI_DELETERANGE, 0, strlen(str));
        eu_sci_call(pnode, SCI_ENDUNDOACTION, 0, 0);
    }
    return 0;
}

int
on_tabpage_reload_file(eu_tabpage *pnode, int flags)
{
    EU_VERIFY(pnode != NULL);
    if (pnode->hex_mode || pnode->plugin)
    {
        return 0;
    }
    switch (flags)
    {
        case 0: // 保留
            on_tabpage_editor_modify(pnode, "X");
            break;
        case 1: // 丢弃
            eu_sci_call(pnode, SCI_SETSAVEPOINT, 0, 0);
            on_file_close(pnode, FILE_ONLY_CLOSE);
            break;
        case 2: // 重载, 滚动到末尾行
        {
            eu_sci_call(pnode, SCI_CLEARALL, 0, 0);
            if (on_file_load(pnode, NULL, true))
            {
                return 1;
            }
            eu_sci_call(pnode, SCI_SETUNDOCOLLECTION, 1, 0);
            eu_sci_call(pnode, SCI_EMPTYUNDOBUFFER, 0, 0);
            eu_sci_call(pnode, SCI_SETSAVEPOINT, 0, 0);
            pnode->st_mtime = util_last_time(pnode->pathfile);
            SendMessage(pnode->hwnd_sc, WM_KEYDOWN, VK_END, 0);
            break;
        }
        default:
            return 1;
    }
    return 0;
}

int
on_tabpage_theme_changed(eu_tabpage *p)
{
    if (p && p->hwnd_sc)
    {
        PostMessage(p->hwnd_sc, WM_THEMECHANGED, 0, 0);
    }
    if (p->sym_show)
    {
        if (p->hwnd_symlist)
        {
            PostMessage(p->hwnd_symlist, WM_THEMECHANGED, 0, 0);
        }
        else if (p->hwnd_symtree)
        {
            SendMessage(p->hwnd_symtree, WM_THEMECHANGED, 0, 0);
        }
    }
    if (p->result_show)
    {
        if (p->presult && p->presult->hwnd_sc)
        {
            SendMessage(p->presult->hwnd_sc, WM_THEMECHANGED, 0, (sptr_t)p);
        }
        if (p->hwnd_qrtable)
        {
            SendMessage(p->hwnd_qrtable, WM_THEMECHANGED, 0, 0);
        }
    }
    return 0;
}

bool
on_tabpage_check_map(void)
{
    for (int index = 0, count = TabCtrl_GetItemCount(g_tabpages); index < count; ++index)
    {
        eu_tabpage *p = on_tabpage_get_ptr(index);
        if (p && p->map_show)
        {
            return true;
        }
    }
    return false;
}

void
on_tabpage_foreach(tab_ptr fntab)
{
    for (int index = 0, count = TabCtrl_GetItemCount(g_tabpages); index < count; ++index)
    {
        eu_tabpage *p = on_tabpage_get_ptr(index);
        if (p)
        {
            fntab(p);
        }
    }
}

eu_tabpage *
on_tabpage_get_ptr(int index)
{
    TCITEM tci = {TCIF_PARAM};
    if (TabCtrl_GetItem(g_tabpages, index, &tci))
    {
        return (eu_tabpage *) (tci.lParam);
    }
    return NULL;
}

eu_tabpage *
on_tabpage_focus_at(void)
{
    return g_tabpages ? on_tabpage_get_ptr(TabCtrl_GetCurSel(g_tabpages)) : NULL;
}

int
on_tabpage_selection(eu_tabpage *pnode, int index)
{
    EU_VERIFY(pnode != NULL && g_tabpages != NULL);
    eu_tabpage *p = NULL;
    int count = TabCtrl_GetItemCount(g_tabpages);
    if (index < 0)
    {
        for (index = 0; index < count; ++index)
        {
            p = on_tabpage_get_ptr(index);
            if (p && p == pnode)
            {
                break;
            }
        }
    }
    if(index >= 0 && index < count)
    {
        HWND hwnd = eu_module_hwnd();
        on_tabpage_set_active(index);
        on_proc_resize(hwnd);
        if ((p = on_tabpage_get_ptr(index)))
        {   // 窗口处理过程中可能改变了标签位置, 重置它
            on_tabpage_deselect(index);
            TabCtrl_SetCurFocus(g_tabpages, index);
            TabCtrl_SetCurSel(g_tabpages, index);
            on_toolbar_update_button();
            util_set_title(p);
        }
    }
    return (index >= 0 && index < count ? index : SKYLARK_TABCTRL_ERR);
}

eu_tabpage *
on_tabpage_get_handle(void *hwnd_sc)
{
    eu_tabpage *p = NULL;
    EU_VERIFY(g_tabpages != NULL);
    for (int index = 0, count = TabCtrl_GetItemCount(g_tabpages); index < count; ++index)
    {
        p = on_tabpage_get_ptr(index);
        if (p && p->hwnd_sc == hwnd_sc)
        {
            return p;
        }
    }
    return NULL;
}

int
on_tabpage_get_index(eu_tabpage *pnode)
{
    if (g_tabpages)
    {
        eu_tabpage *p = NULL;
        for (int index = 0, count = TabCtrl_GetItemCount(g_tabpages); index < count; ++index)
        {
            if ((p = on_tabpage_get_ptr(index)) && p && (p == pnode))
            {
                return index;
            }
        }
    }
    return SKYLARK_TABCTRL_ERR;
}

void
on_tabpage_active_tab(eu_tabpage *pnode)
{
    on_tabpage_set_active(on_tabpage_get_index(pnode));
}

eu_tabpage *
on_tabpage_select_index(int index)
{
    eu_tabpage *p = on_tabpage_get_ptr(index);
    if (p)
    {
        on_tabpage_selection(p, index);
        return p;
    }
    return NULL;
}

void
on_tabpage_switch_next(HWND hwnd)
{
    EU_VERIFY(g_tabpages != NULL);
    UNREFERENCED_PARAMETER(hwnd);
    int count = TabCtrl_GetItemCount(g_tabpages);
    int index = TabCtrl_GetCurSel(g_tabpages);
    if (index >= 0 && count > 1 && index < count)
    {
        index = (index < count - 1 ? index + 1 : 0);
        on_tabpage_active_one(index);
    }
}

void
on_tabpage_symlist_click(eu_tabpage *pnode)
{
    if (pnode && pnode->doc_ptr && pnode->doc_ptr->fn_click_symlist)
    {
        pnode->doc_ptr->fn_click_symlist(pnode);
    }
}
