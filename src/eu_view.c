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

void
on_view_filetree(void)
{
    eu_get_config()->m_ftree_show ^= true;
    on_proc_redraw(NULL);
}

void
on_view_symtree(eu_tabpage *pnode)
{
    if (pnode && !TAB_HEX_MODE(pnode) && !pnode->pmod && pnode->doc_ptr && pnode->doc_ptr->fn_init_before)
    {   // 运行右侧边栏控件
        if (pnode->doc_ptr->fn_reload_symlist)
        {
            if (pnode->hwnd_symlist || pnode->doc_ptr->fn_init_before(pnode) == SKYLARK_OK)
            {
                pnode->doc_ptr->fn_reload_symlist(pnode);
            }
        }
        else if (pnode->doc_ptr->fn_reload_symtree)
        {
            if (pnode->hwnd_symtree || pnode->doc_ptr->fn_init_before(pnode) == SKYLARK_OK)
            {
                pnode->doc_ptr->fn_reload_symtree(pnode);     
            }  
        }
        pnode->sym_show ^= true;
        eu_window_resize();
    }
}

void
on_view_split_tabbar(void)
{
    struct eu_config *pconf = eu_get_config();
    if (pconf)
    {
        eu_tabpage *p = on_tabpage_focused();
        if (pconf->eu_tab.slave_show)
        {   // 如果标签都为空则隐藏从视图
            int e1 = 0;
            bool reset = false;
            const HWND htab = HSLAVE_GET;
            const int count = htab ? TabCtrl_GetItemCount(htab) : 0;
            on_tabpage_count_empty(NULL, &e1);
            if (count < 1)
            {
                reset = true;
            }
            else if (e1 == count)
            {
                eu_tabpage *p = NULL;
                if ((reset = true))
                {
                    for (int index = 0; index < count; ++index)
                    {
                        if ((p = on_tabpage_get_ptr(htab, index)) && p->is_blank && !TAB_NOT_NUL(p) && !on_sci_doc_modified(p))
                        {
                            on_tabpage_delete_item(htab, 0);
                            on_sci_free_tab(&p);
                        }
                    }
                }
            }
            if (reset)
            {
                pconf->eu_tab.slave_show = false;
                eu_window_resize();
            }
        }
        else
        {   // 新建一个空标签再刷新
            if (eu_get_config()->eu_tab.s_copy)
            {
                on_tabpage_clone_tab(HMAIN_GET);
            }
            else
            {
                on_file_new(HSLAVE_GET, NULL);
            }
        }
        if (p)
        {
            on_tabpage_selection(p);
        }
    }
}

void
on_view_document_map(eu_tabpage *pnode)
{
    if (pnode && !TAB_HEX_MODE(pnode) && !pnode->plugin)
    {
        if ((pnode->map_show ^= true))
        {
            if (!on_map_launch())
            {
                pnode->map_show = false; 
            }
        }
        on_proc_redraw(NULL);
    }
}

void
on_view_result_show(eu_tabpage *pnode, const int key)
{
    if (pnode && !TAB_HEX_MODE(pnode) && !pnode->pmod && pnode->doc_ptr && pnode->doc_ptr->fn_init_before && pnode->doc_ptr->fn_keydown)
    {
        if (!pnode->result_show)
        {
            pnode->result_show = on_result_launch(pnode);
        }
        if (RESULT_SHOW(pnode))
        {
            pnode->qrtable_show = true;
            if (pnode->hwnd_qrtable || pnode->doc_ptr->fn_init_before(pnode) == SKYLARK_OK)
            {
                pnode->presult->pwant = on_toolbar_no_highlight;
                on_result_reload(pnode->presult);
                eu_window_resize();
                pnode->doc_ptr->fn_keydown(pnode, VK_F5, key);
            }
        }
    }
}

int
on_view_switch_type(eu_tabpage *p, const int m_type)
{
    if (p && TAB_HAS_TXT(p))
    {
        on_sci_resever_tab(p);
        if (m_type < 0)
        {
            p->doc_ptr = NULL;
        }
        else
        {
            p->doc_ptr = eu_doc_get_ptr() + m_type;
        }
        on_sci_before_file(p, false);
        on_sci_after_file(p, false);
        eu_window_resize();
        return 0;
    }
    return 1;
}

static void
on_view_refresh_scroll(eu_tabpage *pnode)
{
    if (pnode)
    {
        bool h = false;
        bool v = (bool)eu_sci_call(pnode, SCI_GETVSCROLLBAR, 0, 0);
        if (v)
        {
            eu_sci_call(pnode, SCI_SETVSCROLLBAR, 0, 0);
            eu_sci_call(pnode, SCI_SETVSCROLLBAR, v, 0);
        }
        if ((h = (bool)eu_sci_call(pnode, SCI_GETHSCROLLBAR, 0, 0)))
        {
            eu_sci_call(pnode, SCI_SETHSCROLLBAR, 0, 0);
            eu_sci_call(pnode, SCI_SETHSCROLLBAR, h, 0);
        }
        if (RESULT_SHOW(pnode))
        {
            if ((v = (bool)eu_sci_call(pnode->presult, SCI_GETVSCROLLBAR, 0, 0)))
            {
                eu_sci_call(pnode->presult, SCI_SETVSCROLLBAR, 0, 0);
                eu_sci_call(pnode->presult, SCI_SETVSCROLLBAR, v, 0);
            }
            if ((h = (bool)eu_sci_call(pnode->presult, SCI_GETHSCROLLBAR, 0, 0)))
            {
                eu_sci_call(pnode->presult, SCI_SETHSCROLLBAR, 0, 0);
                eu_sci_call(pnode->presult, SCI_SETHSCROLLBAR, h, 0);
            }
        }
    }
}

int
on_view_refresh_theme(HWND hwnd, const bool reload)
{
    HWND hmap = NULL;
    HWND snippet = NULL;
    eu_tabpage *p = NULL;
    HWND htab[2] = {HMAIN_GET, HMAIN_SHOW ? HSLAVE_GET : NULL};
    on_dark_delete_theme_brush();
    if (reload && util_under_wine())
    {
        on_theme_setup_font(hwnd);
    }
    on_treebar_update_theme();
    for (int k = 0; k < 2 && htab[k]; ++k)
    {
        for (int index = 0, count = TabCtrl_GetItemCount(htab[k]); index < count; ++index)
        {
            if (!(p = on_tabpage_get_ptr(htab[k], index)))
            {
                break;
            }
            if (TAB_HEX_MODE(p))
            {
                hexview_update_theme(p);
            }
            else
            {
                on_sci_init_style(p);
                on_sci_after_file(p, false);
            }
            if (p->hwnd_symlist)
            {
                on_symlist_update_theme(p);
                InvalidateRect(p->hwnd_symlist, NULL, true);
            }
            else if (p->hwnd_symtree)
            {
                on_symtree_update_theme(p);
                InvalidateRect(p->hwnd_symtree, NULL, true);
            }
            if (p->presult && p->presult->reserved0)
            {
                SendMessage((HWND)p->presult->reserved0, WM_THEMECHANGED, (WPARAM)p, 0);
            }
            if (p->hwnd_qrtable)
            {
                on_table_update_theme(p);
            }
            if (p->pmod)
            {
                np_plugins_setvalue(&p->plugin->funcs, &p->plugin->npp, NV_THEME_CHANGE, NULL);
            }
            if (p->file_attr & FILE_READONLY_COLOR)
            {   // 切换主题后为只读文件重新上色
                p->file_attr &= ~FILE_READONLY_COLOR;
                on_statusbar_btn_rw(p, true);
            }
            if (reload && (index == TabCtrl_GetCurSel(htab[k])))
            {
                on_view_refresh_scroll(p);
            }
        }
    }
    if ((hmap = on_map_hwnd()))
    {
        SendMessage(hmap, WM_THEMECHANGED, 0, 0);
    }
    if ((snippet = eu_snippet_hwnd()) && IsWindowVisible(snippet))
    {
        eu_tabpage *pview = (eu_tabpage *)GetWindowLongPtr(snippet, GWLP_USERDATA);
        if (pview && pview->hwnd_sc)
        {
            on_snippet_reload(pview);
        }
    }
    if (reload)
    {
        menu_bmp_destroy();
        InvalidateRect(on_toolbar_hwnd(), NULL, false);
        util_tips_dark();
        on_splitter_redraw();
        on_proc_redraw(NULL);
        UpdateWindowEx(hwnd);
    }
    return SKYLARK_OK;
}

int
on_view_switch_theme(HWND hwnd, const int id)
{
    int count = 0;
    HFONT hfont = NULL;
    eu_tabpage *p = NULL;
    TCHAR *pbuf = NULL;
    TCHAR buf[QW_SIZE+1] = {0};
    TCHAR old[QW_SIZE+1] = {0};
    if (!GetMenuString(GetMenu(hwnd), id, buf, QW_SIZE, MF_BYCOMMAND))
    {
        return 1;
    }
    if (!MultiByteToWideChar(CP_UTF8, 0, eu_get_theme()->name, -1, old, QW_SIZE))
    {
        return 1;
    }
    if (!(pbuf = on_theme_query_name(buf)))
    {
        return 1;
    }
    if (_tcscmp(pbuf, old) == 0)
    {
        return 0;
    }
    if (on_theme_load_script(pbuf))
    {
        eu_logmsg("%s: on_theme_load_script return false\n", __FUNCTION__);
        return 1;
    }
    if (strncpy(eu_get_config()->window_theme, eu_get_theme()->name, QW_SIZE))
    {
        on_dark_delete_brush();
    }
    if (_tcscmp(pbuf, _T("black")) == 0)
    {
        if (eu_dark_theme_init(true, true))
        {
            SendMessageTimeout(HWND_BROADCAST, WM_THEMECHANGED, 0, 0, SMTO_NORMAL, 10, 0);
        }
    }
    else
    {
        eu_dark_theme_release(false);
    }
    return on_view_refresh_theme(hwnd, true);
}

int
on_view_modify_theme(void)
{
    if (on_theme_create_dlg())
    {
        HWND hwnd = eu_module_hwnd();
        if (hwnd && !on_theme_setup_font(hwnd))
        {
            eu_logmsg("%s: on_theme_setup_font failed\n", __FUNCTION__);
            return 1;
        }
        return on_view_refresh_theme(hwnd, false);
    }
    return SKYLARK_OK;
}

void
on_view_copy_theme(void)
{
    TCHAR theme_name[QW_SIZE+1] = {0};
    TCHAR old_theme[QW_SIZE+1] = {0};
    LOAD_I18N_RESSTR(IDC_MSG_THEME_NAME, caption);
    if (!MultiByteToWideChar(CP_UTF8, 0, eu_get_theme()->name, -1, theme_name, QW_SIZE))
    {
        return;
    }
    else
    {
        _tcsncpy(old_theme, theme_name, QW_SIZE);
    }
    if (eu_input(caption, theme_name, QW_SIZE))
    {
        if (!theme_name[0])
        {
            MSG_BOX(IDC_MSG_THEME_ERR1, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
        }
        else if (_tcsicmp(theme_name, old_theme) == 0)
        {
            MSG_BOX(IDC_MSG_THEME_ERR2, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
        }
        else if (_tcschr(theme_name, _T(' ')) || _tcschr(theme_name, _T('\t')))
        {
            MSG_BOX(IDC_MSG_THEME_ERR3, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
        }
        else
        {
            on_theme_copy_style(theme_name);
        }
    }
}

void
on_view_tab_width(HWND hwnd, eu_tabpage *pnode)
{
    TCHAR tab_width[4] = {0};
    LOAD_I18N_RESSTR(IDC_MSG_TAB_LEN, tab_tips);
    if (!pnode)
    {
        return;
    }
    if (pnode->doc_ptr && pnode->doc_ptr->tab_width > 0)
    {   // 由脚本设定
        MSG_BOX(IDS_USERTAB_TIPS1, IDC_MSG_TIPS, MB_OK);
        return;
    }
    _sntprintf(tab_width, _countof(tab_width)-1, _T("%d"), eu_get_config()->tab_width);
    if (eu_input(tab_tips, tab_width, _countof(tab_width)))
    {
        if (tab_width[0])
        {
            eu_tabpage *p = NULL;
            HWND htab[2] = {HMAIN_GET, HMAIN_SHOW ? HSLAVE_GET : NULL};
            int m_width = _tstoi(tab_width);
            if (m_width > QW_SIZE)
            {
                _sntprintf(tab_width, _countof(tab_width)-1, _T("%d"), QW_SIZE);
                eu_get_config()->tab_width = QW_SIZE;
            }
            else
            {
                eu_get_config()->tab_width = m_width;
            }
            for (int k = 0; k < 2 && htab[k]; ++k)
            {
                for (int index = 0, count = TabCtrl_GetItemCount(htab[k]); index < count; ++index)
                {
                    if ((p = on_tabpage_get_ptr(htab[k], index)) != NULL && TAB_HAS_TXT(p))
                    {
                        if (p->doc_ptr)
                        {
                            eu_sci_call(p, SCI_SETTABWIDTH, p->doc_ptr->tab_width > 0 ? p->doc_ptr->tab_width : eu_get_config()->tab_width, 0);
                        }
                        else
                        {
                            eu_sci_call(p, SCI_SETTABWIDTH, eu_get_config()->tab_width, 0);
                        }
                    }
                }
            }
        }
    }
}

void
on_view_space_converter(HWND hwnd, eu_tabpage *pnode)
{
    eu_tabpage *p = NULL;
    HWND htab[2] = {HMAIN_GET, HMAIN_SHOW ? HSLAVE_GET : NULL};
    if (!pnode)
    {
        return;
    }
    if (pnode->doc_ptr && pnode->doc_ptr->tab_convert_spaces >= 0)
    {   // 由脚本设定
        MSG_BOX(IDS_USERTAB_TIPS1, IDC_MSG_TIPS, MB_OK);
        return;
    }
    eu_get_config()->tab2spaces ^= true;
    for (int k = 0; k < 2 && htab[k]; ++k)
    {
        for (int index = 0, count = TabCtrl_GetItemCount(htab[k]); index < count; ++index)
        {
            if ((p = on_tabpage_get_ptr(htab[k], index)) && TAB_HAS_TXT(p))
            {
                if (p->doc_ptr)
                {
                    eu_sci_call(p, SCI_SETUSETABS, p->doc_ptr->tab_convert_spaces >= 0 ? !p->doc_ptr->tab_convert_spaces : !eu_get_config()->tab2spaces, 0);
                }
                else
                {
                    eu_sci_call(p, SCI_SETUSETABS, !eu_get_config()->tab2spaces, 0);
                }
            }
        }
    }
}

void
on_view_light_brace(eu_tabpage *p)
{
    if (p)
    {
        eu_get_config()->eu_brace.matching ^= true;
        if (!eu_get_config()->eu_brace.matching)
        {   // 取消括号匹配
            eu_sci_call(p, SCI_BRACEBADLIGHT, INVALID_POSITION, INVALID_POSITION);
        }
    }
}

void
on_view_light_str(eu_tabpage *p)
{
    if (p)
    {
        eu_get_config()->m_light_str ^= true;
        if (!eu_get_config()->m_light_str)
        {   // 取消指示器高亮
            eu_sci_call(p, SCI_INDICATORCLEARRANGE, 0, eu_sci_call(p, SCI_GETLENGTH, 0, 0));
        }
    }
}

void
on_view_light_fold(void)
{
    eu_tabpage *p = NULL;
    HWND htab[2] = {HMAIN_GET, HMAIN_SHOW ? HSLAVE_GET : NULL};
    eu_get_config()->light_fold ^= true;
    for (int k = 0; k < 2 && htab[k]; ++k)
    {
        for (int index = 0, count = TabCtrl_GetItemCount(htab[k]); index < count; ++index)
        {
            if ((p = on_tabpage_get_ptr(htab[k], index)) && TAB_HAS_TXT(p))
            {   // 是否高亮显示当前折叠块
                eu_sci_call(p, SCI_MARKERENABLEHIGHLIGHT, (sptr_t) eu_get_config()->light_fold, 0);
            }
        }
    }
}

void
on_view_wrap_line(void)
{
    eu_tabpage *p = NULL;
    HWND htab[2] = {HMAIN_GET, HMAIN_SHOW ? HSLAVE_GET : NULL};
    eu_get_config()->line_mode ^= true;
    for (int k = 0; k < 2 && htab[k]; ++k)
    {
        for (int index = 0, count = TabCtrl_GetItemCount(htab[k]); index < count; ++index)
        {
            if ((p = on_tabpage_get_ptr(htab[k], index)) && TAB_HAS_TXT(p))
            {
                eu_sci_call(p, SCI_SETWRAPMODE, (eu_get_config()->line_mode ? 2 : 0), 0);
            }
        }
    }
}

void
on_view_line_num(void)
{
    eu_tabpage *p = NULL;
    HWND htab[2] = {HMAIN_GET, HMAIN_SHOW ? HSLAVE_GET : NULL};
    eu_get_config()->m_linenumber ^= true;
    for (int k = 0; k < 2 && htab[k]; ++k)
    {
        for (int index = 0, count = TabCtrl_GetItemCount(htab[k]); index < count; ++index)
        {
            if ((p = on_tabpage_get_ptr(htab[k], index)) && TAB_HAS_TXT(p))
            {
                eu_sci_call(p, SCI_SETMARGINWIDTHN, MARGIN_LINENUMBER_INDEX, (eu_get_config()->m_linenumber ? MARGIN_LINENUMBER_WIDTH : 0));
            }
        }
    }
}

void
on_view_bookmark(void)
{
    eu_tabpage *p = NULL;
    HWND htab[2] = {HMAIN_GET, HMAIN_SHOW ? HSLAVE_GET : NULL};
    eu_get_config()->eu_bookmark.visable ^= true;
    for (int k = 0; k < 2 && htab[k]; ++k)
    {
        for (int index = 0, count = TabCtrl_GetItemCount(htab[k]); index < count; ++index)
        {
            if ((p = on_tabpage_get_ptr(htab[k], index)) && TAB_HAS_TXT(p))
            {
                eu_sci_call(p, SCI_SETMARGINWIDTHN, MARGIN_BOOKMARK_INDEX, (eu_get_config()->eu_bookmark.visable ? MARGIN_BOOKMARK_WIDTH : 0));
            }
        }
    }
}

static void
on_view_update_fold(void)
{
    eu_tabpage *p = NULL;
    HWND htab[2] = {HMAIN_GET, HMAIN_SHOW ? HSLAVE_GET : NULL};
    for (int k = 0; k < 2 && htab[k]; ++k)
    {
        for (int index = 0, count = TabCtrl_GetItemCount(htab[k]); index < count; ++index)
        {
            if ((p = on_tabpage_get_ptr(htab[k], index)) && TAB_HAS_TXT(p) && p->doc_ptr && p->foldline)
            {
                eu_sci_call(p, SCI_SETMARGINWIDTHN, MARGIN_FOLD_INDEX, eu_get_config()->block_fold ? MARGIN_FOLD_WIDTH : 0);
            }
        }
    }
}

void
on_view_show_fold_lines(void)
{
    eu_get_config()->block_fold ^= true;
    on_view_update_fold();
}

void
on_view_identation(void)
{
    eu_get_config()->m_ident ^= true;
    on_toolbar_update_button(NULL);
}

void
on_view_white_space(void)
{
    eu_tabpage *p = NULL;
    HWND htab[2] = {HMAIN_GET, HMAIN_SHOW ? HSLAVE_GET : NULL};
    eu_get_config()->ws_visiable ^= true;
    for (int k = 0; k < 2 && htab[k]; ++k)
    {
        for (int index = 0, count = TabCtrl_GetItemCount(htab[k]); index < count; ++index)
        {
            if ((p = on_tabpage_get_ptr(htab[k], index)) && TAB_HAS_TXT(p))
            {
                eu_sci_call(p, SCI_SETVIEWWS, (eu_get_config()->ws_visiable == true ? SCWS_VISIBLEALWAYS : SCWS_INVISIBLE), 0);
            }
        }
    }
}

void
on_view_line_visiable(void)
{
    eu_tabpage *p = NULL;
    HWND htab[2] = {HMAIN_GET, HMAIN_SHOW ? HSLAVE_GET : NULL};
    eu_get_config()->newline_visialbe ^= true;
    for (int k = 0; k < 2 && htab[k]; ++k)
    {
        for (int index = 0, count = TabCtrl_GetItemCount(htab[k]); index < count; ++index)
        {
            if ((p = on_tabpage_get_ptr(htab[k], index)) && TAB_HAS_TXT(p))
            {
                eu_sci_call(p, SCI_SETVIEWEOL, eu_get_config()->newline_visialbe, 0);
            }
        }
    }
}

void
on_view_indent_visiable(void)
{
    eu_tabpage *p = NULL;
    HWND htab[2] = {HMAIN_GET, HMAIN_SHOW ? HSLAVE_GET : NULL};
    eu_get_config()->m_indentation ^= true;
    for (int k = 0; k < 2 && htab[k]; ++k)
    {
        for (int index = 0, count = TabCtrl_GetItemCount(htab[k]); index < count; ++index)
        {
            if ((p = on_tabpage_get_ptr(htab[k], index)) && TAB_HAS_TXT(p))
            {
                eu_sci_call(p, SCI_SETINDENTATIONGUIDES, (eu_get_config()->m_indentation ? SC_IV_LOOKBOTH : SC_IV_NONE), 0);
            }
        }
    }
}

void
on_view_history_visiable(eu_tabpage *pnode, const int wm_id)
{
    const int history_mask = (const int)eu_get_config()->history_mask;
    if (pnode && wm_id > 0 && wm_id != history_mask)
    {
        int result = IDOK;
        bool affected = false;
        eu_tabpage *p = NULL;
        HWND htab[2] = {HMAIN_GET, HMAIN_SHOW ? HSLAVE_GET : NULL};
        int k = 0, index = 0, count = 0;
        const uint32_t maskn = wm_id - IDM_VIEW_HISTORY_PLACEHOLDE;
        for (; k < 2 && htab[k]; ++k)
        {
            for (index = 0, count = TabCtrl_GetItemCount(htab[k]); index < count; ++index)
            {
                if ((p = on_tabpage_get_ptr(htab[k], index)) && TAB_HAS_TXT(p))
                {   // 先给出提示
                    if (maskn > 1 && history_mask - IDM_VIEW_HISTORY_PLACEHOLDE == 1 &&
                       (eu_sci_call(p, SCI_CANUNDO, 0, 0) || eu_sci_call(p, SCI_CANREDO, 0, 0)))
                    {
                        if (!affected)
                        {
                            MSG_BOX_SEL(IDS_HISTORY_CLEAR_UNDO, IDC_MSG_TIPS, MB_ICONSTOP | MB_OKCANCEL, result);
                            affected = true;
                        }
                        if (result == IDOK)
                        {
                            eu_sci_call(p, SCI_EMPTYUNDOBUFFER, 0, 0);
                            p == pnode ? on_toolbar_update_button(pnode) : (void)0;
                        }
                    }
                }
            }
        }
        if (result == IDOK)
        {   // 然后再显示
            eu_get_config()->history_mask = (uint32_t)wm_id;
            for (k = 0; k < 2 && htab[k]; ++k)
            {
                for (index = 0, count = TabCtrl_GetItemCount(htab[k]); index < count; ++index)
                {
                    if ((p = on_tabpage_get_ptr(htab[k], index)) && TAB_HAS_TXT(p))
                    {
                        on_sci_update_history_margin(p);
                    }
                }
            }
            util_redraw(pnode->hwnd_sc, false);
        }
    }
}

void
on_view_zoom_out(eu_tabpage *pnode)
{
    if (pnode)
    {
        if (pnode->plugin && pnode->plugin->funcs.event)
        {
            pnode->plugin->funcs.event(&pnode->plugin->npp, (void *)IDM_ZOOMOUT_PLUGIN);
        }
        else
        {
            eu_sci_call(pnode, SCI_ZOOMOUT, 0, 0);
            on_sci_update_line_margin(pnode);
            on_sci_update_fold_margin(pnode);
        }
    }
}

void
on_view_zoom_in(eu_tabpage *pnode)
{
    if (pnode)
    {
        if (pnode->plugin && pnode->plugin->funcs.event)
        {
            pnode->plugin->funcs.event(&pnode->plugin->npp, (void *)IDM_ZOOMIN_PLUGIN);
        }
        else
        {
            eu_sci_call(pnode, SCI_ZOOMIN, 0, 0);
            on_sci_update_line_margin(pnode);
            on_sci_update_fold_margin(pnode);
        }
    }
}

void
on_view_zoom_reset(eu_tabpage *pnode)
{
    if (pnode)
    {
        if (pnode->plugin && pnode->plugin->funcs.event)
        {
            pnode->plugin->funcs.event(&pnode->plugin->npp, (void *)IDM_ZOOMFIT_PLUGIN);
        }
        else
        {
            eu_sci_call(pnode, SCI_SETZOOM, 0, 0);
            on_sci_update_line_margin(pnode);
            pnode->zoom_level = 0;
            on_sci_update_fold_margin(pnode);
        }
    }
}

int
on_view_editor_selection(eu_tabpage *pnode)
{
    if (!pnode || TAB_HEX_MODE(pnode) || pnode->pmod)
    {
        return SKYLARK_OK;
    }
    if (KEY_UP(VK_LBUTTON))
    {
        return SKYLARK_OK;
    }
    size_t select_len = 0;
    char *select_buf = util_strdup_select(pnode, &select_len, 0);
    sptr_t total_len = eu_sci_call(pnode, SCI_GETLENGTH, 0, 0);
    sptr_t sel_start = eu_sci_call(pnode, SCI_GETSELECTIONSTART, 0, 0);
    eu_sci_call(pnode, SCI_SETINDICATORCURRENT, INDIC_SKYLARK_SELECT, 0);
    eu_sci_call(pnode, SCI_INDICATORCLEARRANGE, 0, total_len);
    pnode->match_count = 0;
    if (select_buf)
    {
        sptr_t start_pos = 0;
        sptr_t found_pos = 0;
        size_t flags = SCFIND_MATCHCASE;
        sptr_t end_pos = total_len;
        if (eu_get_config() && eu_get_config()->last_flags & SCFIND_WHOLEWORD)
        {
            flags |= SCFIND_WHOLEWORD;
        }
        while (found_pos >= 0)
        {
            found_pos = on_search_process_find(pnode, select_buf, start_pos, end_pos, flags);
            if (found_pos >= 0)
            {
                if (found_pos != sel_start)
                {
                    eu_sci_call(pnode, SCI_INDICATORFILLRANGE, found_pos, select_len);
                }
                start_pos = found_pos+select_len;
                ++pnode->match_count;
            }
        }
        free(select_buf);
    }
    return SKYLARK_OK;
}

static void
on_view_update_taskbar(bool hide)
{
    HWND taskbar = FindWindow(_T("Shell_TrayWnd"), NULL);
    HWND start = FindWindow(_T("Button"), NULL);
    if (taskbar != NULL)
    {
        ShowWindow(taskbar, !hide ? SW_SHOW : SW_HIDE);
        UpdateWindow(taskbar);
    }
    if (start != NULL)
    {
        // Vista
        ShowWindow(start, !hide ? SW_SHOW : SW_HIDE);
        UpdateWindow(start);
    }
}

void
on_view_setfullscreenimpl(HWND hwnd)
{
    static bool saved = false;
    static WINDOWPLACEMENT wndpl = {0};
    static RECT saved_rect = {0};
    static LONG_PTR saved_style = 0;
    static LONG_PTR saved_exstyle = 0;
    if (eu_get_config()->m_fullscreen)
    {
        HMONITOR hmonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = {0};
        if (!saved)
        {
            saved = true;
            saved_style = GetWindowLongPtr(hwnd, GWL_STYLE);
            saved_exstyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
            SystemParametersInfo(SPI_GETWORKAREA, 0, &saved_rect, 0);
            wndpl.length = sizeof(WINDOWPLACEMENT);
            GetWindowPlacement(hwnd, &wndpl);
        }
        mi.cbSize = sizeof(mi);
        GetMonitorInfo(hmonitor, &mi);
        const int x = mi.rcMonitor.left;
        const int y = mi.rcMonitor.top;
        const int w = mi.rcMonitor.right - x;
        const int h = mi.rcMonitor.bottom - y;
        SystemParametersInfo(SPI_SETWORKAREA, 0, NULL, SPIF_SENDCHANGE);
        SetWindowLongPtr(hwnd, GWL_STYLE, saved_style & ~(WS_CAPTION | WS_THICKFRAME));
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, saved_exstyle & ~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));
        SetWindowPos(hwnd, HWND_TOP, x, y, w, h, SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    }
    else
    {
        saved = false;
        SetWindowLongPtr(hwnd, GWL_STYLE, saved_style);
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, saved_exstyle);
        if (wndpl.length)
        {
            SystemParametersInfo(SPI_SETWORKAREA, 0, &saved_rect, 0);
            if (wndpl.showCmd == SW_SHOWMAXIMIZED)
            {
                ShowWindow(hwnd, SW_RESTORE);
                ShowWindow(hwnd, SW_SHOWMAXIMIZED);
            }
            else
            {
                SetWindowPlacement(hwnd, &wndpl);
            }
        }
        if (!eu_get_config()->eu_titlebar.icon)
        {
            util_updateui_icon(hwnd, true);
            eu_get_config()->eu_titlebar.icon = true;
        }
    }
    SetForegroundWindow(hwnd);
}

void
on_view_full_sreen(HWND hwnd)
{
    eu_get_config()->m_fullscreen ^= true;
    if (!eu_get_config()->m_fullscreen)
    {
        int size = on_toolbar_icon_get();
        eu_get_config()->m_menubar = true;
        eu_get_config()->m_toolbar = size > 0 ? size : IDB_SIZE_1;
        on_toolbar_set_height(size > 0 ? size : IDB_SIZE_1);
        eu_get_config()->m_statusbar = true;
        GetMenu(hwnd)?(void)0:SetMenu(hwnd, i18n_load_menu(IDC_SKYLARK));
        on_toolbar_refresh(hwnd);
    }
    else
    {
        eu_get_config()->m_menubar = false;
        on_toolbar_icon_set(eu_get_config()->m_toolbar);
        eu_get_config()->m_toolbar = IDB_SIZE_0;
        eu_get_config()->m_statusbar = false;
        GetMenu(hwnd)?SetMenu(hwnd, NULL):(void)0;
            
    }
    on_view_setfullscreenimpl(hwnd);
}

void
on_view_font_quality(HWND hwnd, const int res_id)
{
    int old_id = eu_get_config()->m_quality;
    if (eu_get_config()->m_quality != res_id)
    {
        eu_get_config()->m_quality = res_id;
        if (!on_theme_setup_font(hwnd))
        {
            eu_logmsg("%s: on_theme_setup_font return false\n", __FUNCTION__);
            eu_get_config()->m_quality = old_id;
        }
        else 
        {
            eu_tabpage *p = NULL;
            HWND htab[2] = {HMAIN_GET, HMAIN_SHOW ? HSLAVE_GET : NULL};
            for (int k = 0; k < 2 && htab[k]; ++k)
            {
                for (int index = 0, count = TabCtrl_GetItemCount(htab[k]); index < count; ++index)
                {
                    if ((p = on_tabpage_get_ptr(htab[k], index)) && TAB_HAS_TXT(p))
                    {
                        on_sci_init_style(p);
                        on_sci_after_file(p, false);
                    }
                }
            }
            eu_window_resize();
        }
    }
}

void
on_view_enable_rendering(HWND hwnd, const int res_id)
{
    if (!util_under_wine() && eu_get_config()->m_render != res_id)
    {
        eu_tabpage *p = NULL;
        HWND htab[2] = {HMAIN_GET, HMAIN_SHOW ? HSLAVE_GET : NULL};
        eu_get_config()->m_render = res_id;
        for (int k = 0; k < 2 && htab[k]; ++k)
        {
            for (int index = 0, count = TabCtrl_GetItemCount(htab[k]); index < count; ++index)
            {
                if ((p = on_tabpage_get_ptr(htab[k], index)) && TAB_HAS_TXT(p))
                {
                    on_sci_init_style(p);
                    on_sci_after_file(p, false);
                }
            }
        }
        eu_window_resize();
    }
}
