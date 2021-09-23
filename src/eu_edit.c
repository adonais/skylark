/*******************************************************************************
 * This file is part of Skylark project
 * Copyright ©2021 Hua andy <hua.andy@gmail.com>

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

#include "framework.h"

enum htmlblock
{
    HTML_TEXT_BLOCK_TAG = 0,
    HTML_TEXT_BLOCK_CDATA,
    HTML_TEXT_BLOCK_JS,
    HTML_TEXT_BLOCK_VBS,
    HTML_TEXT_BLOCK_PYTHON,
    HTML_TEXT_BLOCK_PHP,
    HTML_TEXT_BLOCK_CSS,
    HTML_TEXT_BLOCK_SGML
};

char eols_undo_str[ACNAME_LEN] = {0};

void
on_edit_undo(eu_tabpage *pnode)
{
    if (pnode)
    {
        eu_clear_undo_off();
        eu_sci_call(pnode, SCI_UNDO, 0, 0);
        util_setforce_eol(pnode);
        on_statusbar_update_eol(pnode);
    }
}

void
on_edit_redo(eu_tabpage *pnode)
{
    if (pnode)
    {
        eu_clear_undo_off();
        eu_sci_call(pnode, SCI_REDO, 0, 0);
        util_setforce_eol(pnode);
        on_statusbar_update_eol(pnode);
    }
}

void
on_edit_cut(eu_tabpage *pnode)
{
    if (pnode)
    {
        eu_sci_call(pnode, SCI_CUT, 0, 0);
    }
}

void
on_edit_copy_text(eu_tabpage *pnode)
{
    if (pnode)
    {
        eu_sci_call(pnode, SCI_COPY, 0, 0);
    }
}

void
on_edit_paste_text(eu_tabpage *pnode)
{
    if (pnode)
    {
        eu_sci_call(pnode, SCI_PASTE, 0, 0);
    }
}

void
on_edit_delete_text(eu_tabpage *pnode)
{
    if (pnode && !pnode->hex_mode)
    {
        eu_sci_call(pnode, SCI_CLEAR, 0, 0);
    }
}

void
on_edit_cut_line(eu_tabpage *pnode)
{
    if (pnode && !pnode->hex_mode)
    {
        eu_sci_call(pnode, SCI_LINECUT, 0, 0);
    }
}

void
on_edit_cut_line_paste(eu_tabpage *pnode)
{
    if (pnode && !pnode->hex_mode)
    {
        eu_sci_call(pnode, SCI_BEGINUNDOACTION, 0, 0);
        on_edit_cut_line(pnode);
        on_edit_paste_line(pnode);
        eu_sci_call(pnode, SCI_ENDUNDOACTION, 0, 0);
    }
}

void
on_edit_copy_line(eu_tabpage *pnode)
{
    if (pnode && !pnode->hex_mode)
    {
        eu_sci_call(pnode, SCI_LINECOPY, 0, 0);
    }
}

void
on_edit_copy_line_paste(eu_tabpage *pnode)
{
    if (pnode && !pnode->hex_mode)
    {
        eu_sci_call(pnode, SCI_SELECTIONDUPLICATE, 0, 0);
    }
}

bool
on_edit_push_clipboard(const TCHAR *buf)
{
    TCHAR *on_edit_copy_text = NULL;
    HGLOBAL hgl = (TCHAR *) GlobalAlloc(GMEM_MOVEABLE, (_tcslen(buf) + 1) * sizeof(TCHAR));
    if (!hgl)
    {
        printf("GlobalAlloc(GMEM_MOVEABLE) failed, cause:%lu\n", GetLastError());
        return false;
    }
    on_edit_copy_text = (TCHAR *) GlobalLock(hgl);
    if (on_edit_copy_text == NULL)
    {
        printf("GlobalLock(hgl) failed, cause:%lu\n", GetLastError());
        GlobalFree(hgl);
        return false;
    }
    _tcscpy(on_edit_copy_text, buf);
    GlobalUnlock(hgl);
    if (OpenClipboard(eu_module_hwnd()))
    {
        EmptyClipboard();
        SetClipboardData(CF_UNICODETEXT, on_edit_copy_text);
        CloseClipboard();
    }
    return true;
}

void
on_edit_copy_filename(TCHAR *filename)
{
    TCHAR *name = _tcsdup(filename);
    if (name)
    {
        if (name[_tcslen(name) - 1] == _T('*'))
        {
            name[_tcslen(name) - 1] = 0;
        }
        on_edit_push_clipboard(name);
        free(name);
    }
}

void
on_edit_paste_line(eu_tabpage *pnode)
{
    if (pnode && !pnode->hex_mode)
    {
        sptr_t pos;
        sptr_t current_line;
        sptr_t next_pos;
        pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
        current_line = eu_sci_call(pnode, SCI_LINEFROMPOSITION, pos, 0);
        next_pos = eu_sci_call(pnode, SCI_POSITIONFROMLINE, current_line + 1, 0);
        eu_sci_call(pnode, SCI_GOTOPOS, next_pos, 0);
        eu_sci_call(pnode, SCI_PASTE, 0, 0);
        eu_sci_call(pnode, SCI_GOTOPOS, next_pos, 0);
    }
}

void
on_edit_paste_line_up(eu_tabpage *pnode)
{
    if (pnode && !pnode->hex_mode)
    {
        sptr_t pos;
        sptr_t current_line;
        sptr_t start_pos;
        pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
        current_line = eu_sci_call(pnode, SCI_LINEFROMPOSITION, pos, 0);
        start_pos = eu_sci_call(pnode, SCI_POSITIONFROMLINE, current_line, 0);
        eu_sci_call(pnode, SCI_GOTOPOS, start_pos, 0);
        eu_sci_call(pnode, SCI_PASTE, 0, 0);
        eu_sci_call(pnode, SCI_GOTOPOS, start_pos, 0);
    }
}

void
on_edit_delete_line(eu_tabpage *pnode)
{
    if (pnode && !pnode->hex_mode)
    {
        eu_sci_call(pnode, SCI_LINEDELETE, 0, 0);
    }
}

static sptr_t
count_line_space(eu_tabpage *pnode, sptr_t start, sptr_t end, bool header)
{
    sptr_t count = 0;
    char *line_buf = on_sci_range_text(pnode, start, end);
    if (!line_buf)
    {
        return count;
    }
    if (header)
    {
        for (int i = 0; i < (int)(end - start); ++i, ++count)
        {
            if (!isspace(line_buf[i]))
            {
                break;
            }
        }
    }
    else
    {
        sptr_t str_len = end - start;
        for (count = 0; count < str_len; ++count)
        {
            if (!(line_buf[str_len - 1 - count] == ' ' || line_buf[str_len - 1 - count] == '\t'))
            {
                break;
            }
        }
    }
    free(line_buf);
    return count;
}

static bool
do_delete_space(eu_tabpage *pnode, sptr_t start, sptr_t end, bool header)
{
    eu_sci_call(pnode, SCI_BEGINUNDOACTION, 0, 0);
    for (sptr_t line = start; line <= end; ++line)
    {
        sptr_t start_pos, end_pos, str_len, del_len;
        start_pos = eu_sci_call(pnode, SCI_POSITIONFROMLINE, line, 0);
        end_pos = eu_sci_call(pnode, SCI_GETLINEENDPOSITION, line, 0);
        str_len = end_pos - start_pos;
        if (str_len == 0)
        {
            continue;
        }
        else
        {
            del_len = count_line_space(pnode, start_pos, end_pos, header);
        }
        if (del_len > 0)
        {
            if (header)
            {
                eu_sci_call(pnode, SCI_DELETERANGE, start_pos, del_len);
            }
            else
            {
                eu_sci_call(pnode, SCI_DELETERANGE, end_pos - del_len, del_len);
            }
        }
    }
    eu_sci_call(pnode, SCI_ENDUNDOACTION, 0, 0);
    return true;
}

void
on_edit_delete_line_header_white(eu_tabpage *pnode)
{
    if (pnode && !pnode->hex_mode)
    {
        sptr_t start_line, end_line;
        util_effect_line(pnode, &start_line, &end_line);
        do_delete_space(pnode, start_line, end_line, true);
    }
}

void
on_edit_delete_line_tail_white(eu_tabpage *pnode)
{
    if (pnode && !pnode->hex_mode)
    {
        sptr_t start_line, end_line;
        util_effect_line(pnode, &start_line, &end_line);
        do_delete_space(pnode, start_line, end_line, false);
    }
}

static bool
space_in_line(eu_tabpage *pnode, sptr_t start, sptr_t end)
{
    bool res = false;
    char *line_buf = on_sci_range_text(pnode, start, end);
    if (!line_buf)
    {
        return false;
    }
    for (int i = 0; i < (int)(end - start); ++i)
    {
        if (!(res = !!isspace(line_buf[i])))
        {
            break;
        }
    }
    free(line_buf);
    return res;
}

static bool
do_delete_lines(eu_tabpage *pnode, sptr_t start, sptr_t end, bool white_chars)
{
    if (!pnode || pnode->hex_mode)
    {
        return false;
    }
    eu_sci_call(pnode, SCI_BEGINUNDOACTION, 0, 0);
    for (sptr_t line = start; line <= end; ++line)
    {
        sptr_t start_pos, end_pos, str_len, line_len;
        start_pos = eu_sci_call(pnode, SCI_POSITIONFROMLINE, line, 0);
        end_pos = eu_sci_call(pnode, SCI_GETLINEENDPOSITION, line, 0);
        str_len = end_pos - start_pos;
        line_len = eu_sci_call(pnode, SCI_LINELENGTH, line, 0);
        if (str_len == 0 || (white_chars && space_in_line(pnode, start_pos, end_pos)))
        {
            eu_sci_call(pnode, SCI_DELETERANGE, start_pos, line_len);
            line--;
            end--;
        }
    }
    eu_sci_call(pnode, SCI_ENDUNDOACTION, 0, 0);
    return true;
}

void
on_edit_delete_blank_line(eu_tabpage *pnode)
{
    if (pnode && !pnode->hex_mode)
    {
        sptr_t start_line, end_line;
        util_effect_line(pnode, &start_line, &end_line);
        do_delete_lines(pnode, start_line, end_line, false);
    }
}

void
on_edit_delete_all_empty_lines(eu_tabpage *pnode)
{
    if (pnode && !pnode->hex_mode)
    {
        sptr_t end_line = eu_sci_call(pnode, SCI_GETLINECOUNT, 0, 0);
        if (end_line > 0)
        {
            do_delete_lines(pnode, 0, end_line, true);
        }
    }
}

void
on_edit_join_line(eu_tabpage *pnode)
{
    if (pnode && !pnode->hex_mode)
    {
        int pos;
        int current_line;
        int cpos;
        pos = (int) eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
        current_line = (int) eu_sci_call(pnode, SCI_LINEFROMPOSITION, pos, 0);
        cpos = (int) eu_sci_call(pnode, SCI_GETLINEENDPOSITION, current_line, 0);
        eu_sci_call(pnode, SCI_GOTOPOS, cpos, 0);
        eu_sci_call(pnode, SCI_CLEAR, 0, 0);
        eu_sci_call(pnode, SCI_GOTOPOS, pos, 0);
    }
}

void
on_edit_lower(eu_tabpage *pnode)
{
    if (pnode)
    {
        eu_sci_call(pnode, SCI_LOWERCASE, 0, 0);
    }
}

void
on_edit_upper(eu_tabpage *pnode)
{
    if (pnode)
    {
        eu_sci_call(pnode, SCI_UPPERCASE, 0, 0);
    }
}

int
on_edit_close_char(eu_tabpage *pnode)
{
    if (!eu_get_config()->auto_close_chars)
    {
        eu_get_config()->auto_close_chars = true;
    }
    else
    {
        eu_get_config()->auto_close_chars = false;
    }
    menu_update_all(eu_module_hwnd(), pnode);
    return 0;
}

int
on_edit_identation(eu_tabpage *pnode)
{
    if (!eu_get_config()->m_ident)
    {
        eu_get_config()->m_ident = true;
    }
    else
    {
        eu_get_config()->m_ident = false;
    }
    menu_update_all(eu_module_hwnd(), pnode);
    return 0;
}

void
on_edit_selection(eu_tabpage *pnode, int type)
{
    size_t len = 0;
    char *text = NULL;
    if (!(text = util_strdup_select(pnode, &len, 0)))
    {
        return;
    }
    switch (type)
    {
        case 0:
        {
            file_backup file = {0};
            if (eu_exist_path(text) && MultiByteToWideChar(CP_UTF8, 0, text, -1, file.rel_path, MAX_PATH - 1) > 0)
            {
                uint32_t attr = GetFileAttributes(file.rel_path);
                if (!(attr & FILE_ATTRIBUTE_DIRECTORY))
                {
                    on_file_only_open(&file);
                }
            }
            break;
        }
        case 1:
        {
            TCHAR pfile[MAX_PATH] = {0};
            if (!MultiByteToWideChar(CP_UTF8, 0, text, -1, pfile, MAX_PATH - 1))
            {
                break;
            }
            if (eu_exist_dir(pfile))
            {
                on_treebar_locate_path(pfile);
                break;
            }
            eu_suffix_strip(pfile);
            if (eu_exist_dir(pfile))
            {
                on_treebar_locate_path(pfile);
            }
            break;
        }
        case 2:
        {
            char *url = NULL;
            const char *google = "https://www.google.com/search?q=%s";
            size_t url_len = strlen(google) + len + 3;
            url = (char *)calloc(1, url_len);
            if (url)
            {
                _snprintf(url, url_len-1, google, text);
                TCHAR *wurl = eu_utf8_utf16(url, NULL);
                if (wurl)
                {
                    ShellExecute(eu_module_hwnd(), NULL, wurl, NULL, NULL, SW_SHOWNORMAL);
                    free(wurl);
                }
                free(url);
            }
            break;
        }
        case 3:
        {
            char *url = NULL;
            const char *baidu = "https://www.baidu.com/s?wd=%s&ie=utf-8";
            size_t url_len = strlen(baidu) + len + 3;
            url = (char *)calloc(1, url_len);
            if (url)
            {
                _snprintf(url, url_len-1, baidu, text);
                TCHAR *wurl = eu_utf8_utf16(url, NULL);
                if (wurl)
                {
                    ShellExecute(eu_module_hwnd(), NULL, wurl, NULL, NULL, SW_SHOWNORMAL);
                    free(wurl);
                }
                free(url);
            }
            break;
        }
        case 4:
        {
            char *url = NULL;
            const char *bing = "https://www.bing.com/search?q=%s";
            size_t url_len = strlen(bing) + len + 3;
            url = (char *)calloc(1, url_len);
            if (url)
            {
                _snprintf(url, url_len-1, bing, text);
                TCHAR *wurl = eu_utf8_utf16(url, NULL);
                if (wurl)
                {
                    ShellExecute(eu_module_hwnd(), NULL, wurl, NULL, NULL, SW_SHOWNORMAL);
                    free(wurl);
                }
                free(url);
            }
            break;
        }
        default:
            break;
    }
    if (text)
    {
        free(text);
    }
}

bool
on_edit_ssl_enc_base64(unsigned char *base64_pass, unsigned char *enc_str, int enc_len)
{
    bool res = false;
    char *fn_name[1] = {"EVP_EncodeBlock"};
    uintptr_t pfunc[1] = {0};
    HMODULE hssl = eu_ssl_open_symbol(fn_name, 1, pfunc);  
    if (hssl)
    {
        if (((eu_evp_encodeblock)pfunc[0])(base64_pass, enc_str, enc_len) >= 0)
        {
            res = true;
        }
        eu_ssl_close_symbol(&hssl);
    }
    return res;
}

int
on_edit_base64_enc(eu_tabpage *pnode)
{
    int err = 0;
    size_t sel_len;
    size_t out_len;
    char *sel_text = NULL;
    char *out_text = NULL;
    if (!pnode)
    {
        return 0;
    }
    if (pnode->hex_mode)
    {
        return 0;
    }
    do
    {
        if ((sel_text = util_strdup_select(pnode, &sel_len, 0)) == NULL)
        {
            err = EUE_OUT_OF_MEMORY;
            break;
        }
        out_len = sel_len * 2 + 1;
        if ((out_text = (char *) calloc(1, out_len)) == NULL)
        {
            err = EUE_OUT_OF_MEMORY;
            break;
        }
        if (!on_edit_ssl_enc_base64((unsigned char *) out_text, (unsigned char *) sel_text, (int) sel_len))
        {
            MSG_BOX(IDC_MSG_ENC_ERR1, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
            err = EUE_OPENSSL_ENC_ERR;
            break;
        }
        eu_sci_call(pnode, SCI_REPLACESEL, 0, (sptr_t) out_text);
    }while(0);
    if (sel_text)
    {
        free(sel_text);
    }
    if (out_text)
    {
        free(out_text);
    }
    return err;
}

bool
on_edit_ssl_dec_base64(unsigned char *base64_pass, unsigned char *enc_str, int enc_len)
{
    bool res = false;
    char *fn_name[1] = {"EVP_DecodeBlock"};
    uintptr_t pfunc[1] = {0};
    HMODULE hssl = eu_ssl_open_symbol(fn_name, 1, pfunc);  
    if (hssl)
    {
        if (((eu_evp_decodeblock)pfunc[0])(base64_pass, enc_str, enc_len) >= 0)
        {
            res = true;
        }
        eu_ssl_close_symbol(&hssl);
    }
    return res;
}

int
on_edit_base64_dec(eu_tabpage *pnode)
{
    int err = 0;
    size_t sel_len;
    size_t out_len;
    char *out_text = NULL;
    char *sel_text = NULL;
    HMODULE hssl = NULL;
    if (!pnode)
    {
        return EUE_TAB_NULL;
    }
    if (pnode->hex_mode)
    {
        return EUE_UNKOWN_ERR;
    }
    do
    {
        if ((sel_text = util_strdup_select(pnode, &sel_len, 0)) == NULL)
        {
            err = EUE_OUT_OF_MEMORY;
            break;
        }
        out_len = sel_len + 1;
        if ((out_text = (char *) calloc(1, out_len)) == NULL)
        {
            err = EUE_OUT_OF_MEMORY;
            break;
        }
        if (!on_edit_ssl_dec_base64((unsigned char *) out_text, (unsigned char *) sel_text, (int) sel_len))
        {
            MSG_BOX(IDC_MSG_ENC_ERR2, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
            err = EUE_OPENSSL_DEC_ERR;
            break;
        }
        eu_sci_call(pnode, SCI_REPLACESEL, 0, (sptr_t) out_text);
    } while(0);
    if (sel_text)
    {
        free(sel_text);
    }
    if (out_text)
    {
        free(out_text);
    }
    return err;
}

int
on_edit_md5(eu_tabpage *pnode)
{
    size_t sel_len;
    char *sel_text = NULL;
    char out_text[MD5_DIGEST_LENGTH + 1] = {0};
    char text_exp[MD5_DIGEST_LENGTH * 2 + 1] = {0};
    if (!pnode)
    {
        return EUE_TAB_NULL;
    }
    if (pnode->hex_mode)
    {
        return EUE_UNKOWN_ERR;
    }    
    if ((sel_text = util_strdup_select(pnode, &sel_len, 0)) == NULL)
    {
        printf("memory allocation failed\n");
        return EUE_OUT_OF_MEMORY;
    }
    char *fn_name[1] = {"MD5"};
    uintptr_t pfunc[1] = {0};
    HMODULE hssl = eu_ssl_open_symbol(fn_name, 1, pfunc);  
    if (hssl)
    {
        ((eu_md5)pfunc[0])((unsigned char *) sel_text, (int) sel_len, (unsigned char *) out_text);
        util_hex_expand(out_text, MD5_DIGEST_LENGTH, text_exp);
        eu_sci_call(pnode, SCI_REPLACESEL, 0, (sptr_t) text_exp);
        eu_ssl_close_symbol(&hssl);
    }
    free(sel_text);
    return SKYLARK_OK;
}

int
on_edit_sha1(eu_tabpage *pnode)
{
    size_t sel_len;
    char *sel_text = NULL;
    char out_text[SHA_DIGEST_LENGTH + 1] = {0};
    char text_exp[SHA_DIGEST_LENGTH * 2 + 1] = {0};
    if (!pnode)
    {
        return EUE_TAB_NULL;
    }
    if (pnode->hex_mode)
    {
        return EUE_UNKOWN_ERR;
    }
    if ((sel_text = util_strdup_select(pnode, &sel_len, 0)) == NULL)
    {
        printf("memory allocation failed\n");
        return EUE_OUT_OF_MEMORY;
    }
    char *fn_name[1] = {"SHA1"};
    uintptr_t pfunc[1] = {0};
    HMODULE hssl = eu_ssl_open_symbol(fn_name, 1, pfunc);
    if (hssl)
    {
        ((eu_sha1)pfunc[0])((unsigned char *) sel_text, (int) sel_len, (unsigned char *) out_text);
        util_hex_expand(out_text, SHA_DIGEST_LENGTH, text_exp);
        eu_sci_call(pnode, SCI_REPLACESEL, 0, (sptr_t) text_exp);
        eu_ssl_close_symbol(&hssl);
    }
    free(sel_text);
    return SKYLARK_OK;
}

int
on_edit_sha256(eu_tabpage *pnode)
{
    size_t sel_len;
    char *sel_text = NULL;
    char out_text[SHA256_DIGEST_LENGTH + 1] = {0};
    char text_exp[SHA256_DIGEST_LENGTH * 2 + 1] = {0};
    if (!pnode)
    {
        return EUE_TAB_NULL;
    }
    if (pnode->hex_mode)
    {
        return EUE_UNKOWN_ERR;
    }
    if ((sel_text = util_strdup_select(pnode, &sel_len, 0)) == NULL)
    {
        printf("memory allocation failed\n");
        return EUE_OUT_OF_MEMORY;
    }
    char *fn_name[1] = {"SHA256"};
    uintptr_t pfunc[1] = {0};
    HMODULE hssl = eu_ssl_open_symbol(fn_name, 1, pfunc);
    if (hssl)
    {
        ((eu_sha256)pfunc[0])((unsigned char *) sel_text, (int) sel_len, (unsigned char *) out_text);
        util_hex_expand(out_text, SHA256_DIGEST_LENGTH, text_exp);
        eu_sci_call(pnode, SCI_REPLACESEL, 0, (sptr_t) text_exp);
        eu_ssl_close_symbol(&hssl);
    }
    free(sel_text);
    return SKYLARK_OK;
}

int
on_edit_descbc_enc(eu_tabpage *pnode)
{
    int err = 0;
    long out_len;
    size_t sel_len;
    char *sel_text = NULL;
    char *out_text = NULL;
    char *text_exp = NULL;
    char key[24 + 1] = { 0 };
    if (!pnode)
    {
        return 0;
    }
    if (pnode->hex_mode)
    {
        return 0;
    }
    do
    {
        TCHAR key_str[24 + 1] = {0};
        sel_text = util_strdup_select(pnode, &sel_len, 8);
        if (sel_text == NULL)
        {
            err = EUE_OUT_OF_MEMORY;
            break;
        }
        out_len = (long) sel_len;
        out_text = (char *) calloc(1, out_len + 1);
        if (out_text == NULL)
        {
            err = EUE_OUT_OF_MEMORY;
            break;
        }
        text_exp = (char *) calloc(1, out_len * 2 + 1);
        if (text_exp == NULL)
        {
            err = EUE_OUT_OF_MEMORY;
            break;
        }
        LOAD_I18N_RESSTR(IDC_MSG_ENC_STR, enc_str);
        eu_input(enc_str, key_str, _countof(key_str));
        if (*key_str == 0)
        {
            err = EUE_POINT_NULL;
            break;
        }
        else
        {
            WideCharToMultiByte(CP_UTF8, 0, key_str, -1, key, 24, NULL, NULL);
        }
        util_enc_des_cbc_192((unsigned char *) key, (unsigned char *) sel_text, (int) sel_len, (unsigned char *) out_text, &out_len, NULL);
        memset(text_exp, 0, out_len * 2 + 1);
        util_hex_expand(out_text, (int) out_len, text_exp);
        eu_sci_call(pnode, SCI_REPLACESEL, 0, (sptr_t) text_exp);
    }while(0);
    if (sel_text)
    {
        free(sel_text);
    }
    if (out_text)
    {
        free(out_text);
    }
    if (text_exp)
    {
        free(text_exp);
    }
    return err;
}

int
on_edit_descbc_dec(eu_tabpage *pnode)
{
    int err = 0;
    long out_len;
    size_t sel_len;
    size_t input_len;
    char *sel_text = NULL;
    char *input_text = NULL;
    char *out_text = NULL;
    char key[24 + 1] = { 0 };
    if (!pnode)
    {
        return 0;
    }
    if (pnode->hex_mode)
    {
        return 0;
    }
    do
    {
        TCHAR key_str[24 + 1] = {0};
        sel_text = util_strdup_select(pnode, &sel_len, 8);
        if (sel_text == NULL)
        {
            err = EUE_OUT_OF_MEMORY;
            break;
        }
        input_len = sel_len / 2;
        input_text = (char *) calloc(1, input_len + 1);
        if (input_text == NULL)
        {
            err = EUE_OUT_OF_MEMORY;
            break;
        }
        util_hex_fold(sel_text, (int) sel_len, input_text);
        out_len = (long) sel_len;
        out_text = (char *) calloc(1, input_len + 1);
        if (out_text == NULL)
        {
            err = EUE_OUT_OF_MEMORY;
            break;
        }
        LOAD_I18N_RESSTR(IDC_MSG_DEC_STR, dec_str);
        eu_input(dec_str, key_str, _countof(key_str));
        if (*key_str == 0)
        {
            err = EUE_POINT_NULL;
            break;
        }
        else
        {
            WideCharToMultiByte(CP_UTF8, 0, key_str, -1, key, 24, NULL, NULL);
        }
        util_dec_des_cbc_192((unsigned char *) key, (unsigned char *) input_text, (int) input_len, (unsigned char *) out_text, &out_len, NULL);
        eu_sci_call(pnode, SCI_REPLACESEL, 0, (sptr_t) out_text);
    } while(0);
    if (sel_text)
    {
        free(sel_text);
    }
    if (input_text)
    {
        free(input_text);
    }
    if (out_text)
    {
        free(out_text);
    }
    return err;
}

static void
on_close_selection(eu_tabpage *pnode, const char *open_str, const char *close_str)
{
    int len = 0;
    if (eu_sci_call(pnode, SCI_SELECTIONISRECTANGLE, 0, 0))
    {
        MSG_BOX(IDS_SELRECT, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
        return;
    }
    if (STR_IS_NUL(open_str) && STR_IS_NUL(close_str))
    {
        return;
    }
    util_wait_cursor(pnode);
    sptr_t sel_start = eu_sci_call(pnode, SCI_GETSELECTIONSTART, 0, 0);
    sptr_t sel_end = eu_sci_call(pnode, SCI_GETSELECTIONEND, 0, 0);
    eu_sci_call(pnode, SCI_BEGINUNDOACTION, 0, 0);
    if (STR_NOT_NUL(open_str))
    {
        len = (int) strlen(open_str);
        eu_sci_call(pnode, SCI_SETTARGETRANGE, sel_start, sel_start);
        eu_sci_call(pnode, SCI_REPLACETARGET, len, (LPARAM) open_str);
    }

    if (STR_NOT_NUL(close_str))
    {
        eu_sci_call(pnode, SCI_SETTARGETRANGE, sel_end + len, sel_end + len);
        eu_sci_call(pnode, SCI_REPLACETARGET, strlen(close_str), (LPARAM) close_str);
    }
    eu_sci_call(pnode, SCI_ENDUNDOACTION, 0, 0);
    // fix selection
    if (sel_start == sel_end)
    {
        eu_sci_call(pnode, SCI_SETSEL, sel_start + len, sel_start + len);
    }
    else
    {
        sptr_t cur_pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
        sptr_t anchor_pos = eu_sci_call(pnode, SCI_GETANCHOR, 0, 0);

        if (cur_pos < anchor_pos)
        {
            cur_pos = sel_start + len;
            anchor_pos = sel_end + len;
        }
        else
        {
            anchor_pos = sel_start + len;
            cur_pos = sel_end + len;
        }
        eu_sci_call(pnode, SCI_SETSEL, anchor_pos, cur_pos);
    }
    util_restore_cursor(pnode);
}

static void
on_comment_newline(eu_tabpage *pnode, const char *open_str, const char *close_str)
{
    int offset = 0;
    char start[ACNAME_LEN + 1] = { 0 };
    char end[ACNAME_LEN + 1] = { 0 };
    const char *line_end = on_encoding_get_eol(pnode);
    sptr_t pos = eu_sci_call(pnode, SCI_GETSELECTIONSTART, 0, 0);
    sptr_t line = eu_sci_call(pnode, SCI_LINEFROMPOSITION, pos, 0);
    sptr_t line_start = eu_sci_call(pnode, SCI_POSITIONFROMLINE, line, 0);
    if (pos != line_start)
    {
        strncat(start, line_end, ACNAME_LEN);
    }
    strncat(start, open_str, ACNAME_LEN);
    strncat(start, line_end, ACNAME_LEN);
    pos = eu_sci_call(pnode, SCI_GETSELECTIONEND, 0, 0);
    line = eu_sci_call(pnode, SCI_LINEFROMPOSITION, pos, 0);
    if (pos != line_start)
    {
        strncat(end, line_end, ACNAME_LEN);
    }
    strncat(end, close_str, ACNAME_LEN);
    strncat(end, line_end, ACNAME_LEN);
    on_close_selection(pnode, start, end);
}

static void
eu_toggle_comment(eu_tabpage *pnode, const char *pcomment, bool at_start)
{
    if (eu_sci_call(pnode, SCI_SELECTIONISRECTANGLE, 0, 0))
    {
        MSG_BOX(IDS_SELRECT, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
        return;
    }
    util_wait_cursor(pnode);
    sptr_t line_start;
    sptr_t line_end;
    sptr_t sel_start = eu_sci_call(pnode, SCI_GETSELECTIONSTART, 0, 0);
    sptr_t sel_end = eu_sci_call(pnode, SCI_GETSELECTIONEND, 0, 0);
    sptr_t cur_pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
    const int cch_comment = (int) strlen(pcomment);
    util_effect_line(pnode, &line_start, &line_end);    
    sptr_t comment_col = 0;
    if (!at_start)
    {
        comment_col = MAX_BUFFER;
        for (sptr_t lc = line_start; lc <= line_end; lc++)
        {
            const sptr_t line_end_pos = eu_sci_call(pnode, SCI_GETLINEENDPOSITION, lc, 0);
            const sptr_t line_indent_pos = eu_sci_call(pnode, SCI_GETLINEINDENTPOSITION, lc, 0);

            if (line_indent_pos != line_end_pos)
            {
                const sptr_t indent_col = eu_sci_call(pnode, SCI_GETCOLUMN, line_indent_pos, 0);
                comment_col = MIN_POS(comment_col, indent_col);
            }
        }
    }
    eu_sci_call(pnode, SCI_BEGINUNDOACTION, 0, 0);
    int m_action = 0;

    for (sptr_t lc = line_start; lc <= line_end; lc++)
    {
        const sptr_t indent_pos = eu_sci_call(pnode, SCI_GETLINEINDENTPOSITION, lc, 0);
        bool whilte_line = false;
        // 空白行 [space/tab]
        if (comment_col && indent_pos == eu_sci_call(pnode, SCI_GETLINEENDPOSITION, lc, 0))
        {
            whilte_line = true;
        }
        char ch_buf[FT_LEN + 1] = { 0 };
        struct Sci_TextRange tr = { {indent_pos, indent_pos + MIN_POS(FT_LEN, cch_comment) }, ch_buf };
        eu_sci_call(pnode, SCI_GETTEXTRANGE, 0, (sptr_t) &tr);

        sptr_t comment_pos;
        if (_strnicmp(ch_buf, pcomment, cch_comment) == 0)
        {
            int ch;
            switch (m_action)
            {
                case 0:
                    m_action = 2;
                    FALLTHROUGH_ATTR;
                    // fall through
                case 2:
                    comment_pos = indent_pos;
                    // 包括注释的空白行 [space/tab/comment]
                    ch = (int) eu_sci_call(pnode, SCI_GETCHARAT, indent_pos + cch_comment, 0);
                    if (ch == '\n' || ch == '\r')
                    {
                        comment_pos = eu_sci_call(pnode, SCI_POSITIONFROMLINE, lc, 0);
                    }
                    eu_sci_call(pnode, SCI_SETTARGETRANGE, comment_pos, indent_pos + cch_comment);
                    eu_sci_call(pnode, SCI_REPLACETARGET, 0, (LPARAM) "");
                    break;
                case 1:
                    comment_pos = eu_sci_call(pnode, SCI_FINDCOLUMN, lc, comment_col);
                    ch = (int) eu_sci_call(pnode, SCI_GETCHARAT, comment_pos, 0);
                    if (ch == '\t' || ch == ' ')
                    {
                        eu_sci_call(pnode, SCI_INSERTTEXT, comment_pos, (LPARAM) pcomment);
                    }
                    break;
            }
        }
        else
        {
            switch (m_action)
            {
                case 0:
                    m_action = 1;
                    FALLTHROUGH_ATTR;
                case 1:
                    comment_pos = eu_sci_call(pnode, SCI_FINDCOLUMN, lc, comment_col);
                    if (!whilte_line || (line_start == line_end))
                    {
                        eu_sci_call(pnode, SCI_INSERTTEXT, comment_pos, (LPARAM) pcomment);
                    }
                    else
                    {
                        char psz_comment[MAX_BUFFER] = { 0 };
                        sptr_t tab = 0;
                        bool tab_as_spaces = eu_get_config()->tab2spaces;
                        int tab_width = (int)eu_sci_call(pnode, SCI_GETTABWIDTH, 0, 0);
                        sptr_t count = comment_col;
                        if (!tab_as_spaces && tab_width > 0)
                        {
                            tab = comment_col / tab_width;
                            FillMemory(psz_comment, tab, '\t');
                            count -= tab * tab_width;
                        }
                        FillMemory(psz_comment + tab, count, ' ');
                        strcat(psz_comment, pcomment);
                        eu_sci_call(pnode, SCI_INSERTTEXT, comment_pos, (LPARAM) psz_comment);
                    }
                    break;
                case 2:
                    break;
            }
        }
    }
    eu_sci_call(pnode, SCI_ENDUNDOACTION, 0, 0);
    if (sel_start != sel_end)
    {
        sptr_t anchor_pos;
        if (cur_pos == sel_start)
        {
            cur_pos = eu_sci_call(pnode, SCI_POSITIONFROMLINE, line_start, 0);
            anchor_pos = eu_sci_call(pnode, SCI_POSITIONFROMLINE, line_end + 1, 0);
        }
        else
        {
            anchor_pos = eu_sci_call(pnode, SCI_POSITIONFROMLINE, line_start, 0);
            cur_pos = eu_sci_call(pnode, SCI_POSITIONFROMLINE, line_end + 1, 0);
        }
        eu_sci_call(pnode, SCI_SETSEL, anchor_pos, cur_pos);
    }
    util_restore_cursor(pnode);
}

static int
get_html_block(int style)
{
    if (style == SCE_H_CDATA)
    {
        return HTML_TEXT_BLOCK_CDATA;
    }
    if ((style >= SCE_HJ_START && style <= SCE_HJ_REGEX) || (style >= SCE_HJA_START && style <= SCE_HJA_REGEX))
    {
        return HTML_TEXT_BLOCK_JS;
    }
    if ((style >= SCE_HB_START && style <= SCE_HB_STRINGEOL) || (style >= SCE_HBA_START && style <= SCE_HBA_STRINGEOL))
    {
        return HTML_TEXT_BLOCK_VBS;
    }
    if ((style >= SCE_HP_START && style <= SCE_HP_IDENTIFIER) || (style >= SCE_HPA_START && style <= SCE_HPA_IDENTIFIER))
    {
        return HTML_TEXT_BLOCK_PYTHON;
    }
    if ((style >= SCE_HPHP_DEFAULT && style <= SCE_HPHP_COMPLEX_VARIABLE))
    {
        return HTML_TEXT_BLOCK_PHP;
    }
    if ((style >= SCE_H_SGML_DEFAULT && style <= SCE_H_SGML_BLOCK_DEFAULT))
    {
        return HTML_TEXT_BLOCK_SGML;
    }
    return HTML_TEXT_BLOCK_TAG;
}

static int
on_html_block(eu_tabpage *pnode)
{
    const sptr_t pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
    const int style = (int) eu_sci_call(pnode, SCI_GETSTYLEAT, pos, 0);
    return get_html_block(style);
}

int
on_edit_comment_line(eu_tabpage *pnode)
{
    if (!(pnode && pnode->doc_ptr))
    {
        return 1;
    }
    if (pnode->hex_mode)
    {
        return 0;
    }
    switch (pnode->doc_ptr->doc_type)
    {
        case DOCTYPE_CPP:
        case DOCTYPE_CS:
        case DOCTYPE_GO:
        case DOCTYPE_SWIFT:
        case DOCTYPE_RUST:
        case DOCTYPE_CSS:
        case DOCTYPE_LOG:
        case DOCTYPE_JSON:
        case DOCTYPE_JAVA:
        case DOCTYPE_JAVASCRIPT:
            eu_toggle_comment(pnode, "// ", false);
            break;
        case DOCTYPE_CMAKE:
        case DOCTYPE_PYTHON:
        case DOCTYPE_PERL:
        case DOCTYPE_RUBY:
        case DOCTYPE_YAML:
        case DOCTYPE_MAKEFILE:
        case DOCTYPE_NIM:
            eu_toggle_comment(pnode, "# ", false);
            break;
        case DOCTYPE_HTML:
        case DOCTYPE_XML:
        {
            const int block = on_html_block(pnode);
            switch (block)
            {
                case HTML_TEXT_BLOCK_VBS:
                    eu_toggle_comment(pnode, "' ", false);
                    break;
                case HTML_TEXT_BLOCK_PYTHON:
                    eu_toggle_comment(pnode, "# ", false);
                    break;
                case HTML_TEXT_BLOCK_CDATA:
                case HTML_TEXT_BLOCK_JS:
                case HTML_TEXT_BLOCK_PHP:
                    eu_toggle_comment(pnode, "// ", false);
                    break;
                default:
                    break;
            }
            break;
        }
        case DOCTYPE_LUA:
        case DOCTYPE_SQL:
            eu_toggle_comment(pnode, "-- ", false);
            break;
        case DOCTYPE_LISP:
        case DOCTYPE_AU3:
            eu_toggle_comment(pnode, "; ", false);
            break;
        case DOCTYPE_FORTRAN:
            eu_toggle_comment(pnode, "! ", false);
            break;            
        case DOCTYPE_COBOL:
            eu_toggle_comment(pnode, "* ", false);
            break;
        case DOCTYPE_JULIA:
            eu_toggle_comment(pnode, "# ", false);
            break;            
        case DOCTYPE_SH:
        {
            TCHAR *sp = on_doc_get_ext(pnode);
            if (sp && _tcsstr(_T(";*.bat;*.cmd;*.nt;"), sp))
            {
                eu_toggle_comment(pnode, "@rem ", false);
            }         
            else
            {
                eu_toggle_comment(pnode, "# ", false);
            }
            break;
        }
        default:
            break;
    }
    return 0;
}

int
on_edit_comment_stream(eu_tabpage *pnode)
{
    if (!(pnode && pnode->doc_ptr))
    {
        return 1;
    }
    if (pnode->hex_mode)
    {
        return 0;
    }
    switch (pnode->doc_ptr->doc_type)
    {
        case DOCTYPE_CPP:
        case DOCTYPE_CS:
        case DOCTYPE_GO:
        case DOCTYPE_SWIFT:
        case DOCTYPE_RUST:
        case DOCTYPE_CSS:
        case DOCTYPE_JSON:
        case DOCTYPE_JAVA:
        case DOCTYPE_JAVASCRIPT:
        case DOCTYPE_SQL:
            on_close_selection(pnode, "/*", "*/");
            break;
        case DOCTYPE_CMAKE:
            on_close_selection(pnode, "#[[", "]]");
            break;
        case DOCTYPE_HTML:
        case DOCTYPE_XML:
        {
            const int block = on_html_block(pnode);
            switch (block)
            {
                case HTML_TEXT_BLOCK_TAG:
                    on_close_selection(pnode, "<!--", "-->");
                    break;
                case HTML_TEXT_BLOCK_CSS:
                case HTML_TEXT_BLOCK_CDATA:
                case HTML_TEXT_BLOCK_JS:
                case HTML_TEXT_BLOCK_PHP:
                    on_close_selection(pnode, "/*", "*/");
                    break;
                case HTML_TEXT_BLOCK_SGML:
                    on_close_selection(pnode, "--", "--");
                    break;
                default:
                    break;
            }
            break;
        }
        case DOCTYPE_LUA:
            on_comment_newline(pnode, "--[=[", "]=]");
            break;
        case DOCTYPE_LISP:
            on_close_selection(pnode, "#|", "|#");
            break;
        case DOCTYPE_NIM:
            on_close_selection(pnode, "#[ ", "]#");
            break;
        case DOCTYPE_JULIA:
            on_comment_newline(pnode, "#=", "=#");
            break;            
        case DOCTYPE_SH:
        {
            TCHAR *sp = on_doc_get_ext(pnode);
            if ((sp && _tcsstr(_T(";*.ps1;*.psc1;*.psd1;*.psm1;"), sp)))
            {
                on_comment_newline(pnode, "<#", "#>");
            }
            else
            {
                on_edit_comment_line(pnode);
            }
            break;
        }
        case DOCTYPE_AU3:
            on_comment_newline(pnode, "#cs", "#ce");
            break;
        case DOCTYPE_FORTRAN:
            on_comment_newline(pnode, "#if 0", "#endif");
            break;
        default:
            break;
    }
    return 0;
}

int
on_edit_convert_eols(eu_tabpage *pnode, int eol_mode)
{
    int ret = 1;
    if (pnode && (eu_sci_call(pnode, SCI_GETEOLMODE, 0, 0) != eol_mode))
    {
        eu_sci_call(pnode, SCI_BEGINUNDOACTION, 0, 0);
        eu_sci_call(pnode, SCI_CONVERTEOLS, eol_mode, 0);
        eu_sci_call(pnode, SCI_SETEOLMODE, eol_mode, 0);
        _snprintf(eols_undo_str, ACNAME_LEN-1, "%s=%d=%d", "_eol/?@#$%^&*()`/~", pnode->eol, eol_mode);
        pnode->eol = eol_mode;
        menu_update_all(eu_module_hwnd(), pnode);
        eu_sci_call(pnode, SCI_INSERTTEXT, 0, (sptr_t) eols_undo_str);
        eu_sci_call(pnode, SCI_DELETERANGE, 0, strlen(eols_undo_str));
        eu_sci_call(pnode, SCI_ENDUNDOACTION, 0, 0);
        ret = 0;
    }
    return ret;
}

void
on_edit_undo_iconv(eu_tabpage *pnode)
{
    int old_codepage = IDM_UNKNOWN;
    int new_codepage = IDM_UNKNOWN;
    if (sscanf(iconv_undo_str, "%*[^0-9]%i=%i", &old_codepage, &new_codepage) == 2)
    {
        pnode->codepage = old_codepage;
        memset(pnode->pre_context, 0, sizeof(pnode->pre_context));
        on_encoding_set_bom_from_cp(pnode);
        on_statusbar_update_coding(pnode, 0);
    }
}

void
on_edit_undo_eol(eu_tabpage *pnode)
{
    int old_eol = -1;
    int new_eol = -1;
    if (sscanf(eols_undo_str, "%*[^0-9]%i=%i", &old_eol, &new_eol) == 2)
    {
        pnode->eol = old_eol;
        eu_sci_call(pnode, SCI_SETEOLMODE, pnode->eol, 0);
        on_statusbar_update_eol(pnode);
    }
}
