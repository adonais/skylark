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

#ifndef _H_SKYLARK_VIEW_
#define _H_SKYLARK_VIEW_

#define KEY_DOWN(vk_code) ((GetAsyncKeyState(vk_code) & 0x8000) ? 1 : 0)
#define KEY_UP(vk_code) ((GetAsyncKeyState(vk_code) & 0x8000) ? 0 : 1)
    
#ifdef __cplusplus
extern "C" {
#endif

int on_view_switch_type(const int type);
int on_view_refresh_theme(HWND hwnd, const bool reload);
int on_view_switch_theme(HWND hwnd, const int id);
int on_view_modify_theme(void);
int on_view_editor_selection(eu_tabpage *pnode);
void on_view_filetree(void);
void on_view_symtree(eu_tabpage *pnode);
void on_view_document_map(eu_tabpage *pnode);
void on_view_show_fold_lines(void);
void on_view_wrap_line(void);
void on_view_line_num(void);
void on_view_bookmark(void);
void on_view_white_space(void);
void on_view_line_visiable(void);
void on_view_indent_visiable(void);
void on_view_history_visiable(eu_tabpage *p, const int wm_id);
void on_view_light_str(eu_tabpage *p);
void on_view_light_brace(eu_tabpage *p);
void on_view_light_fold(void);
void on_view_identation(void);
void on_view_result_show(eu_tabpage *pnode, const int key);
void on_view_copy_theme(void);
void on_view_tab_width(HWND hwnd, eu_tabpage *pnode);
void on_view_space_converter(HWND hwnd, eu_tabpage *pnode);
void on_view_zoom_out(eu_tabpage *pnode);
void on_view_zoom_in(eu_tabpage *pnode);
void on_view_zoom_reset(eu_tabpage *pnode);
void on_view_setfullscreenimpl(HWND hwnd);
void on_view_full_sreen(HWND hwnd);
void on_view_font_quality(HWND hwnd, const int res_id);
void on_view_enable_rendering(HWND hwnd, const int res_id);

#ifdef __cplusplus
}
#endif

#endif  // _H_SKYLARK_VIEW_
