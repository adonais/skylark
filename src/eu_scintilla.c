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

#define SKYLARK_CURSORHAND 0x8

volatile sptr_t eu_edit_wnd = 0;
static volatile sptr_t ptr_scintilla = 0;
static volatile sptr_t last_sci_eusc = 0;
static volatile sptr_t last_sci_hwnd = 0;

static const char *auto_xpm[] = {
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

void
on_sci_clear_history(eu_tabpage *pnode, const bool refresh)
{
    if (pnode)
    {
        eu_sci_call(pnode, SCI_EMPTYUNDOBUFFER, 0, 0);
        eu_sci_call(pnode, SCI_SETCHANGEHISTORY, SC_CHANGE_HISTORY_DISABLED, 0);
        if (refresh)
        {
            const int maskn = eu_get_config()->history_mask - IDM_VIEW_HISTORY_PLACEHOLDE;
            if (maskn > 1)
            {
                eu_sci_call(pnode, SCI_SETCHANGEHISTORY, maskn, 0);
                util_redraw(pnode->hwnd_sc, false);
            }
        }
    }
}

void
on_sci_update_history_margin(eu_tabpage *pnode)
{
    const int maskn = eu_get_config()->history_mask - IDM_VIEW_HISTORY_PLACEHOLDE;
    if (pnode && maskn > 0)
    {
        const bool margin_enable = maskn > 1 && maskn & SC_CHANGE_HISTORY_MARKERS;
        eu_sci_call(pnode, SCI_SETMARGINWIDTHN, MARGIN_HISTORY_INDEX, margin_enable ? MARGIN_HISTORY_WIDTH : 0);
        eu_sci_call(pnode, SCI_SETCHANGEHISTORY, maskn == 1 ? SC_CHANGE_HISTORY_DISABLED : maskn, 0);
    }
}

void
on_sci_init_default(eu_tabpage *pnode, intptr_t bgcolor)
{
    if (!pnode || !eu_get_config() || !eu_get_theme())
    {
        return;
    }
    // 编辑区样式与字体设置
    eu_sci_call(pnode, SCI_STYLERESETDEFAULT, 0, 0);
    eu_sci_call(pnode, SCI_STYLESETFONT, STYLE_DEFAULT, (sptr_t)(eu_get_theme()->item.text.font));
    eu_sci_call(pnode, SCI_STYLESETSIZE, STYLE_DEFAULT, eu_get_theme()->item.text.fontsize);
    eu_sci_call(pnode, SCI_STYLESETFORE, STYLE_DEFAULT, eu_get_theme()->item.text.color);
    eu_sci_call(pnode, SCI_STYLESETBACK, STYLE_DEFAULT, bgcolor >= 0 ? bgcolor : eu_get_theme()->item.text.bgcolor);
    eu_sci_call(pnode, SCI_STYLESETBOLD, STYLE_DEFAULT, eu_get_theme()->item.text.bold);
    eu_sci_call(pnode, SCI_STYLECLEARALL, 0, 0);
    // scintilla 5.3.4, DirectWrite on Win32, Can text measurement be safely performed concurrently on multiple threads?
    eu_sci_call(pnode, SCI_SUPPORTSFEATURE, SC_SUPPORTS_THREAD_SAFE_MEASURE_WIDTHS, 0);
    eu_sci_call(pnode, SCI_SETLAYOUTTHREADS, (sptr_t)util_num_cores(), 0);
    // 页边区设置
    eu_sci_call(pnode, SCI_SETMARGINS, MARGIN_NUMBER_OF, 0);
    // 行号显示以及样式设置
    eu_sci_call(pnode, SCI_SETMARGINTYPEN, MARGIN_LINENUMBER_INDEX, SC_MARGIN_NUMBER);
    eu_sci_call(pnode, SCI_STYLESETFONT, STYLE_LINENUMBER, (sptr_t)(eu_get_theme()->item.linenumber.font));
    eu_sci_call(pnode, SCI_STYLESETSIZE, STYLE_LINENUMBER, eu_get_theme()->item.linenumber.fontsize);
    eu_sci_call(pnode, SCI_STYLESETFORE, STYLE_LINENUMBER, eu_get_theme()->item.linenumber.color);
    eu_sci_call(pnode, SCI_STYLESETBACK, STYLE_LINENUMBER, eu_get_theme()->item.linenumber.bgcolor);
    eu_sci_call(pnode, SCI_SETMARGINWIDTHN, MARGIN_LINENUMBER_INDEX, eu_get_config()->m_linenumber ? MARGIN_LINENUMBER_WIDTH : 0);
    sptr_t bgra = eu_get_theme()->item.indicator.bgcolor;
    // 自动补全窗口颜色
    eu_sci_call(pnode, SCI_SETELEMENTCOLOUR, SC_ELEMENT_LIST, eu_get_theme()->item.text.color);
    eu_sci_call(pnode, SCI_SETELEMENTCOLOUR, SC_ELEMENT_LIST_BACK, eu_get_theme()->item.text.bgcolor);
    eu_sci_call(pnode, SCI_SETELEMENTCOLOUR, SC_ELEMENT_LIST_SELECTED_BACK, bgra);
    // 函数原型窗口颜色
    eu_sci_call(pnode, SCI_CALLTIPSETFORE, eu_get_config()->eu_calltip.rgb != (uint32_t)-1 ? eu_get_config()->eu_calltip.rgb : eu_get_theme()->item.text.color, 0);
    eu_sci_call(pnode, SCI_CALLTIPSETBACK, eu_get_theme()->item.text.bgcolor, 0);
    // https://www.scintilla.org/ScintillaDoc.html#SCI_STYLESETCHECKMONOSPACED
    eu_sci_call(pnode, SCI_STYLESETCHECKMONOSPACED, STYLE_DEFAULT, true);
    // 书签栏样式
    eu_sci_call(pnode, SCI_SETMARGINSENSITIVEN, MARGIN_BOOKMARK_INDEX, TRUE);
    eu_sci_call(pnode, SCI_SETMARGINMASKN, MARGIN_BOOKMARK_INDEX, MARGIN_BOOKMARK_MASKN);
    eu_sci_call(pnode, SCI_SETMARGINWIDTHN, MARGIN_BOOKMARK_INDEX, (eu_get_config()->eu_bookmark.visable ? MARGIN_BOOKMARK_WIDTH : 0));
    eu_sci_call(pnode, SCI_MARKERDEFINE, MARGIN_BOOKMARK_VALUE, eu_get_config()->eu_bookmark.shape);
    eu_sci_call(pnode, SCI_MARKERSETBACKTRANSLUCENT, MARGIN_BOOKMARK_VALUE, eu_get_config()->eu_bookmark.argb);
    eu_sci_call(pnode, SCI_MARKERSETFORETRANSLUCENT, MARGIN_BOOKMARK_VALUE, eu_get_config()->eu_bookmark.argb);
    eu_sci_call(pnode, SCI_SETMARGINCURSORN, MARGIN_BOOKMARK_INDEX, SKYLARK_CURSORHAND);
    // 修改文本历史记录样式
    eu_sci_call(pnode, SCI_SETMARGINSENSITIVEN, MARGIN_HISTORY_INDEX, TRUE);
    eu_sci_call(pnode, SCI_SETMARGINTYPEN, MARGIN_HISTORY_INDEX, SC_MARGIN_SYMBOL);
    eu_sci_call(pnode, SCI_SETMARGINMASKN, MARGIN_HISTORY_INDEX, MARGIN_HISTORY_MASKN);
    eu_sci_call(pnode, SCI_MARKERSETFORE, SC_MARKNUM_HISTORY_MODIFIED, eu_get_theme()->item.nchistory.color);
    eu_sci_call(pnode, SCI_MARKERSETBACK, SC_MARKNUM_HISTORY_MODIFIED, eu_get_theme()->item.nchistory.color);
    eu_sci_call(pnode, SCI_MARKERSETFORE, SC_MARKNUM_HISTORY_SAVED, eu_get_theme()->item.nchistory.bgcolor);
    eu_sci_call(pnode, SCI_MARKERSETBACK, SC_MARKNUM_HISTORY_MODIFIED, eu_get_theme()->item.nchistory.bgcolor);
    eu_sci_call(pnode, SCI_INDICSETFORE, INDICATOR_HISTORY_MODIFIED_INSERTION, eu_get_theme()->item.dochistory.color);
    eu_sci_call(pnode, SCI_INDICSETFORE, INDICATOR_HISTORY_SAVED_INSERTION, eu_get_theme()->item.dochistory.bgcolor);
    // 代码折叠栏颜色与亮量颜色, 这里与背景色相同
    eu_sci_call(pnode, SCI_SETFOLDMARGINCOLOUR, true, eu_get_theme()->item.foldmargin.bgcolor);
    eu_sci_call(pnode, SCI_SETFOLDMARGINHICOLOUR, true, eu_get_theme()->item.foldmargin.bgcolor);
    // 当前行背景色
    eu_sci_call(pnode, SCI_SETCARETLINEVISIBLE, TRUE, 0);
    eu_sci_call(pnode, SCI_SETCARETLINEVISIBLEALWAYS, 1, 0);
    eu_sci_call(pnode, SCI_SETCARETLINEBACK, eu_get_theme()->item.caretline.bgcolor, 0);
    // 设置边框透明度
    eu_sci_call(pnode, SCI_SETCARETLINEBACKALPHA, eu_get_theme()->item.caretline.bgcolor >> 24, 0);
    eu_sci_call(pnode, SCI_SETCARETLINEFRAME, 2, 0);
    // 设置插入符样式
    eu_sci_call(pnode, SCI_SETCARETSTYLE, 1, 0);
    eu_sci_call(pnode, SCI_SETCARETWIDTH, eu_get_theme()->item.caret.color >> 24, 0);
    eu_sci_call(pnode, SCI_SETCARETPERIOD, eu_get_theme()->item.caret.bold, 0);
    eu_sci_call(pnode, SCI_SETCARETFORE, eu_get_theme()->item.caret.color & 0x00FFFFFF, 0);
    eu_sci_call(pnode, SCI_SETADDITIONALCARETFORE, eu_get_theme()->item.caret.color & 0x00FFFFFF, 0);
    // 选中行背景色
    eu_sci_call(pnode, SCI_SETSELECTIONLAYER, SC_LAYER_OVER_TEXT, 0);
    eu_sci_call(pnode, SCI_SETELEMENTCOLOUR, SC_ELEMENT_SELECTION_BACK, bgra);
    // 其他选中
    eu_sci_call(pnode, SCI_SETELEMENTCOLOUR, SC_ELEMENT_SELECTION_ADDITIONAL_BACK, bgra);
    eu_sci_call(pnode, SCI_SETELEMENTCOLOUR, SC_ELEMENT_SELECTION_SECONDARY_BACK, bgra);
    // 窗口未激活时选中色
    eu_sci_call(pnode, SCI_SETELEMENTCOLOUR, SC_ELEMENT_SELECTION_INACTIVE_BACK, bgra);
    // 是否自动换行
    eu_sci_call(pnode, SCI_SETWRAPMODE, (eu_get_config()->line_mode ? 2 : 0), 0);
    // 换行符样式
    eu_sci_call(pnode, SCI_SETVIEWEOL, eu_get_config()->newline_visialbe, 0);
    // tab字符是否当成空格
    if (pnode->doc_ptr)
    {
        eu_sci_call(pnode, SCI_SETTABWIDTH, pnode->doc_ptr->tab_width > 0 ? pnode->doc_ptr->tab_width : eu_get_config()->tab_width, 0);
        eu_sci_call(pnode, SCI_SETUSETABS, pnode->doc_ptr->tab_convert_spaces >= 0 ? !pnode->doc_ptr->tab_convert_spaces : !eu_get_config()->tab2spaces, 0);
    }
    else
    {
        eu_sci_call(pnode, SCI_SETTABWIDTH, eu_get_config()->tab_width, 0);
        eu_sci_call(pnode, SCI_SETUSETABS,!eu_get_config()->tab2spaces, 0);
    }
    // tab字符显示时的样式
    eu_sci_call(pnode, SCI_SETTABDRAWMODE, SCTD_LONGARROW, 0);
    // 空白字符样式
    eu_sci_call(pnode, SCI_SETVIEWWS, (eu_get_config()->ws_visiable ? SCWS_VISIBLEALWAYS : SCWS_INVISIBLE), 0);
    eu_sci_call(pnode, SCI_SETWHITESPACESIZE, eu_get_config()->ws_size, 0);
    eu_sci_call(pnode, SCI_SETELEMENTCOLOUR, SC_ELEMENT_WHITE_SPACE_BACK, rgb_alpha(eu_get_theme()->item.text.bgcolor, SC_ALPHA_OPAQUE));
    eu_sci_call(pnode, SCI_SETELEMENTCOLOUR, SC_ELEMENT_WHITE_SPACE, rgb_alpha(eu_get_theme()->item.text.color, 0x3f));
    // 是否显示对齐线
    eu_sci_call(pnode, SCI_SETINDENTATIONGUIDES, (eu_get_config()->m_indentation ? SC_IV_LOOKBOTH : SC_IV_NONE), 0);
    eu_sci_call(pnode, SCI_SETMULTIPLESELECTION, true, 0);
    eu_sci_call(pnode, SCI_SETADDITIONALSELECTIONTYPING, true, 0);
    eu_sci_call(pnode, SCI_SETVIRTUALSPACEOPTIONS, 1, 0);
    // 设置字体抗锯齿
    if (eu_get_config()->m_quality ==  IDM_VIEW_FONTQUALITY_STANDARD)
    {
        eu_sci_call(pnode, SCI_SETFONTQUALITY,  SC_EFF_QUALITY_ANTIALIASED, 0);
    }
    else if (eu_get_config()->m_quality == IDM_VIEW_FONTQUALITY_NONE)
    {
        eu_sci_call(pnode, SCI_SETFONTQUALITY,  SC_EFF_QUALITY_NON_ANTIALIASED, 0);
    }
    else
    {
        eu_sci_call(pnode, SCI_SETFONTQUALITY, SC_EFF_QUALITY_LCD_OPTIMIZED, 0);
    }
    // 设置字体渲染方式
    if (util_under_wine())
    {
        eu_sci_call(pnode, SCI_SETTECHNOLOGY, SC_TECHNOLOGY_DEFAULT, 0);
    }
    else if (eu_get_config()->m_render == IDM_SET_RENDER_TECH_D2D)
    {
        eu_sci_call(pnode, SCI_SETTECHNOLOGY, SC_TECHNOLOGY_DIRECTWRITE, 0);
    }
    else if (eu_get_config()->m_render == IDM_SET_RENDER_TECH_D2DRETAIN)
    {
        eu_sci_call(pnode, SCI_SETTECHNOLOGY, SC_TECHNOLOGY_DIRECTWRITERETAIN, 0);
    }
    else
    {
        eu_sci_call(pnode, SCI_SETTECHNOLOGY, SC_TECHNOLOGY_DEFAULT, 0);
    }
    // 需要时显示水平滚动条, 但是删除文本后, 滚动条不会消失
    eu_sci_call(pnode, SCI_SETSCROLLWIDTH, 1, 0);
    eu_sci_call(pnode, SCI_SETSCROLLWIDTHTRACKING, 1, 0);
    // 设置undo掩码, 接受SCN_MODIFIED消息
    eu_sci_call(pnode, SCI_SETMODEVENTMASK, SC_MOD_INSERTTEXT|SC_MOD_DELETETEXT|SC_PERFORMED_UNDO|SC_PERFORMED_REDO, 0);
    // 支持多列粘贴
    eu_sci_call(pnode, SCI_SETMULTIPASTE, SC_MULTIPASTE_EACH, 0);
    // 设置括号匹配高亮色以及指示出不匹配的大括号
    eu_sci_call(pnode, SCI_STYLESETFORE, STYLE_BRACELIGHT, eu_get_config()->eu_brace.rgb);
    eu_sci_call(pnode, SCI_STYLESETBACK, STYLE_BRACELIGHT, eu_get_theme()->item.text.bgcolor);
    eu_sci_call(pnode, SCI_STYLESETBOLD, STYLE_BRACELIGHT, true);
    eu_sci_call(pnode, SCI_STYLESETITALIC, STYLE_BRACEBAD, true);
    eu_sci_call(pnode, SCI_STYLESETUNDERLINE, STYLE_BRACEBAD, true);
    eu_sci_call(pnode, SCI_BRACEBADLIGHTINDICATOR, true, INDIC_STRIKE);
    // 不产生鼠标悬浮消息(SCN_DWELLSTART, SCN_DWELLEND, 设置SC_TIME_FOREVER>0则产生
    eu_sci_call(pnode, SCI_SETMOUSEDWELLTIME, SC_TIME_FOREVER, 0);
    // 注册补全列表图标
    eu_sci_call(pnode, SCI_REGISTERIMAGE, SNIPPET_FUNID, (sptr_t)auto_xpm);
}

void
on_sci_init_style(eu_tabpage *pnode)
{
    on_sci_init_default(pnode, -1);
    // 为超链接设置indicators样式
    on_hyper_set_style(pnode);
}

static void
on_sci_reset_zoom(eu_tabpage *pnode)
{
    if (pnode && pnode->zoom_level != 0)
    {
        int zoom = pnode->zoom_level;
        if (pnode->zoom_level > 0)
        {
            while (zoom--)
            {
                on_view_zoom_in(pnode);
            }
        }
        else if (pnode->zoom_level < 0)
        {
            while (zoom++)
            {
                on_view_zoom_out(pnode);
            }
        }
    }
}

void
on_sci_before_file(eu_tabpage *pnode, const bool init)
{
    if (pnode)
    {
        on_sci_init_style(pnode);
        eu_sci_call(pnode, SCI_CANCEL, 0, 0);
        eu_sci_call(pnode, SCI_SETREADONLY, 0, 0);
        init ? eu_sci_call(pnode, SCI_SETUNDOCOLLECTION, 0, 0) : (void)0;
        if (pnode->doc_ptr && pnode->doc_ptr->fn_init_before)
        {   // 初始化侧边栏控件
            pnode->doc_ptr->fn_init_before(pnode);
        }
    }
}

void
on_sci_after_file(eu_tabpage *pnode, const bool init)
{
    if (pnode)
    {
        if (!pnode->hex_mode && !pnode->pmod)
        {
            eu_sci_call(pnode, SCI_SETEOLMODE, pnode->eol, 0);
            if (init)
            {
                eu_sci_call(pnode, SCI_SETUNDOCOLLECTION, 1, 0);
                on_sci_clear_history(pnode, false);
                on_sci_update_history_margin(pnode);
            }
            on_sci_reset_zoom(pnode);
            if (!pnode->raw_size)
            {
                pnode->raw_size = eu_sci_call(pnode, SCI_GETLENGTH, 0, 0) + pnode->pre_len;
            }
            if (pnode->doc_ptr && pnode->doc_ptr->fn_init_after)
            {   // 设置此标签页的语法解析
                pnode->doc_ptr->fn_init_after(pnode);
            }
            on_sci_update_line_margin(pnode);
        }
        else if (!pnode->plugin)
        {
            on_sci_reset_zoom(pnode);
        }
    }
}

void
on_sci_refresh_ui(eu_tabpage *pnode)
{
    if (pnode)
    {
        on_toolbar_update_button();
        on_statusbar_update();
        on_sci_update_line_margin(pnode);
        util_redraw(g_tabpages, true);
    }
}

void
on_sci_resever_tab(eu_tabpage *pnode)
{
    EU_VERIFY(pnode != NULL);
    if (pnode->hwnd_symlist)
    {
        DestroyWindow(pnode->hwnd_symlist);
        pnode->hwnd_symlist = NULL;
    }
    if (pnode->hwnd_symtree)
    {
        DestroyWindow(pnode->hwnd_symtree);
        pnode->hwnd_symtree = NULL;
    }
    if (pnode->hwnd_qrtable)
    {
        DestroyWindow(pnode->hwnd_qrtable);
        pnode->hwnd_qrtable = NULL;
    }
}

// 清理上一次的备份
static void
on_sci_delete_file(const eu_tabpage *pnode)
{
    if (pnode && !pnode->be_modify && eu_exist_file(pnode->bakpath))
    {
        wchar_t tmp[MAX_BUFFER] = {0};
        _sntprintf(tmp, MAX_BUFFER, _T("%s~~"), pnode->bakpath);
        if (!util_delete_file(pnode->bakpath))
        {
            eu_logmsg("%s: delete(pnode->bakpath) error, cause: %lu\n", __FUNCTION__, GetLastError());
        }
        if (eu_exist_file(tmp) && !DeleteFile(tmp))
        {
            eu_logmsg("%s: delete(pnode->bakpath~~) error, cause: %lu\n", __FUNCTION__, GetLastError());
        }
    }
}

static void
on_sci_swap_hwnd(eu_tabpage *pnode)
{
    if (pnode)
    {
        if (!pnode->phex && !pnode->pmod && TabCtrl_GetItemCount(g_tabpages) <= 0 && pnode->hwnd_sc && pnode->eusc)
        {   // 最后一个标签时, 保存scintilla窗口句柄
            inter_atom_exchange(&last_sci_hwnd, (sptr_t)pnode->hwnd_sc);
            inter_atom_exchange(&last_sci_eusc, (sptr_t)pnode->eusc);
        }
        else if (pnode->hwnd_sc)
        {   // 销毁scintilla窗口
            SendMessage(pnode->hwnd_sc, WM_CLOSE, 0, 0);
            inter_atom_exchange(&last_sci_hwnd, 0);
            inter_atom_exchange(&last_sci_eusc, 0);
        }
    }
}

void
on_sci_destroy_control(eu_tabpage *pnode)
{
    if (pnode)
    {   // 关闭数据库链接
        if (pnode->db_ptr)
        {
            on_table_disconnect_database(pnode, true);
        }
        if (pnode->redis_ptr)
        {
            on_symtree_disconnect_redis(pnode);
        }
        // 销毁控件句柄
        if (pnode->hwnd_symlist)
        {
            SendMessage(pnode->hwnd_symlist, WM_CLOSE, 0, 0);
            pnode->hwnd_symlist = NULL;
            pnode->sym_show = false;
        }
        else if (pnode->hwnd_symtree)
        {
            SendMessage(pnode->hwnd_symtree, WM_CLOSE, 0, 0);
            pnode->hwnd_symtree = NULL;
            pnode->sym_show = false;
        }
        if (pnode->hwnd_qrtable)
        {
            SendMessage(pnode->hwnd_qrtable, WM_CLOSE, 0, 0);
            pnode->hwnd_qrtable = NULL;
        }
        if (pnode->presult && pnode->presult->hwnd_sc)
        {
            SendMessage(pnode->presult->hwnd_sc, WM_CLOSE, 0, 0);
            pnode->presult->hwnd_sc = NULL;
            pnode->result_show = false;
            eu_safe_free(pnode->presult);
        }
        // 释放保存结果的vec数组
        if (pnode->ret_vec)
        {
            cvector_freep(&pnode->ret_vec);
        }
        if (pnode->ac_vec)
        {
            cvector_freep(&pnode->ac_vec);
        }
        if (pnode->re_group)
        {
            cvector_freep(&pnode->re_group);
        }
        // 关闭minimap窗口
        if (!on_tabpage_check_map() && hwnd_document_map)
        {
            DestroyWindow(hwnd_document_map);
            pnode->map_show = false;
        }
    }
}

void
on_sci_free_tab(eu_tabpage **ppnode)
{
    if (STR_NOT_NUL(ppnode))
    {
        util_lock(&(*ppnode)->busy_id);
        // 销毁子窗口资源
        on_sci_destroy_control(*ppnode);
        // 销毁插件窗口资源
        if ((*ppnode)->plugin)
        {
            np_plugins_destroy(&(*ppnode)->plugin->funcs, &(*ppnode)->plugin->npp, NULL);
            np_plugins_shutdown(&(*ppnode)->pmod, &(*ppnode)->plugin);
        }
        // 切换16进制时,销毁相关资源
        on_sci_swap_hwnd(*ppnode);
        // 关闭可能加载的动态库
        eu_close_dll((*ppnode)->pmod);
        // 清理缓存文件
        on_sci_delete_file(*ppnode);
        // 清除标签单次运行锁状态
        _InterlockedExchange(&(*ppnode)->lock_id, 0);
        util_unlock(&(*ppnode)->busy_id);
        eu_safe_free(*ppnode);
        eu_logmsg("%s: we free the node's memory\n", __FUNCTION__);
    }
}

void
on_sci_insert_egg(eu_tabpage *pnode)
{
    unsigned char represent[QW_SIZE] = \
                  {0x0a,0x2f,0x2f,0x20, 0x53,0x6b,0x79,0x6c,0x61,0x72,0x6B,0XE6,0X98,0xAF,0xe4,0xb8,0x80,0xe4,
                   0xb8,0xaa,0xe5,0xbe,0x88,0xe6,0xa3,0x92,0xe7,0x9a,0x84,0xe7,0xbc,0x96,0xe8,0xbe,0x91,0xe5,
                   0x99,0xa8,0xf0,0x9f,0x99,0x82,0x00};
    if (util_os_version() < 1000)
    {
        represent[strlen((const char *)represent) - 4] = 0;
        strncat((char *)represent, "\x5E\x30\x5E", QW_SIZE);
    }
    strncat((char *)represent, on_encoding_get_eol(pnode), QW_SIZE);
    sptr_t cur_pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
    eu_sci_call(pnode, SCI_INSERTTEXT, cur_pos, (LPARAM) represent);
    eu_sci_call(pnode, SCI_GOTOPOS, cur_pos + strlen((const char *)represent), 0);
}

char *
on_sci_range_text(eu_tabpage *pnode, sptr_t start, sptr_t end)
{
    char *text = NULL;
    if (pnode && (text = (char *) calloc(1, end - start + 1)))
    {
        Sci_TextRangeFull tr = {{start, end}, text};
        eu_sci_call(pnode, SCI_GETTEXTRANGEFULL, 0, (sptr_t) &tr);
    }
    return text;
}

bool
on_sci_line_text(eu_tabpage *pnode, size_t lineno, char *buf, size_t len)
{
    *buf = 0;
    if (pnode)
    {
        sptr_t start = eu_sci_call(pnode, SCI_POSITIONFROMLINE, lineno, 0);
        sptr_t end = eu_sci_call(pnode, SCI_GETLINEENDPOSITION, lineno, 0);
        if (end - start > (sptr_t)len)
        {
            return false;
        }
        else
        {
            Sci_TextRange tr = {{start, end}, buf};
            return eu_sci_call(pnode, SCI_GETTEXTRANGE, 0, (sptr_t) &tr) > 0;
        }
    }
    return (*buf != 0);
}

bool
on_sci_doc_modified(eu_tabpage *pnode)
{
    return (pnode ? pnode->be_modify : false);
}

int
on_sci_point_reached(eu_tabpage *pnode)
{
    if (pnode && g_tabpages)
    {
        if (!pnode->fn_modify)
        {
            pnode->be_modify = false;
        }
        on_toolbar_update_button();
        util_redraw(g_tabpages, false);
    }
    return SKYLARK_OK;
}

int
on_sci_point_left(eu_tabpage *pnode)
{
    if (pnode && g_tabpages)
    {
        if (!pnode->be_modify)
        {
            pnode->be_modify = true;
        }
        on_toolbar_update_button();
        util_redraw(g_tabpages, false);
    }
    return SKYLARK_OK;
}

void
on_sci_character(eu_tabpage *pnode, ptr_notify lpnotify)
{
    if (pnode && !pnode->hex_mode && pnode->doc_ptr && pnode->doc_ptr->fn_on_char)
    {
        pnode->doc_ptr->fn_on_char(pnode, lpnotify);
    }
}

void
on_sci_update_line_margin(eu_tabpage *pnode)
{
    EU_VERIFY(pnode != NULL);
    sptr_t m_width = eu_sci_call(pnode, SCI_GETMARGINWIDTHN, MARGIN_LINENUMBER_INDEX, 0);
    sptr_t m_line = eu_sci_call(pnode, SCI_GETLINECOUNT, 0, 0);
    char marg_width[DW_SIZE] = { 0 };
    int m_zoom = (int) eu_sci_call(pnode, SCI_GETZOOM, 0, 0);
    snprintf(marg_width, DW_SIZE - 1, "__%d", m_line);
    sptr_t cur_width = eu_sci_call(pnode, SCI_TEXTWIDTH, STYLE_LINENUMBER, (sptr_t) marg_width);
    if (cur_width != m_width)
    {
        eu_sci_call(pnode, SCI_SETMARGINWIDTHN, MARGIN_LINENUMBER_INDEX, (eu_get_config()->m_linenumber ? cur_width + m_zoom : 0));
    }
}

static void
on_sci_menu_callback(HMENU hpop, void *param)
{
    eu_tabpage *p = (eu_tabpage *)param;
    if (p && hpop)
    {
        util_enable_menu_item(hpop, IDM_EDIT_CUT, util_can_selections(p));
        util_enable_menu_item(hpop, IDM_EDIT_COPY, !p->pmod && TAB_NOT_NUL(p));
        util_enable_menu_item(hpop, IDM_EDIT_PASTE, eu_sci_call(p, SCI_CANPASTE, 0, 0));
    }
}

LRESULT CALLBACK
sc_edit_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    eu_tabpage *pnode = NULL;
    switch (message)
    {
        case WM_KEYDOWN:
        {   // 按下ESC键时
            if (!(pnode = on_tabpage_focus_at()))
            {
                break;
            }
            if ((wParam == VK_ESCAPE || KEY_DOWN(VK_ESCAPE)))
            {
                sptr_t total_len = eu_sci_call(pnode, SCI_GETLENGTH, 0, 0);
                sptr_t cur_pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
                eu_sci_call(pnode, SCI_INDICATORCLEARRANGE, 0, total_len);
                eu_sci_call(pnode, SCI_SETEMPTYSELECTION, cur_pos, 0);
                if (pnode->zoom_level == SELECTION_ZOOM_LEVEEL)
                {
                    on_view_zoom_reset(pnode);
                    pnode->zoom_level = 0;
                }
                if (pnode->ac_vec && (!(eu_sci_call(pnode, SCI_AUTOCACTIVE, 0, 0) || eu_sci_call(pnode, SCI_CALLTIPACTIVE, 0, 0))))
                {
                    on_complete_reset_focus(pnode);
                }
                eu_sci_call(pnode, SCI_CANCEL, 0, 0);
            }
            else if (pnode->map_show && document_map_initialized)
            {
                if (wParam == VK_UP)
                {
                    SendMessage(hwnd_document_map, DOCUMENTMAP_SCROLL, (WPARAM)MOVE_UP, 0);
                }
                if (wParam == VK_DOWN)
                {
                    SendMessage(hwnd_document_map, DOCUMENTMAP_SCROLL, (WPARAM)MOVE_DOWN, 0);
                }
                if (wParam == VK_PRIOR)
                {
                    SendMessage(hwnd_document_map, DOCUMENTMAP_SCROLL, (WPARAM)MOVE_UP, 1);
                }
                if (wParam == VK_NEXT)
                {
                    SendMessage(hwnd_document_map, DOCUMENTMAP_SCROLL, (WPARAM)MOVE_DOWN, 1);
                }
            }
            break;
        }
        case WM_KEYUP:
        {
            if (KEY_DOWN(VK_CONTROL) || KEY_DOWN(VK_MENU))
            {
                break;
            }
            if (wParam == VK_CONTROL || wParam == VK_MENU)
            {
                break;
            }
            if ((pnode = on_tabpage_focus_at()) == NULL)
            {
                break;
            }
            if (pnode->doc_ptr && pnode->doc_ptr->fn_keyup)
            {
                if (!(lParam & (1 << 24)))
                {
                    pnode->doc_ptr->fn_keyup(pnode, wParam, lParam);
                }
            }
            on_search_update_navigate_list(pnode, eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0));
            on_statusbar_update_line(pnode);
            break;
        }
        case WM_LBUTTONUP:
        {
            if ((pnode = on_tabpage_focus_at()) == NULL)
            {
                break;
            }
            if (!(wParam & 0xff))
            {
                on_doc_brace_light(pnode, false);
            }
            if (pnode->ac_vec)
            {
                on_complete_reset_focus(pnode);
            }
            pnode->nc_pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
            on_search_add_navigate_list(pnode, pnode->nc_pos);
            on_statusbar_update_line(pnode);
            break;
        }
        case WM_RBUTTONUP:
        {
            if ((pnode = on_tabpage_get_handle(hwnd)) != NULL)
            {
                return menu_pop_track(hwnd, IDR_EDITOR_POPUPMENU, 0, 0, on_sci_menu_callback, pnode);
            }
            return 1;
        }
        case WM_COMMAND:
        {
            PostMessage(eu_module_hwnd(), WM_COMMAND, wParam, lParam);
            break;
        }
        case WM_THEMECHANGED:
        {
            eu_logmsg("scintilla WM_THEMECHANGED\n");
            if (eu_get_config()->m_toolbar != IDB_SIZE_0)
            {
                on_toolbar_update_button();
            }
            break;
        }
        case WM_DPICHANGED:
        {
            if ((pnode = (eu_tabpage *) lParam) != NULL)
            {
                eu_logmsg("scintilla WM_DPICHANGED\n");
            }
            break;
        }
        case WM_DPICHANGED_AFTERPARENT:
        {
            if ((pnode = on_tabpage_get_handle(hwnd)) != NULL)
            {
                eu_logmsg("scintilla WM_DPICHANGED_AFTERPARENT\n");
                on_sci_update_line_margin(pnode);
            }
            break;
        }
        case WM_SETFOCUS:
        {
            NMHDR nm = {0};
            eu_send_notify(hwnd, NM_SETFOCUS, &nm);
            break;
        }
        case WM_DESTROY:
        {
            eu_logmsg("scintilla WM_DESTROY\n");
            break;
        }
        default:
            break;
    }
    return CallWindowProc((WNDPROC)eu_edit_wnd, hwnd, message, wParam, lParam);
}

void
on_sci_send_extra(void *pdata, uint32_t code, LPNMHDR phdr)
{
    phdr->hwndFrom = (HWND)pdata;
    phdr->code = code;
    SendMessage(eu_module_hwnd(), WM_NOTIFY, 0, (LPARAM) phdr);
}

void
on_sic_mousewheel(eu_tabpage *pnode, WPARAM wParam, LPARAM lParam)
{
    sc_edit_proc(pnode->hwnd_sc, WM_MOUSEWHEEL, wParam, lParam);
}

int
on_sci_create(eu_tabpage *pnode, HWND parent, int flags, WNDPROC sc_callback)
{
    EU_VERIFY(pnode != NULL);
    int exflags = flags ? flags : WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_EX_RTLREADING;
    pnode->hwnd_sc = CreateWindowEx(0, TEXT("Scintilla"), TEXT(""), exflags, 0, 0, 0, 0, parent ? parent : eu_module_hwnd(), 0, eu_module_handle(), 0);
    if (pnode->hwnd_sc == NULL)
    {
        MSG_BOX(IDC_MSG_SCINTILLA_ERR1, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
        return 1;
    }
    if (inter_atom_compare_exchange(&eu_edit_wnd, SetWindowLongPtr(pnode->hwnd_sc, GWLP_WNDPROC, sc_callback ? (LONG_PTR)sc_callback : (LONG_PTR)sc_edit_proc), 0))
    {
        SetWindowLongPtr(pnode->hwnd_sc, GWLP_WNDPROC, sc_callback ? (LONG_PTR)sc_callback : (LONG_PTR)sc_edit_proc);
    }
    if (!inter_atom_compare_exchange(&ptr_scintilla, SendMessage(pnode->hwnd_sc, SCI_GETDIRECTFUNCTION, 0, 0), 0));
    pnode->eusc = SendMessage(pnode->hwnd_sc, SCI_GETDIRECTPOINTER, 0, 0);
    eu_sci_call(pnode, SCI_USEPOPUP, 0, 0);
    return SKYLARK_OK;
}

int
on_sci_init_dlg(eu_tabpage *pnode)
{
    if (!pnode)
    {
        return EUE_POINT_NULL;
    }
    if (last_sci_hwnd)
    {
        pnode->hwnd_sc = (HWND)last_sci_hwnd;
        pnode->eusc = last_sci_eusc;
        inter_atom_exchange(&last_sci_hwnd, 0);
        inter_atom_exchange(&last_sci_eusc, 0);
        return SKYLARK_OK;
    }
    return on_sci_create(pnode, NULL, !pnode->hex_mode && pnode->pmod ? WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_EX_RTLREADING : 0, NULL);
}

void
eu_send_notify(HWND hwnd, uint32_t code, LPNMHDR phdr)
{
    phdr->hwndFrom = hwnd;
    phdr->code = code;
    SendMessage(GetParent(hwnd), WM_NOTIFY, 0, (LPARAM) phdr);
}

int
eu_sci_release(void)
{
    return Scintilla_ReleaseResources();
}

sptr_t
eu_sci_call(eu_tabpage *p, int m, sptr_t w, sptr_t l)
{
    return ((p && p->hwnd_sc) ?
            (p->hex_mode ? SendMessage(p->hwnd_sc, m, w, l) :
            (ptr_scintilla && p->eusc) ? ((SciFnDirect)ptr_scintilla)(p->eusc, m, w, l) : 0) :
            0);
}
