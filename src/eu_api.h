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

// clang supports MS pragma intrinsic
#if defined(_MSC_VER)
#pragma intrinsic(memcpy, memset, memcmp, strlen)
#pragma intrinsic(_InterlockedIncrement,_InterlockedDecrement)
#pragma intrinsic(_InterlockedCompareExchange64,_InterlockedExchange64)
#pragma intrinsic(_InterlockedCompareExchange,_InterlockedExchange,_InterlockedExchangeAdd)
#endif

#if defined(_WIN64) || defined(_M_X64)
#define inter_atom_exchange _InterlockedExchange64
#define inter_atom_compare_exchange _InterlockedCompareExchange64
#else
#define inter_atom_exchange _InterlockedExchange
#define inter_atom_compare_exchange _InterlockedCompareExchange
#endif

#if defined(__cplusplus)
#define FALLTHRU_ATTR		[[fallthrough]]
#elif (defined(__GNUC__) && __GNUC__ >= 7) || (defined(__clang__) && __clang_major__ >= 10)
#define FALLTHRU_ATTR		__attribute__((fallthrough))
#else
#define FALLTHRU_ATTR
#endif

#ifndef DET_EPSILON
#define DET_EPSILON 0.000001
#endif

#ifndef WM_DPICHANGED_AFTERPARENT
#define WM_DPICHANGED_AFTERPARENT 0x02E3
#endif

#define CHECK_1ST   0.500000
#define CHECK_2ND   0.925000

#ifndef OVECCOUNT
#define OVECCOUNT   30   // pcre, should be multiple of 3
#endif

#define OVEC_LEN    16
#define FT_LEN      32
#define ACNAME_LEN  64
#define FILESIZE    128
#define MAX_SIZE    256
#define ENV_LEN     512

#define SNIPPET_FUNID 100

#ifndef MAX_BUFFER
#define MAX_BUFFER  1024
#endif
#ifndef LARGER_LEN
#define LARGER_LEN 2048
#endif
#ifndef VALUE_LEN
#define VALUE_LEN 4096
#endif

#define MAX_ACCELS 200
#define BUFF_64K 0x10000
#define BUFF_200M 0xc800000

#ifndef WM_COPYGLOBALDATA
#define WM_COPYGLOBALDATA (0x0049)
#endif

#define REMOTEFS_PROTOCOL_SUBID 0x38
#define REMOTEFS_ACCESS_SUBID 0x39
#define SEARCH_COMBO_SUBID 0x40
#define SNIPPET_EDT_SUBID 0x41
#define SNIPPET_CMB_SUBID 0x42
#define TBCTL_LIST_SUBID 0x43
#define STATUSBAR_SUBID 0x44

// Custom message
#define HVM_SETEXTENDEDSTYLE      (WM_USER + 100)
#define HVM_SETITEMCOUNT          (WM_USER + 101)
#define HVM_GETSEL                (WM_USER + 102)
#define HVM_SETSEL                (WM_USER + 103)
#define HVM_SETTEXTCOLOR          (WM_USER + 104)
#define HVM_SETBKCOLOR            (WM_USER + 105)
#define HVM_SETSELBKCOLOR         (WM_USER + 106)
#define HVM_SETMODIFIEDCOLOR      (WM_USER + 107)
#define HVM_GOPOS                 (WM_USER + 108)
#define HVM_GETHEXADDR            (WM_USER + 109)
#define HVM_SETLINECOUNT          (WM_USER + 110)
#define HVN_GETDISPINFO           (WMN_FIRST - 0)
#define HVN_ITEMCHANGING          (WMN_FIRST - 1)
#define WM_BTN_PRESS              (WM_USER + 201)
#define WM_FILEOPEN               (WM_USER + 301)
#define TVI_LOADREMOTE            (WM_USER + 401)
#define DOCUMENTMAP_SCROLL        (WM_USER + 501)
#define DOCUMENTMAP_MOUSECLICKED  (WM_USER + 502)
#define DOCUMENTMAP_MOUSEWHEEL    (WM_USER + 503)
#define WM_ABOUT_STC              (WM_USER + 600)
#define WM_BACKUP_OVER            (WM_USER+10001)
#define WM_SYSLIST_OVER           (WM_USER+10002)
#define WM_STATUS_REFRESH         (WM_USER+10003)
#define WM_TAB_CLICK              (WM_USER+10004)
#define WM_SKYLARK_DESC           (WM_USER+10005)
// Tab notification message
#define TCN_TABDROPPED_OUT        (WM_USER+20000)

#if APP_DEBUG
#define EU_ABORT(...) (eu_logmsg(__VA_ARGS__), exit(-1))
#define EU_VERIFY(x) (void)((x) || (EU_ABORT("failed assert(%s): %s:%d\n", #x, __FILE__, __LINE__), 0))
#else
static inline void assert_in_release(const char *fmt, const char *exp, const char *file, int line)
{
    char msg[256] = {0};
    _snprintf(msg, 256, fmt, exp, file, line);
    MessageBoxA(NULL, msg, "skylark error:", MB_OK|MB_ICONERROR);
    ExitProcess(255);
}
#define EU_VERIFY(x) (void)((x) || (assert_in_release("failed assert(%s): %s:%d", #x, __FILE__, __LINE__), 0))
#endif
static inline int eu_cvector_at(int *v, int n)
{
    for (int i = 0; i < cvector_size(v); ++i)
    {
        if (n == v[i])
        {
            return i;
        }
    }
    return -1;
}
#define MEM_RESERVED ((char *)(uintptr_t)0x200)
#define eu_int_cast(n) ((int)((intptr_t)(n)))
#define eu_uint_cast(n) ((uint32_t)((size_t)(n)))
#define eu_safe_free(p) ((p) ? ((free((void *)(p))), ((p) = NULL)) : (void *)(p))
#define eu_close_file(p) ((p) ? ((fclose((FILE *)(p))), ((p) = NULL)) : (void *)(p))
#define eu_close_console(h) eu_close_file(h), FreeConsole()
#define eu_close_handle(h)                      \
    if (NULL != h && INVALID_HANDLE_VALUE != h) \
    {                                           \
        CloseHandle(h);                         \
    }                                           \
    h = (void *)MEM_RESERVED
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
    EUE_PCRE_BACK_ABORT   = -40,
    EUE_PCRE_NO_MATCHING  = -39,
    EUE_PCRE_EXP_ERR      = -38,
    EUE_POINT_NULL        = -37,
    EUE_PATH_NULL         = -36,
    EUE_UNEXPECTED_SAVE   = -35,
    EUE_ICONV_FAIL        = -34,
    EUE_API_CONV_FAIL     = -33,
    EUE_CURL_INIT_FAIL    = -32,
    EUE_CURL_NETWORK_ERR  = -31,
    EUE_LOCAL_FILE_ERR    = -30,
    EUE_WRITE_FILE_ERR    = -29,
    EUE_OPEN_FILE_ERR     = -28,
    EUE_COPY_FILE_ERR     = -27,
    EUE_MOVE_FILE_ERR     = -26,
    EUE_DELETE_FILE_ERR   = -25,
    EUE_EXT_FILTER_ERR    = -24,
    EUE_FILE_ATTR_ERR     = -23,
    EUE_FILE_SIZE_ERR     = -22,
    EUE_LOAD_SCRIPT_ERR   = -21,
    EUE_PARSE_FILE_ERR    = -20,
    EUE_MAP_HEX_ERR       = -19,
    EUE_NOT_ENOUGH_MEMORY = -18,
    EUE_API_OPEN_FILE_ERR = -17,
    EUE_API_READ_FILE_ERR = -16,
    EUE_CREATE_MAP_ERR    = -15,
    EUE_MAPPING_MEM_ERR   = -14,
    EUE_CHOOSE_FONT_ERR   = -13,
    EUE_MENU_STATUS_ERR   = -12,
    EUE_OPENSSL_ENC_ERR   = -11,
    EUE_OPENSSL_DEC_ERR   = -10,
    EUE_UNKOWN_ERR        = -9,
    EUE_TAB_NULL          = -8,
    EUE_WRITE_TAB_FAIL    = -7,
    EUE_INSERT_TAB_FAIL   = -6,
    EUE_PRESET_FILE_ERR   = -5,
    EUE_OUT_OF_MEMORY     = -4,
    SKYLARK_TABCTRL_ERR   = -3,
    SKYLARK_NOT_OPENED    = -2,
    SKYLARK_OPENED        = -1,
    SKYLARK_OK            = 0,
    SKYLARK_SQL_END       = 200
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
typedef SCNotification *ptr_notify;

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

typedef struct _caret_set
{
    int blink;
    int width;
    uint32_t rgb;
}caret_set;

typedef struct _bookmark_set
{
    bool visable;
    int  shape;
    uint32_t argb;
}bookmark_set;

typedef struct _brace_set
{
    bool matching;
    bool autoc;
    uint32_t rgb;
}brace_set;

typedef struct _calltip_set
{
    bool enable;
    uint32_t rgb;
}calltip_set;

typedef struct _complete_set
{
    bool enable;
    int  characters;
    int  snippet;
}complete_set;

struct eu_config
{
    int new_file_eol;
    int new_file_enc;

    bool m_ident;
    char window_theme[ACNAME_LEN];
    bool m_fullscreen;
    bool m_menubar;
    bool m_toolbar;
    bool m_statusbar;
    bool m_linenumber;

    uint32_t last_flags;
    bool ws_visiable;
    int ws_size;
    bool newline_visialbe;

    bool m_indentation;
    int tab_width;
    bool tab2spaces;
    bool light_fold;
    bool line_mode;
    bool m_ftree_show;

    int file_tree_width;
    int sym_list_width;
    int sym_tree_width;
    int sidebar_width;
    int document_map_width;
    int result_edit_height;
    int result_list_height;
    int file_recent_number;
    int inter_reserved_0;
    int inter_reserved_1;
    int inter_reserved_2;

    bool block_fold;
    bool m_tab_tip;

    int m_close_way;
    int m_tab_active;
    int m_quality;
    int m_render;
    int  m_upfile;
    bool m_light_str;
    bool m_write_copy;
    bool m_session;
    bool m_exit;
    bool m_instance;
    char m_placement[MAX_BUFFER];
    char m_language[ACNAME_LEN];
    bookmark_set eu_bookmark;
    brace_set eu_brace;
    caret_set eu_caret;
    calltip_set eu_calltip;
    complete_set eu_complete;
    print_set eu_print;
    int m_limit;
    uint64_t m_id;
    char m_path[MAX_PATH];
    char editor[MAX_PATH];
    char m_reserved_0[MAX_PATH];
    char m_reserved_1[MAX_PATH];
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
    
    struct styleclass activetab;
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
EU_EXT_CLASS void eu_sqlite3_free_table(char **result);
EU_EXT_CLASS void eu_sqlite3_free(void *point);
EU_EXT_CLASS int eu_sqlite3_close(sqlite3 *);
EU_EXT_CLASS int eu_sqlite3_send(const char *sql, sql3_callback, void *);
EU_EXT_CLASS void eu_push_find_history(const char *key);
EU_EXT_CLASS void eu_delete_find_history(const char *key);
EU_EXT_CLASS void eu_push_replace_history(const char *key);
EU_EXT_CLASS void eu_delete_replace_history(const char *key);
EU_EXT_CLASS void eu_push_folder_history(const char *key);
EU_EXT_CLASS void eu_delete_folder_history(const char *key);
EU_EXT_CLASS void eu_update_backup_table(file_backup *pbak, int mode);
EU_EXT_CLASS void eu_clear_backup_table(void);
EU_EXT_CLASS void eu_get_find_history(sql3_callback pfunc);
EU_EXT_CLASS void eu_get_replace_history(sql3_callback pfunc);
EU_EXT_CLASS void eu_get_folder_history(sql3_callback pfunc);

// eu_api.c
EU_EXT_CLASS bool __stdcall eu_touch(LPCTSTR path);
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
EU_EXT_CLASS char *eu_find_completed_tree(root_t *acshow_root, const char *key, const char *pre_str);
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
EU_EXT_CLASS void eu_send_notify(HWND hwnd, uint32_t code, LPNMHDR nmhdr);

// for iconv
EU_EXT_CLASS int eu_iconvctl(iconv_t cd, int request, void* argument);
EU_EXT_CLASS iconv_t eu_iconv_open(const char* tocode, const char* fromcode);
EU_EXT_CLASS size_t eu_iconv(iconv_t cd, char** inbuf, size_t *inbytesleft, char** outbuf, size_t *outbytesleft);
EU_EXT_CLASS int eu_iconv_close(iconv_t cd);

// for curl
extern EU_EXT_CLASS ptr_curl_easy_strerror eu_curl_easy_strerror;
extern EU_EXT_CLASS ptr_curl_easy_setopt eu_curl_easy_setopt;
extern EU_EXT_CLASS ptr_curl_easy_perform eu_curl_easy_perform;
extern EU_EXT_CLASS ptr_curl_easy_getinfo eu_curl_easy_getinfo;
extern EU_EXT_CLASS ptr_curl_slist_append eu_curl_slist_append;
extern EU_EXT_CLASS ptr_curl_slist_free_all eu_curl_slist_free_all;

EU_EXT_CLASS int eu_curl_init_global(long flags);
EU_EXT_CLASS CURL* eu_curl_easy_init(void);
EU_EXT_CLASS void eu_curl_global_release(void);
EU_EXT_CLASS void eu_curl_easy_cleanup(CURL *);

// for eu_changes.c
EU_EXT_CLASS int __stdcall eu_msgbox(HWND hwnd, LPCWSTR text, LPCWSTR title, uint32_t type);

// for openssl
EU_EXT_CLASS HMODULE eu_ssl_open_symbol(char *s[], int n, uintptr_t *pointer);
EU_EXT_CLASS void eu_ssl_close_symbol(HMODULE *pssl);

// for main.c
extern EU_EXT_CLASS TCHAR eu_module_path[MAX_PATH+1];
EU_EXT_CLASS HINSTANCE eu_module_handle(void);

// for eu_about.c
EU_EXT_CLASS void __stdcall eu_about_command(void);

// for eu_proc.h
EU_EXT_CLASS HWND eu_module_hwnd(void);
EU_EXT_CLASS void eu_close_edit(void);
EU_EXT_CLASS HWND eu_create_main_window(HINSTANCE instance);
EU_EXT_CLASS bool eu_create_toolbar(HWND hwnd);
EU_EXT_CLASS bool eu_create_statusbar(HWND hwnd);
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
EU_EXT_CLASS void __stdcall share_unmap(LPVOID memory);
EU_EXT_CLASS void __stdcall share_close(HANDLE handle);
EU_EXT_CLASS HANDLE __stdcall share_open(uint32_t dw_access, LPCTSTR name);
EU_EXT_CLASS bool __stdcall share_envent_create(void);
EU_EXT_CLASS void __stdcall share_envent_set(bool signaled);
EU_EXT_CLASS void __stdcall share_envent_close(void);
EU_EXT_CLASS void __stdcall share_envent_release(void);
EU_EXT_CLASS HANDLE __stdcall share_envent_open_file_sem(void);

// for eu_search.c
EU_EXT_CLASS HWND eu_get_search_hwnd(void);

// for eu_resultctl.c
EU_EXT_CLASS HWND eu_result_hwnd(void);

// for eu_snippet.c
EU_EXT_CLASS HWND __stdcall eu_snippet_hwnd(void);

// for eu_config.c
EU_EXT_CLASS bool __stdcall eu_load_main_config(void);
EU_EXT_CLASS bool __stdcall eu_load_config(void);
EU_EXT_CLASS bool __stdcall eu_check_arg(const wchar_t **args, int argc, const wchar_t *);
EU_EXT_CLASS void __stdcall eu_load_file(void);
EU_EXT_CLASS void __stdcall eu_postion_setup(wchar_t **args, int argc, file_backup *pbak);
EU_EXT_CLASS bool __stdcall eu_config_parser_path(wchar_t **args, int argc, wchar_t *path);

// for eu_script.c
EU_EXT_CLASS int __stdcall eu_lua_script_convert(const TCHAR *file, const TCHAR *save);
EU_EXT_CLASS int __stdcall eu_lua_script_exec(const TCHAR *fname);
EU_EXT_CLASS bool __stdcall eu_lua_path_setting(eu_tabpage *pnode);
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
EU_EXT_CLASS void eu_doc_ptr_free(void);
EU_EXT_CLASS void on_doc_enable_foldline(eu_tabpage *pnode);

// for eu_tablectl.c
EU_EXT_CLASS void eu_close_db_handle(void);

/* 默认的 init_before_ptr 回调函数入口 */
EU_EXT_CLASS int on_doc_init_list(eu_tabpage *pnode);
EU_EXT_CLASS int on_doc_init_tree(eu_tabpage *pnode);
EU_EXT_CLASS int on_doc_init_result(eu_tabpage *pnode);
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
EU_EXT_CLASS int on_doc_identation(eu_tabpage *pnode, ptr_notify lpnotify);
EU_EXT_CLASS int on_doc_cpp_like(eu_tabpage *pnode, ptr_notify lpnotify);
EU_EXT_CLASS int on_doc_sql_like(eu_tabpage *pnode, ptr_notify lpnotify);
EU_EXT_CLASS int on_doc_redis_like(eu_tabpage *pnode, ptr_notify lpnotify);
EU_EXT_CLASS int on_doc_html_like(eu_tabpage *pnode, ptr_notify lpnotify);
EU_EXT_CLASS int on_doc_xml_like(eu_tabpage *pnode, ptr_notify lpnotify);
EU_EXT_CLASS int on_doc_css_like(eu_tabpage *pnode, ptr_notify lpnotify);
EU_EXT_CLASS int on_doc_json_like(eu_tabpage *pnode, ptr_notify lpnotify);
EU_EXT_CLASS int on_doc_makefile_like(eu_tabpage *pnode, ptr_notify lpnotify);
EU_EXT_CLASS int on_doc_cmake_like(eu_tabpage *pnode, ptr_notify lpnotify);

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
EU_EXT_CLASS int on_doc_init_after_scilexer(eu_tabpage *pnode, const  char *name);
EU_EXT_CLASS void on_doc_default_light(eu_tabpage *pnode, int lex, int64_t fg_rgb, int64_t bk_rgb, bool force);
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
