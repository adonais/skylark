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

#define HEXEDIT_MODE_FIRST32LINE1 _T("Offset(H)| 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F |     UTF-8      \0")
#define HEXEDIT_MODE_FIRST32LINE2 _T("Offset(H)| 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F |   ANSI ASCII   \0")
#define HEXEDIT_MODE_SECOND32LINE _T("---------+------------------------------------------------+----------------\0")

#define HEXEDIT_MODE_FIRST64LINE1 _T("    Offset(H)    | 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F |     UTF-8      \0")
#define HEXEDIT_MODE_FIRST64LINE2 _T("    Offset(H)    | 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F |   ANSI ASCII   \0")
#define HEXEDIT_MODE_SECOND64LINE _T("-----------------+------------------------------------------------+----------------\0")

static int hex_area = 0;
static volatile long hex_zoom = 0;
static volatile long affected_switch = 0;

/*******************************************
 * 计算utf8编码字节数
 * 返回1不是有效的utf8首字节
 * 返回0, 是 ascii 编码
 * 返回 2, 3, 4, 5, 6代表utf8字符的字节数
 ******************************************/
static int
hexview_builtin_clz(uint8_t ch)
{
    int num = 0;
    uint8_t type = ch;
    type &= 0xfe;        //确保最低位为0, 结束循环
    while (type & 0x80)  //检测最高位是不是1。
    {
        ++num;
        type <<= 1;
    }
    return num;
}

static int
hexview_is_utf8(const uint8_t *ptext, int pos)
{
    int num = 0;
    if ((num = hexview_builtin_clz(ptext[0])) == 1)
    {
        num = 0;
    }
    if (num + pos > 16)
    {
        return 0;
    }
    return num;
}

void
hexview_set_area(int value)
{
    hex_area = value;
}

static void
hexview_draw_line(HWND hwnd, HDC mem_hdc, PHEXVIEW hexview, int line_number)
{
    RECT rc = {0};
    // 清除内存dc
    rc.right = hexview->width_view;
    rc.bottom = hexview->height_char;
    ExtTextOut(mem_hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
    // 因为加上了2辅助行
    if ((hexview->vscroll_pos + line_number + 2) < hexview->totallines)
    {
        NMHVDISPINFO dispinfo = {0};
        TCHAR buffer[32];
        int pos1, pos2, num, x;
        size_t select_start, select_end;
        size_t i, number_items;
        COLORREF clr_bk;

        // address column
        number_items = (hexview->vscroll_pos + line_number) * 16;
        dispinfo.item.mask = HVIF_ADDRESS;
        dispinfo.item.number_items = number_items;
        eu_send_notify(hwnd, HVN_GETDISPINFO, (LPNMHDR) &dispinfo);
        SetTextColor(mem_hdc, hexview->clr_text);
        _sntprintf(buffer, _countof(buffer), (hexview->ex_style & HVS_ADDRESS64) ? _T("%016I64X") : _T("%08X"), dispinfo.item.address);
        x = hexview->width_char * (0 - hexview->hscroll_pos);
        TextOut(mem_hdc, x, 0, (PCTSTR) buffer, (int) _tcslen(buffer));
        // DATA and VALUE columns
        pos1 = hexview->line1;
        pos2 = hexview->line2;

        select_start = min(hexview->select_start, hexview->select_end);
        select_end = max(hexview->select_start, hexview->select_end);
        for (i = number_items; i <= number_items + 15 && i < hexview->total_items; ++i)
        {

            dispinfo.item.mask = HVIF_BYTE;
            dispinfo.item.state = 0;
            dispinfo.item.number_items = i;
            dispinfo.item.address = 0;
            dispinfo.item.value = 0;
            eu_send_notify(hwnd, HVN_GETDISPINFO, (LPNMHDR) &dispinfo);
            if ((hexview->ct_flags & HVF_SELECTED) && (i >= select_start && i <= select_end))
            {
                clr_bk = SetBkColor(mem_hdc, hexview->clr_bg_selected);
                if (i != select_end && i != number_items + 15)
                {
                    rc.top = 0;
                    rc.bottom = hexview->height_view;
                    rc.left = pos1 + hexview->width_char * 2;
                    rc.right = rc.left + hexview->width_char;
                    ExtTextOut(mem_hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
                }
            }
            else
            {
                clr_bk = SetBkColor(mem_hdc, hexview->clr_bg_text);
            }
            if (dispinfo.item.state & HVIS_MODIFIED)
            {
                SetTextColor(mem_hdc, hexview->clr_modify_text);
            }
            else
            {
                SetTextColor(mem_hdc, hexview->clr_text);
            }
            // 绘制16进制值
            uint8_t *ptext = (uint8_t *)(hexview->pbase + i);
            num = hexview_is_utf8(ptext, i%16);
            if (hexview->hex_ascii)
            {
                _sntprintf(buffer, _countof(buffer), _T("%02X"), dispinfo.item.value);
                num = 1;
            }
            else if (num > 1)
            {
                for (int k = 0; k < num; ++k)
                {
                    _stprintf(buffer + (3 * k), _T("%02X "), ptext[k]);
                }
                if (num + i%16 == 16)
                {
                    buffer[_tcslen(buffer)-1] = 0;
                }
            }
            else
            {
                _sntprintf(buffer, _countof(buffer), _T("%02X"), dispinfo.item.value);
                num = 1;
            }
            TextOut(mem_hdc, pos1, 0, (PCTSTR) buffer, num > 1 ? (int)_tcslen(buffer) : 2);
            // 绘制文本数据
            if (isprint(dispinfo.item.value))
            {
                _sntprintf(buffer, _countof(buffer), _T("%c"), dispinfo.item.value);
                num = 1;
            }
            else if (hexview->hex_ascii)
            {
                _tcsncpy(buffer, _T("."), _countof(buffer));
                num = 1;
            }
            else if (num > 1)
            {
                char pin[8] = {0};
                memcpy(pin, ptext, num);
                if (MultiByteToWideChar(CP_UTF8, 0, pin, -1, buffer, _countof(buffer)-1) > 0)
                {
                    i += num - 1;
                }
            }
            else
            {
                _tcsncpy(buffer, _T("."), _countof(buffer));
                num = 1;
            }
            TextOut(mem_hdc, pos2, 0, (PCTSTR) buffer, num > 1 ? (int)_tcslen(buffer) : 1);
            SetBkColor(mem_hdc, clr_bk);
            pos1 = pos1 + hexview->width_char * 3 * num;
            pos2 = pos2 + (num >1 ? hexview->width_char * 2 : hexview->width_char);
        }
    }
}

static void
hexview_paint(HWND hwnd, HDC hdc, PHEXVIEW hexview)
{
    HDC mem_hdc;
    HBITMAP hbm_mem;
    HANDLE hold;
    HANDLE hold_font;
    COLORREF clr_bk;
    int line_number;
    int add_line = 0;
    RECT rect = {0};
    UNREFERENCED_PARAMETER(hwnd);
    mem_hdc = CreateCompatibleDC(hdc);
    hbm_mem = CreateCompatibleBitmap(hdc, hexview->width_view, hexview->height_char);

    hold = SelectObject(mem_hdc, hbm_mem);
    hold_font = SelectObject(mem_hdc, hexview->hfont);
    clr_bk = SetBkColor(mem_hdc, hexview->clr_bg_text);
    if (hexview->total_items > 0)
    {
        SetTextColor(mem_hdc, hexview->clr_text);
        rect.right = hexview->width_view;
        rect.bottom = hexview->height_char;
        ExtTextOut(mem_hdc, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);
        if (hexview->ex_style & HVS_ADDRESS64)
        {
            if (hexview->hex_ascii)
            {
                TextOut(mem_hdc, 0, 0, HEXEDIT_MODE_FIRST64LINE2, (int) _tcslen(HEXEDIT_MODE_FIRST64LINE2));
            }
            else
            {
                TextOut(mem_hdc, 0, 0, HEXEDIT_MODE_FIRST64LINE1, (int) _tcslen(HEXEDIT_MODE_FIRST64LINE1));
            }
            BitBlt(hdc, 0, 0, hexview->width_view, hexview->height_char, mem_hdc, 0, 0, SRCCOPY);
            TextOut(mem_hdc, 0, 0, HEXEDIT_MODE_SECOND64LINE, (int) _tcslen(HEXEDIT_MODE_SECOND64LINE));
            BitBlt(hdc, 0, hexview->height_char, hexview->width_view, hexview->height_char, mem_hdc, 0, 0, SRCCOPY);
        }
        else
        {
            if (hexview->hex_ascii)
            {
                TextOut(mem_hdc, 0, 0, HEXEDIT_MODE_FIRST32LINE2, (int) _tcslen(HEXEDIT_MODE_FIRST32LINE2));
            }
            else
            {
                TextOut(mem_hdc, 0, 0, HEXEDIT_MODE_FIRST32LINE1, (int) _tcslen(HEXEDIT_MODE_FIRST32LINE1));
            }
            BitBlt(hdc, 0, 0, hexview->width_view, hexview->height_char, mem_hdc, 0, 0, SRCCOPY);
            TextOut(mem_hdc, 0, 0, HEXEDIT_MODE_SECOND32LINE, (int) _tcslen(HEXEDIT_MODE_SECOND32LINE));
            BitBlt(hdc, 0, hexview->height_char, hexview->width_view, hexview->height_char, mem_hdc, 0, 0, SRCCOPY);
        }
        add_line = 2;
    }
    for (line_number = 0; line_number <= hexview->visiblelines; line_number++)
    {

        hexview_draw_line(hwnd, mem_hdc, hexview, line_number);
        BitBlt(hdc, 0, (add_line * hexview->height_char) + (hexview->height_char * line_number), hexview->width_view, hexview->height_char, mem_hdc, 0, 0, SRCCOPY);
    }
    SetBkColor(mem_hdc, clr_bk);
    SelectObject(mem_hdc, hold_font);
    SelectObject(mem_hdc, hold);
    DeleteObject(hbm_mem);
    DeleteDC(mem_hdc);
}

static bool
hexview_pin_bottom(PHEXVIEW hexview)
{
    bool ret = false;
    intptr_t visiblelines;
    int visiblechars;
    visiblelines = min(hexview->visiblelines, hexview->totallines);
    visiblechars = min(hexview->visiblechars, hexview->longest_line);
    if (hexview->vscroll_pos + visiblelines > hexview->totallines)
    {
        hexview->vscroll_pos = hexview->totallines - visiblelines;
        ret = true;
    }
    if (hexview->hscroll_pos + visiblechars > hexview->longest_line)
    {
        int hscroll_pos = hexview->longest_line - visiblechars;
        hexview->line1 -= hexview->width_char * (hscroll_pos - hexview->hscroll_pos);
        hexview->line2 -= hexview->width_char * (hscroll_pos - hexview->hscroll_pos);
        hexview->hscroll_pos = hscroll_pos;
        ret = true;
    }
    return ret;
}

static intptr_t
hexview_track_pos(HWND hwnd, PHEXVIEW hexview, int nbar)
{
    SCROLLINFO si = {0};
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_TRACKPOS | SIF_PAGE;
    GetScrollInfo(hwnd, nbar, &si);
    if (nbar == SB_VERT)
    {
        if (hexview->vscroll_max > MAXLONG)
        {
            if (si.nTrackPos == (int) (MAXSHORT - si.nPage + 1))
            {
                return hexview->vscroll_max - si.nPage + 1;
            }
            return (hexview->vscroll_max / MAXSHORT) * si.nTrackPos;
        }
    }
    return si.nTrackPos;
}

static void
hexview_srollinfo(HWND hwnd, PHEXVIEW hexview)
{
    SCROLLINFO si = {0};
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_POS | SIF_PAGE | SIF_RANGE;
    if (hexview->vscroll_max <= MAXLONG)
    {
        si.nPos = (int) hexview->vscroll_pos;
        si.nPage = (uint32_t) hexview->visiblelines;
        si.nMin = 0;
        si.nMax = (int) hexview->vscroll_max;
    }
    else
    {
        si.nPos = (int) (hexview->vscroll_pos / (hexview->vscroll_max / MAXSHORT));
        si.nPage = (uint32_t) hexview->visiblelines;
        si.nMin = 0;
        si.nMax = MAXSHORT;
    }
    SetScrollInfo(hwnd, SB_VERT, &si, true);
    si.nPos = hexview->hscroll_pos;
    si.nPage = (uint32_t) hexview->visiblechars;
    si.nMin = 0;
    si.nMax = hexview->hscroll_max;
    SetScrollInfo(hwnd, SB_HORZ, &si, true);
}

/*************************************************************************************
 *  https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-hidecaret
 *  Hiding is cumulative.
 *  If your application calls HideCaret five times in a row,
 *  it must also call ShowCaret five times before the caret is displayed.
 *  So we need IsCaretVisible in order to hide the caret only once.
**************************************************************************************/
static void
hexview_caret(HWND hwnd, PHEXVIEW hexview)
{
    sptr_t line_number = hexview->number_items / 16 + 2;
    int in_line = (int) (hexview->number_items % 16);
    if (hexview->total_items && line_number >= hexview->vscroll_pos && line_number <= hexview->vscroll_pos + hexview->visiblelines)
    {
        switch (hexview->active_column)
        {
            case COLUMN_DATA:
                SetCaretPos(hexview->line1 + (hexview->width_char * ((in_line * 3) + hexview->hl_position)),
                            (int) (line_number - hexview->vscroll_pos) * hexview->height_char);
                break;

            case COLUMN_VALUE:
                SetCaretPos(hexview->line2 + (hexview->width_char * in_line),
                            (int) (line_number - hexview->vscroll_pos) * hexview->height_char);
                break;
        }
        ShowCaret(hwnd);
        hexview->ct_flags |= HVF_CARETVISIBLE;
    }
    else if (hexview->ct_flags & HVF_CARETVISIBLE)
    {
        HideCaret(hwnd);
        hexview->ct_flags &= ~HVF_CARETVISIBLE;
    }
}

static void
hexview_columns(PHEXVIEW hexview)
{
    if (hexview->ex_style & HVS_ADDRESS64)
    {

        hexview->line1 = hexview->width_char * (NUMBEROF_CHARS_IN_FIRST_COLUMN_64BIT - hexview->hscroll_pos);
        hexview->line2 = hexview->width_char * ((NUMBEROF_CHARS_IN_FIRST_COLUMN_64BIT + NUMBEROF_CHARS_IN_SECOND_COLUMN) - hexview->hscroll_pos);
    }
    else
    {

        hexview->line1 = hexview->width_char * (NUMBEROF_CHARS_IN_FIRST_COLUMN_32BIT - hexview->hscroll_pos);
        hexview->line2 = hexview->width_char * ((NUMBEROF_CHARS_IN_FIRST_COLUMN_32BIT + NUMBEROF_CHARS_IN_SECOND_COLUMN) - hexview->hscroll_pos);
    }
}

static bool
hexview_number_item(PHEXVIEW hexview, int xpos, int ypos)
{
    intptr_t line_number;
    EU_VERIFY(ypos >= 0);
    // 忽略最上面两行
    ypos -= 2 * hexview->height_char;
    if (ypos < 0)
    {
        ypos = 0;
    }
    // 选中的行数, 从0开始计
    line_number = ypos / hexview->height_char;
    if (line_number == hexview->visiblelines)
    {
        line_number--;
    }
    line_number += hexview->vscroll_pos;
    if (line_number >= 0 && line_number < hexview->totallines)
    {
        size_t number_items = line_number * 16;
        intptr_t linenumer_of_item = hexview->total_items - number_items;
        if (linenumer_of_item > 16)
        {
            linenumer_of_item = 16;
        }
        if ((xpos > hexview->line1 - hexview->width_char * 2) &&
            (xpos < (hexview->line1 + ((linenumer_of_item * 3) - 1) * hexview->width_char)))
        {
            xpos = (xpos - hexview->line1) / hexview->width_char;
            hexview->number_items = number_items + ((xpos + 1) / 3);
            hexview->hl_position = ((xpos + 1) % 3) >> 1;
            hexview->active_column = COLUMN_DATA;
        }
        else if ((xpos > hexview->line2 - hexview->width_char) && (xpos < (hexview->line2 + linenumer_of_item * hexview->width_char)))
        {
            xpos = (xpos - hexview->line2) / hexview->width_char;
            hexview->number_items = number_items + xpos;
            hexview->hl_position = 0;
            hexview->active_column = COLUMN_VALUE;
        }
        return true;
    }
    return false;
}

int
hexview_update_theme(eu_tabpage *p)
{
    if (p && p->hwnd_sc)
    {
        if (p->hex_mode)
        {
            SendMessage(p->hwnd_sc, WM_SETFONT, 0, 0);
            SendMessage(p->hwnd_sc, HVM_SETTEXTCOLOR, 0, (LPARAM) eu_get_theme()->item.text.color);
            SendMessage(p->hwnd_sc, HVM_SETBKCOLOR, 0, (LPARAM) eu_get_theme()->item.text.bgcolor);
            SendMessage(p->hwnd_sc, HVM_SETSELBKCOLOR, 0, (LPARAM) eu_get_theme()->item.indicator.bgcolor);
            return 0;
        }
        else
        {
            SendMessage(p->hwnd_sc, WM_DPICHANGED, 0, (LPARAM) p);
        }
        if (p->hwnd_symlist)
        {
            SendMessage(p->hwnd_symlist, WM_DPICHANGED, 0, 0);
        }
        else if (p->hwnd_symtree)
        {
            SendMessage(p->hwnd_symtree, WM_DPICHANGED, 0, 0);
        }
        if (p->hwnd_qrtable)
        {
            SendMessage(p->hwnd_qrtable, WM_DPICHANGED, 0, 0);
        }
    }
    return 0;
}

static void
hexview_destoy(eu_tabpage *pnode)
{
    if (pnode && pnode->phex)
    {
        if (pnode->phex->hfont && (pnode->phex->ct_flags & HVF_FONTCREATED))
        {   // 如果建立了,删除字体句柄
            DeleteObject(pnode->phex->hfont);
            pnode->phex->hfont = NULL;
        }
        if (pnode->phex->hmap)
        {
            share_unmap(pnode->phex->pbase);
        }
        else if (pnode->phex->pbase)
        {
            eu_safe_free(pnode->phex->pbase);
        }
        pnode->hex_mode = false;
        eu_safe_free(pnode->phex);
    }
}

static bool
hexview_vscroll_visible(HWND hwnd)
{
    if (!hwnd)
    {
        return false;
    }
    return (GetWindowLongPtr(hwnd, GWL_STYLE) & WS_VSCROLL) != 0;
}

static bool
hexview_create_font(HWND hwnd, PHEXVIEW hexview)
{
    LOGFONT logfont;
    TCHAR font[LF_FACESIZE] = {0};
    uint8_t quality = CLEARTYPE_QUALITY;
    if (!MultiByteToWideChar(CP_UTF8, 0, eu_get_theme()->item.text.font, -1, font, LF_FACESIZE-1))
    {
        return false;
    }
    if (eu_get_config()->m_quality ==  IDM_VIEW_FONTQUALITY_STANDARD)
    {
        quality = DEFAULT_QUALITY;
    }
    else if (eu_get_config()->m_quality == IDM_VIEW_FONTQUALITY_NONE)
    {
        quality = NONANTIALIASED_QUALITY;
    }
    int font_size = eu_get_theme()->item.text.fontsize + hex_zoom + 1;
    logfont.lfHeight = -MulDiv(font_size, eu_get_dpi(NULL), 72);
    logfont.lfWidth = 0;
    logfont.lfEscapement = 0;
    logfont.lfOrientation = 0;
    logfont.lfWeight = FW_NORMAL;
    logfont.lfItalic = false;
    logfont.lfUnderline = false;
    logfont.lfStrikeOut = false;
    logfont.lfOutPrecision = OUT_DEFAULT_PRECIS;
    logfont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    logfont.lfQuality = quality;
    logfont.lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;
    logfont.lfCharSet = ANSI_CHARSET;
    _tcsncpy(logfont.lfFaceName, font, _countof(logfont.lfFaceName)-1);
    hexview->hfont = CreateFontIndirect(&logfont);
    if (hexview->hfont)
    {
        hexview->ct_flags |= HVF_FONTCREATED;
    }
    else
    {
        hexview->hfont = (HFONT) GetStockObject(SYSTEM_FIXED_FONT);
    }
    return (hexview->hfont != NULL);
}

static void
hexview_bin_paste(char *ptext, PHEXVIEW hexview)
{
    char *p = NULL;
    char *str = ptext;
    size_t i = hexview->number_items;
    const char *seps = " ";
    if (hexview->select_end - hexview->select_start > 0)
    {
        i = hexview->select_start;
    }
    p = strtok(str, seps);
    while (p)
    {
        hexview->pbase[i++] = util_get_hex_byte(p);
        p = strtok(NULL, seps);
    }
}

static bool
hexview_data_final(char *ptext, size_t len)
{
    bool ret = true;
    char *p = NULL;
    const char *seps = " ";
    char *str = _strdup(ptext);
    if (!str)
    {
        return false;
    }
    if (len < 2)
    {
        ret = false;
        goto do_clean;
    }
    p = strtok(str, seps);
    while (p)
    {
        if (strlen(p) != 2)
        {
            ret = false;
            break;
        }
        if (!(isxdigit(p[0]) && isxdigit(p[1])))
        {
            ret = false;
            break;
        }
        p = strtok(NULL, " ");
    }
do_clean:
    eu_safe_free(str);
    return ret;
}

static bool
hexview_map_realloc(PHEXVIEW phex, int offset)
{
    bool ret = false;
    uint8_t *pstr = NULL;
    size_t nbyte = phex->total_items + offset;
    if (!phex->pbase || !offset || (phex->ex_style & HVS_READONLY))
    {
        return false;
    }
    if ((phex->hmap = share_create(NULL, PAGE_WRITECOPY, nbyte, NULL)) != NULL)
    {
        if ((pstr = (uint8_t *)share_map(phex->hmap, nbyte, FILE_MAP_COPY)) != NULL)
        {
            memcpy(pstr + phex->number_items + offset, phex->pbase + phex->number_items, phex->total_items - phex->number_items);
            if (phex->number_items > 0)
            {
                memcpy(pstr, phex->pbase, phex->number_items);
            }
            memset(pstr + phex->number_items, 0x20, offset);
            share_unmap(phex->pbase);
            phex->pbase = pstr;
            phex->total_items = nbyte;
            ret = true;
        }
    }
    eu_close_handle(phex->hmap);
    return ret;
}

static bool
hexview_mem_realloc(PHEXVIEW phex, int offset)
{
    uint8_t *pstr = NULL;
    if (!phex->pbase || !offset || phex->hmap || (phex->ex_style & HVS_READONLY))
    {
        return false;
    }
    if ((on_file_get_avail_phys() > (offset + phex->total_items)) && (pstr = (uint8_t *)malloc(offset + phex->total_items)))
    {
        memcpy(pstr + phex->number_items + offset, phex->pbase + phex->number_items, phex->total_items - phex->number_items);
        if (phex->number_items > 0)
        {
            memcpy(pstr, phex->pbase, phex->number_items);
        }
        memset(pstr + phex->number_items, 0x20, offset);
        free(phex->pbase);
        phex->pbase = pstr;
        phex->total_items += offset;
        return true;
    }
    return false;
}

static void
hexview_insert_data(HWND hwnd, eu_tabpage *pnode)
{
    int m_inser = 0;
    TCHAR data_str[8 + 1] = {0};
    LOAD_I18N_RESSTR(IDS_HEXVIEW_BYTES, desc_str);
    if (eu_input(desc_str, data_str, _countof(data_str)) && *data_str)
    {
        m_inser = _tstoi(data_str);
    }
    if (m_inser > 0)
    {
        bool ret = false;
        if (pnode->phex->hmap)
        {
            ret = hexview_map_realloc(pnode->phex, m_inser);
        }
        else
        {
            ret = hexview_mem_realloc(pnode->phex, m_inser);
        }
        if (ret)
        {
            SendMessage(hwnd, HVM_SETLINECOUNT, 0, 0);
            InvalidateRect(hwnd, NULL, false);
            on_sci_point_left(pnode);
            on_statusbar_update_filesize(pnode);
        }
    }
}

static void
hexview_on_keydown(HWND hwnd, PHEXVIEW hexview, WPARAM wParam, LPARAM lParam)
{
    do
    {
        eu_tabpage *pnode = (eu_tabpage *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
        if (pnode && hexview->total_items > 0)
        {
            bool ctrl_down = KEY_DOWN(VK_CONTROL);
            if (ctrl_down && wParam == 0x11)
            {
                break;
            }
            if ((wParam == VK_ESCAPE || KEY_DOWN(VK_ESCAPE)))
            {
                break;
            }
            switch (wParam)
            {
                case VK_TAB:
                {
                    switch (hexview->active_column)
                    {
                        case COLUMN_DATA:
                            hexview->active_column = COLUMN_VALUE;
                            break;
                        case COLUMN_VALUE:
                            hexview->active_column = COLUMN_DATA;
                            break;
                    }
                    hexview->hl_position = 0;
                    break;
                }
                case VK_LEFT:
                {
                    int style = (int)GetWindowLongPtr(hwnd, GWL_STYLE);
                    if (ctrl_down && (style & WS_HSCROLL))
                    {
                        SendMessage(hwnd, WM_HSCROLL, SB_LINELEFT, 0);
                    }
                    switch (hexview->active_column)
                    {
                        case COLUMN_DATA:
                        {
                            switch (hexview->hl_position)
                            {
                                case 0:
                                {
                                    if (hexview->number_items != 0)
                                    {
                                        intptr_t line_number;
                                        hexview->hl_position = 1;
                                        hexview->number_items--;
                                        line_number = hexview->number_items / 16;
                                        if (line_number == hexview->vscroll_pos - 1)
                                        {
                                            SendMessage(hwnd, WM_VSCROLL, SB_LINEUP, 0);
                                        }
                                    }
                                    break;
                                }
                                case 1:
                                {
                                    hexview->hl_position = 0;
                                    break;
                                }
                            }
                            break;
                        }
                        case COLUMN_VALUE:
                        {
                            if (hexview->number_items != 0)
                            {
                                intptr_t line_number;
                                hexview->number_items--;
                                line_number = hexview->number_items / 16;
                                if (line_number == hexview->vscroll_pos - 1)
                                {
                                    SendMessage(hwnd, WM_VSCROLL, SB_LINEUP, 0);
                                }
                            }
                            break;
                        }
                    }
                    on_search_update_navigate_list(pnode, eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0));
                    break;
                }
                case VK_RIGHT:
                {
                    int style = (int)GetWindowLongPtr(hwnd, GWL_STYLE);
                    if (ctrl_down && (style & WS_HSCROLL))
                    {
                        SendMessage(hwnd, WM_HSCROLL, SB_LINERIGHT, 0);
                    }
                    switch (hexview->active_column)
                    {
                        case COLUMN_DATA:
                        {
                            switch (hexview->hl_position)
                            {
                                case 0:
                                {
                                    hexview->hl_position = 1;
                                    break;
                                }
                                case 1:
                                {
                                    if (hexview->number_items < hexview->total_items - 1)
                                    {
                                        intptr_t line_number;
                                        hexview->hl_position = 0;
                                        hexview->number_items++;
                                        line_number = hexview->number_items / 16;
                                        if (line_number == hexview->vscroll_pos + hexview->visiblelines)
                                        {
                                            SendMessage(hwnd, WM_VSCROLL, SB_LINEDOWN, 0);
                                        }
                                    }
                                    break;
                                }
                            }
                            break;
                        }
                        case COLUMN_VALUE:
                        {
                            if (hexview->number_items < hexview->total_items - 1)
                            {
                                intptr_t line_number;
                                hexview->number_items++;
                                line_number = hexview->number_items / 16;
                                if (line_number == hexview->vscroll_pos + hexview->visiblelines)
                                {
                                    SendMessage(hwnd, WM_VSCROLL, SB_LINEDOWN, 0);
                                }
                            }
                            break;
                        }
                    }
                    on_search_update_navigate_list(pnode, eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0));
                    break;
                }
                case VK_UP:
                {
                    if (ctrl_down)
                    {
                        SendMessage(hwnd, WM_VSCROLL, SB_LINEUP, 0);
                    }
                    if (hexview->number_items >= 16)
                    {
                        intptr_t line_number;
                        hexview->number_items -= 16;
                        line_number = hexview->number_items / 16;
                        if (line_number == hexview->vscroll_pos - 1)
                        {
                            SendMessage(hwnd, WM_VSCROLL, SB_LINEUP, 0);
                        }
                    }
                    on_search_update_navigate_list(pnode, eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0));
                    break;
                }
                case VK_DOWN:
                {
                    if (ctrl_down)
                    {
                        SendMessage(hwnd, WM_VSCROLL, SB_LINEDOWN, 0);
                    }
                    if ((hexview->number_items + 16) < hexview->total_items)
                    {
                        intptr_t line_number;
                        hexview->number_items += 16;
                        line_number = hexview->number_items / 16;
                        if (line_number == hexview->vscroll_pos + hexview->visiblelines)
                        {
                            SendMessage(hwnd, WM_VSCROLL, SB_LINEDOWN, 0);
                        }
                    }
                    on_search_update_navigate_list(pnode, eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0));
                    break;
                }
                case VK_PRIOR:
                {
                    if ((hexview->number_items - 16 * hexview->visiblelines) >= 0)
                    {
                        hexview->number_items -= 16 * hexview->visiblelines;
                    }
                    else
                    {
                        size_t NumberOfLines = hexview->number_items / 16;
                        hexview->number_items -= 16 * NumberOfLines;
                    }
                    SendMessage(hwnd, WM_VSCROLL, SB_PAGEUP, 0);
                    on_search_add_navigate_list(pnode, eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0));
                    break;
                }
                case VK_NEXT:
                {
                    if ((hexview->number_items + 16 * hexview->visiblelines) < hexview->total_items)
                    {
                        hexview->number_items += 16 * hexview->visiblelines;
                    }
                    else
                    {
                        size_t lines_number = (hexview->total_items - hexview->number_items - 1) / 16;
                        hexview->number_items += 16 * lines_number;
                    }
                    SendMessage(hwnd, WM_VSCROLL, SB_PAGEDOWN, 0);
                    on_search_add_navigate_list(pnode, eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0));
                    break;
                }
                case VK_HOME:
                {
                    if (ctrl_down)
                    {
                        hexview->number_items = 0;
                        hexview->hl_position = 0;
                        SendMessage(hwnd, WM_VSCROLL, SB_TOP, 0);
                    }
                    else
                    {
                        hexview->number_items &= ~0xf;
                        hexview->hl_position = 0;
                        if (hexview->active_column == COLUMN_DATA)
                        {
                            SendMessage(hwnd, WM_HSCROLL, SB_LEFT, 0);
                        }
                    }
                    on_search_add_navigate_list(pnode, eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0));
                    break;
                }
                case VK_END:
                {
                    if (ctrl_down)
                    {
                        hexview->number_items = hexview->total_items - 1;
                        hexview->hl_position = 0;
                        SendMessage(hwnd, WM_VSCROLL, SB_BOTTOM, 0);
                    }
                    else
                    {
                        hexview->number_items = (hexview->number_items & ~0xf) + 15;
                        hexview->hl_position = 0;
                        if (hexview->number_items >= hexview->total_items)
                        {
                            hexview->number_items = hexview->total_items - 1;
                        }
                        SendMessage(hwnd, WM_HSCROLL, SB_RIGHT, 0);
                    }
                    on_search_add_navigate_list(pnode, eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0));
                    break;
                }
            }
            //
            // if lParam == 0 then WM_KEYDOWN came from WM_CHAR
            // and we don't need to select text.
            //
            if ((GetKeyState(VK_SHIFT) & 0x8000) && lParam)
            {
                if (hexview->hl_position || (hexview->select_start != hexview->number_items))
                {
                    hexview->select_end = hexview->number_items;
                    hexview->ct_flags |= HVF_SELECTED;
                }
                else
                {
                    hexview->ct_flags &= ~HVF_SELECTED;
                }
            }
            else if (!ctrl_down && !(GetKeyState(VK_APPS) & 0x8000))
            {
                hexview->select_start = hexview->select_end = hexview->number_items;
                hexview->ct_flags &= ~HVF_SELECTED;
            }
            // pagedown, pageup 由滚动条消息设置光标位置
            if (wParam == VK_PRIOR || wParam == VK_NEXT)
            {
                break;
            }
            if (!(hexview->ct_flags & HVF_CARETVISIBLE))
            {
                intptr_t line_number = hexview->number_items / 16;
                if (line_number > hexview->totallines - hexview->visiblelines)
                {
                    hexview->vscroll_pos = hexview->totallines - hexview->visiblelines;
                }
                else
                {
                    hexview->vscroll_pos = line_number;
                }
                hexview_srollinfo(hwnd, hexview);
            }
            hexview_caret(hwnd, hexview);
            on_statusbar_update_line(pnode);
        }
    } while(0);
}

static int
hexview_postion_offset(PHEXVIEW hexview, const size_t position, int *pnum)
{
    int offset = -1;
    int line_pos = position%16;
    *pnum = 0;
    sptr_t line_fist = position - line_pos;
    for (int i = 0; i < 16; ++i)
    {
        if ((*pnum = hexview_builtin_clz(hexview->pbase[line_fist + i])) > 1 && line_pos >= i && line_pos < i + *pnum)
        {
            offset = line_pos - i;
            break;
        }
    }
    return offset;
}

static LRESULT CALLBACK
hexview_proc(HWND hwnd, uint32_t message, WPARAM wParam, LPARAM lParam)
{
    PHEXVIEW hexview = NULL;
    eu_tabpage *pnode = (eu_tabpage *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (pnode && pnode->phex)
    {
        hexview = pnode->phex;
    }
    switch (message)
    {
        case WM_CREATE:
        {
            pnode = (eu_tabpage *)((LPCREATESTRUCTW)lParam)->lpCreateParams;
            if (!(pnode && pnode->phex))
            {
                eu_logmsg("pnode or pnode->phex is null\n");
                return -1;
            }
            hexview = pnode->phex;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (intptr_t) pnode);
            hexview->active_column = COLUMN_DATA;
            hexview->clr_text = eu_get_theme()->item.text.color;
            hexview->clr_bg_text = eu_get_theme()->item.text.bgcolor;
            hexview->clr_bg_selected = eu_get_theme()->item.indicator.bgcolor;
            hexview->clr_modify_text = RGB(255, 0, 0);
            if (pnode->bytes_remaining > UINT_MAX)
            {
                hexview->ex_style |= HVS_ADDRESS64;
            }
            SendMessage(hwnd, WM_SETFONT, 0, 0);
            break;
        }
        case WM_SETFONT:
        {
            HDC hdc;
            TEXTMETRIC text_metric;
            HANDLE hold_font;
            //
            // Delete the font, if we created it by yourself.
            //

            if ((hexview->ct_flags & HVF_FONTCREATED) && hexview->hfont)
            {
                DeleteObject(hexview->hfont);
                hexview->ct_flags &= ~HVF_FONTCREATED;
            }
            hexview->hfont = (HFONT) wParam;
            if (!hexview->hfont)
            {
                hexview_create_font(hwnd, hexview);
            }
            hdc = GetDC(hwnd);
            hold_font = SelectObject(hdc, hexview->hfont);
            GetTextMetrics(hdc, &text_metric);
            SelectObject(hdc, hold_font);
            ReleaseDC(hwnd, hdc);
            hexview->width_char = text_metric.tmAveCharWidth;
            hexview->height_char = text_metric.tmHeight;
            hexview->visiblelines = hexview->height_view / hexview->height_char;
            hexview->visiblechars = hexview->width_view / hexview->width_char;
            hexview_columns(hexview);
            hexview_srollinfo(hwnd, hexview);
            break;
        }
        case WM_RBUTTONUP:
        {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            // 限制右键显示区域
            if (x < hexview->longest_line*hexview->width_char && (y > 2*hexview->height_char &&  y < hexview->totallines*hexview->height_char))
            {
                return menu_pop_track(hwnd, IDR_HEXVIEW_MENU, 0, -1, NULL, NULL);
            }
            return 1;
        }
        case WM_COMMAND:
        {
            WORD high = HIWORD(wParam);
            WORD low = LOWORD(wParam);
            if (high != 0)
            {
                break;
            }
            switch(low)
            {
                case IDM_HEXVIEW_COPY:
                    SendMessage(hwnd, WM_COPY, 0, 0);
                    break;
                case IDM_HEXVIEW_COPY_ADDR:
                    TCHAR s_xy[FILESIZE] = {0};
                    _sntprintf(s_xy, FILESIZE-1, _T("0x%zX"), hexview->number_items);
                    on_edit_push_clipboard(s_xy);
                    break;
                case IDM_HEXVIEW_PASTE:
                    SendMessage(hwnd, WM_PASTE, 0, 0);
                    break;
                case IDM_HEXVIEW_CUT:
                    SendMessage(hwnd, WM_CUT, 0, -1);
                    break;
                case IDM_HEXVIEW_DEL:
                    SendMessage(hwnd, WM_CLEAR, 0, -1);
                    break;
                case IDM_HEXVIEW_INS:
                    hexview_insert_data(hwnd, pnode);
                    break;
                default:
                    break;
            }
            break;
        }
        case WM_COPY:
        {
            char *ptext = NULL;
            size_t len = 0;
            if ((ptext = util_strdup_select(pnode, &len, 0)) != NULL)
            {
                TCHAR *u16_text = eu_utf8_utf16(ptext, NULL);
                if (u16_text)
                {
                    on_edit_push_clipboard(u16_text);
                    hex_area = hexview->active_column;
                    free(u16_text);
                }
                free(ptext);
            }
            break;
        }
        case WM_PASTE:
        case SCI_PASTE:
        {
            if (hexview->total_items > 0 && !(hexview->ex_style & HVS_READONLY))
            {
                int txt_len = 0;
                char *ptext = NULL;
                if (!util_get_clipboard(&ptext))
                {
                    break;
                }
                txt_len = eu_int_cast(strlen(ptext));
                if (hexview->active_column == COLUMN_DATA)
                {
                    if (hex_area == COLUMN_DATA || hexview_data_final(ptext, txt_len))
                    {
                        if ((hexview->number_items + txt_len < hexview->total_items))
                        {
                            hexview_bin_paste(ptext, hexview);
                            on_sci_point_left(pnode);
                        }
                    }
                    else
                    {
                        int i = 0;
                        int asc_len = txt_len * 3 + 1;
                        char *asc_buf = (char *)calloc(asc_len, 1);
                        char *hex_buf = (char *)calloc(asc_len, 1);
                        util_hex_expand(ptext, txt_len, asc_buf);
                        txt_len = eu_int_cast(strlen(asc_buf));
                        for( int idx = 0; idx < txt_len; idx += 2, i+=2)
                        {
                            strncpy(&hex_buf[i], asc_buf + idx, 2);
                            if (idx != txt_len-2)
                            {
                                ++i;
                                strcat(hex_buf, " ");
                            }
                        }
                        txt_len = eu_int_cast(strlen(hex_buf));
                        if (STR_NOT_NUL(hex_buf) && (hexview->number_items + txt_len < hexview->total_items))
                        {
                            hexview_bin_paste(hex_buf, hexview);
                            on_sci_point_left(pnode);
                            free(hex_buf);
                            free(asc_buf);
                        }
                    }
                }
                else
                {
                    if (hex_area == COLUMN_DATA)
                    {
                        char *pstr = (char *)calloc(txt_len+1, 1);
                        if (pstr)
                        {
                            if (!util_hex_fold(ptext, txt_len, pstr))
                            {
                                eu_logmsg("hex convert, pstr = %s\n", pstr);
                                txt_len = eu_int_cast(strlen(pstr));
                                if (hexview->number_items + txt_len < hexview->total_items)
                                {
                                    memcpy(&hexview->pbase[hexview->number_items], pstr, txt_len);
                                    on_sci_point_left(pnode);
                                }
                            }
                            free(pstr);
                        }
                    }
                    else
                    {
                        if (hexview->number_items + txt_len < hexview->total_items)
                        {
                            memcpy(&hexview->pbase[hexview->number_items], ptext, txt_len);
                            on_sci_point_left(pnode);
                        }
                    }
                }
                InvalidateRect(hwnd, NULL, true);
                eu_safe_free(ptext);
            }
            break;
        }
        case SCI_CUT:
        case WM_CUT:
        {
            if (hexview->total_items > 0 && !(hexview->ex_style & HVS_READONLY))
            {
                char *ptext = NULL;
                if ((ptext = util_strdup_select(pnode, NULL, 0)) != NULL)
                {
                    TCHAR *u16_text = NULL;
                    if ((u16_text = eu_utf8_utf16(ptext, NULL)) != NULL)
                    {
                        size_t select_start = min(hexview->select_start, hexview->select_end);
                        size_t select_end = max(hexview->select_start, hexview->select_end);
                        size_t len = select_end - select_start + 1;
                        uint8_t *poffset = &hexview->pbase[select_start];
                        memmove(poffset, poffset + len, hexview->total_items - select_end);
                        hexview->total_items -= len;
                        eu_logmsg("len = %zu, select_start = %zu, select_end = %zu\n", len, select_start, select_end);
                        on_edit_push_clipboard(u16_text);
                        SendMessage(hwnd, HVM_SETLINECOUNT, 0, 0);
                        InvalidateRect(hwnd, NULL, false);
                        on_sci_point_left(pnode);
                        free(u16_text);
                        on_statusbar_update_filesize(pnode);
                    }
                    free(ptext);
                }
            }
            break;
        }
        case SCI_CLEAR:
        case WM_CLEAR:
        {
            if (hexview->total_items > 0 && !(hexview->ex_style & HVS_READONLY))
            {
                char *ptext = NULL;
                if ((ptext = util_strdup_select(pnode, NULL, 0)) != NULL)
                {
                    size_t select_start = min(hexview->select_start, hexview->select_end);
                    size_t select_end = max(hexview->select_start, hexview->select_end);
                    size_t len = select_end - select_start + 1;
                    uint8_t *poffset = &hexview->pbase[select_start];
                    memmove(poffset, poffset + len, hexview->total_items - select_end);
                    hexview->total_items -= len;
                    SendMessage(hwnd, HVM_SETLINECOUNT, 0, 0);
                    InvalidateRect(hwnd, NULL, false);
                    on_sci_point_left(pnode);
                    free(ptext);
                    on_statusbar_update_filesize(pnode);
                }
            }
            break;
        }
        case WM_LBUTTONDOWN:
        {
            int xpos = GET_X_LPARAM(lParam);
            int ypos = GET_Y_LPARAM(lParam);
            if (hexview_number_item(hexview, xpos, ypos))
            {
                hexview->select_start = hexview->select_end = hexview->number_items;
                if (hexview->ct_flags & HVF_SELECTED)
                {
                    hexview->ct_flags &= ~HVF_SELECTED;
                }
                if (!(hexview->ct_flags & HVF_CARETVISIBLE))
                {
                    hexview->ct_flags |= HVF_CARETVISIBLE;
                }
                SendMessage(hwnd, WM_SETFOCUS, 0, 0);
                SetCapture(hwnd);
            }
            break;
        }
        case WM_LBUTTONUP:
        {
            int num = 0;
            int offset = -1;
            if (hexview->select_end == hexview->select_start)
            {
                if (!hexview->hex_ascii && (offset = hexview_postion_offset(hexview, hexview->number_items, &num)) >= 0)
                {
                    hexview->select_start = hexview->number_items - offset;
                    hexview->select_end = hexview->select_start + num - 1;
                }
            }
            else
            {
                if (!hexview->hex_ascii)
                {
                    if ((offset = hexview_postion_offset(hexview, hexview->select_start, &num)) >= 0)
                    {
                        size_t fist_start = hexview->select_start - offset;
                        size_t fist_end = fist_start + num - 1;
                        hexview->select_start = hexview->select_start > hexview->select_end ? fist_end : fist_start;
                    }
                    if ((offset = hexview_postion_offset(hexview, hexview->number_items, &num)) >= 0)
                    {
                        size_t final_start = hexview->number_items - offset;
                        size_t final_end = final_start + num - 1;
                        hexview->select_end = hexview->select_start > hexview->select_end ? final_start : final_end;
                    }
                }
                if (hexview->select_start > hexview->select_end)
                {
                    UTIL_SWAP(size_t, hexview->select_start, hexview->select_end);
                }
            }
            if (pnode != NULL)
            {
                on_search_add_navigate_list(pnode, eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0));
                on_statusbar_update_line(pnode);
            }
            if (util_under_wine())
            {   // wine上鼠标点击不产生mousemove消息, 所以
                SendMessage(hwnd, WM_MOUSEMOVE, wParam , lParam);
            }
            else
            {
                InvalidateRect(hwnd, NULL, false);
            }
            if (hwnd == GetCapture())
            {
                ReleaseCapture();
            }
            break;
        }
        case WM_MOUSEMOVE:
        {
            RECT rc;
            int xpos = GET_X_LPARAM(lParam);
            int ypos = GET_Y_LPARAM(lParam);
            GetClientRect(hwnd, &rc);
            if (ypos < rc.top || ypos > rc.bottom)
            {
                break;
            }
            if (hwnd == GetCapture())
            {
                if (hexview_number_item(hexview, xpos, ypos))
                {
                    hexview->select_end = hexview->number_items;
                    if (!(hexview->ct_flags & HVF_SELECTED))
                    {
                        hexview->ct_flags |= HVF_SELECTED;
                    }
                    hexview_caret(hwnd, hexview);
                    InvalidateRect(hwnd, NULL, util_under_wine() ? true : false);
                }
            }
            break;
        }
        case WM_CHAR:
        {
            if (hexview->total_items && !(hexview->ex_style & HVS_READONLY))
            {
                switch (hexview->active_column)
                {
                    case COLUMN_DATA:
                    {
                        if (_istxdigit((wint_t) wParam))
                        {

                            NMHVDISPINFO dispinfo = {0};
                            NMHEXVIEW nm_hexview = {0};
                            TCHAR buffer[4] = {0};
                            uint8_t value;
                            dispinfo.item.mask = HVIF_BYTE;
                            dispinfo.item.number_items = hexview->number_items;
                            eu_send_notify(hwnd, HVN_GETDISPINFO, (LPNMHDR) &dispinfo);

                            buffer[0] = (TCHAR) wParam;
                            value = (uint8_t) _tcstoul((PCTSTR) buffer, 0, 16);

                            if (hexview->hl_position)
                            {
                                dispinfo.item.value &= 0xf0;
                                dispinfo.item.value |= value;
                            }
                            else
                            {
                                dispinfo.item.value &= 0x0f;
                                dispinfo.item.value |= (value << 4);
                            }
                            nm_hexview.item.number_items = hexview->number_items;
                            nm_hexview.item.value = dispinfo.item.value;
                            eu_send_notify(hwnd, HVN_ITEMCHANGING, (LPNMHDR) &nm_hexview);
                            SendMessage(hwnd, WM_KEYDOWN, VK_RIGHT, 0);
                        }
                        break;
                    }
                    case COLUMN_VALUE:
                    {
                        if (_istprint((wint_t) wParam) && wParam != VK_TAB)
                        {
                            NMHEXVIEW nm_hexview = {0};
                            nm_hexview.item.number_items = hexview->number_items;
                            nm_hexview.item.value = (uint8_t) wParam;
                            eu_send_notify(hwnd, HVN_ITEMCHANGING, (LPNMHDR) &nm_hexview);
                            SendMessage(hwnd, WM_KEYDOWN, VK_RIGHT, 0);
                        }
                        break;
                    }
                }
            }
            break;
        }
        case WM_KEYDOWN:
        {
            hexview_on_keydown(hwnd, hexview, wParam, lParam);
            InvalidateRect(hwnd, NULL, util_under_wine() ? true : false);
            break;
        }
        case HVM_GOPOS:
        {
            sptr_t i = 0;
            sptr_t last_pos =  wParam >= hexview->total_items ? hexview->total_items - 1: wParam;
            sptr_t skip_line = (last_pos / 16) - (hexview->select_start / 16);
            sptr_t offset = ((last_pos - hexview->select_start) - (sptr_t)(skip_line * 16)) * 2;
            if (offset && hexview->hl_position)
            {   // 如果光标在下半字节
                --offset;
            }
            if (hexview->total_items / 16 * 16 <= (size_t)last_pos)
            {   // 在末尾行, 使用VK_END跳转
                hexview->number_items = hexview->total_items - 1;
                hexview->hl_position = 0;
                SendMessage(hwnd, WM_VSCROLL, SB_BOTTOM, 0);
                hexview_caret(hwnd, hexview);
                offset = (last_pos - hexview->total_items + 1) * 2;
            }
            else
            {   // goto line
                hexview->ct_flags &= ~HVF_SELECTED;
                hexview->number_items = max(0, hexview->number_items + (skip_line * 16));
                hexview->vscroll_pos = hexview->number_items / 16;
                hexview_srollinfo(hwnd, hexview);
                hexview_caret(hwnd, hexview);
            }
            if (offset < 0)
            {
                for (i = 0; i > offset; --i)
                {
                    SendMessage(hwnd, WM_KEYDOWN, VK_LEFT, 0);
                }
            }
            else if (offset > 0)
            {
                for (i = 0; i < offset; ++i)
                {
                    SendMessage(hwnd, WM_KEYDOWN, VK_RIGHT, 0);
                }
            }
            // 设置跳转地址高亮
            BYTERANGE lpos = {last_pos, last_pos+1};
            SendMessage(hwnd, HVM_SETSEL, 0, (sptr_t)&lpos);
            InvalidateRect(hwnd, NULL, false);
            break;
        }
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc;
            hdc = BeginPaint(hwnd, &ps);
            hexview_paint(hwnd, hdc, hexview);
            EndPaint(hwnd, &ps);
            break;
        }
        case WM_SETFOCUS:
        {
            CreateCaret(hwnd, NULL, 1, hexview->height_char);
            if (hexview->ct_flags & HVF_CARETVISIBLE)
            {
                NMHDR nmhdr = {0};
                hexview_caret(hwnd, hexview);
                ShowCaret(hwnd);
                eu_send_notify(hwnd, NM_SETFOCUS, &nmhdr);
            }
            else
            {   // caret invisible, show it!
                hexview_caret(hwnd, hexview);
            }
            if (GetFocus() != hwnd)
            {   // 可能被plugin窗口强占了键盘焦点
                on_proc_resize(NULL);
            }
            break;
        }
        case WM_KILLFOCUS:
        {
            if (hexview->ct_flags & HVF_CARETVISIBLE)
            {
                HideCaret(hwnd);
            }
            DestroyCaret();
            break;
        }
        case WM_VSCROLL:
        {
            intptr_t old_pos = hexview->vscroll_pos;
            switch (LOWORD(wParam))
            {
                case SB_TOP:
                    hexview->vscroll_pos = 0;
                    break;
                case SB_BOTTOM:
                    hexview->vscroll_pos = max(0, hexview->vscroll_max - hexview->visiblelines + 1);
                    break;
                case SB_LINEUP:
                    hexview->vscroll_pos = max(0, hexview->vscroll_pos - 1);
                    break;
                case SB_LINEDOWN:
                    hexview->vscroll_pos = min(hexview->vscroll_pos + 1, max(0, hexview->vscroll_max - hexview->visiblelines + 1));
                    break;
                case SB_PAGEUP:
                    hexview->vscroll_pos = max(0, hexview->vscroll_pos - hexview->visiblelines);
                    break;
                case SB_PAGEDOWN:
                    hexview->vscroll_pos =
                        min(hexview->vscroll_pos + hexview->visiblelines, max(0, hexview->vscroll_max - hexview->visiblelines + 1));
                    break;
                case SB_THUMBTRACK:
                    hexview->vscroll_pos = hexview_track_pos(hwnd, hexview, SB_VERT);
                    break;
            }
            if (old_pos != hexview->vscroll_pos)
            {
                hexview_srollinfo(hwnd, hexview);
                // lParam == -1 时, 由WM_MOUSEMOVE消息的定时器设置光标位置
                if (-1 != lParam)
                {
                    hexview_caret(hwnd, hexview);
                }
                InvalidateRect(hwnd, NULL, false);
            }
            break;
        }
        case WM_HSCROLL:
        {
            int old_pos = hexview->hscroll_pos;
            switch (LOWORD(wParam))
            {
                case SB_LEFT:
                    hexview->hscroll_pos = 0;
                    break;
                case SB_RIGHT:
                    hexview->hscroll_pos = max(0, hexview->hscroll_max - hexview->visiblechars + 1);
                    break;
                case SB_LINELEFT:
                case SB_PAGELEFT:
                    hexview->hscroll_pos = max(0, hexview->hscroll_pos - 1);
                    break;
                case SB_LINERIGHT:
                case SB_PAGERIGHT:
                    hexview->hscroll_pos = min(hexview->hscroll_pos + 1, max(0, hexview->hscroll_max - hexview->visiblechars + 1));
                    break;
                case SB_THUMBTRACK:
                    hexview->hscroll_pos = (int) hexview_track_pos(hwnd, hexview, SB_HORZ);
                    break;
            }
            if (old_pos != hexview->hscroll_pos)
            {
                hexview_columns(hexview);
                hexview_srollinfo(hwnd, hexview);
                hexview_caret(hwnd, hexview);
                InvalidateRect(hwnd, NULL, false);
            }
            break;
        }
        case WM_SIZE:
        {
            hexview->width_view = LOWORD(lParam);
            hexview->height_view = HIWORD(lParam);
            if (hexview->width_view)
            {
                hexview->visiblechars = hexview->width_view / hexview->width_char;
            }
            if (hexview->height_view)
            {
                hexview->visiblelines = hexview->height_view / hexview->height_char;
            }
            if (hexview_pin_bottom(hexview))
            {
                hexview_caret(hwnd, hexview);
                InvalidateRect(hwnd, NULL, false);
            }
            hexview_srollinfo(hwnd, hexview);
            break;
        }
        case WM_MOUSEWHEEL:
        {
            int scr_line = 0;
            int16_t delta;
            delta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
            SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &scr_line, 0);
            if (delta > 0)
            {
                hexview->vscroll_pos = max(0, hexview->vscroll_pos - (delta * scr_line));
            }
            else if (delta < 0)
            {
                hexview->vscroll_pos =
                    min(hexview->vscroll_pos + (-delta * scr_line), max(0, hexview->vscroll_max - hexview->visiblelines + 1));
            }
            hexview_srollinfo(hwnd, hexview);
            hexview_caret(hwnd, hexview);
            InvalidateRect(hwnd, NULL, false);
            break;
        }
        case HVM_SETEXTENDEDSTYLE:
        {
            hexview->ex_style = (uint32_t) lParam;
            break;
        }
        case HVM_SETITEMCOUNT:
        {
            hexview->total_items = 0;
            hexview->totallines = 0;
            hexview->longest_line = 0;
            if (lParam)
            {
                hexview->total_items = (size_t) lParam;
                hexview->totallines = hexview->total_items / 16 + 2;
                if (hexview->total_items % 16)
                {
                    hexview->totallines += 1;
                }
                hexview->longest_line = ((hexview->ex_style & HVS_ADDRESS64) ? NUMBEROF_CHARS_IN_FIRST_COLUMN_64BIT :
                                        NUMBEROF_CHARS_IN_FIRST_COLUMN_32BIT) + NUMBEROF_CHARS_IN_SECOND_COLUMN + NUMBEROF_CHARS_IN_THIRD_COLUMN;
            }
            hexview->number_items = 0;
            hexview->hl_position = 0;
            hexview->select_start = 0;
            hexview->select_end = 0;
            hexview->ct_flags &= ~HVF_SELECTED;
            hexview->active_column = COLUMN_DATA;
            hexview->vscroll_pos = 0;
            hexview->vscroll_max = max(hexview->totallines - 1, 0);
            hexview->hscroll_pos = 0;
            hexview->hscroll_max = max(hexview->longest_line - 1, 0);
            hexview_columns(hexview);
            hexview_srollinfo(hwnd, hexview);
            hexview_caret(hwnd, hexview);
            InvalidateRect(hwnd, NULL, false);
            break;
        }
        case HVM_GETSEL:
        {
            if (lParam)
            {
                ((PBYTERANGE) lParam)->min = min(hexview->select_start, hexview->select_end);
                ((PBYTERANGE) lParam)->max = max(hexview->select_start, hexview->select_end);
                if (hexview->ct_flags & HVF_SELECTED)
                {
                    ((PBYTERANGE) lParam)->max += 1;
                }
            }
            break;
        }
        case HVM_SETSEL:
        {
            if (lParam)
            {
                if ((((PBYTERANGE) lParam)->min >= 0 && ((PBYTERANGE) lParam)->min < hexview->total_items) &&
                    (((PBYTERANGE) lParam)->max >= 0 && ((PBYTERANGE) lParam)->max <= hexview->total_items))
                {
                    hexview->select_start = ((PBYTERANGE) lParam)->min;
                    hexview->select_end = ((PBYTERANGE) lParam)->max;
                    if (((PBYTERANGE) lParam)->min != ((PBYTERANGE) lParam)->max)
                    {
                        hexview->select_end -= 1;
                        hexview->ct_flags |= HVF_SELECTED;
                    }
                    else
                    {
                        hexview->ct_flags &= ~HVF_SELECTED;
                    }
                    hexview->number_items = hexview->select_start;
                    hexview->hl_position = 0;
                    hexview->vscroll_pos =
                        min((intptr_t)(hexview->number_items / 16), max(0, hexview->vscroll_max - hexview->visiblelines + 1));
                    hexview_srollinfo(hwnd, hexview);
                    hexview_caret(hwnd, hexview);
                    InvalidateRect(hwnd, NULL, false);
                }
            }
            break;
        }
        case HVM_SETTEXTCOLOR:
        {
            hexview->clr_text = (COLORREF) lParam;
            InvalidateRect(hwnd, NULL, false);
            break;
        }
        case HVM_SETBKCOLOR:
        {
            hexview->clr_bg_text = (COLORREF) lParam;
            InvalidateRect(hwnd, NULL, false);
            break;
        }
        case HVM_SETSELBKCOLOR:
        {
            hexview->clr_bg_selected = (COLORREF) lParam;
            InvalidateRect(hwnd, NULL, false);
            break;
        }
        case HVM_SETMODIFIEDCOLOR:
        {
            hexview->clr_modify_text = (COLORREF) lParam;
            InvalidateRect(hwnd, NULL, false);
            break;
        }
        case HVM_GETHEXADDR:
        {
            return hexview->number_items;
        }
        case HVM_SETLINECOUNT:
        {
            hexview->totallines = hexview->total_items / 16 + 2;
            if (hexview->total_items % 16)
            {
                hexview->totallines += 1;
            }
            return hexview->totallines;
        }
        case WM_THEMECHANGED:
        {
            if (eu_get_config()->m_toolbar != IDB_SIZE_0)
            {
                on_toolbar_update_button();
            }
            break;
        }
        // 兼容scintilla控件
        case SCI_ZOOMIN:
        {
            _InterlockedExchangeAdd(&hex_zoom, 1);
            SendMessage(hwnd, WM_SETFONT, 0, 0);
            InvalidateRect(hwnd, NULL, false);
            break;
        }
        case SCI_ZOOMOUT:
        {
            _InterlockedExchangeAdd(&hex_zoom, -1);
            SendMessage(hwnd, WM_SETFONT, 0, 0);
            InvalidateRect(hwnd, NULL, false);
            break;
        }
        case SCI_SETZOOM:
        {
            _InterlockedExchange(&hex_zoom, (long)wParam);
            SendMessage(hwnd, WM_SETFONT, 0, 0);
            InvalidateRect(hwnd, NULL, false);
            break;
        }
        case SCI_GETZOOM:
        {
            return hex_zoom;
        }
        case SCI_GOTOPOS:
        {
            SendMessage(hwnd, HVM_GOPOS, wParam, 0);
            break;
        }
        case SCI_SETREADONLY:
        {
            if (wParam)
            {
                !(hexview->ex_style & HVS_READONLY) ? hexview->ex_style |= HVS_READONLY : (void)0;
            }
            else
            {
                (hexview->ex_style & HVS_READONLY) ? hexview->ex_style &= ~HVS_READONLY : (void)0;
            }
            break;
        }
        case SCI_GETREADONLY:
        {
            return (hexview->ex_style & HVS_READONLY);
        }
        case SCI_GETEOLMODE:
        {
            return -1;
        }
        case SCI_GETLENGTH:
        {
            return hexview->total_items;
        }
        case SCI_GETSELECTIONSTART:
        {
            return (min(hexview->select_start, hexview->select_end));
        }
        case SCI_GETSELECTIONEND:
        {
            return (max(hexview->select_start, hexview->select_end));
        }
        case SCI_LINEFROMPOSITION:
        {
            if (wParam >= 0)
            {
                return wParam/16;
            }
            return 0;
        }
        case SCI_POSITIONFROMLINE:
        {
            if (wParam >= 0)
            {
                return min(wParam * 16 + 1, hexview->total_items);
            }
            return 0;
        }
        case SCI_GETLINECOUNT:
        {
            return hexview->totallines - 2;
        }
        case SCI_GETMODIFY:
        {
            if (pnode->be_modify)
            {
                return 1;
            }
            return 0;
        }
        case SCI_SETSAVEPOINT:
        {
            return on_sci_point_reached(pnode);
        }
        case SCI_GETCURRENTPOS:
        {
            return hexview->number_items;
        }
        case SCN_SAVEPOINTLEFT:
        {
            return on_sci_point_left(pnode);
        }
        case SCI_GETSELTEXT:
        {
            size_t select_start = min(hexview->select_start, hexview->select_end);
            size_t select_end = max(hexview->select_start, hexview->select_end);
            size_t len = select_end - select_start + 1;
            size_t buf_len = 0;
            if (hexview->active_column == COLUMN_DATA)
            {
                buf_len = 3 * len + 1;
            }
            else
            {
                buf_len = len + 1;
            }
            if (wParam && lParam)
            {
                if (hexview->active_column == COLUMN_DATA)
                {
                    char *buf = (char *)lParam;
                    for (size_t i = select_start; i < select_end+1; ++i)
                    {
                        int m = 0;
                        if (i == select_end)
                        {
                            m = sprintf(buf, "%02X", pnode->phex->pbase[i]);
                        }
                        else
                        {
                            m = sprintf(buf, "%02X ", pnode->phex->pbase[i]);
                        }
                        buf += m;
                    }
                }
                else
                {
                    memcpy((uint8_t *)lParam, &pnode->phex->pbase[select_start], len);
                }
            }
            else
            {
                return (buf_len + 1);
            }
            return 0;
        }
        case SCI_GETTEXTRANGE:
        {
            if (lParam == 0)
            {
                return 0;
            }
            Sci_TextRange *tr = (Sci_TextRange *)lParam;
            intptr_t cpmax = (intptr_t)(tr->chrg.cpMax);
            if (cpmax == -1)
            {
                cpmax = hexview->total_items;
            }
            intptr_t len = cpmax - tr->chrg.cpMin;
            memcpy(tr->lpstrText, &pnode->phex->pbase[tr->chrg.cpMin], len);
            tr->lpstrText[len] = '\0';
            // not including '\0' char
            return len;
        }
        case WM_DESTROY:
        {
            if (pnode)
            {
                hexview_destoy(pnode);
                SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
                eu_logmsg("HEXVIEW WM_DESTROY\n");
            }
            break;
        }
        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}

static HWND
hexview_create_dlg(HWND parent, LPVOID lparam)
{
    return CreateWindowEx(0, HEX_CLASS, NULL, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, parent, 0, eu_module_handle(), lparam);
}

static void
hexview_register_class(void)
{
    WNDCLASSEX wcex = {wcex.cbSize = sizeof(WNDCLASSEX)};
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = hexview_proc;
    wcex.hInstance = GetModuleHandle(NULL);
    wcex.hCursor = LoadCursor(NULL, IDC_IBEAM);
    wcex.lpszClassName = HEX_CLASS;
    RegisterClassEx(&wcex);
}

bool
hexview_init(eu_tabpage *pnode)
{
    HWND hwnd = eu_module_hwnd();
    if (!(pnode && hwnd))
    {
        return false;
    }
    if ((pnode->tab_id = on_tabpage_get_index(pnode)) < 0)
    {   // tab保存原先的位置
        return false;
    }
    if (pnode->hwnd_sc)
    {   // 销毁scintilla窗口与其关联窗口, 复用pnode指针
        on_sci_destroy_control(pnode);
        SendMessage(pnode->hwnd_sc, WM_CLOSE, 0, 0);
    }
    if (true)
    {
        pnode->eusc = 0;
        pnode->begin_pos = -1;
        pnode->hex_mode = true;
        hexview_register_class();
    }
    if (!(pnode->hwnd_sc = hexview_create_dlg(hwnd, pnode)))
    {
        eu_logmsg("hexview_create_dlg failed on %s:%d\n", __FILE__, __LINE__);
        return false;
    }
    if ((pnode->file_attr & FILE_ATTRIBUTE_READONLY))
    {
        if (!(pnode->phex->ex_style & HVS_READONLY))
        {
            pnode->phex->ex_style |= HVS_READONLY;
        }
        pnode->file_attr &= ~FILE_READONLY_COLOR;
        on_statusbar_btn_colour(pnode, true);
    }
    if (pnode->codepage == IDM_OTHER_BIN && pnode->phex)
    {
        pnode->phex->hex_ascii = true;
    }
    SendMessage(pnode->hwnd_sc, HVM_SETITEMCOUNT, 0, (LPARAM) pnode->bytes_remaining);
    on_tabpage_selection(pnode, pnode->tab_id);
    PostMessage(pnode->hwnd_sc, WM_SETFOCUS, 0, 0);
    return true;
}

static HANDLE
hexview_map_read(const TCHAR *filepath, uintptr_t *ppbase)
{
    HANDLE hmap = NULL;
    HANDLE hfile = CreateFile(filepath,
                   GENERIC_READ | GENERIC_WRITE,
                   FILE_SHARE_READ|FILE_SHARE_WRITE,
                   0,
                   OPEN_EXISTING,
                   FILE_ATTRIBUTE_NORMAL,
                   0);
    if (INVALID_HANDLE_VALUE != hfile)
    {
        if ((hmap = CreateFileMapping(hfile, NULL, PAGE_READWRITE, 0, 0, NULL)))
        {
            *ppbase = (uintptr_t)MapViewOfFile(hmap, FILE_MAP_WRITE, 0, 0, 0);
            if (!*ppbase)
            {
                eu_logmsg("%s: MapViewOfFile failed, cause %lu\n", __FUNCTION__, GetLastError());
                CloseHandle(hmap);
                hmap = NULL;
            }
        }
        CloseHandle(hfile);
        return hmap;
    }
    return NULL;
}

static int
hexview_map_write(const uint8_t *pbuf, const size_t buf_len, const TCHAR *dst_path, uint32_t dw_create)
{
    HANDLE hmap = NULL;
    HANDLE hfile = NULL;
    int err = SKYLARK_OK;
    uint8_t *data = NULL;
    uint64_t block = 0x10000000;  // 每次映射256M
    uint64_t filesize = buf_len;
    uint64_t offset = 0;
    if (!share_open_file(dst_path, false, dw_create, &hfile))
    {
        eu_logmsg("%s: share_open_file failed, cause: %lu\n", __FUNCTION__, GetLastError());
        return EUE_API_OPEN_FILE_ERR;
    }
    if (!(hmap = share_create(hfile, PAGE_READWRITE, buf_len, NULL)))
    {
        CloseHandle(hfile);
        return EUE_CREATE_MAP_ERR;
    }
    while (offset < buf_len)
    {
        if (!filesize)
        {
            break;
        }
        if (filesize < block)
        {
            block = filesize;
        }
        data = share_map_section(hmap, offset, (size_t)block, false);
        if (!data)
        {
            eu_logmsg("%s: create_file_mem error, cause : %lu\n", __FUNCTION__, GetLastError());
            err = EUE_MAPPING_MEM_ERR;
            break;
        }
        else
        {
            memcpy(data, pbuf+offset, (size_t)block);
            // 刷新缓存, 但是对大文件来说太慢
            // FlushViewOfFile(data, block);
            // FlushFileBuffers(hfile);
            share_unmap(data);
            offset += block;
            filesize -= block;
        }
    }
    CloseHandle(hmap);
    CloseHandle(hfile);
    return err;
}

static void
hexview_updata(intptr_t *arr, intptr_t m)
{
    for (int i = 0; i < BUFF_32K; ++i)
    {
        if (arr[i] < 0)
        {
            arr[i] = m;
            break;
        }
    }
}

int
hexview_save_data(eu_tabpage *pnode, const TCHAR *bakfile)
{
    int len = 0;
    HANDLE hmap = NULL;
    uintptr_t pbase = 0;
    TCHAR path[MAX_BUFFER] = {0};
    if (!(pnode && pnode->phex && pnode->phex->pbase))
    {
        return EUE_TAB_NULL;
    }
    if (bakfile)
    {
        if (eu_exist_file(bakfile))
        {
            _sntprintf(path, MAX_BUFFER, _T("%s.bakcup"), bakfile);
            if (hexview_map_write(pnode->phex->pbase, pnode->phex->total_items, path, CREATE_ALWAYS) == SKYLARK_OK)
            {
                if (!MoveFileEx(path, bakfile, MOVEFILE_COPY_ALLOWED|MOVEFILE_REPLACE_EXISTING))
                {
                    return EUE_MOVE_FILE_ERR;
                }
            }
        }
        else
        {
            return hexview_map_write(pnode->phex->pbase, pnode->phex->total_items, bakfile, CREATE_ALWAYS);
        }
    }
    else
    {  // 文件已存在, 写入备份后替换
        _sntprintf(path, MAX_BUFFER, _T("%s.bakcup"), pnode->pathfile);
        if (hexview_map_write(pnode->phex->pbase, pnode->phex->total_items, path, CREATE_ALWAYS) == SKYLARK_OK)
        {
            if (!MoveFileEx(path, pnode->pathfile, MOVEFILE_COPY_ALLOWED|MOVEFILE_REPLACE_EXISTING))
            {
                eu_logmsg("%s: movefile failed, cause: %lu\n", __FUNCTION__, GetLastError());
                return EUE_MOVE_FILE_ERR;
            }
        }
    }
    return SKYLARK_OK;
}

uint8_t *
hexview_strdup_data(eu_tabpage *pnode, size_t *plen)
{
    *plen = 0;
    uint8_t *pstr = NULL;
    if (!(pnode && pnode->phex && pnode->phex->pbase))
    {
        return NULL;
    }
    if (on_file_get_avail_phys() > pnode->phex->total_items && (pstr = (uint8_t *)malloc(pnode->phex->total_items)))
    {
        memcpy(pstr, pnode->phex->pbase, pnode->phex->total_items);
        *plen = pnode->phex->total_items;
    }
    return pstr;
}

int
hexview_switch_mode(eu_tabpage *pnode)
{
    int err = EUE_TAB_NULL;
    uint8_t *pdst = NULL;
    if (!pnode)
    {
        return err;
    }
    if (on_tabpage_focus_at() != pnode)
    {
        on_tabpage_active_tab(pnode);
    }
    util_lock(&pnode->busy_id);
    if (!pnode->hex_mode)
    {
        pnode->nc_pos = eu_sci_call(pnode, SCI_GETCURRENTPOS, 0, 0);
        if (!pnode->phex)
        {
            pnode->phex = (PHEXVIEW) calloc(1, sizeof(HEXVIEW));
            hex_zoom = pnode->zoom_level > SELECTION_ZOOM_LEVEEL ? (int) eu_sci_call(pnode, SCI_GETZOOM, 0, 0) : 0;
            if (!pnode->phex)
            {
                err = EUE_POINT_NULL;
                goto HEX_ERROR;
            }
            if (!(pnode->phex->pbase = (uint8_t *) util_strdup_content(pnode, &pnode->bytes_remaining)))
            {
                eu_logmsg("%s: txt maybe null\n", __FUNCTION__);
                err = EUE_POINT_NULL;
                eu_safe_free(pnode->phex);
                goto HEX_ERROR;
            }
            if (!hexview_init(pnode))
            {
                err = EUE_CREATE_MAP_ERR;
                eu_safe_free(pnode->phex->pbase);
                eu_safe_free(pnode->phex);
                goto HEX_ERROR;
            }
            if ((err = pnode->tab_id) >= 0 && pnode->nc_pos >= 0)
            {
                eu_sci_call(pnode, SCI_GOTOPOS, pnode->nc_pos, 0);
            }
            // 清理文本模式下的导航信息
            on_search_clean_navigate_this(pnode);
        }
    }
    else
    {
        int count = 0;
        size_t offset = 0;
        size_t  dst_len = 0;
        bool is_utf8 = pnode->codepage == IDM_UNI_UTF8;
        pnode->begin_pos = -1;
        pnode->tab_id = on_tabpage_get_index(pnode);
        pnode->doc_ptr = on_doc_get_type(pnode->filename);
        pnode->nc_pos = pnode->phex ? pnode->phex->number_items : -1;
        pnode->zoom_level = hex_zoom > SELECTION_ZOOM_LEVEEL ? hex_zoom : 0;
        pnode->needpre = pnode->pre_len > 0;
        if (pnode->phex && pnode->phex->hex_ascii)
        {
            is_utf8 = false;
        }
        else
        {
            is_utf8 = true;
        }
        if (is_utf8)
        {
            pdst = hexview_strdup_data(pnode, &dst_len);
        }
        else
        {
            size_t src_len = (size_t)pnode->raw_size;
            uint8_t *data = pnode->phex->pbase;
            euconv_t evd = {0};
            evd.src_from = eu_query_encoding_name(pnode->codepage);
            evd.dst_to = "utf-8";
            if (evd.src_from && strcmp(evd.src_from, "UTF-8(BOM)") == 0)
            {
                evd.src_from = "utf-8";
            }
            eu_logmsg("%s: on_encoding_do_iconv, from %s to %s\n", __FUNCTION__, evd.src_from, evd.dst_to);
            size_t res = on_encoding_do_iconv(&evd, (char *) (data), &src_len, &pdst, &dst_len);
            if (res == (size_t) -1)
            {
                eu_logmsg("%s: on_encoding_do_iconv error\n", __FUNCTION__);
                err = EUE_ICONV_FAIL;
                goto HEX_ERROR;
            }
            // 保存bom
            on_encoding_set_bom(pnode->phex->pbase, pnode);
            if (memcmp(pdst, (const uint8_t *) "\xEF\xBB\xBF", 3) == 0)
            {   // 因为pbase带bom情况下转换为utf8, 会产生bom
                // 而我们不需要, 因为前面已经保存了原始文本的bom
                offset = 3;
                eu_logmsg("we save utf8 bom, offset = 3\n");
            }
        }
        if (!pdst)
        {
            err = EUE_POINT_NULL;
            goto HEX_ERROR;
        }
        // 清理16进制编辑器下的导航信息
        on_search_clean_navigate_this(pnode);
        SendMessage(pnode->hwnd_sc, WM_CLOSE, 0, 0);
        if (on_sci_init_dlg(pnode))
        {
            eu_logmsg("on_sci_init_dlg return failed on %s:%d\n", __FILE__, __LINE__);
            err = EUE_UNKOWN_ERR;
            goto HEX_ERROR;
        }
        on_sci_before_file(pnode, true);
        eu_sci_call(pnode, SCI_CLEARALL, 0, 0);
        eu_sci_call(pnode, SCI_ADDTEXT, dst_len - offset, (LPARAM)(pdst + offset));
        eu_sci_call(pnode, SCI_SETOVERTYPE, false, 0);
        on_sci_after_file(pnode, true);
        on_search_add_navigate_list(pnode, 0);
        if ((err = on_tabpage_selection(pnode, pnode->tab_id)) >= 0)
        {
            if (pnode->nc_pos >= 0)
            {
                on_search_jmp_pos(pnode);
            }
            if ((pnode->file_attr & FILE_ATTRIBUTE_READONLY))
            {
                pnode->file_attr &= ~FILE_READONLY_COLOR;
                on_statusbar_btn_colour(pnode, true);
            }
            eu_logmsg("%s: pnode->eol = %d\n", __FUNCTION__, pnode->eol);
            PostMessage(pnode->hwnd_sc, WM_SETFOCUS, 0, 0);
        }
    }
HEX_ERROR:
    eu_safe_free(pdst);
    ShowWindow(eu_get_search_hwnd(), SW_HIDE);
    util_unlock(&pnode->busy_id);
    return err;
}

void
hexview_switch_item(eu_tabpage *pnode)
{
    if (g_tabpages && pnode)
    {
        int result = IDOK;
        if (!_InterlockedCompareExchange(&affected_switch, 1, 0))
        {
            MSG_BOX_SEL(IDS_HISTORY_CLEAR_UNDO, IDC_MSG_TIPS, MB_ICONSTOP | MB_OKCANCEL, result);
        }
        if (result == IDOK)
        {
            eu_tabpage *p = NULL;
            cvector_vector_type(int) v = NULL;
            int num = on_tabpage_sel_number(&v, false);
            for (int i = 0; i < num; ++i)
            {
                eu_tabpage *p = on_tabpage_get_ptr(v[i]);
                if (p && p != pnode && p->hex_mode == pnode->hex_mode && TAB_NOT_NUL(p) && TAB_NOT_BIN(p))
                {
                    hexview_switch_mode(p);
                }
            }
            hexview_switch_mode(pnode);
            cvector_freep(&v);
        }
        else
        {
            _InterlockedExchange(&affected_switch, 0);
        }
    }
}
