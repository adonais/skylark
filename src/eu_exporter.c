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

static sci_export *
on_exporter_new(void)
{
    sci_export *px = (sci_export *) calloc(1, sizeof(sci_export));
    if (px)
    {
        if (!(px->pstyle = (style_export *) calloc(sizeof(style_export), NRSTYLES)))
        {
            free(px);
            px = NULL;
        }
    }
    return px;
}

static void
on_exporter_destory(sci_export **p)
{
    if (p && *p)
    {
        eu_safe_free((*p)->pstyle);
        eu_safe_free((*p)->data);
        eu_safe_free(*p);
    }
}

static bool
on_exporter_init_data(eu_tabpage *pnode, sci_export *ptr, const sptr_t start, const sptr_t end)
{
    if (pnode && ptr && end > start)
    {
        int i = 0;
        int prev_style = -1;
        int current_style = -1;
        const sptr_t len = end - start;
        if (!(ptr->data = (char *) calloc(1, len * 2 + 2)))
        {
            return false;
        }
        ptr->nschar = eu_int_cast(len);
        ptr->tabsize = (int) eu_sci_call(pnode, SCI_GETTABWIDTH, 0, 0);
        Sci_TextRangeFull tr = { { start, end }, ptr->data };
        eu_sci_call(pnode, SCI_GETSTYLEDTEXTFULL, 0, (sptr_t) &tr);
        ptr->nrstyle_switches = 0;
        ptr->nr_ustyles = 1;
        ptr->font_length = 0;
        ptr->ustyles[STYLE_DEFAULT] = true;
        eu_sci_call(pnode, SCI_STYLEGETFONT, STYLE_DEFAULT, (sptr_t) (ptr->pstyle[STYLE_DEFAULT].fontname));
        ptr->font_length += (int) strlen((ptr->pstyle[STYLE_DEFAULT].fontname));
        ptr->pstyle[STYLE_DEFAULT].size = (int) eu_sci_call(pnode, SCI_STYLEGETSIZE, STYLE_DEFAULT, 0);
        ptr->pstyle[STYLE_DEFAULT].bold = (int) eu_sci_call(pnode, SCI_STYLEGETBOLD, STYLE_DEFAULT, 0);
        ptr->pstyle[STYLE_DEFAULT].italic = (int) eu_sci_call(pnode, SCI_STYLEGETITALIC, STYLE_DEFAULT, 0);
        ptr->pstyle[STYLE_DEFAULT].underlined = (int) eu_sci_call(pnode, SCI_STYLEGETUNDERLINE, STYLE_DEFAULT, 0);
        ptr->pstyle[STYLE_DEFAULT].fgcolor = (int) eu_sci_call(pnode, SCI_STYLEGETFORE, STYLE_DEFAULT, 0);
        ptr->pstyle[STYLE_DEFAULT].bgcolor = (int) eu_sci_call(pnode, SCI_STYLEGETBACK, STYLE_DEFAULT, 0);
        ptr->pstyle[STYLE_DEFAULT].eol_ext = (bool) eu_sci_call(pnode, SCI_STYLEGETEOLFILLED, STYLE_DEFAULT, 0);
        for (i = 0; i < (int) len; ++i)
        {
            current_style = ptr->data[i * 2 + 1];
            if (current_style != prev_style)
            {
                prev_style = current_style;
                ptr->nrstyle_switches++;
            }
            if (current_style >= 0 && current_style < NRSTYLES && ptr->ustyles[current_style] == false)
            {
                ptr->nr_ustyles++;
                eu_sci_call(pnode, SCI_STYLEGETFONT, current_style, (sptr_t) (ptr->pstyle[current_style].fontname));
                ptr->font_length += (int) strlen((ptr->pstyle[current_style].fontname));
                ptr->pstyle[current_style].size = (int) eu_sci_call(pnode, SCI_STYLEGETSIZE, current_style, 0);
                ptr->pstyle[current_style].bold = (int) eu_sci_call(pnode, SCI_STYLEGETBOLD, current_style, 0);
                ptr->pstyle[current_style].italic = (int) eu_sci_call(pnode, SCI_STYLEGETITALIC, current_style, 0);
                ptr->pstyle[current_style].underlined = (int) eu_sci_call(pnode, SCI_STYLEGETUNDERLINE, current_style, 0);
                ptr->pstyle[current_style].fgcolor = (int) eu_sci_call(pnode, SCI_STYLEGETFORE, current_style, 0);
                ptr->pstyle[current_style].bgcolor = (int) eu_sci_call(pnode, SCI_STYLEGETBACK, current_style, 0);
                ptr->pstyle[current_style].eol_ext = (bool) eu_sci_call(pnode, SCI_STYLEGETEOLFILLED, current_style, 0);
                ptr->ustyles[current_style] = true;
            }
        }
        if (true)
        {
            TCHAR font_str[DW_SIZE] = { 0 };
            HDC hdc = GetDC(pnode->hwnd_sc);
            // font magic
            int pplix = GetDeviceCaps(hdc, LOGPIXELSX);
            int ppliy = GetDeviceCaps(hdc, LOGPIXELSY);
            int height = -MulDiv(ptr->pstyle[STYLE_DEFAULT].size, ppliy, 72);
            
            util_make_u16(ptr->pstyle[STYLE_DEFAULT].fontname, font_str, DW_SIZE);
            HFONT font = CreateFont(height, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, font_str);
            HGDIOBJ old = SelectObject(hdc, font);
            SIZE size = { 8 }; // fallback, 8 pix default
            GetTextExtentPoint32(hdc, TEXT(" "), 1, &size);
            int twips = size.cx * (1440 / pplix);
            SelectObject(hdc, old);
            DeleteObject(font);
            ReleaseDC(pnode->hwnd_sc, hdc);
            // We now have the amount of twips per space in the default font
            ptr->twips = twips;
        }
        return true;
    }
    return false;
}

static char *
on_exporter_get(eu_tabpage *pnode, sci_export *ptr)
{
    char *pcli = NULL;
    if (pnode && ptr && ptr->data)
    {
        int i = 0;
        int offset = 0;
        int tx_size = 0;
        int font_index = 0;
        int color_index = 0;
        uint8_t tch = 0;
        style_export *pcurrent;
        char *buffer = ptr->data;
        size_t total = EXPORT_SIZE_RTF_STATIC + EXPORT_SIZE_RTF_STYLE * ptr->nr_ustyles + ptr->font_length +
                       EXPORT_SIZE_RTF_SWITCH * ptr->nrstyle_switches + 1;
        for (i = 0; i < ptr->nschar; ++i)
        {
            tch = buffer[(i * 2)];
            switch (tch)
            {
                case '{':
                    total += 2; // '\{'
                    break;
                case '}':
                    total += 2; // '\}'
                    break;
                case '\\':
                    total += 2; // '\\'
                    break;
                case '\t':
                    total += 5; // '\tab '
                    break;
                case '\r':
                    if (buffer[(i + 1) * 2] == '\n')
                    {
                        break;
                    }
                case '\n':
                    total += 6; // '\par\r\n'
                    break;
                default:
                    if (tch < 0x80)
                    {
                        total += 1; // 'char'
                    }
                    else
                    {
                        total += 8; // '\u#####?
                        ++i;
                        if (tch >= 0xE0)
                        {
                            ++i;
                        }
                    }
                    break;
            }
        }
        if (!(pcli = (char *) calloc(1, total + 1)))
        {
            return NULL;
        }
        tx_size = ptr->tabsize * ptr->twips;
        offset += sprintf(pcli + offset, "{\\rtf1\\ansi\\deff0\\deftab%u\r\n\r\n", (unsigned) (tx_size));
        offset += sprintf(pcli + offset, "{\\fonttbl\r\n");
        offset += sprintf(pcli + offset, "{\\f%03d %s;}\r\n", font_index, (ptr->pstyle + STYLE_DEFAULT)->fontname);
        ++font_index;
        for (i = 0; i < NRSTYLES; ++i)
        {
            if (i == STYLE_DEFAULT)
            {
                continue;
            }
            if (ptr->ustyles[i] == true)
            {
                pcurrent = (ptr->pstyle) + i;
                offset += sprintf(pcli + offset, "{\\f%03d %s;}\r\n", font_index, pcurrent->fontname);
                if (!strcmp(pcurrent->fontname, (ptr->pstyle + STYLE_DEFAULT)->fontname))
                {
                    pcurrent->ifont = (ptr->pstyle + STYLE_DEFAULT)->ifont;
                }
                else
                {
                    pcurrent->ifont = font_index;
                }
                ++font_index;
            }
        }
        offset += sprintf(pcli + offset, "}\r\n\r\n"); // fonttbl
        offset += sprintf(pcli + offset, "{\\colortbl\r\n");
        for (i = 0; i < NRSTYLES; ++i)
        {
            if (ptr->ustyles[i])
            {
                pcurrent = (ptr->pstyle) + i;

                offset += sprintf(pcli + offset,
                                  "\\red%03d\\green%03d\\blue%03d;\r\n",
                                  (pcurrent->fgcolor >> 0) & 0xFF,
                                  (pcurrent->fgcolor >> 8) & 0xFF,
                                  (pcurrent->fgcolor >> 16) & 0xFF);
                pcurrent->fg_clr = color_index++;
                offset += sprintf(pcli + offset,
                                  "\\red%03d\\green%03d\\blue%03d;\r\n",
                                  (pcurrent->bgcolor >> 0) & 0xFF,
                                  (pcurrent->bgcolor >> 8) & 0xFF,
                                  (pcurrent->bgcolor >> 16) & 0xFF);
                pcurrent->bg_clr = color_index++;
            }
        }
        offset += sprintf(pcli + offset, "}\r\n\r\n"); // colortbl
        if (true)
        { //-------Dump text to RTF
            uint8_t ch;
            int last_style = -1;
            int prev_style = STYLE_DEFAULT;
            int buffer_style = STYLE_DEFAULT;
            style_export *styles = ptr->pstyle;
            utf16 uvalue = { 0 };
            // print default style information
            offset += sprintf(pcli + offset,
                              "\\f%d\\fs%d\\cb%d\\cf%d ",
                              styles[STYLE_DEFAULT].ifont,
                              styles[STYLE_DEFAULT].size * 2,
                              styles[STYLE_DEFAULT].bg_clr,
                              styles[STYLE_DEFAULT].fg_clr);

            for (i = 0; i < ptr->nschar; ++i)
            {
                ch = buffer[(i * 2)];
                buffer_style = buffer[(i * 2) + 1];
                // print new style info if style changes
                if (last_style != buffer_style)
                {
                    if (last_style != -1)
                    {
                        prev_style = last_style;
                    }
                    last_style = buffer_style;
                    if (styles[last_style].ifont != styles[prev_style].ifont)
                    {
                        offset += sprintf(pcli + offset, "\\f%d", styles[last_style].ifont);
                    }
                    if (styles[last_style].size != styles[prev_style].size)
                    {
                        offset += sprintf(pcli + offset, "\\fs%d", styles[last_style].size * 2);
                    }
                    if (styles[last_style].bg_clr != styles[prev_style].bg_clr)
                    {
                        offset += sprintf(pcli + offset, "\\highlight%d", styles[last_style].bg_clr);
                    }
                    if (styles[last_style].fg_clr != styles[prev_style].fg_clr)
                    {
                        offset += sprintf(pcli + offset, "\\cf%d", styles[last_style].fg_clr);
                    }
                    if (styles[last_style].bold != styles[prev_style].bold)
                    {
                        if (styles[last_style].bold)
                        {
                            offset += sprintf(pcli + offset, "\\b");
                        }
                        else
                        {
                            offset += sprintf(pcli + offset, "\\b0");
                        }
                    }
                    if (styles[last_style].italic != styles[prev_style].italic)
                    {
                        if (styles[last_style].underlined)
                        {
                            offset += sprintf(pcli + offset, "\\i");
                        }
                        else
                        {
                            offset += sprintf(pcli + offset, "\\i0");
                        }
                    }
                    if (styles[last_style].underlined != styles[prev_style].underlined)
                    {
                        if (styles[last_style].underlined)
                        {
                            offset += sprintf(pcli + offset, "\\ul");
                        }
                        else
                        {
                            offset += sprintf(pcli + offset, "\\ul0");
                        }
                    }
                    offset += sprintf(pcli + offset, " ");
                }
                // print character, parse special ones
                switch (ch)
                {
                    case '{':
                        offset += sprintf(pcli + offset, "\\{");
                        break;
                    case '}':
                        offset += sprintf(pcli + offset, "\\}");
                        break;
                    case '\\':
                        offset += sprintf(pcli + offset, "\\\\");
                        break;
                    case '\t':
                        offset += sprintf(pcli + offset, "\\tab ");
                        break;
                    case '\r':
                        if (buffer[(i * 2) + 2] == '\n')
                        {
                            break;
                        }
                    case '\n':
                        offset += sprintf(pcli + offset, "\\par\r\n");
                        break;
                    default:
                        if (ch < 0x20) // ignore control characters
                        {
                            break;
                        }
                        if (ch > 0x7F)
                        { // this may be some UTF-8 character, so parse it as such
                            uvalue.value = 0;
                            if (ch < 0xE0)
                            {
                                uvalue.value = ((0x1F & ch) << 6);
                                ++i;
                                ch = buffer[(i * 2)];
                                uvalue.value |= (0x3F & ch);
                            }
                            else
                            {
                                uvalue.value = ((0xF & ch) << 12);
                                ++i;
                                ch = buffer[(i * 2)];
                                uvalue.value |= ((0x3F & ch) << 6);
                                ++i;
                                ch = buffer[(i * 2)];
                                uvalue.value |= (0x3F & ch);
                            }
                            offset += sprintf(pcli + offset, "\\u%d?", uvalue.value); // signed values
                        }
                        else
                        {
                            offset += sprintf(pcli + offset, "%c", ch);
                        }
                        break;
                }
            }
            offset += sprintf(pcli + offset, "}\r\n"); // rtf/ansi
        }
    }
    return pcli;
}

char *
on_exporter_rtf(eu_tabpage *pnode, const sptr_t start, const sptr_t end)
{
    char *pcli = NULL;
    if (pnode && end > start)
    {
        sci_export *ptr_exp = NULL;
        if (!(ptr_exp = on_exporter_new()))
        {
            return NULL;
        }
        if (on_exporter_init_data(pnode, ptr_exp, start, end))
        {
            pcli = on_exporter_get(pnode, ptr_exp);
        }
        on_exporter_destory(&ptr_exp);
    }
    return pcli;
}
