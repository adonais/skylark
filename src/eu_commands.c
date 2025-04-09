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
on_command_find(eu_tabpage *pnode, const char *key, const uint32_t opt)
{
    sptr_t start_pos = 0;
    sptr_t end_pos = 0;
    sptr_t found_pos = -1;
    sptr_t target_end = -1;
    uint32_t find_flags = (uint32_t)opt;
    bool m_loop = true;
    bool reverse = opt & SCCMD_REVERSE;
    sptr_t pos = on_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
    sptr_t max_pos = on_sci_call(pnode, SCI_GETTEXTLENGTH, 0, 0);
    if (reverse)
    {
        find_flags &= ~SCCMD_REVERSE;
        start_pos = pos - 1;
        if (start_pos > on_sci_call(pnode, SCI_GETSELECTIONSTART, 0, 0))
        {
            start_pos = on_sci_call(pnode, SCI_GETSELECTIONSTART, 0, 0);
        }
    }
    else
    {
        start_pos = pos;
        end_pos = max_pos;
        if (start_pos == on_sci_call(pnode, SCI_GETSELECTIONSTART, 0, 0))
        {
            start_pos = on_sci_call(pnode, SCI_GETSELECTIONEND, 0, 0);
        }
    }
    DO_COMMANDS_LOOP:
    {
        found_pos = on_search_process_find(pnode, key, start_pos, end_pos, find_flags);
        if (found_pos >= 0)
        {
            SetFocus(pnode->hwnd_sc);
            target_end = on_sci_call(pnode, SCI_GETTARGETEND, 0, 0);
            on_sci_call(pnode, SCI_SETSEL, found_pos, target_end);
        }
        else if (found_pos == -2)
        {
            eu_logmsg("Cmds: re error\n");
        }
        else if (m_loop)
        {
            m_loop = false;
            if (reverse)
            {
                start_pos = max_pos;
                end_pos = pos;
            }
            else
            {
                start_pos = 0;
                end_pos = pos;
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
on_command_launch(void)
{
    eu_tabpage *p = on_tabpage_focus_at();
    if (p && !p->is_blank && TAB_HAS_TXT(p))
    {
        if (RESULT_SHOW(p) && p->presult->pwant == on_command_light)
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
            eu_window_resize();
        }
    }
}

bool
on_command_search(const int t, const char *key, const uint32_t opt)
{
    eu_tabpage *p = t < 0 ? (eu_tabpage *)on_tabpage_focus_at() : (eu_tabpage *)on_tabpage_get_ptr(t);
    if (p && TAB_HAS_TXT(p))
    {
        if (on_command_find(p, key, opt))
        {
            eu_get_config()->last_flags = opt & ~SCCMD_REVERSE;
            return true;
        }
    }
    return false;
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
    size_t len = 0;
    wchar_t *u16 = eu_utf8_utf16(path, &len);
    eu_tabpage *p = t < 0 ? (eu_tabpage *)on_tabpage_focus_at() : (eu_tabpage *)on_tabpage_get_ptr(t);
    if (p && u16 && len > 0)
    {
        p->reserved0 = (intptr_t)util_wstr_unquote(u16, (const int)len);
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
