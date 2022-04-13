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

#ifndef _H_SKYLARK_ENCODING_
#define _H_SKYLARK_ENCODING_

typedef char *(__stdcall *enc_back)(const char *, size_t *);

#ifdef __cplusplus
extern "C" {
#endif

enum wanring_ll
{
    WANRING_AUTO = -1,
    WANRING_OFF,
    WANRING_ENABLE
};

typedef struct _euconv
{
    int warning;
    iconv_t cd;
    const char *src_from;
    const char *dst_to;
    api_convert fn_convert;
}euconv_t;

int on_encoding_line_mode(const char *str , size_t len);
int on_encoding_set_bom(const uint8_t *buf, eu_tabpage *pnode);
int on_encoding_convert_internal_code(eu_tabpage *pnode, enc_back fn);
void on_encoding_set_bom_from_cp(eu_tabpage *pnode);
size_t on_encoding_do_iconv(euconv_t *icv, char *src, size_t *src_len, char **dst, size_t *plen);
const char *on_encoding_get_eol(eu_tabpage *pnode);
const int on_encoding_eol_char(eu_tabpage *pnode);
char* __stdcall on_encoding_gb_big5(const char *gb2, size_t *out_len);
char*__stdcall on_encoding_big5_gb(const char *bg5, size_t *out_len);

#ifdef __cplusplus
}
#endif

#endif
