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

#ifndef _EU_WNDPROC_H_
#define _EU_WNDPROC_H_

#define APP_CLASS _T("__eu_skylark__")
#define HEX_CLASS _T("__eu_hexview__")
#define RESULT_SHOW(p)  (p && p->result_show && p->presult && p->presult->hwnd_sc)

#ifdef __cplusplus
extern "C"
{
#endif

extern volatile long g_interval_count;

int eu_dpi_scale_font(void);
int eu_dpi_scale_xy(int adpi, int m);
void on_proc_destory_brush(void);
void on_proc_resize(HWND hwnd);
void on_proc_undo_off(void);
void eu_window_resize(HWND hwnd);
void eu_window_layout_dpi(HWND hwnd, const RECT *pnew_rect, const uint32_t adpi);
HWND eu_hwnd_self(void);
LRESULT __stdcall eu_main_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

#ifdef __cplusplus
}
#endif

#endif  // _EU_WNDPROC_H_
