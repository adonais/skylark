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

#ifndef _EU_SCINTILLA_H_
#define _EU_SCINTILLA_H_

#ifdef __cplusplus
extern "C"
{
#endif

extern volatile sptr_t eu_edit_wnd;
int on_sci_init_dlg(eu_tabpage *pnode);
int on_sci_query_tab(eu_tabpage *pnode);
int on_sci_point_reached(eu_tabpage *pnode);
int on_sci_point_left(eu_tabpage *pnode);
int on_sci_create(eu_tabpage *pnode, HWND parent, int flags, WNDPROC sc_callback);
void on_sci_init_style(eu_tabpage *pnode);
void on_sci_before_file(eu_tabpage *pnode);
void on_sci_after_file(eu_tabpage *pnode);
void on_sci_character(eu_tabpage *pnode, ptr_notify lpnotify);
void on_sci_send_extra(void *pdata, uint32_t code, LPNMHDR phdr);
void on_sci_update_margin(eu_tabpage *pnode);
void on_sci_resever_tab(eu_tabpage *pnode);
void on_sci_free_tab(eu_tabpage **ppnode);
void on_sci_insert_egg(eu_tabpage *pnode);
void on_sic_mousewheel(eu_tabpage *pnode, WPARAM wParam, LPARAM lParam);
bool on_sci_doc_modified(eu_tabpage *pnode);
bool on_sci_line_text(eu_tabpage *pnode, size_t lineno, char *buf, size_t len);
char *on_sci_range_text(eu_tabpage *pnode, sptr_t start, sptr_t end);

#ifdef __cplusplus
}
#endif

#endif  // _EU_SCINTILLA_H_
