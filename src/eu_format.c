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

#include "framework.h"

#define FORMAT_MAX_LEN (4 * 1024 * 1024)
#define FORMAT_DLL _T("clang-format.dll")

typedef bool (*ptr_format)(const char *filename, const char *data, size_t size, char **pout);

static int the_a;
static int the_b;
static int look_ahead = -1;
static int the_x = -1;
static int the_y = -1;

/*  js_alphanum -- return true if the character is a letter, digit, underscore,
    dollar sign, or non-ASCII character.
*/
static inline int
js_alphanum(int c)
{
    return ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || c == '_' || c == '$' || c == '\\' || c > 0x7E);
}

/*  js_get -- return the js_next character from stdin. Watch out for lookahead. If
    the character is a control character, translate it to a space or
    linefeed.
*/
static int
js_get(const uint8_t *ptext, size_t *in)
{
    int c = look_ahead;
    look_ahead = -1;
    if (c == -1)
    {
        c = *in <= strlen((const char *) ptext) ? ptext[(*in)++] : -1;
    }
    if (c >= ' ' || c == '\n' || c == -1)
    {
        return c;
    }
    if (c == '\r')
    {
        return '\n';
    }
    return ' ';
}

static inline void
js_put(int ch, uint8_t *pbuf, size_t *out)
{
    pbuf[(*out)++] = ch;
}

/* js_peek -- js_get the js_next character without getting it.
 */
static inline int
js_peek(const uint8_t *ptext, size_t *in)
{
    look_ahead = js_get(ptext, in);
    return look_ahead;
}

/*  js_next -- js_get the js_next character, excluding comments. js_peek() is used to see
    if a '/' is followed by a '/' or '*'.
*/
static int
js_next(const uint8_t *ptext, size_t *in)
{
    int c = js_get(ptext, in);
    if (c == '/')
    {
        switch (js_peek(ptext, in))
        {
            case '/':
                for (;;)
                {
                    c = js_get(ptext, in);
                    if (c <= '\n')
                    {
                        break;
                    }
                }
                break;
            case '*':
                js_get(ptext, in);
                while (c != ' ')
                {
                    switch (js_get(ptext, in))
                    {
                        case '*':
                            if (js_peek(ptext, in) == '/')
                            {
                                js_get(ptext, in);
                                c = ' ';
                            }
                            break;
                        case -1:
                            eu_logmsg("Error: JSMIN Unterminated comment.\n");
                            return EUE_UNKOWN_ERR;
                    }
                }
                break;
        }
    }
    the_y = the_x;
    the_x = c;
    return c;
}

/*  js_action -- do something! What you do is determined by the argument:
    1   Output A. Copy B to A. Get the js_next B.
    2   Copy B to A. Get the js_next B. (Delete A).
    3   Get the js_next B. (Delete B).
    js_action treats a string as a single character. Wow!
    js_action recognizes a regular expression if it is preceded by ( or , or =.
*/
static int
js_action(const uint8_t *ptext, size_t *in, uint8_t *pbuf, size_t *out, int d)
{
    switch (d)
    {
        case 1:
            js_put(the_a, pbuf, out);
            if ((the_y == '\n' || the_y == ' ') && (the_a == '+' || the_a == '-' || the_a == '*' || the_a == '/') &&
                (the_b == '+' || the_b == '-' || the_b == '*' || the_b == '/'))
            {
                js_put(the_y, pbuf, out);
            }
            FALLTHRU_ATTR;
        case 2:
            the_a = the_b;
            if (the_a == '\'' || the_a == '"' || the_a == '`')
            {
                for (;;)
                {
                    js_put(the_a, pbuf, out);
                    the_a = js_get(ptext, in);
                    if (the_a == the_b)
                    {
                        break;
                    }
                    if (the_a == '\\')
                    {
                        js_put(the_a, pbuf, out);
                        the_a = js_get(ptext, in);
                    }
                    if (the_a == -1)
                    {
                        eu_logmsg("Error: JSMIN unterminated string literal\n");
                        return EUE_UNKOWN_ERR;
                    }
                }
            }
            FALLTHRU_ATTR;
        case 3:
            the_b = js_next(ptext, in);
            if (the_b == EUE_UNKOWN_ERR)
            {
                return EUE_UNKOWN_ERR;
            }
            else if (the_b == '/' && (the_a == '(' || the_a == ',' || the_a == '=' || the_a == ':' || the_a == '[' || the_a == '!' ||
                     the_a == '&' || the_a == '|' || the_a == '?' || the_a == '+' || the_a == '-' || the_a == '~' ||
                     the_a == '*' || the_a == '/' || the_a == '{' || the_a == '\n'))
            {
                js_put(the_a, pbuf, out);
                if (the_a == '/' || the_a == '*')
                {
                    js_put(' ', pbuf, out);
                }
                js_put(the_b, pbuf, out);
                for (;;)
                {
                    the_a = js_get(ptext, in);
                    if (the_a == '[')
                    {
                        for (;;)
                        {
                            js_put(the_a, pbuf, out);
                            the_a = js_get(ptext, in);
                            if (the_a == ']')
                            {
                                break;
                            }
                            if (the_a == '\\')
                            {
                                js_put(the_a, pbuf, out);
                                the_a = js_get(ptext, in);
                            }
                            if (the_a == -1)
                            {
                                eu_logmsg("Unterminated set in Regular Expression literal.\n");
                                return EUE_UNKOWN_ERR;
                            }
                        }
                    }
                    else if (the_a == '/')
                    {
                        switch (js_peek(ptext, in))
                        {
                            case '/':
                            case '*':
                                eu_logmsg("Unterminated set in Regular Expression literal.\n");
                                return EUE_UNKOWN_ERR;
                        }
                        break;
                    }
                    else if (the_a == '\\')
                    {
                        js_put(the_a, pbuf, out);
                        the_a = js_get(ptext, in);
                    }
                    if (the_a == -1)
                    {
                        eu_logmsg("Unterminated Regular Expression literal.\n");
                        return EUE_UNKOWN_ERR;
                    }
                    js_put(the_a, pbuf, out);
                }
                the_b = js_next(ptext, in);
                if (the_b == EUE_UNKOWN_ERR)
                {
                    return EUE_UNKOWN_ERR;
                }
            }
    }
    return SKYLARK_OK;
}

static void
on_format_init_std(FILE** pout, FILE **perr)
{
    if ((_fileno(stderr) != 2) && AllocConsole())
    {
        *pout = freopen("CONOUT$", "w", stdout);
        *perr = freopen("CONOUT$", "w", stderr);
        ShowWindow(FindWindow(_T("ConsoleWindowClass"), NULL), SW_HIDE);
    }
}

static char*
on_format_append_ext(eu_tabpage *pnode)
{
    char *pname = NULL;
    if (pnode && pnode->doc_ptr)
    {
        TCHAR filename[MAX_BUFFER + 1] = {0};
        const TCHAR *tmp = pnode->pathfile[0] && !url_has_remote(pnode->pathfile) ? pnode->pathfile : _T("a");
        switch (pnode->doc_ptr->doc_type)
        {
            case DOCTYPE_CPP:
            {
                _sntprintf(filename, MAX_BUFFER, _T("%s%s"), tmp, _T(".cpp"));
                break;
            }
            case DOCTYPE_CS:
            {
                _sntprintf(filename, MAX_BUFFER, _T("%s%s"), tmp, _T(".cs"));
                break;
            }
            case DOCTYPE_VERILOG:
            {
                _sntprintf(filename, MAX_BUFFER, _T("%s%s"), tmp, _T(".v"));
                break;
            }
            case DOCTYPE_JAVA:
            {
                _sntprintf(filename, MAX_BUFFER, _T("%s%s"), tmp, _T(".java"));
                break;
            }
            case DOCTYPE_JAVASCRIPT:
            {
                _sntprintf(filename, MAX_BUFFER, _T("%s%s"), tmp, _T(".js"));
                break;
            }
            case DOCTYPE_JSON:
            {
                _sntprintf(filename, MAX_BUFFER, _T("%s%s"), tmp, _T(".json"));
                break;
            }
            default:
            {
                break;
            }
        }
        if (filename[0])
        {
            pname = eu_utf16_utf8(filename, NULL);
        }
    }
    return pname;
}

static bool
on_format_init_dll(const char *filename, const char *data, size_t size, char **pout)
{
    bool ret = false;
    HMODULE m_dll = np_load_plugin_library(FORMAT_DLL, false);
    if (m_dll)
    {
        ptr_format fn_lib_format = (ptr_format)GetProcAddress(m_dll, "lib_format");
        if (fn_lib_format)
        {
            FILE *out = NULL, *err = NULL;
            on_format_init_std(&out, &err);
            ret = fn_lib_format(filename, data, size, pout);
            eu_close_file(out);
            eu_close_console(err);
        }
        FreeLibrary(m_dll);
    }
    return ret;
}

int
on_format_js_callback(const uint8_t *text, uint8_t **pbuf)
{
    size_t in = 0;
    size_t out = 0;
    size_t length;
    uint8_t *output = NULL;
    if (!text || !pbuf)
    {
        return 1;
    }    
    length = strlen((const char *)text);
    *pbuf = length > 0 ? (uint8_t *) malloc(length) : NULL;
    if (*pbuf == NULL)
    {
        return 1;
    }
    else
    {
        output = *pbuf;
    }
    the_a = '\n';
    js_action(text, &in, output, &out, 3);
    while (the_a != -1)
    {
        switch (the_a)
        {
            case ' ':
                js_action(text, &in, output, &out, js_alphanum(the_b) ? 1 : 2);
                break;
            case '\n':
                switch (the_b)
                {
                    case '{':
                    case '[':
                    case '(':
                    case '+':
                    case '-':
                    case '!':
                    case '~':
                        js_action(text, &in, output, &out, 1);
                        break;
                    case ' ':
                        js_action(text, &in, output, &out, 3);
                        break;
                    default:
                        js_action(text, &in, output, &out, js_alphanum(the_b) ? 1 : 2);
                }
                break;
            default:
                switch (the_b)
                {
                    case ' ':
                        js_action(text, &in, output, &out, js_alphanum(the_a) ? 1 : 3);
                        break;
                    case '\n':
                        switch (the_a)
                        {
                            case '}':
                            case ']':
                            case ')':
                            case '+':
                            case '-':
                            case '"':
                            case '\'':
                            case '`':
                                js_action(text, &in, output, &out, 1);
                                break;
                            default:
                                js_action(text, &in, output, &out, js_alphanum(the_a) ? 1 : 3);
                        }
                        break;
                    default:
                        js_action(text, &in, output, &out, 1);
                        break;
                }
        }
    }
    output[out] = 0;
    return SKYLARK_OK;
}

int
on_format_json_callback(const uint8_t *text, uint8_t **pbuf)
{
    size_t in = 0;
    size_t out = 0;
    size_t length;
    int state = 0;
    uint8_t *output = NULL;
    /* check if pre-conditions are met */
    if (!text || !pbuf)
    {
        return 1;
    }
    length = strlen((const char *)text);
    *pbuf = length > 0 ? (uint8_t *) malloc(length) : NULL;
    if (*pbuf == NULL)
    {
        return 1;
    }
    else
    {
        output = *pbuf;
    }
    while (in < length)
    {
        switch (text[in])
        {
            case '\x20': /* space */
            case '\x09': /* horizontal tab */
            case '\x0A': /* line feed or new line */
            case '\x0D': /* Carriage return */
                if (state == 1)
                {
                    output[out++] = text[in];
                }
                break;
            case '\"':
                if (!state)
                {
                    state = 1;
                }
                else if (text[in - 1] != '\\')
                {
                    state = 0;
                }
                output[out++] = text[in];
                break;
            default:
                output[out++] = text[in];
        }
        ++in;
    }
    (*pbuf)[out] = 0;
    return SKYLARK_OK;
}

int
on_format_xml_compress(eu_tabpage *pnode)
{
    on_search_do_space(pnode, ">[\\s\r\n]*<", "><", RE_REGXP);
    return SKYLARK_OK;
}

void
on_format_do_compress(eu_tabpage *pnode, format_back fn)
{
    char *text = NULL;
    uint8_t *out = NULL;
    size_t txt_len = 0;
    do
    {
        if (!pnode || !pnode->doc_ptr || pnode->hex_mode || pnode->plugin)
        {
            break;
        }
        if (pnode->doc_ptr->doc_type == DOCTYPE_JSON && !pnode->hwnd_symtree)
        {
            MSG_BOX(IDC_MSG_JSON_ERR1, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
            break;
        }
        if (!(text = util_strdup_content(pnode, &txt_len)))
        {
            break;
        }
        if (txt_len > FORMAT_MAX_LEN)
        {
            MSG_BOX(IDC_MSG_JSON_ERR1, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
            break;
        }
        eu_sci_call(pnode, SCI_BEGINUNDOACTION, 0, 0);
        if (fn((const uint8_t *)text, &out) == SKYLARK_OK && out)
        {
            if (pnode->doc_ptr->doc_type == DOCTYPE_JSON)
            {
                if (strcmp(text, (const char *)out))
                {
                    eu_sci_call(pnode, SCI_CLEARALL, 0, 0);
                    eu_sci_call(pnode, SCI_ADDTEXT, strlen((const char *)out), (LPARAM)out);
                }
            }
            else if (pnode->doc_ptr->doc_type == DOCTYPE_JAVASCRIPT)
            {
                uint8_t *p = out;
                util_skip_whitespace(&p, eu_int_cast(strlen((const char *) p)), 0);
                if (strcmp(text, (const char *)p))
                {
                    eu_sci_call(pnode, SCI_CLEARALL, 0, 0);
                    eu_sci_call(pnode, SCI_ADDTEXT, strlen((const char *)p), (LPARAM)p);
                }
            }
        }
        eu_sci_call(pnode, SCI_ENDUNDOACTION, 0, 0);
    } while(0);
    eu_safe_free(text);
    eu_safe_free(out);
}

void
on_format_clang_file(eu_tabpage *p, const bool whole)
{
    cvector_vector_type(int) v = NULL;
    UNREFERENCED_PARAMETER(p);
    if ((on_tabpage_sel_number(&v, false)) > 0)
    {
        int count = eu_int_cast(cvector_size(v));
        for (int i = 0; i < count; ++i)
        {
            char *out = NULL;
            char *text = NULL;
            char *filename = NULL;
            eu_tabpage *pnode = on_tabpage_get_ptr(v[i]);
            do
            {
                size_t text_len = 0;
                if (!pnode || !pnode->doc_ptr || pnode->hex_mode || pnode->plugin)
                {
                    break;
                }
                if (!(pnode->doc_ptr->doc_type == DOCTYPE_CPP ||
                    pnode->doc_ptr->doc_type == DOCTYPE_CS ||
                    pnode->doc_ptr->doc_type == DOCTYPE_VERILOG ||
                    pnode->doc_ptr->doc_type == DOCTYPE_JAVA ||
                    pnode->doc_ptr->doc_type == DOCTYPE_JAVASCRIPT ||
                    pnode->doc_ptr->doc_type == DOCTYPE_JSON))
                {
                    break;
                }
                if (!(filename = on_format_append_ext(pnode)))
                {
                    break;
                }
                if (whole)
                {
                    if (!(text = util_strdup_content(pnode, &text_len)))
                    {
                        break;
                    }
                }
                else if (eu_sci_call(pnode, SCI_GETSELECTIONS, 0, 0) > 1)
                {
                    MSG_BOX(IDS_SELRECT_MULTI, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                    break;
                }
                else if (!(text = util_strdup_select(pnode, &text_len, 0)))
                {
                    break;
                }
                if (text_len > FORMAT_MAX_LEN)
                {
                    MSG_BOX(IDC_MSG_JSON_ERR1, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                    break;
                }
                eu_sci_call(pnode, SCI_BEGINUNDOACTION, 0, 0);
                if (on_format_init_dll(filename, text, whole ? text_len + 1 : text_len, &out) && strcmp(text, out))
                {
                    if (whole)
                    {
                        eu_sci_call(pnode, SCI_CLEARALL, 0, 0);
                        eu_sci_call(pnode, SCI_ADDTEXT, strlen(out), (sptr_t)out);
                    }
                    else
                    {
                        eu_sci_call(pnode, SCI_REPLACESEL, 0, (sptr_t)out);
                    }
                    if (pnode->doc_ptr && pnode->doc_ptr->doc_type == DOCTYPE_JSON)
                    {
                        on_symtree_json(pnode);
                    }
                    else
                    {
                        on_symlist_reqular(pnode);
                    }
                }
                eu_sci_call(pnode, SCI_ENDUNDOACTION, 0, 0);
            } while(0);
            eu_safe_free(text);
            eu_safe_free(filename);
            eu_safe_free(out);
        }
    }
    cvector_freep(&v);
}

void
on_format_file_style(eu_tabpage *pnode)
{
    if (pnode && !pnode->hex_mode && !pnode->pmod && pnode->doc_ptr)
    {
        if (pnode->doc_ptr->doc_type == DOCTYPE_JSON || pnode->doc_ptr->doc_type == DOCTYPE_JAVASCRIPT)
        {
            on_format_clang_file(pnode, true);
        }
        else if (pnode->doc_ptr->doc_type == DOCTYPE_XML)
        {
            on_xml_format(pnode);
        }
    }
}

static int
on_format_cmp_header(const char *pheader, const int len, const bool use_tab)
{
    int ret = 0;
    char *str = (char *) calloc(1, len + 1);
    if (str)
    {
        use_tab ? memset(str, 0x09, len) : memset(str, 0x20, len);
        if ((ret = strcmp(pheader, str)) != 0)
        {
            use_tab ? memset(str, 0x20, len) : memset(str, 0x09, len);
            if (!(ret = strcmp(pheader, str)))
            {
                ret = 1;
            }
            else
            {
                ret = -1;
            }
        }
    }
    return ret;
}

static void
on_format_get_lines(sptr_t *vec_lines, const int vec_size, wchar_t *pstr, const int len)
{
    int offset = 0;
    int count = min(10, (int)vec_size);
    memset(pstr, 0, sizeof(wchar_t) * len);
    for (int i = 0; i < count; ++i)
    {
        _snwprintf(pstr + offset, len - offset - 1, L"%zd, ", vec_lines[i]);
        offset = (int)wcslen(pstr);
    }
    if (count > 0)
    {
        pstr[wcslen(pstr) - 2] = L'\0';
    }
    if (vec_size > 10)
    {
        wcsncat(pstr, L"...", len - 1);
    }
}

void
on_format_check_indentation(eu_tabpage *pnode)
{
    if (pnode && !pnode->hex_mode && !pnode->pmod)
    {
        size_t vec_size = 0;
        wchar_t *pmsg = NULL;
        wchar_t line_str[ENV_LEN];
        wchar_t opps_str[ENV_LEN] = {0};
        cvector_vector_type(sptr_t) opposite = NULL;
        cvector_vector_type(sptr_t) jumble = NULL;
        bool use_tab = (bool)eu_sci_call(pnode, SCI_GETUSETABS, 0, 0);
        const sptr_t line = eu_sci_call(pnode, SCI_GETLINECOUNT, 0, 0);
        for (sptr_t i = 0; i < line; ++i)
        {
            char *pheader = NULL;
            const sptr_t line_start = eu_sci_call(pnode, SCI_POSITIONFROMLINE, i, 0);
            const sptr_t line_end = eu_sci_call(pnode, SCI_GETLINEENDPOSITION, i, 0);
            int indent = (int)util_line_header(pnode, line_start, line_end, &pheader);
            if (indent > 0 && pheader)
            {
                int ret = on_format_cmp_header(pheader, indent, use_tab);
                if (ret > 0)
                {
                    cvector_push_back(opposite, i + 1);
                }
                else if (ret < 0)
                {
                    cvector_push_back(jumble, i + 1);
                }
            }
            eu_safe_free(pheader);
        }
        if ((vec_size = cvector_size(opposite)) > 0 && (pmsg = (wchar_t *)calloc(sizeof(wchar_t), MAX_BUFFER)))
        {
            LOAD_I18N_RESSTR(use_tab ? IDS_INDENT_TAB_STR : IDS_INDENT_SPACE_STR, var);
            LOAD_I18N_RESSTR(use_tab ? IDS_INDENT_SPACE_OPPS : IDS_INDENT_TAB_OPPS, opps);
            on_format_get_lines(opposite, (const int)vec_size, line_str, ENV_LEN);
            _snwprintf(opps_str, ENV_LEN - 1, opps, line_str, vec_size);
            if (!cvector_size(jumble))
            {
                LOAD_I18N_RESSTR(IDS_INDENT_PROPOSE, propose);
                _snwprintf(pmsg, MAX_BUFFER - 1, L"%s%s%s", var, opps_str, propose);
            }
            else
            {
                _snwprintf(pmsg, MAX_BUFFER - 1, L"%s%s", var, opps_str);
            }
        }
        if ((vec_size = cvector_size(jumble)) > 0)
        {
            LOAD_I18N_RESSTR(IDS_INDENT_TAB_SPACE, jumb);
            LOAD_I18N_RESSTR(IDS_INDENT_PROPOSE, propose);
            on_format_get_lines(jumble, (const int)vec_size, line_str, ENV_LEN);
            _snwprintf(opps_str, ENV_LEN - 1, jumb, line_str, vec_size);
            if (!pmsg && (pmsg = (wchar_t *)calloc(sizeof(wchar_t), MAX_BUFFER)))
            {
                LOAD_I18N_RESSTR(use_tab ? IDS_INDENT_TAB_STR : IDS_INDENT_SPACE_STR, var);
                _snwprintf(pmsg, MAX_BUFFER - 1, L"%s", var);
            }
            wcsncat(pmsg, opps_str, MAX_BUFFER - 1);
            wcsncat(pmsg, propose, MAX_BUFFER - 1);
        }
        if (pmsg)
        {
            LOAD_I18N_RESSTR(IDS_APP_TITLE, title);
            eu_msgbox(eu_module_hwnd(), pmsg, title, MB_ICONWARNING | MB_OK);
        }
        else
        {
            MSG_BOX(IDS_INDENT_CONSISTENT, IDS_APP_TITLE, MB_ICONINFORMATION | MB_OK);
        }
        cvector_free(opposite);
        cvector_free(jumble);
        eu_safe_free(pmsg);
    }
}
