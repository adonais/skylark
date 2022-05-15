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

#ifndef _H_SKYLARK_SPLITTER_
#define _H_SKYLARK_SPLITTER_

#ifdef __cplusplus
extern "C"
{
#endif

extern HWND g_splitter_treebar;
extern HWND g_splitter_symbar;
extern HWND g_splitter_editbar;
extern HWND g_splitter_tablebar;

bool on_splitter_init_treebar(HWND parent);
bool on_splitter_init_symbar(HWND parent);
bool on_splitter_init_editbar(HWND parent);
bool on_splitter_init_tablebar(HWND parent);

#ifdef __cplusplus
}
#endif

#endif
