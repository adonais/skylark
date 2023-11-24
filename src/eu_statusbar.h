/*******************************************************************************
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

#ifndef _EU_STATUSBAR_H_
#define _EU_STATUSBAR_H_

#define STATUSBAR_DEFHIGHT 22
#define STATUSBAR_DOC_INFO 0
#define STATUSBAR_DOC_POS  1
#define STATUSBAR_DOC_EOLS 2
#define STATUSBAR_DOC_ENC  3
#define STATUSBAR_DOC_TYPE 4
#define STATUSBAR_DOC_SIZE 5
#define STATUSBAR_DOC_BTN  6
#define STATUSBAR_PART     7
#define EDITNUMBS          11
#define IMAGEWIDTH         16
#define IMAGEHEIGHT        15

#define FILE_READONLY_COLOR 0x00800000

// c++ compiler
#ifdef __cplusplus
extern "C" {
#endif

extern HWND g_statusbar;

int  on_statusbar_create_dlg(HWND hwnd);
int  on_statusbar_height(void);
int  on_statusbar_btn_rw(eu_tabpage *pnode, bool m_auto);
void on_statusbar_size(eu_tabpage *pnode);
void on_statusbar_update(void);
void on_statusbar_destroy(void);
void on_statusbar_adjust_box(void);
void on_statusbar_update_eol(eu_tabpage *pnode, const int eol);
void on_statusbar_update_line(eu_tabpage *pnode);
void on_statusbar_update_filesize(eu_tabpage *pnode);
void on_statusbar_update_coding(eu_tabpage *pnode, const int res_id);
void on_statusbar_update_fileinfo(eu_tabpage *pnode, const TCHAR *print_str);
void on_statusbar_dark_mode(const bool dark);
void on_statusbar_pop_menu(int parts, LPPOINT pt);
void on_statusbar_btn_colour(eu_tabpage *pnode, bool only_read);

#ifdef __cplusplus
}
#endif

#endif // _EU_STATUSBAR_H_