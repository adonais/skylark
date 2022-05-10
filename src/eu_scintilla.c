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

static WNDPROC sc_edit_wnd = NULL;
static volatile sptr_t ptr_scintilla = 0;

int
on_sci_init_style(eu_tabpage *pnode)
{
    EU_VERIFY(pnode != NULL);
    eu_sci_call(pnode, SCI_STYLERESETDEFAULT, 0, 0);
    eu_sci_call(pnode, SCI_STYLESETFONT, STYLE_DEFAULT, (sptr_t)(eu_get_theme()->item.text.font));
    eu_sci_call(pnode, SCI_STYLESETSIZE, STYLE_DEFAULT, eu_get_theme()->item.text.fontsize);
    eu_sci_call(pnode, SCI_STYLESETFORE, STYLE_DEFAULT, eu_get_theme()->item.text.color);
    eu_sci_call(pnode, SCI_STYLESETBACK, STYLE_DEFAULT, eu_get_theme()->item.text.bgcolor);
    eu_sci_call(pnode, SCI_STYLESETBOLD, STYLE_DEFAULT, eu_get_theme()->item.text.bold);
    eu_sci_call(pnode, SCI_STYLECLEARALL, 0, 0);
    eu_sci_call(pnode, SCI_SETMARGINS, 3, 0);
    eu_sci_call(pnode, SCI_SETMARGINTYPEN, MARGIN_LINENUMBER_INDEX, SC_MARGIN_NUMBER);
    eu_sci_call(pnode, SCI_STYLESETFONT, STYLE_LINENUMBER, (sptr_t)(eu_get_theme()->item.linenumber.font));
    eu_sci_call(pnode, SCI_STYLESETSIZE, STYLE_LINENUMBER, eu_get_theme()->item.linenumber.fontsize);
    eu_sci_call(pnode, SCI_STYLESETFORE, STYLE_LINENUMBER, eu_get_theme()->item.linenumber.color);
    eu_sci_call(pnode, SCI_STYLESETBACK, STYLE_LINENUMBER, eu_get_theme()->item.linenumber.bgcolor);
    if (eu_get_config()->m_linenumber)
    {
        eu_sci_call(pnode, SCI_SETMARGINWIDTHN, MARGIN_LINENUMBER_INDEX, MARGIN_LINENUMBER_WIDTH);
    }
    else
    {
        eu_sci_call(pnode, SCI_SETMARGINWIDTHN, MARGIN_LINENUMBER_INDEX, 0);
    }
    // 书签栏样式
    eu_sci_call(pnode, SCI_SETMARGINSENSITIVEN, MARGIN_BOOKMARK_INDEX, TRUE);
    eu_sci_call(pnode, SCI_SETMARGINWIDTHN, MARGIN_BOOKMARK_INDEX, (eu_get_config()->bookmark_visable ? MARGIN_BOOKMARK_WIDTH : 0));
    eu_sci_call(pnode, SCI_MARKERDEFINE, MARGIN_BOOKMARK_VALUE, eu_get_config()->bookmark_shape);
    eu_sci_call(pnode, SCI_MARKERSETBACKTRANSLUCENT, MARGIN_BOOKMARK_VALUE, eu_get_config()->bookmark_argb);
    eu_sci_call(pnode, SCI_MARKERSETFORETRANSLUCENT, MARGIN_BOOKMARK_VALUE, eu_get_config()->bookmark_argb);
    // 代码折叠栏颜色与亮量颜色
    eu_sci_call(pnode, SCI_SETFOLDMARGINHICOLOUR, true, eu_get_theme()->item.foldmargin.bgcolor);
    eu_sci_call(pnode, SCI_SETFOLDMARGINCOLOUR, true, eu_get_theme()->item.foldmargin.bgcolor);
    // 当前行背景色
    eu_sci_call(pnode, SCI_SETCARETLINEVISIBLE, TRUE, 0);
    eu_sci_call(pnode, SCI_SETCARETLINEVISIBLEALWAYS, 1, 0);
    eu_sci_call(pnode, SCI_SETCARETLINEBACK, eu_get_theme()->item.caretline.bgcolor, 0);
    // 设置边框透明度
    eu_sci_call(pnode, SCI_SETCARETLINEBACKALPHA, eu_get_theme()->item.caretline.bgcolor >> 24, 0);
    eu_sci_call(pnode, SCI_SETCARETLINEFRAME, 2, 0);

    eu_sci_call(pnode, SCI_SETCARETSTYLE, 1, 0);
    eu_sci_call(pnode, SCI_SETCARETFORE, eu_get_theme()->item.text.color, 0);
    // 选中行背景色
    eu_sci_call(pnode, SCI_SETSELBACK, true, eu_get_theme()->item.indicator.bgcolor);
    eu_sci_call(pnode, SCI_SETSELALPHA, eu_get_theme()->item.indicator.bgcolor >> 24, 0);
    // 设置换行符
    eu_sci_call(pnode, SCI_SETWRAPMODE, (eu_get_config()->line_mode ? 2 : 0), 0);
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
    eu_sci_call(pnode, SCI_SETVIEWWS, (eu_get_config()->ws_visiable ? SCWS_VISIBLEALWAYS : SCWS_INVISIBLE), 0);
    eu_sci_call(pnode, SCI_SETWHITESPACESIZE, eu_get_config()->ws_size, 0);
    eu_sci_call(pnode, SCI_SETTABDRAWMODE, SCTD_LONGARROW, 0);
    eu_sci_call(pnode, SCI_SETVIEWEOL, eu_get_config()->newline_visialbe, 0);
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
    if (eu_get_config()->m_render == IDM_SET_RENDER_TECH_D2D)
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
    // 总是显示垂直滚动条
    // eu_sci_call(pnode, SCI_SETVSCROLLBAR, 1, 0);
    // 需要时显示水平滚动条, 但是删除文本后, 滚动条不会消失
    eu_sci_call(pnode, SCI_SETSCROLLWIDTH, 1, 0);
    eu_sci_call(pnode, SCI_SETSCROLLWIDTHTRACKING, 1, 0);
    // 设置undo掩码, 接受SCN_MODIFIED消息
    eu_sci_call(pnode, SCI_SETMODEVENTMASK, SC_MOD_INSERTTEXT|SC_MOD_DELETETEXT|SC_PERFORMED_UNDO|SC_PERFORMED_REDO, 0);
    // 支持多列粘贴
    eu_sci_call(pnode, SCI_SETMULTIPASTE, SC_MULTIPASTE_EACH, 0);
    // 指示出不匹配的大括号
    eu_sci_call(pnode, SCI_BRACEBADLIGHTINDICATOR, true, INDIC_STRIKE);
    // 不产生鼠标悬浮消息(SCN_DWELLSTART, SCN_DWELLEND, 设置SC_TIME_FOREVER>0则产生
    eu_sci_call(pnode, SCI_SETMOUSEDWELLTIME, SC_TIME_FOREVER, 0);
    return 0;
}

int
on_sci_before_file(eu_tabpage *pnode)
{
    if (pnode)
    {
        on_sci_init_style(pnode);
        eu_sci_call(pnode, SCI_CANCEL, 0, 0);
        eu_sci_call(pnode, SCI_EMPTYUNDOBUFFER, 0, 0);
        eu_sci_call(pnode, SCI_SETUNDOCOLLECTION, 0, 0);
        eu_sci_call(pnode, SCI_SETREADONLY, 0, 0);
        if (pnode->doc_ptr && pnode->doc_ptr->fn_init_before)
        {   // 初始化侧边栏控件
            pnode->doc_ptr->fn_init_before(pnode);
        }
    }
    return 0;
}

int
on_sci_after_file(eu_tabpage *pnode)
{
    if (pnode)
    {
        eu_sci_call(pnode, SCI_SETUNDOCOLLECTION, 1, 0);
        eu_sci_call(pnode, SCI_EMPTYUNDOBUFFER, 0, 0);
        eu_sci_call(pnode, SCI_SETEOLMODE, pnode->eol, 0);
        if (pnode->zoom_level != 0)
        {
            int zoom = pnode->zoom_level;
            printf("pnode->zoom_level = %d\n", pnode->zoom_level);
            if (pnode->zoom_level > 0)
            {
                while (zoom--)
                {
                    on_view_zoom_out(pnode);
                }
            }
            else if (pnode->zoom_level < 0)
            {
                while (zoom++)
                {
                    on_view_zoom_in(pnode);
                }
            }
        }
        if (!pnode->raw_size)
        {
            pnode->raw_size = eu_sci_call(pnode, SCI_GETLENGTH, 0, 0) + pnode->pre_len;
        }
        if (pnode->doc_ptr && pnode->doc_ptr->fn_init_after)
        {   // 设置侧边栏数据
            pnode->doc_ptr->fn_init_after(pnode);
        }
        on_sci_update_margin(pnode);
        return 0;
    }
    return 1;
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
    if (pnode->hwnd_qredit)
    {
        DestroyWindow(pnode->hwnd_qredit);
        pnode->hwnd_qredit = NULL;
    }
    if (pnode->hwnd_qrtable)
    {
        DestroyWindow(pnode->hwnd_qrtable);
        pnode->hwnd_qrtable = NULL;
    }
}

void
on_sci_free_tab(eu_tabpage **ppnode)
{
    if (!(ppnode && *ppnode))
    {
        return;
    }
    if ((*ppnode)->db_ptr)
    {
        // 关闭数据库链接
        on_table_disconnect_database(*ppnode, true);
    }
    if ((*ppnode)->redis_ptr)
    {
        // 关闭redis数据库链接
        on_symtree_disconnect_redis(*ppnode);
    }
    // 销毁控件句柄
    if ((*ppnode)->hwnd_symlist)
    {
        SendMessage((*ppnode)->hwnd_symlist, WM_CLOSE, 0, 0);
        (*ppnode)->hwnd_symlist = NULL;
    }
    if ((*ppnode)->hwnd_symtree)
    {
        SendMessage((*ppnode)->hwnd_symtree, WM_CLOSE, 0, 0);
        (*ppnode)->hwnd_symtree = NULL;
    }
    if ((*ppnode)->hwnd_qredit)
    {
        SendMessage((*ppnode)->hwnd_qredit, WM_CLOSE, 0, 0);
        (*ppnode)->hwnd_qredit = NULL;
    }
    if ((*ppnode)->hwnd_qrtable)
    {
        SendMessage((*ppnode)->hwnd_qrtable, WM_CLOSE, 0, 0);
        (*ppnode)->hwnd_qrtable = NULL;
    }
    if (!(*ppnode)->phex)
    {
        if ((*ppnode)->hwnd_sc)
        {
            SendMessage((*ppnode)->hwnd_sc, WM_CLOSE, 0, 0);
        }
        // 清理上一次的备份
        if ((*ppnode)->bakpath[0] && (_taccess((*ppnode)->bakpath, 0 ) != -1))
        {
            if (!DeleteFile((*ppnode)->bakpath))
            {
                printf("on on_sci_free_tab(), Delete(%ls) error, cause: %lu\n", (*ppnode)->bakpath, GetLastError());
            }
        }
        printf("we destroy pnode\n");
        eu_safe_free(*ppnode);
    }
    else
    {
        SendMessage((*ppnode)->hwnd_sc, HVM_SETHEXDEAD, 0, 0);
        printf("not destroy pnode->hex_mode\n");
    }
}

void
on_sci_insert_egg(eu_tabpage *pnode)
{
    unsigned char represent[]={0x0a,0x2f,0x2f,0x20, 0x53,0x6b,0x79,0x6c,0x61,0x72,0x6B,0XE6,0X98,0xAF,0xe4,0xb8,0x80,0xe4,
                               0xb8,0xaa,0xe5,0xbe,0x88,0xe6,0xa3,0x92,0xe7,0x9a,0x84,0xe7,0xbc,0x96,0xe8,0xbe,0x91,0xe5,
                               0x99,0xa8,0xf0,0x9f,0x98,0x80,0x0a,0x00};
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
        Sci_TextRange tr = {{start, end}, text};
        eu_sci_call(pnode, SCI_GETTEXTRANGE, 0, (sptr_t) &tr);
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
    if (pnode)
    {
        return eu_sci_call(pnode, SCI_GETMODIFY, 0, 0);
    }
    return false;
}

int
on_sci_query_tab(eu_tabpage *pnode)
{
    EU_VERIFY(pnode != NULL && g_tabpages != NULL);
    eu_tabpage *p = NULL;
    int count = TabCtrl_GetItemCount(g_tabpages);
    for (int index = 0; index < count; ++index)
    {
        TCITEM tci = {TCIF_PARAM};
        TabCtrl_GetItem(g_tabpages, index, &tci);
        p = (eu_tabpage *) (tci.lParam);
        if (p == pnode)
        {
            return index;
        }
    }
    return -1;
}

int
on_sci_point_reached(eu_tabpage *pnode)
{
    int index;
    if (!(pnode && *pnode->filename))
    {
        return -1;
    }
    index = on_sci_query_tab(pnode);
    if (index < 0)
    {
        return -2;
    }
    on_tabpage_set_title(index, pnode->filename);
    util_set_title(pnode->pathfile);
    pnode->be_modify = false;
    on_toolbar_update_button();
    return 0;
}

int
on_sci_point_left(eu_tabpage *pnode)
{
    int index;
    if (!(pnode && *pnode->filename))
    {
        return -1;
    }
    if ((index = on_sci_query_tab(pnode)) < 0)
    {
        return -2;
    }
    on_tabpage_set_title(index, pnode->filename);
    util_set_title(pnode->pathfile);
    pnode->be_modify = true;
    on_toolbar_update_button();
    return 0;
}

int
on_sci_character(eu_tabpage *pnode, SCNotification *lpnotify)
{
    if (!pnode->hex_mode)
    {
        if (pnode->doc_ptr && pnode->doc_ptr->fn_on_char)
        {
            return pnode->doc_ptr->fn_on_char(pnode, lpnotify);
        }
    }
    return 0;
}

void
on_sci_update_margin(eu_tabpage *pnode)
{
    EU_VERIFY(pnode != NULL);
    sptr_t m_width = eu_sci_call(pnode, SCI_GETMARGINWIDTHN, MARGIN_LINENUMBER_INDEX, 0);
    sptr_t m_line = eu_sci_call(pnode, SCI_GETLINECOUNT, 0, 0);
    char marg_width[FT_LEN] = { 0 };
    int m_zoom = (int) eu_sci_call(pnode, SCI_GETZOOM, 0, 0);
    snprintf(marg_width, FT_LEN - 1, "__%d", m_line);
    sptr_t cur_width = eu_sci_call(pnode, SCI_TEXTWIDTH, STYLE_LINENUMBER, (sptr_t) marg_width);
    if (cur_width != m_width)
    {
        eu_sci_call(pnode, SCI_SETMARGINWIDTHN, MARGIN_LINENUMBER_INDEX, (eu_get_config()->m_linenumber ? cur_width + m_zoom : 0));
    }
}

static void
on_sci_goto_positon(eu_tabpage *pnode, sptr_t adjust_pos)
{
    EU_VERIFY(pnode != NULL);
    eu_sci_call(pnode, SCI_SCROLLCARET, 0, 0);
    eu_sci_call(pnode, SCI_GOTOPOS, adjust_pos, 0);
    eu_sci_call(pnode, SCI_CHOOSECARETX, 0, 0);
}

LRESULT CALLBACK
sc_edit_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    eu_tabpage *pnode = NULL;
    switch (message)
    {
        //case WM_SIZE:
        //    return 1;
        case WM_KEYDOWN:
        {   // 按下ESC键时
            if ((wParam == VK_ESCAPE || KEY_DOWN(VK_ESCAPE)) && (pnode = on_tabpage_focus_at()))
            {
                sptr_t total_len = eu_sci_call(pnode, SCI_GETLENGTH, 0, 0);
                sptr_t cur_pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
                eu_sci_call(pnode, SCI_CANCEL, 0, 0);
                eu_sci_call(pnode, SCI_INDICATORCLEARRANGE, 0, total_len);
                eu_sci_call(pnode, SCI_SETEMPTYSELECTION, cur_pos, 0);
                if (pnode->zoom_level == SELECTION_ZOOM_LEVEEL)
                {
                    on_view_zoom_reset(pnode);
                    pnode->zoom_level = 0;
                }
            }
            break;
        }
        case WM_KEYUP:
        {
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
            on_search_add_navigate_list(pnode, eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0));
            on_statusbar_update_line(pnode);
            break;
        }
        case WM_RBUTTONDOWN:
        {
            if ((pnode = on_tabpage_get_handle(hwnd)) == NULL || pop_editor_menu == NULL)
            {
                break;
            }
            util_enable_menu_item(pop_editor_menu, IDM_EDIT_CUT, util_can_selections(pnode));
            util_enable_menu_item(pop_editor_menu, IDM_EDIT_COPY, util_can_selections(pnode));
            util_enable_menu_item(pop_editor_menu, IDM_EDIT_PASTE, eu_sci_call(pnode,SCI_CANPASTE, 0, 0));
            HMENU pop_menu = GetSubMenu(pop_editor_menu, 0);
            if (pop_menu)
            {
                POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                ClientToScreen(hwnd, &pt);
                TrackPopupMenu(pop_menu, 0, pt.x, pt.y, 0, hwnd, NULL);
            }
            break;
        }
        case WM_COMMAND:
        {
            PostMessage(eu_module_hwnd(), WM_COMMAND, wParam, lParam);
            break;
        }
        case WM_THEMECHANGED:
        {
            printf("scintilla WM_THEMECHANGED\n");
            if (eu_get_config()->m_toolbar)
            {
                on_toolbar_update_button();
            }
            break;
        }
        case WM_DPICHANGED:
        {
            if ((pnode = (eu_tabpage *) lParam) != NULL)
            {
                printf("scintilla WM_DPICHANGED\n");
            }
            break;
        }
        case WM_DPICHANGED_AFTERPARENT:
        {
            if ((pnode = on_tabpage_get_handle(hwnd)) != NULL)
            {
                printf("scintilla WM_DPICHANGED_AFTERPARENT\n");
                on_sci_update_margin(pnode);
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
            printf("scintilla WM_DESTROY\n");
            break;
        }
        default:
            break;
    }
    return CallWindowProc(sc_edit_wnd, hwnd, message, wParam, lParam);
}

int
eu_sci_release(void)
{
    return Scintilla_ReleaseResources();
}

sptr_t
eu_sci_call(eu_tabpage *p, int m, sptr_t w, sptr_t l)
{
    if (!p)
    {
        return 0;
    }
    if (p->hex_mode && p->hwnd_sc)
    {
        return SendMessage(p->hwnd_sc, m, w, l);
    }
    return ((SciFnDirect)ptr_scintilla)(p->eusc, m, w, l);
}

void
eu_send_notify(HWND hwnd, uint32_t code, LPNMHDR phdr)
{
    phdr->hwndFrom = hwnd;
    phdr->code = code;
    SendMessage(GetParent(hwnd), WM_NOTIFY, 0, (LPARAM) phdr);
}

int
on_sci_init_dlg(eu_tabpage *pnode)
{
    EU_VERIFY(pnode != NULL);
    const int flags = WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_EX_RTLREADING;
    pnode->hwnd_sc = CreateWindowEx(0, TEXT("Scintilla"), TEXT(""), flags, 0, 0, 0, 0, eu_module_hwnd(), 0, eu_module_handle(), 0);
    if (pnode->hwnd_sc == NULL)
    {
        MSG_BOX(IDC_MSG_SCINTILLA_ERR1, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
        return 1;
    }
    if (!(sc_edit_wnd = (WNDPROC) SetWindowLongPtr(pnode->hwnd_sc, GWLP_WNDPROC, (LONG_PTR) sc_edit_proc)))
    {
        printf("SetWindowLongPtr(pnode->hwnd_sc) failed\n");
        return 1;
    }
#ifdef _WIN64
    if (!_InterlockedCompareExchange64(&ptr_scintilla, SendMessage(pnode->hwnd_sc, SCI_GETDIRECTFUNCTION, 0, 0), 0));
#else
    if (!_InterlockedCompareExchange(&ptr_scintilla, SendMessage(pnode->hwnd_sc, SCI_GETDIRECTFUNCTION, 0, 0), 0));
#endif
    pnode->eusc = SendMessage(pnode->hwnd_sc, SCI_GETDIRECTPOINTER, 0, 0);
    eu_sci_call(pnode, SCI_USEPOPUP, 0, 0);
    return 0;
}
