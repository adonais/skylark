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
start_element(void *userdata, const char *name, const char **atts)
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
end_element(void *userdata, const char *name)
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
        XML_Parser parser = XML_ParserCreate(NULL);
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
