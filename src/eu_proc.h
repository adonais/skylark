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
#define QRTABLE_SHOW(p) (p && p->hwnd_qrtable && p->qrtable_show)

#ifdef __cplusplus
extern "C"
{
#endif

void on_proc_redraw(const RECT *prc);
void on_proc_counter_stop(void);
void on_proc_sync_wait(void);
void on_proc_msg_active(eu_tabpage *pnode);
void on_proc_tab_click(eu_tabpage *pnode);
void eu_window_resize(void);
void eu_window_layout_dpi(HWND hwnd, const RECT *pnew_rect, const uint32_t adpi);
int  eu_dpi_scale_font(void);
int  eu_dpi_scale_style(int value, const int scale, const int min_value);
int  eu_dpi_scale_xy(int adpi, int m);
HWND eu_hwnd_self(void);
unsigned long on_proc_thread(void);

#ifdef __cplusplus
}
#endif

#endif  // _EU_WNDPROC_H_
