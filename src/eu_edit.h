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

#ifndef _H_SKYLARK_EDIT_
#define _H_SKYLARK_EDIT_

#define MIN_POS(x, y) (((x) < (y)) ? (x) : (y))
// suppress -Wimplicit-fallthrough in C source
#if defined(__cplusplus)
#define FALLTHROUGH_ATTR        [[fallthrough]]
#elif (defined(__GNUC__) && __GNUC__ >= 7) || (defined(__clang__) && __clang_major__ >= 10)
#define FALLTHROUGH_ATTR        __attribute__((fallthrough))
#else
#define FALLTHROUGH_ATTR
#endif
#define STR_IS_NUL(s) (s == NULL || *s == 0)
#define STR_NOT_NUL(s) (s != NULL && *s != 0)
#define TAB_NOT_NUL(p) (eu_sci_call(p, SCI_GETLENGTH, 0, 0) > 0)
#define TAB_NOT_BIN(p) (p->codepage != IDM_OTHER_BIN)

#ifdef __cplusplus
extern "C"
{
#endif

extern char eols_undo_str[QW_SIZE];

void on_edit_undo(eu_tabpage *pnode);
void on_edit_redo(eu_tabpage *pnode);
void on_edit_cut(eu_tabpage *pnode);
void on_edit_copy_text(eu_tabpage *pnode);
void on_edit_paste_text(eu_tabpage *pnode);
void on_edit_delete_text(eu_tabpage *pnode);
void on_edit_line_up(eu_tabpage *pnode);
void on_edit_line_down(eu_tabpage *pnode);
void on_edit_cut_line(eu_tabpage *pnode);
void on_edit_copy_line(eu_tabpage *pnode);
void on_edit_delete_line(eu_tabpage *pnode);
void on_edit_delete_dups(eu_tabpage *pnode);
void on_edit_line_transpose(eu_tabpage *pnode);
void on_edit_delete_all_empty_lines(eu_tabpage *pnode);
void on_edit_delete_line_header_white(eu_tabpage *pnode);
void on_edit_delete_line_tail_white(eu_tabpage *pnode);
void on_edit_delete_line_header_all(eu_tabpage *pnode);
void on_edit_delete_line_tail_all(eu_tabpage *pnode);
bool on_edit_push_clipboard(const TCHAR *buf);

void on_edit_swap_clipborad(eu_tabpage *pnode);
void on_edit_clear_clipborad(HWND hwnd);
void on_edit_push_editor(eu_tabpage *pnode);
void on_edit_join_line(eu_tabpage *pnode);
void on_edit_lower(eu_tabpage *pnode);
void on_edit_upper(eu_tabpage *pnode);
void on_edit_sentence_upper(eu_tabpage *pnode, const bool sentence);
void on_edit_selection(eu_tabpage *pnode, int type);
void on_edit_undo_eol(eu_tabpage *pnode);
void on_edit_undo_iconv(eu_tabpage *pnode);
void on_edit_sorting(eu_tabpage *p, int wm_id);
void on_edit_push_compare(void);

int on_edit_base64_enc(eu_tabpage *pnode);
int on_edit_base64_dec(eu_tabpage *pnode);
int on_edit_md5(eu_tabpage *pnode);
int on_edit_sha1(eu_tabpage *pnode);
int on_edit_sha256(eu_tabpage *pnode);
int on_edit_descbc_enc(eu_tabpage *pnode);
int on_edit_descbc_dec(eu_tabpage *pnode);
int on_edit_comment_line(eu_tabpage *pnode);
int on_edit_comment_stream(eu_tabpage *pnode);
int on_edit_convert_eols(eu_tabpage *pnode, int new_eol);

bool on_edit_ssl_enc_base64(unsigned char *base64_pass, unsigned char *enc_str, int enc_len);
bool on_edit_ssl_dec_base64(unsigned char *base64_pass, unsigned char *enc_str, int enc_len);

#ifdef __cplusplus
}
#endif

#endif
