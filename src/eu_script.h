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

#ifndef _EU_SCRIPT_H_
#define _EU_SCRIPT_H_

#ifdef __cplusplus
extern "C" {
#endif

int do_lua_func(const char *, const char *, const char *arg);
int do_lua_code(const char *s);
int do_byte_code(eu_tabpage *pnode);
int do_lua_point(const char *fname, const char *func, void *arg);
int do_lua_parser_doctype(const char *fname, const char *func);
void do_lua_parser_release(void);
bool do_lua_setting_path(eu_tabpage *pnode);
TCHAR* do_lua_parser_path(const char *file);

#ifdef __cplusplus
}
#endif

#endif  // _EU_SCRIPT_H_
