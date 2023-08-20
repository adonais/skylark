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

#ifdef _MSC_VER
#include <io.h>
#else
#include <sys/types.h>
#include <unistd.h>
#endif

#if defined(_MSC_VER) && defined(_WIN64)
#pragma comment(linker, "/include:luaL_openlibs")
#endif

#include "framework.h"
#include "eu_encoding_utf8.h"

#define CHAR_IN_RANGE(c, lower, upper) (((c) >= (lower)) && ((c) <= (upper)))
#define CHAR_IS_LATIN(c) ((c) <= 0x7F)
#define IS_1LST_BYTE(c) (CHAR_IN_RANGE((c), 0x81, 0xFE))
#define IS_2NDBYTE_16(c) (CHAR_IN_RANGE((c), 0x40, 0xFE) && (c) != 0x7F)
#define IS_2NDBYTE_32(c) (CHAR_IN_RANGE((c), 0x30, 0x39))
#define IS_2ND_BYTE(c) (IS_2NDBYTE_16(c) || IS_2NDBYTE_32(c))
#define IS_3RD_BYTE(c) (CHAR_IN_RANGE((c), 0x81, 0xFE))
#define IS_4TH_BYTE(c) (CHAR_IN_RANGE((c), 0x30, 0x39))
#define EU_RESET_BACKUP_NAME _T("conf_old_backup")

wchar_t eu_module_path[MAX_PATH+1] = {0};
wchar_t eu_config_path[MAX_BUFFER] = {0};
static volatile long eu_curl_initialized;
static HINSTANCE eu_instance;   // 当前实例
static uint32_t fn_config_mask = 0x0;

/* generic implementation */
#define FOREACH(node, collection)                      \
    for (node  = (collection)->first(collection);      \
         node != (collection)->last(collection);       \
         node  = (collection)->next(collection, node))

#define EXPAND_STYLETHEME(memb)              \
    g_theme->item.memb.font,                 \
    g_theme->item.memb.fontsize,             \
    g_theme->item.memb.color,                \
    g_theme->item.memb.bgcolor,              \
    g_theme->item.memb.bold

enum
{
    EN_CODEING_NONE = -1,
    EN_CODEING_BINARY = 0,
    EN_CODEING_UTF8BOM,
    UTF16_LE_NOBOM,
    UTF16_LE_BOM,
    UTF16_BE_NOBOM,
    UTF16_BE_BOM,
    UTF32_LE_BOM,
    UTF32_BE_BOM
};

static HMODULE eu_curl_symbol;
static ptr_curl_easy_init fn_curl_easy_init;
static ptr_curl_global_init fn_curl_global_init;
static ptr_curl_global_cleanup fn_curl_global_cleanup;
static ptr_curl_easy_cleanup fn_curl_easy_cleanup;

ptr_curl_easy_setopt eu_curl_easy_setopt = NULL;
ptr_curl_easy_perform eu_curl_easy_perform = NULL;
ptr_curl_slist_append eu_curl_slist_append = NULL;
ptr_curl_slist_free_all eu_curl_slist_free_all = NULL;
ptr_curl_easy_getinfo eu_curl_easy_getinfo = NULL;
ptr_curl_easy_strerror eu_curl_easy_strerror = NULL;

static struct eu_config *g_config;
static struct eu_theme  *g_theme;
static eue_accel *g_accel;
static eue_toolbar *g_toolbar;
static eue_code eue_coding[] =
{
    {IDM_UNI_UTF8    , "UTF-8"}            ,
    {IDM_UNI_UTF8B   , "UTF-8(BOM)"}       ,
    {IDM_UNI_UTF16LE , "UTF-16LE"}         ,
    {IDM_UNI_UTF16LEB, "UTF-16LE"}         ,
    {IDM_UNI_UTF16BE , "UTF-16BE"}         ,
    {IDM_UNI_UTF16BEB, "UTF-16BE"}         ,
    {IDM_UNI_UTF32LE , "UTF-32LE"}         ,
    {IDM_UNI_UTF32BE , "UTF-32BE"}         ,
    {IDM_UNI_ASCII   , "C99"}              ,
    {IDM_ANSI_1      , "windows-1250"}     ,
    {IDM_ANSI_2      , "windows-1251"}     ,
    {IDM_ANSI_3      , "windows-1252"}     ,
    {IDM_ANSI_4      , "windows-1253"}     ,
    {IDM_ANSI_5      , "windows-1254"}     ,
    {IDM_ANSI_6      , "windows-1255"}     ,
    {IDM_ANSI_7      , "windows-1256"}     ,
    {IDM_ANSI_8      , "windows-1257"}     ,
    {IDM_ANSI_9      , "windows-1258"}     ,
    {IDM_ANSI_10     , "tis-620"}          ,
    {IDM_ANSI_11     , "cp932"}            ,
    {IDM_ANSI_12     , "gb18030"}          ,
    {IDM_ANSI_13     , "euc-kr"}           ,
    {IDM_ANSI_14     , "BIG5"}             ,
    {IDM_ISO_1       , "iso-8859-1"}       ,
    {IDM_ISO_2       , "iso-8859-2"}       ,
    {IDM_ISO_3       , "iso-8859-3"}       ,
    {IDM_ISO_4       , "iso-8859-4"}       ,
    {IDM_ISO_5       , "iso-8859-5"}       ,
    {IDM_ISO_6       , "ISO-8859-6"}       ,
    {IDM_ISO_7       , "iso-8859-7"}       ,
    {IDM_ISO_8       , "iso-8859-8"}       ,
    {IDM_ISO_9       , "iso-8859-9"}       ,
    {IDM_ISO_10      , "iso-8859-10"}      ,
    {IDM_ISO_11      , "iso-8859-11"}      ,
    {IDM_ISO_13      , "iso-8859-13"}      ,
    {IDM_ISO_15      , "iso-8859-15"}      ,
    {IDM_ISO_16      , "iso-8859-16"}      ,
    {IDM_ISO_KR      , "iso-2022-kr"}      ,
    {IDM_ISO_CN      , "iso-2022-cn-ext"}  ,
    {IDM_ISO_JP_2    , "iso-2022-jp-2"}    ,
    {IDM_ISO_JP_2004 , "iso-2022-jp-2004"} ,
    {IDM_ISO_JP_MS   , "iso-2022-jp-ms"}   ,
    {IDM_IBM_1       , "ibm852"}           ,
    {IDM_IBM_2       , "ibm855"}           ,
    {IDM_IBM_3       , "ibm866"}           ,
    {IDM_EUC_1       , "euc-jp"}           ,
    {IDM_EUC_2       , "euc-tw"}           ,
    {IDM_OTHER_HZ    , "HZ-GB-2312"}       ,
    {IDM_OTHER_1     , "KOI8-R"}           ,
    {IDM_OTHER_2     , "MACCYRILLIC"}      ,
    {IDM_OTHER_3     , "MACCENTRALEUROPE"} ,
    {IDM_OTHER_ANSI  , "ANSI"}             ,
    {IDM_OTHER_BIN   , "Binary encoding"}  ,
    {IDM_UNKNOWN     , "Unknown encoding"} ,
    {0               , NULL}
};

static bool
eu_exist_wpath(const wchar_t *path)
{
    return (GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES);
}

/** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * 产生一个长度为len的伪随机数字符串
 */
LPTSTR
eu_rand_str(TCHAR *str, const int len)
{
    srand((uint32_t)time(0) + GetCurrentProcessId());
    for (int i = 0; i < len; ++i)
    {
        str[i] = _T('A') + rand() % 26;
    }
    str[len] = _T('\0');
    return str;
}

/** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * c风格的字符串替换函数
 * 直接在输入的字符串内做替换, 所以务必保证in_size有足够的空间
 * 在这里, in_size最大值为4096, 超出这个值将导致字符串被截断
 ** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
LPTSTR
eu_wstr_replace(TCHAR *in, const size_t in_size, LPCTSTR pattern, LPCTSTR by)
{
    TCHAR *in_ptr = in;
    TCHAR res[VALUE_LEN] = { 0 };
    size_t offset = 0;
    TCHAR *needle;
    while ((needle = _tcsstr(in, pattern)) && offset < in_size && offset < VALUE_LEN)
    {
        _tcsncpy(res + offset, in, needle - in);
        offset += needle - in;
        in = needle + (int) _tcslen(pattern);
        _tcsncpy(res + offset, by, VALUE_LEN - offset);
        offset += (int) _tcslen(by);
    }
    _tcsncpy(res + offset, in, VALUE_LEN - offset);
    _sntprintf(in_ptr, eu_int_cast(in_size), _T("%s"), res);
    in = in_ptr;
    return in;
}

char*
eu_str_replace(char *in, const size_t in_size, const char *pattern, const char *by)
{
    char *in_ptr = in;
    char res[VALUE_LEN] = {0};
    size_t offset = 0;
    char *needle;
    while ((needle = strstr(in, pattern)) && offset < in_size && offset < VALUE_LEN)
    {
        strncpy(res + offset, in, needle - in);
        offset += needle - in;
        in = needle + (int) strlen(pattern);
        strncpy(res + offset, by, VALUE_LEN - offset);
        offset += (int) strlen(by);
    }
    strncpy(res + offset, in, VALUE_LEN - offset);
    _snprintf(in_ptr, eu_int_cast(in_size), "%s", res);
    in = in_ptr;
    return in;
}

TCHAR*
eu_suffix_strip(TCHAR *path)
{
    TCHAR mark = 0;
    int len = (int) _tcslen(path);
    if (path[len - 1] == _T('"'))
    {
        mark = _T('"');
    }
    for (int i = len; i > 0; --i)
    {
        if (path[i] == _T('\\') || path[i] == _T('/'))
        {
            if (mark)
            {
                path[i] = mark;
                path[i+1] = _T('\0');
            }
            else
            {
                path[i] = _T('\0');
            }
            break;
        }
    }
    return path;
}

bool
eu_exist_path(const char *path)
{
    TCHAR wide_dir[MAX_BUFFER] = {0};
    int m = MultiByteToWideChar(CP_UTF8, 0, path, -1, wide_dir, MAX_BUFFER);
    if (m > 0 && m < MAX_BUFFER)
    {
        return eu_exist_wpath(wide_dir);
    }
    return false;
}

bool
eu_exist_dir(LPCTSTR path)
{
    DWORD fileattr = GetFileAttributes(path);
    if (fileattr != INVALID_FILE_ATTRIBUTES)
    {
        return (fileattr & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }
    return false;
}

bool
eu_exist_file(LPCTSTR path)
{
    bool ret = false;
    uint32_t fileattr = INVALID_FILE_ATTRIBUTES;
    if (STR_IS_NUL(path))
    {
        return false;
    }
    if (_tcslen(path) > 1 && path[1] == L':')
    {
        fileattr = GetFileAttributes(path);
    }
    else
    {
        TCHAR file_path[MAX_BUFFER] = {0};
        _sntprintf(file_path, MAX_BUFFER, _T("%s\\%s"), eu_module_path, path);
        fileattr = GetFileAttributes(file_path);
    }
    if (fileattr != INVALID_FILE_ATTRIBUTES)
    {
        ret = (fileattr & FILE_ATTRIBUTE_DIRECTORY) == 0;
    }
    return ret;
}

bool
eu_exist_libssl(void)
{
    TCHAR ssl_path[MAX_BUFFER] = {0};
#ifdef _WIN64
    _sntprintf(ssl_path, MAX_BUFFER, _T("%s\\plugins\\%s"), eu_module_path, _T("libcrypto-1_1-x64.dll"));
#else
    _sntprintf(ssl_path, MAX_BUFFER, _T("%s\\plugins\\%s"), eu_module_path, _T("libcrypto-1_1.dll"));
#endif
    return eu_exist_file(ssl_path);
}

bool
eu_exist_libcurl(void)
{
    return util_exist_libcurl();
}

bool
eu_mk_dir(LPCTSTR dir)
{
    LPTSTR p = NULL;
    TCHAR tmp_name[MAX_BUFFER] = {0};
    _tcsncpy(tmp_name, dir, MAX_BUFFER);
    p = _tcschr(tmp_name, _T('\\'));
    for (; p != NULL; *p = _T('\\'), p = _tcschr(p + 1, _T('\\')))
    {
        *p = _T('\0');
        if (eu_exist_dir(tmp_name))
        {
            continue;
        }
        if (!CreateDirectory(tmp_name, NULL))
        {
            return false;
        }
    }
    return (eu_exist_dir(tmp_name)? true: (CreateDirectory(tmp_name, NULL)));
}

bool
eu_touch(LPCTSTR path)
{
    TCHAR *fullpath = NULL;
    bool ret = eu_exist_file(path);
    if (path && !ret && (fullpath = (TCHAR *)_tcsdup(path)))
    {
        eu_suffix_strip(fullpath);
        eu_mk_dir(fullpath);
        FILE *fd = _tfopen(path, _T("w+b"));
        if (fd)
        {
            fclose(fd);
            ret = true;
        }
        eu_safe_free(fullpath);
    }
    return ret;
}

static bool
do_rename_operation(const TCHAR *porig)
{
    if (STR_NOT_NUL(porig))
    {
        TCHAR pold[MAX_BUFFER] = {0};
        for (int i = 1; i < 99; ++i)
        {
            _sntprintf(pold, MAX_BUFFER - 1, _T("%s.old.%d"), porig, i);
            if (!eu_exist_wpath(pold))
            {
                break;
            }
        }
        if (pold[0] && porig[0])
        {
            return (_wrename(porig, pold) == 0);
        }
    }
    return false;
}

static void
do_move_operation(const TCHAR *pname)
{
    if (STR_NOT_NUL(pname))
    {
        TCHAR porig[MAX_BUFFER] = {0};
        TCHAR pback[MAX_BUFFER] = {0};
        _sntprintf(porig, MAX_BUFFER - 1, _T("%s\\%s"), eu_config_path, pname);
        _sntprintf(pback, MAX_BUFFER - 1, _T("%s\\%s\\%s"), eu_config_path, EU_RESET_BACKUP_NAME, pname);
        MoveFileEx(porig, pback, MOVEFILE_COPY_ALLOWED|MOVEFILE_REPLACE_EXISTING);
    }
}

static bool
do_rename_thmem(void)
{
    WIN32_FIND_DATA data;
    TCHAR filepath[MAX_BUFFER];
    HANDLE hfile;
    _sntprintf(filepath, MAX_BUFFER, _T("%s\\styletheme*.conf"), eu_config_path);
    hfile = FindFirstFile(filepath, &data);
    if (hfile == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    do
    {
        if (_tcscmp(data.cFileName, _T(".")) == 0 || _tcscmp(data.cFileName, _T("..")) == 0)
        {
            continue;
        }
        if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            continue;
        }
        _sntprintf(filepath, MAX_BUFFER, _T("%s\\%s"), eu_config_path, data.cFileName);
        if (data.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
        {
            SetFileAttributes(filepath, data.dwFileAttributes & (~FILE_ATTRIBUTE_READONLY));
        }
        do_rename_operation(filepath);
    } while (FindNextFile(hfile, &data));
    FindClose(hfile);
    return true;
}

static bool
do_configs_backup(void)
{
    TCHAR filepath[MAX_BUFFER];
    TCHAR backpath[MAX_BUFFER];
    _sntprintf(filepath, MAX_BUFFER, _T("%s\\*.old.*"), eu_config_path);
    _sntprintf(backpath, MAX_BUFFER - 1, _T("%s\\%s"), eu_config_path, EU_RESET_BACKUP_NAME);
    if (eu_exist_wpath(backpath))
    {
        util_delete_file(backpath);
    }
    if (eu_mk_dir(backpath))
    {
        WIN32_FIND_DATA data;
        HANDLE hfile = FindFirstFile(filepath, &data);
        if (hfile == INVALID_HANDLE_VALUE)
        {
            return false;
        }
        do
        {
            if (_tcscmp(data.cFileName, _T(".")) == 0 || _tcscmp(data.cFileName, _T("..")) == 0)
            {
                continue;
            }
            _sntprintf(filepath, MAX_BUFFER, _T("%s\\%s"), eu_config_path, data.cFileName);
            if (data.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
            {
                SetFileAttributes(filepath, data.dwFileAttributes & (~FILE_ATTRIBUTE_READONLY));
            }
            do_move_operation(data.cFileName);
        } while (FindNextFile(hfile, &data));
        FindClose(hfile);
        return true;
    }
    return false;
}

static bool
rename_configs(uint32_t mask)
{
    bool result = false;
    if (mask != 0)
    {
        TCHAR conf_orig[MAX_BUFFER] = {0};
        if (mask & SNIP_CONFIG_MASK)
        {
            _sntprintf(conf_orig, MAX_BUFFER - 1, _T("%s\\snippets"), eu_config_path);
            result = do_rename_operation(conf_orig);
        }
        if (mask & ACCS_CONFIG_MASK)
        {
            _sntprintf(conf_orig, MAX_BUFFER - 1, _T("%s\\skylark_input.conf"), eu_config_path);
            result = do_rename_operation(conf_orig);
        }
        if (mask & DOCS_CONFIG_MASK)
        {
            _sntprintf(conf_orig, MAX_BUFFER - 1, _T("%s\\script-opts"), eu_config_path);
            result = do_rename_operation(conf_orig);
        }
        if (mask & SQLS_CONFIG_MASK)
        {
            _sntprintf(conf_orig, MAX_BUFFER - 1, _T("%s\\skylark_prefs.sqlite3"), eu_config_path);
            result = do_rename_operation(conf_orig);
            if (result)
            {
                _sntprintf(conf_orig, MAX_BUFFER - 1, _T("%s\\cache"), eu_config_path);
                result = do_rename_operation(conf_orig);
            }
            _sntprintf(conf_orig, MAX_BUFFER - 1, _T("%s\\skylark_pfavs.sqlite3"), eu_config_path);
            result = do_rename_operation(conf_orig);
        }
        if (mask & MAIN_CONFIG_MASK)
        {
            _sntprintf(conf_orig, MAX_BUFFER - 1, _T("%s\\skylark.conf"), eu_config_path);
            result = do_rename_operation(conf_orig);
            _sntprintf(conf_orig, MAX_BUFFER - 1, _T("%s\\skylark_toolbar.conf"), eu_config_path);
            result = do_rename_operation(conf_orig);
        }
        if (mask & THEM_CONFIG_MASK)
        {
            result = do_rename_thmem();
            if (!(mask & MAIN_CONFIG_MASK))
            {
                _sntprintf(conf_orig, MAX_BUFFER - 1, _T("%s\\skylark.conf"), eu_config_path);
                result = do_rename_operation(conf_orig);
            }
        }
    }
    return result;
}

void
eu_reset_main_mask(void)
{
    fn_config_mask |= MAIN_CONFIG_MASK;
}

void
eu_reset_sqls_mask(void)
{
    fn_config_mask |= SQLS_CONFIG_MASK;
}

void
eu_reset_docs_mask(void)
{
    fn_config_mask |= DOCS_CONFIG_MASK;
}

void
eu_reset_accs_mask(void)
{
    fn_config_mask |= ACCS_CONFIG_MASK;
}

void
eu_reset_snip_mask(void)
{
    fn_config_mask |= SNIP_CONFIG_MASK;
}

void
eu_reset_theme_mask(void)
{
    fn_config_mask |= THEM_CONFIG_MASK;
}

void
eu_reset_all_mask(void)
{
    fn_config_mask |= EAPI_RSTART_MASK;
}

void
eu_reset_config(void)
{
    if (rename_configs(fn_config_mask) && fn_config_mask == EAPI_RSTART_MASK)
    {
        if (do_configs_backup())
        {
            eu_logmsg("%s: We will reset all configuration files and restart the editor\n", __FUNCTION__);
        }
    }
}

static int
query_encode(const char *coding)
{
    eue_code *iter = NULL;
    for (iter = &eue_coding[0]; iter->nid; ++iter)
    {
        if (_strnicmp(iter->desc, coding, strlen(coding)) == 0)
        {
            return iter->nid;
        }
    }
    return IDM_UNKNOWN;
}

#if APP_DEBUG
static void
printf_bytes(const char *str, size_t len, const char *name)
{
    if (!(str && name))
    {
        return;
    }
    size_t str_len = strlen(str);
    if (len > str_len)
    {
        len = str_len;
    }
    if (eu_strcasestr(name, "utf-16"))
    {
        for(int i = 0; i < eu_int_cast(len) && (str[i] || str[i+1]); ++i)
        {
            printf("%.2x ",(unsigned char)str[i]);
        }
        printf("\n");
        return;
    }
    else
    {
        for(int i = 0; i < eu_int_cast(len) && str[i];++i)
        {
            printf("%.2x ",(unsigned char)str[i]);
        }
        printf("\n");
    }
}
#endif

/*******************************************************************************************************
 * 功能, 调用libiconv转换编码.
 * src, 输入缓存区
 * src_len, 输入缓存区长度指针
 * pout, 输出缓冲区, 为NULL时不输出, 只为了测试编码转换
 * from_desc 源编码类型, 如 utf-16
 * dst_dest, 目标编码类型, 如 utf-8
 * 返回值, 能成功转换返回true, 否则返回false
 *******************************************************************************************************/
bool
eu_iconv_converter(char *src, size_t *src_len, char **pout, const char *from_desc, const char *dst_desc)
{
    size_t ret = (size_t)-1;
    iconv_t cd = (iconv_t)-1;
    int argument = 0;
    char *psrc = src;
    char *ptmp = NULL, *pdst = NULL;
    size_t lsrc = *src_len;
    size_t ldst = lsrc * 4 + 5;
    cd = eu_iconv_open(dst_desc, from_desc);
    if (cd == (iconv_t) -1)
    {
        eu_logmsg("eu_iconv_open error!\n");
        goto iconv_err;
    }
    if (eu_iconvctl(cd, ICONV_SET_DISCARD_ILSEQ, &argument) != 0)
    {
        eu_logmsg("can't enable illegal feature!\n");
        goto iconv_err;
    }
    ptmp = pdst = (char *) calloc(1, ldst + 1);
    if (pdst == NULL)
    {
        goto iconv_err;
    }
    else if (pout)
    {
        *pout = pdst;
    }
    ret = eu_iconv(cd, &psrc, &lsrc, &pdst, &ldst);
iconv_err:
    if (!pout)
    {
        if (ptmp)
        {
            free(ptmp);
        }
    }
    eu_iconv_close(cd);
    if (ret != (size_t)-1)
    {
        eu_logmsg("%s->%s ok, ret = %zu!\n", from_desc, dst_desc, ret);
    }
    return (ret == 0);
}

static int
check_utf16_newline(const uint8_t *pbuffer, const size_t len)
{
    if (len < 2)
    {
        return EN_CODEING_NONE;
    }
    size_t pos = 0;
    size_t size = len;
    int le_control_chars = 0;
    int be_control_chars = 0;
    int result_le = IS_TEXT_UNICODE_STATISTICS | IS_TEXT_UNICODE_REVERSE_STATISTICS;
    int result_be = IS_TEXT_UNICODE_UNICODE_MASK;
    uint8_t ch1, ch2;
     // 避免数组跨越边界问题
    while (pos < size - 1)
    {
        ch1 = pbuffer[pos++];
        ch2 = pbuffer[pos++];
        if (ch1 == 0)
        {
            if (ch2 == 0x0a || ch2 == 0x0d)
            {
                ++be_control_chars;
            }
        }
        else if (ch2 == 0)
        {
            if (ch1 == 0x0a || ch1 == 0x0d)
            {
                ++le_control_chars;
            }
        }
        // 如果同时得到LE和BE控制字符，那么这个文件不是utf16
        if (le_control_chars && be_control_chars)
        {
            return EN_CODEING_NONE;
        }
    }
    if (le_control_chars)
    {
        // 如果存在utf-16le换行符, 测试一下编码转换, 因为可能为二进制文件
        if (eu_iconv_converter((char *)pbuffer, &size, NULL, "UTF-16LE", "GBK"))
        {
            return UTF16_LE_NOBOM;
        }
        else
        {
            return EN_CODEING_NONE;
        }
    }
    else if (be_control_chars)
    {
        // 如果存在utf-16be换行符, 测试一下编码转换, 因为可能为二进制文件
        if (eu_iconv_converter((char *)pbuffer, &size, NULL, "UTF-16BE", "GBK"))
        {
            return UTF16_BE_NOBOM;
        }
        else
        {
            return EN_CODEING_NONE;
        }
    }   // 如果通过utf-16le静态分析, 测试一下编码转换, 因为可能为二进制文件
    if (IsTextUnicode(pbuffer, eu_int_cast(len), &result_le) && (result_le & IS_TEXT_UNICODE_STATISTICS))
    {
        eu_logmsg("result_le = %d\n", result_le);
        if (eu_iconv_converter((char *)pbuffer, &size, NULL, "UTF-16LE", "GBK"))
        {
            return UTF16_LE_NOBOM;
        }
    }   // 如果通过utf-16be静态分析, 测试一下编码转换, 因为可能为二进制文件
    else if (IsTextUnicode(pbuffer, eu_int_cast(len), &result_be) && eu_iconv_converter((char *)pbuffer, &size, NULL, "UTF-16BE", "GBK"))
    {
        eu_logmsg("result_be = %d\n", result_be);
        return UTF16_BE_NOBOM;
    }
    return EN_CODEING_NONE;
}

static bool
is_mbcs_gb18030(const char *chars, size_t len)
{
    int score = 0;
    size_t str_len = strlen(chars);
    if (len > str_len)
    {
        len = str_len;
    }
    for (size_t i = 0; i < eu_int_cast(len); ++i)
    {
        uint8_t ch = chars[i];
        // 非法字符0xFF
        if ((ch & 0xFF) == 0xFF)
        {
            return false;
        }
        if (CHAR_IS_LATIN(ch))
        {
            // 单字节 ASCII
            score++;
        }   // 0x80-0xFE,根据下一位判断
        else if (IS_1LST_BYTE(ch))
        {
            if (i < len - 1)
            {
                uint8_t ch2 = chars[++i];
                if (IS_2NDBYTE_32(ch2) && i < len - 2)
                {   // 四字节判断
                    uint8_t ch3 = chars[++i];
                    if (IS_3RD_BYTE(ch3) && i < len - 1)
                    {
                        uint8_t ch4 = chars[++i];
                        if (!IS_4TH_BYTE(ch4))
                        {
                            return -1;
                        }
                        score += 2;
                    }
                    else
                    {
                        return false;
                    }
                }   // 双字节判断
                else if (++score && !IS_2NDBYTE_16(ch2))
                {
                    return false;
                }
            }
            ++score;
        }
        else
        {
            // 非法编码
            return false;
        }
    }
    return (score == len);
}

/*******************************************************************
 * 16进制搜索
 * haystack, 源字符串或内存块
 * needle 子串为 16 进制格式 ,如: FFFE2B
 * size, 源字符串或内存块长度, 以字节为单位
 *******************************************************************/
const uint8_t *
eu_memstr(const uint8_t *haystack, const char *needle, size_t size)
{
    int i = 0;
    const uint8_t *p;
    uint8_t *need = NULL;
    size_t needlesize = strlen(needle);
    if (needlesize%2 !=0 )
    {
        eu_logmsg("not double byte\n");
        return NULL;
    }
    if (!(need = (uint8_t *)malloc(needlesize/2)))
    {
        return NULL;
    }
    while (1)
    {
        if (1 == sscanf(needle+i*2,"%02x", (unsigned int *)(need+i)))
        {
            i++;
            if ((size_t)i >= needlesize)
                break;
        }
        else
        {
            break;
        }
    }
#if APP_DEBUG
    printf("need[0] = %.2x\n", need[0]);
    printf("need[1] = %.2x\n", need[1]);
#endif
    for (p = haystack; p <= (haystack-needlesize+size); p++)
    {
        if (memcmp(p, need, needlesize/2) == 0)
        {
            free(need);
            return p; /* found */
        }
    }
    free(need);
    return NULL;
}

static void
eu_update_array(intptr_t *arr, int len, intptr_t m)
{
    for (int i = 0; i < len; ++i)
    {
        if (arr[i] < 0)
        {
            arr[i] = m;
            break;
        }
    }
}

/*******************************************************************
 * sunday16进制匹配算法
 * str, 源字符串或内存块
 * pattern 子串
 * str_len, 源字符串长度
 * reverse, 是否反向查找
 * pret保存最初或最后(反向搜索)的结果偏移地址
 * 返回匹配的个数, 没有匹配则返回0
 *******************************************************************/
int
eu_sunday_hex(const uint8_t *str, const char *pattern, size_t str_len, bool reverse, intptr_t *pret)
{
    int match = 0;
    uint8_t *pmark = NULL;
    bool table[256] = {false};
    size_t  i = 0;
    size_t len = strlen(pattern);
    len /= 2;
    // 将字符串转换成byte型
    if (!(pmark = (uint8_t *) calloc(1, len)))
    {
        return 0;
    }
    for (i = 0; i < len; ++i)
    {
        char temp[3] = { 0 };
        strncpy(temp, &pattern[i * 2], 2);
        pmark[i] = (uint8_t) strtoul(temp, 0, 16);
    }
    for (i = 0; i < len; ++i)
    {
        table[pmark[i]] = true;
    }
    *pret = -1;
    for (i = 0; i < str_len; ++i)
    {
        int found = 0;
        for (size_t j = 0; j < len; ++j)
        {
            if (str[i + j] == pmark[j])
            {
                found = 1;
            }
            else
            {
                if (!table[str[i + len]])
                {
                    i = i + len;
                }
                found = 0;
                break;
            }
        }
        if (found == 1)
        {
            ++match;
            if (reverse)
            {
                *pret = i;
            }
            else if (*pret < 0)
            {
                *pret = i;
            }
            i += len;
        }
    }
    free(pmark);
    return match;
}

static bool
eu_availed_char(const uint8_t ch)
{
    if (isalnum(ch)||(ch == '_')||(ch > 0x7f))
    {
        return true;
    }
    return false;
}

/*******************************************************************
 * sunday字符串匹配算法
 * str, 源字符串或内存块
 * pattern 子串
 * n, b, 源字符串长度, 子串长度
 * incase, 忽略大小写
 * whole, 是否整词匹配
 * reverse, 反向搜索
 * pret保存最初或最后(反向搜索)的结果偏移地址
 * 返回匹配的个数, 没有匹配则返回0
 *******************************************************************/
int
eu_sunday(const uint8_t *str, const uint8_t *pattern, size_t n, size_t b, bool incase, bool whole, bool reverse, intptr_t *pret)
{
    bool table[256] = {false};
    int match = 0;
    size_t i = 0;
    for (i = 0; i < b; ++i)
    {
        if (incase)
        {
            table[tolower(pattern[i])] = true;
        }
        else
        {
            table[pattern[i]] = true;
        }
    }
    for (i = 0; i < n; ++i)
    {
        int found = 0;
        for (size_t j = 0; j < b; ++j)
        {
            if (incase && tolower(str[i + j]) == tolower(pattern[j]))
            {
                found = 1;
            }
            else if (!incase && str[i + j] == pattern[j])
            {
                found = 1;
            }
            else
            {
                if (incase && !table[tolower(str[i + b])])
                {
                    i = i + b;
                }
                else if (!incase && !table[str[i + b]])
                {
                    i = i + b;
                }
                found = 0;
                break;
            }
        }
        if (found == 1)
        {
            if (whole)
            {
                bool pre = i - 1 >= 0?!eu_availed_char(str[i-1]):true;
                bool next = i + b < n?!eu_availed_char(str[i+b]):true;
                if (pre && next)
                {
                    ++match;
                }
            }
            else
            {
                ++match;
            }
            if (pret)
            {
                if (reverse)
                {
                    *pret = i;
                }
                else if (*pret < 0)
                {
                    *pret = i;
                }
            }
            i += b;
        }
    }
    return match;
}

static int
is_plan_file(const uint8_t *name, const size_t len, const bool nobinary)
{
    int ret = 0;
    const bom_type file_bom[] = {
        {3, "\xEF\xBB\xBF"},        // BOM_UTF8
        {4, "\xFF\xFE\x00\x00"},    // BOM_UTF32_LE
        {4, "\x00\x00\xFE\xFF"},    // BOM_UTF32_BE
        {2, "\xFF\xFE"},            // BOM_UTF16_LE
        {2, "\xFE\xFF"},            // BOM_UTF16_BE
        {0, {0}}
    };
    for (int i = 0; file_bom[i].len; ++i)
    {
        if (memcmp(name , file_bom[i].bom , file_bom[i].len ) == 0)
        {
            switch (i)
            {
                case 0:
                    return EN_CODEING_UTF8BOM;
                case 1:
                    return UTF32_LE_BOM;
                case 2:
                    return UTF32_BE_BOM;
                case 3:
                    return UTF16_LE_BOM;
                case 4:
                    return UTF16_BE_BOM;
                default:
                    break;
            }
        }
    }
    if ((ret = check_utf16_newline(name, len)) > 0)
    {
        return ret;
    }
    if (!nobinary)
    {
        uint32_t white_list = 0;
        for (size_t i = 0; i < len; ++i)
        {
            // Check for textual ("white-listed") bytes.
            if (name[i] == 9 || (name[i] >= 10 && name[i] <= 15) || (name[i] >= 32 && name[i] <= 255))
            {
                ++white_list;
            }
            else
            {
                int n;
                uint32_t black_mask = 0xf3ffc07fUL;
                // Check for non-textual ("black-listed") bytes.
                for (n = 0; n <= 31; n++, black_mask >>= 1)
                {
                    if ((black_mask & 1) && (name[i] == n))
                    {
                        eu_logmsg("\nbinary char, name[%zu] = %.02x\n", i, name[i]);
                        return EN_CODEING_BINARY;
                    }
                }
            }
        }
        if (white_list < 1)
        {
            return EN_CODEING_BINARY;
        }
    }
    return EN_CODEING_NONE;
}

HANDLE
eu_new_process(LPCTSTR wcmd, LPCTSTR param, LPCTSTR pcd, int flags, uint32_t *o)
{
    PROCESS_INFORMATION pi;
    STARTUPINFOW si;
    uint32_t dw_creat = 0;
    LPCTSTR lp_dir = NULL;
    TCHAR process[LARGER_LEN+1] = {0};
    if (STR_NOT_NUL(pcd))
    {
        lp_dir = pcd;
    }
    else
    {
        lp_dir = (LPCTSTR)eu_module_path;
    }
    if (param != NULL && _tcslen(param ) > 1)
    {
        _sntprintf(process, LARGER_LEN, _T("%s %s"), wcmd, param);
    }
    else
    {
        _sntprintf(process, LARGER_LEN, _T("%s"), wcmd);
    }
    if (true)
    {
        memset(&si, 0, sizeof(si));
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        if (flags > 1)
        {
            si.wShowWindow = SW_SHOWNOACTIVATE;
        }
        else if (flags == 1)
        {
            si.wShowWindow = SW_MINIMIZE;
        }
        else if (!flags)
        {
            si.wShowWindow = SW_HIDE;
            dw_creat |= CREATE_NEW_PROCESS_GROUP;
        }
        if(!CreateProcess(NULL,
                          process,
                          NULL,
                          NULL,
                          FALSE,
                          dw_creat,
                          NULL,
                          lp_dir,
                          &si,&pi))
        {
            char u8[MAX_BUFFER] = {0};
            eu_logmsg("CreateProcessW [%s] error, cause: %lu\n", util_make_u8(wcmd, u8, MAX_BUFFER - 1), GetLastError());
            return NULL;
        }
        if (NULL != o)
        {
            *o = pi.dwProcessId;
        }
        CloseHandle(pi.hThread);
        util_set_working_dir(eu_module_path, NULL);
    }
    return pi.hProcess;
}

bool
eu_open_file(LPCTSTR path, pf_stream pstream)
{
    return util_open_file(path, pstream);
}

char *
eu_strcasestr(const char *haystack, const char *needle)
{
    size_t l = strlen(needle);
    for (; *haystack; haystack++)
    {
        if (!_strnicmp(haystack, needle, l))
        {
            return (char *)haystack;
        }
    }
    return NULL;
}

static bool
eu_ascii_escaped(const char *checkstr)
{
    bool ret = false;
    char *p = eu_strcasestr(checkstr, "\\u");
    if (p && (p - (char *)checkstr > 1 ? (*(p - 1) != '\\') : true))
    {
        size_t end = strlen(p);
        if (end > 6)
        {
            end = 6;
        }
        for (int i = 2; i < eu_int_cast(end); ++i)
        {
            if (!isxdigit(p[i]))
            {
                ret = false;
                break;
            }
            ret = true;
        }
    }
    return ret;
}

const char *
eu_query_encoding_name(int code)
{
    eue_code *iter = NULL;
    for (iter = &eue_coding[0]; iter->nid; ++iter)
    {
        if (iter->nid == code)
        {
            return iter->desc;
        }
    }
    return NULL;
}

static bool
is_exclude_char(const char *encoding)
{
    return (0
           /* Legacy Japanese encodings */
           || STRICMP(encoding, ==, "euc-jp")
           || STRICMP(encoding, ==, "cp932")
           /* Legacy Chinese traditional encodings */
           || STRICMP(encoding, ==, "euc-tw")
           || STRICMP(encoding, ==, "big5")
           /* Legacy Korean encodings */
           || STRICMP(encoding, ==, "euc-kr")
           /* Legacy ISO-x encodings */
           || STRICMP(encoding, ==, "iso-8859-1")
           || STRICMP(encoding, ==, "iso-8859-2")
           || STRICMP(encoding, ==, "iso-8859-3")
           || STRICMP(encoding, ==, "iso-8859-16")
           /* central europe */
           || STRICMP(encoding, ==, "ibm852")
           || STRICMP(encoding, ==, "maccentraleurope")
           || STRICMP(encoding, ==, "windows-1250")
           || STRICMP(encoding, ==, "windows-1252")
           );
}

static bool
eu_iconv_full_text(const TCHAR *file_name, const char *from_desc, const char *dst_desc)
{
    FILE *fp = NULL;
    bool  ret = true;
    size_t len = 0;
    uint8_t *data = NULL;
    struct _stat st = {0};
    size_t buf_len = BUFF_SIZE;
    if ((fp = _tfopen(file_name, _T("rb"))) == NULL)
    {
        return false;
    }
    _tstat(file_name, &st);
    if (st.st_size <= 0)
    {
        fclose(fp);
        return false;
    }
    if (st.st_size < BUFF_SIZE)
    {
        buf_len = (size_t)st.st_size;
    }
    if ((data = (uint8_t *) calloc(1, buf_len)) == NULL)
    {
        fclose(fp);
        return false;
    }
    else
    {
        fseek(fp, 0, SEEK_SET);
    }
    while ((len = fread((char *) data, 1, buf_len, fp)) > 0)
    {
        if (!eu_iconv_converter((char *)data, &len, NULL, from_desc, dst_desc))
        {
            ret = false;
            break;
        }
    }
    fclose(fp);
    eu_safe_free(data);
    return ret;
}

int
eu_try_encoding(uint8_t *buffer, size_t len, bool is_file, const TCHAR *file_name)
{
    FILE *fp = NULL;
    size_t read_len;
    DetectObj *obj = NULL;
    int type = IDM_UNKNOWN;
    bool nobinary = false;
    uint8_t *checkstr = buffer;
    if (!(buffer || is_file))
    {
        return type;
    }
    if (is_file)
    {
        if (STR_IS_NUL(file_name))
        {
            return type;
        }
        if ((fp = _tfopen(file_name, _T("rb"))) == NULL)
        {
            eu_logmsg("_tfopen failed in %s\n", __FUNCTION__);
            return type;
        }
        read_len = fread(checkstr, 1, len - 1, fp);
        fclose(fp);
    }
    else if (STR_NOT_NUL(file_name) && on_doc_get_type(file_name))
    {   // 如果是支持高亮的文件类型, 跳过二进制检测
        nobinary = true;
        read_len = eu_int_cast(len > BUFF_SIZE ? BUFF_SIZE : len); 
    }
    else
    {
        read_len = eu_int_cast(len > BUFF_SIZE ? BUFF_SIZE : len);
    }
    if (!(type = is_plan_file(checkstr, read_len, nobinary)))
    {
        // BINARY file
        eu_logmsg("this is BINARY file\n");
        return IDM_OTHER_BIN;
    }
    switch (type)
    {
        case EN_CODEING_UTF8BOM:
            return IDM_UNI_UTF8B;
        case UTF16_LE_NOBOM:
            return IDM_UNI_UTF16LE;
        case UTF16_LE_BOM:
            return IDM_UNI_UTF16LEB;
        case UTF16_BE_NOBOM:
            return IDM_UNI_UTF16BE;
        case UTF16_BE_BOM:
            return IDM_UNI_UTF16BEB;
        case UTF32_LE_BOM:
            return IDM_UNI_UTF32LE;
        case UTF32_BE_BOM:
            return IDM_UNI_UTF32BE;
        default:
            type = IDM_UNKNOWN;
            break;
    }
    if ((obj = detect_obj_init()) == NULL)
    {
        eu_logmsg("Memory Allocation failed in %s\n", __FUNCTION__);
        return type;
    }
    switch (detect_r((const char *)checkstr, read_len, &obj))
    {
        case CHARDET_OUT_OF_MEMORY:
            eu_logmsg("On handle processing, occured out of memory\n");
            detect_obj_free(&obj);
            return type;
        case CHARDET_NULL_OBJECT:
            eu_logmsg("2st argument of chardet() is must memory allocation with detect_obj_init API\n");
            return type;
        default:
            break;
    }
    eu_logmsg("%s, confidence: %f, exists bom: %d\n", obj->encoding, obj->confidence, obj->bom);
    if (!obj->encoding || DET_EPSILON > obj->confidence)
    {
        if (is_mbcs_gb18030((const char *)checkstr, read_len))
        {
            // GB18030!
            type = IDM_ANSI_12;
        }
        else if (on_encoding_validate_utf8((const char *)checkstr, read_len))
        {
            eu_logmsg("Maybe UTF-8!\n");
            type = obj->bom?IDM_UNI_UTF8B:IDM_UNI_UTF8;
        }
        else
        {
            type = IDM_OTHER_ANSI;
        }
    }
    else if (strcmp(obj->encoding, "ASCII") == 0)
    {
        if (CHECK_1ST > obj->confidence && eu_iconv_converter((char *)checkstr, &read_len, NULL, "iso-2022-cn-ext", "utf-8"))
        {
            type = IDM_ISO_CN;
        }
        else
        {
            type = IDM_UNI_UTF8;
        }
    }
    else if (_strnicmp(obj->encoding, "ISO-8859-", 9) == 0)
    {
        if (on_encoding_validate_utf8((const char *)checkstr, read_len))
        {
            eu_logmsg("Not iso encode, it's maybe UTF-8!\n");
            type = obj->bom?IDM_UNI_UTF8B:IDM_UNI_UTF8;
        }
        else if (CHECK_1ST > obj->confidence && is_mbcs_gb18030((const char *)checkstr, read_len))
        {
            eu_logmsg("Confidence[%f] < %f, Maybe GB18030!\n", obj->confidence, CHECK_1ST);
            type = IDM_ANSI_12;
        }
        else
        {
            type = query_encode(obj->encoding);
        }
    }
    else if (strcmp(obj->encoding, "ISO-2022-JP") == 0 && obj->confidence > CHECK_1ST)
    {
        if (eu_iconv_converter((char *)checkstr, &read_len, NULL, "iso-2022-jp-2004", "utf-8"))
        {
            type = IDM_ISO_JP_2004;
        }
        else if (eu_iconv_converter((char *)checkstr, &read_len, NULL, "iso-2022-jp-2", "utf-8"))
        {
            type = IDM_ISO_JP_2;
        }
        else if (eu_iconv_converter((char *)checkstr, &read_len, NULL, "iso-2022-jp-ms", "utf-8"))
        {
            type = IDM_ISO_JP_MS;
        }
        else
        {
            type = IDM_UNKNOWN;
        }
    }
    else if (obj->confidence < CHECK_2ND)
    {
        if (on_encoding_validate_utf8((const char *)checkstr, read_len))
        {
            eu_logmsg("we reconfirm that's UTF-8!\n");
            type = obj->bom?IDM_UNI_UTF8B:IDM_UNI_UTF8;
        }
        else if (obj->confidence > CHECK_1ST && is_exclude_char(obj->encoding))
        {
            type = query_encode(obj->encoding);
        }
        else if (is_mbcs_gb18030((const char *)checkstr, read_len))
        {
            eu_logmsg("Maybe GB18030!\n");
            type = IDM_ANSI_12;
        }
        else
        {
            type = query_encode(obj->encoding);
            eu_logmsg("Blur identification! type = %d, obj->encoding = %s\n", type, obj->encoding);
            if ((file_name != NULL) && !eu_iconv_full_text(file_name, obj->encoding, "utf-8"))
            {
                eu_logmsg("It doesn't look like %s, We think of it as binary coding\n", obj->encoding);
                type = IDM_OTHER_BIN;
            }
        }
    }
    else if (strcmp(obj->encoding, "UTF-8") == 0)
    {
        type = obj->bom?IDM_UNI_UTF8B:IDM_UNI_UTF8;
    }
    else
    {
        type = query_encode(obj->encoding);
    }
    detect_obj_free(&obj);
    return type;
}
/***********************************************************************************
 * 如果函数执行成功, (*out_len) 返回转换后的字符个数, 包含结束符0
 * 所以, buf =  (*out_len - 1) * sizeof(TCHAR)
 ***********************************************************************************/
char*
eu_utf16_utf8(const wchar_t *utf16, size_t *out_len)
{
    int   m, size = 0;
    char *utf8 = NULL;

    size = WideCharToMultiByte(CP_UTF8, 0, utf16, -1, NULL, 0, NULL, NULL);
    utf8 = size > 0 ? (char*) malloc(size+1) : 0;
    if (NULL == utf8 )
    {
        return NULL;
    }
    m = WideCharToMultiByte(CP_UTF8, 0, utf16, -1, utf8, size, NULL, NULL);
    if (m > 0 && m <= size)
    {
        if (out_len)
        {
            *out_len = (size_t)m;
        }
        utf8[m-1] = 0;
    }
    else
    {
        free(utf8);
        utf8 = NULL;
    }
    return utf8;
}

char*
eu_utf16_mbcs(int codepage, const wchar_t *utf16, size_t *out_len)
{
    int   size = 0;
    char *a8 = NULL;
    if (codepage < 0)
    {
        codepage= AreFileApisANSI() ? CP_ACP : CP_OEMCP;
    }
    size = WideCharToMultiByte(codepage, 0, utf16, -1, NULL, 0, NULL, NULL);
    a8 = size > 0 ? (char*) malloc(size+ 1) : 0;
    if (NULL == a8)
    {
        return NULL;
    }
    size = WideCharToMultiByte(codepage, 0, utf16, -1, a8, size, NULL, NULL);
    if (size > 0)
    {
        if (out_len)
        {
            *out_len = (size_t)size;
        }
    }
    else
    {
        free(a8);
        a8 = NULL;
    }
    return a8;
}

wchar_t*
eu_mbcs_utf16(int codepage, const char *ansi, size_t *out_len)
{
    int size;
    wchar_t *u16 = NULL;
    if (codepage < 0)
    {
        codepage= AreFileApisANSI() ? CP_ACP : CP_OEMCP;
    }
    size = MultiByteToWideChar(codepage, 0, ansi, -1, NULL, 0);
    u16 = size > 0 ? (wchar_t*) malloc(sizeof(wchar_t) * (size + 1)) : 0;
    if (!u16)
    {
        return NULL;
    }
    size = MultiByteToWideChar(codepage, 0, ansi, -1, u16, size);
    if (size > 0)
    {
        if (out_len)
        {
            *out_len = (size_t)size;
        }
    }
    else
    {
        free(u16);
        return NULL;
    }
    return u16;
}

char*
eu_mbcs_utf8(int codepage, const char *ansi, size_t *out_len)
{
#ifdef _WIN32
    char *utf8 = NULL;
    wchar_t *u16 = eu_mbcs_utf16(codepage, ansi, NULL);
    if (u16)
    {
        utf8 = eu_utf16_utf8(u16, out_len);
        free(u16);
    }
    return utf8;
#else
  return strdup(ansi);
#endif
}

wchar_t*
eu_utf8_utf16(const char *utf8, size_t *out_len)
{
    int size;
    wchar_t *u16 = NULL;
    size = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
    u16 = size > 0 ? (wchar_t*) malloc(sizeof(wchar_t) * (size + 1)) : 0;
    if (!u16)
    {
        return NULL;
    }
    size = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, u16, size);
    if (size > 0)
    {
        if (out_len)
        {
            *out_len = (size_t)size;
        }
    }
    else
    {
        free(u16);
        return NULL;
    }
    return u16;
}

char*
eu_utf8_mbcs(int codepage, const char *utf8, size_t *out_len)
{
#ifdef _WIN32
    char *a8 = NULL;
    wchar_t *u16 = eu_utf8_utf16(utf8, NULL);
    if (u16)
    {
        a8 = eu_utf16_mbcs(codepage, u16, out_len);
        free(u16);
    }
    return a8;
#else
  return strdup(utf8);
#endif
}

void
eu_setpos_window(HWND hwnd, HWND affer, int x, int y, int cx, int cy, uint32_t flags)
{
    SetWindowPos(hwnd, affer, x, y, cx, cy, flags);
}

bool
eu_config_ptr(struct eu_config *pconfig)
{
    if (g_config)
    {
        return true;
    }
    if (!pconfig)
    {
        return false;
    }
    g_config = (struct eu_config *)malloc(sizeof(struct eu_config));
    if (g_config)
    {
        memcpy(g_config, pconfig, sizeof(struct eu_config));
    }
    return g_config != NULL;
}

bool
eu_theme_ptr(struct eu_theme *ptheme, bool init)
{
    if (!ptheme)
    {
        return false;
    }
    if (!init)
    {
        if (g_theme)
        {
            free(g_theme);
        }
        g_theme = ptheme;
    }
    else
    {
        g_theme = (struct eu_theme *)malloc(sizeof(struct eu_theme));
        if (g_theme)
        {
            memcpy(g_theme, ptheme, sizeof(struct eu_theme));
        }
    }
    return g_theme != NULL;
}

bool
eu_accel_ptr(ACCEL *accel)
{
    if (g_accel)
    {
        return true;
    }
    if (!accel)
    {
        return false;
    }
    ;
    if (!(g_accel = (eue_accel *)calloc(1, sizeof(eue_accel))))
    {
        return false;
    }
    for (int i = 0; accel && (i < MAX_ACCELS); ++i, ++accel)
    {
        if (!accel->cmd)
        {
            break;
        }
        memcpy(&g_accel->accel_ptr[i], accel, sizeof(ACCEL));
        g_accel->accel_num++;
    }
    return (g_accel->accel_num>0);
}

bool
eu_toolbar_ptr(eue_toolbar *pdata, int num)
{
    if (g_toolbar)
    {
        return true;
    }
    if (!pdata)
    {
        return false;
    }
    g_toolbar = (eue_toolbar *)malloc(sizeof(eue_toolbar) * num);
    if (g_toolbar)
    {
        memcpy(g_toolbar, pdata, sizeof(eue_toolbar) * num);
    }
    return g_toolbar != NULL;
}

struct eu_config*
eu_get_config(void)
{
    return g_config;
}

struct eu_theme*
eu_get_theme(void)
{
    return g_theme;
}

eue_accel*
eu_get_accel(void)
{
    return g_accel;
}

eue_toolbar*
eu_get_toolbar(void)
{
    return g_toolbar;
}

static void
eu_free_config(void)
{
    if (g_config)
    {   // 销毁菜单图标资源
        for (int i = 0; i < DW_SIZE; ++i)
        {
            if (g_config->m_customize[i].hbmp)
            {
                DeleteObject((HBITMAP)g_config->m_customize[i].hbmp);
                g_config->m_customize[i].hbmp = 0;
            }
        }
		free(g_config);
		g_config = NULL;
    }
}

static void
eu_free_theme(void)
{
    eu_safe_free(g_theme);
}

static void
eu_free_accel(void)
{
    if (g_accel)
    {
        if (g_accel->haccel)
        {
            DestroyAcceleratorTable(g_accel->haccel);
            g_accel->haccel = NULL;
        }
        eu_safe_free(g_accel);
    }
}

static void
eu_free_toolbar(void)
{
    eu_safe_free(g_toolbar);
}

void
eu_config_api_release(void)
{
    eu_free_theme();
    eu_free_config();
    eu_free_accel();
    eu_free_toolbar();
    on_doc_ptr_free();
}

TCHAR *
eu_process_path(void)
{
    if (!eu_module_path[0])
    {
        HMODULE module = NULL;
        if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCTSTR)eu_process_path, &module) && module)
        {
            if (GetModuleFileName(module, eu_module_path, MAX_PATH) > 0)
            {
                TCHAR *p = _tcsrchr(eu_module_path, _T('\\'));
                if (p)
                {
                    *p = 0;
                }
            }
        }
    }
    EU_VERIFY(eu_module_path[0]);
    return eu_module_path;
}

static char*
eu_cat_process(void)
{
    char *pactions = NULL;
    int count = on_doc_count();
    if (count > 0)
    {
        int offset = 0;
        const size_t len = (size_t)(count * MAX_PATH);
        if ((pactions = (char *)calloc(1, len + 1)) != NULL)
        {
            for (int i = 1; i <= count && len > (size_t)offset; ++i)
            {
                _snprintf(pactions + offset, len - offset, "    \"%s\"%s", g_config->m_actions[i], i == count ? "\n" : ",\n");
                offset += (int)(strlen(g_config->m_actions[i]) + 8);
            }
        }
    }
    return pactions;
}

static char*
eu_customize_process(void)
{
    char *pcustomize = NULL;
    const int len = (const int)(sizeof(g_config->m_customize) + MAX_BUFFER);
    if ((pcustomize = (char *)calloc(1, len + 1)) != NULL)
    {
        int offset = 0;
        for (int i = 0; i < DW_SIZE && len > offset; ++i)
        {
            _snprintf(pcustomize + offset, len - offset, "    {\n"
                                                         "        ['hide'] = %s,\n"
                                                         "        ['name'] = \"%s\",\n"
                                                         "        ['path'] = \"%s\",\n"
                                                         "        ['param'] = \"%s\",\n"
                                                         "        ['micon'] = %d,\n"
                                                         "    }%s",
                                                         g_config->m_customize[i].hide ? "true" : "false",
                                                         g_config->m_customize[i].name,
                                                         g_config->m_customize[i].path,
                                                         g_config->m_customize[i].param,
                                                         g_config->m_customize[i].micon,
                                                         i == DW_SIZE - 1 ? "\n" : ",\n");
            offset = (int)strlen(pcustomize);
        }
    }
    return pcustomize;
}

void
eu_save_config(void)
{
    FILE *fp = NULL;
    char *save = NULL;
    char *pactions = NULL;
    char *pcustomize = NULL;
    TCHAR path[MAX_BUFFER+1] = {0};
    const char *pconfig =
        "-- if you edit the file, please keep the encoding correct(utf-8 nobom)\n"
        "newfile_eols = %d\n"
        "-- the macro is defined in the targetver.h\n"
        "newfile_encoding = %d\n"
        "enable_auto_identation = %s\n"
        "window_theme = \"%s\"\n"
        "window_full_screen = %s\n"
        "window_menubar_visiable = %s\n"
        "window_toolbar_visiable = %d\n"
        "window_statusbar_visiable = %s\n"
        "line_number_visiable = %s\n"
        "last_search_flags = 0x%08X\n"
        "white_space_visiable = %s\n"
        "white_space_size = %d\n"
        "newline_visiable = %s\n"
        "indentation_guides_visiable = %s\n"
        "tab_width = %d\n"
        "onkeydown_tab_convert_spaces = %s\n"
        "light_fold = %s\n"
        "wrapline_mode = %s\n"
        "enable_filetree_show = %s\n"
        "file_treebar_width = %d\n"
        "symbol_list_width = %d\n"
        "symbol_tree_width = %d\n"
        "sidebar_width = %d\n"
        "document_map_width = %d\n"
        "sqlquery_result_edit_height = %d\n"
        "sqlquery_result_listview_height = %d\n"
        "file_recent_number = %d\n"
        "scroll_to_cursor = %s\n"
        "-- always reload the current file?\n"
        "inter_reserved_0 = %d\n"
        "-- lock the tabbar?\n"
        "inter_reserved_1 = %d\n"
        "-- not used\n"
        "inter_reserved_2 = %d\n"
        "block_fold_visiable = %s\n"
        "tabs_tip_show_enable = %s\n"
        "tab_close_way = %d\n"
        "tab_close_draw = %d\n"
        "tab_new_way = %d\n"
        "tab_switch_forward = %d\n"
        "edit_font_quality = %d\n"
        "edit_rendering_technology = %d\n"
        "update_file_mask = %d\n"
        "update_file_notify = %d\n"
        "light_all_find_str = %s\n"
        "backup_on_file_write = %s\n"
        "save_last_session = %s\n"
        "exit_when_close_last_tab = %s\n"
        "allow_multiple_instance = %s\n"
        "enable_runtime_logging = %s\n"
        "save_last_placement = \"%s\"\n"
        "ui_language = \"%s\"\n"
        "-- bookmark default setting\n"
        "bookmark = {\n"
        "    visable = %s,\n"
        "    shape = %d,\n"
        "    argb = 0x%08X\n"
        "}\n"
        "-- brace default setting\n"
        "brace = {\n"
        "    matching = %s,\n"
        "    autoc = %s,\n"
        "    rgb = 0x%08X\n"
        "}\n"
        "-- calltip default setting\n"
        "calltip = {\n"
        "    enable = %s,\n"
        "    rgb = 0x%08X\n"
        "}\n"
        "-- auto complete default setting\n"
        "complete = {\n"
        "    enable = %s,\n"
        "    characters = %d,\n"
        "    snippet = %d\n"
        "}\n"
        "-- print default setting\n"
        "printer = {\n"
        "    header = %d,\n"
        "    footer = %d,\n"
        "    color_mode = %d,\n"
        "    zoom = %d,\n"
        "    margin_left = %d,\n"
        "    margin_top = %d,\n"
        "    margin_right = %d,\n"
        "    margin_bottom = %d\n"
        "}\n"
        "-- hyperlink hotspot default setting\n"
        "hyperlink_detection = %s\n"
        "-- automatically cached file (size < 200MB)\n"
        "cache_limit_size = %d\n"
        "app_upgrade = {\n"
        "    enable = %s,\n"
        "    flags = %d,\n"
        "    msg_id = %d,\n"
        "    last_check = %I64u,\n"
        "    url = \"%s\"\n"
        "}\n"
        "-- uses the backslash ( / ) to separate directories in file path. default value: cmd.exe\n"
        "process_path = \"%s\"\n"
        "other_editor_path = \"%s\"\n"
        "-- beyond compare Path\n"
        "m_reserved_0 = \"%s\"\n"
        "-- reserved variables\n"
        "m_reserved_1 = \"%s\"\n"
        "process_actions = {\n"
        "%s"
        "}\n"
        "process_customized = {\n"
        "%s"
        "}\n";
    if (!g_config)
    {
        return;
    }
    if ((save = (char *)calloc(1, BUFF_32K)) == NULL)
    {
        return;
    }
    if (!(pactions = eu_cat_process()))
    {
        free(save);
        return;
    }
    if (!(pcustomize = eu_customize_process()))
    {
        free(save);
        free(pactions);
        return;
    }
    _sntprintf(path, MAX_BUFFER, _T("%s\\skylark.conf"), eu_config_path);
    _snprintf(save, BUFF_32K - 1, pconfig,
              g_config->new_file_eol,
              g_config->new_file_enc,
              g_config->m_ident?"true":"false",
              g_config->window_theme[0]?g_config->window_theme:"default",
              g_config->m_fullscreen?"true":"false",
              g_config->m_menubar?"true":"false",
              g_config->m_toolbar,
              g_config->m_statusbar?"true":"false",
              g_config->m_linenumber?"true":"false",
              g_config->last_flags,
              g_config->ws_visiable?"true":"false",
              g_config->ws_size,
              g_config->newline_visialbe?"true":"false",
              g_config->m_indentation?"true":"false",
              g_config->tab_width,
              g_config->tab2spaces?"true":"false",
              g_config->light_fold?"true":"false",
              g_config->line_mode?"true":"false",
              g_config->m_ftree_show?"true":"false",
              g_config->file_tree_width,
              g_config->sym_list_width,
              g_config->sym_tree_width,
              g_config->sidebar_width,
              g_config->document_map_width,
              g_config->result_edit_height,
              g_config->result_list_height,
              (g_config->file_recent_number > 0 && g_config->file_recent_number < 100 ? g_config->file_recent_number : 29),
              g_config->scroll_to_cursor?"true":"false",
              0,
              g_config->inter_reserved_1,
              g_config->inter_reserved_2,
              g_config->block_fold?"true":"false",
              g_config->m_tab_tip?"true":"false",
              g_config->m_close_way,
              g_config->m_close_draw,
              g_config->m_new_way,
              g_config->m_tab_active,
              g_config->m_quality,
              g_config->m_render,
              0,
              g_config->m_up_notify,
              g_config->m_light_str?"true":"false",
              g_config->m_write_copy?"true":"false",
              g_config->m_session?"true":"false",
              g_config->m_exit?"true":"false",
              "false",
              g_config->m_logging?"true":"false",
              g_config->m_placement,
              g_config->m_language[0]?g_config->m_language:"auto",
              g_config->eu_bookmark.visable?"true":"false",
              g_config->eu_bookmark.shape,
              g_config->eu_bookmark.argb,
              g_config->eu_brace.matching?"true":"false",
              g_config->eu_brace.autoc?"true":"false",
              g_config->eu_brace.rgb,
              g_config->eu_calltip.enable?"true":"false",
              g_config->eu_calltip.rgb,
              g_config->eu_complete.enable?"true":"false",
              g_config->eu_complete.characters,
              g_config->eu_complete.snippet,
              g_config->eu_print.header,
              g_config->eu_print.footer,
              g_config->eu_print.color_mode,
              g_config->eu_print.zoom,
              g_config->eu_print.rect.left,
              g_config->eu_print.rect.top,
              g_config->eu_print.rect.right,
              g_config->eu_print.rect.bottom,
              g_config->m_hyperlink?"true":"false",
              g_config->m_limit,
              g_config->upgrade.enable?"true":"false",
              g_config->upgrade.flags,
              g_config->upgrade.msg_id,
              g_config->upgrade.last_check,
              g_config->upgrade.url,
              g_config->m_path,
              g_config->editor,
              g_config->m_reserved_0,
              g_config->m_reserved_1,
              pactions,
              pcustomize);
    if ((fp = _tfopen(path , _T("wb"))) != NULL)
    {
        fwrite(save, strlen(save), 1, fp);
        fclose(fp);
    }
    free(save);
    free(pactions);
    free(pcustomize);
}

void
eu_save_theme(void)
{
    int  len = 0;
    FILE *fp = NULL;
    char *save = NULL;
    wchar_t *path = NULL;
    const char *pconfig =
        "linenumber_font = \"%s\"\n"
        "linenumber_fontsize = %d\n"
        "linenumber_color = 0x%08X\n"
        "linenumber_bgcolor = 0x%08X\n"
        "linenumber_bold = %d\n"
        "foldmargin_font = \"%s\"\n"
        "foldmargin_fontsize = %d\n"
        "-- Fold mark(+, -) color\n"
        "foldmargin_color = 0x%08X\n"
        "-- fold margin color\n"
        "foldmargin_bgcolor = 0x%08X\n"
        "foldmargin_bold = %d\n"
        "text_font = \"%s\"\n"
        "text_fontsize = %d\n"
        "text_color = 0x%08X\n"
        "text_bgcolor = 0x%08X\n"
        "text_bold = %d\n"
        "caretline_font = \"%s\"\n"
        "caretline_fontsize = %d\n"
        "caretline_color = 0x%08X\n"
        "caretline_bgcolor = 0x%08X\n"
        "caretline_bold = %d\n"
        "indicator_font = \"%s\"\n"
        "indicator_fontsize = %d\n"
        "indicator_color = 0x%08X\n"
        "indicator_bgcolor = 0x%08X\n"
        "indicator_bold = %d\n"
        "keywords_font = \"%s\"\n"
        "keywords_fontsize = %d\n"
        "keywords_color = 0x%08X\n"
        "keywords_bgcolor = 0x%08X\n"
        "keywords_bold = %d\n"
        "keywords2_font = \"%s\"\n"
        "keywords2_fontsize = %d\n"
        "keywords2_color = 0x%08X\n"
        "keywords2_bgcolor = 0x%08X\n"
        "keywords2_bold = %d\n"
        "string_font = \"%s\"\n"
        "string_fontsize = %d\n"
        "string_color = 0x%08X\n"
        "string_bgcolor = 0x%08X\n"
        "string_bold = %d\n"
        "character_font = \"%s\"\n"
        "character_fontsize = %d\n"
        "character_color = 0x%08X\n"
        "character_bgcolor = 0x%08X\n"
        "character_bold = %d\n"
        "number_font = \"%s\"\n"
        "number_fontsize = %d\n"
        "number_color = 0x%08X\n"
        "number_bgcolor = 0x%08X\n"
        "number_bold = %d\n"
        "operator_font = \"%s\"\n"
        "operator_fontsize = %d\n"
        "operator_color = 0x%08X\n"
        "operator_bgcolor = 0x%08X\n"
        "operator_bold = %d\n"
        "preprocessor_font = \"%s\"\n"
        "preprocessor_fontsize = %d\n"
        "preprocessor_color = 0x%08X\n"
        "preprocessor_bgcolor = 0x%08X\n"
        "preprocessor_bold = %d\n"
        "comment_font = \"%s\"\n"
        "comment_fontsize = %d\n"
        "comment_color = 0x%08X\n"
        "comment_bgcolor = 0x%08X\n"
        "comment_bold = %d\n"
        "commentline_font = \"%s\"\n"
        "commentline_fontsize = %d\n"
        "commentline_color = 0x%08X\n"
        "commentline_bgcolor = 0x%08X\n"
        "commentline_bold = %d\n"
        "commentdoc_font = \"%s\"\n"
        "commentdoc_fontsize = %d\n"
        "commentdoc_color = 0x%08X\n"
        "commentdoc_bgcolor = 0x%08X\n"
        "commentdoc_bold = %d\n"
        "tags_font = \"%s\"\n"
        "tags_fontsize = %d\n"
        "tags_color = 0x%08X\n"
        "tags_bgcolor = 0x%08X\n"
        "tags_bold = %d\n"
        "unknowtags_font = \"%s\"\n"
        "unknowtags_fontsize = %d\n"
        "unknowtags_color = 0x%08X\n"
        "unknowtags_bgcolor = 0x%08X\n"
        "unknowtags_bold = %d\n"
        "attributes_font = \"%s\"\n"
        "attributes_fontsize = %d\n"
        "attributes_color = 0x%08X\n"
        "attributes_bgcolor = 0x%08X\n"
        "attributes_bold = %d\n"
        "unknowattributes_font = \"%s\"\n"
        "unknowattributes_fontsize = %d\n"
        "unknowattributes_color = 0x%08X\n"
        "unknowattributes_bgcolor = 0x%08X\n"
        "unknowattributes_bold = %d\n"
        "entities_font = \"%s\"\n"
        "entities_fontsize = %d\n"
        "entities_color = 0x%08X\n"
        "entities_bgcolor = 0x%08X\n"
        "entities_bold = %d\n"
        "tagends_font = \"%s\"\n"
        "tagends_fontsize = %d\n"
        "tagends_color = 0x%08X\n"
        "tagends_bgcolor = 0x%08X\n"
        "tagends_bold = %d\n"
        "cdata_font = \"%s\"\n"
        "cdata_fontsize = %d\n"
        "cdata_color = 0x%08X\n"
        "cdata_bgcolor = 0x%08X\n"
        "cdata_bold = %d\n"
        "phpsection_font = \"%s\"\n"
        "phpsection_fontsize = %d\n"
        "phpsection_color = 0x%08X\n"
        "phpsection_bgcolor = 0x%08X\n"
        "phpsection_bold = %d\n"
        "aspsection_font = \"%s\"\n"
        "aspsection_fontsize = %d\n"
        "aspsection_color = 0x%08X\n"
        "aspsection_bgcolor = 0x%08X\n"
        "aspsection_bold = %d\n"
        "activetab_font = \"%s\"\n"
        "activetab_fontsize = %d\n"
        "activetab_color = 0x%08X\n"
        "activetab_bgcolor = 0x%08X\n"
        "activetab_bold = %d\n"
        "caret_font = \"%s\"\n"
        "caret_fontsize = %d\n"
        "caret_color = 0x%08X\n"
        "caret_bgcolor = 0x%08X\n"
        "caret_bold = %d\n"
        "symbolic_font = \"%s\"\n"
        "symbolic_fontsize = %d\n"
        "symbolic_color = 0x%08X\n"
        "symbolic_bgcolor = 0x%08X\n"
        "symbolic_bold = %d\n"
        "hyperlink_font = \"%s\"\n"
        "hyperlink_fontsize = %d\n"
        "hyperlink_color = 0x%08X\n"
        "hyperlink_bgcolor = 0x%08X\n"
        "hyperlink_bold = %d\n"
        "results_font = \"%s\"\n"
        "results_fontsize = %d\n"
        "results_color = 0x%08X\n"
        "results_bgcolor = 0x%08X\n"
        "results_bold = %d";
    if (!g_theme)
    {
        return;
    }
    else
    {
        len = (int)strlen(pconfig) * 3;
    }
    if ((save = (char *)calloc(1, len + 1)) == NULL)
    {
        return;
    }
    _snprintf(save, len, pconfig,
        EXPAND_STYLETHEME(linenumber),
        EXPAND_STYLETHEME(foldmargin),
        EXPAND_STYLETHEME(text),
        EXPAND_STYLETHEME(caretline),
        EXPAND_STYLETHEME(indicator),
        EXPAND_STYLETHEME(keywords0),
        EXPAND_STYLETHEME(keywords1),
        EXPAND_STYLETHEME(string),
        EXPAND_STYLETHEME(character),
        EXPAND_STYLETHEME(number),
        EXPAND_STYLETHEME(operators),
        EXPAND_STYLETHEME(preprocessor),
        EXPAND_STYLETHEME(comment),
        EXPAND_STYLETHEME(commentline),
        EXPAND_STYLETHEME(commentdoc),
        EXPAND_STYLETHEME(tags),
        EXPAND_STYLETHEME(unknowtags),
        EXPAND_STYLETHEME(attributes),
        EXPAND_STYLETHEME(unknowattributes),
        EXPAND_STYLETHEME(entities),
        EXPAND_STYLETHEME(tagends),
        EXPAND_STYLETHEME(cdata),
        EXPAND_STYLETHEME(phpsection),
        EXPAND_STYLETHEME(aspsection),
        EXPAND_STYLETHEME(activetab),
        EXPAND_STYLETHEME(caret),
        EXPAND_STYLETHEME(symbolic),
        EXPAND_STYLETHEME(hyperlink),
        EXPAND_STYLETHEME(results));
    if ((path = eu_utf8_utf16(g_theme->pathfile, NULL)) != NULL)
    {
        if ((fp = _wfopen(path , L"wb")) != NULL)
        {
            fwrite(save, strlen(save), 1, fp);
            fclose(fp);
        }
        free(path);
    }
    free(save);
}

bool
eu_init_calltip_tree(doctype_t *p, const char *key, const char *val)
{
    if (!(p && key && val))
    {
        return false;
    }
    return !map_put(&p->ctshow_tree, key, val);
}

const char *
eu_query_calltip_tree(root_t *root, const char *key)
{
    map_t *node = map_get(root, key);
    if (node)
    {
        return node->val;
    }
    return NULL;
}

void
eu_print_calltip_tree(root_t *root)
{
    map_t *node = NULL;
    for (node = map_first(root); node != NULL;)
    {
        node = map_next(&(node->node));
        if (!node)
        {
            break;
        }
    }
}

void
eu_destory_calltip_tree(root_t *root)
{
    map_destory(root);
}

void
eu_lua_calltip(const char *pstr)
{
    eu_tabpage *p = NULL;
    if (pstr && (p = on_tabpage_focus_at()) && !p->hex_mode && !p->pmod)
    {
        const sptr_t end = eu_sci_call(p, SCI_GETSELECTIONEND, 0, 0);
        eu_sci_call(p, SCI_SETEMPTYSELECTION, end, 0);
        if (stricmp(pstr, "NaN") == 0 || stricmp(pstr, "INFINITY") == 0)
        {
            eu_sci_call(p, SCI_CALLTIPSHOW, end, (sptr_t) pstr);
        }
        else
        {
            char *text = NULL;
            int ch = (int) eu_sci_call(p, SCI_GETCHARAT, end, 0);
            if (ch == 0x3d)
            {
                eu_sci_call(p, SCI_INSERTTEXT, end + 1, (sptr_t)pstr);
                eu_sci_call(p, SCI_GOTOPOS, end + 1 + strlen(pstr), 0);
            }
            else if ((text = (char *)calloc(1, QW_SIZE + 1)))
            {
                _snprintf(text, QW_SIZE, "=%s", pstr);
                eu_sci_call(p, SCI_INSERTTEXT, end, (sptr_t)text);
                eu_sci_call(p, SCI_GOTOPOS, end + strlen(text), 0);
                free(text);
            }
        }
    }
}

bool
eu_init_completed_tree(doctype_t *p, const char *str)
{
    if (!p || STR_IS_NUL(str))
    {
        return false;
    }
    return !ac_put(&p->acshow_tree, str);
}


void
eu_print_completed_tree(root_t *acshow_root)
{
    acshow_t *node = NULL;
    for (node = ac_first(acshow_root); node != NULL;)
    {
        node = ac_next(&(node->node));
        if (!node)
        {
            break;
        }
    }
}

char *
eu_find_completed_tree(root_t *acshow_root, const char *key, const char *pre_str)
{
    char *res = (char *)calloc(VALUE_LEN + 1, 1);
    if (res)
    {
        if (pre_str)
        {
            _snprintf(res, VALUE_LEN, "%s%c%d", pre_str, '\x1E', SNIPPET_FUNID);
        }
        else
        {
            ac_get(acshow_root, key, res, VALUE_LEN);
            size_t len = strlen(res);
            if (len > 0 && res[len - 1] == 0x20)
            {
                res[len - 1] = 0;
            }
        }
        return res;
    }
    return NULL;
}

void
eu_destory_completed_tree(root_t *root)
{
    ac_destory(root);
}

static pcre_conainer *
pcre_container_new(void)
{
    pcre_conainer *p = NULL;
    return (p = calloc(1, sizeof(pcre_conainer)));
}

// 销毁pcre结构体
void
eu_pcre_delete(pcre_conainer *pcre_info)
{
    if (!pcre_info)
    {
        return;
    }
    pcre_free(pcre_info->re);
    free(pcre_info);
    pcre_info = NULL;
}

// 初始化pcre结构
pcre_conainer *
eu_pcre_init(const char *buf, size_t len, const char *pattern, const char *named_substring, int opt)
{
    pcre_conainer *pc = pcre_container_new();
    if (!(pc && buf && pattern))
    {
        return NULL;
    }
    pc->buffer = buf;
    pc->buffer_length = len;
    pc->pattern = pattern;
    pc->named_substring = named_substring;
    pc->opt = opt;
    return pc;
}

int
eu_pcre_named_substring(const char *named_substring, pcre_conainer *pcre_info, const char **matched_substring)
{
    int rs = pcre_get_named_substring(pcre_info->re,      // 预编译过的正则表达式
                                      pcre_info->buffer,  // 匹配的串
                                      pcre_info->ovector, // pcre_exec() 使用的偏移向量
                                      pcre_info->rc,      // pcre_exec()的返回值
                                      named_substring,    // 捕获字串的名字, 例如 (?P<str>)
                                      matched_substring); // 存放结果的字符串指针
    return rs;
}

int
eu_pcre_exec_single(pcre_conainer *pcre_info, ptr_recallback callback, void *para)
{
    pcre_info->re = pcre_compile(pcre_info->pattern,                      // 包含字串的结构体
                                 PCRE_MULTILINE|pcre_info->opt,           // 标准, 匹配多行, 不区分大小写
                                 &(pcre_info->error),                     // 错误消息
                                 &(pcre_info->erroroffset),               // 偏移
                                 NULL);

    if (pcre_info->re == NULL)
    {   // 正则表达式预编译失败
        if (pcre_info->error)
        {
            eu_logmsg("PCRE compilation failed at offset %d: %s\n", pcre_info->erroroffset, pcre_info->error);
        }
        return 1;
    }
    pcre_info->rc = pcre_exec(pcre_info->re,
                              NULL,
                              pcre_info->buffer,
                              (int)pcre_info->buffer_length,
                              0,
                              0,
                              pcre_info->ovector,
                              OVECCOUNT);

    if (pcre_info->rc < 0)
    {   // 没有匹配, 输出消息
        switch (pcre_info->rc)
        {
            case PCRE_ERROR_NOMATCH:
                eu_logmsg("pcre: no match.\n");
                break;
            default:
                eu_logmsg("pcre: matching error.\n");
                break;
        }
        return 1;
    }

    if (pcre_info->rc == 0)
    {   // 处理偏移量的数组不够大, 输出错误
        pcre_info->rc = OVECCOUNT / 3;
        eu_logmsg("error: ovector only has room for %d substrings\n", (pcre_info->rc) - 1);
        return 1;
    }

    (void) pcre_fullinfo(pcre_info->re, NULL, PCRE_INFO_NAMECOUNT, &(pcre_info->namecount));

    // 存在匹配, 运行回调函数
    if (callback(pcre_info, para))
    {
        return 1;
    }
#if PCRE_DEBUG
    printf("Match succeeded at offset %d\n", pcre_info->ovector[0]);
    // debug 模式下输出详细内容
    int i;
    for (i = 0; i < (pcre_info->rc); i++)
    {
        const char *substring_start = pcre_info->buffer + pcre_info->ovector[2 * i];
        int substring_length = pcre_info->ovector[2 * i + 1] - pcre_info->ovector[2 * i];
        printf("%2d: %.*s\n", i, substring_length, substring_start);
    }
    if (pcre_info->namecount > 0)
    {
        int name_entry_size;

        (void) pcre_fullinfo(pcre_info->re,           // 预编译过的正则表达式
                             NULL,                    // 没有额外的数据, 因为我们不使用pcre_study
                             PCRE_INFO_NAMEENTRYSIZE, // 表中每个条目的大小
                             &name_entry_size);       // 存储应答消息的位置
        unsigned char *name_table;

        (void) pcre_fullinfo(pcre_info->re,
                             NULL,                // no extra data - we didn't study the pattern
                             PCRE_INFO_NAMETABLE, // address of the table
                             &name_table);        // where to put the answer

        unsigned char *tabptr = name_table;
        printf("Named substrings\n");
        for (i = 0; i < pcre_info->namecount; i++)
        {
            int n = (tabptr[0] << 8) | tabptr[1];
            printf("(%d) %*s: %.*s\n",
                   n,
                   name_entry_size - 3,
                   tabptr + 2,
                   pcre_info->ovector[2 * n + 1] - pcre_info->ovector[2 * n],
                   pcre_info->buffer + pcre_info->ovector[2 * n]);
            tabptr += name_entry_size;
        }
    }
#endif
    return 0;
}

int
eu_pcre_exec_multi(pcre_conainer *pcre_info, ptr_recallback callback, void *para)
{
    int utf8;
    unsigned int option_bits;
    if (eu_pcre_exec_single(pcre_info, callback, para) > 0)
    {
        return 1;
    }
    (void) pcre_fullinfo(pcre_info->re, NULL, PCRE_INFO_OPTIONS, &option_bits);
    utf8 = option_bits & PCRE_UTF8;
    option_bits &= PCRE_NEWLINE_CR | PCRE_NEWLINE_LF | PCRE_NEWLINE_CRLF | PCRE_NEWLINE_ANY | PCRE_NEWLINE_ANYCRLF;
    if (option_bits == 0)
    {
        int d;
        (void) pcre_config(PCRE_CONFIG_NEWLINE, &d);
        option_bits = (d == 13) ? PCRE_NEWLINE_CR :
                                  (d == 10) ?
                                  PCRE_NEWLINE_LF :
                                  (d == (13 << 8 | 10)) ? PCRE_NEWLINE_CRLF :
                                  (d == -2) ? PCRE_NEWLINE_ANYCRLF : (d == -1) ? PCRE_NEWLINE_ANY : 0;
    }
    int crlf_is_newline;
    crlf_is_newline = option_bits == PCRE_NEWLINE_ANY || option_bits == PCRE_NEWLINE_CRLF || option_bits == PCRE_NEWLINE_ANYCRLF;
    // 如果需要
    // 循环进行第二次和后续匹配
    for (;;)
    {
        int options = 0;
        int start_offset = pcre_info->ovector[1];
        if (pcre_info->ovector[0] == pcre_info->ovector[1])
        {
            if (pcre_info->ovector[0] == pcre_info->buffer_length)
            {
                break;
            }
            options = PCRE_NOTEMPTY_ATSTART | PCRE_ANCHORED;
        }
        // 下一个匹配操作
        pcre_info->rc =
            pcre_exec(pcre_info->re, NULL, pcre_info->buffer, (int)pcre_info->buffer_length, start_offset, options, pcre_info->ovector, OVECCOUNT);

        if (pcre_info->rc == PCRE_ERROR_NOMATCH)
        {
            if (options == 0)
            {
                // 如果选项为0，则我们已找到所有可能的匹配项
                break;
            }
            pcre_info->ovector[1] = start_offset + 1; // advance one byte
            if (crlf_is_newline && start_offset < (int)pcre_info->buffer_length - 1 && pcre_info->buffer[start_offset] == '\r' &&
                pcre_info->buffer[start_offset + 1] == '\n')
            {
                pcre_info->ovector[1] += 1;
            }
            else if (utf8)
            {
                while (pcre_info->ovector[1] < (int)pcre_info->buffer_length)
                {
                    if ((pcre_info->buffer[pcre_info->ovector[1]] & 0xc0) != 0x80)
                    {
                        break;
                    }
                    pcre_info->ovector[1] += 1;
                }
            }
            continue;
        }
        if (pcre_info->rc < 0)
        {
            eu_logmsg("pcre: Matching error %d\n", pcre_info->rc);
            return 1;
        }
        if (pcre_info->rc == 0)
        {
            pcre_info->rc = OVECCOUNT / 3;
            eu_logmsg("pcre: ovector only has room for %d captured substrings\n", pcre_info->rc - 1);
        }

    #if PCRE_DEBUG
        // As before, show substrings stored in the output vector
        // by number, and then also any named substrings.

        printf("\nMatch succeeded again at offset %d\n", pcre_info->ovector[0]);

        int i;
        for (i = 0; i < pcre_info->rc; i++)
        {
            const char *substring_start = pcre_info->buffer + pcre_info->ovector[2 * i];
            int substring_length = pcre_info->ovector[2 * i + 1] - pcre_info->ovector[2 * i];
            printf("%2d: %.*s\n", i, substring_length, substring_start);
        }
    #endif

        (void) pcre_fullinfo(pcre_info->re, NULL, PCRE_INFO_NAMECOUNT, &(pcre_info->namecount));

        // We have a match, run callback
        if (callback(pcre_info, para))
        {
            break;
        }

    #ifdef PCRE_DEBUG
        if (pcre_info->namecount > 0)
        {
            int name_entry_size;

            (void) pcre_fullinfo(pcre_info->re,
                                 NULL,
                                 PCRE_INFO_NAMEENTRYSIZE,
                                 &name_entry_size);
            unsigned char *name_table;

            (void) pcre_fullinfo(pcre_info->re,
                                 NULL,
                                 PCRE_INFO_NAMETABLE,
                                 &name_table);

            unsigned char *tabptr = name_table;
            printf("Named substrings\n");
            for (i = 0; i < pcre_info->namecount; i++)
            {
                int n = (tabptr[0] << 8) | tabptr[1];
                printf("(%d) %*s: %.*s\n",
                       n,
                       name_entry_size - 3,
                       tabptr + 2,
                       pcre_info->ovector[2 * n + 1] - pcre_info->ovector[2 * n],
                       pcre_info->buffer + pcre_info->ovector[2 * n]);
                tabptr += name_entry_size;
            }
        }
    #endif
    }
    return 0;
}

int
eu_sci_register(HINSTANCE hinstance)
{
    return Scintilla_RegisterClasses(hinstance);
}

int
eu_iconvctl(iconv_t cd, int request, void* argument)
{
    return iconvctl(cd, request, argument);
}

iconv_t
eu_iconv_open (const char* tocode, const char* fromcode)
{
    return iconv_open(tocode, fromcode);
}

size_t
eu_iconv(iconv_t cd, char** inbuf, size_t *inbytesleft, char** outbuf, size_t *outbytesleft)
{
    return iconv(cd,  inbuf, inbytesleft, outbuf, outbytesleft);
}

int
eu_iconv_close(iconv_t cd)
{
    if (cd != (iconv_t)-1 && cd != 0)
    {
        iconv_close(cd);
    }
    return 0;
}

/*******************************************************************
 * 加载curl动态库与初始化curl全局变量
 * 在最新的 7.84 版本, curl_global_init已经默认支持线程安全
 * 函数调用失败, 返回 EUE_CURL_INIT_FAIL
 * 成功, 返回值与 curl_global_init 函数相同
 *******************************************************************/
int
eu_curl_global_init(long flags)
{
    int result = CURLE_OK;
    if (!InterlockedCompareExchange(&eu_curl_initialized, 1, 0))
    {
        if ((eu_curl_symbol = np_load_plugin_library(_T("libcurl.dll"), false)) != NULL)
        {
            fn_curl_global_init = (ptr_curl_global_init)GetProcAddress(eu_curl_symbol,"curl_global_init");
            fn_curl_easy_init = (ptr_curl_easy_init)GetProcAddress(eu_curl_symbol,"curl_easy_init");
            fn_curl_global_cleanup = (ptr_curl_global_cleanup)GetProcAddress(eu_curl_symbol,"curl_global_cleanup");
            fn_curl_easy_cleanup = (ptr_curl_easy_cleanup)GetProcAddress(eu_curl_symbol,"curl_easy_cleanup");
            eu_curl_easy_setopt = (ptr_curl_easy_setopt)GetProcAddress(eu_curl_symbol,"curl_easy_setopt");
            eu_curl_easy_perform = (ptr_curl_easy_perform)GetProcAddress(eu_curl_symbol,"curl_easy_perform");
            eu_curl_slist_append = (ptr_curl_slist_append)GetProcAddress(eu_curl_symbol,"curl_slist_append");
            eu_curl_slist_free_all = (ptr_curl_slist_free_all)GetProcAddress(eu_curl_symbol,"curl_slist_free_all");
            eu_curl_easy_getinfo = (ptr_curl_easy_getinfo)GetProcAddress(eu_curl_symbol,"curl_easy_getinfo");
            eu_curl_easy_strerror = (ptr_curl_easy_strerror)GetProcAddress(eu_curl_symbol,"curl_easy_strerror");
        }
        if (!(fn_curl_global_init && fn_curl_easy_init && fn_curl_global_cleanup && eu_curl_easy_setopt && eu_curl_easy_perform &&
              fn_curl_easy_cleanup && eu_curl_slist_append && eu_curl_slist_free_all && eu_curl_easy_getinfo && eu_curl_easy_strerror)
           )
        {
            result = EUE_CURL_INIT_FAIL;
            eu_close_dll(eu_curl_symbol);
            _InterlockedExchange(&eu_curl_initialized, 0);
        }
        else
        {
            result = fn_curl_global_init(flags);
        }
    }
    return result;
}

CURL *
eu_curl_easy_init(void)
{
    if (!eu_curl_initialized)
    {
        if (eu_curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK)
        {
            return NULL;
        }
    }
    return fn_curl_easy_init();
}

void
eu_curl_easy_cleanup(CURL *curl)
{
    if (eu_curl_initialized)
    {
        fn_curl_easy_cleanup(curl);
    }
}

void
eu_curl_global_cleanup(void)
{
    if (eu_curl_initialized)
    {
        if (fn_curl_global_cleanup)
        {
            fn_curl_global_cleanup();
        }
        if (eu_curl_symbol)
        {
            FreeLibrary(eu_curl_symbol);
            eu_curl_symbol = NULL;
            fn_curl_global_init = NULL;
            fn_curl_easy_init = NULL;
            fn_curl_global_cleanup = NULL;
            fn_curl_easy_cleanup = NULL;
            eu_curl_easy_setopt = NULL;
            eu_curl_easy_perform = NULL;
            eu_curl_easy_getinfo = NULL;
            eu_curl_slist_append = NULL;
            eu_curl_slist_free_all = NULL;
        }
        _InterlockedExchange(&eu_curl_initialized, 0);
    }
}

HINSTANCE
eu_module_handle(void)
{
    if (!eu_instance)
    {
        eu_instance = (HINSTANCE) GetModuleHandle(NULL);
    }
    return eu_instance;
}

void
eu_restore_placement(HWND hwnd)
{
    util_restore_placement(hwnd);
}

const uint32_t
eu_win10_or_later(void)
{
    uint32_t major, minor, patch = (uint32_t)-1;
    RtlGetNtVersionNumbersPtr fnRtlGetNtVersionNumbers = NULL;
    if (!(fnRtlGetNtVersionNumbers = (RtlGetNtVersionNumbersPtr)GetProcAddress(GetModuleHandle(_T("ntdll.dll")), "RtlGetNtVersionNumbers")))
    {
        return (uint32_t)-1;
    }
    fnRtlGetNtVersionNumbers(&major, &minor, &patch);
    patch &= ~0xf0000000;
    if (major == 10 && minor == 0 && patch >= 10240)
    {
        return patch;
    }
    return (uint32_t)-1;
}

const int
eu_theme_index(void)
{
    TCHAR pname[QW_SIZE+1] = {0};
    if (!g_config)
    {
        return (const int)THEME_UNUSABLE;
    }
    if (!MultiByteToWideChar(CP_UTF8, 0, g_config->window_theme, -1, pname, QW_SIZE) || !pname[0])
    {
        return (const int)THEME_UNUSABLE;
    }
    if (_tcscmp(pname, _T("default")) == 0)
    {
        return (const int)THEME_DEFAULT;
    }
    else if (_tcscmp(pname, _T("white")) == 0)
    {
        return (const int)THEME_WHITE;
    }
    else if (_tcscmp(pname, _T("black")) == 0)
    {
        return (const int)THEME_BLACK;
    }
    else
    {
        return (const int)THEME_OTHER;
    }
    return (const int)THEME_UNUSABLE;
}

double
eu_te_eval(const te_expr *n)
{
    return te_eval(n);
}

double
eu_te_interp(const char *expression, int *error)
{
    return te_interp(expression, error);
}

void
eu_te_print(const te_expr *n)
{
    te_print(n);
}

void
eu_te_free(te_expr *n)
{
    te_free(n);
}

te_expr *
eu_te_compile(const char *expression, const te_variable *variables, int var_count, int *error)
{
    return (void *)te_compile(expression, variables, var_count, error);
}
