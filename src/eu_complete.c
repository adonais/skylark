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

#include "framework.h"

#ifndef ANY_WORD
#define ANY_WORD "///*///"
#endif

#define DEFAULT_CHARS "_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
#define CUSTOM_CHARS  "_!#$&/abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
#define CUSTOM_CHARS_AU3 "_!#$&@abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"

typedef void (*ptr_replace)(void *p1, void **p2, const char *by, int start, int offset);

static volatile long last_snippet_focus = 0;
static char *auto_xpm[] = {
    /* columns rows colors chars-per-pixel */
    "14 14 3 1 ",
    "  c None",
    ". c #407F40",
    "X c #408040",
    /* pixels */
    "              ",
    "              ",
    "     XXXXX    ",
    "    XXXXXX    ",
    "   XXXX  X    ",
    "   XXXX       ",
    "    .XXXX     ",
    "     XXXXX    ",
    "       XXXX   ",
    "   XX   XXX   ",
    "   XXXXXXX    ",
    "    XXXX.X    ",
    "              ",
    "              "
};

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

static int
on_complete_str_cmp(snippet_t *pvec, const char *value)
{
    if (pvec && pvec->name[0])
    {
        return (strcmp(pvec->name, value));
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
on_complete_postion_cmp(complete_t *pvec, intptr_t pos)
{
    if (pvec)
    {
        for (int j = 0; j < OVEC_LEN && pvec->pos[j].min > 0; ++j)
        {
            if (pos >= pvec->pos[j].min && pos <= pvec->pos[j].max)
            {
            #ifdef _DEBUG
                printf("pos = %Id, pvec->pos[%d].min = %Id, pvec->pos[%d].max = %Id\n", pos, j, pvec->pos[j].min, j, pvec->pos[j].max);
            #endif
                return 0;
            }
        }
    }
    return 1;
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
        complete_t *it;
        const char *psub = by;
        while ((p = strchr(psub, '$')) && strlen(p) > 1 && UTIL_BASE10(p[1]))
        {
            complete_t *oit = NULL;
            add += eu_int_cast(p - psub);
            cvector_for_each_and_cmp(pvec, on_complete_char_cmp, p[1], &oit);
            if (oit)
            {
                psub = oit->value;
            }
        }
        pos->min = start;
        pos->max = pos->min + add + (intptr_t)strlen(psub);
        for (it = cvector_begin(pvec); it != cvector_end(pvec); ++it)
        {
            if (it->pos[0].min > start)
            {
                it->pos[0].min -= offset;
                it->pos[0].max -= offset;
            }
        }
        (*(auto_postion **)p2)++;
    }
}

static void
on_complete_replace_holder(void *p1, void **p2, const char *by, int start, int offset)
{
    complete_t *pvec = (complete_t *)p1;
    complete_t  *oit = (complete_t **)p2 ? *(complete_t **)p2 : NULL;
    if (pvec && oit)
    {
        for (complete_t *it = cvector_begin(pvec); it != cvector_end(pvec); ++it)
        {
            for (int j = 0; j < OVEC_LEN; ++j)
            {
                if (oit == it)
                {
                    if (it->pos[j].min < 0)
                    {
                        it->pos[j].min = start;
                        it->pos[j].max = it->pos[j].min + (intptr_t)strlen(by);
                        break;
                    }
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
        fn_replace(p1, &p2, by, eu_int_cast(needle - in + strlen(res)), eu_int_cast(strlen(pattern) - strlen(by)));
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

static bool
on_complete_get_str(snippet_t *pv, const char *key, char **pstr)
{
    bool ret = false;
    *pstr = NULL;
    if (pv)
    {
        snippet_t *it;
        for (it = cvector_begin(pv); it != cvector_end(pv); ++it)
        {
            if ((_stricmp(it->name, key) == 0) && (*pstr = (char *)calloc(1, VALUE_LEN)))
            {
                _snprintf(*pstr, VALUE_LEN - 1, "%s", it->body);
                ret = true;
                break;
            }
        }
    }
    return ret;
}

static int
on_complete_get_key(eu_tabpage *pnode, char *key, int len, sptr_t *ptr_pos)
{
    int m_indent = -1;
    if (pnode)
    {
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
        eu_sci_call(pnode, SCI_SETSELECTION, word_start, word_end);
        on_complete_unset_word(pnode);
        if (ptr_pos)
        {
            *ptr_pos = word_start;
        }
    }
    return m_indent;
}

static int
on_complete_re_callback(eu_tabpage *pnode, const char *pstr, const char *re_str, int start, int end)
{
    complete_t data;
    on_complete_vec_init(&data);
    if (pnode && pstr && re_str)
    {
        char buf[MAX_PATH] = {0};
        int length = end - start;
        if (length > 0 && length < MAX_PATH)
        {
            snprintf(buf, MAX_PATH - 1, "%.*s", length, re_str);
        }
        if (strlen(buf) > 0)
        {
            char *p = NULL;
            if ((p = strchr(buf, ':')) && strlen(p) > 1)
            {
                ++p;
                util_skip_whitespace(&p, eu_int_cast(strlen(p)), 0);
                _snprintf(data.value, ACNAME_LEN, "%s", p);
                data.index = buf[0] - 0x30;
                data.pos[0].min = eu_int_cast(re_str - pstr);
                data.pos[0].max = data.pos[0].min + length + 1;
                data.pos[0].min -= 2;
                int sub_length = eu_int_cast(data.pos[0].max - data.pos[0].min);
                snprintf(data.word, MAX_ACCELS-1, "%.*s", sub_length, &pstr[data.pos[0].min]);
                cvector_push_back(pnode->ac_vec, data);
            }
        }
    }
    return 0;
}

static void
on_complete_pcre_match(eu_tabpage *pnode, const char *pstr, int buf_size)
{
    pcre *re;
    int erroffset;
    int ovector[OVECCOUNT], rc, src_len;
    char *src, *src1;
    const char *err;
    const char *pattern = "\\$\\{(\\d:.+)\\}";
    re = pcre_compile(pattern, PCRE_NO_UTF8_CHECK|PCRE_MULTILINE|PCRE_DOTALL|PCRE_UNGREEDY, &err, &erroffset, NULL);
    if (NULL == re) {
        printf("pcre compile error\n");
        return;
    }
    src = src1 = (char *)pstr;
    src_len = eu_int_cast(strlen(src));
    while (1)
    {
        int ret = 0;
        rc = pcre_exec(re, NULL, src, src_len, 0, 0, ovector, OVECCOUNT);
        if (rc < 0)
        {
            printf("no more matches\n");
            break;
        }
        if (ovector[2] > 0) {
            src1 += ovector[2];
            src_len = eu_int_cast(src_len - (src1 - src));
            src = src1;
            if (src_len <= 0)
            {
                printf("src_len = %d\n", src_len);
                break;
            }
            on_complete_re_callback(pnode, pstr, src, ovector[2], ovector[3]);
        }
    }
    pcre_free(re);
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
            printf("pv[%d] = %d, %s, %s,", i, it->index, it->value, it->word);
            for (int j = 0; j < OVEC_LEN; ++j)
            {
                if (it->pos[j].min >= 0)
                {
                    printf("it->pos[%d].min = %Id, it->pos[%d].max = %Id\n", j, it->pos[j].min, j, it->pos[j].max);
                }
            }
        }
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
        len = eu_int_cast(strlen(pstr));
        if (pnode->ac_vec)
        {
            cvector_clear(pnode->ac_vec);
        }
        // 使用正则搜索占位符
        on_complete_pcre_match(pnode, pstr, VALUE_LEN);
        if (true)
        {
            int offset = 0;
            char *p = NULL;
            char *pvalue = NULL;
            char tmp[3] = {0};
            complete_t *it;
            complete_t *oit = NULL;
            // 替换正则表达式查找到的$(d:xxx)
            for (it = cvector_begin(pnode->ac_vec); it != cvector_end(pnode->ac_vec); ++it)
            {
                if (it->index >= 0 && it->word[0])
                {
                    int add = 0;
                    pvalue = it->value;
                    while ((p = strchr(pvalue, '$')) && UTIL_BASE10(p[1]))
                    {
                        add += eu_int_cast(p - pvalue);
                        oit = NULL;
                        cvector_for_each_and_cmp(pnode->ac_vec, on_complete_char_cmp, p[1], &oit);
                        if (oit)
                        {
                            pvalue = oit->value;
                        }
                    }
                    it->pos[0].min -= offset;
                    it->pos[0].max = it->pos[0].min + add + (intptr_t)strlen(pvalue);
                    eu_str_replace(pstr, VALUE_LEN, it->word, it->value);
                    offset += eu_int_cast(strlen(it->word) - strlen(it->value));
                }
            }
            // 替换占位符, 并将位置信息写入auto_postion数组
            for (it = cvector_begin(pnode->ac_vec); it != cvector_end(pnode->ac_vec); ++it)
            {
                if (it->index >= 0)
                {
                    _snprintf(tmp, 3, "$%d", it->index);
                    on_complete_replace_fn(pstr, VALUE_LEN, tmp, it->value, on_complete_replace_callback, pnode->ac_vec, &it->pos[1]);
                }
            }
            // 替换value里面可能包含的占位符, 为下一步做好准备
            for (it = cvector_begin(pnode->ac_vec); it != cvector_end(pnode->ac_vec); ++it)
            {
                pvalue = it->value;
                while ((p = strchr(pvalue, '$')) && UTIL_BASE10(p[1]))
                {
                    _snprintf(tmp, 3, "$%c", p[1]);
                    oit = NULL;
                    cvector_for_each_and_cmp(pnode->ac_vec, on_complete_char_cmp, p[1], &oit);
                    if (oit)
                    {
                        pvalue = oit->value;
                        eu_str_replace(it->value, ACNAME_LEN, tmp, pvalue);
                    }
                }
            }
            // 替换所有占位符, 并将位置信息写入auto_postion数组
            for (it = cvector_begin(pnode->ac_vec); it != cvector_end(pnode->ac_vec); ++it)
            {
                if (it->index >= 0)
                {
                    auto_postion *pos = it->pos;
                    _snprintf(tmp, 3, "$%d", it->index);
                    for (int j = 0; j < OVEC_LEN; ++j)
                    {
                        if (it->pos[j].min > 0)
                        {
                            ++pos;
                        }
                    }
                    on_complete_replace_fn(pstr, VALUE_LEN, tmp, it->value, on_complete_replace_callback, pnode->ac_vec, pos);
                }
            }
            // 可能存在没匹配正则的占位符$0..$9, 替换并将位置信息写入vec数组
            while ((p = strstr(pstr, "$")) && UTIL_BASE10(p[1]))
            {
                complete_t data;
                on_complete_vec_init(&data);
                _snprintf(tmp, 3, "$%c", p[1]);
                data.index = UTIL_NUMBER(p[1]);
                cvector_push_back(pnode->ac_vec, data);
                on_complete_replace_fn(pstr, VALUE_LEN, tmp, "", on_complete_replace_holder, pnode->ac_vec, &pnode->ac_vec[cvector_size(pnode->ac_vec) - 1]);
            }
        }
    }
}

static unsigned __stdcall
on_complete_snippet_thread(void *lp)
{
    Sleep(20);
    return on_complete_snippet(on_tabpage_focus_at());
}

static void
on_complete_update_vec(complete_t *it, intptr_t pos)
{
    if (it && pos)
    {
        for (int j = 0; j < OVEC_LEN; ++j)
        {
            if (it->pos[j].min >= 0)
            {
                it->pos[j].min += pos;
                it->pos[j].max += pos;
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
on_complete_struct_cmp(const void* _a, const void* _b)
{
    return (*(complete_t *)_a).index > (*(complete_t *)_b).index ? 1 : -1;
}

static void
on_complete_sort_update(eu_tabpage *pnode, complete_t *it, int **pv, int offset, int index)
{
    if (pnode && it && (*pv) && cvector_size(*pv) > 0)
    {
        char *p = NULL;
        char *pword = NULL;
        int first_pos = (*pv)[0];
        for (int j = 0; j < OVEC_LEN; ++j)
        {
            if (it->pos[j].min > first_pos)
            {
                bool msub = false;
                cvector_push_back((*pv), eu_int_cast(it->pos[j].min));
                qsort((*pv), cvector_size((*pv)), sizeof(int), on_complete_sort_callback);
                int m = eu_cvector_at((*pv), eu_int_cast(it->pos[j].min));
                if (m > 0)
                {
                    char *pword = it->word;
                    complete_t *oit = NULL;
                    it->pos[j].min += (m * offset);
                    it->pos[j].max += (m * offset);
                    if ((p = strchr(pword, ':')))
                    {
                        pword = p;
                    }
                    while (pword && (p = strchr(pword, '$')) && strlen(p) > 1 && UTIL_BASE10(p[1]))
                    {
                        if (UTIL_NUMBER(p[1]) == index)
                        {
                            msub = true;
                        }
                        cvector_for_each_and_cmp(pnode->ac_vec, on_complete_char_cmp, p[1], &oit);
                        if (oit)
                        {
                            pword = strchr(oit->word, ':');
                        }
                    }
                    if (msub && oit && oit != it)
                    {
                        _snprintf(it->value, ACNAME_LEN, "%s", oit->value);
                        it->pos[j].max += offset;
                    }
                }
                cvector_erase((*pv), m);
            }
        }
    }
}

static bool
on_complete_update_postion(eu_tabpage *pnode, complete_t **ptr_vec, bool back)
{
    int offset = 0;
    bool nsel = false;
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
        nsel = true;
    }
    if (current_pos >= 0)
    {
        if (nsel)
        {   // 根据选中焦点, 获取要跳转的vec index
            cvector_for_each_and_cmp(pnode->ac_vec, on_complete_postion_cmp, current_pos, &oit);
        }
        else
        {   // 根据上一个焦点, 获取要跳转的vec index
            if (last_snippet_focus >= 0 && last_snippet_focus < cvector_size(pnode->ac_vec))
            {
                oit = &pnode->ac_vec[last_snippet_focus];
            }
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
        if (current_pos > oit->pos[0].min && current_pos - oit->pos[0].min < ACNAME_LEN)
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
    if (offset && oit && v)
    {   // 找出位置在它们之前或之后的vec, 更新位置信息
        for (complete_t *it = cvector_begin(pnode->ac_vec); it != cvector_end(pnode->ac_vec); ++it)
        {
            if (it != oit)
            {
                on_complete_sort_update(pnode, it, &v, offset, oit->index);
            }
        }
    }
    cvector_freep(&v);
    return true;
}

static bool
on_complete_snippet_jmp(eu_tabpage *pnode, complete_t *it)
{
    if (it && it->index >= 0)
    {
        eu_sci_call(pnode, SCI_SETEMPTYSELECTION, it->pos[0].min, 0);
        eu_sci_call(pnode, SCI_SETSELECTION, it->pos[0].min, it->pos[0].max);
        for (int j = 1; j < OVEC_LEN && it->pos[j].min > 0 && it->pos[j].max - it->pos[j].min < ACNAME_LEN; ++j)
        {
            char value[ACNAME_LEN] = {0};
            Sci_TextRange tr = {{it->pos[j].min, it->pos[j].max}, value};
            eu_sci_call(pnode, SCI_GETTEXTRANGE, 0, (sptr_t) &tr);
            if (!strcmp(value, it->value))
            {
                eu_sci_call(pnode, SCI_ADDSELECTION, it->pos[j].min, it->pos[j].max);
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

static void
on_complete_call_autocshow(eu_tabpage *pnode, const char *word_buffer, const sptr_t pos)
{
    char *key = NULL;
    const char *snippet_str = NULL;
    int flags = SC_AUTOCOMPLETE_FIXED_SIZE;
    if (eu_get_config()->m_snippet_enable)
    {
        snippet_str = on_complete_get_func(pnode, word_buffer);
    }
    if ((key = eu_find_completed_tree(&pnode->doc_ptr->acshow_tree, word_buffer, snippet_str)) && key[0])
    {
        if (snippet_str)
        {
            flags |= SC_AUTOCOMPLETE_SNIPPET;
            eu_sci_call(pnode, SCI_REGISTERIMAGE, SNIPPET_FUNID, (sptr_t)auto_xpm);
        }
        eu_sci_call(pnode, SCI_AUTOCSETOPTIONS, flags, 0);
        eu_sci_call(pnode, SCI_AUTOCSHOW, pos, (sptr_t)key);
    }
    eu_safe_free(key);
}

static void
on_complete_any_autocshow(eu_tabpage *pnode)
{
    if (pnode && pnode->doc_ptr)
    {
        char *key = eu_find_completed_tree(&pnode->doc_ptr->acshow_tree, ANY_WORD, NULL);
        if ((key = eu_find_completed_tree(&pnode->doc_ptr->acshow_tree, ANY_WORD, NULL)) && key[0])
        {
            eu_sci_call(pnode, SCI_AUTOCSETOPTIONS, SC_AUTOCOMPLETE_FIXED_SIZE, 0);
            eu_sci_call(pnode, SCI_AUTOCSHOW, 0, (sptr_t) key);
        }
        eu_safe_free(key);
    }
}

void
on_complete_doc(eu_tabpage *pnode, ptr_notify lpnotify)
{   /* 自动补全提示 */
    if (eu_get_config()->m_acshow && pnode && pnode->doc_ptr && !RB_EMPTY_ROOT(&(pnode->doc_ptr->acshow_tree)))
    {
        on_complete_set_word(pnode);
        sptr_t current_pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
        sptr_t start_pos = eu_sci_call(pnode, SCI_WORDSTARTPOSITION, current_pos - 1, true);
        sptr_t end_pos = eu_sci_call(pnode, SCI_WORDENDPOSITION, current_pos - 1, true);
        if (end_pos - start_pos >= ACNAME_LEN)
        {
            end_pos = start_pos + ACNAME_LEN;
        }
        if (end_pos - start_pos > eu_get_config()->acshow_chars)
        {
            char word_buffer[ACNAME_LEN+1] = {0};
            Sci_TextRange tr = {{start_pos, end_pos}, word_buffer};
            eu_sci_call(pnode, SCI_GETTEXTRANGE, 0, (sptr_t) &tr);
            if (word_buffer[0])
            {
                on_complete_call_autocshow(pnode, word_buffer, current_pos - start_pos);
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
    if (pnode && pnode->doc_ptr && lpnotify && lpnotify->ch > 0 && eu_get_config()->m_acshow)
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
            if (end_pos - start_pos >= ACNAME_LEN)
            {
                end_pos = start_pos + ACNAME_LEN;
            }
            if (start_pos > 0 && end_pos - start_pos >= eu_get_config()->acshow_chars)
            {
                bool is_attr = false;
                char word_buffer[ACNAME_LEN+1];
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
                        on_complete_call_autocshow(pnode, word_buffer, current_pos - start_pos);
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
    complete_t *it;
    if (!pnode)
    {
        return false;
    }
    if (pnode->ac_vec && (size = cvector_size(pnode->ac_vec)) > 0 && pnode->ac_mode != AUTO_NONE)
    {
        if (!on_complete_update_postion(pnode, &it, false))
        {   // 光标所在编辑过, 更新位置信息
            on_complete_reset_focus(pnode);
            return false;
        }
        if (it && it - pnode->ac_vec < (intptr_t)size)
        {   // 解析vec数组, 设置tab跳转
            it++;
            ret = on_complete_snippet_jmp(pnode, it);
            if (ret && pnode->ac_vec)
            {
                on_complete_inc_focus();
            }
        }
        return ret;
    }
    while (pnode->doc_ptr && pnode->doc_ptr->snippet && pnode->ac_mode != AUTO_CODE)
    {
        intptr_t pos = -1;
        char key[ACNAME_LEN] = {0};
        if (pnode->doc_ptr->ptrv && cvector_size(pnode->doc_ptr->ptrv) > 0)
        {   // 获取解析过的snippets配置文件, 替换占位符并生成ac_vec
            int count = on_complete_get_key(pnode, key, ACNAME_LEN, &pos);
            if (count >= 0 && key[0])
            {
                ret = on_complete_get_str(pnode->doc_ptr->ptrv, key, &str);
                if (ret)
                {
                    char *str_space = (char *)calloc(1, count+1);
                    if (!str_space)
                    {
                        break;
                    }
                    memset(str_space, 0x20, count);
                    on_complete_replace(pnode, str, str_space);
                    free(str_space);
                    eu_sci_call(pnode, SCI_REPLACESEL, 0, (sptr_t)str);
                    if (pnode->ac_vec && (size = cvector_size(pnode->ac_vec)) > 0)
                    {
                        cvector_for_each_and_do(pnode->ac_vec, on_complete_update_vec, pos);
                        if (size > 1)
                        {   // 把结构数组按1,2..9,0排序
                            qsort(pnode->ac_vec, size, sizeof(complete_t), on_complete_struct_cmp);
                            if (!pnode->ac_vec[0].index)
                            {
                                complete_t data = {0};
                                memcpy(&data, &pnode->ac_vec[0], sizeof(complete_t));
                                cvector_erase(pnode->ac_vec, 0);
                                cvector_push_back(pnode->ac_vec, data);
                            }
                        }
                    #ifdef _DEBUG
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
on_complete_delay_snippet(void)
{
    CloseHandle((HANDLE) _beginthreadex(NULL, 0, on_complete_snippet_thread, NULL, 0, NULL));
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
            eu_sci_call(pnode, SCI_SETCARETPERIOD, eu_get_config()->eu_caret.blink, 0);
        }
    }
}

const char *
on_complete_get_func(eu_tabpage *pnode, const char *key)
{
    if (pnode && pnode->doc_ptr && pnode->doc_ptr->ptrv)
    {
        snippet_t *it = NULL;
        cvector_for_each_and_cmp(pnode->doc_ptr->ptrv, on_complete_str_cmp, key, &it);
        if (it)
        {
            return it->name;
        }
    }
    return NULL;
}
