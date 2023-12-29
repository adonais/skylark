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

#define TAB_MIN_RIGHT 8
#define TAB_MIN_LEFT 30
#define TAB_MIN_WIDTH 160
#define CLOSEBUTTON_DEFAULT 11
#define CX_ICON  16
#define CY_ICON  16
#define COLORREF2RGB(c) ((c & 0xff00) | ((c >> 16) & 0xff) | ((c << 16) & 0xff0000))
#define TABS_CHILD_WIDGETS(n_) \
    ((!_tcscmp((n_), TEXT("Scintilla"))) || (!_tcscmp((n_), HEX_CLASS)) || (!_tcscmp((n_), WC_TABCONTROL)))

static POINT g_point = {0};
static volatile sptr_t old_tabproc = 0;
static bool tab_drag = false;
static bool tab_mutil_select = false;
static volatile bool tab_do_drag = false;;
static volatile int tab_move_from = -1;
static HCURSOR g_drag_hcursor = NULL;

int
on_tabpage_get_height(const int i)
{
    int tab_height = eu_dpi_scale_xy(0, TABS_HEIGHT_DEFAULT);
    HWND htab = !i ? HMAIN_GET : HSLAVE_GET;
    if (htab)
    {
        RECT tabs = {0};
        int row = TabCtrl_GetRowCount(htab);
        TabCtrl_GetItemRect(htab, 0, &tabs);
        tab_height = row * (tabs.bottom - tabs.top) + (row > 1 ? (row - 1) * 3 + 1 : 1);
    }
    return tab_height;
}

static bool
on_tabpage_has_drag(POINT *pt)
{
    int x = pt->x - g_point.x;
    int y = pt->y - g_point.y;
    if (!(g_point.x || g_point.y))
    {
        return false;
    }
    if (abs(x) > 3 || abs(y) > 3)
    {
        return true;
    }
    return false;
}

static bool
on_tabpage_item_hwnd(const HWND other, const POINT *pt)
{
    if (other && pt)
    {
        RECT rc = {0};
        POINT pi = {0};
        eu_tabpage *p = NULL;
        memcpy(&pi, pt, sizeof(POINT));
        GetClientRect(other, &rc);
        MapWindowPoints(HWND_DESKTOP, other, &pi, 1); 
        if (PtInRect(&rc, pi))
        {
            return true;
        }
        else if ((p = on_tabpage_focus_at(other)) && p->hwnd_sc)
        {
            GetClientRect(p->hwnd_sc, &rc);
            MapWindowPoints(other, p->hwnd_sc, &pi, 1); 
            if (PtInRect(&rc, pi))
            {
                return true;
            }
        }
    }
    return false;
}

static void
on_tabpage_destroy_tabbar(const HWND hwnd)
{
    if (hwnd && hwnd == (HWND)eu_get_config()->eu_tab.hmain)
    {
        HIMAGELIST himg = TabCtrl_GetImageList(hwnd);
        if (himg)
        {
            ImageList_Destroy(himg);
        }
        if (g_drag_hcursor)
        {
            DestroyCursor(g_drag_hcursor);
            g_drag_hcursor = NULL;
        }
        eu_get_config()->eu_tab.hmain = 0;
        eu_logmsg("tabbar WM_DESTROY\n");
    }
}

void
on_tabpage_slave_show(const bool hide)
{
    if (hide)
    {
        eu_get_config()->eu_tab.slave_show &= 0x0;
    }
    else
    {
        eu_get_config()->eu_tab.slave_show |= 0x1;
    }
    eu_window_resize();
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

static void
on_tabpage_reset_status(const HWND hwnd, const int index)
{
    if (HSLAVE_SHOW)
    {
        long state = 0;
        int i = 0, count = 0;
        eu_tabpage *p = NULL;
        eu_tabpage *pmain = NULL;
        TCHAR path[MAX_BUFFER] = {0};
        HWND other = NULL;
        HWND htab[2] = {HMAIN_GET, HSLAVE_SHOW ? HSLAVE_GET : NULL};
        for (int k = 0; k < 2 && htab[k]; ++k)
        {
            for (i = 0, count = TabCtrl_GetItemCount(htab[k]); i < count; ++i)
            {
                if ((p = on_tabpage_get_ptr(htab[k], i)))
                {
                    if (htab[k] == hwnd && i == index)
                    {
                        other = !k ? htab[1] : htab[0];
                        pmain = p;
                        _sntprintf(path, MAX_BUFFER, _T("%s"), p->pathfile);
                        state = ((p->stat_id | TABS_FOUCED) & ~TABS_DUPED);
                        _InterlockedExchange(&p->stat_id, state);
                    }
                    else
                    {
                        state = p->stat_id & ~TABS_FOUCED;
                        _InterlockedExchange(&p->stat_id, state);
                    }
                }
            }
        }
        if (other && pmain && path[0])
        {
            bool dup = false;
            for (i = 0, count = TabCtrl_GetItemCount(other); i < count; ++i)
            {
                if ((p = on_tabpage_get_ptr(other, i)) && _tcscmp(p->pathfile, path) == 0)
                {
                    state = p->stat_id | TABS_DUPED;
                    _InterlockedExchange(&p->stat_id, state);
                    dup = true;
                    break;
                }
            }
            if (dup)
            {
                state = (pmain->stat_id | TABS_MAIN);
                _InterlockedExchange(&pmain->stat_id, state);
            }
            else
            {
                state = (pmain->stat_id & ~TABS_MAIN);
                _InterlockedExchange(&pmain->stat_id, state);
            }
        }
    }
}

static void
on_tabpage_set_active(const HWND htab, const int index)
{
    if (htab && index >= 0)
    {
        TabCtrl_DeselectAll(htab, true);
        TabCtrl_SetCurSel(htab, index);
        TabCtrl_SetCurFocus(htab, index);
        on_tabpage_reset_status(htab, index);
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
on_tabpage_changing(const HWND htab, const int index)
{
    eu_tabpage *p = NULL;
    if((p = on_tabpage_get_ptr(htab, index)) != NULL)
    {
        util_set_title(p);
        on_tabpage_reset_status(htab, index);
        on_toolbar_update_button(p);
        if (!tab_mutil_select)
        {
            on_proc_tab_click(p);
        }
        if (p->pmod)
        {
            on_tabpage_setpos(p);
        }
    }
}

static void
on_tabpage_get_padding(const HWND hwnd, const int index, RECT *prc)
{
    if (hwnd && prc)
    {
        TabCtrl_GetItemRect(hwnd, index, prc);
        if (index > 0)
        {
            int diff = 0;
            RECT pre_rc = {0};
            RECT pre_zero = {0};
            const bool split_hide = !eu_get_config()->m_tab_split;
            split_hide ? TabCtrl_GetItemRect(hwnd, 0, &pre_zero) : (void)0;
            TabCtrl_GetItemRect(hwnd, index - 1, &pre_rc);
            if ((pre_rc.bottom == prc->bottom))
            {
                if ((diff = prc->left - pre_rc.right) > 0)
                {
                    prc->left -= diff;
                }
                if (split_hide && (diff = prc->top - pre_zero.top) > 0)
                {
                    prc->top = pre_rc.top - 3;
                }
            }
            else if (split_hide && (diff = prc->top - pre_rc.bottom) > 0)
            {
                prc->top -= diff;
            }
        }
    }
}

static void
on_tabpage_deselect(const HWND htab, const int index)
{
    if (htab && index >= 0)
    {
        RECT rc;
        TCITEM tci = {TCIF_STATE, 0, TCIS_BUTTONPRESSED};
        SendMessage(htab, TCM_SETITEM, index, (LPARAM)&tci);
        on_tabpage_get_padding(htab, index, &rc);
        InvalidateRect(htab, &rc, true);
    }
}

static bool
on_tabpage_nfocus(const HWND htab, int index)
{
    if (htab && index >= 0)
    {
        TCITEM tci = {TCIF_STATE};
        tci.dwStateMask = TCIS_BUTTONPRESSED;
        if (SendMessage(htab, TCM_GETITEM, index, (LPARAM)&tci))
        {
            return ((tci.dwState & TCIS_BUTTONPRESSED) || (TabCtrl_GetCurSel(htab) == index));
        }
    }
    return false;
}

static inline int
on_tabpage_close_size(void)
{
    return (util_under_wine() ? (CLOSEBUTTON_DEFAULT + TAB_MIN_RIGHT) : eu_dpi_scale_xy(0, CLOSEBUTTON_DEFAULT + TAB_MIN_RIGHT));
}

void
on_tabpage_variable_reset(void)
{
    tab_move_from = -1;
    tab_drag = false;
    tab_mutil_select = false;
    tab_do_drag = false;
    g_point.x = 0, g_point.y = 0;
}

int
on_tabpage_sel_number(const HWND hwnd, int **pvec, const bool ascending)
{
    int num = 0;
    HWND htab = (HWND)hwnd;
    if ((htab || (htab = on_tabpage_hwnd(on_tabpage_focused()))))
    {
        int count = TabCtrl_GetItemCount(htab);
        int index = ascending ? 0 : count - 1;
        for (; ascending ? index < count : index >= 0; ascending ? ++index : --index)
        {
            if (on_tabpage_nfocus(htab, index))
            {
                if (pvec)
                {
                    cvector_push_back(*pvec, index);
                }
                ++num;
            }
        }
    }
    return num;
}

int
on_tabpage_sel_path(wchar_t ***pvec, bool *hex)
{
    int num = 0;
    eu_tabpage *p = NULL;
    HWND htab[2] = {HMAIN_GET, HSLAVE_SHOW ? HSLAVE_GET : NULL};
    for (int k = 0; k < 2 && htab[k]; ++k)
    {
        for (int i = 0, count = TabCtrl_GetItemCount(htab[k]); i < count; ++i)
        {
            if (on_tabpage_nfocus(htab[k], i) && (p = on_tabpage_get_ptr(htab[k], i)) && !p->is_blank && !url_has_remote(p->pathfile))
            {
                if (pvec)
                {
                    cvector_push_back(*pvec, _wcsdup(p->pathfile));
                }
                if (hex && TAB_HEX_MODE(p))
                {
                    *hex = p->hex_mode == TYPES_HEX;
                }
                ++num;
            }
        }
    }
    return num;
}

void
on_tabpage_active_one(const HWND htab, const int index)
{
    if (htab && index >= 0)
    {
        cvector_vector_type(int) v = NULL;
        int num = on_tabpage_sel_number(htab, &v, false);
        if (num > 1 && eu_cvector_at(v, index) >= 0)
        {
            tab_mutil_select = true;
            on_tabpage_changing(htab, index);
        }
        else if (!on_tabpage_nfocus(htab, index))
        {
            cvector_for_each_v2(v, on_tabpage_deselect, htab);
            TabCtrl_SetCurSel(htab, index);
            TabCtrl_SetCurFocus(htab, index);
            on_tabpage_changing(htab, index);
        }
        else 
        {
            eu_tabpage *p = on_tabpage_focus_at(htab);
            if (p)
            {
                util_set_title(p);
                on_tabpage_reset_status(htab, index);
                on_statusbar_update(p);
                on_toolbar_update_button(p);
                SetFocus(p->hwnd_sc);
                if (p->pmod)
                {
                    on_tabpage_setpos(p);
                }
            }
        }
        cvector_freep(&v);
    }
}

void
on_tabpage_active_tab(eu_tabpage *p)
{
    const HWND htab = on_tabpage_hwnd(p);
    const int index = on_tabpage_get_index(p);
    if (p && htab && index >= 0)
    {
        cvector_vector_type(int) v = NULL;
        int num = on_tabpage_sel_number(htab, &v, false);
        if (num > 1 && eu_cvector_at(v, index) >= 0)
        {
            tab_mutil_select = true;
            on_tabpage_changing(htab, index);
        }
        else if (!on_tabpage_nfocus(htab, index))
        {
            cvector_for_each_v2(v, on_tabpage_deselect, htab);
            TabCtrl_SetCurSel(htab, index);
            TabCtrl_SetCurFocus(htab, index);
            on_tabpage_changing(htab, index);
        }
        else 
        {
            util_set_title(p);
            on_tabpage_reset_status(htab, index);
            on_statusbar_update(p);
            on_toolbar_update_button(p);
            SetFocus(p->hwnd_sc);
            if (p->pmod)
            {
                on_tabpage_setpos(p);
            }
        }
        cvector_freep(&v);
    }
}

static void
on_tabpage_draw_sign(const HDC hdc, const LPRECT lprect)
{
    const TCHAR *text  = util_os_version() < 603 || util_under_wine() ? _T("x") : _T("✕");
    HGDIOBJ oldj = SelectObject(hdc, on_theme_font_hwnd());
    RECT rc = {lprect->right - on_tabpage_close_size(),
               lprect->top,
               lprect->right,
               lprect->bottom
              };
    DrawText(hdc, text, (int)_tcslen(text), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    SelectObject(hdc, oldj);
}

static void
on_tabpage_draw_close(HWND hwnd, const LPRECT lprect, bool sel)
{
    HDC hdc = GetDC(hwnd);
    const bool dark = on_dark_enable();
    colour cr = 0;
    if (sel)
    {
        cr = !dark ? eu_get_theme()->item.activetab.bgcolor : eu_get_theme()->item.activetab.color;
        SetBkColor(hdc, cr);
    }
    else
    {
        cr = set_tabface_color(hdc, dark);
    }
    on_tabpage_draw_sign(hdc, lprect);
    ReleaseDC(hwnd, hdc);
}

static void
on_tabpage_flush_close(HWND hwnd, const LPRECT lprect)
{
    RECT rc = {lprect->right - on_tabpage_close_size(),
               lprect->top,
               lprect->right,
               lprect->bottom
              };
    InvalidateRect(hwnd, &rc, true);
}

static bool
on_tabpage_hit_button(const LPRECT lprect, const LPPOINT pt)
{
    RECT rc = {lprect->right - on_tabpage_close_size(),
               lprect->top,
               lprect->right,
               lprect->bottom
              };
    return PtInRect(&rc, *pt);
}

static int
on_tabpage_hit_index(const HWND htab, const LPPOINT pt)
{
    TCHITTESTINFO hit_info = {{pt->x, pt->y},};
    return (int)(SendMessage(htab, TCM_HITTEST, 0, (LPARAM)(&hit_info)));
}

static void
on_tabpage_paint_draw(HWND hwnd, HDC hdc)
{
    const bool dark_mode = on_dark_enable();
    HGDIOBJ old_font = SelectObject(hdc, on_theme_font_hwnd());
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
            on_tabpage_get_padding(hwnd, index, &rc);
            FrameRect(hdc, &rc, dark_mode ? GetSysColorBrush(COLOR_3DDKSHADOW) : GetSysColorBrush(COLOR_BTNSHADOW));
            if (on_tabpage_nfocus(hwnd, index))
            {   // 从主题获取COLOR_HIGHLIGHT值
                cr = !dark_mode ? eu_get_theme()->item.activetab.bgcolor : eu_get_theme()->item.activetab.color;
                SetBkColor(hdc, cr);
                ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
            }
            else
            {
                cr = set_tabface_color(hdc, dark_mode);
            }
            if (p->file_attr & FILE_ATTRIBUTE_READONLY)
            {
                ImageList_Draw(himg, 1, hdc, rc.left + 6, rc.top + (rc.bottom - rc.top - CY_ICON)/2, ILD_TRANSPARENT);
            }
            else if (p->be_modify || p->fn_modify)
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
                if (on_tabpage_hit_index(hwnd, &pt) == index)
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
on_tabpage_exchange_item(const HWND htab, const int old_index, const int new_index, const bool active)
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
    TabCtrl_GetItem(htab, old_index, &drag_item);
    if (old_index == new_index || old_index < 0 || new_index < 0)
    {
        return;
    }
    if (old_index > new_index)
    {
        for (i = old_index; i > new_index; --i)
        {
            SendMessage(htab, TCM_GETITEM, i - 1, (LPARAM)(&shift_item));
            SendMessage(htab, TCM_SETITEM, i, (LPARAM)(&shift_item));
        }
    }
    else
    {
        for (i = old_index; i < new_index; ++i)
        {
            SendMessage(htab, TCM_GETITEM, i + 1, (LPARAM)(&shift_item));
            SendMessage(htab, TCM_SETITEM, i, (LPARAM)(&shift_item));
        }
    }
    SendMessage(htab, TCM_SETITEM, new_index, (LPARAM)(&drag_item));
    if (active)
    {
        on_tabpage_select_index(htab, new_index);
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

static bool
on_tabpage_can_copy(const HWND htab, const HWND other, int **ivec, const bool copy)
{
    bool ret = false;
    cvector_vector_type(wchar_t *) pvec = NULL;
    if (htab)
    {
        int i = 0, count = 0;
        eu_tabpage *p = NULL;
        for (count = TabCtrl_GetItemCount(htab); i < count; ++i)
        {
            if (on_tabpage_nfocus(htab, i) && (p = on_tabpage_get_ptr(htab, i)) && !p->is_blank && (copy ? TAB_HAS_TXT(p) : true))
            {
                cvector_push_back(pvec, _wcsdup(p->pathfile));
                cvector_push_back(*ivec, i);
            }
        }
        for (i = 0, count = TabCtrl_GetItemCount(other); i < count; ++i)
        {
            if ((p = on_tabpage_get_ptr(other, i)))
            {
                for (size_t k = 0; k < eu_int_cast(cvector_size(pvec)); ++k)
                {
                    if (_tcscmp(p->pathfile, pvec[k]) == 0)
                    {
                        free(pvec[k]);
                        cvector_erase(pvec, k);
                        cvector_erase(*ivec, k);
                    }
                }
            }
        }
    }
    if ((ret = cvector_size(pvec) > 0))
    {
        cvector_free_each_and_free(pvec, free);
    }
    else
    {
        cvector_free(pvec);
    }
    return ret;
}

static void
on_tabpage_send_file(const HWND htab, const HWND hwin, const int index)
{
    eu_tabpage *p = on_tabpage_get_ptr(htab, index);
    if (p && (!p->is_blank  || TAB_NOT_NUL(p)))
    {
        int err = SKYLARK_NOT_OPENED;
        file_backup bak = {0};
        bak.view = p->view;
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
                err = on_file_close(&p, FILE_ONLY_CLOSE);
            }
        }
        else
        {
            if (!(err = on_file_close(&p, FILE_REMOTE_CLOSE)))
            {
                const char *sql = "SELECT * FROM skylark_session;";
                err = on_sql_post(sql, on_tabpage_parser_bakup, &bak);
                if (err != SKYLARK_OK && err != SQLITE_ABORT)
                {
                    eu_logmsg("%s: on_sql_post return false, cause: %d\n", __FUNCTION__, err);
                }
                else
                {
                    err = SKYLARK_OK;
                }
            }
        }
        if (err == SKYLARK_OK)
        {
            COPYDATASTRUCT cpd = {1};
            cpd.lpData = (PVOID) &bak;
            cpd.cbData = (DWORD) sizeof(file_backup);
            SendMessageW(hwin, WM_COPYDATA, 0, (LPARAM) &cpd);
            SwitchToThisWindow(hwin, true);
        }
    }
}

static void
on_tabpage_send_group(const HWND htab, const HWND hwin)
{
    cvector_vector_type(int) v = NULL;
    if (on_tabpage_sel_number(htab, &v, false) > 0 && eu_cvector_at(v, tab_move_from) >= 0)
    {
        for (size_t i = 0; i < cvector_size(v); ++i)
        {
            on_tabpage_send_file(htab, hwin, v[i]);
        }
    }
    cvector_freep(&v);
}

static void
on_tabpage_new_hinst(const HWND htab)
{
    int num = 0;
    int index = -1;
    cvector_vector_type(int) v = NULL;
    if ((num = on_tabpage_sel_number(htab, &v, false)) > 0 && (index = eu_cvector_at(v, tab_move_from)) >= 0)
    {
        uint32_t pid = 0;
        HWND hwin = NULL;
        on_file_out_open(htab, v[0], &pid);
        if (num > 1 && pid > 0)
        {   // 等待新进程窗口初始化
            Sleep(600);
            // 发送其他文件到新进程
            if ((hwin = util_get_hwnd(pid)) != NULL)
            {
                for (size_t i = 1; i < cvector_size(v); ++i)
                {
                    on_tabpage_send_file(htab, hwin, v[i]);
                }
            }
        }
    }
    cvector_freep(&v);
}

void
on_tabpage_move_tab(const HWND htab, const HWND other)
{
    cvector_vector_type(int) v = NULL;
    eu_tabpage *pnode = on_tabpage_focus_at(htab);
    int index = on_tabpage_get_index(pnode);
    if (pnode && index >= 0 && on_tabpage_can_copy(htab, other, &v, false) && (eu_cvector_at(v, index)) >= 0)
    {
        bool done = false;
        for (size_t i = 0; i < cvector_size(v); ++i)
        {
            eu_tabpage *p = on_tabpage_get_ptr(htab, v[i]);
            const int count = TabCtrl_GetItemCount(other);
            if (!p->is_blank)
            {
                p->tab_id = count > 0 ? count : 0;
                p->view ^=  0x1;
                if (p->tab_id >= 0 && (on_tabpage_insert(p) == SKYLARK_OK) && on_tabpage_delete_item(htab, v[i]))
                {
                    done = true;
                    eu_logmsg("%d moved\n", v[i]);
                }
                if (v[i] < index && done)
                {
                    --index;
                }
            }
        }
        if (done)
        {
            if (other == HSLAVE_GET && !eu_get_config()->eu_tab.slave_show)
            {
                eu_get_config()->eu_tab.slave_show = true;
            }
            on_file_active_condition(htab, index);
            on_tabpage_select_index(htab, index);
        }
    }
    cvector_freep(&v);
}

bool
on_tabpage_clone_tab(const HWND htab)
{
    bool ret = false;
    cvector_vector_type(int) v = NULL;
    eu_tabpage *pnode = on_tabpage_focus_at(htab);
    const int index = on_tabpage_get_index(pnode);
    const HWND other = (htab == HMAIN_GET) ? (HSLAVE_GET) : (HMAIN_GET);
    if (pnode && index >= 0 && on_tabpage_can_copy(htab, other, &v, true) && (eu_cvector_at(v, index)) >= 0)
    {
        sptr_t pdoc = 0;
        for (size_t i = 0; i < cvector_size(v); ++i)
        {
            pnode = on_tabpage_get_ptr(htab, v[i]);
            if (pnode && (pdoc = eu_sci_call(pnode, SCI_GETDOCPOINTER, 0, 0)))
            {
                eu_tabpage *pnew = NULL;
                file_backup bak = {-1, -1};
                bak.postion = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
                bak.zoom = pnode->zoom_level != SELECTION_ZOOM_LEVEEL ? pnode->zoom_level : 0;
                bak.cp = pnode->codepage;
                bak.bakcp = pnode->bakcp;
                bak.focus = (int)(TabCtrl_GetCurSel(htab) == index);
                bak.eol = pnode->eol;
                bak.status = (int)pnode->be_modify;
                bak.view = pnode->view ^ 0x1;
                bak.tab_id = TabCtrl_GetItemCount(other);
                _tcscpy(bak.rel_path, pnode->pathfile);
                _tcscpy(bak.bak_path, pnode->bakpath);
                on_search_page_mark(pnode, bak.mark_id, MAX_BUFFER-1);
                on_search_fold_kept(pnode, bak.fold_id, MAX_BUFFER-1);
                if (other == HSLAVE_GET && !eu_get_config()->eu_tab.slave_show)
                {
                    eu_get_config()->eu_tab.slave_show = true;
                    eu_window_resize();
                }
                if (on_file_node_init(&pnew, &bak) == SKYLARK_OK && on_tabpage_insert(pnew) == SKYLARK_OK)
                {
                    on_file_before_open(pnew);
                    eu_sci_call(pnew, SCI_SETDOCPOINTER, 0, pdoc);
                    if (on_file_after_open(pnew) == SKYLARK_OK)
                    {
                        ret = true;
                    }
                }
            }
        }
    }
    cvector_freep(&v);
    return ret;
}

static void
on_tabpage_drag_mouse(const HWND htab, const HWND hwin)
{
    if (hwin && !eu_get_config()->inter_reserved_1)
    {
        int fn = 0;
        TCHAR name[FILESIZE] = {0};
        HWND parent = GetParent(hwin);
        GetClassName(hwin, name, FILESIZE - 1);
        eu_session_backup(SESSION_BOTH);
        if (parent == eu_hwnd_self() || hwin == eu_hwnd_self())
        {
            if (fn = TABS_CHILD_WIDGETS(name))
            {
                if (_tcscmp(name, WC_TABCONTROL) == 0)
                {   // 拖放在另一个标签上, 移动标签
                    printf("hwin = %p, we move tab\n", hwin);
                    on_tabpage_move_tab(htab, hwin);
                }
                else 
                {   // 拖放在自身标签以及子窗口, 复制标签
                    on_tabpage_clone_tab(htab);
                }
            }
            else
            {   // 拖放在编辑器除tabbar本身界面上, 启动新实例
                printf("parent = %p, name = %ls\n", parent, name);
                on_tabpage_new_hinst(htab);
            }
        }
        else if (!(fn = _tcscmp(name, APP_CLASS)) || TABS_CHILD_WIDGETS(name))
        {
            int code = fn ? (int)SendMessage(parent, WM_SKYLARK_DESC, 0, 0) : (int)SendMessage(hwin, WM_SKYLARK_DESC, 0, 0);
            if (code != eu_int_cast(WM_SKYLARK_DESC))
            {   // 确认不是skylark编辑器, 启动新实例
                on_tabpage_new_hinst(htab);
            }
            else
            {   // 拖放在另一个skylark编辑器上, 发送文件到窗口句柄
                if (!_tcscmp(name, APP_CLASS))
                {
                    printf("on_tabpage_send_group(htab, hwin)\n");
                    on_tabpage_send_group(htab, hwin);
                }
                else
                {
                    printf("on_tabpage_send_group(htab, parent)\n");
                    on_tabpage_send_group(htab, parent);
                }
            }
        }
        else
        {   // 拖放在空白处, 启动新实例
            on_tabpage_new_hinst(htab);
        }
    }
}

static void
on_tabpage_menu_callback(HMENU hpop, void *param)
{
    HWND htab = NULL;
    eu_tabpage *p = (eu_tabpage *)param;
    if ((htab = on_tabpage_hwnd(p)) && hpop)
    {
        int num = on_tabpage_sel_number(htab, NULL, false);
        if (num > 1)
        {
            TCHAR sub_str[MAX_PATH] = {0};
            LOAD_I18N_RESSTR(IDS_TABPAGE_CLOSE_NUM, mstr);
            _sntprintf(sub_str, MAX_PATH - 1, mstr, num);
            ModifyMenu(hpop, 0, MF_BYPOSITION | MF_STRING, IDM_FILE_CLOSE, sub_str);
        }
        util_enable_menu_item(hpop, IDM_TABPAGE_SAVE, on_sci_doc_modified(p) && !eu_sci_call(p, SCI_GETREADONLY, 0, 0));
        util_set_menu_item(hpop, IDM_TABPAGE_LOCKED, eu_get_config()->inter_reserved_1);
        util_enable_menu_item(hpop, IDM_FILE_ADD_FAVORITES, !p->is_blank);
        util_enable_menu_item(hpop, IDM_VIEW_OTHER_COPY, !p->is_blank && TAB_HAS_TXT(p));
        util_enable_menu_item(hpop, IDM_VIEW_OTHER_MOVE, !p->is_blank);
        util_enable_menu_item(hpop, IDM_FILE_WORKSPACE, !p->is_blank);
        util_enable_menu_item(hpop, IDM_FILE_EXPLORER, !p->is_blank);
    }
}

static void
on_tabpage_pressed(const HWND htab, const int index)
{
    if (htab && index >= 0)
    {
        TCITEM tci = {TCIF_STATE, 0, TCIS_BUTTONPRESSED};
        SendMessage(htab, TCM_GETITEM, index, (LPARAM)&tci);
        if (!(tci.dwState & TCIS_BUTTONPRESSED))
        {
            tci.dwState |= TCIS_BUTTONPRESSED;
            SendMessage(htab, TCM_SETITEM, index, (LPARAM)&tci);
        }
    }
}

static void
on_tabpage_set_cursor(const HWND htab, const int index)
{
    if (htab && index >= 0)
    {
        RECT rc;
        POINT point;
        GetCursorPos(&point);
        MapWindowPoints(HWND_DESKTOP, htab, &point, 2);
        on_tabpage_get_padding(htab, index, &rc);
        if (!PtInRect(&rc, point))
        {
            int cx = (rc.right - rc.left)/2;
            int cy = (rc.bottom - rc.top)/2;
            rc.left += cx;
            rc.top += cy;
            MapWindowPoints(htab, HWND_DESKTOP, (POINT*)(&rc), 2);
            SetCursorPos(rc.left, rc.top);
        }
    }
}

static void
on_tabpage_arr_drag(const HWND htab, const int index)
{
    int num = 0;
    cvector_vector_type(int) v = NULL;
    cvector_vector_type(int) out = NULL;
    if ((num = on_tabpage_sel_number(htab, &v, true)) > 1)
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
                on_tabpage_exchange_item(htab, v[i], index - c, false);
            }
        }
        for (i = num + 1, c = 0; i < end; ++i)
        {
            ++c;
            cvector_push_back(out, index + c);
            if (v[i] != index + c)
            {
                on_tabpage_exchange_item(htab, v[i], index + c, false);
            }
        }
        on_tabpage_select_index(htab, index);
        cvector_for_each_v2(out, on_tabpage_pressed, htab);
        on_tabpage_set_cursor(htab, index);
    }
    cvector_freep(&v);
    cvector_freep(&out);
}

static void
on_tabpage_arr_drag2(const HWND htab, const int from, const int dest)
{
    if (htab && from >= 0 && eu_get_config() && !eu_get_config()->inter_reserved_1)
    {
        int num = 0;
        cvector_vector_type(int) v = NULL;
        cvector_vector_type(int) out = NULL;
        if ((num = on_tabpage_sel_number(htab, &v, true)) > 1)
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
                        on_tabpage_exchange_item(htab, v[i], dest + c, false);
                    }
                }
                else
                {
                    for (i = end - 1, c = 0; i >= 0; --i, ++c)
                    {
                        cvector_push_back(out, dest - c);
                        on_tabpage_exchange_item(htab, v[i], dest - c, false);
                    }
                }
                on_tabpage_select_index(htab, dest);
                cvector_for_each_v2(out, on_tabpage_pressed, htab);
                on_tabpage_set_cursor(htab, dest);
            }
        }
        else if (eu_cvector_at(v, dest) < 0)
        {
            on_tabpage_exchange_item(htab, from, dest, true);
        }
        cvector_freep(&v);
        cvector_freep(&out);
    }
}

static void
on_tabpage_adjust_box(const RECT *prc, RECT *ptab1, RECT *ptab2)
{
    HWND hslave = HMAIN_GET;
    if (ptab1)
    {
        RECT rc_main = { 0 };
        RECT rc_treebar = { 0 };
        if (prc == NULL)
        {
            GetClientRect(eu_hwnd_self(), &rc_main);
            prc = &rc_main;
        }
        on_treebar_adjust_box(prc, &rc_treebar);
        if (!eu_get_config()->m_ftree_show)
        {
            ptab1->left = prc->left;
        }
        else
        {
            ptab1->left = rc_treebar.right;
        }
        ptab1->right = prc->right;
        ptab1->top = rc_treebar.top;
        ptab1->bottom = rc_treebar.bottom;
        eu_get_config()->eu_tab.main_size = ptab1->right - ptab1->left;
        if (hslave)
        {
            RECT rc_slave = {0};
            int sec_width = eu_get_config()->eu_tab.slave_size;
            if (!ptab2)
            {
                ptab2 = &rc_slave;
            }
            else 
            {
                memcpy(ptab2, &rc_slave, sizeof(RECT));
            }
            if (eu_get_config()->eu_tab.slave_show)
            {
                ptab2->right = prc->right - SCINTILLA_MARGIN_RIGHT;
                ptab2->top = ptab1->top;
                ptab2->bottom = ptab1->bottom;
                if (sec_width > 0)
                {
                    ptab2->left = ptab2->right - sec_width;
                    ptab1->right = ptab2->left - TABS_SPLIT;
                }
                else
                {
                    ptab1->right /= 2;
                    ptab2->left = ptab1->right + TABS_SPLIT;
                    eu_get_config()->eu_tab.slave_size = ptab2->right - ptab2->left;
                }
                eu_get_config()->eu_tab.main_size -= eu_get_config()->eu_tab.slave_size + TABS_SPLIT;
            }
        }
    }
    else if (eu_get_config()->eu_tab.hmain)
    {
        util_tab_height((HWND)eu_get_config()->eu_tab.hmain, TAB_MIN_WIDTH);
        if (eu_get_config()->eu_tab.slave_show)
        {
            util_tab_height(hslave, TAB_MIN_WIDTH);    
        }
    }
}

static LRESULT CALLBACK
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
        case WM_THEMECHANGED:
        {
            on_dark_tips_theme(hwnd, TCM_GETTOOLTIPS);
            break;
        }
        case WM_DPICHANGED:
        {
            on_tabpage_adjust_box(NULL, NULL, NULL);
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
                g_drag_hcursor ? SetCursor(g_drag_hcursor) : (void *)0;
                break;
            }
            for (index = 0, count = TabCtrl_GetItemCount(hwnd); index < count; ++index)
            {
                if (!(p = on_tabpage_get_ptr(hwnd, index)))
                {
                    break;
                }
                on_tabpage_get_padding(hwnd, index, &rect);
                if (PtInRect(&rect, point))
                {
                    if (!tab_do_drag && tab_drag)
                    {
                        tab_do_drag = true;
                        on_tabpage_arr_drag(hwnd, index);
                        break;
                    }
                    if (tab_move_from >= 0 && on_tabpage_has_drag(&point) && KEY_DOWN(VK_LBUTTON))
                    {
                        tab_drag = true;
                    }
                    else if (eu_get_config()->m_close_draw == IDM_TABCLOSE_FOLLOW && !tab_drag && !p->at_close)
                    {
                        on_tabpage_draw_close(hwnd, &rect, on_tabpage_nfocus(hwnd, index));
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
            if ((tab_move_from = on_tabpage_hit_index(hwnd, &g_point)) != -1)
            {
                RECT rc;
                on_tabpage_get_padding(hwnd, tab_move_from, &rc);
                if (eu_get_config()->m_close_draw != IDM_TABCLOSE_NONE && on_tabpage_hit_button(&rc, &g_point))
                {
                    on_tabpage_variable_reset();
                    return (LRESULT)PostMessage(hwnd, WM_MBUTTONUP, 0, lParam);
                }
                if (KEY_UP(VK_CONTROL))
                {
                    on_tabpage_active_one(hwnd, tab_move_from);
                    break;
                }
                if (on_tabpage_nfocus(hwnd, tab_move_from))
                {    // 取消多选
                    if (TabCtrl_GetCurSel(hwnd) != tab_move_from)
                    {
                        on_tabpage_deselect(hwnd, tab_move_from);
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
                if (!PtInRect(&rect, point))
                {
                    if (!eu_get_config()->inter_reserved_1)
                    {   // inter_reserved_1 = 是否锁定标签
                        HWND other = (hwnd == HMAIN_GET ? HSLAVE_GET : HMAIN_GET);
                        // Get cursor position of "Screen"    
                        GetCursorPos(&point);
                        if (other && on_tabpage_item_hwnd(other, &point))
                        {
                            on_tabpage_drag_mouse(hwnd, other);
                        }
                        else
                        {
                            on_tabpage_drag_mouse(hwnd, WindowFromPoint(point));
                        }
                    }
                }
                else if ((index = on_tabpage_hit_index(hwnd, &point)) != -1)
                {
                    if (tab_drag && index != tab_move_from)
                    {
                        on_tabpage_arr_drag2(hwnd, tab_move_from, index);
                    }
                    else if (tab_mutil_select && !tab_drag)
                    {
                        tab_mutil_select = false;
                        on_tabpage_select_index(hwnd, index);
                    }
                }
            }
            on_tabpage_variable_reset();
            break;
        }
        case WM_RBUTTONDOWN:
        {
            POINT point = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            if ((index = on_tabpage_hit_index(hwnd, &point)) != -1)
            {
                on_tabpage_active_one(hwnd, index);
            }
            return 1;
        }
        case WM_RBUTTONUP:
        {
            if (eu_get_config()->m_close_way == IDM_VIEW_TAB_RIGHT_CLICK && KEY_UP(VK_SHIFT))
            {
                return (LRESULT)PostMessage(hwnd, WM_MBUTTONUP, 0, lParam);
            }
            if (eu_get_config()->m_new_way == IDM_VIEW_TAB_RIGHT_NEW && KEY_UP(VK_SHIFT))
            {
                break;
            }
            if ((p = on_tabpage_focus_at(hwnd)))
            {
                return menu_pop_track(eu_hwnd_self(), IDR_TABPAGE_POPUPMENU, 0, 0, on_tabpage_menu_callback, p);
            }
            break;
        }
        case WM_MBUTTONUP:
        {
            POINT point = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            if ((index = on_tabpage_hit_index(hwnd, &point)) != -1 && (p = on_tabpage_get_ptr(hwnd, index)) != NULL)
            {
                on_file_close(&p, FILE_ONLY_CLOSE);
                return 1;
            }
            break;
        }
        case WM_TAB_NCCLICK:
        {
            if (eu_get_config()->m_new_way == IDM_VIEW_TAB_RIGHT_NEW && wParam == MK_RBUTTON)
            {
                on_file_new(hwnd, NULL);
            }
            if (eu_get_config()->m_new_way == IDM_VIEW_TAB_DBCLICK_NEW && wParam == MK_LBUTTON)
            {
                on_file_new(hwnd, NULL);
            }
            return 1;
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
                for (index = 0, count = TabCtrl_GetItemCount(hwnd); index < count; ++index)
                {
                    if (!(p = on_tabpage_get_ptr(hwnd, index)))
                    {
                        break;
                    }
                    on_tabpage_get_padding(hwnd, index, &rect);
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
            on_tabpage_destroy_tabbar(hwnd);
            break;
        }
        default:
            break;
    }
    return CallWindowProc((WNDPROC)old_tabproc, hwnd, message, wParam, lParam);
}

static int
on_tabpage_create(const HWND hwnd, const intptr_t resid)
{
    int err = 0;
    const uint32_t flags = WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TCS_TOOLTIPS |
          TCS_BUTTONS | TCS_MULTISELECT | TCS_MULTILINE | TCS_OWNERDRAWFIXED | TCS_FOCUSNEVER;
    HWND htab = CreateWindow(WC_TABCONTROL, NULL, flags, 0, 0, 0, 0, hwnd, (HMENU)resid, eu_module_handle(), NULL);
    do
    {
        if (htab == NULL)
        {
            MSG_BOX(IDC_MSG_TABCONTROL_FAIL, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
            err = 1;
            break;
        }
        if (!init_icon_img_list(htab))
        {
            eu_logmsg("%s: init_icon_img_list return false\n", __FUNCTION__);
            err = 1;
            break;
        }
        if (!g_drag_hcursor && (g_drag_hcursor = LoadCursor(eu_module_handle(), MAKEINTRESOURCE(IDC_CURSOR_DRAG))) == NULL)
        {
            eu_logmsg("%s: LoadCursor return false\n", __FUNCTION__);
            err = 1;
            break;
        }
        if (!err)
        {
            TabCtrl_SetPadding(htab, TAB_MIN_LEFT, 0);
            TabCtrl_SetMinTabWidth(htab, TAB_MIN_WIDTH);
            on_theme_update_font(tabbar_id);
            util_tab_height(htab, TAB_MIN_WIDTH);
        }
        if (inter_atom_compare_exchange(&old_tabproc, SetWindowLongPtr(htab, GWLP_WNDPROC, (LONG_PTR)on_tabpage_proc_callback), 0))
        {
            SetWindowLongPtr(htab, GWLP_WNDPROC, (LONG_PTR)on_tabpage_proc_callback);
        }
        if (!old_tabproc)
        {
            err = 1;
            break;
        }
    } while(0);
    if (err)
    {
        htab ? DestroyWindow(htab) : (void)0;
    }
    else
    {
        if (inter_atom_compare_exchange(&eu_get_config()->eu_tab.hmain, (sptr_t)htab, 0))
        {
            inter_atom_compare_exchange(&eu_get_config()->eu_tab.hslave, (sptr_t)htab, 0);
        }
    }
    return err;
}

int
on_tabpage_create_dlg(const HWND hwnd)
{
    int err = SKYLARK_TABCTRL_ERR;
    if ((err = on_tabpage_create(hwnd, IDM_TABPAGE_BAR1)) == SKYLARK_OK)
    {
        err = on_tabpage_create(hwnd, IDM_TABPAGE_BAR2);
    }
    return err;
}

void
on_tabpage_close_tabs(const HWND htab, const int index)
{
    eu_tabpage *p = on_tabpage_get_ptr(htab, index);
    if (p)
    {
        on_file_close(&p, FILE_ONLY_CLOSE);
    }
}

bool
on_tabpage_delete_item(const HWND htab, const int index)
{
    if (htab && index >= 0)
    {
        return (bool)TabCtrl_DeleteItem(htab, index);
    }
    return false;
}

void
on_tabpage_save_files(const HWND htab, const int index)
{
    on_file_save(on_tabpage_get_ptr(htab, index), false);
}

void
on_tabpage_push_editor(const HWND htab, const int index)
{
    eu_tabpage *p = on_tabpage_get_ptr(htab, index);
    if (p && !p->is_blank && *p->pathfile)
    {
        on_edit_push_editor(p);
    }
}

void
on_tabpage_do_file(tab_callback func, eu_tabpage *p)
{
    const HWND htab = on_tabpage_hwnd(p);
    if (htab)
    {
        cvector_vector_type(int) v = NULL;
        int num = on_tabpage_sel_number(htab, &v, false);
        if (num > 0)
        {
            cvector_for_each_v2(v, func, htab);
        }
        cvector_freep(&v);
    }
}

void
on_tabpage_size(const RECT *prc)
{
    RECT rctab1;
    RECT rctab2 = {0};
    HWND hmain = HMAIN_GET;
    HWND hslave = HSLAVE_GET;
    on_tabpage_adjust_box(prc, &rctab1, &rctab2);
    if (hmain)
    {
        MoveWindow(hmain, rctab1.left, rctab1.top, rctab1.right - rctab1.left, rctab1.bottom - rctab1.top, TRUE);
        ShowWindow(hmain, SW_SHOW);
    }
    if (hslave && eu_get_config()->eu_tab.slave_show)
    {
        eu_setpos_window(g_splitter_tabbar, HWND_TOP, rctab1.right, rctab1.top, TABS_SPLIT, rctab1.bottom - rctab1.top, SWP_SHOWWINDOW);
        MoveWindow(hslave, rctab2.left, rctab2.top, rctab2.right - rctab2.left, rctab2.bottom - rctab2.top, TRUE);
        ShowWindow(hslave, SW_SHOW);
    }
}

void
on_tabpage_adjust_window(const RECT *prc, eu_tabpage *p1, eu_tabpage *p2, RECT *ptab1, RECT *ptab2)
{
    RECT rc_main;
    RECT rc_tab1 = {0};
    RECT rc_tab2 = {0};
    int rect_bottom = 0;
    if (!ptab1)
    {
        ptab1 = &rc_tab1;
    }
    if (!ptab2)
    {
        ptab2 = &rc_tab2;
    }
    if (!prc)
    {
        GetClientRect(eu_hwnd_self(), &rc_main);
        prc = &rc_main;    
    }
    if (prc)
    {
        on_tabpage_adjust_box(prc, ptab1, ptab2);
    }
    if (p1)
    {
        eu_tabpage *p = (p2 && p2->map_show) ? p2 : NULL;
        if ((p1->map_show || p) && on_map_hwnd())
        {
            const bool slave = p2 && HSLAVE_SHOW;
            int *pright = !slave ? &ptab1->right : &ptab2->right;
            if (!p)
            {
                p = p1;
            }
            *pright -= eu_get_config()->document_map_width + SPLIT_WIDTH;
            p->rect_map.left = *pright;
            p->rect_map.right = prc->right - SPLIT_WIDTH;
            p->rect_map.top = !slave ? ptab1->top : ptab2->top;
            p->rect_map.bottom = !slave ? ptab1->bottom : ptab2->bottom;
        }
        if (p1->hwnd_sc)
        {
            p1->rect_sc.left = ptab1->left;
            if (eu_get_config()->m_ftree_show)
            {
                p1->rect_sc.left += SPLIT_WIDTH;
            }
            p1->rect_sc.right = ptab1->right - SPLIT_WIDTH;
            p1->rect_sc.top = ptab1->top + on_tabpage_get_height(0);
            p1->rect_sc.bottom = ptab1->bottom;
        }
        if (p1->sym_show)
        {
            if (p1->hwnd_symlist || p1->hwnd_symtree)
            {
                p1->rect_sc.right -= (p1->hwnd_symlist ? eu_get_config()->sym_list_width : eu_get_config()->sym_tree_width)
                                      + SPLIT_WIDTH;
                p1->rect_sym.left = p1->rect_sc.right + SPLIT_WIDTH;
                p1->rect_sym.right = ptab1->right - SPLIT_WIDTH;
                p1->rect_sym.top = p1->rect_sc.top;
                p1->rect_sym.bottom = p1->rect_sc.bottom;
            }
        }
        if (RESULT_SHOW(p1))
        {
            rect_bottom = p1->rect_sc.bottom;
            p1->rect_sc.bottom -= SPLIT_WIDTH + eu_get_config()->result_edit_height + (QRTABLE_SHOW(p1) ? eu_get_config()->result_list_height : 0);
            p1->rect_result.left = p1->rect_sc.left;
            p1->rect_result.right = p1->rect_sc.right;
            p1->rect_result.top = p1->rect_sc.bottom + SPLIT_WIDTH;
            p1->rect_result.bottom = rect_bottom;
            if (QRTABLE_SHOW(p1))
            {
                p1->rect_result.bottom -= SPLIT_WIDTH + eu_get_config()->result_list_height;
                p1->rect_qrtable.left = p1->rect_sc.left;
                p1->rect_qrtable.right = p1->rect_sc.right;
                p1->rect_qrtable.top = p1->rect_result.bottom + SPLIT_WIDTH;
                p1->rect_qrtable.bottom = rect_bottom;
            }
        }
    }
    if (p2)
    {
        if (p2->hwnd_sc)
        {
            p2->rect_sc.left = ptab2->left;
            p2->rect_sc.right = ptab2->right - SPLIT_WIDTH;
            p2->rect_sc.top = ptab2->top + on_tabpage_get_height(1);
            p2->rect_sc.bottom = ptab2->bottom;
        }
        if (p2->sym_show)
        {
            if (p2->hwnd_symlist || p2->hwnd_symtree)
            {
                p2->rect_sc.right -= (p2->hwnd_symlist ? eu_get_config()->sym_list_width : eu_get_config()->sym_tree_width)
                                      + SPLIT_WIDTH;
                p2->rect_sym.left = p2->rect_sc.right + SPLIT_WIDTH;
                p2->rect_sym.right = ptab2->right - SPLIT_WIDTH;
                p2->rect_sym.top = p2->rect_sc.top;
                p2->rect_sym.bottom = p2->rect_sc.bottom;
            }
        }
        if (RESULT_SHOW(p2))
        {
            rect_bottom = p2->rect_sc.bottom;
            p2->rect_sc.bottom -= SPLIT_WIDTH + eu_get_config()->result_edit_height + (QRTABLE_SHOW(p2) ? eu_get_config()->result_list_height : 0);
            p2->rect_result.left = p2->rect_sc.left;
            p2->rect_result.right = p2->rect_sc.right;
            p2->rect_result.top = p2->rect_sc.bottom + SPLIT_WIDTH;
            p2->rect_result.bottom = rect_bottom;
            if (QRTABLE_SHOW(p2))
            {
                p2->rect_result.bottom -= SPLIT_WIDTH + eu_get_config()->result_list_height;
                p2->rect_qrtable.left = p2->rect_sc.left;
                p2->rect_qrtable.right = p2->rect_sc.right;
                p2->rect_qrtable.top = p2->rect_result.bottom + SPLIT_WIDTH;
                p2->rect_qrtable.bottom = rect_bottom;
            }
        }
    }
}

eu_tabpage *
on_tabpage_remove(const eu_tabpage *pnode, const CLOSE_MODE mode)
{
    HWND htab = on_tabpage_hwnd(pnode);
    if (htab)
    {
        for (int index = 0, count = TabCtrl_GetItemCount(htab); index < count; ++index)
        {
            eu_tabpage *p = NULL;
            if ((p = on_tabpage_get_ptr(htab, index)) && p == pnode)
            {   /* 从控件删除选项卡 */
                p->tab_id = index;
                TabCtrl_DeleteItem(htab, index);
                if (htab != HSLAVE_GET && count < 2 && !TAB_HEX_MODE(pnode) && !pnode->plugin)
                {   // 不是从视图, 未设置最后一个标签退出, 也不是关闭编辑器, 则保留最后一个空标签
                    if (file_click_close(mode) && (!eu_get_config()->m_exit || !on_tabpage_other_empty(htab)))
                    {
                        p->reason = TABS_MAYBE_RESERVE;
                    }
                }
                return p;
            }
        }
    }
    return NULL;
}

void
on_tabpage_count_tabs(int *pv0, int *pv1)
{
    HWND hmain = HMAIN_GET;
    if (pv0)
    {
        *pv0 = hmain ? TabCtrl_GetItemCount(hmain) : 0;    
    }
    if (pv1)
    {
        *pv1 = HSLAVE_SHOW ? TabCtrl_GetItemCount(HSLAVE_GET) : 0;
    }
}

void
on_tabpage_count_empty(int *pv0, int *pv1)
{
    int index = 0;
    int v0 = 0, v1 = 0;
    HWND htab = NULL;
    eu_tabpage *p = NULL;
    TCITEM tci = {TCIF_TEXT | TCIF_PARAM};
    on_tabpage_count_tabs(&v0, &v1);
    if (pv0)
    {
        *pv0 = 0;
    }
    if (pv1)
    {
        *pv1 = 0;
    }
    if (pv0 && v0 > 0 && (htab = HMAIN_GET))
    {
        for (; index < v0; ++index)
        {
            if (TabCtrl_GetItem(htab, index, &tci) && (p = (eu_tabpage *) (tci.lParam)))
            {
                if (p->is_blank && !TAB_NOT_NUL(p) && !on_sci_doc_modified(p))
                {
                    ++(*pv0);
                }
            }
        }
    }
    if (pv1 && v1 > 0 && (htab = HSLAVE_GET))
    {
        for (index = 0; index < v1; ++index)
        {
            if (TabCtrl_GetItem(htab, index, &tci) && (p = (eu_tabpage *) (tci.lParam)))
            {
                if (p->is_blank && !TAB_NOT_NUL(p) && !on_sci_doc_modified(p))
                {
                    ++(*pv1);
                }
            }
        }
    }
}

static int
on_tabpage_replace_empty(eu_tabpage *pre)
{
    HWND htab = on_tabpage_hwnd(pre);
    if (htab)
    {
        eu_tabpage *p = NULL;
        for (int index = 0, count = TabCtrl_GetItemCount(htab); index < count; ++index)
        {
            bool repace = false;
            TCITEM tci = {TCIF_TEXT | TCIF_PARAM};
            if (TabCtrl_GetItem(htab, index, &tci) && (p = (eu_tabpage *) (tci.lParam)))
            {
                if (p->is_blank && !TAB_NOT_NUL(p) && !on_sci_doc_modified(p))
                {
                    if ((repace = pre->hwnd_sc == NULL))
                    {
                        pre->hwnd_sc = p->hwnd_sc;
                        pre->eusc = p->eusc;
                    }
                    tci.pszText = pre->filename;
                    tci.lParam = (LPARAM)pre;
                    TabCtrl_SetItem(htab, index, &tci);
                    if (!repace && p->hwnd_sc)
                    {
                        SendMessage(p->hwnd_sc, WM_CLOSE, 0, 0);
                        p->hwnd_sc = NULL;
                    }
                    free(p);
                    return index;
                }
            }
        }
    }
    return SKYLARK_TABCTRL_ERR;
}

bool
on_tabpage_other_empty(const HWND htab)
{
    bool empty = true;
    HWND other = TAB_GET_SUB(htab);
    if (other)
    {
        eu_tabpage *p = NULL;
        for (int index = 0, count = TabCtrl_GetItemCount(other); index < count; ++index)
        {
            if ((p = on_tabpage_get_ptr(other, index)))
            {
                if (!p->is_blank || TAB_NOT_NUL(p) || on_sci_doc_modified(p))
                {
                    empty = false;
                    break;
                }
            }
        }
    }
    return empty;
}

TCHAR *
on_tabpage_generator(HWND htab, TCHAR *filename, const int len)
{
    const TCHAR ch = _T(' ');
    const TCHAR *pstr = NULL;
    LOAD_I18N_RESSTR(IDC_MSG_NEW_FILE, m_file);
    if ((htab || (htab = HMAIN_GET)) && filename && len > 0)
    {
        if ((pstr = _tcsrchr(m_file, ch)) != NULL && (pstr - m_file) > 0)
        {
            int ret = 1;
            int vlen = 0;
            cvector_vector_type(int) v = NULL;
            int count = TabCtrl_GetItemCount(htab);
            _tcsncpy(filename, m_file, pstr - m_file);
            for (int index = 0; index < count; ++index)
            {
                eu_tabpage *p = on_tabpage_get_ptr(htab, index);
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
    }
    return filename;
}

void
on_tabpage_newdoc_reload(void)
{
    LOAD_I18N_RESSTR(IDC_MSG_NEW_FILE, m_file);
    const TCHAR ch = _T(' ');
    const TCHAR *pstr = NULL;
    TCHAR filename[MAX_PATH] = {0};
    if ((pstr = _tcsrchr(m_file, ch)) != NULL && (pstr - m_file) > 0)
    {
        eu_tabpage *p = NULL;
        HWND htab[2] = {HMAIN_GET, HSLAVE_SHOW ? HSLAVE_GET : NULL};
        for (int i = 0; i < 2 && htab[i]; ++ i)
        {
            int count = TabCtrl_GetItemCount(htab[i]);
            _tcsncpy(filename, m_file, pstr - m_file);
            pstr = NULL;
            for (int index = 0; index < count; ++index)
            {
                if ((p = on_tabpage_get_ptr(htab[i], index)) && p->is_blank)
                {
                    TCHAR old[MAX_BUFFER] = {0};
                    // 防止locale切换时出现不同语种的空标签
                    on_sql_delete_backup_row(p);
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
}

int
on_tabpage_insert(eu_tabpage *pnode)
{
    const HWND htab = on_tabpage_hwnd(pnode);
    if (htab)
    {
        int tab_id = pnode->is_blank ? -1 : on_tabpage_replace_empty(pnode);
        if (tab_id < 0)
        {
            TCITEM tci = {TCIF_TEXT | TCIF_PARAM};
            tci.pszText = pnode->filename;
            tci.lParam = (LPARAM) pnode;
            if (TabCtrl_InsertItem(htab, pnode->tab_id < 0 ? 0 : pnode->tab_id, &tci) == -1)
            {
                eu_logmsg("%s: TabCtrl_InsertItem return false\n", __FUNCTION__);
                return EUE_INSERT_TAB_FAIL;
            }
        }
        else
        {
            pnode->tab_id = tab_id;
            eu_logmsg("%s: Replacing empty Tab, pnode->tab_id = %d\n", __FUNCTION__, pnode->tab_id);
        }
        if (!pnode->hwnd_sc && on_sci_init_dlg(pnode))
        {   // 当复用scintilla窗口时, 不重复创建
            TabCtrl_DeleteItem(htab, pnode->tab_id);
            return EUE_INSERT_TAB_FAIL;
        }
        return SKYLARK_OK;
    }
    return EUE_INSERT_TAB_FAIL;
}

int
on_tabpage_reload_file(eu_tabpage *pnode, int flags, sptr_t *pline)
{
    if (!pnode || TAB_HEX_MODE(pnode) || pnode->plugin)
    {
        return 0;
    }
    switch (flags)
    {
        case 0: // 保留
            pnode->fn_modify = true;
            on_sci_point_left(pnode);
            break;
        case 1: // 丢弃
            pnode->be_modify = false;
            pnode->fn_modify = false;
            on_file_close(&pnode, FILE_ONLY_CLOSE);
            break;
        case 2: // 重载, 滚动到末尾行
        {
            if (!url_has_remote(pnode->pathfile))
            {
                on_sci_clear_history(pnode, false);
                eu_sci_call(pnode, SCI_CLEARALL, 0, 0);
                if (on_file_load(pnode, true) == SKYLARK_OK)
                {
                    sptr_t max_line = eu_sci_call(pnode, SCI_GETLINECOUNT, 0, 0);
                    if (pline && *pline > max_line - 1)
                    {
                        *pline = max_line - 1;
                    }
                    on_sci_clear_history(pnode, true);
                    pnode->be_modify = false;
                    pnode->fn_modify = false;
                    if (!pnode->is_blank)
                    {
                        pnode->st_mtime = util_last_time(pnode->pathfile);
                        if (pline)
                        {
                            on_search_jmp_line(pnode, *pline, 0);
                        }
                        else
                        {
                            on_search_jmp_line(pnode, max_line, 0);
                        }
                    }
                    on_toolbar_update_button(pnode);
                    util_redraw(on_tabpage_hwnd(pnode), false);
                }
            }
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
        SendMessage(p->hwnd_sc, WM_THEMECHANGED, 0, 0);
    }
    if (p->sym_show)
    {
        if (p->hwnd_symlist)
        {
            SendMessage(p->hwnd_symlist, WM_THEMECHANGED, 0, 0);
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
on_tabpage_exist_map(void)
{
    eu_tabpage *p = NULL;
    HWND htab[2] = {HMAIN_GET, HSLAVE_SHOW ? HSLAVE_GET : NULL};
    for (int k = 0; k < 2 && htab[k]; ++k)
    {
        for (int i = 0, count = TabCtrl_GetItemCount(htab[k]); i < count; ++i)
        {
            if ((p = on_tabpage_get_ptr(htab[k], i)) && p->map_show)
            {
                return true;
            }
        }
    }
    return false;
}

void
on_tabpage_foreach(tab_ptr fntab)
{
    eu_tabpage *p = NULL;
    HWND htab[2] = {HMAIN_GET, HSLAVE_SHOW ? HSLAVE_GET : NULL};
    for (int k = 0; k < 2 && htab[k]; ++k)
    {
        for (int i = 0, count = TabCtrl_GetItemCount(htab[k]); i < count; ++i)
        {
            if ((p = on_tabpage_get_ptr(htab[k], i)))
            {
                fntab(p);
            }
        }
    }
}

eu_tabpage *
on_tabpage_get_ptr(const HWND htab, const int index)
{
    TCITEM tci = {TCIF_PARAM};
    if (htab && index >= 0 && TabCtrl_GetItem(htab, index, &tci))
    {
        return (eu_tabpage *) (tci.lParam);
    }
    return NULL;
}

eu_tabpage *
on_tabpage_focused(void)
{
    eu_tabpage *p = NULL;
    HWND hmain = HMAIN_GET;
    if (HSLAVE_SHOW)
    {
        HWND htab[2] = {hmain, HSLAVE_GET};
        for (int k = 0; k < 2 && htab[k]; ++k)
        {
            for (int i = 0, count = TabCtrl_GetItemCount(htab[k]); i < count; ++i)
            {
                if ((p = on_tabpage_get_ptr(htab[k], i)) && (p->stat_id & TABS_FOUCED))
                {   // 设置正确的tab_id, 以备用
                    p->tab_id = i;
                    return p;
                }
            }
        }
    }
    else
    {
        p = on_tabpage_get_ptr(hmain, TabCtrl_GetCurSel(hmain));
    }
    return p;
}

eu_tabpage *
on_tabpage_focus_at(const HWND htab)
{
    return htab ? on_tabpage_get_ptr(htab, TabCtrl_GetCurSel(htab)) : NULL;
}

eu_tabpage *
on_tabpage_dup_at(const HWND htab, const TCHAR *path)
{
    if (htab && STR_NOT_NUL(path))
    {
        eu_tabpage *p = NULL;
        const HWND other = TAB_GET_SUB(htab);
        if (other)
        {
            for (int i = 0, count = TabCtrl_GetItemCount(other); i < count; ++i)
            {
                if ((p = on_tabpage_get_ptr(other, i)) && (!p->is_blank) && _tcscmp(p->pathfile, path) == 0)
                {
                    return p;
                }
            }
        }
    }
    return NULL;
}

HWND
on_tabpage_hwnd(const eu_tabpage *pnode)
{
    return (pnode ? (!pnode->view ? HMAIN_GET : HSLAVE_GET) : NULL);
}

HWND
on_tabpage_sci(const HWND htab)
{
    eu_tabpage *p = on_tabpage_focus_at(htab);
    if (p && p->hwnd_sc)
    {
        return p->hwnd_sc;
    }
    return NULL;
}

HWND
on_tabpage_symlist(const HWND htab)
{
    eu_tabpage *p = on_tabpage_focus_at(htab);
    if (p && p->hwnd_symlist)
    {
        return p->hwnd_symlist;
    }
    return NULL;
}

HWND
on_tabpage_symtree(const HWND htab)
{
    eu_tabpage *p = on_tabpage_focus_at(htab);
    if (p && p->hwnd_symtree)
    {
        return p->hwnd_symtree;
    }
    return NULL;
}

HWND
on_tabpage_qrtable(const HWND htab)
{
    eu_tabpage *p = on_tabpage_focus_at(htab);
    if (p && p->hwnd_qrtable)
    {
        return p->hwnd_qrtable;
    }
    return NULL;
}

HWND
on_tabpage_resultctl(const HWND htab)
{
    eu_tabpage *p = on_tabpage_focus_at(htab);
    if (p && p->presult && p->presult->hwnd_sc)
    {
        return p->presult->hwnd_sc;
    }
    return NULL;
}

eu_tabpage *
on_tabpage_select_index(const HWND htab, int index)
{
    eu_tabpage *p = on_tabpage_get_ptr(htab, index);
    if (p)
    {
        on_tabpage_set_active(htab, index);
        on_toolbar_update_button(p);
        util_set_title(p);
        eu_window_resize();
        return p;
    }
    return NULL;
}

int
on_tabpage_selection(const eu_tabpage *pnode)
{
    eu_tabpage *p = NULL;
    const HWND htab = on_tabpage_hwnd(pnode);
    for (int index = 0, count = TabCtrl_GetItemCount(htab); index < count; ++index)
    {
        if ((p = on_tabpage_get_ptr(htab, index)) && p == pnode)
        {
            on_tabpage_set_active(htab, index);
            on_toolbar_update_button(p);
            util_set_title(p);
            eu_window_resize();
            return index;
        }
    }
    return (SKYLARK_TABCTRL_ERR);
}

eu_tabpage *
on_tabpage_from_handle(void *hwnd_sc, tab_hwnd func)
{
    eu_tabpage *p = NULL;
    HWND htab = HMAIN_GET;
    if ((HWND)hwnd_sc != NULL)
    {
        if (func(htab) == (HWND)hwnd_sc)
        {
            p = on_tabpage_focus_at(htab);
        }
        else if (HSLAVE_SHOW && (htab = HSLAVE_GET) && func(htab) == (HWND)hwnd_sc)
        {
            p = on_tabpage_focus_at(htab);
        }
    }
    return p;
}

int
on_tabpage_get_index(const eu_tabpage *pnode)
{
    HWND htab = on_tabpage_hwnd(pnode);
    if (htab)
    {
        eu_tabpage *p = NULL;
        for (int index = 0, count = TabCtrl_GetItemCount(htab); index < count; ++index)
        {
            if ((p = on_tabpage_get_ptr(htab, index)) && p && (p == pnode))
            {
                return index;
            }
        }
    }
    return SKYLARK_TABCTRL_ERR;
}

void
on_tabpage_switch_next(void)
{
    eu_tabpage *p = on_tabpage_focused();
    HWND htab = on_tabpage_hwnd(p);
    if (htab)
    {
        int count = TabCtrl_GetItemCount(htab);
        int index = TabCtrl_GetCurSel(htab);
        if (index >= 0 && count > 1 && index < count)
        {
            index = (index < count - 1 ? index + 1 : 0);
            on_tabpage_active_one(htab, index);
        }
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
