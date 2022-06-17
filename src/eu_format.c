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

#define FORMAT_MAX_LEN  (2 * 1024 * 1024)
typedef bool (*ptr_format)(const char *filename, const char *data, size_t size, char **pout);

static void
init_stderr_redirect(FILE **pconsole)
{
    *pconsole = NULL;
    if ((_fileno(stderr) != 2) && AllocConsole())
    {
        *pconsole = freopen("CONOUT$", "w", stderr);
        ShowWindow (FindWindow(_T("ConsoleWindowClass"), NULL), SW_HIDE);
    }
}

static bool
init_lib_format(const char *filename, const char *data, size_t size, char **pout)
{
    bool ret = false;
    TCHAR format_path[MAX_PATH+1] = {0};
    HMODULE m_dll = NULL;
    _sntprintf(format_path, MAX_PATH, _T("%s\\clang-format.dll"), eu_module_path);
    if ((m_dll = LoadLibrary(format_path)))
    {
        ptr_format fn_lib_format = (ptr_format) GetProcAddress(m_dll, "lib_format");
        if (fn_lib_format)
        {
            FILE *console;
            init_stderr_redirect(&console);
            ret = fn_lib_format(filename, data, size, pout);
            safe_close_console(console);
        }
        FreeLibrary(m_dll);
    }
    return ret;
}

char *
on_format_compress_callback(const char *text)
{
    size_t in = 0;
    size_t out = 0;
    size_t length;
    int state = 0;
    char *output = NULL;
    /* check if pre-conditions are met */
    EU_VERIFY(text != NULL);
    length = strlen(text);
    output = length > 0 ? (char *) malloc(length) : NULL;
    if (output == NULL)
    {
        return NULL;
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
    output[out] = '\0';
    return output;
}

void
on_format_do_json(eu_tabpage *pnode, format_back fn)
{
    char *text = NULL;
    char *fjson = NULL;
    size_t txt_len = 0;
    if (!pnode)
    {
        return;
    }
    if (!pnode->hwnd_symtree)
    {
        MSG_BOX(IDC_MSG_JSON_ERR1, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
        return;
    }
    do
    {
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
        if ((fjson = fn(text)))
        {
            eu_sci_call(pnode, SCI_CLEARALL, 0, 0);
            eu_sci_call(pnode, SCI_ADDTEXT, strlen(fjson), (LPARAM) fjson);
            free(fjson);
        }
        eu_sci_call(pnode, SCI_ENDUNDOACTION, 0, 0);
    } while(0);
    eu_safe_free(text);
}

void
on_format_json_style(eu_tabpage *pnode)
{
    if (!(pnode && pnode->doc_ptr))
    {
        return;
    }
    if (pnode->doc_ptr->doc_type == DOCTYPE_JSON)
    {
        on_format_clang_file(pnode);
    }
}

void
on_format_clang_file(eu_tabpage *pnode)
{
    char *out = NULL;
    char *text = NULL;
    sptr_t text_len = 0;
    char *filename = NULL;
    if (!pnode)
    {
        return;
    }
    filename = eu_utf16_utf8(pnode->filename, NULL);
    if (!filename)
    {
        return;
    }
    do
    {
        if (!(text = util_strdup_content(pnode, (size_t *)&text_len)))
        {
            break;
        }
        if (text_len > FORMAT_MAX_LEN)
        {
            MSG_BOX(IDC_MSG_JSON_ERR1, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
            break;
        }
        eu_sci_call(pnode, SCI_BEGINUNDOACTION, 0, 0);
        if (init_lib_format(filename, text, text_len+1, &out))
        {
            eu_sci_call(pnode, SCI_CLEARALL, 0, 0);
            eu_sci_call(pnode, SCI_ADDTEXT, strlen(out), (LPARAM) out);
        }
        eu_sci_call(pnode, SCI_ENDUNDOACTION, 0, 0);
    } while(0);
    eu_safe_free(text);
    eu_safe_free(filename);
    eu_safe_free(out);
}

void
on_format_clang_str(eu_tabpage *pnode)
{
    char *out = NULL;
    char *text = NULL;
    char *filename = NULL;
    size_t len = 0;
    if (!pnode)
    {
        return;
    }
    if (!(filename = eu_utf16_utf8(pnode->filename, NULL)))
    {
        return;
    }
    do
    {
        if (!(text = util_strdup_select(pnode, &len, 0)))
        {
            break;
        }
        if (len > FORMAT_MAX_LEN)
        {
            MSG_BOX(IDC_MSG_JSON_ERR1, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
            break;
        }
        eu_sci_call(pnode, SCI_BEGINUNDOACTION, 0, 0);
        if (text && init_lib_format(filename, text, len, &out))
        {
            eu_sci_call(pnode, SCI_REPLACESEL, 0, (sptr_t) out);
        }
        eu_sci_call(pnode, SCI_ENDUNDOACTION, 0, 0);
    } while(0);
    eu_safe_free(filename);
    eu_safe_free(text);
    eu_safe_free(out);
}
