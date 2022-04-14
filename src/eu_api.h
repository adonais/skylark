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

#ifndef _EU_API_H_
#define _EU_API_H_

#ifndef EU_CLASS_EXPORT
#define EU_CLASS_EXPORT __declspec(dllexport)
#endif

#ifndef EU_CLASS_IMPORT
#define EU_CLASS_IMPORT __declspec(dllimport)
#endif

#if defined(EUAPI_STATIC)
#define EU_EXT_CLASS extern
#elif defined(EUAPI_BUILDDING)
#define EU_EXT_CLASS EU_CLASS_EXPORT
#else
#define EU_EXT_CLASS EU_CLASS_IMPORT
#endif

#if defined(_MSC_VER) && !defined(__clang__)
#pragma intrinsic(memcpy, memset, memcmp, strlen)
#pragma intrinsic(_InterlockedCompareExchange, _InterlockedExchange)
#endif

#ifndef DET_EPSILON
#define DET_EPSILON 0.000001
#endif

#ifndef WM_DPICHANGED_AFTERPARENT
#define WM_DPICHANGED_AFTERPARENT 0x02E3
#endif

#define CHECK_1ST   0.500000
#define CHECK_2ND   0.925000
#define ENV_LEN     512
#define FILESIZE    128
#define ACNAME_LEN  64
#define FT_LEN      32
#define OVECCOUNT   30   // pcre, should be multiple of 3
#ifndef MAX_BUFFER
#define MAX_BUFFER  1024
#endif
#define MAX_ACCELS 200

#if APP_DEBUG
#define EU_ABORT(...) (eu_logmsg(__VA_ARGS__), exit(-1))
#define EU_VERIFY(x) (void)((x) || (EU_ABORT("failed assert(%s): %s:%d\n", #x, __FILE__, __LINE__), 0))
#else
static inline void 
assert_in_release(const char *fmt, const char *exp, const char *file, int line)
{
    char msg[256] = {0};
    _snprintf(msg, 256, fmt, exp, file, line);
    MessageBoxA(NULL, msg, "skylark error:", MB_OK|MB_ICONERROR);
    ExitProcess(255);
}
#define EU_VERIFY(x) (void)((x) || (assert_in_release("failed assert(%s): %s:%d", #x, __FILE__, __LINE__), 0))
#endif
#define eu_int_cast(n) ((int)((size_t)n > INT_MAX ? INT_MAX : n))
#define eu_uint_cast(n) ((uint32_t)((size_t)n > UINT_MAX ? UINT_MAX : n))
#define eu_safe_free(p) ((p) ? ((free((void *)(p))), ((p) = NULL)) : (void *)(p))
#define ONCE_RUN(code)                                      \
{                                                           \
    static volatile long _done = 0;                         \
    if (!_InterlockedCompareExchange(&_done, 1, 0))         \
    {                                                       \
        code;                                               \
        _InterlockedExchange(&_done, 0);                    \
    }                                                       \
}

enum 
{
    SKYLARK_OK = 0,
    EUE_TAB_NULL,
    EUE_POINT_NULL,
    EUE_PATH_NULL,
    EUE_UNEXPECTED_SAVE,
    EUE_ICONV_FAIL,
    EUE_API_CONV_FAIL,
    EUE_CURL_INIT_FAIL,
    EUE_CURL_NETWORK_ERR,
    EUE_LOCAL_FILE_ERR,
    EUE_WRITE_FILE_ERR,
    EUE_OPEN_FILE_ERR,
    EUE_COPY_FILE_ERR,
    EUE_MOVE_FILE_ERR,
    EUE_DELETE_FILE_ERR,
    EUE_EXT_FILTER_ERR,
    EUE_FILE_ATTR_ERR,
    EUE_FILE_SIZE_ERR,
    EUE_LOAD_SCRIPT_ERR,
    EUE_PRESET_FILE_ERR,
    EUE_PARSE_FILE_ERR,
    EUE_MAP_HEX_ERR,
    EUE_NOT_ENOUGH_MEMORY,
    EUE_OUT_OF_MEMORY,
    EUE_API_OPEN_FILE_ERR,
    EUE_API_READ_FILE_ERR,
    EUE_CREATE_MAP_ERR,
    EUE_MAPPING_MEM_ERR,
    EUE_INSERT_TAB_FAIL,
    EUE_WRITE_TAB_FAIL,
    EUE_CHOOSE_FONT_ERR,
    EUE_MENU_STATUS_ERR,
    EUE_OPENSSL_ENC_ERR,
    EUE_OPENSSL_DEC_ERR,
    EUE_UNKOWN_ERR,
};

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <wchar.h>
#include <windows.h>
#include "eu_map.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _doc_data doctype_t;
typedef struct _tabpage eu_tabpage;
typedef struct _file_backup file_backup;

typedef struct _eue_accel
{
    ACCEL  accel_ptr[MAX_ACCELS];
    int    accel_num;
    HACCEL haccel;
}eue_accel;

typedef struct _eue_code
{
    int   nid;
    const char *desc;
}eue_code;

typedef struct _bom_type
{
    int len;
    unsigned char bom[5];
} bom_type;

typedef struct _print_set
{
    int header;
    int footer;
    int color_mode;
    int zoom;
    RECT rect;
}print_set;

struct eu_config
{
    int new_file_eol;
    int new_file_enc;
    
    bool auto_close_chars;
    bool m_ident;
    char window_theme[ACNAME_LEN];
    bool m_fullscreen;
    bool m_menubar;
    bool m_toolbar;
    bool m_statusbar;    
    bool m_linenumber;
    
    bool bookmark_visable;
    int  bookmark_shape;
    uint32_t bookmark_argb;
    bool ws_visiable;
    int ws_size;
    bool newline_visialbe;
    
    bool m_indentation;
    int tab_width;
    bool tab2spaces;
    bool light_fold;
    bool line_mode;
    
    bool m_ftree_show;
    bool m_sym_show;

    int file_tree_width;
    int sym_list_width;
    int sym_tree_width;
    int result_edit_height;
    
    int result_list_height;
    bool block_fold;
    bool m_acshow;
    int acshow_chars;
    
    bool m_ctshow;
    int m_quality;
    int m_render;
    int  m_upfile;
    bool m_light_str;
    bool m_write_copy;
    bool m_session;
    char m_placement[MAX_BUFFER];
    char m_language[ACNAME_LEN];
    print_set eu_print;
    int m_limit;
    uint64_t m_id;
    char m_path[MAX_PATH];
    char m_actions[100][MAX_PATH];
};

struct styleclass
{
    char font[FT_LEN];
    int fontsize;
    uint32_t color;
    uint32_t bgcolor;
    int bold;
};

struct styletheme
{
    struct styleclass linenumber;
    struct styleclass foldmargin;

    struct styleclass text;
    struct styleclass caretline;
    struct styleclass indicator;

    struct styleclass keywords0;
    struct styleclass keywords1;
    struct styleclass string;
    struct styleclass character;
    struct styleclass number;
    struct styleclass operators;
    struct styleclass preprocessor;
    struct styleclass comment;
    struct styleclass commentline;
    struct styleclass commentdoc;

    struct styleclass tags;
    struct styleclass unknowtags;
    struct styleclass attributes;
    struct styleclass unknowattributes;
    struct styleclass entities;
    struct styleclass tagends;
    struct styleclass cdata;
    struct styleclass phpsection;
    struct styleclass aspsection;
};

struct eu_theme
{
    char pathfile[MAX_PATH];
    char name[ACNAME_LEN];
    struct styletheme item;
};

typedef struct _pcre_conainer
{
    const char *buffer;          // 需要匹配的字符串
    size_t buffer_length;        // 字符串长度
    const char *pattern;         // 正则表达式
    pcre *re;                    // 正则表达式的内部表示
    const char *error;           // 错误字符串
    int erroroffset;             // 错误所在移量
    int namecount;               // 命名捕获分组个数
    const char *named_substring; // 命名捕获分组
    int opt;                     // 编译时表达式的功能, 如:PCRE_CASELESS
    int rc;                      // 匹配时的分组数
    int match;                   // 匹配次数
    int ovector[OVECCOUNT];      // 匹配偏移量数组
} pcre_conainer;

typedef int (*ptr_recallback)(pcre_conainer *, void *);
typedef const char* (*ptr_curl_easy_strerror)(CURLcode);
typedef CURL* (*ptr_curl_easy_init)(void);
typedef CURLcode (*ptr_curl_global_init)(long flags);
typedef CURLcode (*ptr_curl_easy_setopt)(CURL*, int, ...);
typedef CURLcode (*ptr_curl_easy_perform)(CURL*);
typedef void (*ptr_curl_easy_cleanup)(CURL*);
typedef void (*ptr_curl_global_cleanup)(void);
typedef void (*ptr_curl_slist_free_all)(struct curl_slist *);
typedef struct curl_slist* (*ptr_curl_slist_append)(struct curl_slist *, const char *);
typedef CURLcode (*ptr_curl_easy_getinfo)(CURL *data, CURLINFO info, ...);
typedef void* (__stdcall *api_convert)(int, const void*, size_t *);
typedef int (*sql3_callback)(void*, int, char**, char**);

// openssl function export
typedef int (*eu_md5_init)(MD5_CTX *c);
typedef int (*eu_md5_update)(MD5_CTX *c, const void *data, size_t len);
typedef int (*eu_md5_final)(unsigned char *md, MD5_CTX *c);
typedef unsigned char* (*eu_md5)(const unsigned char *d, size_t n, unsigned char *md);
typedef int (*eu_sha1_init)(SHA_CTX *c);
typedef int (*eu_sha1_update)(SHA_CTX *c, const void *data, size_t len);
typedef int (*eu_sha1_final)(unsigned char *md, SHA_CTX *c);
typedef unsigned char* (*eu_sha1)(const unsigned char *d, size_t n, unsigned char *md);
typedef int (*eu_sha256_init)(SHA256_CTX *c);
typedef int (*eu_sha256_update)(SHA256_CTX *c, const void *data, size_t len);
typedef int (*eu_sha256_final)(unsigned char *md, SHA256_CTX *c);
typedef unsigned char* (*eu_sha256)(const unsigned char *d, size_t n, unsigned char *md);
typedef unsigned char* (*eu_crypto_sha224)(const unsigned char *, size_t, unsigned char *);
typedef int (*eu_evp_encodeblock)(unsigned char *t, const unsigned char *f, int n);
typedef int (*eu_evp_decodeblock)(unsigned char *t, const unsigned char *f, int n);
typedef int (*eu_aes_set_encrypt_key)(const unsigned char *userKey, const int bits, AES_KEY *key);
typedef int (*eu_aes_set_decrypt_key)(const unsigned char *userKey, const int bits, AES_KEY *key);
typedef void (*eu_aes_cbc_encrypt)(const unsigned char *, unsigned char *, size_t, const AES_KEY *,unsigned char *, const int); 
typedef void (*eu_des_set_key_unchecked)(const_DES_cblock *key, DES_key_schedule *schedule);
typedef void (*eu_des_ecb3_encrypt)(const_DES_cblock *input, DES_cblock *output,
                                    DES_key_schedule *ks1, DES_key_schedule *ks2,
                                    DES_key_schedule *ks3, int enc);
typedef void (*eu_des_ede3_cbc_encrypt)(const unsigned char *input, unsigned char *output,
                                        long length,
                                        DES_key_schedule *ks1, DES_key_schedule *ks2,
                                        DES_key_schedule *ks3, DES_cblock *ivec, int enc);

// eu_sql.c
EU_EXT_CLASS int eu_sqlite3_open(const char *filename, sqlite3 **ppdb);
EU_EXT_CLASS int eu_sqlite3_exec(sqlite3*,  const char *sql, sql3_callback, void *, char **errmsg);
EU_EXT_CLASS int eu_sqlite3_get_table(sqlite3 *db,const char *psql,char ***presult,int *prow,int *pcolumn,char **pzmsg);
EU_EXT_CLASS int eu_sqlite3_close(sqlite3 *);
EU_EXT_CLASS int eu_sqlite3_send(const char *sql, sql3_callback, void *);

EU_EXT_CLASS void eu_sqlite3_free_table(char **result);
EU_EXT_CLASS void eu_push_find_history(const char *key);
EU_EXT_CLASS void eu_delete_find_history(const char *key);
EU_EXT_CLASS void eu_push_replace_history(const char *key);
EU_EXT_CLASS void eu_delete_replace_history(const char *key);
EU_EXT_CLASS void eu_push_folder_history(const char *key);
EU_EXT_CLASS void eu_delete_folder_history(const char *key);
EU_EXT_CLASS void eu_update_backup_table(file_backup *pbak);
EU_EXT_CLASS void eu_clear_backup_table(void);
EU_EXT_CLASS void eu_get_find_history(sql3_callback pfunc);
EU_EXT_CLASS void eu_get_replace_history(sql3_callback pfunc);
EU_EXT_CLASS void eu_get_folder_history(sql3_callback pfunc);

// eu_api.c
EU_EXT_CLASS bool __stdcall eu_exist_path(const char *path);
EU_EXT_CLASS bool __stdcall eu_mk_dir(LPCTSTR dir);
EU_EXT_CLASS bool __stdcall eu_try_path(LPCTSTR dir);
EU_EXT_CLASS bool __stdcall eu_exist_dir(LPCTSTR path);
EU_EXT_CLASS bool __stdcall eu_exist_file(LPCTSTR path);
EU_EXT_CLASS bool __stdcall eu_exist_libcurl(void);
EU_EXT_CLASS bool __stdcall eu_exist_libssl(void);
EU_EXT_CLASS LPTSTR __stdcall eu_suffix_strip(TCHAR *path);
EU_EXT_CLASS LPTSTR __stdcall eu_rand_str(TCHAR *str, const int len);
EU_EXT_CLASS char* __stdcall eu_str_replace(char *in, const size_t in_size, const char *pattern, const char *by);
EU_EXT_CLASS LPTSTR __stdcall eu_wstr_replace(TCHAR *in, size_t in_size, LPCTSTR pattern, LPCTSTR by);

EU_EXT_CLASS char *eu_strcasestr(const char *haystack, const char *needle);
EU_EXT_CLASS const char *eu_query_encoding_name(int code);
EU_EXT_CLASS const uint8_t *eu_memstr(const uint8_t *haystack, const char *needle, size_t size);
EU_EXT_CLASS int eu_sunday(const uint8_t *str, const uint8_t *pattern, size_t n, size_t b, bool incase, bool whole, bool reverse, intptr_t *pret);
EU_EXT_CLASS int eu_sunday_hex(const uint8_t *str, const char *pattern, size_t str_len, bool reverse, intptr_t *pret);

EU_EXT_CLASS int __stdcall eu_try_encoding(uint8_t *, size_t, bool is_file, const TCHAR *);
EU_EXT_CLASS char *__stdcall eu_utf16_utf8(const wchar_t *utf16, size_t *out_len);
EU_EXT_CLASS char *__stdcall eu_utf16_mbcs(int codepage, const wchar_t *utf16, size_t *out_len);
EU_EXT_CLASS wchar_t *__stdcall eu_mbcs_utf16(int codepage, const char *ansi, size_t *out_len);
EU_EXT_CLASS char *__stdcall eu_mbcs_utf8(int codepage, const char *ansi, size_t *out_len);
EU_EXT_CLASS wchar_t *__stdcall eu_utf8_utf16(const char *utf8, size_t *out_len);
EU_EXT_CLASS char *__stdcall eu_utf8_mbcs(int codepage, const char *utf8, size_t *out_len);
EU_EXT_CLASS void __stdcall eu_setpos_window(HWND, HWND, int,int, int, int, uint32_t);
EU_EXT_CLASS bool __stdcall eu_config_ptr(struct eu_config *pconfig);
EU_EXT_CLASS bool __stdcall eu_theme_ptr(struct eu_theme *ptheme, bool init);
EU_EXT_CLASS bool __stdcall eu_accel_ptr(ACCEL *accel);
EU_EXT_CLASS HANDLE __stdcall eu_new_process(LPCTSTR wcmd, LPCTSTR param, LPCTSTR pcd, int flags, uint32_t *o);

EU_EXT_CLASS struct eu_theme *eu_get_theme(void);
EU_EXT_CLASS struct eu_config *eu_get_config(void);
EU_EXT_CLASS eue_accel *eu_get_accel(void);
EU_EXT_CLASS TCHAR *eu_process_path(TCHAR *path, const int len);
EU_EXT_CLASS void eu_save_config(void);
EU_EXT_CLASS void eu_save_theme(void);
EU_EXT_CLASS void eu_free_theme(void);
EU_EXT_CLASS void eu_free_accel(void);
EU_EXT_CLASS bool eu_init_calltip_tree(doctype_t *root, const char *key, const char *val);
EU_EXT_CLASS const char *eu_query_calltip_tree(root_t *root, const char *key);
EU_EXT_CLASS void eu_print_calltip_tree(root_t *root);
EU_EXT_CLASS void eu_destory_calltip_tree(root_t *root);
EU_EXT_CLASS bool eu_init_completed_tree(doctype_t *root, const char *str);
EU_EXT_CLASS void eu_print_completed_tree(root_t *acshow_root);
EU_EXT_CLASS const char *eu_find_completed_tree(root_t *acshow_root, const char *key);
EU_EXT_CLASS void eu_destory_completed_tree(root_t *root);
EU_EXT_CLASS void eu_set_build_id(uint64_t);

// for pcre
EU_EXT_CLASS pcre_conainer *eu_pcre_init(const char *buf, size_t len, const char *pattern, const char *named_substring, int opt);
EU_EXT_CLASS void eu_pcre_delete(pcre_conainer *pcre_info);
EU_EXT_CLASS int eu_pcre_named_substring(const char *named_substring, pcre_conainer *pcre_info, const char **matched_substring);
EU_EXT_CLASS int eu_pcre_exec_single(pcre_conainer *pcre_info, ptr_recallback pback, void *para);
EU_EXT_CLASS int eu_pcre_exec_multi(pcre_conainer *pcre_info, ptr_recallback pback, void *para);

// for scintilla
EU_EXT_CLASS int eu_sci_register(HINSTANCE hinstance);
EU_EXT_CLASS int eu_sci_release(void);
EU_EXT_CLASS sptr_t eu_sci_call(eu_tabpage *p, int m, sptr_t w, sptr_t l);

// for iconv
EU_EXT_CLASS int eu_iconvctl(iconv_t cd, int request, void* argument);
EU_EXT_CLASS iconv_t eu_iconv_open(const char* tocode, const char* fromcode);
EU_EXT_CLASS size_t eu_iconv(iconv_t cd, char** inbuf, size_t *inbytesleft, char** outbuf, size_t *outbytesleft);
EU_EXT_CLASS int eu_iconv_close(iconv_t cd);

// for curl
extern EU_EXT_CLASS ptr_curl_easy_strerror eu_curl_easy_strerror;
extern EU_EXT_CLASS ptr_curl_easy_setopt eu_curl_easy_setopt;
extern EU_EXT_CLASS ptr_curl_easy_perform eu_curl_easy_perform;
extern EU_EXT_CLASS ptr_curl_easy_cleanup eu_curl_easy_cleanup;
extern EU_EXT_CLASS ptr_curl_easy_getinfo eu_curl_easy_getinfo;
extern EU_EXT_CLASS ptr_curl_slist_append eu_curl_slist_append;
extern EU_EXT_CLASS ptr_curl_slist_free_all eu_curl_slist_free_all;

EU_EXT_CLASS int eu_curl_init_global(long flags);
EU_EXT_CLASS CURL* eu_curl_easy_init(void);
EU_EXT_CLASS void eu_curl_global_release(void);

// for eu_changes.c
EU_EXT_CLASS int __stdcall eu_msgbox(HWND hwnd, LPCWSTR text, LPCWSTR title, uint32_t type);

// for openssl
EU_EXT_CLASS HMODULE eu_ssl_open_symbol(char *s[], int n, uintptr_t *pointer);
EU_EXT_CLASS void eu_ssl_close_symbol(HMODULE *pssl);

// for main.c
extern EU_EXT_CLASS TCHAR eu_module_path[MAX_PATH+1];
EU_EXT_CLASS HINSTANCE eu_module_handle(void);

// for eu_proc.h
EU_EXT_CLASS HWND eu_module_hwnd(void);
EU_EXT_CLASS HWND eu_create_main_window(HINSTANCE instance);
EU_EXT_CLASS bool eu_create_toolbar(HWND hwnd);
EU_EXT_CLASS bool eu_create_statusbar(HWND hwnd);
EU_EXT_CLASS bool eu_create_search_dlg(void);
EU_EXT_CLASS void eu_create_fullscreen(HWND hwnd);
EU_EXT_CLASS int eu_before_proc(MSG *p_msg);
EU_EXT_CLASS uint32_t eu_get_dpi(HWND hwnd);

// for eu_registry.c
EU_EXT_CLASS int eu_create_registry_dlg(void);
EU_EXT_CLASS int eu_undo_file_popup(void);
EU_EXT_CLASS int eu_undo_dir_popup(void);
EU_EXT_CLASS int eu_reg_file_popup_menu(void);
EU_EXT_CLASS int eu_reg_dir_popup_menu(void);

// for eu_hook.c
EU_EXT_CLASS bool __stdcall on_hook_exception(void);
EU_EXT_CLASS void __stdcall on_hook_do(void);
EU_EXT_CLASS void __stdcall on_hook_undo(void);

// for eu_share.c
EU_EXT_CLASS void __stdcall share_close_lang(void);
EU_EXT_CLASS LPVOID __stdcall share_map(HANDLE hmap, size_t bytes, uint32_t dw_access);
EU_EXT_CLASS unsigned __stdcall share_send_msg(void *param);
EU_EXT_CLASS HANDLE __stdcall share_load_lang(void);
EU_EXT_CLASS HANDLE __stdcall share_create(HANDLE handle, uint32_t dw_protect, size_t size, LPCTSTR name);
EU_EXT_CLASS bool __stdcall share_unmap(LPVOID memory);
EU_EXT_CLASS void __stdcall share_close(HANDLE handle);
EU_EXT_CLASS HANDLE __stdcall share_open(uint32_t dw_access, LPCTSTR name);
EU_EXT_CLASS bool __stdcall share_envent_create(void);
EU_EXT_CLASS void __stdcall share_envent_set(bool signaled);
EU_EXT_CLASS void __stdcall share_envent_close(void);
EU_EXT_CLASS void __stdcall share_envent_release(void);
EU_EXT_CLASS HANDLE __stdcall share_envent_open_file_sem(void);

// for eu_search.c
EU_EXT_CLASS HWND eu_get_search_hwnd(void);

// for eu_config.c
EU_EXT_CLASS bool __stdcall eu_load_config(HMODULE *pmod);
EU_EXT_CLASS void __stdcall eu_load_file(void);

// for eu_script.c
EU_EXT_CLASS int __stdcall eu_lua_script_convert(const TCHAR *file, const TCHAR *save);
EU_EXT_CLASS int __stdcall eu_lua_script_exec(const TCHAR *fname);
EU_EXT_CLASS bool __stdcall eu_lua_path_setting(void);
EU_EXT_CLASS int luaopen_euapi(void *L);

// for eu_theme.c
EU_EXT_CLASS void eu_font_release(void);

// for eu_theme_dark.c
EU_EXT_CLASS bool eu_on_dark_init(bool fix_scroll, bool dark);
EU_EXT_CLASS void eu_on_dark_release(bool shutdown);

// for eu_remotefs.c
EU_EXT_CLASS void eu_remote_list_release(void);

// for eu_locale.c
EU_EXT_CLASS bool eu_i18n_load_str(uint16_t id, TCHAR *str, int len);

// for eu_util.c
EU_EXT_CLASS void eu_restore_placement(HWND hwnd);

// for eu_doctype.c
EU_EXT_CLASS void eu_doc_config_release(void);

/* 默认的 init_before_ptr 回调函数入口 */
EU_EXT_CLASS int on_doc_init_list(eu_tabpage *pnode);
EU_EXT_CLASS int on_doc_init_tree(eu_tabpage *pnode);
EU_EXT_CLASS int on_doc_init_result(eu_tabpage *pnode);
EU_EXT_CLASS int on_doc_init_result_list(eu_tabpage *pnode);
EU_EXT_CLASS int on_doc_init_list_sh(eu_tabpage *pnode);

/* 默认的 init_after_ptr 回调函数入口 */
EU_EXT_CLASS int on_doc_init_after_cpp(eu_tabpage *pnode);
EU_EXT_CLASS int on_doc_init_after_cs(eu_tabpage *pnode);
EU_EXT_CLASS int on_doc_init_after_java(eu_tabpage *pnode);
EU_EXT_CLASS int on_doc_init_after_go(eu_tabpage *pnode);
EU_EXT_CLASS int on_doc_init_after_swift(eu_tabpage *pnode);
EU_EXT_CLASS int on_doc_init_after_sql(eu_tabpage *pnode);
EU_EXT_CLASS int on_doc_init_after_redis(eu_tabpage *pnode);
EU_EXT_CLASS int on_doc_init_after_python(eu_tabpage *pnode);
EU_EXT_CLASS int on_doc_init_after_lua(eu_tabpage *pnode);
EU_EXT_CLASS int on_doc_init_after_perl(eu_tabpage *pnode);
EU_EXT_CLASS int on_doc_init_after_shell(eu_tabpage *pnode);
EU_EXT_CLASS int on_doc_init_after_rust(eu_tabpage *pnode);
EU_EXT_CLASS int on_doc_init_after_ruby(eu_tabpage *pnode);
EU_EXT_CLASS int on_doc_init_after_lisp(eu_tabpage *pnode);
EU_EXT_CLASS int on_doc_init_after_asm(eu_tabpage *pnode);
EU_EXT_CLASS int on_doc_init_after_cobol(eu_tabpage *pnode);
EU_EXT_CLASS int on_doc_init_after_html(eu_tabpage *pnode);
EU_EXT_CLASS int on_doc_init_after_css(eu_tabpage *pnode);
EU_EXT_CLASS int on_doc_init_after_js(eu_tabpage *pnode);
EU_EXT_CLASS int on_doc_init_after_xml(eu_tabpage *pnode);
EU_EXT_CLASS int on_doc_init_after_json(eu_tabpage *pnode);
EU_EXT_CLASS int on_doc_init_after_yaml(eu_tabpage *pnode);
EU_EXT_CLASS int on_doc_init_after_makefile(eu_tabpage *pnode);
EU_EXT_CLASS int on_doc_init_after_cmake(eu_tabpage *pnode);
EU_EXT_CLASS int on_doc_init_after_markdown(eu_tabpage *pnode);
EU_EXT_CLASS int on_doc_init_after_log(eu_tabpage *pnode);
EU_EXT_CLASS int on_doc_init_after_nim(eu_tabpage *pnode);
EU_EXT_CLASS int on_doc_init_after_shell_sh(eu_tabpage *pnode);
EU_EXT_CLASS int on_doc_init_after_properties(eu_tabpage *pnode);
EU_EXT_CLASS int on_doc_init_after_diff(eu_tabpage *pnode);

/* 默认的 key_ptr 回调函数入口 */
EU_EXT_CLASS int on_doc_keydown_jmp(eu_tabpage *pnode, WPARAM wParam, LPARAM lParam);
EU_EXT_CLASS int on_doc_keydown_sql(eu_tabpage *pnode, WPARAM wParam, LPARAM lParam);
EU_EXT_CLASS int on_doc_keydown_redis(eu_tabpage *pnode, WPARAM wParam, LPARAM lParam);
EU_EXT_CLASS int on_doc_keyup_general(eu_tabpage *pnode, WPARAM wParam, LPARAM lParam);
EU_EXT_CLASS int on_doc_keyup_general_sh(eu_tabpage *, WPARAM, LPARAM);

/* 默认的 add_ptr 回调函数入口 */
EU_EXT_CLASS int on_doc_identation(eu_tabpage *pnode, SCNotification *lpnotify);
EU_EXT_CLASS int on_doc_cpp_like(eu_tabpage *pnode, SCNotification *lpnotify);
EU_EXT_CLASS int on_doc_sql_like(eu_tabpage *pnode, SCNotification *lpnotify);
EU_EXT_CLASS int on_doc_redis_like(eu_tabpage *pnode, SCNotification *lpnotify);
EU_EXT_CLASS int on_doc_html_like(eu_tabpage *pnode, SCNotification *lpnotify);
EU_EXT_CLASS int on_doc_xml_like(eu_tabpage *pnode, SCNotification *lpnotify);
EU_EXT_CLASS int on_doc_css_like(eu_tabpage *pnode, SCNotification *lpnotify);
EU_EXT_CLASS int on_doc_json_like(eu_tabpage *pnode, SCNotification *lpnotify);
EU_EXT_CLASS int on_doc_makefile_like(eu_tabpage *pnode, SCNotification *lpnotify);
EU_EXT_CLASS int on_doc_cmake_like(eu_tabpage *pnode, SCNotification *lpnotify);
EU_EXT_CLASS int on_doc_markdown_like(eu_tabpage *pnode, SCNotification *lpnotify);

/* 默认的 reload_list_ptr,reload_tree_ptr  回调函数入口 */
EU_EXT_CLASS int on_doc_reload_list_reqular(eu_tabpage *pnode);
EU_EXT_CLASS int on_doc_reload_list_sh(eu_tabpage *pnode);
EU_EXT_CLASS int on_doc_reload_tree_xml(eu_tabpage *pnode);
EU_EXT_CLASS int on_doc_reload_tree_json(eu_tabpage *pnode);
EU_EXT_CLASS int on_doc_reload_tree_sql(eu_tabpage *pnode);
EU_EXT_CLASS int on_doc_reload_tree_redis(eu_tabpage *pnode);

EU_EXT_CLASS int on_doc_click_list_jmp(eu_tabpage *pnode);
EU_EXT_CLASS int on_doc_click_list_jump_sh(eu_tabpage *pnode);
EU_EXT_CLASS int on_doc_click_tree_sql(eu_tabpage *pnode);
EU_EXT_CLASS int on_doc_click_tree_json(eu_tabpage *pnode);
EU_EXT_CLASS int on_doc_click_tree_redis(eu_tabpage *pnode);

/* 脚本调用 */
EU_EXT_CLASS int on_doc_enable_scilexer(eu_tabpage *pnode, int lex);
EU_EXT_CLASS void on_doc_default_light(eu_tabpage *pnode, int lex, int64_t rgb);
EU_EXT_CLASS void on_doc_keyword_light(eu_tabpage *pnode, int lex, int index, int64_t rgb);
EU_EXT_CLASS void on_doc_function_light(eu_tabpage *pnode, int lex, int index, int64_t rgb);
EU_EXT_CLASS void on_doc_preprocessor_light(eu_tabpage *pnode, int lex, int index, int64_t rgb);
EU_EXT_CLASS void on_doc_marcro_light(eu_tabpage *pnode, int lex, int index, int64_t rgb);
EU_EXT_CLASS void on_doc_variable_light(eu_tabpage *pnode, int lex, int64_t rgb);
EU_EXT_CLASS void on_doc_string_light(eu_tabpage *pnode, int lex, int64_t rgb);
EU_EXT_CLASS void on_doc_operator_light(eu_tabpage *pnode, int lex, int64_t rgb);
EU_EXT_CLASS void on_doc_char_light(eu_tabpage *pnode, int lex, int64_t rgb);
EU_EXT_CLASS void on_doc_number_light(eu_tabpage *pnode, int lex, int64_t rgb);
EU_EXT_CLASS void on_doc_special_light(eu_tabpage *pnode, int lex, int64_t rgb);
EU_EXT_CLASS void on_doc_send_light(eu_tabpage *pnode, int lex, int index, int64_t rgb);
EU_EXT_CLASS void on_doc_tags_light(eu_tabpage *pnode, int lex, int64_t rgb);
EU_EXT_CLASS void on_doc_comment_light(eu_tabpage *pnode, int lex, int64_t rgb);
EU_EXT_CLASS void on_doc_commentblock_light(eu_tabpage *pnode, int lex, int64_t rgb);
EU_EXT_CLASS void on_doc_commentdoc_light(eu_tabpage *pnode, int lex, int64_t rgb);

#ifdef __cplusplus
}
#endif

#endif  // _EU_API_H_
