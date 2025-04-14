/*******************************************************************************
 * This file is part of Skylark project
 * Copyright ©2025 Hua andy <hua.andy@gmail.com>

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
#include <sys/utime.h>

static bool
on_command_common_search(eu_tabpage *pnode, const char *key, sptr_t start_pos, sptr_t end_pos, const uint32_t flags)
{
    sptr_t found_pos = -1;
    uint32_t find_flags = (uint32_t)flags;
    bool loop = flags & SCCMD_LOOP;
    const bool reverse = flags & SCCMD_REVERSE;
    const sptr_t cur_pos = on_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
    const sptr_t max_pos = on_sci_call(pnode, SCI_GETTEXTLENGTH, 0, 0);
    const sptr_t sel_start = on_sci_call(pnode, SCI_GETSELECTIONSTART, 0, 0);

    find_flags &= ~SCCMD_REVERSE;
    find_flags &= ~SCCMD_LOOP;

    if (start_pos < 0)
    {
        if (reverse)
        {
            start_pos = cur_pos - 1;
            if (sel_start >= 0  && start_pos > sel_start)
            {
                start_pos = sel_start;
            }
        }
        else
        {
            start_pos = cur_pos;
            if (sel_start >= 0  && start_pos == sel_start)
            {
                start_pos = on_sci_call(pnode, SCI_GETSELECTIONEND, 0, 0);
            }
        }
    }
    if (end_pos < 0)
    {
        if (reverse)
        {
            end_pos = 0;
        }
        else
        {
            end_pos = max_pos;
        }
    }
    DO_COMMANDS_LOOP:
    {
        if ((found_pos = on_search_process_find(pnode, key, start_pos, end_pos, find_flags)) >= 0)
        {
            SetFocus(pnode->hwnd_sc);
            const sptr_t target_end = on_sci_call(pnode, SCI_GETTARGETEND, 0, 0);
            on_sci_call(pnode, SCI_SETSEL, found_pos, target_end);
        }
        else if (found_pos == -2)
        {
            char msg[MAX_PATH] = {0};
            LOAD_I18N_RESSTR(IDS_RE_ERROR, retips);
            on_result_append_text_utf8(util_make_u8(retips, msg, MAX_PATH));
        }
        else if (loop)
        {
            loop = false;
            if (reverse)
            {
                end_pos = start_pos;
                start_pos = max_pos;
            }
            else
            {
                end_pos = start_pos;
                start_pos = 0;
            }
            goto DO_COMMANDS_LOOP;
        }
    }
    if (found_pos >= 0)
    {
        on_search_set_fsm();
        eu_push_find_history(key);
        return true;
    }
    return false;
}

static bool
on_command_find(eu_tabpage *pnode, const char *key, const uint32_t flags)
{
    return on_command_common_search(pnode, key, -1, -1, flags);
}

bool
on_command_focus(const eu_tabpage *p)
{
    if ((RESULT_SHOW(p) && (on_sci_call(p->presult, SCI_GETCARETSTYLE, 0, 0) & CARETSTYLE_LINE)))
    {
        SetFocus(p->presult->hwnd_sc);
        return true;
    }
    return false;
}

void
on_command_light(void *lp)
{
    const eu_tabpage *p = (const eu_tabpage *)lp;
    if (p)
    {
        result_vec *pvec = NULL;
        on_sci_call(p, SCI_SETREADONLY, 0, 0);
        on_sci_call(p, SCI_CLEARALL, 0, 0);
        on_sci_call(p, SCI_SETCARETSTYLE, CARETSTYLE_LINE, 0);
        on_sci_call(p, SCI_SETPROPERTY, (sptr_t)result_extra, (sptr_t)&pvec);
    }
}

void
eu_command_launch(const int index)
{
    eu_tabpage *p = index < 0 ? on_tabpage_focus_at() : on_tabpage_get_ptr(index);
    if (p && !p->is_blank && TAB_HAS_TXT(p))
    {
        if (RESULT_SHOW(p) && p->presult->pwant == on_command_light && index < 0)
        {
            on_result_destroy(p);
            eu_logmsg("Command: we destroy commands window\n");
            return;
        }
        if (!p->result_show)
        {
            p->result_show = on_result_launch(p);
        }
        if (RESULT_SHOW(p))
        {
            p->presult->pwant = on_command_light;
            on_result_lexer(p->presult);
            if (index < 0)
            {
                eu_window_resize();
            }
        }
    }
}

bool
eu_command_search(const int t, const char *key, const uint32_t opt)
{
    eu_tabpage *p = t < 0 ? (eu_tabpage *)on_tabpage_focus_at() : (eu_tabpage *)on_tabpage_get_ptr(t);
    if (p && TAB_HAS_TXT(p))
    {
        if (on_command_find(p, key, opt | SCCMD_LOOP))
        {
            eu_get_config()->last_flags = opt & ~SCCMD_REVERSE;
            return true;
        }
    }
    return false;
}

bool
eu_command_replace(const int t, const char *key, const char *replace, const sptr_t n1, const sptr_t n2, const uint32_t opt)
{
    eu_tabpage *p = t < 0 ? (eu_tabpage *)on_tabpage_focus_at() : (eu_tabpage *)on_tabpage_get_ptr(t);
    if (p && TAB_HAS_TXT(p) && STR_NOT_NUL(key) && replace)
    {
        sptr_t offset = 0;
        sptr_t start = 0;
        sptr_t end = 0;
        bool result = false;
        bool txt_first = opt & SCCMD_TEXT_FIRST;
        bool line_first = opt & SCCMD_LINE_FIRST;
        if (n1 >= 0 && n2 >= 0)
        {
            start = n1;
            end = n2;
        }
        else if (txt_first || (opt & SCCMD_TEXT_ALL))
        {
            end = on_sci_call(p, SCI_GETTEXTLENGTH, 0, 0);
        }
        else
        {
            const sptr_t current_pos = on_sci_call(p, SCI_GETCURRENTPOS, 0, 0);
            const sptr_t current_line = on_sci_call(p, SCI_LINEFROMPOSITION, current_pos, 0);
            start = on_sci_call(p, SCI_POSITIONFROMLINE, current_line, 0);
            end = on_sci_call(p, SCI_GETLINEENDPOSITION, current_line, 0);
        }
        p->match_count = 0;
        if (!on_command_common_search(p, key, start, end, opt))
        {
            return false;
        }
        on_sci_call(p, SCI_BEGINUNDOACTION, 0, 0);
        do
        {
            if ((result = on_search_replace_target(p, replace, &offset)))
            {
                if (line_first || txt_first)
                {
                    break;
                }
                result = on_search_next(p, key, end + offset, opt);
            }
        } while(result);
        on_sci_call(p, SCI_ENDUNDOACTION, 0, 0);
        return true;
    }
    return false;
}

void
eu_command_jump(const int t, const intptr_t line)
{
    const eu_tabpage *p = t < 0 ? on_tabpage_focus_at() : on_tabpage_get_ptr(t);
    if (p && !p->pmod)
    {
        if (line == -1)
        {
            on_sci_scroll(p);
        }
        else if (line >= 0)
        {
            on_search_jmp_now(p, line);
        }
        SetFocus(p->hwnd_sc);
    }
}

bool
eu_command_xsave(const int t)
{
    eu_tabpage *p = t < 0 ? (eu_tabpage *)on_tabpage_focus_at() : (eu_tabpage *)on_tabpage_get_ptr(t);
    if (p && !p->is_blank && !url_has_remote(p->pathfile))
    {
        struct _stat st = {0};
        struct _utimbuf ut = {0};
        if (_tstat(p->pathfile, &st) == 0)
        {
            ut.actime = st.st_atime;
            ut.modtime = st.st_mtime;
        }
        if (ut.modtime > 0 && on_file_save(p, SAVE_ONLY) == SKYLARK_OK && _tutime(p->pathfile, &ut) == 0)
        {
            p->st_mtime = ut.modtime;
            on_statusbar_update_fileinfo(p, NULL);
            return true;
        }
    }
    return false;
}

bool
eu_command_saveas(const int t, const char *path, const int mode)
{
    wchar_t *u16 = eu_utf8_utf16(path, NULL);
    eu_tabpage *p = t < 0 ? (eu_tabpage *)on_tabpage_focus_at() : (eu_tabpage *)on_tabpage_get_ptr(t);
    if (p && u16)
    {
        p->reserved0 = (intptr_t)util_wstr_unquote(u16);
        return on_file_save(p, mode) == SKYLARK_OK;
    }
    return false;
}

bool
eu_command_convert(const int t, const char *enc)
{
    int index = IDM_UNKNOWN;
    eu_tabpage *p = t < 0 ? (eu_tabpage *)on_tabpage_focus_at() : (eu_tabpage *)on_tabpage_get_ptr(t);
    if (p && !p->is_blank && TAB_HAS_TXT(p) && (index = eu_query_encoding_index(enc)) != IDM_UNKNOWN)
    {
        if (on_statusbar_convert_coding(p, index) == SKYLARK_OK)
        {
            on_statusbar_update_coding(p);
            return true;
        }
    }
    return false;
}

bool
eu_command_reload(const int t, const char *enc)
{
    int index = IDM_UNKNOWN;
    eu_tabpage *p = t < 0 ? (eu_tabpage *)on_tabpage_focus_at() : (eu_tabpage *)on_tabpage_get_ptr(t);
    if (p && !p->is_blank && TAB_HAS_TXT(p) && (index = eu_query_encoding_index(enc)) != IDM_UNKNOWN)
    {
        p->codepage = index;
        on_tabpage_reload_file(p, 2);
        on_statusbar_update_coding(p);
        return true;
    }
    return false;
}

bool
eu_command_tabs_hint(const char *str, const bool focus)
{
    wchar_t *u16 = eu_utf8_utf16(str, NULL);
    eu_tabpage *pnode = on_tabpage_focus_at();
    if (pnode && u16 && util_wstr_unquote(u16))
    {
        int count, index;
        int save = -1;
        for (index = 0, count = TabCtrl_GetItemCount(g_tabpages); index < count; ++index)
        {
            eu_tabpage *p = on_tabpage_get_ptr(index);
            if (p && p != pnode && _tcsicmp(p->filename, u16) == 0)
            {
                save = index;
                break;
            }
        }
        if (save == -1)
        {
            for (index = 0, count = TabCtrl_GetItemCount(g_tabpages); index < count; ++index)
            {
                eu_tabpage *p = on_tabpage_get_ptr(index);
                if (p && p != pnode && _tcsnicmp(p->filename, u16, _tcslen(u16)) == 0)
                {
                    save = index;
                    break;
                }
            }
        }
        if (save == -1)
        {
            for (index = 0, count = TabCtrl_GetItemCount(g_tabpages); index < count; ++index)
            {
                eu_tabpage *p = on_tabpage_get_ptr(index);
                if (p && p != pnode && eu_tcasestr(p->filename, u16))
                {
                    save = index;
                    break;
                }
            }
        }
        if (save != -1)
        {
            if (focus)
            {
                TabCtrl_SetCurFocus(g_tabpages, save);
                InvalidateRect(g_tabpages, NULL, TRUE);
            }
            else
            {
                on_tabpage_active_one(save);
            }
            return true;
        }
    }
    return false;
}

void
eu_command_which(const char *str)
{
    char *path = NULL;
    char msg[MAX_PATH] = {0};
    if (eu_which(str, &path))
    {
        LOAD_I18N_RESSTR(IDS_WHICH_EXIST, info);
        on_result_append_text_utf8(util_make_u8(info, msg, MAX_PATH), path);
    }
    else
    {
        LOAD_I18N_RESSTR(IDS_WHICH_NONE, info);
        on_result_append_text_utf8(util_make_u8(info, msg, MAX_PATH), str);
    }
}