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

const int
on_sci_bitmask_get(const uint32_t pos, const uint32_t len)
{
    const unsigned long long _zone = 0;
    return (const int)(~(~((int)_zone) << (len)) << (pos));
}

void
on_sci_set_margin(eu_tabpage *pnode)
{
    if (pnode)
    {
        eu_sci_call(pnode, SCI_SETMARGINS, 1, 0);
        eu_sci_call(pnode, SCI_SETMARGINWIDTHN, 0, MARGIN_FOLD_WIDTH);
        eu_sci_call(pnode, SCI_SETMARGINTYPEN, 0, SC_MARGIN_SYMBOL);
        eu_sci_call(pnode, SCI_SETMARGINMASKN, 0, SC_MASK_FOLDERS);
        eu_sci_call(pnode, SCI_SETFOLDMARGINCOLOUR, true, eu_get_theme()->item.text.bgcolor);
        eu_sci_call(pnode, SCI_SETFOLDMARGINHICOLOUR, true, eu_get_theme()->item.text.bgcolor);
    }
}

void
on_sci_default_fonts(eu_tabpage *pnode, const uint32_t bgcolor)
{
    if (pnode)
    {
        eu_sci_call(pnode, SCI_STYLERESETDEFAULT, 0, 0);
        eu_sci_call(pnode, SCI_STYLESETFONT, STYLE_DEFAULT, eu_doc_get_font_name(pnode));
        eu_sci_call(pnode, SCI_STYLESETSIZE, STYLE_DEFAULT, eu_doc_get_font_size(pnode));
        eu_sci_call(pnode, SCI_STYLESETFORE, STYLE_DEFAULT, eu_get_theme()->item.text.color);
        eu_sci_call(pnode, SCI_STYLESETBACK, STYLE_DEFAULT, bgcolor != (uint32_t)-1 ? bgcolor : eu_get_theme()->item.text.bgcolor);
        eu_sci_call(pnode, SCI_STYLESETBOLD, STYLE_DEFAULT, eu_get_theme()->item.text.bold);
        eu_sci_call(pnode, SCI_STYLECLEARALL, 0, 0);
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
    }
}

void
on_sci_default_theme(eu_tabpage *pnode, const uint32_t bgcolor)
{
    if (pnode)
    {   // 编辑区样式与字体设置
        on_sci_default_fonts(pnode, bgcolor);
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
        const sptr_t bgra = eu_get_theme()->item.indicator.bgcolor;
        eu_sci_call(pnode, SCI_SETSELECTIONLAYER, SC_LAYER_OVER_TEXT, 0);
        eu_sci_call(pnode, SCI_SETELEMENTCOLOUR, SC_ELEMENT_SELECTION_BACK, bgra);
        // 其他选中
        eu_sci_call(pnode, SCI_SETELEMENTCOLOUR, SC_ELEMENT_SELECTION_ADDITIONAL_BACK, bgra);
        eu_sci_call(pnode, SCI_SETELEMENTCOLOUR, SC_ELEMENT_SELECTION_SECONDARY_BACK, bgra);
        // 窗口未激活时选中色
        eu_sci_call(pnode, SCI_SETELEMENTCOLOUR, SC_ELEMENT_SELECTION_INACTIVE_BACK, bgra);
    }
}

void
on_sci_init_default(eu_tabpage *pnode, const uint32_t bgcolor)
{
    if (pnode && eu_get_config() && eu_get_theme())
    {
        on_sci_default_theme(pnode, bgcolor);
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
        // 自动补全窗口颜色
        eu_sci_call(pnode, SCI_SETELEMENTCOLOUR, SC_ELEMENT_LIST, eu_get_theme()->item.text.color);
        eu_sci_call(pnode, SCI_SETELEMENTCOLOUR, SC_ELEMENT_LIST_BACK, eu_get_theme()->item.text.bgcolor);
        eu_sci_call(pnode, SCI_SETELEMENTCOLOUR, SC_ELEMENT_LIST_SELECTED_BACK, eu_get_theme()->item.indicator.bgcolor);
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
        eu_sci_call(pnode, SCI_SETMARGINSENSITIVEN, MARGIN_HISTORY_INDEX, FALSE);
        eu_sci_call(pnode, SCI_SETMARGINTYPEN, MARGIN_HISTORY_INDEX, SC_MARGIN_SYMBOL);
        eu_sci_call(pnode, SCI_SETMARGINMASKN, MARGIN_HISTORY_INDEX, MARGIN_HISTORY_MASKN);
        eu_sci_call(pnode, SCI_MARKERSETFORE, SC_MARKNUM_HISTORY_MODIFIED, eu_get_theme()->item.nchistory.color);
        eu_sci_call(pnode, SCI_MARKERSETBACK, SC_MARKNUM_HISTORY_MODIFIED, eu_get_theme()->item.nchistory.color);
        eu_sci_call(pnode, SCI_MARKERSETFORE, SC_MARKNUM_HISTORY_SAVED, eu_get_theme()->item.nchistory.bgcolor);
        eu_sci_call(pnode, SCI_MARKERSETBACK, SC_MARKNUM_HISTORY_MODIFIED, eu_get_theme()->item.nchistory.bgcolor);
        eu_sci_call(pnode, SCI_INDICSETFORE, INDICATOR_HISTORY_MODIFIED_INSERTION, eu_get_theme()->item.dochistory.color);
        eu_sci_call(pnode, SCI_INDICSETFORE, INDICATOR_HISTORY_SAVED_INSERTION, eu_get_theme()->item.dochistory.bgcolor);
        // 代码折叠栏亮量颜色与填充色
        eu_sci_call(pnode, SCI_SETFOLDMARGINCOLOUR, true, eu_get_theme()->item.foldmargin.bgcolor);
        eu_sci_call(pnode, SCI_SETFOLDMARGINHICOLOUR, true, eu_get_theme()->item.foldmargin.bgcolor);
        eu_sci_call(pnode, SCI_MARKERSETFORETRANSLUCENT, SC_MARKNUM_FOLDER, eu_get_theme()->item.foldmargin.color);
        eu_sci_call(pnode, SCI_MARKERSETFORETRANSLUCENT, SC_MARKNUM_FOLDEREND, eu_get_theme()->item.foldmargin.color);
        eu_sci_call(pnode, SCI_MARKERSETFORETRANSLUCENT, SC_MARKNUM_FOLDEROPEN, eu_get_theme()->item.foldmargin.color);
        eu_sci_call(pnode, SCI_MARKERSETFORETRANSLUCENT, SC_MARKNUM_FOLDEROPENMID, eu_get_theme()->item.foldmargin.color);
        // 是否自动换行
        eu_sci_call(pnode, SCI_SETWRAPMODE, (eu_get_config()->line_mode ? SC_WRAP_CHAR : SC_WRAP_NONE), 0);
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
        // 需要时显示水平滚动条, 但是删除文本后, 滚动条不会消失
        eu_sci_call(pnode, SCI_SETSCROLLWIDTH, 1, 0);
        eu_sci_call(pnode, SCI_SETSCROLLWIDTHTRACKING, 1, 0);
        // 设置undo掩码, 接受SCN_MODIFIED消息
        eu_sci_call(pnode, SCI_SETMODEVENTMASK, SC_MOD_INSERTTEXT|SC_MOD_DELETETEXT|SC_PERFORMED_UNDO|SC_PERFORMED_REDO, 0);
        // 支持多列粘贴
        eu_sci_call(pnode, SCI_SETMULTIPASTE, SC_MULTIPASTE_EACH, 0);
        // 设置括号匹配高亮色以及指示出不匹配的大括号
        eu_sci_call(pnode, SCI_STYLESETFORE, STYLE_BRACELIGHT, eu_get_theme()->item.bracesection.color);
        eu_sci_call(pnode, SCI_STYLESETBACK, STYLE_BRACELIGHT, eu_get_theme()->item.bracesection.bgcolor);
        eu_sci_call(pnode, SCI_STYLESETBOLD, STYLE_BRACELIGHT, eu_get_theme()->item.bracesection.bold);
        eu_sci_call(pnode, SCI_STYLESETITALIC, STYLE_BRACEBAD, true);
        eu_sci_call(pnode, SCI_STYLESETUNDERLINE, STYLE_BRACEBAD, true);
        eu_sci_call(pnode, SCI_BRACEBADLIGHTINDICATOR, true, INDIC_STRIKE);
        // 注册补全列表图标
        eu_sci_call(pnode, SCI_REGISTERIMAGE, SNIPPET_FUNID, (sptr_t)auto_xpm);
    }
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

static inline void
on_sci_update_size(eu_tabpage *pnode)
{
    if (pnode)
    {
        pnode->raw_size = eu_sci_call(pnode, SCI_GETLENGTH, 0, 0) + pnode->pre_len;    
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
    }
}

void
on_sci_after_file(eu_tabpage *pnode, const bool init)
{
    if (pnode)
    {
        if (TAB_HAS_TXT(pnode))
        {
            eu_sci_call(pnode, SCI_SETEOLMODE, pnode->eol, 0);
            if (init)
            {
                eu_sci_call(pnode, SCI_SETUNDOCOLLECTION, 1, 0);
                on_sci_clear_history(pnode, false);
                on_sci_update_history_margin(pnode);
            }
            if (!pnode->raw_size)
            {
                on_sci_update_size(pnode);
            }
            if (pnode->doc_ptr && pnode->doc_ptr->fn_init_after)
            {
                if (on_proc_thread() == GetCurrentThreadId())
                {   // 主线程加载解析器
                    pnode->doc_ptr->fn_init_after(pnode);
                }
                else 
                {   // 线程加载lua脚本导致crash, 在主线程加载
                    SendMessage(eu_hwnd_self(), WM_SCI_LEXER, (sptr_t)pnode, 0);
                }
            }
            on_sci_update_line_margin(pnode);
        }
        if (!pnode->plugin)
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
        on_toolbar_update_button(pnode);
        on_statusbar_update(pnode);
        on_sci_update_line_margin(pnode);
        on_sci_update_fold_margin(pnode);
        on_sci_update_size(pnode);
        util_redraw(on_tabpage_hwnd(pnode), true);
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
on_sci_destroy_hwnd(eu_tabpage *pnode)
{
    if (pnode && pnode->hwnd_sc)
    {
        SendMessage(pnode->hwnd_sc, WM_CLOSE, 0, 0);
    }
}

void
on_sci_destroy_control(eu_tabpage *pnode)
{
    if (pnode)
    {
        HWND hmap = NULL;
        // 关闭数据库链接
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
        if (pnode->presult)
        {
            on_result_free(&pnode->presult);
            pnode->result_show = false;
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
        if (!on_tabpage_exist_map())
        {
            if ((hmap = on_map_hwnd()))
            {
                SendMessage(hmap, WM_CLOSE, 0, 0);
                pnode->map_show = false;
            }
        }
    }
}

void
on_sci_free_tab(eu_tabpage **ppnode)
{
    if (STR_NOT_NUL(ppnode))
    {
        int reason = (*ppnode)->reason;
        util_lock_v2((*ppnode));
        // 销毁子窗口资源
        on_sci_destroy_control(*ppnode);
        // 销毁插件窗口资源
        if ((*ppnode)->plugin)
        {
            np_plugins_destroy(&(*ppnode)->plugin->funcs, &(*ppnode)->plugin->npp, NULL);
            np_plugins_shutdown(&(*ppnode)->pmod, &(*ppnode)->plugin);
        }
        // 销毁编辑器窗口
        if (reason != TABS_MAYBE_RESERVE)
        {
            on_sci_destroy_hwnd(*ppnode);
        }
        // 关闭可能加载的动态库
        eu_close_dll((*ppnode)->pmod);
        // 清理缓存文件
        on_sci_delete_file(*ppnode);
        // 清除标签单次运行锁状态
        _InterlockedExchange(&(*ppnode)->lock_id, 0);
        util_unlock_v2((*ppnode));
        if (reason != TABS_MAYBE_RESERVE)
        {
            eu_safe_free(*ppnode);
            eu_logmsg("%s: we free the node's memory\n", __FUNCTION__);
        }
        else 
        {   // 存在复制标签时, 不复用hwnd_sc
            const bool stat = ((*ppnode)->stat_id & TABS_DUPED) || ((*ppnode)->stat_id & TABS_MAIN);
            (*ppnode)->reason = 0;
            on_file_new(on_tabpage_hwnd(*ppnode), stat ? NULL : (*ppnode));
            eu_logmsg("%s: on_file_new() execute\n", __FUNCTION__);
        }
        if (reason == TABS_MAYBE_EIXT && on_sql_sync_session() == SKYLARK_OK)
        {
            eu_logmsg("close last tab, skylark exit ...\n");
            SendMessage(eu_module_hwnd(), WM_BACKUP_OVER, 0, 0);
        }
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
    HWND htab = on_tabpage_hwnd(pnode);
    if (htab && pnode)
    {
        if (!pnode->fn_modify)
        {
            pnode->be_modify = false;
        }
        on_toolbar_update_button(pnode);
        util_redraw(htab, false);
    }
    return SKYLARK_OK;
}

int
on_sci_point_left(eu_tabpage *pnode)
{
    HWND htab = on_tabpage_hwnd(pnode);
    if (htab && pnode)
    {
        if (!pnode->be_modify)
        {
            pnode->be_modify = true;
        }
        on_toolbar_update_button(pnode);
        util_redraw(htab, false);
    }
    return SKYLARK_OK;
}

void
on_sci_character(eu_tabpage *pnode, ptr_notify lpnotify)
{
    if (pnode && !TAB_HEX_MODE(pnode) && pnode->doc_ptr && pnode->doc_ptr->fn_on_char)
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

void
on_sci_update_fold_margin(eu_tabpage *pnode)
{
    if (pnode && !TAB_HEX_MODE(pnode) && !pnode->plugin)
    {
        const int zoom = (const int) eu_sci_call(pnode, SCI_GETZOOM, 0, 0);
        const int scalex = eu_get_dpi(eu_hwnd_self()) * zoom;
        const int scaley = eu_dpi_scale_xy(0, MARGIN_FOLD_WIDTH) + zoom;
        const int fold_marks[] =
        {
            SC_MARKNUM_FOLDER,
            SC_MARKNUM_FOLDEROPEN,
            SC_MARKNUM_FOLDERSUB,
            SC_MARKNUM_FOLDERTAIL,
            SC_MARKNUM_FOLDEREND,
            SC_MARKNUM_FOLDEROPENMID,
            SC_MARKNUM_FOLDERMIDTAIL
        };
        const int mstroke = eu_dpi_scale_style(100, scalex, 100);
        eu_sci_call(pnode, SCI_SETMARGINWIDTHN, MARGIN_FOLD_INDEX, (eu_get_config()->block_fold ? scaley : 0));
        for (int i = 0; i < _countof(fold_marks); ++i)
        {
            eu_sci_call(pnode, SCI_MARKERSETSTROKEWIDTH, fold_marks[i], mstroke);
        }
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

static bool
on_sci_parser_line(eu_tabpage *p, const sptr_t current_line, const sptr_t last_line, const long current_y, char ***vec_buf, int *pcount, int *pmax, bool *ward)
{
    if (p && vec_buf && pcount && pmax && ward)
    {
        int last = 0;
        size_t len = 0;
        const sptr_t end_y = (const sptr_t )(p->rect_sc.bottom - p->rect_sc.top);
        const sptr_t font_hight = eu_sci_call(p, SCI_TEXTHEIGHT, 0, 0);
        const int tab_width = (const int)eu_sci_call(p, SCI_GETTABWIDTH, 0, 0);
        const int w_count = (const int)(last_line - current_line + 1);
        // 确认代码提示向上还是向下
        if (w_count <= eu_int_cast((end_y - current_y)/font_hight - 1))
        {
            *ward = true;
        }
        else if (w_count <= eu_int_cast(current_y/font_hight - 1))
        {
            *ward = false;
        }
        else if (end_y -  current_y >= current_y)
        {
            *ward = true;
        }
        else
        {
            *ward = false;
        }
        if (true)
        {
            *pmax = 0;
            *pcount = MIN(w_count, *ward ? eu_int_cast((end_y - current_y)/font_hight - 3) : eu_int_cast(current_y/font_hight - 3));
        }
        for (sptr_t i = current_line; i <= last_line && last < *pcount; ++i, ++last)
        {   // clang, 整型指针转size_t指针导致__fastfail?
            char *buffer = util_strdup_line(p, i, &len);
            if (buffer)
            {   // 把tab键占用的字符数算进去
                int j = util_count_characters(buffer, '\t');
                if (j > 0 && tab_width > 0)
                {
                    j *= (tab_width - 1);
                    len += (size_t)j;
                }
                cvector_push_back(*vec_buf, buffer);
            }
            if (len > (size_t)*pmax)
            {
                *pmax = eu_int_cast(len);
            }
        }
        if (cvector_size(*vec_buf) > 0)
        {
            if (w_count > *pcount)
            {
                cvector_push_back(*vec_buf, _strdup("..."));
            }
            *pcount = (int)cvector_size(*vec_buf);
        }
        return true;
    }
    return false;
}

LRESULT CALLBACK
sc_edit_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    eu_tabpage *pnode = NULL;
    switch (message)
    {
        case WM_KEYDOWN:
        {   // 按下ESC键时
            HWND hmap = NULL;
            if (!(pnode = on_tabpage_from_handle(hwnd, on_tabpage_sci)))
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
            else if (pnode->map_show && (hmap = on_map_hwnd()))
            {
                if (wParam == VK_UP)
                {
                    SendMessage(hmap, DOCUMENTMAP_SCROLL, (WPARAM)MOVE_UP, 0);
                }
                if (wParam == VK_DOWN)
                {
                    SendMessage(hmap, DOCUMENTMAP_SCROLL, (WPARAM)MOVE_DOWN, 0);
                }
                if (wParam == VK_PRIOR)
                {
                    SendMessage(hmap, DOCUMENTMAP_SCROLL, (WPARAM)MOVE_UP, 1);
                }
                if (wParam == VK_NEXT)
                {
                    SendMessage(hmap, DOCUMENTMAP_SCROLL, (WPARAM)MOVE_DOWN, 1);
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
            if ((pnode = on_tabpage_from_handle(hwnd, on_tabpage_sci)) == NULL)
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
        case WM_MOUSEMOVE:
        {
            if ((pnode = on_tabpage_from_handle(hwnd, on_tabpage_sci)) != NULL && eu_get_config()->m_code_hint)
            {
                POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                TRACKMOUSEEVENT event = {sizeof(TRACKMOUSEEVENT), TME_HOVER, hwnd, HOVER_DEFAULT};
                TrackMouseEvent(&event);
                on_hint_hide(&pt);
            }
            break;
        }
        case WM_MOUSEHOVER:
        {
            if ((pnode = on_tabpage_from_handle(hwnd, on_tabpage_sci)) != NULL)
            {
                const POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                const sptr_t current_pos = eu_sci_call(pnode, SCI_POSITIONFROMPOINT, pt.x, pt.y);
                const sptr_t current_line = eu_sci_call(pnode, SCI_LINEFROMPOSITION, current_pos, 0);
                const bool status = (const bool)eu_sci_call(pnode, SCI_GETFOLDEXPANDED, current_line, 0);
                if (!status)
                {
                    const long offset = (const long)eu_dpi_scale_xy(0, 8);
                    const sptr_t last_line = eu_sci_call(pnode, SCI_GETLASTCHILD, current_line, -1);
                    const sptr_t current_header = eu_sci_call(pnode, SCI_POSITIONFROMLINE, current_line, 0);
                    const long lx = (const long)eu_sci_call(pnode, SCI_POINTXFROMPOSITION, 0, current_pos) + offset;
                    const long ly = (const long)eu_sci_call(pnode, SCI_POINTYFROMPOSITION, 0, current_pos);
                    const RECT rc = {lx, ly, lx + eu_dpi_scale_xy(0, 26), ly + (eu_dpi_scale_xy, 0, 18)};
                    pnode->reserved1 = eu_sci_call(pnode, SCI_POINTXFROMPOSITION, 0, current_header) + offset;
                    pnode->zoom_level = (int) eu_sci_call(pnode, SCI_GETZOOM, 0, 0);
                    if (PtInRect(&rc, pt) && on_hint_initialized())
                    {   // 在折叠框内
                        int line_max = 0;
                        int line_count = 0;
                        bool downward = false;
                        cvector_vector_type(char *) buf = NULL;
                        if (on_sci_parser_line(pnode, current_line, last_line, ly, &buf, &line_count, &line_max, &downward))
                        {
                            on_hint_launch(pnode, &rc, buf, line_count, line_max, downward);
                        }
                        if (cvector_size(buf) > 0)
                        {
                            cvector_free_each_and_free(buf, free);
                        }
                    }
                }
            }
            break;
        }
        case WM_LBUTTONDOWN:
        {
            if (HSLAVE_SHOW && (pnode = on_tabpage_from_handle(hwnd, on_tabpage_sci)) && !(pnode->stat_id & TABS_FOUCED))
            {
                on_tabpage_active_tab(pnode);
            }
            break;
        }
        case WM_LBUTTONUP:
        {
            if ((pnode = on_tabpage_from_handle(hwnd, on_tabpage_sci)) == NULL)
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
            if ((pnode = on_tabpage_from_handle(hwnd, on_tabpage_sci)) != NULL)
            {
                return menu_pop_track(hwnd, IDR_EDITOR_POPUPMENU, 0, 0, on_sci_menu_callback, pnode);
            }
            return 1;
        }
        case WM_COMMAND:
        {
            SendMessage(eu_hwnd_self(), WM_COMMAND, wParam, lParam);
            break;
        }
        case WM_THEMECHANGED:
        {
            eu_logmsg("scintilla WM_THEMECHANGED\n");
            if (eu_get_config()->m_toolbar != IDB_SIZE_0)
            {
                on_toolbar_update_button(NULL);
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
            if ((pnode = on_tabpage_from_handle(hwnd, on_tabpage_sci)) != NULL)
            {
                eu_logmsg("scintilla WM_DPICHANGED_AFTERPARENT\n");
                on_sci_update_line_margin(pnode);
                on_sci_update_fold_margin(pnode);
            }
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

bool
on_sci_view_sync(void)
{
    return (eu_get_config() ? eu_get_config()->eu_tab.slave_show &&
           (eu_get_config()->eu_tab.vertical || eu_get_config()->eu_tab.horizontal) : false);
}

void
on_sci_scroll(eu_tabpage *p)
{
    HWND htab = NULL, hsci = NULL;
    if ((htab = on_tabpage_hwnd(p)) && (hsci = on_tabpage_sci(htab == HMAIN_GET ? HSLAVE_GET : HMAIN_GET)))
    {
        eu_tabpage *psub = NULL;
        int pos1 = 0, pos2 = 0;
        int vmin = 0, vmax = 0;
        int hmin = 0, hmax = 0;
        const uint32_t ex1 = (const uint32_t)GetWindowLongPtr(p->hwnd_sc, GWL_STYLE);
        const uint32_t ex2 = (const uint32_t)GetWindowLongPtr(hsci, GWL_STYLE);
        if (eu_get_config()->eu_tab.vertical && (ex1 & WS_VSCROLL) && (ex2 & WS_VSCROLL) && GetScrollRange(hsci, SB_VERT, &vmin, &vmax))
        {
            psub = (eu_tabpage *)eu_tabpage_from_handle(hsci);
            pos1 = GetScrollPos(p->hwnd_sc, SB_VERT);
            pos2 = GetScrollPos(hsci, SB_VERT);
            if (pos1 >= 0 && vmin >= 0 && pos1 != pos2)
            {
                if (TAB_HAS_TXT(psub))
                {
                    SetScrollPos(hsci, SB_VERT, (pos2 = pos1 <= vmax ? pos1 : vmax), TRUE);
                    CallWindowProc((WNDPROC)eu_edit_wnd, hsci, WM_VSCROLL, MAKEWPARAM(SB_THUMBPOSITION, pos2), 0);    
                }
                else if (TAB_HEX_MODE(psub))
                {
                    SetScrollPos(hsci, SB_VERT, (pos2 = pos1 <= vmax ? pos1 : vmax), TRUE);
                    SendMessage(hsci, WM_VSCROLL, MAKEWPARAM(SB_THUMBPOSITION, pos2), 0);     
                }
                else if (TAB_HAS_PDF(psub))
                {
                    eu_logmsg("not support\n");
                }
            }
        }
        if (eu_get_config()->eu_tab.horizontal && (ex1 & WS_HSCROLL) && (ex2 & WS_HSCROLL) && GetScrollRange(hsci, SB_HORZ, &hmin, &hmax))
        {
            psub = (eu_tabpage *)eu_tabpage_from_handle(hsci);
            pos1 = GetScrollPos(p->hwnd_sc, SB_HORZ);
            pos2 = GetScrollPos(hsci, SB_HORZ);
            if (pos1 >= 0 && hmin >= 0 && pos1 != pos2)
            {
                if (TAB_HAS_TXT(psub))
                {
                    SetScrollPos(hsci, SB_HORZ, (pos2 = pos1 <= hmax ? pos1 : hmax), TRUE);
                    CallWindowProc((WNDPROC)eu_edit_wnd, hsci, WM_HSCROLL, MAKEWPARAM(SB_THUMBPOSITION, pos2), 0);
                }
                else if (TAB_HEX_MODE(psub))
                {
                    SetScrollPos(hsci, SB_HORZ, (pos2 = pos1 <= hmax ? pos1 : hmax), TRUE);
                    SendMessage(hsci, SB_HORZ, MAKEWPARAM(SB_THUMBPOSITION, pos2), 0);     
                }
                else if (TAB_HAS_PDF(psub))
                {
                    eu_logmsg("not support\n");
                }
            }
        }
    }
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
    if (pnode)
    {
        return on_sci_create(pnode, NULL, !TAB_HEX_MODE(pnode) && pnode->pmod ? WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_EX_RTLREADING : 0, NULL);
    }
    return EUE_POINT_NULL;
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
            (TAB_HEX_MODE(p) ? SendMessage(p->hwnd_sc, m, w, l) :
            (ptr_scintilla && p->eusc) ? ((SciFnDirect)ptr_scintilla)(p->eusc, m, w, l) : 0) :
            0);
}
