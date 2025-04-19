/******************************************************************************
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

#ifndef _EU_SCRIPT_H_
#define _EU_SCRIPT_H_

enum
{
    SKYLARK_INIT = 1,
    SKYLARK_SHUTDOWN,
    SKYLARK_MINIMIZED,
    SKYLARK_RESTORED,
    SKYLARK_KEYUP,
    SKYLARK_MOUSEUP,
    SKYLARK_SELETION,
    SKYLARK_FILESAVE,
    SKYLARK_FILESAVEAS,
    SKYLARK_COMMANDS
};

typedef struct _obs_skylark
{
    int index;
    char pfile[MAX_BUFFER];
    char pname[MAX_PATH];
} obs_skylark;

#ifdef __cplusplus
extern "C" {
#endif

int   do_lua_func(const char *, const char *, const char *arg);
int   do_lua_code(const char *s, const char *filename);
int   do_byte_code(eu_tabpage *pnode);
int   do_lua_point(const char *fname, const char *func, void *arg);
int   do_lua_parser_doctype(const char *fname, const char *func);
int   on_script_bcsaved(char *arg1, char *arg2);
void  on_script_loader(void);
void  on_script_loader_event(const int event, void *pnode);
void  do_lua_parser_release(void);
bool  do_lua_setting_path(eu_tabpage *pnode);
TCHAR *do_lua_parser_path(const char *file);

#ifdef __cplusplus
}
#endif

#endif  // _EU_SCRIPT_H_
