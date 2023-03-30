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

#ifndef _H_SKYLARK_UTIL_
#define _H_SKYLARK_UTIL_

#ifndef MAX
#define MAX(_a_, _b_) ((_a_) > (_b_) ? (_a_) : (_b_))
#endif

#ifndef MIN
#define MIN(_a_, _b_) ((_a_) < (_b_) ? (_a_) : (_b_))
#endif

#ifndef UTIL_SWAP
#define UTIL_SWAP(TYPE,A,B) {TYPE t=A; A=B; B=t;}
#endif

#ifndef UTIL_NUMBER
#define UTIL_NUMBER(ch) ((ch) - 0x30)
#endif

#ifndef UTIL_BASE10
#define UTIL_BASE10(ch) ((ch) >= 0x30 && (ch) <= 0x39)
#endif

#ifndef TWO_DISM
#define TWO_DISM 10
#endif

#define util_malloc(x) (HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (x)))
#define util_free(x)   (HeapFree(GetProcessHeap(), HEAP_ZERO_MEMORY, (x)),(x = NULL))

#define util_prev(p) ((p) - (psrc) > 0 ? (p[-1]) : (0))

#define END_CHARACTERS (" \t\r\n\"']>.*")

typedef struct _HANDLE_DATA
{
    uint32_t pid;
    HWND handle;
} handle_data;

#ifdef __cplusplus
extern "C"
{
#endif

// for openssl
HMODULE util_ssl_open_symbol(char *s[], int n, uintptr_t *pointer);
void util_ssl_close_symbol(HMODULE *pssl);

void util_lock(volatile long *gcs);
void util_unlock(volatile long *gcs);

int util_aes_enc(unsigned char *dec, unsigned char *enc, int len);
int util_aes_dec(unsigned char *enc, unsigned char *dec, int len);
int util_enc_des_ecb_192(unsigned char *key_192bits, unsigned char *decrypt, long decrypt_len, unsigned char *encrypt, long *encrypt_len);
int util_dec_des_ecb_192(unsigned char *key_192bits, unsigned char *encrypt, long encrypt_len, unsigned char *decrypt, long *decrypt_len);
int util_enc_des_cbc_192(unsigned char *key_192bits, unsigned char *decrypt, long decrypt_len, unsigned char *encrypt, long *encrypt_len, unsigned char *init_vector);
int util_dec_des_cbc_192(unsigned char *key_192bits, unsigned char *encrypt, long encrypt_len, unsigned char *decrypt, long *decrypt_len, unsigned char *init_vector);

int util_file_md5(const TCHAR *path, TCHAR **pout);
int util_file_sha1(const TCHAR *path, TCHAR **pout);
int util_file_sha256(const TCHAR *path, TCHAR **pout);
int util_file_base64(const TCHAR *path, char **pout);

int util_hex_expand(char *hex_buf, int hex_len, char *asc_buf);
int util_hex_fold(char *asc_buf, int asc_len, char *hex_buf);
int util_set_title(const TCHAR *filename);
int util_set_working_dir(const TCHAR *path, TCHAR **old);
int util_query_hostname(char *hostname, char *ip, int bufsize);
int util_effect_line(eu_tabpage *pnode, sptr_t *, sptr_t *);
int util_get_hex_byte(const char *p);
int util_strnspace(const char *s1, const char *s2, int *plen);

char*  util_unix_newline(const char *in, const size_t in_size);
char*  util_strdup_select(eu_tabpage *pnode, size_t *text_len, size_t multiple);
char*  util_strdup_line(eu_tabpage *pnode, const sptr_t line_number, size_t *plen);
char*  util_strdup_content(eu_tabpage *pnode, size_t *plen);
void   util_set_undo(eu_tabpage *p);
void   util_push_text_dlg(eu_tabpage *pnode, HWND hwnd);
void   util_enable_menu_item(HMENU hmenu, uint32_t m_id, bool enable);
void   util_set_menu_item(HMENU hmenu, uint32_t m_id, bool checked);
void   util_update_menu_chars(HMENU hmenu, uint32_t m_id, int width);
void   util_upper_string(char *str);
void   util_kill_thread(uint32_t pid);
void   util_wait_cursor(eu_tabpage *pnode);
void   util_restore_cursor(eu_tabpage *pnode);
void   util_setforce_eol(eu_tabpage *pnode);
void   util_save_placement(HWND hwnd);
void   util_restore_placement(HWND hwnd);
void   util_skip_whitespace(uint8_t **cp, int n, int term);
bool   util_availed_char(int ch);
bool   util_under_wine(void);
void   util_trim_right_star(TCHAR *str);
char*  util_struct_to_string(void *buf, size_t bufsize);
bool   util_string_to_struct(const char *buffer, void *buf, size_t bufsize);
bool   util_creater_window(HWND hwnd, HWND hparent);
bool   util_can_selections(eu_tabpage *pnode);
bool   util_file_size(HANDLE hfile, uint64_t *psize);
bool   util_open_file(LPCTSTR path, pf_stream pstream);
bool   util_delete_file(LPCTSTR filepath);
bool   util_exist_libcurl(void);
bool   util_upcheck_exist(void);
bool   util_get_clipboard(char **ppstr);
time_t util_last_time(const TCHAR *path);
uint64_t util_gen_tstamp(void);
void util_switch_menu_group(HMENU hmenu, int pop_id, uint32_t first_id, uint32_t last_id, uint32_t select);
WCHAR* util_to_abs(const char *path);
TCHAR* util_make_u16(const char *utf8, TCHAR *utf16, int len);
char*  util_make_u8(const TCHAR *utf16, char *utf8, int len);
char*  util_string_match(const char *str, const char *pattern, bool incase, bool match_start, bool whole);
char*  util_str_replace(const char *in, const char *pattern, const char *by);
HANDLE util_mk_temp(TCHAR *file_path, TCHAR *ext);
HWND   util_create_tips(HWND hwnd_stc, HWND hwnd, TCHAR* ptext);
HWND   util_get_hwnd(const uint32_t pid);
TCHAR* util_unix2path(TCHAR *path, int len);
TCHAR* util_path2unix(TCHAR *path, int len);
TCHAR* util_add_double_quotes(const TCHAR *path);
TCHAR* util_which(const TCHAR *name);
sptr_t util_line_header(eu_tabpage *pnode, const sptr_t start, const sptr_t end, char **pout);
wchar_t* util_wstr_unquote(wchar_t *path, const int size);
char* util_str_unquote(const char *path);
const char* util_trim_left_white(const char *str, int *length);
unsigned long util_compress_bound(unsigned long source_len);
int util_uncompress(uint8_t *dest, unsigned long *dest_len, const uint8_t *source, unsigned long *source_len);
int util_compress(uint8_t *dest, unsigned long *dest_len, const uint8_t *source, unsigned long source_len, int level);
int util_count_number(size_t number);
int util_split(const char *pstr, char (*pout)[MAX_PATH], int ch);
int util_strim_end(char *pstr, int len);
int util_num_cores(void);
void  util_transparent(HWND hwnd, int percent);
void  util_untransparent(HWND hwnd);
void  util_postion_xy(eu_tabpage *pnode, sptr_t pos, sptr_t *px, sptr_t *py);
void  util_explorer_open(eu_tabpage *pnode);
bool  util_product_name(LPCWSTR filepath, LPWSTR out_string, size_t len);
bool  util_file_access(LPCTSTR filename, uint32_t *pgranted);
bool  util_get_unix_file_name(LPCWSTR path, wchar_t *out, const int len);
wchar_t* util_get_nt_file_name(LPCWSTR path);
const uint32_t util_os_version(void);
const TCHAR* util_path_ext(const TCHAR *path);
HFONT util_create_font(const char* name, const int font_size, const bool bold);
sptr_t util_select_characters(eu_tabpage *pnode, const sptr_t start, const sptr_t end);

#ifdef __cplusplus
}
#endif

#endif // _H_SKYLARK_UTIL_
