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

#ifndef _SKYLARK_FORMAT_H_
#define _SKYLARK_FORMAT_H_

typedef int (*format_back)(const uint8_t *ptext, uint8_t **pbuf);
typedef struct _rcstring
{
    char *text;    /*<! char c-string */
    size_t length; /*<! put in place to avoid strlen() calls */
    size_t max;    /*<! usable memory allocated to text minus the space for the nul character */
} rcstring;

#ifdef __cplusplus
extern "C"
{
#endif

int  on_format_js_callback(const uint8_t *text, uint8_t **pbuf);
int  on_format_json_callback(const uint8_t *text, uint8_t **pbuf);
void on_format_file_style(eu_tabpage *pnode);
void on_format_do_compress(eu_tabpage *pnode, format_back fn); 
void on_format_clang_file(eu_tabpage *p, const bool whole);

#ifdef __cplusplus
}
#endif

#endif // _SKYLARK_FORMAT_H_
