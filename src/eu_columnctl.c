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

typedef struct _columnmodeinfo
{
    intptr_t sel_lpos      ;
    intptr_t sel_rpos      ;
    intptr_t order         ;
    intptr_t virtual_anchor;
    intptr_t virtual_caret ;
    bool     direction     ;
} columnmodeinfo;

static HWND hwnd_column = NULL;

// 计算数字的长度
static inline size_t
count_digits(size_t num, size_t base)
{
    size_t nb = 0;
    do
    {
        ++nb;
        num /= base;
    } while (num != 0);
    return nb;
}

// 数字转成二进制字符串
static char*
num_to_binary(size_t num)
{
    char *result = NULL;
    size_t len = count_digits(num, 2);
    if (len > 0 && (result = (char *)calloc(1, len + 1)) != NULL)
    {
        for (size_t i = len - 1; i != (size_t)-1; i--)
        {
            result[len -1 - i] = (num & ((size_t)1 << i)) ? '1' : '0';
        }
        result[len] = '\0';
        return result;
    }
    return NULL;
}

// 数字转成八进制字符串
static char*
num_to_octal(size_t num)
{
    char *result = NULL;
    size_t len = count_digits(num, 8);
    if (len > 0 && (result = (char *)calloc(1, len + 1)) != NULL)
    {
        _snprintf(result, len, "%zo", num);
        return result;
    }
    return NULL;
}

static char*
num_to_hex(size_t num)
{
    char *result = NULL;
    size_t len = count_digits(num, 16);
    if (len > 0 && (result = (char *)calloc(1, len + 1)) != NULL)
    {
        _snprintf(result, len, "%zx", num);
        return result;
    }
    return NULL;
}

static char*
num_to_decimal(size_t num)
{
    char *result = NULL;
    size_t len = count_digits(num, 10);
    if (len > 0 && (result = (char *)calloc(1, len + 1)) != NULL)
    {
        _snprintf(result, len, "%zu", num);
        return result;
    }
    return NULL;
}

// 处理数字进制转换与对齐
static char*
number_string(char *str, const size_t str_len, const size_t number, const size_t base, const size_t nb_digits, const int lead)
{
    char * number_str = NULL;
    size_t num_a = (size_t)number;
    if (nb_digits == 0 || nb_digits >= str_len)
    {
        return NULL;
    }
    memset(str, 0, str_len);
    if (base == 2)
    {
        number_str = num_to_binary(num_a);
    }
    else if (base == 8)
    {
        number_str = num_to_octal(num_a);
    }
    else if (base == 16)
    {
        number_str = num_to_hex(num_a);
    }
    else
    {   // base == 10
        number_str = num_to_decimal(num_a);
    }
    if (number_str)
    {
        size_t number_len = strlen(number_str);
        size_t align_len = nb_digits - number_len;
        
        size_t nb_start = 0;
        size_t nb_end = 0;
        size_t none_ustart = 0;
        size_t none_uend = 0;
        char used_align = ' ';
        if (lead == IDS_COLUMN_FILL_SPACES)
        {
            none_ustart = 0;
            none_uend = nb_start = align_len;
            nb_end = nb_digits;
        }
        else if (lead == IDS_COLUMN_FILL_ZEROS)
        {
            used_align = '0';
            none_ustart = 0;
            none_uend = nb_start = align_len;
            nb_end = nb_digits;
        }
        else
        {   // IDS_COLUMN_FILL_NONE
            nb_start = 0;
            nb_end = none_ustart = number_len;
            none_uend = nb_digits;
        }
        // 按照对齐方式填充缓冲区
        for (size_t i = 0, k = nb_start; k < nb_end; ++k)
        {
            str[k] = number_str[i++];
        }
        for (size_t j = none_ustart; j < none_uend; ++j)
        {
            str[j] = used_align;
        }
        free(number_str);
        return str;
    }
    return NULL;
}

static bool
get_columnmodeinfo(const eu_tabpage *p, columnmodeinfo **pvec)
{
    if (p && pvec)
    {
        sptr_t sel = on_sci_call(p, SCI_GETSELECTIONS, 0, 0);
        for (sptr_t i = 0 ; i < sel ; ++i)
        {
            sptr_t sel_line_start = on_sci_call(p, SCI_GETSELECTIONNANCHOR, i, 0);
            sptr_t sel_line_end = on_sci_call(p, SCI_GETSELECTIONNCARET, i, 0);
            sptr_t virtual_anchor = on_sci_call(p, SCI_GETSELECTIONNANCHORVIRTUALSPACE, i, 0);
            sptr_t virtual_caret = on_sci_call(p, SCI_GETSELECTIONNCARETVIRTUALSPACE, i, 0);

            if (sel_line_start == sel_line_end && on_sci_call(p, SCI_SELECTIONISRECTANGLE, 0, 0))
            {
                columnmodeinfo column = {sel_line_start, sel_line_end, i, virtual_anchor, virtual_caret, virtual_anchor < virtual_caret? true : false};
                cvector_push_back(*pvec, column);
            }
            else if (sel_line_start > sel_line_end)
            {
                columnmodeinfo column = {sel_line_end, sel_line_start, i, virtual_anchor, virtual_caret, false};
                cvector_push_back(*pvec, column);
            }
            else
            {
                columnmodeinfo column = {sel_line_start, sel_line_end, i, virtual_anchor, virtual_caret, true};
                cvector_push_back(*pvec, column);
            }
        }
        return (cvector_size(*pvec) > 0);
    }
    return false;
}

static int
on_column_position_order(const void* _a, const void* _b)
{
    return (int)((*(columnmodeinfo *)_a).sel_lpos - (*(columnmodeinfo *)_b).sel_rpos);
}

static int
on_column_select_order(const void* _a, const void* _b)
{
    return (int)((*(columnmodeinfo *)_a).order - (*(columnmodeinfo *)_b).order);
}

static inline bool
position_valid(columnmodeinfo *pcmi)
{
    return (pcmi->order >= 0 && pcmi->sel_lpos >= 0 && pcmi->sel_rpos >= 0 && pcmi->sel_lpos <= pcmi->sel_rpos);
}

static void
on_column_replace(const eu_tabpage *p, columnmodeinfo *pcmi, const size_t initial, const size_t incr, const size_t repeat, const size_t base, const int lead)
{
    size_t i = 0;
    size_t *numbers = NULL;
    size_t cur_number = (size_t)initial;
    const size_t kmax = cvector_size(pcmi);
    while (cvector_size(numbers) < kmax)
    {
        for (i = 0; i < repeat; ++i)
        {
            cvector_push_back(numbers, cur_number);
            if (cvector_size(numbers) >= kmax)
            {
                break;
            }
        }
        cur_number += incr;
    }
    if ((i = cvector_size(numbers)) > 0)
    {
        char str[MAX_BUFFER] = {0};
        size_t end_number = *cvector_back(numbers);
        size_t kib_end = count_digits(end_number, base);
        size_t kib_init = count_digits(initial, base);
        size_t kib = MAX(kib_init, kib_end);
        intptr_t total_diff = 0;
        const size_t len = i;
        for (i = 0 ; i < len ; ++i)
        {
            if (position_valid(&pcmi[i]))
            {
                const intptr_t len_replaced = pcmi[i].sel_rpos - pcmi[i].sel_lpos;
                const intptr_t diff = kib - len_replaced;
                pcmi[i].sel_lpos += total_diff;
                pcmi[i].sel_rpos += total_diff;
                if (!number_string(str, MAX_BUFFER, *cvector_at(numbers, i), base, kib, lead))
                {
                    eu_logmsg("Column: number_string failed\n");
                    break;
                }
                if (pcmi[i].virtual_anchor > 0) // if virtual space is present, then insert space
                {
                    for (intptr_t j = 0, k = pcmi[i].sel_lpos; j < pcmi[i].virtual_caret ; ++j, ++k)
                    {
                        on_sci_call(p, SCI_INSERTTEXT, k, (LPARAM)(" "));
                    }
                    pcmi[i].sel_lpos += pcmi[i].virtual_anchor;
                    pcmi[i].sel_rpos += pcmi[i].virtual_caret;
                }
                on_sci_call(p, SCI_SETTARGETRANGE, pcmi[i].sel_lpos, pcmi[i].sel_rpos);
                on_sci_call(p, SCI_REPLACETARGET, (sptr_t)(-1), (sptr_t)(str));
        
                if (pcmi[i].virtual_anchor > 0)
                {
                    total_diff += pcmi[i].virtual_anchor + strlen(str);
                    // Now there's no more virtual space
                    pcmi[i].virtual_anchor = 0;
                    pcmi[i].virtual_caret = 0;
                }
                else
                {
                    total_diff += diff;
                }
                pcmi[i].sel_rpos += diff;
            }
        }
    }
}

static void
on_column_multi(const eu_tabpage *p, columnmodeinfo *pcmi)
{
    for (size_t i = 0, len = cvector_size(pcmi); i < len ; ++i)
    {
        if (position_valid(&pcmi[i]))
        {
            const sptr_t sel_start = pcmi[i].direction == true ? pcmi[i].sel_lpos : pcmi[i].sel_rpos;
            const sptr_t sel_end   = pcmi[i].direction == true ? pcmi[i].sel_rpos : pcmi[i].sel_lpos;
            on_sci_call(p, SCI_SETSELECTIONNSTART, i, sel_start);
            on_sci_call(p, SCI_SETSELECTIONNEND, i, sel_end);
        }
        if (pcmi[i].virtual_anchor)
        {
            on_sci_call(p, SCI_SETSELECTIONNANCHORVIRTUALSPACE, i, pcmi[i].virtual_anchor);
        }
        if (pcmi[i].virtual_caret)
        {
            on_sci_call(p, SCI_SETSELECTIONNCARETVIRTUALSPACE, i, pcmi[i].virtual_caret);
        }
    }
}

static void
on_column_do(void)
{
    eu_tabpage *p = NULL;
    if ((p = on_tabpage_focus_at()) != NULL)
    {
        size_t i = 0;
        size_t base = 10;
        const size_t initial = (const size_t)eu_get_config()->eu_column.initnum;
        const size_t increase = (const size_t)eu_get_config()->eu_column.increase;
        const size_t repeat = (const size_t)eu_get_config()->eu_column.repeater;
        if (eu_get_config()->eu_column.format == IDC_COL_OCT_RADIO)
        {
            base = 8;
        }
        else if (eu_get_config()->eu_column.format == IDC_COL_HEX_RADIO)
        {
            base = 16;
        }
        else if (eu_get_config()->eu_column.format == IDC_COL_BIN_RADIO)
        {
            base = 2;
        }
        on_sci_call(p, SCI_BEGINUNDOACTION, 0, 0);
        if (on_sci_call(p, SCI_SELECTIONISRECTANGLE, 0, 0) || on_sci_call(p, SCI_GETSELECTIONS, 0, 0) > 1)
        {
            columnmodeinfo *pinfo = NULL;
            if (get_columnmodeinfo(p, &pinfo) && (i = cvector_size(pinfo)) > 0)
            {
                qsort(pinfo, i, sizeof(columnmodeinfo), on_column_position_order);
                on_column_replace(p, pinfo, initial, increase, repeat, base, eu_get_config()->eu_column.leading);
                qsort(pinfo, i, sizeof(columnmodeinfo), on_column_select_order);
                on_column_multi(p, pinfo);
            }
        }
        else if (!util_can_selections(p))
        {
            size_t *numb = NULL;
            const sptr_t cursor_pos = on_sci_call(p, SCI_GETCURRENTPOS, 0, 0);
            const sptr_t cursor_col = on_sci_call(p, SCI_GETCOLUMN, cursor_pos, 0);
            const sptr_t cursor_line = on_sci_call(p, SCI_LINEFROMPOSITION, cursor_pos, 0);
            const sptr_t end_pos = on_sci_call(p, SCI_GETLENGTH, 0, 0);
            const sptr_t end_line = on_sci_call(p, SCI_LINEFROMPOSITION, end_pos, 0);
            const size_t kmax_size = 1 + (size_t)end_line - (size_t)cursor_line;
            size_t cur_number = (size_t)initial;
            while (cvector_size(numb) < kmax_size)
            {
                for (i = 0; i < repeat; ++i)
                {
                    cvector_push_back(numb, cur_number);
                    if (cvector_size(numb) >= kmax_size)
                    {
                        break;
                    }
                }
                cur_number += increase;
            }
            if (cvector_size(numb) > 0)
            {
                char *space = NULL;
                char str[MAX_BUFFER] = {0};
                jobject_string ds = {0};
                size_t end_number = *cvector_back(numb);
                size_t nb_end = count_digits(end_number, base);
                size_t nb_init = count_digits(initial, base);
                size_t nb = MAX(nb_init, nb_end);
                for (i = cursor_line ; i <= (size_t)end_line ; ++i)
                {
                    char *line = NULL;
                    const sptr_t line_begin = on_sci_call(p, SCI_POSITIONFROMLINE, i, 0);
                    const sptr_t line_end = on_sci_call(p, SCI_GETLINEENDPOSITION, i, 0);
                    const sptr_t line_end_col = on_sci_call(p, SCI_GETCOLUMN, line_end, 0);
                    const sptr_t line_len = line_end - line_begin + 1;
                    if ((line = on_sci_range_text(p, line_begin, line_end)) != NULL && init_dynamic_string(&ds) == SKYLARK_OK)
                    {
                        append_string(&ds, line);
                        if (!number_string(str, MAX_BUFFER, *cvector_at(numb, i - cursor_line), base, nb, eu_get_config()->eu_column.leading))
                        {
                            eu_logmsg("Column: number_string failed\n");
                            destory_dynamic_string(&ds);
                            eu_safe_free(line);
                            break;
                        }
                        if (line_end_col < cursor_col)
                        {
                            if ((space = (char *)calloc(1, cursor_col - line_end_col + 1)) != NULL)
                            {
                                memset(space, 0x20, cursor_col - line_end_col);
                                append_string(&ds, space);
                                append_string(&ds, str);
                                free(space);
                            }
                        }
                        else
                        {
                            const sptr_t abs_start = on_sci_call(p, SCI_FINDCOLUMN, i, cursor_col);
                            sptr_t  relative_start = abs_start - line_begin;
                            if (relative_start > (sptr_t)ds.length)
                            {
                                relative_start = (sptr_t)ds.length;
                            }
                            insert_string(&ds, str, relative_start);
                        }
                        on_sci_call(p, SCI_SETTARGETRANGE, line_begin, line_end);
                        on_sci_call(p, SCI_REPLACETARGET, (sptr_t)-1, (sptr_t)ds.data);
                        destory_dynamic_string(&ds);
                        eu_safe_free(line);
                    }
                }
                cvector_free(numb);
            }
        }
        on_sci_call(p, SCI_ENDUNDOACTION, 0, 0);
    }
}

static void
on_column_apply(HWND hdlg)
{
    WCHAR str[QW_SIZE + 1] = {0};
    HWND hwnd_edt1 = GetDlgItem(hdlg, IDC_COL_INITNUM_EDIT);
    HWND hwnd_edt2 = GetDlgItem(hdlg, IDC_COL_INCREASENUM_EDIT);
    HWND hwnd_edt3 = GetDlgItem(hdlg, IDC_COL_REPEATNUM_EDIT);
    if (hwnd_edt1 && hwnd_edt2 && hwnd_edt3)
    {
        int ret = -1;
        Edit_GetText(hwnd_edt1, str, QW_SIZE);
        if (STR_NOT_NUL(str) && ((ret = _wtoi(str)) >= 0))
        {
            eu_get_config()->eu_column.initnum = ret;
        }
        Edit_GetText(hwnd_edt2, str, QW_SIZE);
        if (STR_NOT_NUL(str) && ((ret = _wtoi(str)) >= 0))
        {
            eu_get_config()->eu_column.increase = (ret == 0 ? 1 : ret);
        }
        Edit_GetText(hwnd_edt3, str, QW_SIZE);
        if (STR_NOT_NUL(str) && ((ret = _wtoi(str)) >= 0))
        {
            eu_get_config()->eu_column.repeater = (ret == 0 ? 1 : ret);
        }
    }
    SendMessage(hdlg, WM_CLOSE, 0, 0);
    on_column_do();
}

static INT_PTR CALLBACK
on_column_proc(HWND hdlg, uint32_t msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            WPARAM sel = 0;
            int format = IDC_COL_DEC_RADIO;
            HWND hwnd_cmb = GetDlgItem(hdlg, IDC_COL_LEADING_COMBO);
            HICON m_icon = LoadIcon(eu_module_handle(), MAKEINTRESOURCE(IDI_SKYLARK));
            if (m_icon)
            {
                SetClassLongPtr(hdlg, GCLP_HICONSM, (LONG_PTR)m_icon);
            }
            if (eu_get_config()->eu_column.leading >= IDS_COLUMN_FILL_NONE && eu_get_config()->eu_column.leading <= IDS_COLUMN_FILL_SPACES)
            {
                sel = eu_get_config()->eu_column.leading - IDS_COLUMN_FILL_NONE;
            }
            if (eu_get_config()->eu_column.format > IDC_COL_DEC_RADIO && eu_get_config()->eu_column.format <= IDC_COL_BIN_RADIO)
            {
                format = eu_get_config()->eu_column.format;
            }
            LOAD_I18N_RESSTR(IDS_COLUMN_FILL_NONE, bufn);
            LOAD_I18N_RESSTR(IDS_COLUMN_FILL_ZEROS, bufz);
            LOAD_I18N_RESSTR(IDS_COLUMN_FILL_SPACES, bufs);
            SetWindowSubclass(hwnd_cmb, on_dark_cmb_proc, COLUMN_CMB_SUBID, 0);
            SetDlgItemInt(hdlg, IDC_COL_INITNUM_EDIT, eu_get_config()->eu_column.initnum >= 0 ? eu_get_config()->eu_column.initnum : MAX_BUFFER, FALSE);
            SetDlgItemInt(hdlg, IDC_COL_INCREASENUM_EDIT, eu_get_config()->eu_column.increase >= 0 ? eu_get_config()->eu_column.increase : 1, FALSE);
            SetDlgItemInt(hdlg, IDC_COL_REPEATNUM_EDIT, eu_get_config()->eu_column.repeater >= 0 ? eu_get_config()->eu_column.repeater : 1, FALSE);
            SendDlgItemMessage(hdlg, IDC_COL_LEADING_COMBO, CB_ADDSTRING, 0, (LPARAM)bufn);
            SendDlgItemMessage(hdlg, IDC_COL_LEADING_COMBO, CB_ADDSTRING, 0, (LPARAM)bufz);
            SendDlgItemMessage(hdlg, IDC_COL_LEADING_COMBO, CB_ADDSTRING, 0, (LPARAM)bufs);
            SendMessage(hwnd_cmb, CB_SETCURSEL, sel, 0);
            SendDlgItemMessage(hdlg, format, BM_SETCHECK, TRUE, 0);
            util_creater_window(hdlg, eu_module_hwnd());
            if (on_dark_enable())
            {
                on_dark_set_theme(hdlg, L"Explorer", NULL);
            }
            return (INT_PTR)SendMessage(hdlg, WM_SETFONT, (WPARAM) on_theme_font_hwnd(), 0);
        }
        case WM_THEMECHANGED:
        {
            if (on_dark_supports())
            {
                int id = 0;
                bool dark = on_dark_enable();
                on_dark_allow_window(hdlg, dark);
                on_dark_refresh_titlebar(hdlg);
                const int buttons[] = {IDOK,
                                       IDCANCEL
                                      };
                const int bs_btn[] = {IDC_COL_FORMAT_GRP_STATIC,
                                      IDC_COL_DEC_RADIO,
                                      IDC_COL_HEX_RADIO,
                                      IDC_COL_OCT_RADIO,
                                      IDC_COL_BIN_RADIO
                                      };
                for (; id < _countof(buttons); ++id)
                {
                    HWND btn = GetDlgItem(hdlg, buttons[id]);
                    on_dark_allow_window(btn, dark);
                    on_dark_set_theme(btn, L"Explorer", NULL);
                }
                for (id = 0; id < _countof(bs_btn); ++id)
                {
                    HWND btn = GetDlgItem(hdlg, bs_btn[id]);
                    on_dark_set_theme(btn, L"", L"");
                    SendMessage(btn, WM_THEMECHANGED, 0, 0);
                }
                UpdateWindowEx(hdlg);
            }
            break;
        }
        CASE_WM_CTLCOLOR_SET:
        {
            return on_dark_set_contorl_color(wParam);
        }
        case WM_COMMAND:
        {
            WORD mid = LOWORD(wParam);
            if (HIWORD(wParam) == CBN_SELCHANGE && mid == IDC_COL_LEADING_COMBO)
            {
                int sel = (int)SendDlgItemMessage(hdlg, IDC_COL_LEADING_COMBO, CB_GETCURSEL, 0, 0);
                if (sel >= 0)
                {
                    eu_get_config()->eu_column.leading = sel + IDS_COLUMN_FILL_NONE;
                }
                return 1;
            }
            switch (mid)
            {
                case IDCANCEL:
                {
                    on_column_destory();
                    break;
                }
                case IDC_COL_DEC_RADIO:
                case IDC_COL_HEX_RADIO:
                case IDC_COL_OCT_RADIO:
                case IDC_COL_BIN_RADIO:
                {
                    eu_get_config()->eu_column.format = (int)wParam;
                    return 1;
                }
                case IDOK:
                {
                    on_column_apply(hdlg);
                    break;
                }
                default:
                {
                    break;
                }
            }
            break;
        }
        case WM_DESTROY:
        {
            if (hwnd_column)
            {
                hwnd_column = NULL;
                eu_logmsg("Column: hwnd_column destroy\n");
            }
            break;
        }
        default:
        {
            break;
        }
    }
    return 0;
}

void
on_column_destory(void)
{
    if (hwnd_column)
    {
        DestroyWindow(hwnd_column);
    }
}

void
on_column_create_dlg(HWND parent)
{
    if (!hwnd_column && !(hwnd_column = i18n_create_dialog(parent, IDD_COLUMNEDIT_DLG, on_column_proc)))
    {
        eu_logmsg("Column: %s, hwnd_column is null\n", __FUNCTION__);
    }
}

HWND
eu_column_hwnd(void)
{
    return hwnd_column;
}
