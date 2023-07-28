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

#ifndef ANY_WORD
#define ANY_WORD "///*///"
#endif

#define COMPLETE_LINE_HEADER   0x00000010
#define COMPLETE_WITH_REGXP    0x00000020
#define COMPLETE_AUTO_EXPAND   0x00000040

#define DEFAULT_CHARS "_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
#define CUSTOM_CHARS  "_!#$&/abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
#define CUSTOM_CHARS_CSS  ":_!#$&/abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
#define CUSTOM_CHARS_AU3  "_!#$&@abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"

#define str_prev(p) ((p) - (psrc) > 0 ? (p[-1]) : (0))

typedef void (*ptr_replace)(void *p1, void **p2, const char *by, int start, int offset);
typedef int (*ptr_re_callback)(eu_tabpage *pnode, const char *pstr, int start, int end);

static int on_complete_pcre_match(eu_tabpage *pnode, const char *pstr, const char *pattern, ptr_re_callback fn);
static volatile long last_snippet_focus = 0;
static inline void
on_complete_vec_init(complete_t *vec)
{
    if (vec)
    {
        memset(vec, 0, sizeof(complete_t));
        vec->index = -1;
        memset(vec->pos, -1, OVEC_LEN * sizeof(auto_postion));
    }
}

static inline void
on_complete_inc_focus(void)
{
    _InterlockedIncrement(&last_snippet_focus);
}

static inline void
on_complete_dec_focus(void)
{
    _InterlockedDecrement(&last_snippet_focus);
}

static inline void
on_complete_zero_focus(void)
{
    _InterlockedExchange(&last_snippet_focus, 0);
}

static inline bool
on_complete_equal_value(eu_tabpage *pnode, complete_t *it, int j)
{
    char value[MAX_BUFFER] = {0};
    Sci_TextRange tr = {{it->pos[j].min, it->pos[j].max}, value};
    eu_sci_call(pnode, SCI_GETTEXTRANGE, 0, (sptr_t) &tr);
    return (strcmp(value, it->value) == 0);
}

static inline void
on_complete_swap(auto_postion *p1, auto_postion *p2)
{
    auto_postion tmp = *p1;
    *p1 = *p2;
    *p2 = tmp;
}

static void
on_complete_exchang_end(complete_t *it, int j)
{
    for (; j < OVEC_LEN - 1 && it->pos[j + 1].min >= 0; ++j)
    {
        on_complete_swap(&it->pos[j], &it->pos[j + 1]);
    }
}

static int
on_complete_str_cmp(snippet_t *pvec, const char *value)
{
    if (pvec && pvec->name[0])
    {
        return strcmp(pvec->name, value);
    }
    return 1;
}

static int
on_complete_str_match(snippet_t *pvec, const char *value)
{
    if (pvec && pvec->name[0])
    {
        int ret = strcmp(pvec->name, value);
        if (ret && strchr(pvec->parameter, 'r'))
        {
            ret = on_complete_pcre_match(NULL, value, pvec->name, NULL);
        }
        return ret;
    }
    return 1;
}

static int
on_complete_char_cmp(complete_t *pvec, int value)
{
    if (pvec)
    {
        return (UTIL_NUMBER(value) != pvec->index);
    }
    return 1;
}

static int
on_complete_group_cmp(capture_set *pvec, int value)
{
    if (pvec)
    {
        return (UTIL_NUMBER(value) != UTIL_NUMBER(pvec->cap[1]));
    }
    return 1;
}

static int
on_complete_postion_cmp(complete_t *pvec, intptr_t pos)
{
    if (pvec)
    {
        for (int j = 0; j < OVEC_LEN && pvec->pos[j].min > 0; ++j)
        {
            if (pos >= pvec->pos[j].min && pos < pvec->pos[j].max)
            {
            #if APP_DEBUG
                printf("prev_pos = %Id, pvec->pos[%d].min = %Id, pvec->pos[%d].max = %Id\n", pos, j, pvec->pos[j].min, j, pvec->pos[j].max);
            #endif
                return 0;
            }
        }
    }
    return 1;
}

static uint32_t
on_complete_build_flags(eu_tabpage *pnode, const char *key)
{
    uint32_t flags = 0;
    if (pnode && pnode->doc_ptr && pnode->doc_ptr->ptrv)
    {
        snippet_t *it = NULL;
        cvector_for_each_and_cmp(pnode->doc_ptr->ptrv, on_complete_str_match, key, &it);
        if (it && strlen(it->parameter) > 0)
        {
            if (strchr(it->parameter, 'b'))
            {
                flags |= COMPLETE_LINE_HEADER;
            }
            if (strchr(it->parameter, 'r'))
            {
                flags |= COMPLETE_WITH_REGXP;
            }
            if (strchr(it->parameter, 'A'))
            {
                flags |= COMPLETE_AUTO_EXPAND;
            }
        }
    }
    return flags;
}

static inline void
on_complete_muti_autoc(eu_tabpage *pnode)
{
    if (pnode)
    {
        if (!eu_sci_call(pnode, SCI_AUTOCGETMULTI, 0, 0))
        {
            eu_sci_call(pnode, SCI_AUTOCSETMULTI, SC_MULTIAUTOC_EACH, 0);
        }
        if (eu_sci_call(pnode, SCI_GETCARETPERIOD, 0, 0) > 0)
        {
            eu_sci_call(pnode, SCI_SETCARETPERIOD, 0, 0);
        }
    }
}

static void
on_complete_replace_callback(void *p1, void **p2, const char *by, int start, int offset)
{
    complete_t *pvec = (complete_t *)p1;
    auto_postion *pos = (auto_postion **)p2 ? *(auto_postion **)p2 : NULL;
    if (pvec && pos)
    {
        int add = 0;
        char *p = NULL;
        complete_t *oit = NULL;
        const char *psub = by;
        while ((p = strchr(psub, '$')) && strlen(p) > 1 && UTIL_BASE10(p[1]))
        {
            add += eu_int_cast(p - psub);
            cvector_for_each_and_cmp(pvec, on_complete_char_cmp, p[1], &oit);
            if (oit)
            {
                psub = oit->value;
            }
        }
        pos->min = start;
        pos->max = pos->min + add + (intptr_t)strlen(psub);
        for (complete_t *it = cvector_begin(pvec); it != cvector_end(pvec); ++it)
        {
            for (int j = 0; j < OVEC_LEN; ++j)
            {
                if (it->pos[j].min > start)
                {
                    it->pos[j].min -= offset;
                    it->pos[j].max -= offset;
                }
            }
        }
        (*(auto_postion **)p2)++;
    }
}

static void
on_complete_replace_group(void *p1, void **p2, const char *by, int start, int offset)
{
    eu_tabpage *p = (eu_tabpage *)p1;
    complete_t *pvec = p ? p->ac_vec : NULL;
    if (p && pvec)
    {
        for (complete_t *it = cvector_begin(pvec); it != cvector_end(pvec); ++it)
        {
            for (int j = 0; j < OVEC_LEN; ++j)
            {
                if (it->pos[j].min == start)
                {
                    it->pos[j].max = it->pos[j].min + (intptr_t)strlen(by);
                }
                else if (it->pos[j].min > start)
                {
                    it->pos[j].min -= offset;
                    it->pos[j].max -= offset;
                }
            }
        }
    }
}

static int
on_complete_replace_fn(char *in, const size_t in_size, const char *pattern, const char *by, ptr_replace fn_replace, void *p1, void *p2)
{
    char *in_ptr = in;
    char res[VALUE_LEN] = {0};
    size_t offset = 0;
    char *needle;
    while ((needle = strstr(in, pattern)) && offset < in_size && offset < VALUE_LEN)
    {
        if (fn_replace)
        {
            fn_replace(p1, &p2, by, eu_int_cast(needle - in + strlen(res)), eu_int_cast(strlen(pattern) - strlen(by)));
        }
        strncpy(res + offset, in, needle - in);
        offset += needle - in;
        in = needle + (int) strlen(pattern);
        strncpy(res + offset, by, VALUE_LEN - offset);
        offset += (int) strlen(by);
    }
    strncpy(res + offset, in, VALUE_LEN - offset);
    _snprintf(in_ptr, eu_int_cast(in_size), "%s", res);
    in = in_ptr;
    return 0;
}

static int
on_complete_init_regroup(eu_tabpage *pnode, const char *pstr, int start, int end)
{
    capture_set data = {0};
    if (pnode && pstr)
    {
        int length = end - start;
        int size = eu_int_cast(cvector_size(pnode->re_group));
        if (length > 0 && length < MAX_SIZE)
        {
            snprintf(data.str, MAX_SIZE - 1, "%.*s", length, &pstr[start]);
            _snprintf(data.cap, GROUP_SIZE, "\\%d", size + 1);
            cvector_push_back(pnode->re_group, data);
        }
    }
    return 0;
}

static bool
on_complete_get_str(eu_tabpage *pnode, const char *key, char **pstr)
{
    bool ret = false;
    snippet_t *it;
    snippet_t *pv = pnode->doc_ptr->ptrv;
    *pstr = NULL;
    if (!pv)
    {
        return false;
    }
    if (!(*pstr = (char *)calloc(1, VALUE_LEN)))
    {
        return false;
    }
    if (pnode->re_group)
    {
        cvector_clear(pnode->re_group);
    }
    for (it = cvector_begin(pv); it != cvector_end(pv); ++it)
    {
        if ((_stricmp(it->name, key) == 0))
        {
            _snprintf(*pstr, VALUE_LEN - 1, "%s", it->body);
            ret = true;
            break;
        }
        else if (strchr(it->parameter, 'r') && on_complete_pcre_match(pnode, key, it->name, on_complete_init_regroup) == SKYLARK_OK)
        {
            _snprintf(*pstr, VALUE_LEN - 1, "%s", it->body);
            ret = true;
            break;
        }
    }
    return ret;
}

static int
on_complete_get_key(eu_tabpage *pnode, char *key, int len, sptr_t *ptr_pos, char **pstr)
{
    int m_indent = -1;
    if (pnode && pnode->doc_ptr && pnode->doc_ptr->ptrv)
    {
        snippet_t *it = NULL;
        on_complete_set_word(pnode);
        sptr_t pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
        sptr_t word_start = eu_sci_call(pnode, SCI_WORDSTARTPOSITION, pos, true);
        sptr_t word_end = eu_sci_call(pnode, SCI_WORDENDPOSITION, pos, true);
        sptr_t line = eu_sci_call(pnode, SCI_LINEFROMPOSITION, pos, 0);
        m_indent = (int)eu_sci_call(pnode, SCI_GETLINEINDENTATION, line, 0);
        if (word_end - word_start >= len - 1)
        {
            word_end = word_start + len - 1;
        }
        Sci_TextRange tr = {{word_start, word_end}, key};
        eu_sci_call(pnode, SCI_GETTEXTRANGE, 0, (sptr_t) &tr);
        on_complete_unset_word(pnode);
        if (!on_complete_get_str(pnode, key, pstr))
        {
            m_indent = -1;
        }
        else
        {
            eu_sci_call(pnode, SCI_SETSELECTION, word_start, word_end);
        }
        if (ptr_pos)
        {
            *ptr_pos = word_start;
        }
    }
    return m_indent;
}

static bool
on_complete_at_header(eu_tabpage *pnode, const sptr_t pos)
{
    if (pnode)
    {
        sptr_t lineno = eu_sci_call(pnode, SCI_LINEFROMPOSITION, pos, 0);
        sptr_t linestart = eu_sci_call(pnode, SCI_POSITIONFROMLINE, lineno, 0);
        return (pos - linestart == 0);
    }
    return false;
}

static int
on_complete_pcre_match(eu_tabpage *pnode, const char *pstr, const char *exp, ptr_re_callback fn)
{
    pcre *re;
    int erroffset;
    int ovector[OVECCOUNT], rc;
    const char *err;
    char *src = (char *)pstr;
    int src_len = eu_int_cast(strlen(src));
    char *pattern = util_str_unquote(exp);
    if (!pattern)
    {
        return EUE_POINT_NULL;
    }
    if (NULL == (re = pcre_compile(pattern, PCRE_NO_UTF8_CHECK|PCRE_MULTILINE|PCRE_DOTALL, &err, &erroffset, NULL)))
    {
        eu_logmsg("pcre: compile error[%s], %s\n", pattern, err);
        free(pattern);
        return EUE_PCRE_EXP_ERR;
    }
    rc = pcre_exec(re, NULL, src, src_len, 0, 0, ovector, OVECCOUNT);
    if (rc < 0)
    {
        eu_logmsg("pcre: [%s] didn't match [%s]\n", pattern, pstr);
        free(pattern);
        pcre_free(re);
        return EUE_PCRE_NO_MATCHING;
    }
    if (fn)
    {
        for (int i = 1; i < rc; ++i)
        {
            if (fn(pnode, src, ovector[2*i], ovector[2*i + 1]))
            {
                eu_logmsg("pcre: callback abort\n");
                break;
            }
        }
    }
    free(pattern);
    pcre_free(re);
    return SKYLARK_OK;
}

static int
on_complete_parser_brace(const char *psrc, char *p, char *buf)
{
    int ret = -1;
    if (psrc && p && buf)
    {
        char *cp1 = (char *)p;
        char *cp2 = NULL;
        char *eos = NULL;
        int next_var = 1;  // 下一个变量
        cp2 = buf;
        eos = cp2 + MAX_BUFFER - 1;
        while ((*cp1 != '\0') && (cp2 != eos))
        {
            switch (*cp1)
            {
                case '{':
                    if (str_prev(cp1) == '$')
                    {   /* 设置进入嵌套变量 */
                        ++next_var;
                    }
                    break;
                case '}':
                    if (str_prev(cp1) != '\\')
                    {
                        if (next_var > 0)
                        {
                            --next_var;
                        }
                    }
                    break;
                default:
                    break;
            }
            /* 找到变量结束符 */
            if (*cp1 == '}' && str_prev(cp1) != '\\' && !next_var)
            {
                *cp2 = '\0';
                ret = eu_int_cast(cp1 - psrc + 1);
                break;
            }
            else
            {
                *cp2++ = *cp1;
            }
            ++cp1;
        }
    }
    return ret;
}

static void
on_complete_vec_printer(complete_t *pv)
{
    if (pv)
    {
        complete_t *it;
        int i = 0;
        for (it = cvector_begin(pv); it != cvector_end(pv); ++it, ++i)
        {
        #if APP_DEBUG
            printf("pv[%d] = %d, %s, %s,", i, it->index, it->value, it->word);
        #endif
            for (int j = 0; j < OVEC_LEN; ++j)
            {
                if (it->pos[j].min >= 0)
                {
                #if APP_DEBUG
                    printf("it->pos[%d].min = %Id, it->pos[%d].max = %Id\n", j, it->pos[j].min, j, it->pos[j].max);
                #endif
                }
            }
        }
    }
}

static int
on_complete_init_ac_vec(const char *psrc, char *p, int index, complete_t **pvec)
{
    int ret = -1;
    complete_t data;
    on_complete_vec_init(&data);
    if (!(psrc && p && pvec))
    {
        return ret;
    }
    if ((ret = on_complete_parser_brace(psrc, p, data.value)) > 0)
    {
        data.index = index;
        _snprintf(data.word, MAX_BUFFER - 1, "${%d:%s}", data.index, data.value);
        cvector_push_back(*pvec, data);
    }
    return ret;
}

static void
on_complete_parser_postion(eu_tabpage *pnode)
{
    for (complete_t *it = cvector_begin(pnode->ac_vec); it != cvector_end(pnode->ac_vec); ++it)
    {
        for (int j = 0; j < OVEC_LEN; ++j)
        {
            if (it->pos[j].min >= 0)
            {
                it->pos[j].max = it->pos[j].min + (intptr_t)strlen(it->value);
            }
        }
    }
}

static void
on_complete_parser_value(eu_tabpage *pnode)
{
    char *p = NULL;
    char *psrc = NULL;
    char pvalue[MAX_BUFFER] = {0};
    for (complete_t *it = cvector_begin(pnode->ac_vec); it != cvector_end(pnode->ac_vec); ++it)
    {
        psrc = it->value;
        while (((p = strchr(psrc, '$')) && str_prev(p) != '\\') && 
               ((strlen(p) > 1 && UTIL_BASE10(p[1])) ||
               (strlen(p) > 3 && p[1] == '{' && UTIL_BASE10(p[2]) && p[3] == ':'))
              )
        {
            complete_t *oit = NULL;
            cvector_for_each_and_cmp(pnode->ac_vec, on_complete_char_cmp, UTIL_BASE10(p[1]) ? p[1] : p[2], &oit);
            if (oit)
            {
                if (UTIL_BASE10(p[1]))
                {
                    _snprintf(pvalue, MAX_BUFFER, "$%d", UTIL_NUMBER(p[1]));
                }
                else
                {
                    _snprintf(pvalue, MAX_BUFFER, "${%d:%s}", UTIL_NUMBER(p[2]), oit->value);
                }
                on_complete_replace_fn(psrc, MAX_BUFFER, pvalue, oit->value, NULL, NULL, NULL);
            }
            else
            {
                psrc = p + 2;
            }
        }
    }
}

static int
on_complete_multi_match(eu_tabpage *pnode, const char *pstr)
{
    char *p = NULL;
    char *psrc = (char *)pstr;
    while ((p = strstr(psrc, "${")) && str_prev(p) != '\\' && strlen(p) > 3 && UTIL_BASE10(p[2]) && p[3] == ':')
    {
        on_complete_init_ac_vec(pstr, &p[4], UTIL_NUMBER(p[2]), &pnode->ac_vec);
        psrc = p + 4;
    }
    return SKYLARK_OK;
}

static void
on_complete_do_replace(eu_tabpage *pnode, char *pstr)
{
    char *p = NULL;
    char *psrc = pstr;
    char res[VALUE_LEN] = {0};
    while (((p = strchr(psrc, '$')) && str_prev(p) != '\\') && 
           ((strlen(p) > 1 && UTIL_BASE10(p[1])) ||
           (strlen(p) > 3 && p[1] == '{' && UTIL_BASE10(p[2]) && p[3] == ':'))
          )
    {
        complete_t *oit = NULL;
        cvector_for_each_and_cmp(pnode->ac_vec, on_complete_char_cmp, UTIL_BASE10(p[1]) ? p[1] : p[2], &oit);
        if (!oit)
        {
            if (p[1] == '{')
            {   // 语法错误
                on_complete_reset_focus(pnode);
                break;
            }
            else
            {   // 可能存在没匹配的占位符$0..$9, 写入vec数组
                complete_t data;
                on_complete_vec_init(&data);
                data.index = UTIL_NUMBER(p[1]);
                cvector_push_back(pnode->ac_vec, data);
                oit = &pnode->ac_vec[cvector_size(pnode->ac_vec) - 1];
            }
        }
        for (int j = 0; j < OVEC_LEN; ++j)
        {
            if (oit->pos[j].min < 0)
            {
                oit->pos[j].min = (p - pstr);
                break;
            }
        }
        if (UTIL_BASE10(p[1]))
        {
            _snprintf(res, VALUE_LEN - 1, "%s", p + 2);
        }
        else
        {
            _snprintf(res, VALUE_LEN - 1, "%s", p + strlen(oit->word));
        }
        _snprintf(p, VALUE_LEN - (p - pstr) - 1, "%s%s", oit->value, res);
        psrc = pstr;
    }
}

static void
on_complete_replace(eu_tabpage *pnode, char *pstr, const char *space)
{
    if (pnode && pstr && space)
    {
        int len = eu_int_cast(strlen(pstr));
        char eols[MAX_BUFFER] = {0};
        const char *str_eol = NULL;
        const char *doc_eol = on_encoding_get_eol(pnode);
        int eol = on_encoding_line_eol(pstr, strlen(pstr));
        str_eol = (eol == SC_EOL_LF) ? "\n" : ((eol == SC_EOL_CR) ? "\r" : "\r\n");
        // 去除字符串末尾的换行符
        if (pstr[len - 1] == '\r')
        {
            pstr[len - 1] = 0;
        }
        else if (pstr[len - 1] == '\n')
        {
            if (pstr[len - 2] == '\r')
            {
                pstr[len - 2] = 0;
            }
            else
            {
                pstr[len - 1] = 0;
            }
        }
        // 使用新的换行符+对齐空格替换旧的换行符
        _snprintf(eols, MAX_BUFFER - 1, "%s%s", doc_eol, space);
        eu_str_replace(pstr, VALUE_LEN, str_eol, eols);
        int width = (int)eu_sci_call(pnode, SCI_GETTABWIDTH, 0, 0);
        char *str_width = width > 0 ? (char *)calloc(1, width+1) : NULL;
        if (str_width)
        {
            memset(str_width, 0x20, width);
            if (eu_sci_call(pnode, SCI_GETUSETABS, 0, 0))
            {   // 如果是tab对齐, 使用tab替换掉可能存在的空格
                eu_str_replace(pstr, VALUE_LEN, str_width, "\t");
            }
            else
            {   // 如果是空格对齐, 使用空格替换掉可能存在的tab字符
                eu_str_replace(pstr, VALUE_LEN, "\t", str_width);
            }
            free(str_width);
        }
        if (pnode->ac_vec)
        {
            cvector_clear(pnode->ac_vec);
        }
        // 搜索表达式占位符并生成当前标签页的av_vec数组
        if (on_complete_multi_match(pnode, pstr) == SKYLARK_OK)
        {
            char *p = NULL;
            char *psrc = NULL;
            char tmp[3] = {0};
            complete_t *oit = NULL;
            on_complete_do_replace(pnode, pstr);
            on_complete_parser_value(pnode);
            on_complete_parser_postion(pnode);
        #ifdef APP_DEBUG
            printf("============ ac_vec start ===========\n");
            on_complete_vec_printer(pnode->ac_vec);
        #endif
            // 替换捕获组成员\1...\9
            if (pnode->re_group && cvector_size(pnode->re_group) > 0)
            {
                complete_t *it;
                // 替换value里面捕获组成员
                for (it = cvector_begin(pnode->ac_vec); it != cvector_end(pnode->ac_vec); ++it)
                {
                    psrc = it->value;
                    while ((p = strchr(psrc, '\\')) && str_prev(p) != '\\' && UTIL_BASE10(p[1]))
                    {
                        _snprintf(tmp, 3, "\\%c", p[1]);
                        capture_set *cit = NULL;
                        cvector_for_each_and_cmp(pnode->re_group, on_complete_group_cmp, p[1], &cit);
                        if (cit)
                        {
                            eu_str_replace(it->value, MAX_BUFFER, tmp, cit->str);
                        }
                        else
                        {
                            psrc = p + 1;
                        }
                    }
                }
                // 替换字符串里面的捕获组成员
                for (capture_set *it = cvector_begin(pnode->re_group); it != cvector_end(pnode->re_group); ++it)
                {
                    on_complete_replace_fn(pstr, VALUE_LEN, it->cap, it->str, on_complete_replace_group, pnode, NULL);
                }
            }
            // 如果捕获组为空, 则把 \1...\9 替换为空字符串
            psrc = pstr;
            while ((p = strchr(psrc, '\\')) && str_prev(p) != '\\' && UTIL_BASE10(p[1]))
            {
                _snprintf(tmp, 3, "\\%c", p[1]);
                on_complete_replace_fn(pstr, VALUE_LEN, tmp, "", on_complete_replace_group, pnode, NULL);
            }
        }
    }
}

static int
on_complete_sort_callback(const void* _a, const void* _b)
{
    int* a = (int*)_a;
    int* b = (int*)_b;
    return *a - *b;
}

static int
on_complete_vec_cmp(const void* _a, const void* _b)
{
    return (*(complete_t *)_a).index > (*(complete_t *)_b).index ? 1 : -1;
}

static int
on_complete_struct_cmp(const void* _a, const void* _b)
{
    return (*(auto_postion *)_a).min > (*(auto_postion *)_b).min ? 1 : -1;
}

static void
on_complete_update_vec(complete_t *it, intptr_t pos)
{
    if (it && pos >= 0)
    {
        int size = 0;
        for (int j = 0; j < OVEC_LEN; ++j)
        {
            if (it->pos[j].min >= 0)
            {
                it->pos[j].min += pos;
                it->pos[j].max += pos;
                ++size;
            }
        }
        if (size > 1)
        {
            qsort(it->pos, size, sizeof(auto_postion), on_complete_struct_cmp);
        }
    }
}

static bool
on_complete_var_embed(eu_tabpage *pnode, complete_t *it, int index, complete_t **pvec)
{
    bool msub = false;
    char *p = NULL;
    char *psrc = it->word;
    complete_t *oit = NULL;
    *pvec = NULL;
    if ((p = strchr(psrc, ':')))
    {
        psrc = p;
    }
    while ((psrc && (p = strchr(psrc, '$')) && str_prev(p) != '\\') &&
          (UTIL_BASE10(p[1]) || (strlen(p) > 3 && UTIL_BASE10(p[2]) && p[3] == ':')))
    {
        int ch = UTIL_BASE10(p[1]) ? p[1] : p[2];
        if (UTIL_NUMBER(ch) == index)
        {
            msub = true;
        }
        oit = NULL;
        cvector_for_each_and_cmp(pnode->ac_vec, on_complete_char_cmp, ch, &oit);
        if (oit)
        {
            psrc = strchr(oit->word, ':');
        }
        else
        {
            psrc = NULL;
        }
    }
    if (msub && oit)
    {
        *pvec = oit;
    }
    return msub;
}

static void
on_complete_sort_update(eu_tabpage *pnode, complete_t *it, int **pv, int offset, int index)
{
    if (it && pv && cvector_size(*pv) > 0)
    {
        int first_pos = (*pv)[0];
        for (int j = 0; j < OVEC_LEN; ++j)
        {   // 更新所有大于此变量位置的节点
            if (it->pos[j].min > first_pos)
            {
                bool msub = false;
                complete_t *oit = NULL;
                cvector_push_back((*pv), eu_int_cast(it->pos[j].min));
                qsort((*pv), cvector_size((*pv)), sizeof(int), on_complete_sort_callback);
                int m = eu_cvector_at((*pv), eu_int_cast(it->pos[j].min));
                if (m > 0)
                {
                    it->pos[j].min += (m * offset);
                    it->pos[j].max += (m * offset);
                }
                cvector_erase((*pv), (size_t)m);
                if ((msub = on_complete_var_embed(pnode, it, index, &oit)) && oit && oit != it)
                {
                    Sci_TextRange tr = {{it->pos[j].min, it->pos[j].max + offset}, it->value};
                    eu_sci_call(pnode, SCI_GETTEXTRANGE, 0, (sptr_t) &tr);
                }
            }
        }
    }
}

static void
on_complete_update_part(eu_tabpage *pnode, complete_t *pvec, int offset)
{
    bool msub = false;
    int i = 0;
    complete_t *oit = NULL;
    for (complete_t *it = cvector_begin(pnode->ac_vec); it != cvector_end(pnode->ac_vec); ++it)
    {   // pvec 也可能包含别的变量
        if ((msub = on_complete_var_embed(pnode, pvec, it->index, &oit)) && oit)
        {
            msub = false;
            for (i = 0; i < OVEC_LEN && oit->pos[i].min >= 0; ++i)
            {
                if (!on_complete_equal_value(pnode, oit, i))
                {   // 子变量位置信息被破坏
                    oit->pos[i].min = -1;
                    oit->pos[i].max = -1;
                    on_complete_exchang_end(it, i);
                    i = 0;
                }
            }
        }
        // pvec变量可能被某个变量包含
        if (it != pvec && (msub = on_complete_var_embed(pnode, it, pvec->index, &oit)) && oit)
        {
            msub = false;
            for (i = 0; !msub && i < OVEC_LEN; ++i)
            {
                for (int j = 0; j < OVEC_LEN; ++j)
                {
                    if (pvec->pos[i].min >= 0 && it->pos[j].min >= 0 && it->pos[j].min <= pvec->pos[i].min &&
                        it->pos[j].min + (intptr_t)strlen(it->value) >= pvec->pos[i].max)
                    {
                        msub = true;
                        eu_logmsg("we can affirm that %s include %s\n", it->value, pvec->value);
                        break;
                    }
                }
            }
            if (msub)
            {
                it->pos[0].max += offset;
                if (it->pos[0].max - it->pos[0].min >= 0 && it->pos[0].max - it->pos[0].min < MAX_BUFFER)
                {
                    Sci_TextRange tr = {{it->pos[0].min, it->pos[0].max}, it->value};
                    eu_sci_call(pnode, SCI_GETTEXTRANGE, 0, (sptr_t) &tr);
                }
                for (i = 1; i < OVEC_LEN; ++i)
                {
                    if (it->pos[i].min >= 0)
                    {
                        it->pos[i].max += offset;
                    }
                }
            }
        }
    }
}

static bool
on_complete_update_postion(eu_tabpage *pnode, complete_t **ptr_vec, bool back)
{
    int offset = 0;
    complete_t *oit = NULL;
    intptr_t current_pos = -1;
    cvector_vector_type(int) v = NULL;
    bool bsel = (bool)eu_sci_call(pnode, SCI_GETSELECTIONEMPTY, 0, 0);
    if (bsel)
    {
        current_pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
    }
    else
    {
        current_pos = eu_sci_call(pnode, SCI_GETSELECTIONNCARET, 0, 0);
    }
    if (current_pos >= 0)
    {   // 根据上一个焦点, 获取要跳转的vec index
        if (last_snippet_focus >= 0 && last_snippet_focus < (int)cvector_size(pnode->ac_vec))
        {
            oit = &pnode->ac_vec[last_snippet_focus];
        }
    }
    if (oit)
    {
        if (ptr_vec)
        {
            *ptr_vec = oit;
        }
        if (oit->pos[0].min > current_pos)
        {
            pnode->ac_mode = AUTO_NONE;
            return false;
        }
        if (oit->pos[0].min == current_pos && !back)
        {
            pnode->ac_mode = oit->index ? AUTO_CODE : AUTO_NONE;
            return oit->index ? true : false;
        }
        if (current_pos > oit->pos[0].min && current_pos - oit->pos[0].min < MAX_BUFFER)
        {
            memset(oit->value, 0, sizeof(oit->value));
            Sci_TextRange tr = {{oit->pos[0].min, current_pos}, oit->value};
            eu_sci_call(pnode, SCI_GETTEXTRANGE, 0, (sptr_t) &tr);
            if (oit->value[0])
            {
                offset = eu_int_cast(current_pos - oit->pos[0].max);
                if (offset)
                {
                    oit->pos[0].max = current_pos;
                    cvector_push_back(v, eu_int_cast(oit->pos[0].min));
                    for (int j = 1; j < OVEC_LEN && oit->pos[j].max > 0; ++j)
                    {   // 把min元素放入整型数组v, 准备排序
                        cvector_push_back(v, eu_int_cast(oit->pos[j].min));
                        oit->pos[j].min += (j * offset);
                        oit->pos[j].max = oit->pos[j].min + (intptr_t)strlen(oit->value);
                    }
                }
            }
        }
    }
    if (oit)
    {   // 找出位置在它们之前或之后的vec, 更新位置信息
        for (complete_t *it = cvector_begin(pnode->ac_vec); offset && it != cvector_end(pnode->ac_vec); ++it)
        {
            if (it != oit)
            {
                on_complete_sort_update(pnode, it, &v, offset, oit->index);
            }
        }
        // 变量嵌套时, 更新位置信息
        on_complete_update_part(pnode, oit, offset);
    }
    cvector_freep(&v);
    return true;
}

static bool
on_complete_snippet_jmp(eu_tabpage *pnode, complete_t *it)
{
    if (pnode && it && it->index >= 0)
    {
        bool main_sel = false;
        eu_sci_call(pnode, SCI_SETEMPTYSELECTION, it->pos[0].min, 0);
        if (on_complete_equal_value(pnode, it, 0))
        {
            eu_sci_call(pnode, SCI_SETSELECTION, it->pos[0].min, it->pos[0].max);
            main_sel = true;
        }
        for (int j = 1; j < OVEC_LEN && it->pos[j].min > 0 && it->pos[j].max - it->pos[j].min < MAX_BUFFER; ++j)
        {
            if (on_complete_equal_value(pnode, it, j))
            {
                if (!main_sel)
                {
                    eu_sci_call(pnode, SCI_SETSELECTION, it->pos[j].min, it->pos[j].max);
                    main_sel = true;
                    on_complete_swap(&it->pos[j], &it->pos[j - 1]);
                    it->pos[j].min = -1, it->pos[j].max = -1;
                    // 把损坏的值初始化后放到数组最后.
                    on_complete_exchang_end(it, j);
                }
                else
                {
                    eu_sci_call(pnode, SCI_ADDSELECTION, it->pos[j].min, it->pos[j].max);
                }
            }
        }
        eu_sci_call(pnode, SCI_SETMAINSELECTION, 0, 0);
        if (!it->index)
        {   // 已经到达$0
            on_complete_reset_focus(pnode);
        }
        return true;
    }
    return false;
}

static const char *
on_complete_get_func(eu_tabpage *pnode, const char *key)
{
    if (pnode && pnode->doc_ptr && pnode->doc_ptr->ptrv)
    {
        snippet_t *it = NULL;
        cvector_for_each_and_cmp(pnode->doc_ptr->ptrv, on_complete_str_match, key, &it);
        if (it)
        {
            return key;
        }
    }
    return NULL;
}

static void
on_complete_call_autocshow(eu_tabpage *pnode, const char *word_buffer, const sptr_t current_pos, const sptr_t start_pos)
{
    char *key = NULL;
    const char *snippet_str = NULL;
    int flags = SC_AUTOCOMPLETE_FIXED_SIZE;
    if (eu_get_config()->eu_complete.snippet)
    {
        uint32_t mark = 0;
        snippet_str = on_complete_get_func(pnode, word_buffer);
        if (snippet_str)
        {
            mark = on_complete_build_flags(pnode, word_buffer);
        }   // 如果存在参数b, 但关键字没在行首
        if ((mark & COMPLETE_LINE_HEADER) && !on_complete_at_header(pnode, start_pos))
        {
            snippet_str = NULL;
        }
    }
    if ((key = eu_find_completed_tree(&pnode->doc_ptr->acshow_tree, word_buffer, snippet_str)) && key[0])
    {
        if (snippet_str)
        {
            flags |= SC_AUTOCOMPLETE_SNIPPET;
        }
        eu_sci_call(pnode, SCI_AUTOCSETOPTIONS, flags, 0);
        eu_sci_call(pnode, SCI_AUTOCSHOW, current_pos - start_pos, (sptr_t)key);
    }
    eu_safe_free(key);
}

static void
on_complete_any_autocshow(eu_tabpage *pnode)
{
    if (pnode && pnode->doc_ptr && eu_get_config()->eu_complete.enable)
    {
        char *key = eu_find_completed_tree(&pnode->doc_ptr->acshow_tree, ANY_WORD, NULL);
        if (STR_NOT_NUL(key))
        {
            eu_sci_call(pnode, SCI_AUTOCSETOPTIONS, SC_AUTOCOMPLETE_FIXED_SIZE, 0);
            eu_sci_call(pnode, SCI_AUTOCSHOW, 0, (sptr_t) key);
        }
        eu_safe_free(key);
    }
}

bool
on_complete_auto_expand(eu_tabpage *pnode, const char *key, const sptr_t start_pos)
{
    uint32_t mark = on_complete_build_flags(pnode, key);
    bool expand = (mark & COMPLETE_AUTO_EXPAND);
    if (expand)
    {
        if ((mark & COMPLETE_LINE_HEADER) && !on_complete_at_header(pnode, start_pos))
        {
            expand = false;
        }
    }
    if (mark > 0 && expand && eu_get_config()->eu_complete.snippet && pnode->ac_mode != AUTO_CODE)
    {
        return true;
    }
    return false;
}

void
on_complete_doc(eu_tabpage *pnode, ptr_notify lpnotify)
{   /* 自动补全提示 */
    if (pnode && pnode->doc_ptr)
    {
        char word_buffer[MAX_SIZE+1] = {0};
        on_complete_set_word(pnode);
        sptr_t current_pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
        sptr_t start_pos = eu_sci_call(pnode, SCI_WORDSTARTPOSITION, current_pos - 1, true);
        sptr_t end_pos = eu_sci_call(pnode, SCI_WORDENDPOSITION, current_pos - 1, true);
        if (end_pos - start_pos >= MAX_SIZE)
        {
            end_pos = start_pos + MAX_SIZE;
        }
        Sci_TextRange tr = {{start_pos, end_pos}, word_buffer};
        eu_sci_call(pnode, SCI_GETTEXTRANGE, 0, (sptr_t) &tr);
        if (word_buffer[0])
        {
            if (on_complete_auto_expand(pnode, word_buffer, start_pos))
            {
                on_complete_reset_focus(pnode);
                on_complete_snippet(pnode);
            }
            else if (eu_get_config()->eu_complete.enable && !RB_EMPTY_ROOT(&(pnode->doc_ptr->acshow_tree)) && end_pos - start_pos >= eu_get_config()->eu_complete.characters)
            {
                on_complete_call_autocshow(pnode, word_buffer, current_pos, start_pos);
            }
        }
        on_complete_unset_word(pnode);
    }
}

void
on_complete_html(eu_tabpage *pnode, ptr_notify lpnotify)
{
    sptr_t n_pos = 0;
    sptr_t current_pos = 0;
    if (pnode && pnode->doc_ptr && lpnotify && lpnotify->ch > 0)
    {   /* html类文档自动补全提示 */
        int ch = 0;
        int ch_pre = 0;
        if (lpnotify->ch == '<')
        {
            on_complete_any_autocshow(pnode);
        }
        else if (lpnotify->ch == ' ')
        {
            current_pos = (int) eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
            for (n_pos = current_pos - 1; n_pos >= 0; n_pos--)
            {
                ch_pre = ch;
                ch = (int) eu_sci_call(pnode, SCI_GETCHARAT, n_pos, 0);
                if (ch == '<' || ch == '>')
                {
                    break;
                }
            }
            if (n_pos >= 0 && ch == '<' && ch_pre != '?' && ch_pre != '%')
            {
                on_complete_any_autocshow(pnode);
            }
        }
        else if (isalpha(lpnotify->ch))
        {
            on_complete_set_word(pnode);
            current_pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
            sptr_t start_pos = eu_sci_call(pnode, SCI_WORDSTARTPOSITION, current_pos - 1, true);
            sptr_t end_pos = eu_sci_call(pnode, SCI_WORDENDPOSITION, current_pos - 1, true);
            if (end_pos - start_pos >= MAX_SIZE)
            {
                end_pos = start_pos + MAX_SIZE;
            }
            if (start_pos > 0 && end_pos - start_pos >= eu_get_config()->eu_complete.characters)
            {
                bool is_attr = false;
                char word_buffer[MAX_SIZE+1];
                for (ch = 0, n_pos = current_pos; n_pos >= 0; n_pos--)
                {
                    ch_pre = ch;
                    ch = (int) eu_sci_call(pnode, SCI_GETCHARAT, n_pos, 0);
                    if (ch == ' ')
                    {
                        is_attr = true;
                    }
                    else if (ch == '<' || ch == '>')
                    {
                        break;
                    }
                }
                if (ch > 0 && (is_attr || (ch == '<' && ch_pre != '?' && ch_pre != '%')))
                {
                    memset(word_buffer, 0, sizeof(word_buffer));
                    Sci_TextRange tr = {{start_pos, end_pos}, word_buffer};
                    eu_sci_call(pnode, SCI_GETTEXTRANGE, 0, (sptr_t) &tr);
                    if (word_buffer[0])
                    {
                        if (on_complete_auto_expand(pnode, word_buffer, start_pos))
                        {
                            on_complete_reset_focus(pnode);
                            on_complete_snippet(pnode);
                        }
                        else if (eu_get_config()->eu_complete.enable && !RB_EMPTY_ROOT(&(pnode->doc_ptr->acshow_tree)))
                        {
                            on_complete_call_autocshow(pnode, word_buffer, current_pos, start_pos);
                        }
                    }
                }
            }
            on_complete_unset_word(pnode);
        }
    }
}

bool
on_complete_snippet_back(eu_tabpage *pnode)
{
    bool ret = false;
    while (pnode && pnode->ac_vec && cvector_size(pnode->ac_vec) > 0 && pnode->ac_mode != AUTO_NONE)
    {
        complete_t *oit = NULL;
        complete_t *it = NULL;
        if (!on_complete_update_postion(pnode, &oit, true))
        {
            on_complete_reset_focus(pnode);
            ret = false;
            break;
        }
        if (oit)
        {
            it = oit > pnode->ac_vec ? &pnode->ac_vec[oit - pnode->ac_vec - 1] : NULL;
        }
        if (it)
        {   // 解析vec数组, 设置shift+tab跳转
            ret = on_complete_snippet_jmp(pnode, it);
            if (ret && pnode->ac_vec)
            {
                on_complete_dec_focus();
            }
        }
        else
        {
            on_complete_reset_focus(pnode);
            ret = true;
        }
        break;
    }
    return ret;
}

bool
on_complete_snippet(eu_tabpage *pnode)
{
    size_t size = 0;
    bool ret = false;
    char *str = NULL;
    complete_t *it = NULL;
    if (!pnode)
    {
        return false;
    }
    if (pnode->ac_vec && (size = cvector_size(pnode->ac_vec)) > 0 && pnode->ac_mode != AUTO_NONE)
    {   // 光标跳转前, 先更新前一个操作受影响变量的位置信息
        if (!on_complete_update_postion(pnode, &it, false))
        {
            eu_logmsg("on_complete_update_postion failed\n");
            on_complete_reset_focus(pnode);
            return false;
        }
        if (it)
        {   
            if (it - pnode->ac_vec < (intptr_t)(size - 1))
            {   // 解析vec数组, 设置tab跳转
                ++it;
                ret = on_complete_snippet_jmp(pnode, it);
                if (ret && pnode->ac_vec)
                {
                    on_complete_inc_focus();
                }
            }
            else
            {   // 处理完了最后一个变量
                on_complete_reset_focus(pnode);
                ret = true;
            }
        }
        return ret;
    }
    while (pnode->doc_ptr && pnode->doc_ptr->snippet && pnode->ac_mode != AUTO_CODE)
    {
        intptr_t pos = -1;
        char key[MAX_SIZE] = {0};
        // 获取解析过的snippets配置文件, 替换占位符并生成ac_vec
        if (pnode->doc_ptr->ptrv && cvector_size(pnode->doc_ptr->ptrv) > 0)
        {   // 获取激活片段的关键字, 即snippet_t.name
            // 返回的count是指明关键字前面有多少空白字符
            int count = on_complete_get_key(pnode, key, MAX_SIZE, &pos, &str);
            if ((ret = count >= 0 && key[0] && str))
            {   // 通过snippet_t.name, 保存snippet_t.body到str
                if (ret)
                {
                    char *str_space = (char *)calloc(1, count+1);
                    if (!str_space)
                    {
                        break;
                    }
                    if (count > 0)
                    {
                        memset(str_space, 0x20, count);
                    }
                    // 替换所有占位符, 换行符, 正则捕获组
                    // 解析str并生成ac_vec矢量数组
                    on_complete_replace(pnode, str, str_space);
                    free(str_space);
                    // 在编辑器里生成干净的代码片段
                    eu_sci_call(pnode, SCI_REPLACESEL, 0, (sptr_t)str);
                    if (pnode->ac_vec && (size = cvector_size(pnode->ac_vec)) > 0)
                    {   // 更新ac_vec里面(1..9..0)变量的位置信息, 即complete_t.min与complete_t.max
                        cvector_for_each_and_do(pnode->ac_vec, on_complete_update_vec, pos);
                        if (size > 1)
                        {   // 把ac_vec按1,2..9,0排序
                            qsort(pnode->ac_vec, size, sizeof(complete_t), on_complete_vec_cmp);
                            if (!pnode->ac_vec[0].index)
                            {
                                complete_t data = {0};
                                memcpy(&data, &pnode->ac_vec[0], sizeof(complete_t));
                                cvector_erase(pnode->ac_vec, 0);
                                cvector_push_back(pnode->ac_vec, data);
                            }
                        }
                    #ifdef APP_DEBUG
                        printf("============ac_vec===========\n");
                        on_complete_vec_printer(pnode->ac_vec);
                    #endif
                    }
                }
            }
        }
        if (pos >= 0 && ret && pnode->ac_vec && cvector_size(pnode->ac_vec) > 0)
        {
            it = &pnode->ac_vec[0];
            on_complete_zero_focus();
            on_complete_muti_autoc(pnode);
            ret = on_complete_snippet_jmp(pnode, it);
            if (ret && pnode->ac_vec)
            {
                pnode->ac_mode = AUTO_CODE;
            }
        }
        break;
    }
    eu_safe_free(str);
    return ret;
}

void
on_complete_set_word(eu_tabpage *pnode)
{
    if (pnode && pnode->doc_ptr)
    {
        char chars[MAX_PATH] = {0};
        eu_sci_call(pnode, SCI_GETWORDCHARS, 0, (sptr_t)chars);
        if (pnode->doc_ptr->doc_type == DOCTYPE_AU3)
        {
            if (strcmp(chars, CUSTOM_CHARS_AU3))
            {
                eu_sci_call(pnode, SCI_SETWORDCHARS, 0, (sptr_t)CUSTOM_CHARS_AU3);
            }
        }
        if (pnode->doc_ptr->doc_type == DOCTYPE_CSS)
        {
            if (strcmp(chars, CUSTOM_CHARS_CSS))
            {
                eu_sci_call(pnode, SCI_SETWORDCHARS, 0, (sptr_t)CUSTOM_CHARS_CSS);
            }
        }        
        else if (strcmp(chars, CUSTOM_CHARS))
        {
            eu_sci_call(pnode, SCI_SETWORDCHARS, 0, (sptr_t)CUSTOM_CHARS);
        }
    }
}

void
on_complete_unset_word(eu_tabpage *pnode)
{
    if (pnode && pnode->doc_ptr)
    {
        char chars[MAX_PATH] = {0};
        eu_sci_call(pnode, SCI_GETWORDCHARS, 0, (sptr_t)chars);
        if (strcmp(chars, DEFAULT_CHARS))
        {
            eu_sci_call(pnode, SCI_SETWORDCHARS, 0, (sptr_t)DEFAULT_CHARS);
        }
    }
}

void
on_complete_reset_focus(eu_tabpage *pnode)
{
    _InterlockedExchange(&last_snippet_focus, 0);
    if (pnode)
    {
        pnode->ac_mode = AUTO_NONE;
        cvector_freep(&pnode->ac_vec);
        if (!eu_sci_call(pnode, SCI_SETCARETPERIOD, 0, 0))
        {
            eu_sci_call(pnode, SCI_SETCARETPERIOD, eu_get_theme()->item.caret.bold, 0);
        }
    }
}
