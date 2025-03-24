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

static volatile sptr_t symlist_wnd = 0;

static int
pcre_match_callback(pcre_conainer *pcre_info, void *param)
{
    TCHAR *uni_str = NULL;
    eu_tabpage *pnode = (eu_tabpage *)param;
    if (!pnode || pnode->pcre_id == 1)
    {
        eu_logmsg("Pcre: recv cancel message, thread %u exit ...\n", GetCurrentThreadId());
        return EUE_TAB_NULL;
    }
    if (pcre_info->rc < 0)
    {
        eu_logmsg("Pcre: matching error or not match\n");
        return EUE_PCRE_NO_MATCHING;
    }
    for (int i = 1; i < pcre_info->rc; ++i)
    {
        char buf[MAX_PATH+1] = {0};
        const char *substring_start = pcre_info->buffer + pcre_info->ovector[2 * i];
        int substring_length = pcre_info->ovector[2 * i + 1] - pcre_info->ovector[2 * i];
        if (substring_length > 0)
        {
            snprintf(buf, MAX_PATH, "%.*s", substring_length, substring_start);
        }
        if (*buf != 0 && STRCMP(buf, !=, "if") && (uni_str = eu_utf8_utf16(buf, NULL)))
        {
            PostMessage(pnode->hwnd_symlist, WM_PCRE_ADDSTRING, (WPARAM)uni_str, pcre_info->ovector[2 * i]);
        }
    }
    return SKYLARK_OK;
}

static unsigned WINAPI
reqular_thread(void *lp)
{
    eu_tabpage *pnode = (eu_tabpage *) lp;
    if (pnode)
    {
        pcre_conainer *pcre_info = NULL;
        if ((pcre_info = eu_pcre_init((const char *)pnode->write_buffer, pnode->bytes_written, pnode->doc_ptr->reqular_exp, NULL, PCRE_NO_UTF8_CHECK|PCRE_CASELESS)) != NULL)
        {
            eu_pcre_exec_multi(pcre_info, pcre_match_callback, pnode);
            eu_pcre_delete(pcre_info);
        }
        eu_safe_free(pnode->write_buffer);
        _InterlockedExchange(&pnode->pcre_id, 0);
        eu_logmsg("Pcre: %s exit ...\n", __FUNCTION__);
    }
    return 0;
}

void
on_symlist_wait(eu_tabpage *pnode)
{
    if (pnode && pnode->pcre_id)
    {
        _InterlockedExchange(&pnode->pcre_id, 1);
        while (_InterlockedCompareExchange(&pnode->pcre_id, 0, 0) != 0)
        {
            Sleep(100);
        }
        eu_safe_free(pnode->write_buffer);
    }
}

int
on_symlist_reqular(eu_tabpage *pnode)
{
    if (pnode && !pnode->pcre_id)
    {
        do
        {
            if (!(pnode->hwnd_symlist && pnode->doc_ptr && pnode->doc_ptr->reqular_exp))
            {
                break;
            }
            if (!(pnode->write_buffer = (uint8_t *)util_strdup_content(pnode, &pnode->bytes_written)))
            {
                break;
            }
            ListBox_ResetContent(pnode->hwnd_symlist);
            CloseHandle((HANDLE) _beginthreadex(NULL, 0, &reqular_thread, pnode, 0, (uint32_t *)&pnode->pcre_id));
        } while(0);
    }
    return 0;
}

int
on_symlist_jump_word(eu_tabpage *pnode)
{
    sptr_t pos;
    sptr_t start_pos;
    sptr_t end_pos;
    TCHAR *ptext = NULL;
    char *current_text = NULL;
    if (!pnode)
    {
        return EUE_TAB_NULL;
    }
    pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
    start_pos = eu_sci_call(pnode, SCI_WORDSTARTPOSITION, pos, true);
    end_pos = eu_sci_call(pnode, SCI_WORDENDPOSITION, pos, true);
    current_text = on_sci_range_text(pnode, start_pos, end_pos);
    if (!current_text)
    {
        return EUE_POINT_NULL;
    }
    if (!(ptext = eu_utf8_utf16(current_text, NULL)))
    {
        free(current_text);
        return EUE_POINT_NULL;
    }
    int count = TabCtrl_GetItemCount(g_tabpages);
    for (int index = 0; index < count; ++index)
    {
        eu_tabpage *p = on_tabpage_get_ptr(index);
        if (p && p->doc_ptr && p->doc_ptr->doc_type == DOCTYPE_CPP)
        {
            int i = ListBox_FindStringExact(p->hwnd_symlist, -1, ptext);
            if (i != LB_ERR)
            {
                if (p != pnode)
                {
                    on_tabpage_select_index(index);
                }
                sptr_t line_num = (sptr_t) SendMessage(p->hwnd_symlist, LB_GETITEMDATA, i, 0);
                on_navigate_list_update(p, pos);
                on_search_jmp_line(p, line_num, 0);
                break;
            }
        }
    }
    free(current_text);
    free(ptext);
    return SKYLARK_OK;
}

int
on_symlist_jump_item(eu_tabpage *pnode)
{
    sptr_t  item_num = (sptr_t ) SendMessage(pnode->hwnd_symlist, LB_GETCURSEL, 0, 0);
    sptr_t  line_num = (sptr_t ) SendMessage(pnode->hwnd_symlist, LB_GETITEMDATA, item_num, 0);
    sptr_t  pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
    sptr_t  current_line = eu_sci_call(pnode, SCI_LINEFROMPOSITION, pos, 0);
    on_navigate_list_update(pnode, pos);
    on_search_jmp_line(pnode, line_num, current_line);
    return SKYLARK_OK;
}

int
on_symlist_update_theme(eu_tabpage *pnode)
{
    if (pnode && pnode->hwnd_symlist)
    {
        if (pnode->hwnd_font)
        {
            DeleteObject(pnode->hwnd_font);
        }
        pnode->hwnd_font = util_create_font(eu_get_theme()->item.symbolic.font, eu_get_theme()->item.symbolic.fontsize, eu_get_theme()->item.symbolic.bold);
        SendMessage(pnode->hwnd_symlist, WM_SETFONT, (WPARAM)pnode->hwnd_font, 0);
    }
    return SKYLARK_OK;
}

LRESULT CALLBACK
symlist_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_PCRE_ADDSTRING:
        {
            eu_tabpage *pnode = (eu_tabpage *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
            if (pnode)
            {
                TCHAR *uni_str = (TCHAR *)wParam;
                intptr_t pos = (intptr_t)lParam;
                sptr_t line_num = eu_sci_call(pnode, SCI_LINEFROMPOSITION, pos, 0);
                int index = ListBox_AddString(pnode->hwnd_symlist, uni_str);
                ListBox_SetItemData(pnode->hwnd_symlist, index, (LPARAM) line_num);
                free(uni_str);
            }
            return 1;
        }
        case WM_COMMAND:
        {
            if (LOWORD(wParam) == IDM_RELOAD_SYMBOLLIST)
            {
                eu_tabpage *pnode = NULL;
                if ((pnode = on_tabpage_focus_at()) && pnode->doc_ptr && pnode->doc_ptr->fn_reload_symlist)
                {
                    pnode->doc_ptr->fn_reload_symlist(pnode);
                }
            }
            break;
        }
        case WM_LBUTTONDBLCLK:
        {
            on_tabpage_symlist_click((eu_tabpage *) GetWindowLongPtr(hwnd, GWLP_USERDATA));
            return 1;
        }
        case WM_RBUTTONDOWN:
        {
            return menu_pop_track(hwnd, IDR_SYMBOLLIST_POPUPMENU, 0, -1, NULL, NULL);
        }
        case WM_DPICHANGED:
        {
            on_symlist_update_theme((eu_tabpage *) GetWindowLongPtr(hwnd, GWLP_USERDATA));
            break;
        }
        case WM_DESTROY:
        {
            util_symlink_destroy((eu_tabpage *) GetWindowLongPtr(hwnd, GWLP_USERDATA));
            break;
        }
        default:
            break;
    }
    return CallWindowProc((WNDPROC)symlist_wnd, hwnd, message, wParam, lParam);
}

int
on_symlist_create(eu_tabpage *pnode)
{
    if (pnode && pnode->doc_ptr && pnode->doc_ptr->reqular_exp)
    {
        if (pnode->hwnd_symlist)
        {
            DestroyWindow(pnode->hwnd_symlist);
        }
        pnode->hwnd_symlist = CreateWindow(_T("listbox"),
                                           NULL,
                                           WS_CHILD | WS_CLIPSIBLINGS | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | WS_TABSTOP | WS_VSCROLL,
                                           0,
                                           0,
                                           0,
                                           0,
                                           eu_module_hwnd(),
                                           NULL,
                                           eu_module_handle(),
                                           NULL);
        if (pnode->hwnd_symlist == NULL)
        {
            MSG_BOX(IDC_MSG_SYMLIST_FAIL, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
            return 1;
        }
        if (inter_atom_compare_exchange(&symlist_wnd, SetWindowLongPtr(pnode->hwnd_symlist, GWLP_WNDPROC, (LONG_PTR) symlist_proc), 0))
        {
            SetWindowLongPtr(pnode->hwnd_symlist, GWLP_WNDPROC, (LONG_PTR) symlist_proc);
        }
        if (!symlist_wnd)
        {
            eu_logmsg("%s: SetWindowLongPtr(pnode->hwnd_symlist) failed\n", __FUNCTION__);
            return 1;
        }
        else
        {
            SetWindowLongPtr(pnode->hwnd_symlist, GWLP_USERDATA, (intptr_t) pnode);
        }
        return on_symlist_update_theme(pnode);
    }
    return SKYLARK_OK;
}
