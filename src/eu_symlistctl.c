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

static WNDPROC symlist_wnd;

static int
pcre_match_callback(pcre_conainer *pcre_info, void *param)
{
    MSG msg = {0};
    eu_tabpage *pnode = (eu_tabpage *)param;
    if (!pnode)
    {
        return EUE_TAB_NULL;
    }
    if (pcre_info->rc > 1)
    {
        char buf[MAX_PATH+1] = {0};
        const char *substring_start = pcre_info->buffer + pcre_info->ovector[2];
        int substring_length = pcre_info->ovector[3] - pcre_info->ovector[2];
        sptr_t line_num = eu_sci_call(pnode, SCI_LINEFROMPOSITION, pcre_info->ovector[2], 0);
        snprintf(buf, MAX_PATH, "%.*s", substring_length, substring_start);
        if (STRCMP(buf, !=, "if"))
        {
            TCHAR *uni_str = eu_utf8_utf16(buf, NULL);
            if (uni_str)
            {
                int index = ListBox_AddString(pnode->hwnd_symlist, uni_str);
                free(uni_str);
                ListBox_SetItemData(pnode->hwnd_symlist, index, (LPARAM) line_num);
            }
        }
    }
    return SKYLARK_OK;
}

static unsigned WINAPI
reqular_thread(void *lp)
{
    size_t file_size;
    char *file_buffer = NULL;
    pcre_conainer *pcre_info = NULL;
    eu_tabpage *pnode = (eu_tabpage *) lp;
    if (!pnode)
    {
        return 1;
    }
    do
    {
        if (!(pnode->hwnd_symlist && pnode->doc_ptr && pnode->doc_ptr->reqular_exp))
        {
            break;
        }
        if (!(file_buffer = util_strdup_content(pnode, &file_size)))
        {
            break;
        }
        if (!(pcre_info = eu_pcre_init(file_buffer, file_size, pnode->doc_ptr->reqular_exp, NULL, PCRE_NO_UTF8_CHECK|PCRE_CASELESS)))
        {
            break;
        }
        ListBox_ResetContent(pnode->hwnd_symlist);
        eu_pcre_exec_multi(pcre_info, pcre_match_callback, pnode);
    } while(0);
    if (file_buffer)
    {
        free(file_buffer);
    }
    if (pcre_info)
    {
        eu_pcre_delete(pcre_info);
    }
    _InterlockedExchange(&pnode->pcre_id, 0);
    printf("reqular_thread exit\n");
    return 0;
}

int
on_symlist_reqular(eu_tabpage *pnode)
{
    if (pnode && !pnode->pcre_id)
    {
        CloseHandle((HANDLE) _beginthreadex(NULL, 0, &reqular_thread, pnode, 0, (uint32_t *)&pnode->pcre_id));
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
        eu_tabpage *p = NULL;
        TCITEM tci = {TCIF_PARAM};
        TabCtrl_GetItem(g_tabpages, index, &tci);
        p = (eu_tabpage *) (tci.lParam);
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
                on_search_add_navigate_list(p, pos);
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
    on_search_add_navigate_list(pnode, pos);
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
    eu_tabpage *pnode = NULL;
    switch (message)
    {
        case WM_COMMAND:
        {
            if (LOWORD(wParam) == IDM_RELOAD_SYMBOLLIST)
            {
                if ((pnode = on_tabpage_focus_at()) && pnode->doc_ptr && pnode->doc_ptr->fn_reload_symlist)
                {
                    pnode->doc_ptr->fn_reload_symlist(pnode);
                }
            }
            break;
        }
        case WM_LBUTTONDBLCLK:
        {
            pnode = (eu_tabpage *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
            on_tabpage_symlist_click(pnode);
            return 1;
        }
        case WM_RBUTTONDOWN:
        {
            return menu_pop_track(hwnd, IDR_SYMBOLLIST_POPUPMENU, 0, -1, NULL, NULL);
        }
        case WM_DPICHANGED:
        {
            pnode = (eu_tabpage *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
            on_symlist_update_theme(pnode);
            break;
        }
        case WM_DESTROY:
        {
            pnode = (eu_tabpage *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
            if (pnode)
            {
                if (pnode->hwnd_font)
                {
                    DeleteObject(pnode->hwnd_font);
                    pnode->hwnd_font = NULL;
                }
                // 强制终止后台线程, 当软链接未解析完成时会导致资源泄露
                if (pnode->pcre_id)
                {
                    util_kill_thread((uint32_t)pnode->pcre_id);
                }
            }
            printf("symlist WM_DESTROY\n");
            break;
        }
        default:
            break;
    }
    return CallWindowProc(symlist_wnd, hwnd, message, wParam, lParam);
}

int
on_symlist_create(eu_tabpage *pnode)
{
    EU_VERIFY(pnode != NULL);
    if (pnode->doc_ptr->reqular_exp)
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
        if (!(symlist_wnd = (WNDPROC) SetWindowLongPtr(pnode->hwnd_symlist, GWLP_WNDPROC, (LONG_PTR) symlist_proc)))
        {
            printf("SetWindowLongPtr(pnode->hwnd_symlist) failed\n");
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
