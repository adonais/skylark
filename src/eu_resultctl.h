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

#ifndef _EU_RESULT_EDIT_H_
#define _EU_RESULT_EDIT_H_

#define SQLQUERYRESULT_LISTVIEW_HEIGHT_MIN 40

#ifdef __cplusplus
extern "C" {
#endif

int on_result_append_text(wchar_t*format , ...);
int on_result_append_text_utf8(char *format , ...);
bool on_result_launch(eu_tabpage *pnode);
void on_result_reload(eu_tabpage *p);
void on_result_move_sci(eu_tabpage *p, int width, int height);

#ifdef __cplusplus
}
#endif

#endif  // _EU_RESULT_EDIT_H_
