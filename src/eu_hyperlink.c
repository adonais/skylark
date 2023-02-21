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

#define HYPLNK_REGEX_VALID_STR "a-zA-Z0-9\\u00A0-\\uD7FF\\uF900-\\uFDCF\\uFDF0-\\uFFEF+\\*\\[\\];^°+§&@#/%=~_|'"
#define HYPLNK_REGEX_FULL      "\\b(?:(?:https?|ftp|file)://|www\\.|ftp\\.)"                                      \
                               "(?:\\([-" HYPLNK_REGEX_VALID_STR "?!:,.]*\\)|[-" HYPLNK_REGEX_VALID_STR "?!:,.])*"\
                               "(?:\\([-" HYPLNK_REGEX_VALID_STR "?!:,.]*\\)|[-" HYPLNK_REGEX_VALID_STR "])"

static void
on_hyper_clear_indicator(eu_tabpage *pnode, const int i1, const int i2, const sptr_t start, const int len)
{
    if (pnode)
    {
        eu_sci_call(pnode, SCI_SETINDICATORCURRENT, i1, 0);
        eu_sci_call(pnode, SCI_INDICATORCLEARRANGE, start, len);
        eu_sci_call(pnode, SCI_SETINDICATORCURRENT, i2, 0);
        eu_sci_call(pnode, SCI_INDICATORCLEARRANGE, start, len);
    }
}

static void
on_hyper_add_indicator(eu_tabpage *pnode, const int i1, const int i2, const sptr_t start, const int len)
{
    if (pnode)
    {
        eu_sci_call(pnode, SCI_SETINDICATORCURRENT, i1, 0);
        eu_sci_call(pnode, SCI_INDICATORFILLRANGE, start, len);
        eu_sci_call(pnode, SCI_SETINDICATORCURRENT, i2, 0);
        eu_sci_call(pnode, SCI_INDICATORFILLRANGE, start, len);
    }
}

static unsigned WINAPI
on_hyper_set(void *lp)
{
    eu_tabpage *pnode = (eu_tabpage *) lp;
    if (pnode)
    {
        sptr_t pos = 0;
        sptr_t fpos = -1;
        sptr_t max_pos = eu_sci_call(pnode, SCI_GETTEXTLENGTH, 0, 0);
        while (pos < max_pos && (fpos = on_search_process_find(pnode, HYPLNK_REGEX_FULL, pos, max_pos, SCFIND_REGEXP | SCFIND_POSIX)) >= 0)
        {
            int len = eu_int_cast(eu_sci_call(pnode, SCI_GETTARGETEND, 0, 0) - fpos);
            if (len > 0)
            {
                on_hyper_clear_indicator(pnode, INDIC_SKYLARK_HYPER, INDIC_SKYLARK_HYPER_U, fpos, len);
                on_hyper_add_indicator(pnode, INDIC_SKYLARK_HYPER, INDIC_SKYLARK_HYPER_U, fpos, len);
                pos = fpos + len;
            }
            else
            {
                pos = max_pos;
            }
        }
    }
    _InterlockedExchange64(&pnode->hyper_id, 0);
    printf("hyperlink_thread exit\n");
    return 0;
}

static unsigned WINAPI
on_hyper_clear(void *lp)
{
    eu_tabpage *p = NULL;
    for (int index = 0, count = TabCtrl_GetItemCount(g_tabpages); index < count; ++index)
    {
        TCITEM tci = {TCIF_PARAM};
        TabCtrl_GetItem(g_tabpages, index, &tci);
        if ((p = (eu_tabpage *) (tci.lParam)) && !_InterlockedCompareExchange64(&p->hyper_id, 1, 0))
        {
            sptr_t pos = 0;
            sptr_t fpos = -1;
            sptr_t max_pos = eu_sci_call(p, SCI_GETTEXTLENGTH, 0, 0);
            while (pos < max_pos && (fpos = on_search_process_find(p, HYPLNK_REGEX_FULL, pos, max_pos, SCFIND_REGEXP | SCFIND_POSIX)) >= 0)
            {
                int len = eu_int_cast(eu_sci_call(p, SCI_GETTARGETEND, 0, 0) - fpos);
                if (len > 0)
                {
                    on_hyper_clear_indicator(p, INDIC_SKYLARK_HYPER, INDIC_SKYLARK_HYPER_U, fpos, len);
                    pos = fpos + len;
                }
                else
                {
                    pos = max_pos;
                }
            }
            _InterlockedExchange64(&p->hyper_id, 0);
        }
    }
    return 0;
}

void
on_hyper_update_style(eu_tabpage *pnode)
{
    if (pnode && !pnode->hyper_id)
    {
        const sptr_t start_line = eu_sci_call(pnode, SCI_DOCLINEFROMVISIBLE, (eu_sci_call(pnode, SCI_GETFIRSTVISIBLELINE, 0, 0)), 0);
        const sptr_t end_line = min(start_line + eu_sci_call(pnode, SCI_LINESONSCREEN, 0, 0), eu_sci_call(pnode, SCI_GETLINECOUNT, 0, 0) - 1);
        sptr_t pos = eu_sci_call(pnode, SCI_POSITIONFROMLINE, start_line, 0);
        sptr_t fpos = -1;
        const sptr_t max_pos = eu_sci_call(pnode, SCI_GETLINEENDPOSITION, end_line, 0);
        while (pos < max_pos && (fpos = on_search_process_find(pnode, HYPLNK_REGEX_FULL, pos, max_pos, SCFIND_REGEXP | SCFIND_POSIX)) >= 0)
        {
            int len = eu_int_cast(eu_sci_call(pnode, SCI_GETTARGETEND, 0, 0) - fpos);
            if (len > 0)
            {
                on_hyper_clear_indicator(pnode, INDIC_SKYLARK_HYPER, INDIC_SKYLARK_HYPER_U, fpos, len);
                on_hyper_add_indicator(pnode, INDIC_SKYLARK_HYPER, INDIC_SKYLARK_HYPER_U, fpos, len);
                pos = fpos + len;
            }
            else
            {
                pos = max_pos;
            }
        }
    }
}

void
on_hyper_set_style(eu_tabpage *pnode)
{
    if (pnode && eu_get_theme())
    {
        uint32_t fix_color = eu_get_theme()->item.hyperlink.color;
        uint32_t hover_color = eu_get_theme()->item.hyperlink.bgcolor;
        const sptr_t alpha1 = (fix_color & 0xFF000000) >> 24;
        const sptr_t alpha2 = (hover_color & 0xFF000000) >> 24;
        eu_sci_call(pnode, SCI_INDICSETALPHA, INDIC_SKYLARK_HYPER_U, alpha1);
        eu_sci_call(pnode, SCI_INDICSETOUTLINEALPHA, INDIC_SKYLARK_HYPER_U, alpha2);
        // 超链接显示样式
        eu_sci_call(pnode, SCI_INDICSETSTYLE, INDIC_SKYLARK_HYPER, INDIC_TEXTFORE);
        eu_sci_call(pnode, SCI_INDICSETFORE, INDIC_SKYLARK_HYPER, fix_color & 0x00FFFFFF);
        eu_sci_call(pnode, SCI_INDICSETSTYLE, INDIC_SKYLARK_HYPER_U, INDIC_PLAIN);
        eu_sci_call(pnode, SCI_INDICSETFORE, INDIC_SKYLARK_HYPER_U, fix_color & 0x00FFFFFF);
        // 鼠标指向时的样式
        eu_sci_call(pnode, SCI_INDICSETHOVERSTYLE, INDIC_SKYLARK_HYPER, INDIC_TEXTFORE);
        eu_sci_call(pnode, SCI_INDICSETHOVERFORE, INDIC_SKYLARK_HYPER, hover_color & 0x00FFFFFF);
        eu_sci_call(pnode, SCI_INDICSETHOVERSTYLE, INDIC_SKYLARK_HYPER_U, INDIC_PLAIN);
        eu_sci_call(pnode, SCI_INDICSETHOVERFORE, INDIC_SKYLARK_HYPER_U, hover_color & 0x00FFFFFF);
    }
}

void
on_hyper_click(eu_tabpage *pnode, HWND hwnd, const sptr_t position)
{
    if (eu_sci_call(pnode, SCI_INDICATORVALUEAT, INDIC_SKYLARK_HYPER, position) >= 0)
    {
        sptr_t fpos = -1;
        const sptr_t pos = eu_sci_call(pnode, SCI_INDICATORSTART, INDIC_SKYLARK_HYPER, position);
        const sptr_t max_pos = eu_sci_call(pnode, SCI_INDICATOREND, INDIC_SKYLARK_HYPER, position);
        if (pos < max_pos && (fpos = on_search_process_find(pnode, HYPLNK_REGEX_FULL, pos, max_pos, SCFIND_REGEXP | SCFIND_POSIX)) >= 0)
        {
            char buffer[LARGER_LEN+1] = {0};
            const sptr_t end_pos = eu_sci_call(pnode, SCI_GETTARGETEND, 0, 0);
            int len = eu_int_cast(end_pos - fpos);
            if (pos == fpos && len >= 4 && len < LARGER_LEN)
            {
                Sci_TextRange tr = {{pos, end_pos}, buffer};
                eu_sci_call(pnode, SCI_GETTEXTRANGE, 0, (sptr_t) &tr);
            }
            wchar_t *text = *buffer ? eu_utf8_utf16(buffer, NULL) : NULL;
            if (text)
            {
                SendMessage(pnode->hwnd_sc, WM_KEYDOWN, VK_ESCAPE, 0);
                ShellExecuteW(hwnd, L"open", text, NULL, NULL, SW_SHOWNORMAL);
            }
        }
    }
}

void
on_hyper_clear_style(void)
{
    CloseHandle((HANDLE) _beginthreadex(NULL, 0, &on_hyper_clear, NULL, 0, NULL));
}

int
eu_hyperlink_detection(eu_tabpage *pnode)
{
    if (pnode && !pnode->hyper_id)
    {
        CloseHandle((HANDLE) _beginthreadex(NULL, 0, &on_hyper_set, pnode, 0, (uint32_t *)&pnode->hyper_id));
    }
    return 0;
}
