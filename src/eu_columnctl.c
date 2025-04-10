/*******************************************************************************
 * This file is part of Skylark project
 * Copyright ©2025 Hua andy <hua.andy@gmail.com>

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

static HWND hwnd_column = NULL;

static
on_column_apply(HWND hdlg)
{
    SendMessage(hdlg, WM_CLOSE, 0, 0);
}

static void
on_column_destory(void)
{
    if (hwnd_column)
    {
        DestroyWindow(hwnd_column);
        hwnd_column = NULL;
    }
}

static INT_PTR CALLBACK
on_column_proc(HWND hdlg, uint32_t msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            HWND hwnd_cmb = GetDlgItem(hdlg, IDC_COL_LEADING_COMBO);
            HWND hwnd_edt1 = GetDlgItem(hdlg, IDC_COL_INITNUM_EDIT);
            HWND hwnd_edt2 = GetDlgItem(hdlg, IDC_COL_INCREASENUM_EDIT);
            HWND hwnd_edt3 = GetDlgItem(hdlg, IDC_COL_REPEATNUM_EDIT);
            HICON m_icon = LoadIcon(eu_module_handle(), MAKEINTRESOURCE(IDI_SKYLARK));
            if (m_icon)
            {
                SetClassLongPtr(hdlg, GCLP_HICONSM, (LONG_PTR)m_icon);
            }
            if (hwnd_cmb && hwnd_edt1 && hwnd_edt2 && hwnd_edt3)
            {
                SetWindowSubclass(hwnd_cmb, on_dark_cmb_proc, COLUMN_CMB_SUBID, 0);
                SendMessage(hwnd_edt1, WM_SETFONT, (WPARAM) on_theme_font_hwnd(), 0);
                SendMessage(hwnd_edt2, WM_SETFONT, (WPARAM) on_theme_font_hwnd(), 0);
                SendMessage(hwnd_edt3, WM_SETFONT, (WPARAM) on_theme_font_hwnd(), 0);
            }
            LOAD_I18N_RESSTR(IDS_COLUMN_FILL_NONE, bufn);
            LOAD_I18N_RESSTR(IDS_COLUMN_FILL_ZEROS, bufz);
            LOAD_I18N_RESSTR(IDS_COLUMN_FILL_SPACES, bufs);
            SendDlgItemMessage(hdlg, IDC_COL_LEADING_COMBO, CB_ADDSTRING, 0, (LPARAM)bufn);
            SendDlgItemMessage(hdlg, IDC_COL_LEADING_COMBO, CB_ADDSTRING, 0, (LPARAM)bufz);
            SendDlgItemMessage(hdlg, IDC_COL_LEADING_COMBO, CB_ADDSTRING, 0, (LPARAM)bufs);
            SendDlgItemMessage(hdlg, IDC_COL_DEC_RADIO, BM_SETCHECK, TRUE, 0);
            util_creater_window(hdlg, eu_module_hwnd());
            if (on_dark_enable())
            {
                on_dark_set_theme(hdlg, L"Explorer", NULL);
            }
            return (INT_PTR)SendMessage(hdlg, WM_SETFONT, (WPARAM) on_theme_font_hwnd(), 0);
        }
        case WM_THEMECHANGED:
        {
            if (on_dark_supports())
            {
                int id = 0;
                bool dark = on_dark_enable();
                on_dark_allow_window(hdlg, dark);
                on_dark_refresh_titlebar(hdlg);
                const int buttons[] = {IDOK,
                                       IDCANCEL
                                      };
                const int bs_btn[] = {IDC_COL_FORMAT_GRP_STATIC,
                                      IDC_COL_DEC_RADIO,
                                      IDC_COL_HEX_RADIO,
                                      IDC_COL_OCT_RADIO,
                                      IDC_COL_BIN_RADIO
                                      };
                for (; id < _countof(buttons); ++id)
                {
                    HWND btn = GetDlgItem(hdlg, buttons[id]);
                    on_dark_allow_window(btn, dark);
                    on_dark_set_theme(btn, L"Explorer", NULL);
                }
                for (id = 0; id < _countof(bs_btn); ++id)
                {
                    HWND btn = GetDlgItem(hdlg, bs_btn[id]);
                    on_dark_set_theme(btn, L"", L"");
                    SendMessage(btn, WM_THEMECHANGED, 0, 0);
                }
                UpdateWindowEx(hdlg);
            }
            break;
        }
        CASE_WM_CTLCOLOR_SET:
        {
            return on_dark_set_contorl_color(wParam);
        }
        case WM_COMMAND:
        {
            WORD mid = LOWORD(wParam);
            switch (mid)
            {
                case IDCANCEL:
                {
                    on_column_destory();
                    break;
                }
                case IDC_COL_INITNUM_EDIT:
                {
                    break;
                }
                case IDC_COL_INCREASENUM_EDIT:
                {
                    break;
                }
                case IDC_COL_REPEATNUM_EDIT:
                {
                    break;
                }
                case IDC_COL_LEADING_COMBO:
                {
                    break;
                }
                case IDC_COL_DEC_RADIO:
                case IDC_COL_HEX_RADIO:
                case IDC_COL_OCT_RADIO:
                case IDC_COL_BIN_RADIO:
                {
                    break;
                }
                case IDOK:
                {
                    on_column_apply(hdlg);
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case WM_DESTROY:
        {
            if (hwnd_column)
            {
                hwnd_column = NULL;
                eu_logmsg("Column: hwnd_column destroy\n");
            }
            break;
        }
        default:
            break;
    }
    return 0;
}

void
on_column_create_dlg(HWND parent)
{
    if (!hwnd_column && !(hwnd_column = i18n_create_dialog(parent, IDD_COLUMNEDIT_DLG, on_column_proc)))
    {
        eu_logmsg("Column: %s, hwnd_column is null\n", __FUNCTION__);
    }
}

HWND
eu_column_hwnd(void)
{
    return hwnd_column;
}
