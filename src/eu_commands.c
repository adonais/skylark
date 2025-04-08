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
eu_command_convert(const int t, const char *enc)
{
    int index = IDM_UNKNOWN;
    eu_tabpage *p = t < 0 ? (eu_tabpage *)on_tabpage_focus_at() : (eu_tabpage *)on_tabpage_get_ptr(t);
    if (!p->is_blank && TAB_HAS_TXT(p) && (index = eu_query_encoding_index(enc)) != IDM_UNKNOWN)
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
    if (!p->is_blank && TAB_HAS_TXT(p) && (index = eu_query_encoding_index(enc)) != IDM_UNKNOWN)
    {
        p->codepage = index;
        on_tabpage_reload_file(p, 2);
        on_statusbar_update_coding(p);
        return true;
    }
    return false;
}
