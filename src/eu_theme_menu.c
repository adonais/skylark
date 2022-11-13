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
#include <uxtheme.h>
#include <vsstyle.h>

typedef HRESULT (WINAPI *DrawThemeTextExPtr)(HTHEME, HDC, int, int, LPCWSTR, int, DWORD, LPRECT, const DTTOPTS *);

static HTHEME g_menu_theme;

static HRESULT
on_theme_drawex_text(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, LPCWSTR pszText,int cchText, DWORD dwTextFlags, LPRECT pRect, const DTTOPTS *pOptions)
{
    HRESULT ret = 1;
    HMODULE uxtheme = LoadLibraryEx(_T("uxtheme.dll"), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    DrawThemeTextExPtr fnDrawThemeTextEx = uxtheme ? (DrawThemeTextExPtr)GetProcAddress(uxtheme, "DrawThemeTextEx") : NULL;
    if (fnDrawThemeTextEx)
    {
        ret = fnDrawThemeTextEx(hTheme, hdc, iPartId, iStateId, pszText,cchText, dwTextFlags, pRect, pOptions);
    }
    eu_close_dll(uxtheme);
    return ret;
}

void
on_theme_menu_release(void)
{
    if (g_menu_theme)
    {
        on_dark_close_data((void *)g_menu_theme);
        g_menu_theme = NULL;
    }
}

/*****************************************************************************
 * 处理与UAH/自定义菜单栏绘图相关的消息
 * 如果已处理, 返回true: 如果为false, 标准窗口回调函数继续进行
 ****************************************************************************/
bool
on_theme_menu_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* lr)
{
    UNREFERENCED_PARAMETER(wParam);
    switch (message)
    {
        case WM_UAHDRAWMENU:
        {
            UAHMENU* pdm = (UAHMENU*)lParam;
            RECT rc = { 0 };
            MENUBARINFO mbi = { sizeof(mbi) };
            GetMenuBarInfo(hwnd, OBJID_MENU, 0, &mbi);

            RECT rc_win;
            GetWindowRect(hwnd, &rc_win);

            // 获取菜单位置
            rc = mbi.rcBar;
            OffsetRect(&rc, -rc_win.left, -rc_win.top);

            FillRect(pdm->hdc, &rc, (HBRUSH)on_dark_get_brush());
            return true;
        }
        case WM_UAHDRAWMENUITEM:
        {
            UAHDRAWMENUITEM* pdmi = (UAHDRAWMENUITEM*)lParam;

            // 获取菜单字符串
            wchar_t menuString[MAX_SIZE] = { 0 };
            MENUITEMINFO mii = { sizeof(mii), MIIM_STRING };
            mii.dwTypeData = menuString;
            mii.cch = (sizeof(menuString) / 2) - 1;
            GetMenuItemInfo(pdmi->um.hmenu, pdmi->umi.iPosition, TRUE, &mii);

            uint32_t flags = DT_CENTER | DT_SINGLELINE | DT_VCENTER;
            int text_id = MPI_NORMAL;
            int background_id = MPI_NORMAL;
            {
                if ((pdmi->dis.itemState & ODS_INACTIVE) | (pdmi->dis.itemState & ODS_DEFAULT))
                {
                    // normal display
                    text_id = MPI_NORMAL;
                    background_id = MPI_NORMAL;
                }
                if (pdmi->dis.itemState & ODS_HOTLIGHT)
                {
                    // hot tracking
                    text_id = MPI_HOT;
                    background_id = MPI_HOT;
                }
                if (pdmi->dis.itemState & ODS_SELECTED)
                {
                    // clicked -- MENU_POPUPITEM has no state for this, though MENU_BARITEM does
                    text_id = MPI_HOT;
                    background_id = MPI_HOT;
                }
                if ((pdmi->dis.itemState & ODS_GRAYED) || (pdmi->dis.itemState & ODS_DISABLED))
                {
                    // disabled / grey text
                    text_id = MPI_DISABLED;
                    background_id = MPI_DISABLED;
                }
                if (pdmi->dis.itemState & ODS_NOACCEL)
                {
                    flags |= DT_HIDEPREFIX;
                }
            }
            if (!g_menu_theme)
            {
                g_menu_theme = (HTHEME)on_dark_open_data(hwnd, L"Menu");
            }
            if (background_id == MPI_NORMAL || background_id == MPI_DISABLED)
            {
                FillRect(pdmi->um.hdc, &pdmi->dis.rcItem, (HBRUSH)on_dark_get_brush());
            }
            else if (background_id == MPI_HOT || background_id == MPI_DISABLEDHOT)
            {
                FillRect(pdmi->um.hdc, &pdmi->dis.rcItem, (HBRUSH)on_dark_get_hot_brush());
            }
            else
            {
                on_dark_draw_background(g_menu_theme, pdmi->um.hdc, MENU_POPUPITEM, background_id, &pdmi->dis.rcItem, NULL);
            }
            DTTOPTS dttopts = { sizeof(dttopts) };
            if (text_id == MPI_NORMAL || text_id == MPI_HOT)
            {
                dttopts.dwFlags |= DTT_TEXTCOLOR;
                dttopts.crText = rgb_dark_txt_color;
            }
            on_theme_drawex_text(g_menu_theme, pdmi->um.hdc, MENU_POPUPITEM, text_id, menuString, mii.cch, flags, &pdmi->dis.rcItem, &dttopts);
            *lr = 0;
            return true;
        }
        default:
            return false;
    }
}

