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

#define SNIPPET_START "snippet"
#define SNIPPET_END   "endsnippet"

typedef enum _match_status
{
    match_none = 0,
    match_header,
    match_body,
    match_end
} match_status;

typedef int (*ptr_match)(const char *pstr, match_status *pstate, snippet_t *pdata, const char *eol_str);

typedef struct _state_machine
{
    match_status state;
    ptr_match fn_callback;
} state_machine;

/**************************************************************************************
 * 功能类似strok_r, 但它是严格的按照字符串分割
 * 并且, 如果分隔符处于s字符串开头, 或者连续出现
 * 返回的不是NULL, 而是0长度的字符串""
 **************************************************************************************/
static char *
on_parser_strtok(char *s, const char *delim, char **save_ptr)
{
    char *p;
    char *end = NULL;
    size_t len = strlen(delim);
    if (s == NULL)
    {
        s = *save_ptr;
    }
    if (*s == '\0')
    {
        *save_ptr = s;
        return NULL;
    }
    /* Find the end of the token. */
    p = strstr(s, delim);
    if (p)
    {
        end = p;
    }
    else
    {
        end = s + strlen(s);
    }
    if (*end == '\0')
    {
        *save_ptr = end;
        return s;
    }
    /* Terminate the token and make *SAVE_PTR point past it. */
    *end = '\0';
    *save_ptr = end + len;
    return s;
}

static int
on_parser_header(const char *pstr, match_status *pstate, snippet_t *pdata, const char *eol_str)
{
    (void)eol_str;
    char *cp1 = (char *)pstr;
    char *cp2 = NULL;
    char *eos = NULL;
    int m_sec = 3;
    char buf[QW_SIZE] = {'s', 'n', 'i', 'p', 'p', 'e', 't', 0};
    if (!(pstr && pstate && pdata && !strncasecmp(pstr, buf, strlen(buf))))
    {
        printf("pointer is null in %s\n", __FUNCTION__);
        return 1;
    }
    else
    {
        cp1 += strlen(buf);
    }
    for (; *cp1 && m_sec; --m_sec)
    {
        int sq = 0, dq = 0;         /* 单引号,  双引号 */
        memset(buf, 0, QW_SIZE);
        cp2 = buf;
        eos = cp2 + QW_SIZE - 1;
        util_skip_whitespace((uint8_t **)&cp1, QW_SIZE, 0);
        while ((*cp1 != '\0') && (cp2 != eos))
        {
            switch (*cp1)
            {
                case '\'':
                    if (dq)
                        break;     /* 遇到双引号, 跳出switch */
                    if (sq)
                        sq--;     /* 单引号匹配了, 清除状态 */
                    else if (!sq)
                        sq++;     /* 设置进入单引号状态 */
                    break;
                case '"':
                    if (sq)
                        break;     /* 遇到单引号, 跳出switch */
                    if (dq)
                        dq--;     /* 双引号匹配了, 清除状态 */
                    else if (!dq)
                        dq++;     /* 设置进入双引号状态 */
                    break;
                default:
                    break;
            }  /* 无引号空格: 寻找下一个区段 */
            if (isspace((int)*cp1) && !dq && !sq)
            {
                *cp2 = '\0';
                break;
            }
            else
            {
                *cp2++ = *cp1;
            }
            ++cp1;
        }
        if (strlen(buf) > 0)
        {
            switch (m_sec)
            {
                case 3:
                    _snprintf(pdata->name, QW_SIZE - 1, "%s", buf);
                    break;
                case 2:
                    _snprintf(pdata->comment, QW_SIZE - 1, "%s", buf);
                    break;
                case 1:
                    _snprintf(pdata->parameter, PARAM_LEN - 1, "%s", buf);
                    break;
                default:
                    break;
            }
        }
    }
    *pstate = match_body;
    return SKYLARK_OK;
}

static int
on_parser_body(const char *pstr, match_status *pstate, snippet_t *pdata, const char *eol_str)
{
    if (pstr && pstate && pdata && eol_str)
    {
        size_t len = strlen(pdata->body);
        if (len < LARGER_LEN)
        {
            if (strcmp(pstr, eol_str) == 0)
            {
                _snprintf(pdata->body + len, LARGER_LEN - len, "%s", pstr);
            }
            else
            {
                _snprintf(pdata->body + len, LARGER_LEN - len, "%s%s", pstr, eol_str);
            }
        }
    }
    return SKYLARK_OK;
}

static int
on_parser_end(const char *pstr, match_status *pstate, snippet_t *pdata, const char *eol_str)
{
    (void)pstr;
    (void)pdata;
    (void)eol_str;
    if (*pstate)
    {
        *pstate = match_none;
    }
    return SKYLARK_OK;
}

static int
on_parser_init_statemachine(const char *pstr, match_status *pstatus, snippet_t *pdata, const char *eol_str)
{
    if (STR_IS_NUL(pstr) || !pstatus || !pdata || !eol_str)
    {
        return 1;
    }
    state_machine machines[3] =
    {
        {match_header, on_parser_header},
        {match_body, on_parser_body},
        {match_end, on_parser_end}
    };
    for (int i = 0; i < 3; ++i)
    {
        if (machines[i].state == *pstatus)
        {
            machines[i].fn_callback(pstr, pstatus, pdata, eol_str);
            break;
        }
    }
    return SKYLARK_OK;
}

static int
on_parser_open_file(const TCHAR *filename, const TCHAR *mode, uint8_t **pbuf, FILE **ptr_file)
{
    int size = -1;
    while (filename && mode && pbuf && ptr_file)
    {
        if (!(*ptr_file = _tfopen(filename, mode)))
        {
            break;
        }
        if (fseek(*ptr_file, 0, SEEK_END) == -1)
        {
            break;
        }
        size = (int)ftell(*ptr_file);
        if (size == -1)
        {
            break;
        }
        if (fseek(*ptr_file, 0, SEEK_SET) == -1)
        {
            printf("%s, fseek failed\n", __FUNCTION__);
            break;
        }
        if (*pbuf || (*pbuf = (uint8_t *)calloc(1, size + 1)) == NULL)
        {
            printf("malloc failed\n");
            size = -1;
            break;
        }
        break;
    }
    if (size < 0)
    {
        if (ptr_file && *ptr_file)
        {
            fclose(*ptr_file);
            *ptr_file = NULL;
        }
    }
    return size;
}

static int
on_parser_filter_text(const TCHAR *filename, char **pbuf, intptr_t start, intptr_t end)
{
    uint8_t *buf = NULL;
    FILE *fp = NULL;
    int size = on_parser_open_file(filename, _T("r+b"), &buf, &fp);
    if (size > 0)
    {
        int off = 0;
        *pbuf = (char *)buf;
        while (off < size - 1)
        {
            off = ftell(fp);
            if (off >= start && off <= end + 1)
            {
                fseek(fp, off + 1, SEEK_SET);
            }
            else
            {
                *buf++ = getc(fp);
            }
        }
        if (size > 0 && strlen(*pbuf) >= 0)
        {
            int result = ftruncate(_fileno(fp), 0);
            if (result != 0)
            {
                printf( "problem in changing the size\n" );
            }
            rewind(fp);
            if (strlen(*pbuf) > 0)
            {
                size = eu_int_cast(fwrite(*pbuf, 1, strlen(*pbuf), fp));
            }
            else
            {
                size = 0;
            }
        }
    }
    if (fp)
    {
        fclose(fp);
    }
    return size;
}

static bool
on_parser_update_line(const char *pbuf, int size, snippet_t **ptr_vec)
{
    int eol = -1;
    int dimension = 0;
    char *p = NULL;
    const char *delim = NULL;
    char *pstream = (char *)pbuf;
    snippet_t *it = *ptr_vec;
    match_status status = match_none;
    if ((eol = on_encoding_line_eol(pbuf, size)) == -1)
    {
        return false;
    }
    else
    {
        delim = (eol == SC_EOL_LF) ? "\n" : ((eol == SC_EOL_CR) ? "\r" : "\r\n");
    }
    if (!(p = strtok(pstream, delim)))
    {
        return false;
    }
    while (p)
    {
        int len = 0;
        if (status < match_body)
        {
            it[dimension].start = p - pstream;
        }
        if (util_strnspace(p, SNIPPET_START, &len) == 0)
        {
            p += len;
            status = match_body;
        }
        else if (util_strnspace(p, SNIPPET_END, &len) == 0)
        {
            status = match_header;
            it[dimension].end = (p - pstream + len + strlen(SNIPPET_END));
            ++dimension;
        }
        p = strtok(NULL, delim);
    }
    return true;
}

static int
on_parser_modify_text(FILE *fp, char **pbuf, const char *txt, intptr_t start, intptr_t end)
{
    int size = 0;
    char *header = NULL;
    char *inserter = NULL;
    if (!(fp && pbuf && *pbuf && txt))
    {
        return size;
    }
    int len = eu_int_cast(strlen(*pbuf));
    if (start > 0 && (header = (char *)calloc(1, start + 1)))
    {
        memmove(header, (*pbuf), start);
    }
    if (end < len && (inserter = (char *)calloc(1, len + 1)))
    {
        memmove(inserter, (*pbuf + end), len - end);
    }
    if (header)
    {
        size = eu_int_cast(fwrite(header, 1, strlen(header), fp));
    }
    if (txt)
    {
        size += eu_int_cast(fwrite(txt, 1, strlen(txt), fp));
    }
    if (inserter)
    {
        size += eu_int_cast(fwrite(inserter, 1, strlen(inserter), fp));
    }
    fflush(fp);
    rewind(fp);
    if (size > 0 && size > len)
    {
        free(*pbuf);
        *pbuf = (char *)calloc(1, size + 1);
        if (*pbuf == NULL)
        {
            size = 0;
        }
    }
    if (*pbuf)
    {
        size = size > 0 ? (int)fread(*pbuf, 1, size, fp) : 0;
        (*pbuf)[size] = 0;
    }
    eu_safe_free(header);
    eu_safe_free(inserter);
    return size;
}

static int
on_parser_frequency(const char *str, const char *substr)
{
    int n = 0, s1 = 0, s2 = 0;
    char *p = (char *)str;
    s1 = eu_int_cast(strlen(str));
    s2 = eu_int_cast(strlen(substr));
    while (s1 >= s2)
    {   //长串如果比子串短，不用再找了
        str = strstr(str, substr);
        if (str != 0)
        {   // 找到了, 后移一个子串的长度
            n++;
            str += s2;
        }
        else
        {
            break;
        }
        s1 = eu_int_cast(strlen(str));
    }
    return n;
}

#if defined(_DEBUG)
static void
on_parser_vec_printer(snippet_t *pv)
{
    if (pv)
    {
        snippet_t *it;
        int i = 0;
        for (it = cvector_begin(pv); it != cvector_end(pv); ++it, ++i)
        {
            printf("pv[%d] = %zd, %zd, %s, %s, %s, %s\n", i, it->start, it->end, it->name, it->comment, it->parameter, it->body);
        }
    }
}
#endif

bool
on_parser_vector_new(const TCHAR *path, snippet_t **ptr_vec, int dimension, int eol)
{
    bool ret = false;
    char *buf = NULL;
    char *txt = NULL;
    FILE *fp = NULL;
    int size = on_parser_open_file(path, _T("r+b"), &buf, &fp);
    size = size > 0 ? (int)fread((char *) buf, 1, size, fp) : 0;
#if defined(_DEBUG)
    on_parser_vec_printer(*ptr_vec);
#endif
    while (size >= 0 && buf && fp)
    {
        int n = 255;
        intptr_t start;
        intptr_t end;
        const char *peol = NULL;
        const int txt_len = LARGER_LEN + MAX_PATH;
        if (!(txt = (char *)calloc(1, txt_len + 1)))
        {
            break;
        }
        peol = (eol == SC_EOL_LF) ? "\n" : ((eol == SC_EOL_CR) ? "\r" : "\r\n");
        if (strlen(buf) > 3 * strlen(peol))
        {
            n = on_parser_frequency(&buf[strlen(buf) - 1] - 3 * strlen(peol), peol);
            printf("n = %d\n", n);
        }
        if (n == 0 || n == 1)
        {
            if (!n)
            {
                n = _snprintf(txt, txt_len, "%s%s", peol, peol);
            }
            else
            {
                n = _snprintf(txt, txt_len, "%s", peol);
            }
        }
        else
        {
            n = 0;
        }
        _snprintf(txt + strlen(txt), txt_len - strlen(txt), SNIPPET_START" %s", (*ptr_vec)[dimension].name);
        if ((*ptr_vec)[dimension].comment[0])
        {
            strncat(txt, " ", txt_len);
            strncat(txt, (*ptr_vec)[dimension].comment, txt_len);
            if ((*ptr_vec)[dimension].parameter[0])
            {
                strncat(txt, " ", txt_len);
                strncat(txt, (*ptr_vec)[dimension].parameter, txt_len);
            }
        }
        strncat(txt, peol, txt_len);
        strncat(txt, (*ptr_vec)[dimension].body, txt_len);
        int len = eu_int_cast(strlen((*ptr_vec)[dimension].body));
        if (!((*ptr_vec)[dimension].body[len - 1] == '\r' || (*ptr_vec)[dimension].body[len - 1] == '\n'))
        {
            strncat(txt, peol, txt_len);
        }
        strncat(txt, SNIPPET_END, txt_len);
        strncat(txt, peol, txt_len);
        ftruncate(_fileno(fp), 0);
        rewind(fp);
        start = size > 0 ? size : 0;
        end = size > 0 ? size + strlen(txt) : 0;
        size = on_parser_modify_text(fp, &buf, txt, start, end);
        fclose(fp);
        if (size > 0)
        {
            ret = on_parser_update_line(buf, size, ptr_vec);
        }
        break;
    }
    eu_safe_free(buf);
    eu_safe_free(txt);
    return ret;
}

bool
on_parser_vector_erase(const TCHAR *path, snippet_t **ptr_vec, int dimension)
{
    bool ret = false;
    char *buf = NULL;
    if (path && ptr_vec)
    {
        intptr_t start = (*ptr_vec)[dimension].start;
        intptr_t end = (*ptr_vec)[dimension].end;
        cvector_erase(*ptr_vec, (size_t)dimension);
        int size = on_parser_filter_text(path, &buf, start, end);
        if (size > 0)
        {
            ret = on_parser_update_line(buf, size, ptr_vec);
        }
        eu_safe_free(buf);
    }
    return ret;
}

bool
on_parser_vector_modify(const TCHAR *path, snippet_t **ptr_vec, int dimension)
{
    bool ret = false;
    char *buf = NULL;
    char *txt = NULL;
    FILE *fp = NULL;
    int size = on_parser_open_file(path, _T("r+b"), &buf, &fp);
    size = size > 0 ? (int)fread((char *) buf, 1, size, fp) : 0;
    while (size > 0 && fp)
    {
        int eols = -1;
        const char *peol = NULL;
        const int txt_len = LARGER_LEN + MAX_PATH;
        intptr_t start = (*ptr_vec)[dimension].start;
        intptr_t end = (*ptr_vec)[dimension].end;
        if ((eols = on_encoding_line_eol((const char *)buf, size)) == -1)
        {
            break;
        }
        if (!(txt = (char *)calloc(1, txt_len + 1)))
        {
            break;
        }
        peol = (eols == SC_EOL_LF) ? "\n" : ((eols == SC_EOL_CR) ? "\r" : "\r\n");
        _snprintf(txt, txt_len, SNIPPET_START" %s", (*ptr_vec)[dimension].name);
        if ((*ptr_vec)[dimension].comment[0])
        {
            strncat(txt, " ", txt_len);
            strncat(txt, (*ptr_vec)[dimension].comment, txt_len);
            if ((*ptr_vec)[dimension].parameter[0])
            {
                strncat(txt, " ", txt_len);
                strncat(txt, (*ptr_vec)[dimension].parameter, txt_len);
            }
        }
        strncat(txt, peol, txt_len);
        strncat(txt, (*ptr_vec)[dimension].body, txt_len);
        int len = eu_int_cast(strlen((*ptr_vec)[dimension].body));
        if (!((*ptr_vec)[dimension].body[len - 1] == '\r' || (*ptr_vec)[dimension].body[len - 1] == '\n'))
        {
            strncat(txt, peol, txt_len);
        }
        strncat(txt, SNIPPET_END, txt_len);
        ftruncate(_fileno(fp), 0);
        rewind(fp);
        size = on_parser_modify_text(fp, &buf, txt, start, end);
        fclose(fp);
        if (size > 0)
        {
            ret = on_parser_update_line(buf, size, ptr_vec);
        }
        break;
    }
    eu_safe_free(buf);
    eu_safe_free(txt);
    return ret;
}

bool
on_parser_init(const TCHAR *path, snippet_t **ptr_vec, int *peol)
{
    bool ret = true;
    int size = 0;
    FILE *fp = NULL;
    uint8_t *buf = NULL;
    if (!(path && ptr_vec))
    {
        return false;
    }
    size = on_parser_open_file(path, _T("rb"), &buf, &fp);
    while (size > 0 && (size = (int)fread((char *) buf, 1, size, fp)) > 0)
    {
        char *p = NULL;
        char *save = NULL;
        char *pstream = NULL;
        snippet_t sdata = {0};
        match_status status = match_none;
        if (size >= BUFF_200M)
        {
            ret = false;
            break;
        }
        if ((*peol = on_encoding_line_eol((const char *)buf, size)) == -1)
        {
            ret = false;
            break;
        }
        const char *delim = (*peol == SC_EOL_LF) ? "\n" : ((*peol == SC_EOL_CR) ? "\r" : "\r\n");
        pstream = (char *)buf;
        p = on_parser_strtok(pstream, delim, &save);
        while (p)
        {
            int len = 0;
            if (status < match_body)
            {
                sdata.start = p - pstream;
            }
            if (*p != 0)
            {
                if (util_strnspace(p, SNIPPET_START, &len) == 0)
                {
                    p += len;
                    status = match_header;
                }
                else if (util_strnspace(p, SNIPPET_END, &len) == 0)
                {
                    status = match_end;
                    sdata.end = (p - pstream + len + strlen(SNIPPET_END));
                    cvector_push_back(*ptr_vec, sdata);
                    memset(&sdata, 0, sizeof(snippet_t));
                }
            }
            if (SKYLARK_OK != on_parser_init_statemachine(*p ? p : delim, &status, &sdata, delim))
            {
                ret = false;
                break;
            }
            if (!(p = on_parser_strtok(NULL, delim, &save)))
            {
                break;
            }
        }
        break;
    }
    if (fp)
    {
        fclose(fp);
    }
    eu_safe_free(buf);
    return ret;
}
