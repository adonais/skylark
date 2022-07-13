/******************************************************************************
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

#define TAB_MIN_TOP 4
#define TAB_MIN_LEFT 40
#define TAB_MIN_WIDTH 140
#define CLOSEBUTTON_WIDTH 11
#define CLOSEBUTTON_HEIGHT 11
#define CX_ICON  16
#define CY_ICON  16

HWND g_tabpages = NULL;

static WNDPROC old_tabproc = NULL;
static bool tab_drag = false;
static volatile int tab_move_from = -1;

int
on_tabpage_get_height(void)
{
    RECT rect_tabbar = {0};
    int tab_height = TABS_HEIGHT_DEFAULT;
    TabCtrl_GetItemRect(g_tabpages, 0, &rect_tabbar);
    tab_height = (rect_tabbar.bottom - rect_tabbar.top) * TabCtrl_GetRowCount(g_tabpages);
    return tab_height;
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
    printf("tabbar WM_DESTROY\n");
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

static void
on_tabpage_draw_close(HWND hwnd, const LPRECT lprect, bool sel)
{
    int nclose = on_dark_supports() ? IDB_DARK_CLOSE_BMP : (sel ? IDB_AC_CLOSE_BMP : IDB_UN_CLOSE_BMP);
    HDC hdc = GetDC(hwnd);
    HDC hdc_mem  = CreateCompatibleDC(hdc);
    HBITMAP hbmp = LoadBitmap(eu_module_handle(), MAKEINTRESOURCE(nclose));
    int border = (lprect->bottom - lprect->top - CLOSEBUTTON_HEIGHT + 1) / 2;
    int left = lprect->right - CLOSEBUTTON_WIDTH - TAB_MIN_TOP;
    int top = lprect->top + border;
    SelectObject(hdc_mem, hbmp);
    StretchBlt(hdc, left, top, CLOSEBUTTON_WIDTH, CLOSEBUTTON_HEIGHT, hdc_mem, 0, 0, CLOSEBUTTON_WIDTH, CLOSEBUTTON_HEIGHT, SRCCOPY);
    DeleteDC(hdc_mem);
    DeleteObject(hbmp);
    ReleaseDC(hwnd, hdc);
}

static void
on_tabpage_undraw_close(HWND hwnd, const LPRECT lprect)
{
    RECT rc = {lprect->right - CLOSEBUTTON_WIDTH - TAB_MIN_TOP,
               lprect->top + TAB_MIN_TOP,
               lprect->right - TAB_MIN_TOP,
               lprect->bottom - 1
              };
    InvalidateRect(hwnd, &rc, true);
    UpdateWindow(hwnd);
}

static bool
on_tabpage_hit_button(const LPRECT lprect, const LPPOINT pt)
{
    RECT rc = {lprect->right - CLOSEBUTTON_WIDTH - TAB_MIN_TOP,
               lprect->top + TAB_MIN_TOP,
               lprect->right - TAB_MIN_TOP,
               lprect->top + TAB_MIN_TOP + CLOSEBUTTON_HEIGHT
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
    if (old_font)
    {
        set_text_color(hdc, dark_mode);
        int nsel = TabCtrl_GetCurSel(hwnd);
        for (int index = 0, count = TabCtrl_GetItemCount(hwnd); index < count; ++index)
        {
            TCITEM tci = {TCIF_PARAM | TCIF_TEXT | TCIF_IMAGE};
            TabCtrl_GetItem(hwnd, index, &tci);
            eu_tabpage *p = (eu_tabpage *) (tci.lParam);
            if (p)
            {
                RECT rc;
                colour cr = 0;
                HIMAGELIST himg = TabCtrl_GetImageList(hwnd);
                TabCtrl_GetItemRect(hwnd, index, &rc);
                FrameRect(hdc, &rc, dark_mode ? GetSysColorBrush(COLOR_3DDKSHADOW) : GetSysColorBrush(COLOR_BTNSHADOW));
                if (nsel == index)
                {   // 这里使用固定值, 因为在某些系统上, COLOR_HIGHLIGHT值不一样
                    cr = dark_mode ? on_dark_light_color(rgb_dark_bk_color, 1.5f) : rgb_high_light_color;
                    SetBkColor(hdc, cr);
                    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
                }
                else
                {
                    cr = set_btnface_color(hdc, dark_mode);
                }
                if (p->be_modify && himg)
                {
                    ImageList_Draw(himg, 0, hdc, rc.left + 6, rc.top + 3, ILD_TRANSPARENT);
                }
                if (STR_NOT_NUL(p->filename))
                {
                    DrawText(hdc, p->filename, (int)_tcslen(p->filename), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
                }
            }
        }
        HGDIOBJ hfont = SelectObject(hdc, old_font);
        if (hfont)
        {
            DeleteObject(hfont);
        }
    }
}

static void
on_tabpage_exchange_item(int old_index, int new_index)
{
    int i = 0;
    TCITEM drag_item, shift_item;
    TCHAR str1[MAX_PATH];
    TCHAR str2[MAX_PATH];
    drag_item.mask = shift_item.mask = TCIF_TEXT | TCIF_PARAM;
    drag_item.cchTextMax = shift_item.cchTextMax = MAX_PATH;
    drag_item.pszText = str1;
    shift_item.pszText = str2;
    TabCtrl_GetItem(g_tabpages, old_index, &drag_item);
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

static int
on_tabpage_parser_bakup(void *data, int count, char **column, char **names)
{
    wchar_t path[MAX_PATH] = {0};
    file_backup *pbak = (file_backup *)data;
    for (int i = 0; i < count; ++i)
    {
        if (STRCMP(names[i], ==, "szTabId"))
        {
            pbak->tab_id = (short)atoi(column[i]);
        }
        else if (STRCMP(names[i], ==, "szRealPath"))
        {
            MultiByteToWideChar(CP_UTF8, 0, column[i], -1, path, MAX_PATH);
        }
        else if (STRCMP(names[i], ==, "szBakPath"))
        {
            MultiByteToWideChar(CP_UTF8, 0, column[i], -1, pbak->bak_path, MAX_PATH);
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
            pbak->postion = _atoi64(column[i]);
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
        return 1;
    }
    return 0;
}

void
on_tabpage_drag_mouse(POINT *pscreen)
{
    int fn = 0;
    NMHDR mn = {0};
    TCHAR name[FILESIZE] = {0};
    HWND hwin = WindowFromPoint(*pscreen);
    HWND parent = GetParent(hwin);
    GetClassName(hwin, name, FILESIZE - 1);
    if (parent == eu_hwnd_self() || hwin == eu_hwnd_self())
    {   // 拖放在skylark编辑器本身界面上, 启动新实例
        on_sci_send_extra((void *)(intptr_t)tab_move_from, TCN_TABDROPPED_OUT, &mn);
    }
    else if (!(fn = _tcscmp(name, APP_CLASS)) || (!_tcscmp(name, TEXT("Scintilla"))) || (!_tcscmp(name, HEX_CLASS)) || (!_tcscmp(name, WC_TABCONTROL)))
    {   // 拖放在另一个skylark编辑器上, 发送文件到窗口句柄
        eu_tabpage *p = on_tabpage_get_ptr(tab_move_from);
        int code = fn ? (int)SendMessage(parent, WM_SKYLARK_DESC, 0, 0) : (int)SendMessage(hwin, WM_SKYLARK_DESC, 0, 0);
        if (code != eu_int_cast(WM_SKYLARK_DESC))
        {
            on_sci_send_extra((void *)(intptr_t)tab_move_from, TCN_TABDROPPED_OUT, &mn);
        }
        else if (p && !p->is_blank)
        {
            file_backup bak = {0};
            if (!p->be_modify && !p->hex_mode)
            {
                sptr_t pos = eu_sci_call(p, SCI_GETCURRENTPOS, 0, 0);
                if (pos > 0)
                {
                    sptr_t lineno = eu_sci_call(p, SCI_LINEFROMPOSITION, pos, 0);
                    sptr_t row = eu_sci_call(p, SCI_POSITIONFROMLINE, lineno, 0);
                    bak.x = lineno + 1;
                    bak.y = eu_int_cast(pos - row + 1);
                }
                _tcscpy(bak.rel_path, p->pathfile);
                _tputenv(_T("OPEN_FROM_SQL="));
                on_file_close(p, FILE_ONLY_CLOSE);
            }
            else
            {
                int err = on_file_close(p, FILE_REMOTE_CLOSE);
                if (!err && !_tputenv(_T("OPEN_FROM_SQL=1")))
                {
                    const char *sql = "SELECT * FROM skylark_session;";
                    _tcscpy(bak.rel_path, p->pathfile);
                    int err = eu_sqlite3_send(sql, on_tabpage_parser_bakup, &bak);
                    if (err != 0)
                    {
                        printf("eu_sqlite3_send failed in %s, cause: %d\n", __FUNCTION__, err);
                    }
                }
            }
            COPYDATASTRUCT cpd = { 0 };
            cpd.lpData = (PVOID) &bak;
            cpd.cbData = (DWORD) sizeof(file_backup);
            if (!_tcscmp(name, APP_CLASS))
            {
                SendMessageW(hwin, WM_COPYDATA, 0, (LPARAM) &cpd);
                SwitchToThisWindow(hwin, true);
            }
            else
            {
                SendMessageW(parent, WM_COPYDATA, 0, (LPARAM) &cpd);
                SwitchToThisWindow(parent, true);
            }
        }
    }
    else
    {   // 拖放在空白处, 启动新实例
        on_sci_send_extra((void *)(intptr_t)tab_move_from, TCN_TABDROPPED_OUT, &mn);
    }
}

static void
on_tabpage_menu_callback(HMENU hpop, void *param)
{
    eu_tabpage *p = (eu_tabpage *)param;
    if (p && hpop)
    {
        util_enable_menu_item(hpop, IDM_TABPAGE_SAVE, on_sci_doc_modified(p));
        util_enable_menu_item(hpop, IDM_EDIT_OTHER_EDITOR, !p->is_blank);
        util_enable_menu_item(hpop, IDM_FILE_WORKSPACE, !p->is_blank);
        util_enable_menu_item(hpop, IDM_FILE_EXPLORER, !p->is_blank);
    }
}

LRESULT CALLBACK
tabs_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int count = 0;
    int index = 0;
    eu_tabpage *p = NULL;
    switch (message)
    {
        case WM_ERASEBKGND:
            RECT rc = {0};
            GetClientRect(hwnd, &rc);
            FillRect((HDC)wParam, &rc, (HBRUSH)on_dark_get_brush());
            return 1;
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
            UpdateWindowEx(hwnd);
            PostMessage(eu_module_hwnd(), WM_SIZE, 0, 0);
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
        case WM_MOUSEMOVE:
        {
            TRACKMOUSEEVENT MouseEvent = { sizeof(TRACKMOUSEEVENT), TME_HOVER | TME_LEAVE, hwnd, HOVER_DEFAULT };
            TrackMouseEvent(&MouseEvent);
            RECT rect;
            int nsel = TabCtrl_GetCurSel(hwnd);
            POINT point = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
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
                    if (tab_move_from >= 0 && tab_move_from != index && KEY_DOWN(VK_LBUTTON))
                    {
                        tab_drag = true;
                    }
                    else if (!tab_drag)
                    {
                        on_tabpage_draw_close(hwnd, &rect, index == nsel);
                        p->at_close = true;
                    }
                }
                else if (p->at_close)
                {
                    on_tabpage_undraw_close(hwnd, &rect);
                    p->at_close = false;
                }
            }
            break;
        }
        case WM_MOUSEHOVER:
        {   // 鼠标悬停激活
            break;
        }
        case WM_MOUSELEAVE:
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
                    on_tabpage_undraw_close(hwnd, &rect);
                }
            }
            break;
        }
        case WM_LBUTTONDOWN:
        {
            RECT rect_tabbar;
            POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            count = TabCtrl_GetItemCount(hwnd);
            tab_move_from = -1;
            tab_drag = false;
            for (index = 0; index < count; ++index)
            {
                TabCtrl_GetItemRect(hwnd, index, &rect_tabbar);
                if (on_tabpage_hit_button(&rect_tabbar, &pt))
                {
                    PostMessage(hwnd, WM_MBUTTONUP, 0, lParam);
                    return 1;
                }
                if (rect_tabbar.left < pt.x && pt.x < rect_tabbar.right && rect_tabbar.top < pt.y && pt.y < rect_tabbar.bottom)
                {
                    tab_move_from = index;
                    break;
                }
            }
            if (GetCapture() != hwnd)
            {
                SetCapture(hwnd);
            }
            break;
        }
        case WM_LBUTTONUP:
        {
            RECT rect = {0};
            POINT point = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            GetClientRect(hwnd, &rect);
            ReleaseCapture();
            if (tab_move_from >= 0)
            {
                int count = TabCtrl_GetItemCount(hwnd);
                if (!PtInRect(&rect, point))
                {   // Get cursor position of "Screen"
                    GetCursorPos(&point);
                    on_tabpage_drag_mouse(&point);
                    return 1;
                }
                if (!tab_drag)
                {
                    break;
                }
                if ((index = on_tabpage_hit_index(&point)) != -1)
                {
                    on_tabpage_exchange_item(tab_move_from, index);
                }
            }
            break;
        }
        case WM_RBUTTONDOWN:
        {
            POINT point = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            int nsel = TabCtrl_GetCurSel(hwnd);
            if ((index = on_tabpage_hit_index(&point)) != -1 && index != nsel)
            {
                on_tabpage_select_index(index);
                break;
            }
            return 1;
        }
        case WM_RBUTTONUP:
        {
            if ((p = on_tabpage_get_ptr(TabCtrl_GetCurSel(hwnd))))
            {
                return menu_pop_track(eu_module_hwnd(), IDR_TABPAGE_POPUPMENU, 0, 0, on_tabpage_menu_callback, p);
            }
            return 1;
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
    uint32_t flags = WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TCS_TOOLTIPS | TCS_OWNERDRAWFIXED;
#ifdef _M_X64
    if (!util_under_wine())
    {
        flags &= ~TCS_SINGLELINE;
        flags |= TCS_MULTILINE;
    }
#endif
    g_tabpages =
        CreateWindow(WC_TABCONTROL, NULL, flags, 0, 0, 0, 0, hwnd, (HMENU)IDM_TABPAGE_BAR, eu_module_handle(), NULL);
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
        UpdateWindow(g_tabpages);
        if (!(old_tabproc = (WNDPROC) SetWindowLongPtr(g_tabpages, GWLP_WNDPROC, (LONG_PTR) tabs_proc)))
        {
            err = 1;
            break;
        }
    } while(0);
    if (err)
    {
        if (g_tabpages)
        {
            DestroyWindow(g_tabpages);
            g_tabpages = NULL;
        }
    }
    return err;
}

void
on_tabpage_adjust_box(RECT *ptp)
{
    RECT rect_main;
    RECT rect_treebar = { 0 };
    int tab_height = on_tabpage_get_height();
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
    ptp->right = rect_main.right;
    ptp->top = rect_main.top + on_toolbar_height();
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
    if (RESULT_SHOW(pnode) && hwnd_rst)
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
            on_sci_free_tab(ppnode);
            break;
        }
    }
    return index;
}

static int
on_tabpage_remove_empty(void)
{
    int count;
    int ret = 0;
    EU_VERIFY(g_tabpages != NULL);
    if ((count = TabCtrl_GetItemCount(g_tabpages)) < 2)
    {
        return 0;
    }
    for (int index = 0; index < count; ++index)
    {
        eu_tabpage *p = on_tabpage_get_ptr(index);
        if (p && p->is_blank && !eu_sci_call(p, SCI_GETLENGTH, 0, 0))
        {
            if (!on_sci_doc_modified(p))
            {
                ret = 1;
                TabCtrl_DeleteItem(g_tabpages, index);
                on_sci_free_tab(&p);
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
                if (eu_cvector_at(v, ret))
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
                TCHAR old[MAX_PATH] = {0};
                if ((pstr = _tcsrchr(p->pathfile, ch)) != NULL && (pstr - p->pathfile) > 0 && _tcslen(pstr) > 0 &&
                    _tcsspn(pstr + 1, _T("0123456789")) == _tcslen(pstr + 1))
                {
                    _tcsncpy(old, p->pathfile, pstr - p->pathfile);
                    if (_tcscmp(filename, old) != 0)
                    {
                        _sntprintf(filename, MAX_PATH-1, m_file, _tstoi(pstr + 1));
                        _tcscpy(p->pathfile, filename);
                        _tcscpy(p->filename, filename);
                        util_set_title(p->pathfile);
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
    TCITEM tci = { 0 };
    if (pnode->codepage != IDM_OTHER_BIN && !pnode->hex_mode)
    {
        pnode->doc_ptr = on_doc_get_type(pnode->filename);
    }
    tci.mask = TCIF_TEXT | TCIF_PARAM;
    tci.pszText = pnode->filename;
    tci.lParam = (LPARAM) pnode;
    pnode->tab_id = TabCtrl_GetItemCount(g_tabpages);
    if (TabCtrl_InsertItem(g_tabpages, pnode->tab_id, &tci) == -1)
    {
        printf("TabCtrl_InsertItem return failed on %s:%d\n", __FILE__, __LINE__);
        return 1;
    }
    if (!pnode->is_blank)
    {
        pnode->tab_id -= on_tabpage_remove_empty();
    }
    if ((pnode->fs_server.networkaddr[0] == 0 && pnode->codepage == IDM_OTHER_BIN) || pnode->hex_mode)
    {
        pnode->hex_mode = true;
        pnode->bytes_remaining = pnode->raw_size;
        pnode->tab_id = -1;
        printf("hexview_init, pnode = %p, pnode->bytes_remaining = %I64d\n", pnode, pnode->bytes_remaining);
        return !hexview_init(pnode);
    }
    return on_sci_init_dlg(pnode);
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
    eu_sci_call(pnode, SCI_BEGINUNDOACTION, 0, 0);
    eu_sci_call(pnode, SCI_INSERTTEXT, 0, (sptr_t) str);
    eu_sci_call(pnode, SCI_DELETERANGE, 0, strlen(str));
    eu_sci_call(pnode, SCI_ENDUNDOACTION, 0, 0);
    return 0;
}

int
on_tabpage_reload_file(eu_tabpage *pnode, int flags)
{
    EU_VERIFY(pnode != NULL);
    switch (flags)
    {
        case 0: // 保留
            on_tabpage_editor_modify(pnode, "X");
            break;
        case 1: // 丢弃
            eu_sci_call(pnode, SCI_SETSAVEPOINT, 0, 0);
            on_file_close(pnode, FILE_ONLY_CLOSE);
            break;
        case 2: // 重载
        {
            sptr_t max_line;
            sptr_t pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
            sptr_t current_line = eu_sci_call(pnode, SCI_LINEFROMPOSITION, pos, 0);
            eu_sci_call(pnode, SCI_CLEARALL, 0, 0);
            if (on_file_to_tab(pnode, NULL, true))
            {
                return 1;
            }
            max_line = eu_sci_call(pnode, SCI_GETLINECOUNT, 0, 0);
            if (current_line > max_line - 1)
            {
                current_line = max_line - 1;
            }
            eu_sci_call(pnode, SCI_SETUNDOCOLLECTION, 1, 0);
            eu_sci_call(pnode, SCI_EMPTYUNDOBUFFER, 0, 0);
            eu_sci_call(pnode, SCI_SETSAVEPOINT, 0, 0);
            on_search_jmp_line(pnode, current_line, 0);
            pnode->st_mtime = util_last_time(pnode->pathfile);
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
    if (p->result_show && p->hwnd_qrtable)
    {
        SendMessage(p->hwnd_qrtable, WM_THEMECHANGED, 0, 0);
    }
    return 0;
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
    int count = TabCtrl_GetItemCount(g_tabpages);
    if (index < 0)
    {
        for (index = 0; index < count; ++index)
        {
            eu_tabpage *p = on_tabpage_get_ptr(index);
            if (p && p == pnode)
            {
                break;
            }
        }
    }
    if(index >= 0 && index < count)
    {
        HWND hwnd = eu_module_hwnd();
        TabCtrl_SetCurSel(g_tabpages, index);
        util_set_title(pnode->pathfile);
        on_toolbar_update_button();
        eu_window_resize(hwnd);
        // 窗口处理过程中可能改变了标签位置, 重置它
        TabCtrl_SetCurSel(g_tabpages, index);
        util_set_title(pnode->pathfile);
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
    EU_VERIFY(g_tabpages != NULL);
    eu_tabpage *p = NULL;
    for (int index = 0, count = TabCtrl_GetItemCount(g_tabpages); index < count; ++index)
    {
        if ((p = on_tabpage_get_ptr(index)) && p && (p == pnode))
        {
            return index;
        }
    }
    return SKYLARK_TABCTRL_ERR;
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
on_tabpage_changing(HWND hwnd)
{
    eu_tabpage *p = NULL;
    int index = TabCtrl_GetCurSel(g_tabpages);
    if((p = on_tabpage_get_ptr(index)) != NULL)
    {
        util_set_title(p->pathfile);
        on_toolbar_update_button();
        SendMessage(hwnd, WM_TAB_CLICK, (WPARAM)p, 0);
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
