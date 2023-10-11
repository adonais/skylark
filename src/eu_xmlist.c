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

#define EXPAT_START 0
#define EXPAT_END 1
#define EXPAT_PI 3
#define EXPAT_COMMENT 4
#define EXPAT_CHARACTER 5
#define EXPAT_CDATA 6

typedef struct _ex_stack
{
    int done_start;
    int done_end;
    int done_pi;
    int done_comment;
    int done_character;
    int done_cdata;
} ex_stack;

typedef struct _ex_userdata
{
    int depth;
    int lastoperation;
    bool firsttime;
    bool showattributes;
    const char *eol;
    const char *indent;
    uint8_t **ppbuf;
    ex_stack indentstack[FILESIZE];
} ex_userdata;

typedef struct _ex_context
{
    int depth;
    HTREEITEM hitem;
    XML_Parser parser;
    eu_tabpage *ptab;
} ex_context;

static int
count_depth(const HWND hwnd, const HTREEITEM hti)
{
    int count = 0;
    HTREEITEM parent = hti;
    while ((parent = TreeView_GetParent(hwnd, parent)))
    {
        ++count;
    }
    return count;
}

static void XMLCALL
start_element(void *userdata, const XML_Char *name, const XML_Char **atts)
{
    ex_context *ctx = (ex_context *) userdata;
    HWND hwnd = ctx->ptab->hwnd_symtree;
    HTREEITEM parent = ctx->hitem;
    const int count = count_depth(hwnd, ctx->hitem);
    if (count && ctx->depth <= count)
    {
        for (int i = ctx->depth; i <= count; ++i)
        {
            parent = TreeView_GetParent(hwnd, parent);
        }
    }
    ctx->hitem = on_symtree_insert_str(hwnd, parent, name, XML_GetCurrentByteIndex(ctx->parser));
    ctx->depth += 1;
}

static void XMLCALL
end_element(void *userdata, const XML_Char *name)
{
    ex_context *ctx = (ex_context *) userdata;
    ctx->depth -= 1;
}

static void XMLCALL
start_cdata(void *userdata)
{
    ex_context *ctx = (ex_context *) userdata;
    start_element(userdata, "[CDATA]", NULL);
}

static void XMLCALL
end_cdata(void *userdata)
{
    end_element(userdata, NULL);
}

static void
on_xml_parser(eu_tabpage *pnode, const char *pbuf, const int buf_len)
{
    if (pnode && pnode->hwnd_symtree && pbuf && buf_len > 0)
    {
        int done = 0;
        ex_context ctx = { 0 };
        // 创建XML解析器
        XML_Parser parser = XML_ParserCreate("utf-8");
        if (parser)
        {
            ctx.ptab = pnode;
            ctx.parser = parser;
            XML_SetUserData(parser, &ctx);
            XML_SetElementHandler(parser, start_element, end_element);
            XML_SetCdataSectionHandler(parser, start_cdata, end_cdata);
            if (XML_Parse(parser, pbuf, buf_len, done) == XML_STATUS_ERROR)
            {
                char err[MAX_PATH] = { 0 };
                _snprintf(err, MAX_PATH - 1, "(%s at line %Id)\n", XML_ErrorString(XML_GetErrorCode(parser)), XML_GetCurrentLineNumber(parser));
                start_element(&ctx, err, NULL);
            }
            // 释放解析器内存
            XML_ParserFree(parser);
        }
    }
}

static unsigned WINAPI
on_xml_thread(void *lp)
{
    eu_tabpage *pnode = (eu_tabpage *) lp;
    if (pnode && pnode->hwnd_symtree)
    {
        char *text = NULL;
        size_t text_len = 0;
        if ((text = util_strdup_content(pnode, &text_len)) != NULL)
        {
            TreeView_DeleteAllItems(pnode->hwnd_symtree);
            on_xml_parser(pnode, text, (const int) text_len);
            if (TreeView_GetRoot(pnode->hwnd_symtree))
            {
                on_symtree_expand_all(pnode->hwnd_symtree, NULL);
            }
            free(text);
        }
        _InterlockedExchange(&pnode->json_id, 0);
    }
    return 0;
}

static bool
on_xml_white_space(uint8_t *s)
{
    while (*s)
    {
        if (!util_has_space(*s))
        {
            return false;
        }
        s++;
    }
    return true;
}

static void
on_xml_indent(ex_userdata *ud)
{
    if (ud)
    {
        if (ud->firsttime)
        {
            ud->firsttime = false;
        }
        else
        {
            util_strcat(ud->ppbuf, ud->eol);
            for (int i = 0; i < ud->depth; i++)
            {
                util_strcat(ud->ppbuf, ud->indent);
            }
        }
    }
}

static void XMLCALL
on_xml_element_start(void *userdata, const XML_Char *el, const XML_Char **attr)
{
    ex_userdata *ud = (ex_userdata *) userdata;
    if (ud->depth >= 0 && ud->depth < FILESIZE)
    {
        on_xml_indent(ud);
        cvector_push_back(*(ud->ppbuf), '<');
        util_strcat(ud->ppbuf, el);
        if (ud->showattributes)
        {
            for (int i = 0; attr[i]; i += 2)
            {
                cvector_push_back(*(ud->ppbuf), ' ');
                util_strcat(ud->ppbuf, attr[i]);
                cvector_push_back(*(ud->ppbuf), '=');
                cvector_push_back(*(ud->ppbuf), '"');
                util_strcat(ud->ppbuf, attr[i + 1]);
                cvector_push_back(*(ud->ppbuf), '"');
            }
        }
        cvector_push_back(*(ud->ppbuf), '>');
        ud->lastoperation = EXPAT_START;
        ud->indentstack[ud->depth].done_start = 1;
    }
    ud->depth++;
}

static void XMLCALL
on_xml_element_end(void *userdata, const XML_Char *el)
{
    ex_userdata *ud = (ex_userdata *) userdata;
    ud->depth--;
    if (ud->depth >= 0 && ud->depth < FILESIZE)
    {
        if (ud->lastoperation == EXPAT_END || ud->indentstack[ud->depth].done_pi || ud->indentstack[ud->depth].done_comment ||
            ud->indentstack[ud->depth].done_cdata)
        {
            on_xml_indent(ud);
        }
        util_strcat(ud->ppbuf, "</");
        util_strcat(ud->ppbuf, el);
        cvector_push_back(*(ud->ppbuf), '>');
        ud->lastoperation = EXPAT_END;
        memset(&ud->indentstack[ud->depth], 0, sizeof(ex_stack));
    }
}

static void XMLCALL
on_xml_data_handler(void *userdata, const XML_Char *txt, int txtlen)
{
    ex_userdata *ud = (ex_userdata *) userdata;
    if (ud->depth >= 0 && ud->depth < FILESIZE)
    {
        // 指向文本头部
        uint8_t *s = (uint8_t *) txt;
        // 指向文本尾部
        uint8_t *e = ((uint8_t *) txt) + txtlen - 1;
        // 头部指针跳过空白
        while (util_has_space(*s) && txtlen > 0)
        {
            s++;
            txtlen--;
        }
        // 尾部指针移过空白
        if (txtlen > 0)
        {
            while (util_has_space(*e) && txtlen > 0)
            {
                e--;
                txtlen--;
            }
        }
        // 去除头尾空白的文本数据
        if (txtlen > 0)
        {
            if (ud->lastoperation == EXPAT_COMMENT || ud->lastoperation == EXPAT_PI || ud->lastoperation == EXPAT_CDATA ||
                ud->lastoperation == EXPAT_END)
            {
                on_xml_indent(ud);
            }
            for (int i = 0; i < txtlen; i++)
            {
                cvector_push_back(*(ud->ppbuf), s[i]);
            }
            ud->lastoperation = EXPAT_CHARACTER;
            if (ud->depth > 0)
            {
                ud->indentstack[ud->depth - 1].done_character = 1;
            }
        }
    }
}

static void XMLCALL
on_xml_doctype_handler(void *userdata, const XML_Char *name, const XML_Char *sysid, const XML_Char *pubid, int has_)
{
    ex_userdata *ud = (ex_userdata *) userdata;
    on_xml_indent(ud);
    util_strcat(ud->ppbuf, "<!DOCTYPE ");
    util_strcat(ud->ppbuf, name);
    if (pubid && sysid)
    {
        util_strcat(ud->ppbuf, " PUBLIC \"");
        util_strcat(ud->ppbuf, pubid);
        util_strcat(ud->ppbuf, "\" \"");
        util_strcat(ud->ppbuf, sysid);
        cvector_push_back(*(ud->ppbuf), '"');
    }
    else
    {
        if (pubid)
        {
            util_strcat(ud->ppbuf, " PUBLIC \"");
            util_strcat(ud->ppbuf, pubid);
            cvector_push_back(*(ud->ppbuf), '"');
        }
        else if (sysid)
        {
            util_strcat(ud->ppbuf, " SYSTEM \"");
            util_strcat(ud->ppbuf, sysid);
            cvector_push_back(*(ud->ppbuf), '"');
        }
    }
    // 不支持其他的内部子集
    cvector_push_back(*(ud->ppbuf), '>');
}

static void XMLCALL
on_xml_pi_handler(void *userdata, const XML_Char *target, const XML_Char *pidata)
{
    ex_userdata *ud = (ex_userdata *) userdata;
    if (ud->depth >= 0 && ud->depth < FILESIZE)
    {
        on_xml_indent(ud);
        util_strcat(ud->ppbuf, "<?");
        util_strcat(ud->ppbuf, target);
        cvector_push_back(*(ud->ppbuf), ' ');
        util_strcat(ud->ppbuf, pidata);
        util_strcat(ud->ppbuf, "?>");
        ud->lastoperation = EXPAT_PI;
        if (ud->depth > 0)
        {
            ud->indentstack[ud->depth - 1].done_pi = true;
        }
    }
}

static void XMLCALL
on_xml_comment_handler(void *userdata, const XML_Char *data)
{
    char *p = NULL;
    ex_userdata *ud = (ex_userdata *) userdata;
    if (ud->depth >= 0 && ud->depth < FILESIZE)
    {
        on_xml_indent(ud);
        if (util_has_newline((const char *) data))
        {
            // 如果注释中有换行符, 保留文件格式, 按换行符进行缩进
            char *s = (char *) data;
            util_strcat(ud->ppbuf, "<!--");
            ud->depth++;
            while ((p = util_has_newline(s)))
            {
                *p++ = 0;
                if (*p == '\r' || *p == '\n')
                {
                    p++;
                }
                on_xml_indent(ud);
                util_strcat(ud->ppbuf, s);
                s = p;
            }
            if (strlen(s) > 0 && !on_xml_white_space((const uint8_t *)s))
            {
                on_xml_indent(ud);
                util_strcat(ud->ppbuf, s);
            }
            ud->depth--;
            on_xml_indent(ud);
            util_strcat(ud->ppbuf, "-->");
        }
        else
        {
            p = util_trim_sides_white((const uint8_t *)data);
            util_strcat(ud->ppbuf, "<!-- ");
            util_strcat(ud->ppbuf, p ? p : data);
            util_strcat(ud->ppbuf, " -->");
            eu_safe_free(p);
        }
        ud->lastoperation = EXPAT_COMMENT;
        if (ud->depth > 0)
        {
            ud->indentstack[ud->depth - 1].done_comment = 1;
        }
    }
}

static void XMLCALL
on_xml_cdata_start(void *userdata)
{
    ex_userdata *ud = (ex_userdata *) userdata;
    on_xml_indent(ud);
    util_strcat(ud->ppbuf, "<![CDATA[");
}

static void XMLCALL
on_xml_cdata_end(void *userdata)
{
    ex_userdata *ud = (ex_userdata *) userdata;
    util_strcat(ud->ppbuf, "]]>");
    if (ud->depth > 0)
    {
        ud->indentstack[ud->depth - 1].done_cdata = 1;
    }
    ud->lastoperation = EXPAT_CDATA;
}

static void XMLCALL
on_xml_declaration_handler(void *userdata, const XML_Char *version, const XML_Char *encoding, int standalone)
{
    ex_userdata *ud = (ex_userdata *) userdata;
    if (ud->depth >= 0 && ud->depth < FILESIZE && version)
    {
        on_xml_indent(ud);
        util_strcat(ud->ppbuf, "<?xml version=\"");
        util_strcat(ud->ppbuf, version);
        cvector_push_back(*(ud->ppbuf), '"');
        if (encoding)
        {
            util_strcat(ud->ppbuf, " encoding=\"");
            util_strcat(ud->ppbuf, encoding);
            cvector_push_back(*(ud->ppbuf), '"');
        }
        if (standalone == 0 || standalone == 1)
        {
            util_strcat(ud->ppbuf, " standalone=\"");
            util_strcat(ud->ppbuf, (standalone == 0) ? "no" : "yes");
            cvector_push_back(*(ud->ppbuf), '"');
        }
        util_strcat(ud->ppbuf, "?>");
        ud->lastoperation = EXPAT_PI;
        if (ud->depth > 0)
        {
            ud->indentstack[ud->depth - 1].done_pi = true;
        }
    }
}

bool
on_xml_pretty(void *ptr, struct opt_format *opt)
{
    bool ret = false;
    char *text = NULL;
    eu_tabpage *pnode = (eu_tabpage *) ptr;
    cvector_vector_type(uint8_t) pout = NULL;
    if (pnode && opt)
    {
        size_t text_len = 0;
        ex_userdata ctx = { 0 };
        XML_Parser parser = NULL;
        if ((text = util_strdup_content(pnode, &text_len)) && (parser = XML_ParserCreate("utf-8")) && text_len < INT_MAX)
        {
            int done = 0;
            char indent[QW_SIZE] = { 0 };
            ctx.showattributes = !opt->no_a;
            ctx.firsttime = true;
            ctx.eol = on_encoding_get_eol(pnode);
            if ((bool) eu_sci_call(pnode, SCI_GETUSETABS, 0, 0))
            {
                indent[0] = '\t';
                indent[1] = 0;
            }
            else
            {
                int n = (int) eu_sci_call(pnode, SCI_GETTABWIDTH, 0, 0);
                memset(indent, 0x20, (n > 0 && n < QW_SIZE) ? n : 4);
            }
            cvector_init(&pout, text_len, uint8_t); 
            ctx.indent = indent;
            ctx.ppbuf = &pout;
            XML_SetUserData(parser, &ctx);
            XML_SetElementHandler(parser, on_xml_element_start, on_xml_element_end);
            if (!opt->no_c)
            {
                XML_SetCommentHandler(parser, on_xml_comment_handler);
            }
            if (!opt->no_p)
            {
                XML_SetProcessingInstructionHandler(parser, on_xml_pi_handler);
            }
            if (!opt->no_f)
            {
                XML_SetCdataSectionHandler(parser, on_xml_cdata_start, on_xml_cdata_end);
            }
            if (!opt->no_x)
            {
                XML_SetXmlDeclHandler(parser, on_xml_declaration_handler);
            }
            if (!opt->no_t)
            {
                XML_SetStartDoctypeDeclHandler(parser, on_xml_doctype_handler);
            }
            if (!opt->no_d)
            {
                XML_SetCharacterDataHandler(parser, on_xml_data_handler);
            }
            if (XML_Parse(parser, text, (int) text_len, done) == XML_STATUS_ERROR)
            {
                eu_logmsg("error: %s at line %Id\n", XML_ErrorString(XML_GetErrorCode(parser)), XML_GetCurrentLineNumber(parser));
            }
            else
            {
                ret = true;
            }
            cvector_push_back(pout, '\0');
        }
        if (parser)
        {
            XML_ParserFree(parser);
        }
    }
    if (ret && strcmp(text, (const char *)pout))
    {
        eu_sci_call(pnode, SCI_BEGINUNDOACTION, 0, 0);
        eu_sci_call(pnode, SCI_CLEARALL, 0, 0);
        eu_sci_call(pnode, SCI_ADDTEXT, strlen((const char *)pout), (sptr_t)pout);
        eu_sci_call(pnode, SCI_ENDUNDOACTION, 0, 0);
    }
    eu_safe_free(text);
    cvector_free(pout);
    return ret;
}

int
on_xml_format(eu_tabpage *pnode)
{
    char *lua_path = NULL;
    TCHAR path[MAX_BUFFER] = { 0 };
    int m = _sntprintf(path, MAX_BUFFER, _T("%s\\conf\\conf.d\\xmlformater.lua"), eu_module_path);
    if (!(m > 0 && m < MAX_BUFFER) || ((lua_path = eu_utf16_utf8(path, NULL)) == NULL))
    {
        eu_logmsg("%s: path unavailable\n", __FUNCTION__);
        goto xml_fail;
    }
    if (do_lua_point(lua_path, "run", pnode))
    {
        eu_logmsg("%s: xmlformater.lua failed\n", __FUNCTION__);
    }
xml_fail:
    eu_safe_free(lua_path);
    return SKYLARK_OK;
}

int
on_xml_tree(eu_tabpage *pnode)
{
    if (!pnode || !pnode->hwnd_symtree)
    {
        return 1;
    }
    if (pnode->raw_size > 0xA00000)
    {
        eu_logmsg("Error: This XML file is larger than 10MB\n");
        if (pnode->hwnd_symtree)
        {
            DestroyWindow(pnode->hwnd_symtree);
            pnode->hwnd_symtree = NULL;
        }
        return 1;
    }
    if (!pnode->json_id)
    {
        CloseHandle((HANDLE) _beginthreadex(NULL, 0, &on_xml_thread, pnode, 0, (uint32_t *) &pnode->json_id));
    }
    return 0;
}
