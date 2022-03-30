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

typedef struct _vec_str
{
    const TCHAR *tips;
    TCHAR *str;
    int len;
} vec_str;

static INT_PTR CALLBACK
input_proc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
        {
            HWND text = NULL;
            HWND hdis = GetDlgItem(hdlg, IDC_INPUT_STC1);
            vec_str input = *(vec_str *) lParam;
            SetWindowLongPtr(hdlg, GWLP_USERDATA, lParam);
            if (hdis && input.tips && input.tips[0])
            {
                Static_SetText(hdis, input.tips);
            }
            text = GetDlgItem(hdlg, IDC_INPUT_EDT1);
            if (text && input.str[0])
            {
                Edit_SetText(text, input.str);
            }
            SendMessage(hdlg, DM_SETDEFID, IDC_INPUT_BTN1, 0);
            if (on_dark_enable())
            {
                const int buttons[] = {IDC_INPUT_BTN1, IDC_INPUT_BTN2};
                for (int id = 0; id < _countof(buttons); ++id) 
                {
                    HWND btn = GetDlgItem(hdlg, buttons[id]);
                    on_dark_set_theme(btn, L"Explorer", NULL);
                }
                on_dark_set_theme(hdlg, L"Explorer", NULL);
            }
            return util_creater_window(hdlg, eu_module_hwnd());
        }
        case WM_THEMECHANGED:
        {
            if (on_dark_enable())
            {
                on_dark_allow_window(hdlg, true);
                on_dark_refresh_titlebar(hdlg);
                const int buttons[] = {IDC_INPUT_BTN1, IDC_INPUT_BTN2};
                for (int id = 0; id < _countof(buttons); ++id) 
                {
                    HWND btn = GetDlgItem(hdlg, buttons[id]);
                    on_dark_allow_window(btn, true);
                    SendMessage(btn, WM_THEMECHANGED, 0, 0);
                }
                UpdateWindow(hdlg);
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
            HWND hdis = GetDlgItem(hdlg, IDC_INPUT_EDT1);
            vec_str *pi = (vec_str *) GetWindowLongPtr(hdlg, GWLP_USERDATA);
            switch (mid)
            {
                case IDC_INPUT_BTN1:
                {
                    Edit_GetText(hdis, pi->str, pi->len);
                    return EndDialog(hdlg, LOWORD(wParam));
                }
                case IDC_INPUT_BTN2:
                {
                    *(pi->str) = 0;
                    return EndDialog(hdlg, 0);
                }
                default:
                    break;
            }
            break;
        }
        default:
            break;
    }
    return 0;
}

bool
eu_input(const TCHAR *tips, TCHAR *str, int len)
{
    vec_str input_str = { tips, str, len - 1 };
    return i18n_dlgbox(eu_module_hwnd(), IDD_DLG_INPUT, input_proc, (LPARAM) &input_str) > 0;
}
