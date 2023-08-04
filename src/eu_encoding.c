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

#include "framework.h"

#define LANG_CHT MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)
#define LANG_CHS MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)

static bool
is_cjk_converter(const char *encoding)
{
    return (0
            /* Legacy Japanese encodings */
            || STRICMP(encoding, ==, "euc-jp")
            || STRICMP(encoding, ==, "cp932")
            /* Legacy Chinese encodings */
            || STRICMP(encoding, ==, "gb18030")
            || STRICMP(encoding, ==, "euc-tw")
            || STRICMP(encoding, ==, "big5")
            /* Legacy Korean encodings */
            || STRICMP(encoding, ==, "euc-kr")
            /* Other local encodings */
            || STRICMP(encoding, ==, "ansi"));
}

static bool
init_conv_handle(euconv_t *icv)
{
    if (!(icv->src_from && icv->dst_to))
    {
        return false;
    }
    icv->cd = NULL;
    if (icv->warning < 0 && is_cjk_converter(icv->dst_to))
    {
        icv->warning = WANRING_ENABLE;
    }
    if (stricmp(icv->src_from, "ansi") == 0)
    {
        icv->fn_convert = (api_convert) eu_mbcs_utf8;
    }
    else if (stricmp(icv->dst_to, "ansi") == 0)
    {
        icv->fn_convert = (api_convert) eu_utf8_mbcs;
    }
    else
    {
        icv->fn_convert = NULL;
        icv->cd = eu_iconv_open(icv->dst_to, icv->src_from);
        if (icv->cd == (iconv_t) -1)
        {
            return false;
        }
    }
    return true;
}

static void
close_conv_handle(euconv_t *icv)
{
    if (icv)
    {
        if (icv->cd)
        {
            eu_iconv_close(icv->cd);
        }
        memset(icv, 0, sizeof(euconv_t));
    }
}

int
on_encoding_line_eol(const char *str, size_t len)
{
    if (!str || len <= 0)
    {
        return -1;
    }
    char *p = (char *) memchr((void *) str, '\n', len);
    if (p)
    {
        if (p > str && *(p - 1) == '\r')
        {
            return 0;
        }
        else
        {
            return 2;
        }
    }
    else if ((p = (char *) memchr((void *) str, '\r', len)))
    {
        return 1;
    }
    return -1;
}

int
on_encoding_line_mode(const char *str, size_t len)
{
    int eol = on_encoding_line_eol(str, len);
    if (eol == -1)
    {
        eol = eu_get_config()->new_file_eol;
    }
    return eol;
}

const char *
on_encoding_get_eol(eu_tabpage *pnode)
{
    const int eol_mode = (int) eu_sci_call(pnode, SCI_GETEOLMODE, 0, 0);
    return (eol_mode == SC_EOL_LF) ? "\n" : ((eol_mode == SC_EOL_CR) ? "\r" : "\r\n");
}

const int
on_encoding_eol_char(eu_tabpage *pnode)
{
    const int eol_mode = (int) eu_sci_call(pnode, SCI_GETEOLMODE, 0, 0);
    return (eol_mode == SC_EOL_CR ? 0x0D : 0x0A);
}

/****************************************************************************
 * 根据缓存区前几个字节, 记录bom到节点
 ****************************************************************************/
int
on_encoding_set_bom(const uint8_t *buf, eu_tabpage *pnode)
{
    const bom_type file_bom[] = { { 3, "\xEF\xBB\xBF" },     // BOM_UTF8
                                  { 4, "\xFF\xFE\x00\x00" }, // BOM_UTF32_LE
                                  { 4, "\x00\x00\xFE\xFF" }, // BOM_UTF32_BE
                                  { 2, "\xFF\xFE" },         // BOM_UTF16_LE
                                  { 2, "\xFE\xFF" },         // BOM_UTF16_BE
                                  { 0, { 0 } } };
    if (!pnode)
    {
        return 0;
    }
    pnode->pre_len = 0;
    memset(pnode->pre_context, 0, sizeof(pnode->pre_context));
    for (int i = 0; file_bom[i].len; ++i)
    {
        if (memcmp(buf, file_bom[i].bom, file_bom[i].len) == 0)
        {
            switch (i)
            {
                case 0:
                    pnode->pre_len = 3;
                    memcpy(pnode->pre_context, buf, 3);
                    return (int)pnode->pre_len;
                case 1:
                case 2:
                    pnode->pre_len = 4;
                    memcpy(pnode->pre_context, buf, 4);
                    return (int)pnode->pre_len;
                case 3:
                case 4:
                    pnode->pre_len = 2;
                    memcpy(pnode->pre_context, buf, 2);
                    return (int)pnode->pre_len;
                default:
                    break;
            }
        }
    }
    return (int)pnode->pre_len;
}

/****************************************************************************
 * 根据节点的页面编码, 记录bom到节点
 ****************************************************************************/
void
on_encoding_set_bom_from_cp(eu_tabpage *pnode)
{
    const bom_type file_bom[] = { { IDM_UNI_UTF8B, "\xEF\xBB\xBF" },
                                  { IDM_UNI_UTF32LE, "\xFF\xFE\x00\x00" },
                                  { IDM_UNI_UTF32BE, "\x00\x00\xFE\xFF" },
                                  { IDM_UNI_UTF16LEB, "\xFF\xFE" },
                                  { IDM_UNI_UTF16BEB, "\xFE\xFF" },
                                  { 0, { 0 } } };
    if (!(pnode && pnode->codepage))
    {
        return ;
    }
    pnode->pre_len = 0;
    memset(pnode->pre_context, 0, sizeof(pnode->pre_context));
    for (int i = 0; file_bom[i].len; ++i)
    {
        if (pnode->codepage == file_bom[i].len)
        {
            switch (i)
            {
                case 0:
                    pnode->pre_len = 3;
                    memcpy(pnode->pre_context, file_bom[i].bom, 3);
                    return;
                case 1:
                case 2:
                    pnode->pre_len = 4;
                    memcpy(pnode->pre_context, file_bom[i].bom, 4);
                    return;
                case 3:
                case 4:
                    pnode->pre_len = 2;
                    memcpy(pnode->pre_context, file_bom[i].bom, 2);
                    return;
                default:
                    break;
            }
        }
    }
}

size_t
on_encoding_do_iconv(euconv_t *icv, char *src, size_t *src_len, char **dst, size_t *plen)
{
    size_t ret = 1;
    size_t lsrc = *src_len;
    char *psrc = src;
    char *pdst = NULL;
    size_t ldst = lsrc * 4;
    int msg = IDOK;
    int argument = 0;
    if (!init_conv_handle(icv))
    {
        return (size_t) -1;
    }
    if (icv->warning > 0)
    {
        MSG_BOX_SEL(IDC_MSG_CONV_TIPS, IDC_MSG_TIPS, MB_ICONSTOP | MB_OKCANCEL, msg);
    }
    if (msg == IDCANCEL || msg == IDCLOSE)
    {
        eu_logmsg("we cancel the converter\n");
        close_conv_handle(icv);
        return 255;
    }
    if (icv->fn_convert)
    {   // 根据代码页转换编码, 如果系统代码页与编辑器字符集不一致, 将导致问题
        if ((*dst = (char *) icv->fn_convert(-1, src, plen)) == NULL)
        {
            return (size_t) -1;
        }
        else
        {
            (*plen)--;
            return 0;
        }
    }
    *plen = ldst;
    if (eu_iconvctl(icv->cd, ICONV_SET_DISCARD_ILSEQ, &argument) != 0)
    {
        eu_logmsg("can't enable illegal feature!\n");
        close_conv_handle(icv);
        return (size_t) -1;
    }
    *dst = (char *) malloc(ldst + 1);
    if (*dst == NULL)
    {
        close_conv_handle(icv);
        return (size_t) -1;
    }
    else
    {
        pdst = *dst;
    }
    ret = eu_iconv(icv->cd, &psrc, &lsrc, &pdst, &ldst);
    if (ret != (size_t) -1)
    {
        // 成功, 写入转换后的长度
        *plen -= ldst;
    }
    else
    {
        eu_safe_free(*dst);
        eu_logmsg("%s: [%s->%s] failed! lsrc = %zu, ldst = %zu, ret = %d\n", __FUNCTION__, icv->src_from, icv->dst_to, lsrc, ldst, (int)ret);
    }
    close_conv_handle(icv);
    return ret;
}

static bool
tw2gb(const TCHAR *str_in, TCHAR **str_out, size_t *plen)
{
    WORD wgid = LANG_CHS;
    LCID wlocal = MAKELCID(wgid, SORT_CHINESE_PRCP);
    int nreturn = LCMapString(wlocal, LCMAP_SIMPLIFIED_CHINESE, str_in, -1, NULL, 0);
    if (!nreturn)
    {
        return false;
    }
    *str_out = (TCHAR *) calloc(sizeof(TCHAR), nreturn + 1);
    if (*str_out == NULL)
    {
        return false;
    }
    size_t len = LCMapString(wlocal, LCMAP_SIMPLIFIED_CHINESE, str_in, -1, *str_out, nreturn);
    if (plen)
    {
        *plen = len;
    }
    return (len > 0);
}

static bool
gb2tw(const TCHAR *str_in, TCHAR **str_out, size_t *plen)
{
    WORD wgid = LANG_CHS;
    LCID wlocal = MAKELCID(wgid, SORT_CHINESE_PRCP);
    int nreturn = LCMapString(wlocal, LCMAP_TRADITIONAL_CHINESE, str_in, -1, NULL, 0);
    if (!nreturn)
    {
        return false;
    }
    *str_out = (TCHAR *) calloc(sizeof(TCHAR), nreturn + 1);
    if (*str_out == NULL)
    {
        return false;
    }
    size_t len = LCMapString(wlocal, LCMAP_TRADITIONAL_CHINESE, str_in, -1, *str_out, nreturn);
    if (plen)
    {
        *plen = len;
    }
    return (len > 0);
}

char*
on_encoding_gb_big5(const char *gb2, size_t *out_len)
{
    char *bg5_str = NULL;
    WCHAR *gb_str = NULL;
    TCHAR *szbuf = NULL;
    int code_to = 950;
    if ((gb_str = eu_utf8_utf16(gb2, NULL)) == NULL)
    {
        return NULL;
    }
    if (!gb2tw(gb_str, &szbuf, out_len))
    {
        eu_logmsg("failed to get internal code table\n");
        free(gb_str);
        return NULL;
    }
    if (szbuf)
    {
        bg5_str = eu_utf16_utf8(szbuf, NULL);
        free(szbuf);
    }
    free(gb_str);
    return bg5_str;
}

char*
on_encoding_big5_gb(const char *bg5, size_t *out_len)
{
    char *gb_str = NULL;
    WCHAR *bg5_str = NULL;
    TCHAR *szbuf = NULL;
    if ((bg5_str = eu_utf8_utf16(bg5, NULL)) == NULL)
    {
        return NULL;
    }
    if (!tw2gb(bg5_str, &szbuf, out_len))
    {
        eu_logmsg("failed to get internal code table\n");
        free(bg5_str);
        szbuf = NULL;
    }
    if (szbuf)
    {
        gb_str = eu_utf16_utf8(szbuf, NULL);
        free(szbuf);
    }
    free(bg5_str);
    return gb_str;
}

int
on_encoding_convert_internal_code(eu_tabpage *pnode, enc_back fn)
{
    int ret = 1;
    size_t len = 0;
    char *dst = NULL;
    if (pnode && !pnode->hex_mode && !pnode->pmod && fn)
    {
        bool whole = false;
        char *ptxt = util_strdup_select(pnode, &len, 0);
        eu_sci_call(pnode, SCI_BEGINUNDOACTION, 0, 0);
        do
        {
            if (len < 2)
            {
                eu_safe_free(ptxt);
                if ((ptxt = util_strdup_content(pnode, &len)) == NULL)
                {
                    break;
                }
                whole = true;
            }
            if (ptxt)
            {
                if (!(dst = fn(ptxt, NULL)))
                {
                    break;
                }
                if (!whole)
                {
                    // 替换所选文本内容
                    eu_sci_call(pnode, SCI_REPLACESEL, 0, (sptr_t) dst);
                }
                else
                {
                    // 替换整个文本内容
                    eu_sci_call(pnode, SCI_CLEARALL, 0, 0);
                    eu_sci_call(pnode, SCI_ADDTEXT, strlen(dst), (LPARAM) dst);
                }
                ret = 0;
            }
        } while(0);
        eu_sci_call(pnode, SCI_ENDUNDOACTION, 0, 0);
        if (ptxt)
        {
            free(ptxt);
        }
        if (dst)
        {
            free(dst);
        }
    }
    return ret;
}
