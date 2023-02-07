/******************************************************************************
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

#define FORMAT_MAX_LEN  (4 * 1024 * 1024)
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
                            printf("Error: JSMIN Unterminated comment.\n");
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
                        printf("Error: JSMIN unterminated string literal\n");
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
                                printf("Unterminated set in Regular Expression literal.\n");
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
                                printf("Unterminated set in Regular Expression literal.\n");
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
                        printf("Unterminated Regular Expression literal.\n");
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
init_stderr_redirect(FILE** pout, FILE **perr)
{
    if ((_fileno(stderr) != 2) && AllocConsole())
    {
        *pout = freopen("CONOUT$", "w", stdout);
        *perr = freopen("CONOUT$", "w", stderr);
        ShowWindow(FindWindow(_T("ConsoleWindowClass"), NULL), SW_HIDE);
    }
}

static bool
init_lib_format(const char *filename, const char *data, size_t size, char **pout)
{
    bool ret = false;
    HMODULE m_dll = np_load_plugin_library(_T("clang-format.dll"));
    if (m_dll)
    {
        ptr_format fn_lib_format = (ptr_format)GetProcAddress(m_dll, "lib_format");
        if (fn_lib_format)
        {
            FILE *out = NULL, *err = NULL;
            init_stderr_redirect(&out, &err);
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
    char *output = NULL;
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

void
on_format_do_compress(eu_tabpage *pnode, format_back fn)
{
    char *text = NULL;
    uint8_t *out = NULL;
    size_t txt_len = 0;
    do
    {
        if (!pnode || !pnode->doc_ptr)
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
                if (!(filename = eu_utf16_utf8(pnode->filename, NULL)))
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
                if (init_lib_format(filename, text, whole ? text_len + 1 : text_len, &out) && strcmp(text, out))
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
    if (pnode && pnode->doc_ptr && (pnode->doc_ptr->doc_type == DOCTYPE_JSON || pnode->doc_ptr->doc_type == DOCTYPE_JAVASCRIPT))
    {
        on_format_clang_file(pnode, true);
    }
}
