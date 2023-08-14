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

#define SQL_EXECUTE_FAIL(_n) (nret) = (_n); goto table_clean

static volatile long last_hot_item = -1;

typedef struct _column_t
{
    char data[4000 + 1];
    ub2 data_len;
    sb2 data_indicator;
} column_t, *column_ptr;

db_libs db_funcs = {0};
redis_lib redis_funcs = {0};

static void
on_table_draw_header(HWND hwnd)
{
    HWND hdr = ListView_GetHeader(hwnd);
    if (hdr && on_dark_enable())
    {
        const int bdh = GetSystemMetrics(SM_CYFRAME);
        const HDC hdc_src = GetWindowDC(hdr);
        RECT rcf = {0};
        int count = (int)SendMessage(hdr, HDM_GETITEMCOUNT, 0, 0);
        for (int k = 0; k < count; ++k)
        {
            if (SendMessage(hdr, HDM_GETITEMRECT, k, (LPARAM)&rcf))
            {
                for (int i = 1; i < bdh; ++i)
                {
                    FrameRect(hdc_src, &rcf, (HBRUSH)on_dark_theme_brush());
                    rcf.left -= 1;
                    rcf.top -= 1;
                    rcf.bottom += 1;
                    rcf.right += 1;
                }
                FrameRect(hdc_src, &rcf, GetSysColorBrush(COLOR_3DDKSHADOW));
            }
        }
        ReleaseDC(hdr, hdc_src);
    }
}

void
on_table_update_theme(eu_tabpage *pnode)
{
    EU_VERIFY(pnode != NULL);
    if (pnode->hwnd_qrtable)
    {
        SendMessage(pnode->hwnd_qrtable, WM_SETFONT, (WPARAM) on_theme_font_hwnd(), 0);
        SendMessage(pnode->hwnd_qrtable, LVM_SETTEXTCOLOR, 0, eu_get_theme()->item.text.color);
        SendMessage(pnode->hwnd_qrtable, LVM_SETOUTLINECOLOR, 0, eu_get_theme()->item.text.color);
        SendMessage(pnode->hwnd_qrtable, LVM_SETBKCOLOR, 0, eu_get_theme()->item.text.bgcolor);
        SendMessage(pnode->hwnd_qrtable, LVM_SETTEXTBKCOLOR, 0, eu_get_theme()->item.text.bgcolor);
        on_dark_set_theme(pnode->hwnd_qrtable, L"Explorer", NULL);
        HWND hdr = ListView_GetHeader(pnode->hwnd_qrtable);
        if (hdr)
        {
            const intptr_t style = GetWindowLongPtr(hdr, GWL_STYLE);
            SetWindowLongPtr(hdr, GWL_STYLE, style & ~HDS_FLAT);
            // 移除主题渲染, 使用自绘
            on_dark_set_theme(hdr, L"", L"");
            on_dark_set_theme(pnode->hwnd_qrtable, L"Explorer", NULL);
        }
    }
}

static LRESULT CALLBACK
on_table_listview_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR sub_id, DWORD_PTR dw)
{
    switch (msg)
    {
        case WM_ERASEBKGND:
        {
            return 1;
        }
        case WM_PAINT:
        {
            on_table_draw_header(hwnd);
            break;
        }
        case WM_THEMECHANGED:
        {
            UpdateWindowEx(hwnd);
            break;
        }
        case WM_SIZE:
        {
            HWND hdr = ListView_GetHeader(hwnd);
            if (hdr)
            {
                int count = (int) SendMessage(hdr, HDM_GETITEMCOUNT, 0, 0);
                if (count > 0)
                {
                    ListView_SetColumnWidth(hwnd, count - 1, LVSCW_AUTOSIZE_USEHEADER);
                }
            }
            break;
        }
        case WM_NOTIFY:
        {
            NMHDR *lpnmhdr = (NMHDR *) lp;
            HWND hdr = ListView_GetHeader(hwnd);
            if (lpnmhdr->hwndFrom == hdr && lpnmhdr->code == NM_CUSTOMDRAW)
            {
                NMCUSTOMDRAW *lpnm = (LPNMCUSTOMDRAW)lp;
                switch (lpnm->dwDrawStage)
                {
                    case CDDS_PREPAINT:
                    {
                        return CDRF_NOTIFYITEMDRAW;
                    }
                    case CDDS_ITEMPREPAINT:
                    {
                        FillRect(lpnm->hdc, &lpnm->rc, (HBRUSH)on_dark_theme_brush());
                        SetTextColor(lpnm->hdc, eu_get_theme()->item.text.color);
                        SetBkColor(lpnm->hdc, eu_get_theme()->item.text.bgcolor);
                        return CDRF_DODEFAULT;
                    }
                    default:
                        break;
                }
            }
            break;
        }
        case WM_COMMAND:
        {
            uint16_t wid = LOWORD(wp);
            TCHAR text[LARGER_LEN] = {0};
            switch (wid)
            {
                case IDM_QTABLE_LINE_COPY:
                {
                    if (last_hot_item >= 0)
                    {
                        int  column_count = (int) SendMessage(ListView_GetHeader(hwnd), HDM_GETITEMCOUNT, 0, 0);
                        TCHAR *pstr = column_count > 0 ? (TCHAR *)calloc(1, sizeof(TCHAR) * LARGER_LEN * column_count) : NULL;
                        if (pstr)
                        {
                            for (int i = 0; i < column_count; ++i)
                            {
                                ListView_GetItemText(hwnd, (int)last_hot_item, i, text, LARGER_LEN);
                                if (*text)
                                {
                                    _tcsncat(pstr, text, LARGER_LEN * column_count - 1);
                                    if (i < column_count - 1)
                                    {
                                        _tcsncat(pstr, _T("\t"), LARGER_LEN * column_count - 1);
                                    }
                                }
                            }
                            if (*pstr)
                            {
                                on_edit_push_clipboard(pstr);
                            }
                            free(pstr);
                        }
                    }
                    break;
                }
                case IDM_QTABLE_CELL_COPY:
                {
                    if (last_hot_item >= 0)
                    {
                        int index = ListView_GetSelectedColumn(hwnd);
                        ListView_GetItemText(hwnd, (int)last_hot_item, index, text, LARGER_LEN);
                        on_edit_push_clipboard(text);
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case WM_RBUTTONDOWN:
        {
            POINT pt = {GET_X_LPARAM(lp), pt.y = GET_Y_LPARAM(lp)};
            LVHITTESTINFO lvhti = {0};
            memcpy(&(lvhti.pt), &pt, sizeof(POINT));
            ListView_HitTest(hwnd, &lvhti);
            if (lvhti.flags & LVHT_ONITEMLABEL)
            {
                RECT rc;
                int col = - 1;
                int hpos = GetScrollPos(hwnd, SB_HORZ);
                HWND hdr = ListView_GetHeader(hwnd);
                int column_count = (int) SendMessage(hdr, HDM_GETITEMCOUNT, 0, 0);
                for (int i = 0; i < column_count; ++i)
                {
                    if (!i)
                    {
                        ListView_GetItemRect(hwnd, lvhti.iItem, &rc, LVIR_LABEL);
                        rc.left -= hpos;
                        rc.right -= hpos;
                    }
                    else
                    {
                        RECT rc_header;
                        Header_GetItemRect(hdr, i, &rc_header);
                        // 光标相对位置 = 光标位置 - 水平滚动条位置
                        rc.left = rc_header.left - hpos;
                        rc.right = rc_header.right - hpos;
                    }
                    if (PtInRect(&rc, pt))
                    {
                        col = i;
                        break;
                    }
                }
                // 使当前行成为焦点
                SendMessage(hwnd, WM_LBUTTONDOWN, 0, lp);
                _InterlockedExchange(&last_hot_item, (long)lvhti.iItem);
                // 弹出右键菜单
                menu_pop_track(hwnd, IDR_QTABLE_VIEW_POPUPMENU, 0, -1, NULL, NULL);
                ListView_SetSelectedColumn(hwnd, col);
                return 1;
            }
            break;
        }
        case WM_DPICHANGED:
        {
            SendMessage(hwnd, WM_SETFONT, (WPARAM) on_theme_font_hwnd(), 0);
            break;
        }
        case WM_NCDESTROY:
        {
            RemoveWindowSubclass(hwnd, on_table_listview_proc, sub_id);
            _InterlockedExchange(&last_hot_item, -1);
            break;
        }
    }
    return DefSubclassProc(hwnd, msg, wp, lp);
}

int
on_table_create_query_box(eu_tabpage *pnode)
{
    EU_VERIFY(pnode != NULL);
    if (pnode->hwnd_qrtable)
    {
        DestroyWindow(pnode->hwnd_qrtable);
    }
    /* 创建结果表格控件 */
    const uint32_t style = WS_CHILD | WS_CLIPSIBLINGS | LVS_REPORT;
    pnode->hwnd_qrtable = CreateWindow(WC_LISTVIEW, _T(""), style, 0, 0, 0, 0, eu_module_hwnd(), (HMENU)IDM_TABLE_BAR, eu_module_handle(), NULL);
    if (pnode->hwnd_qrtable == NULL)
    {
        MSG_BOX(IDC_MSG_QUERY_ERR1, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
        return 1;
    }
    SetWindowSubclass(pnode->hwnd_qrtable, on_table_listview_proc, TBCTL_LIST_SUBID, 0);
    ListView_SetExtendedListViewStyle(pnode->hwnd_qrtable, LVS_EX_FLATSB | LVS_EX_DOUBLEBUFFER | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT);
    on_table_update_theme(pnode);
    return 0;
}

int
on_table_oci_error(eu_tabpage *pnode, OCIError *errhpp, int *err_code, char *err_desc, size_t err_size)
{
    sb4 err;
    sword result;
    EU_VERIFY(pnode != NULL);
    oci_lib *oci_sub = &(db_funcs.m_oci);
    err = 0;
    err_code[0] = 0;
    result = oci_sub->fnOCIErrorGet((dvoid *) errhpp, (ub4) 1, (text *) NULL, &err, (ub1 *) err_desc, (ub4) err_size, OCI_HTYPE_ERROR);
    if (result != OCI_SUCCESS)
    {
        LOAD_I18N_RESSTR(IDC_MSG_QUERY_ERR2, err_msg);
        on_result_append_text(err_msg);
        return 1;
    }
    if (err_code)
    {
        *(err_code) = err;
    }
    return 0;
}

static bool
on_table_insert_columns(HWND hwnd, int index, const char *text)
{
    int ret = -1;
    LVCOLUMN lvc = {LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH | LVCF_FMT};
    TCHAR *ptr_name = STR_NOT_NUL(text) ? eu_utf8_utf16(text, NULL) : NULL;
    if (ptr_name)
    {
        lvc.iSubItem = index;
        lvc.pszText = ptr_name;
        lvc.fmt = LVCFMT_CENTER;
        lvc.cx = 100;
        if (!index)
        {
            ListView_DeleteColumn(hwnd, 0);
        }
        ret = ListView_InsertColumn(hwnd, index, &lvc);
        free(ptr_name);
    }
    return (ret >= 0);
}

static void
on_table_fix_columns(HWND hwnd, const unsigned int *afield)
{
    const int count = (const int) SendMessage(ListView_GetHeader(hwnd), HDM_GETITEMCOUNT, 0, 0);
    if (count <= 0)
    {
        on_table_insert_columns(hwnd, 0, "Empty Table");
        ListView_SetColumnWidth(hwnd, 0, LVSCW_AUTOSIZE_USEHEADER);
    }
    else
    {
        for (int index = 0; index < count; ++index)
        {
            if (index == count - 1)
            {
                ListView_SetColumnWidth(hwnd, index, LVSCW_AUTOSIZE_USEHEADER);
            }
            else if (afield)
            {
                ListView_SetColumnWidth(hwnd, index, afield[index]);
            }
        }
    }
}

static void
on_table_reset_table(HWND hwnd)
{
    ListView_DeleteAllItems(hwnd);
    HWND hdr = ListView_GetHeader(hwnd);
    int view_count = (int) SendMessage(hdr, HDM_GETITEMCOUNT, 0, 0);
    for (view_count--; view_count >= 0; view_count--)
    {
        ListView_DeleteColumn(hwnd, view_count);
    }
    on_table_fix_columns(hwnd, NULL);
}

static bool
on_table_insert_item(HWND hwnd, int row_index, int field_index, const char *text)
{
    int ret = -1;
    TCHAR *ptr_name = text ? eu_utf8_utf16(text, NULL) : NULL;
    if (ptr_name)
    {
        if (field_index == 0)
        {
            LVITEM lvi = {LVIF_TEXT | LVIF_STATE};
            lvi.iItem = row_index;
            lvi.stateMask = 0;
            lvi.state = 0;
            lvi.iSubItem = field_index;
            lvi.pszText = ptr_name;
            ret = ListView_InsertItem(hwnd, &lvi);
        }
        else
        {
            ListView_SetItemText(hwnd, row_index, field_index, ptr_name);
            ret = 1;
        }
        free(ptr_name);
    }
    return (ret >= 0);
}

static void
oci_database_cleanup(oci_handle *poci)
{
    if (poci)
    {
        oci_lib *oci_sub = &(db_funcs.m_oci);
        if (poci->envhpp)
        {
            oci_sub->fnOCIHandleFree((dvoid *) (poci->envhpp), OCI_HTYPE_ENV);
            poci->envhpp = NULL;
        }
        if (poci->servhpp)
        {
            oci_sub->fnOCIHandleFree((dvoid *) (poci->servhpp), OCI_HTYPE_SERVER);
            poci->servhpp = NULL;
        }
        if (poci->errhpp)
        {
            oci_sub->fnOCIHandleFree((dvoid *) (poci->errhpp), OCI_HTYPE_ERROR);
            poci->errhpp = NULL;
        }
        if (poci->usrhpp)
        {
            oci_sub->fnOCIHandleFree((dvoid *) (poci->usrhpp), OCI_HTYPE_SESSION);
            poci->usrhpp = NULL;
        }
        if (poci->svchpp)
        {
            oci_sub->fnOCIHandleFree((dvoid *) (poci->svchpp), OCI_HTYPE_SVCCTX);
            poci->svchpp = NULL;
        }
    }
}

void
eu_dbase_release(void)
{
    eu_close_dll(db_funcs.m_mysql.mysql_dll);
    eu_close_dll(db_funcs.m_oci.oci_dll);
    eu_close_dll(db_funcs.m_pq.libpq_dll);
    eu_close_dll(redis_funcs.dll);
}

void
on_table_disconnect_database(eu_tabpage *pnode, bool force)
{
    if (pnode && pnode->db_ptr)
    {
        if (_stricmp(pnode->db_ptr->config.dbtype, "MySQL") == 0)
        {
            mysql_lib *mysql_sub = &(db_funcs.m_mysql);
            mysql_handle *this_mysql = &(pnode->db_ptr->handles.h_mysql);
            if (this_mysql->mysql)
            {
                mysql_sub->fn_mysql_close(pnode->db_ptr->handles.h_mysql.mysql);
                this_mysql->mysql = NULL;
            }
        }
        else if (_stricmp(pnode->db_ptr->config.dbtype, "Oracle") == 0)
        {
            oci_database_cleanup(&(pnode->db_ptr->handles.h_oci));
        }
        else if (_stricmp(pnode->db_ptr->config.dbtype, "Sqlite3") == 0)
        {
            sql3_handle *this_sql3 = &(pnode->db_ptr->handles.h_sql3);
            if (this_sql3->sqlite3)
            {
                eu_sqlite3_close(this_sql3->sqlite3);
                this_sql3->sqlite3 = NULL;
            }
        }
        else if (_stricmp(pnode->db_ptr->config.dbtype, "PostgreSQL") == 0)
        {
            pq_lib *pq_sub = &(db_funcs.m_pq);
            pq_handle *this_pq = &(pnode->db_ptr->handles.h_pq);
            if (this_pq->postgres)
            {
                pq_sub->fnPQfinish(this_pq->postgres);
                this_pq->postgres = NULL;
            }
        }
        else
        {
            TCHAR utf_str[OVEC_LEN+1] = {0};
            LOAD_I18N_RESSTR(IDC_MSG_QUERY_ERR3, err_msg);
            on_result_append_text(err_msg, util_make_u16(pnode->db_ptr->config.dbtype, utf_str, OVEC_LEN));
        }
        pnode->db_ptr->connected = false;
        if (force)
        {
            eu_safe_free(pnode->db_ptr);
        }
    }
}

static int
on_table_connect_database(eu_tabpage *pnode)
{
    char ip[QW_SIZE+1] = {0};
    TCHAR utf_str[QW_SIZE+1] = {0};
    TCHAR user[QW_SIZE+1] = {0};
    TCHAR name[MAX_PATH+1] = {0};
    if (!(pnode && pnode->db_ptr && pnode->db_ptr->config.dbtype[0]))
    {
        return 1;
    }
    if (pnode->db_ptr->connected)
    {
        return 0;
    }
    if (_stricmp(pnode->db_ptr->config.dbtype, "Sqlite3") != 0 &&
        util_query_hostname(pnode->db_ptr->config.dbhost, ip, QW_SIZE))
    {
        eu_logmsg("%s: util_query_hostname error\n", __FUNCTION__);
        return 1;
    }
    if (_stricmp(pnode->db_ptr->config.dbtype, "MySQL") == 0)
    {
        mysql_lib *mysql_sub = &(db_funcs.m_mysql);
        mysql_handle *this_mysql = &(pnode->db_ptr->handles.h_mysql);
        if (mysql_sub->mysql_dll == NULL)
        {
            mysql_sub->mysql_dll = np_load_plugin_library(_T("libmysql.dll"), false);
            if (mysql_sub->mysql_dll == NULL)
            {
                MSG_BOX(IDC_MSG_QUERY_ERR4, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                return 1;
            }
            mysql_sub->fn_mysql_init = (p_mysql_init *) GetProcAddress(mysql_sub->mysql_dll, "mysql_init");
            mysql_sub->fn_mysql_real_connect = (p_mysql_real_connect *) GetProcAddress(mysql_sub->mysql_dll, "mysql_real_connect");
            mysql_sub->fn_mysql_set_character_set = (p_mysql_set_character_set *) GetProcAddress(mysql_sub->mysql_dll, "mysql_set_character_set");
            mysql_sub->fn_mysql_query = (p_mysql_query *) GetProcAddress(mysql_sub->mysql_dll, "mysql_query");
            mysql_sub->fn_mysql_affected_rows = (p_mysql_affected_rows *) GetProcAddress(mysql_sub->mysql_dll, "mysql_affected_rows");
            mysql_sub->fn_mysql_store_result = (p_mysql_store_result *) GetProcAddress(mysql_sub->mysql_dll, "mysql_store_result");
            mysql_sub->fn_mysql_num_fields = (p_mysql_num_fields *) GetProcAddress(mysql_sub->mysql_dll, "mysql_num_fields");
            mysql_sub->fn_mysql_fetch_field = (p_mysql_fetch_field *) GetProcAddress(mysql_sub->mysql_dll, "mysql_fetch_field");
            mysql_sub->fn_mysql_fetch_row = (p_mysql_fetch_row *) GetProcAddress(mysql_sub->mysql_dll, "mysql_fetch_row");
            mysql_sub->fn_mysql_free_result = (p_mysql_free_result *) GetProcAddress(mysql_sub->mysql_dll, "mysql_free_result");
            mysql_sub->fn_mysql_close = (p_mysql_close *) GetProcAddress(mysql_sub->mysql_dll, "mysql_close");
            if (mysql_sub->fn_mysql_init == NULL ||
                mysql_sub->fn_mysql_real_connect == NULL ||
                mysql_sub->fn_mysql_set_character_set == NULL ||
                mysql_sub->fn_mysql_query == NULL ||
                mysql_sub->fn_mysql_affected_rows == NULL ||
                mysql_sub->fn_mysql_store_result == NULL ||
                mysql_sub->fn_mysql_num_fields == NULL ||
                mysql_sub->fn_mysql_fetch_field == NULL ||
                mysql_sub->fn_mysql_fetch_row == NULL ||
                mysql_sub->fn_mysql_free_result == NULL ||
                mysql_sub->fn_mysql_close == NULL)
            {
                MSG_BOX(IDC_MSG_QUERY_ERR5, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                eu_close_dll(mysql_sub->mysql_dll);
                return 1;
            }
        }
        if (this_mysql->mysql == NULL)
        {
            this_mysql->mysql = mysql_sub->fn_mysql_init(NULL);
            if (this_mysql->mysql == NULL)
            {
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_ERR6, err_msg);
                on_result_append_text(err_msg);
                return 1;
            }
            pnode->db_ptr->connected = false;
        }
        if (!pnode->db_ptr->connected)
        {
            if (pnode->db_ptr->config.dbpass[0] == 0)
            {
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_ERR7, m_pass);
                TCHAR dbpass[QW_SIZE] = {0};
                eu_input(m_pass, dbpass, _countof(dbpass));
                if (dbpass[0] == 0)
                {
                    LOAD_I18N_RESSTR(IDC_MSG_QUERY_ERR8, err_msg);
                    on_result_append_text(err_msg);
                    mysql_sub->fn_mysql_close(this_mysql->mysql);
                    this_mysql->mysql = NULL;
                    return 1;
                }
                else
                {
                    WideCharToMultiByte(CP_UTF8, 0, dbpass, -1, pnode->db_ptr->config.dbpass, QW_SIZE, NULL, NULL);
                }
            }
            this_mysql->mysql = mysql_sub->fn_mysql_real_connect(this_mysql->mysql,
                                                                 ip,
                                                                 pnode->db_ptr->config.dbuser,
                                                                 pnode->db_ptr->config.dbpass,
                                                                 pnode->db_ptr->config.dbname,
                                                                 pnode->db_ptr->config.dbport,
                                                                 NULL,
                                                                 0);
            if (this_mysql->mysql == NULL)
            {
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_ERR9, err_msg);
                on_result_append_text(err_msg,
                                      util_make_u16(ip, utf_str, QW_SIZE),
                                      pnode->db_ptr->config.dbport,
                                      util_make_u16(pnode->db_ptr->config.dbuser, user, QW_SIZE),
                                      util_make_u16(pnode->db_ptr->config.dbname, name, MAX_PATH));
                mysql_sub->fn_mysql_close(this_mysql->mysql);
                this_mysql->mysql = NULL;
                if (!pnode->db_ptr->config.config_pass)
                {
                    pnode->db_ptr->config.dbpass[0] = 0;
                }
                return 1;
            }
            else
            {
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_ERR10, err_msg);
                on_result_append_text(err_msg,
                                      util_make_u16(ip, utf_str, QW_SIZE),
                                      pnode->db_ptr->config.dbport,
                                      util_make_u16(pnode->db_ptr->config.dbuser, user, QW_SIZE),
                                      util_make_u16(pnode->db_ptr->config.dbname, name, MAX_PATH));
            }
            if (mysql_sub->fn_mysql_set_character_set(this_mysql->mysql, "utf8mb4"))
            {
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_ERR11, err_msg);
                on_result_append_text(err_msg);
                mysql_sub->fn_mysql_close(this_mysql->mysql);
                this_mysql->mysql = NULL;
                return 1;
            }
            pnode->db_ptr->connected = true;
        }
    }
    else if (_stricmp(pnode->db_ptr->config.dbtype, "Oracle") == 0)
    {
        oci_lib *oci_sub = &(db_funcs.m_oci);
        oci_handle *this_oci = &(pnode->db_ptr->handles.h_oci);
        sword result;
        if (oci_sub->oci_dll == NULL)
        {
            oci_sub->oci_dll = np_load_plugin_library(_T("oci.dll"), false);
            if (oci_sub->oci_dll == NULL)
            {
                MSG_BOX(IDC_MSG_QUERY_ERR12, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                return 1;
            }
            oci_sub->fnOCIEnvCreate = (pOCIEnvCreate *) GetProcAddress(oci_sub->oci_dll, "OCIEnvCreate");
            oci_sub->fnOCIHandleAlloc = (pOCIHandleAlloc *) GetProcAddress(oci_sub->oci_dll, "OCIHandleAlloc");
            oci_sub->fnOCIHandleFree = (pOCIHandleFree *) GetProcAddress(oci_sub->oci_dll, "OCIHandleFree");
            oci_sub->fnOCIServerAttach = (pOCIServerAttach *) GetProcAddress(oci_sub->oci_dll, "OCIServerAttach");
            oci_sub->fnOCIServerDetach = (pOCIServerDetach *) GetProcAddress(oci_sub->oci_dll, "OCIServerDetach");
            oci_sub->fnOCISessionBegin = (pOCISessionBegin *) GetProcAddress(oci_sub->oci_dll, "OCISessionBegin");
            oci_sub->fnOCISessionEnd = (pOCISessionEnd *) GetProcAddress(oci_sub->oci_dll, "OCISessionEnd");
            oci_sub->fnOCIAttrGet = (pOCIAttrGet *) GetProcAddress(oci_sub->oci_dll, "OCIAttrGet");
            oci_sub->fnOCIAttrSet = (pOCIAttrSet *) GetProcAddress(oci_sub->oci_dll, "OCIAttrSet");
            oci_sub->fnOCIParamGet = (pOCIParamGet *) GetProcAddress(oci_sub->oci_dll, "OCIParamGet");
            oci_sub->fnOCIParamSet = (pOCIParamSet *) GetProcAddress(oci_sub->oci_dll, "OCIParamSet");
            oci_sub->fnOCIStmtPrepare = (pOCIStmtPrepare *) GetProcAddress(oci_sub->oci_dll, "OCIStmtPrepare");
            oci_sub->fnOCIStmtPrepare2 = (pOCIStmtPrepare2 *) GetProcAddress(oci_sub->oci_dll, "OCIStmtPrepare2");
            oci_sub->fnOCIDefineByPos = (pOCIDefineByPos *) GetProcAddress(oci_sub->oci_dll, "OCIDefineByPos");
            oci_sub->fnOCIDefineByPos2 = (pOCIDefineByPos2 *) GetProcAddress(oci_sub->oci_dll, "OCIDefineByPos2");
            oci_sub->fnOCIStmtExecute = (pOCIStmtExecute *) GetProcAddress(oci_sub->oci_dll, "OCIStmtExecute");
            oci_sub->fnOCIStmtFetch = (pOCIStmtFetch *) GetProcAddress(oci_sub->oci_dll, "OCIStmtFetch");
            oci_sub->fnOCIStmtFetch2 = (pOCIStmtFetch2 *) GetProcAddress(oci_sub->oci_dll, "OCIStmtFetch2");
            oci_sub->fnOCIErrorGet = (pOCIErrorGet *) GetProcAddress(oci_sub->oci_dll, "OCIErrorGet");
            oci_sub->fnOCITransStart = (pOCITransStart *) GetProcAddress(oci_sub->oci_dll, "OCITransStart");
            oci_sub->fnOCITransCommit = (pOCITransCommit *) GetProcAddress(oci_sub->oci_dll, "OCITransCommit");
            oci_sub->fnOCITransRollback = (pOCITransRollback *) GetProcAddress(oci_sub->oci_dll, "OCITransRollback");
            oci_sub->fnOCITransDetach = (pOCITransDetach *) GetProcAddress(oci_sub->oci_dll, "OCITransDetach");
            if (oci_sub->fnOCIEnvCreate == NULL ||
                oci_sub->fnOCIEnvCreate == NULL ||
                oci_sub->fnOCIHandleAlloc == NULL ||
                oci_sub->fnOCIHandleFree == NULL ||
                oci_sub->fnOCIServerAttach == NULL ||
                oci_sub->fnOCIServerDetach == NULL ||
                oci_sub->fnOCISessionBegin == NULL ||
                oci_sub->fnOCISessionEnd == NULL ||
                oci_sub->fnOCIAttrGet == NULL ||
                oci_sub->fnOCIAttrSet == NULL ||
                oci_sub->fnOCIParamGet == NULL ||
                oci_sub->fnOCIParamSet == NULL ||
                oci_sub->fnOCIStmtPrepare == NULL ||
                oci_sub->fnOCIStmtPrepare2 == NULL ||
                oci_sub->fnOCIDefineByPos == NULL ||
                oci_sub->fnOCIDefineByPos2 == NULL ||
                oci_sub->fnOCIStmtExecute == NULL ||
                oci_sub->fnOCIStmtFetch == NULL ||
                oci_sub->fnOCIStmtFetch2 == NULL ||
                oci_sub->fnOCIErrorGet == NULL ||
                oci_sub->fnOCITransStart == NULL ||
                oci_sub->fnOCITransCommit == NULL ||
                oci_sub->fnOCITransRollback == NULL ||
                oci_sub->fnOCITransDetach == NULL)
            {
                MSG_BOX(IDC_MSG_QUERY_ERR13, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                eu_close_dll(oci_sub->oci_dll);
                return 1;
            }
        }
        if (this_oci->envhpp == NULL)
        {
            result = oci_sub->fnOCIEnvCreate(&(this_oci->envhpp), OCI_DEFAULT, NULL, NULL, NULL, NULL, 0, NULL);
            if (result != OCI_SUCCESS && result != OCI_SUCCESS_WITH_INFO)
            {
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_ERR14, err_msg);
                on_result_append_text(err_msg);
                return 1;
            }
            oci_sub->fnOCIHandleAlloc((dvoid *) (this_oci->envhpp), (dvoid **) &(this_oci->errhpp), OCI_HTYPE_ERROR, (size_t) 0, (dvoid **) 0);
            pnode->db_ptr->connected = false;
        }
        if (!pnode->db_ptr->connected)
        {
            if (pnode->db_ptr->config.dbpass[0] == 0)
            {
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_ERR7, m_pass);
                TCHAR dbpass[QW_SIZE] = {0};
                eu_input(m_pass, dbpass, _countof(dbpass));
                if (pnode->db_ptr->config.dbpass[0] == 0)
                {
                    LOAD_I18N_RESSTR(IDC_MSG_QUERY_ERR8, err_msg);
                    on_result_append_text(err_msg);
                    return 1;
                }
                else
                {
                    WideCharToMultiByte(CP_UTF8, 0, dbpass, -1, pnode->db_ptr->config.dbpass, QW_SIZE, NULL, NULL);
                }
            }
            oci_sub->fnOCIHandleAlloc((dvoid *) (this_oci->envhpp), (dvoid **) &(this_oci->servhpp), OCI_HTYPE_SERVER, (size_t) 0, (dvoid **) 0);

            result = oci_sub->fnOCIServerAttach(this_oci->servhpp, this_oci->errhpp, (text *) ip, (sb4) strlen(ip), 0);
            if (result != OCI_SUCCESS)
            {
                int err_code;
                char err_desc[512] = { 0 };
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_ERR15, err_msg);
                on_table_oci_error(pnode, this_oci->errhpp, &err_code, err_desc, _countof(err_desc) - 1);
                on_result_append_text(err_msg, util_make_u16(ip, utf_str, QW_SIZE), err_code, util_make_u16(err_desc, user, QW_SIZE));
                oci_database_cleanup(this_oci);
                return 1;
            }
            else
            {
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_ERR16, err_msg);
                on_result_append_text(err_msg, util_make_u16(ip, utf_str, QW_SIZE));
            }
            oci_sub->fnOCIHandleAlloc((dvoid *) (this_oci->envhpp), (dvoid **) &(this_oci->svchpp), OCI_HTYPE_SVCCTX, (size_t) 0, (dvoid **) 0);
            oci_sub->fnOCIAttrSet((dvoid *) (this_oci->svchpp),
                                  OCI_HTYPE_SVCCTX,
                                  (dvoid *) (this_oci->servhpp),
                                  (ub4) 0,
                                  OCI_ATTR_SERVER,
                                  (OCIError *) (this_oci->errhpp));

            oci_sub->fnOCIHandleAlloc((dvoid *) (this_oci->envhpp), (dvoid **) &(this_oci->usrhpp), (ub4) OCI_HTYPE_SESSION, (size_t) 0, (dvoid **) 0);

            oci_sub->fnOCIAttrSet((dvoid *) (this_oci->usrhpp),
                                  (ub4) OCI_HTYPE_SESSION,
                                  (dvoid *) (pnode->db_ptr->config.dbuser),
                                  (ub4) strlen(pnode->db_ptr->config.dbuser),
                                  (ub4) OCI_ATTR_USERNAME,
                                  this_oci->errhpp);
            oci_sub->fnOCIAttrSet((dvoid *) (this_oci->usrhpp),
                                  (ub4) OCI_HTYPE_SESSION,
                                  (dvoid *) (pnode->db_ptr->config.dbpass),
                                  (ub4) strlen(pnode->db_ptr->config.dbpass),
                                  (ub4) OCI_ATTR_PASSWORD,
                                  this_oci->errhpp);

            result = oci_sub->fnOCISessionBegin(this_oci->svchpp, this_oci->errhpp, this_oci->usrhpp, OCI_CRED_RDBMS, (ub4) OCI_DEFAULT);
            if (result != OCI_SUCCESS)
            {
                int err_code;
                char err_desc[512] = { 0 };
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_ERR17, err_msg);
                on_table_oci_error(pnode, this_oci->errhpp, &err_code, err_desc, _countof(err_desc) - 1);
                on_result_append_text(err_msg, err_code, err_desc, pnode->db_ptr->config.dbuser);
                oci_database_cleanup(this_oci);
                return 1;
            }
            else
            {
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_ERR18, err_msg);
                on_result_append_text(err_msg, util_make_u16(pnode->db_ptr->config.dbuser, user, QW_SIZE));
            }
            oci_sub->fnOCIAttrSet((dvoid *) (this_oci->svchpp),
                                  (ub4) OCI_HTYPE_SVCCTX,
                                  (dvoid *) (this_oci->usrhpp),
                                  (ub4) 0,
                                  (ub4) OCI_ATTR_SESSION,
                                  this_oci->errhpp);
            pnode->db_ptr->connected = true;
        }
    }
    else if (_stricmp(pnode->db_ptr->config.dbtype, "Sqlite3") == 0)
    {
        sql3_handle *this_sql3 = &(pnode->db_ptr->handles.h_sql3);
        if (this_sql3->sqlite3 == NULL)
        {
            if (eu_sqlite3_open(pnode->db_ptr->config.dbname, &(this_sql3->sqlite3)) != SQLITE_OK)
            {
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_ERR19, err_msg);
                on_result_append_text(err_msg);
                return 1;
            }
            LOAD_I18N_RESSTR(IDC_MSG_QUERY_ERR20, err_msg);
            on_result_append_text(err_msg, util_make_u16(pnode->db_ptr->config.dbname, name, MAX_PATH));
            pnode->db_ptr->connected = true;
        }
    }
    else if (_stricmp(pnode->db_ptr->config.dbtype, "PostgreSQL") == 0)
    {
        pq_lib *pq_sub = &(db_funcs.m_pq);
        pq_handle *this_pq = &(pnode->db_ptr->handles.h_pq);
        if (pq_sub->libpq_dll == NULL)
        {
            pq_sub->libpq_dll = np_load_plugin_library(_T("libpq.dll"), false);
            if (pq_sub->libpq_dll == NULL)
            {
                MSG_BOX(IDC_MSG_QUERY_ERR21, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                return 1;
            }
            pq_sub->fnPQsetdbLogin = (pPQsetdbLogin *) GetProcAddress(pq_sub->libpq_dll, "PQsetdbLogin");
            pq_sub->fnPQfinish = (pPQfinish *) GetProcAddress(pq_sub->libpq_dll, "PQfinish");
            pq_sub->fnPQexec = (pPQexec *) GetProcAddress(pq_sub->libpq_dll, "PQexec");
            pq_sub->fnPQcmdTuples = (pPQcmdTuples *) GetProcAddress(pq_sub->libpq_dll, "PQcmdTuples");
            pq_sub->fnPQntuples = (pPQntuples *) GetProcAddress(pq_sub->libpq_dll, "PQntuples");
            pq_sub->fnPQnfields = (pPQnfields *) GetProcAddress(pq_sub->libpq_dll, "PQnfields");
            pq_sub->fnPQfname = (pPQfname *) GetProcAddress(pq_sub->libpq_dll, "PQfname");
            pq_sub->fnPQgetvalue = (pPQgetvalue *) GetProcAddress(pq_sub->libpq_dll, "PQgetvalue");
            pq_sub->fnPQgetisnull = (pPQgetisnull *) GetProcAddress(pq_sub->libpq_dll, "PQgetisnull");
            pq_sub->fnPQclear = (pPQclear *) GetProcAddress(pq_sub->libpq_dll, "PQclear");
            pq_sub->fnPQresultStatus = (pPQresultStatus *) GetProcAddress(pq_sub->libpq_dll, "PQresultStatus");
            pq_sub->fnPQresultErrorMessage = (pPQresultErrorMessage *) GetProcAddress(pq_sub->libpq_dll, "PQresultErrorMessage");
            pq_sub->fnPQsetClientEncoding = (pPQsetClientEncoding *) GetProcAddress(pq_sub->libpq_dll, "PQsetClientEncoding");
            if (pq_sub->fnPQsetdbLogin == NULL ||
                pq_sub->fnPQfinish == NULL ||
                pq_sub->fnPQexec == NULL ||
                pq_sub->fnPQcmdTuples == NULL ||
                pq_sub->fnPQntuples == NULL ||
                pq_sub->fnPQnfields == NULL ||
                pq_sub->fnPQfname == NULL ||
                pq_sub->fnPQgetvalue == NULL ||
                pq_sub->fnPQgetisnull == NULL ||
                pq_sub->fnPQclear == NULL ||
                pq_sub->fnPQresultStatus == NULL ||
                pq_sub->fnPQresultErrorMessage == NULL ||
                pq_sub->fnPQsetClientEncoding == NULL)
            {
                MSG_BOX(IDC_MSG_QUERY_ERR22, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                eu_close_dll(pq_sub->libpq_dll);
                return 1;
            }
        }
        if (this_pq->postgres == NULL || pnode->db_ptr->connected == false)
        {
            char src_port[20 + 1] = {0};
            snprintf(src_port, 20, "%d", pnode->db_ptr->config.dbport);
            this_pq->postgres = pq_sub->fnPQsetdbLogin(ip,
                                                       src_port,
                                                       NULL,
                                                       NULL,
                                                       pnode->db_ptr->config.dbname,
                                                       pnode->db_ptr->config.dbuser,
                                                       pnode->db_ptr->config.dbpass);
            if (this_pq->postgres == NULL)
            {
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_ERR23, err_msg);
                on_result_append_text(err_msg);
                return 1;
            }
            if (pq_sub->fnPQsetClientEncoding(this_pq->postgres, eu_query_encoding_name(pnode->codepage)))
            {
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_ERR24, err_msg);
                on_result_append_text(err_msg);
                pq_sub->fnPQfinish(this_pq->postgres);
                this_pq->postgres = NULL;
                return 1;
            }
            LOAD_I18N_RESSTR(IDC_MSG_QUERY_ERR25, err_msg);
            on_result_append_text(err_msg,
                                  util_make_u16(ip, utf_str, QW_SIZE),
                                  atoi(src_port),
                                  util_make_u16(pnode->db_ptr->config.dbname, user, QW_SIZE),
                                  util_make_u16(pnode->db_ptr->config.dbuser, name, MAX_PATH));
            pnode->db_ptr->connected = true;
        }
    }
    else
    {
        LOAD_I18N_RESSTR(IDC_MSG_QUERY_ERR26, err_msg);
        on_result_append_text(err_msg, util_make_u16(pnode->db_ptr->config.dbtype, utf_str, QW_SIZE));
    }
    return 0;
}

bool
on_table_sql_header(eu_tabpage *pnode)
{
    sptr_t file_line;
    sptr_t file_line_count;
    bool in_config_secion = false;
    bool m_change = false;
    db_config dbconfig = {0};
    if (!pnode->db_ptr)
    {
        if (!(pnode->db_ptr = (db_conn *)calloc(1, sizeof(db_conn))))
        {
            return false;
        }
    }
    file_line_count = eu_sci_call(pnode, SCI_GETLINECOUNT, 0, 0);
    for (file_line = 0; file_line <= file_line_count; file_line++)
    {
        char line_buf[FILESIZE+1] = {0};
        char config_remark[QW_SIZE] = {0};
        char config_key[QW_SIZE] = {0};
        char config_eval[QW_SIZE] = {0};
        char config_value[QW_SIZE] = {0};
        if (!on_sci_line_text(pnode, file_line, line_buf, _countof(line_buf)))
        {
            return false;
        }
        if (!in_config_secion)
        {
            if (!strncmp(util_trim_left_white(line_buf, NULL), BEGIN_DATABASE_CONNECTION_CONFIG, strlen(BEGIN_DATABASE_CONNECTION_CONFIG)))
            {
                in_config_secion = true;
            }
        }
        else
        {
            if (!strncmp(util_trim_left_white(line_buf, NULL), END_DATABASE_CONNECTION_CONFIG, strlen(END_DATABASE_CONNECTION_CONFIG)))
            {
                if (dbconfig.dbtype[0])
                {
                    if ((pnode->db_ptr->config.dbtype[0] && strcmp(pnode->db_ptr->config.dbtype, dbconfig.dbtype)) ||
                        (pnode->db_ptr->config.dbhost[0] && strcmp(pnode->db_ptr->config.dbhost, dbconfig.dbhost)) ||
                        (pnode->db_ptr->config.dbport && pnode->db_ptr->config.dbport != dbconfig.dbport) ||
                        (pnode->db_ptr->config.dbuser[0] && strcmp(pnode->db_ptr->config.dbuser, dbconfig.dbuser)) ||
                        (pnode->db_ptr->config.dbpass[0] && strcmp(pnode->db_ptr->config.dbpass, dbconfig.dbpass)) ||
                        (pnode->db_ptr->config.dbname[0] && strcmp(pnode->db_ptr->config.dbname, dbconfig.dbname)))
                    {
                        m_change = true;
                    }
                    if (dbconfig.dbpass[0] || _stricmp(pnode->db_ptr->config.dbtype, "Sqlite3") == 0)
                    {
                        dbconfig.config_pass = true;
                    }
                    else
                    {
                        dbconfig.config_pass = false;
                    }
                    memcpy(&(pnode->db_ptr->config), &dbconfig, sizeof(dbconfig));
                }
                break;
            }
            sscanf(line_buf, "%s%s%s%*[ ]%[^\r|\n]", config_remark, config_key, config_eval, config_value);
            if (strcmp(config_remark, "--") != 0 || strcmp(config_eval, ":") != 0)
            {
                continue;
            }
            if (strcmp(config_key, "DBTYPE") == 0)
            {
                strncpy(pnode->db_ptr->config.dbtype, config_value, sizeof(dbconfig.dbtype) - 1);
            }
            else if (strcmp(config_key, "DBHOST") == 0)
            {
                strncpy(pnode->db_ptr->config.dbhost, config_value, sizeof(dbconfig.dbhost) - 1);
            }
            else if (strcmp(config_key, "DBPORT") == 0)
            {
                pnode->db_ptr->config.dbport = atoi(config_value);
            }
            else if (strcmp(config_key, "DBUSER") == 0)
            {
                strncpy(pnode->db_ptr->config.dbuser, config_value, sizeof(dbconfig.dbuser) - 1);
            }
            else if (strcmp(config_key, "DBPASS") == 0)
            {
                strncpy(pnode->db_ptr->config.dbpass, config_value, sizeof(dbconfig.dbpass) - 1);
            }
            else if (strcmp(config_key, "DBNAME") == 0)
            {
                strncpy(pnode->db_ptr->config.dbname, config_value, sizeof(dbconfig.dbname) - 1);
            }
        }
    }
    if (m_change && pnode->db_ptr->connected)
    {
        on_table_disconnect_database(pnode, false);
    }
    return (on_table_connect_database(pnode) == 0);
}

bool
on_table_skip_comment(eu_tabpage *pnode, char **psql)
{
    bool ret = false;
    char *s = NULL;
    const char *p = NULL;
    const int m_eol = on_encoding_eol_char(pnode);
    if (STR_IS_NUL(psql))
    {
        return false;
    }
    s = *psql;
    for (; *s; )
    {
        if (*s == m_eol)
        {
            ++s;
            continue;
        }
        if (util_strnspace(s, "--", NULL) == 0)
        {
            if ((p = strchr(s, m_eol)) != NULL)
            {
                s += (p-s+1);
                continue;
            }
            break;
        }
        else if (!isspace(*s))
        {
            *psql = s;
            ret = true;
            break;
        }
        ++s;
    }
    return ret;
}

int
on_table_sql_query(eu_tabpage *pnode, const char *pq, bool vcontrol, bool clear)
{
    int sel_start = 0;
    int sel_end = 0;
    int sel_len = 0;
    int row_index = 0;
    int nret = 0;
    int field_count = 0;
    int field_index = 0;
    char *sel_sql = NULL;
    char *psel = NULL;
    char *field_value = NULL;
    unsigned int nfield_width = 0;
    unsigned int *afield_width = NULL;
    TCHAR utf_str[MAX_BUFFER+1] = {0};
    EU_VERIFY(pnode != NULL);
    int char_width = (int)eu_sci_call(pnode, SCI_TEXTWIDTH, STYLE_DEFAULT, (sptr_t) "X");
    eu_sci_call(pnode->presult, SCI_SETREADONLY, 0, 0);
    eu_sci_call(pnode->presult, SCI_SETKEYWORDS, 0, (sptr_t)"|");
    if (clear)
    {
        eu_sci_call(pnode->presult, SCI_CLEARALL, 0, 0);
    }
    else
    {
        eu_sci_call(pnode->presult, SCI_ADDTEXT, 1, (LPARAM)"\n");
    }
    if (!on_table_sql_header(pnode))
    {
        eu_logmsg("%s: on_table_sql_header failed\n", __FUNCTION__);
        goto table_clean;
    }
    if (vcontrol && on_symtree_do_sql(pnode, false))
    {
        eu_logmsg("%s: on_symtree_do_sql failed\n", __FUNCTION__);
        goto table_clean;
    }
    if (!pq)
    {
        sel_start = (int) eu_sci_call(pnode, SCI_GETSELECTIONSTART, 0, 0);
        sel_end = (int) eu_sci_call(pnode, SCI_GETSELECTIONEND, 0, 0);
        sel_len = sel_end - sel_start;
        if (sel_len <= 0)
        {
            sel_sql = util_strdup_line(pnode, -1, NULL);
        }
        else
        {
            sel_sql = on_sci_range_text(pnode, sel_start, sel_end);
        }
    }
    else
    {
        sel_sql = _strdup(pq);
    }
    if (sel_sql == NULL)
    {
        eu_logmsg("%s: failed to allocate memory\n", __FUNCTION__);
        goto table_clean;
    }
    on_table_reset_table(pnode->hwnd_qrtable);
    psel = strtok(sel_sql, ";");
    while (psel)
    {
        if (!on_table_skip_comment(pnode, &psel))
        {
            psel = strtok(NULL, ";");
            continue;
        }
        bool is_select_word = _strnicmp(psel, "SELECT", strlen("SELECT")) == 0;
        if (_stricmp(pnode->db_ptr->config.dbtype, "MySQL") == 0)
        {
            mysql_lib *mysql_sub = &(db_funcs.m_mysql);
            mysql_handle *this_mysql = &(pnode->db_ptr->handles.h_mysql);
            MYSQL_RES *presult = NULL;
            if ((nret = mysql_sub->fn_mysql_query(this_mysql->mysql, psel)))
            {
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_STR2, msg_str);
                on_result_append_text(msg_str, nret);
                SQL_EXECUTE_FAIL(nret);
            }
            else if (!is_select_word)
            {
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_STR3, msg_str);
                on_result_append_text(msg_str, (int)(mysql_sub->fn_mysql_affected_rows(this_mysql->mysql)));
            }
            else
            {
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_STR1, msg_str);
                on_result_append_text(msg_str, util_make_u16(psel, utf_str, MAX_BUFFER));
            }
            if ((presult = mysql_sub->fn_mysql_store_result(this_mysql->mysql)))
            {
                MYSQL_FIELD *mysql_field = NULL;
                MYSQL_ROW mysqlrow;
                row_index = 0;
                field_count = mysql_sub->fn_mysql_num_fields(presult);
                if (!field_count)
                {   // 空表
                    mysql_sub->fn_mysql_free_result(presult);
                    SQL_EXECUTE_FAIL(1);
                }
                if ((afield_width = (unsigned int *) calloc(1, sizeof(unsigned int) * field_count)) == NULL)
                {
                    eu_logmsg("%s: Failed to allocate memory\n", __FUNCTION__);
                    mysql_sub->fn_mysql_free_result(presult);
                    SQL_EXECUTE_FAIL(1);
                }
                on_table_reset_table(pnode->hwnd_qrtable);
                for (field_index = 0; field_index < field_count; field_index++)
                {
                    mysql_field = mysql_sub->fn_mysql_fetch_field(presult);
                    if (!on_table_insert_columns(pnode->hwnd_qrtable, field_index, mysql_field->name))
                    {
                        MSG_BOX(IDC_MSG_QUERY_STR4, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                        mysql_sub->fn_mysql_free_result(presult);
                        SQL_EXECUTE_FAIL(1);
                    }
                    nfield_width = ((int) strlen(mysql_field->name) + 2) * char_width;
                    if (afield_width[field_index] < nfield_width)
                    {
                        afield_width[field_index] = nfield_width;
                    }
                }
                while ((mysqlrow = mysql_sub->fn_mysql_fetch_row(presult)))
                {
                    for (field_index = 0; field_index < field_count; field_index++)
                    {
                        if (!(on_table_insert_item(pnode->hwnd_qrtable, row_index, field_index, mysqlrow[field_index])))
                        {
                            MSG_BOX(IDC_MSG_QUERY_STR5, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                            mysql_sub->fn_mysql_free_result(presult);
                            SQL_EXECUTE_FAIL(1);
                        }
                        if (mysqlrow[field_index])
                        {
                            nfield_width = ((int) strlen(mysqlrow[field_index]) + 2) * char_width;
                            if (afield_width[field_index] < nfield_width)
                            {
                                afield_width[field_index] = nfield_width;
                            }
                        }
                    }
                    row_index++;
                }
                mysql_sub->fn_mysql_free_result(presult);
            }
        }
        else if (_stricmp(pnode->db_ptr->config.dbtype, "Oracle") == 0)
        {
            oci_lib *oci_sub = &(db_funcs.m_oci);
            oci_handle *this_oci = &(pnode->db_ptr->handles.h_oci);
            sword result;
            OCIStmt *stmthpp = NULL;
            ub2 stmt_type;
            OCITrans *txnhpp = NULL;
            OCIParam *paramhpp = NULL;
            text *column_name = NULL;
            ub4 column_name_len = 0;
            column_ptr column_data = NULL;
            OCIDefine *column_ocid = NULL;
            oci_sub->fnOCIHandleAlloc((dvoid *) (this_oci->envhpp), (dvoid **) &stmthpp, OCI_HTYPE_STMT, (size_t) 0, (dvoid **) 0);
            result = oci_sub->fnOCIStmtPrepare(stmthpp,
                                               this_oci->errhpp,
                                               (text *) psel,
                                               (ub4) strlen(psel),
                                               (ub4) OCI_NTV_SYNTAX,
                                               (ub4) OCI_DEFAULT);
            if (result != OCI_SUCCESS)
            {
                int err_code;
                char err_desc[512] = { 0 };
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_STR6, msg_str);
                on_table_oci_error(pnode, this_oci->errhpp, &err_code, err_desc, sizeof(err_desc) - 1);
                on_result_append_text(msg_str, err_code, util_make_u16(err_desc, utf_str, MAX_BUFFER));
                SQL_EXECUTE_FAIL(1);
            }
            oci_sub->fnOCIAttrGet(stmthpp, OCI_HTYPE_STMT, &stmt_type, NULL, OCI_ATTR_STMT_TYPE, this_oci->errhpp);
            if (stmt_type != OCI_STMT_SELECT)
            {
                oci_sub->fnOCIHandleAlloc((void *) (this_oci->envhpp), (void **) &txnhpp, OCI_HTYPE_TRANS, 0, 0);
                oci_sub->fnOCIAttrSet((void *) (this_oci->svchpp), OCI_HTYPE_SVCCTX, (void *) txnhpp, 0, OCI_ATTR_TRANS, this_oci->errhpp);

                oci_sub->fnOCITransStart(this_oci->svchpp, this_oci->errhpp, 10, OCI_TRANS_NEW);
            }
            result = oci_sub->fnOCIStmtExecute(this_oci->svchpp,
                                                 stmthpp,
                                                 this_oci->errhpp,
                                                 (ub4)(stmt_type == OCI_STMT_SELECT ? 0 : 1),
                                                 (ub4) 0,
                                                 (OCISnapshot *) NULL,
                                                 (OCISnapshot *) NULL,
                                                 OCI_DEFAULT);
            if (result != OCI_SUCCESS)
            {
                int err_code;
                char err_desc[512] = { 0 };
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_STR7, msg_str);
                on_table_oci_error(pnode, this_oci->errhpp, &err_code, err_desc, _countof(err_desc) - 1);
                on_result_append_text(msg_str, err_code, util_make_u16(err_desc, utf_str, MAX_BUFFER));
                if (stmt_type != OCI_STMT_SELECT)
                {
                    LOAD_I18N_RESSTR(IDC_MSG_QUERY_STR8, msg_str);
                    oci_sub->fnOCITransRollback(this_oci->svchpp, this_oci->errhpp, OCI_DEFAULT);
                    on_result_append_text(msg_str);
                }
                SQL_EXECUTE_FAIL(1);
            }
            else
            {
                if (stmt_type != OCI_STMT_SELECT)
                {
                    LOAD_I18N_RESSTR(IDC_MSG_QUERY_STR9, msg_str);
                    oci_sub->fnOCITransCommit(this_oci->svchpp, this_oci->errhpp, OCI_DEFAULT);
                    on_result_append_text(msg_str);
                    oci_sub->fnOCIHandleFree((void *) txnhpp, OCI_HTYPE_TRANS);
                }
            }
            if (stmt_type == OCI_STMT_SELECT)
            {
                row_index = 0;
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_STR10, msg_str);
                on_result_append_text(msg_str);
                ub4 nFieldCount2;
                oci_sub->fnOCIAttrGet((dvoid *) stmthpp, OCI_HTYPE_STMT, &nFieldCount2, 0, OCI_ATTR_PARAM_COUNT, this_oci->errhpp);
                field_count = (int) nFieldCount2;
                if (!field_count)
                {   // 空表
                    oci_sub->fnOCIHandleFree((dvoid *) stmthpp, OCI_HTYPE_STMT);
                    SQL_EXECUTE_FAIL(1);
                }
                if ((afield_width = (unsigned int *) calloc(1, sizeof(unsigned int) * field_count)) == NULL)
                {
                    MSG_BOX(IDC_MSG_QUERY_STR11, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                    oci_sub->fnOCIHandleFree((dvoid *) stmthpp, OCI_HTYPE_STMT);
                    SQL_EXECUTE_FAIL(1);
                }
                if ((column_data = (column_ptr) calloc(1, sizeof(column_t) * field_count)) == NULL)
                {
                    MSG_BOX(IDC_MSG_QUERY_STR12, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                    oci_sub->fnOCIHandleFree((dvoid *) stmthpp, OCI_HTYPE_STMT);
                    SQL_EXECUTE_FAIL(1);
                }
                on_table_reset_table(pnode->hwnd_qrtable);
                for (field_index = 0; field_index < field_count; field_index++)
                {
                    TCHAR *ptr_table = NULL;
                    result = oci_sub->fnOCIParamGet(stmthpp, OCI_HTYPE_STMT, this_oci->errhpp, (dvoid **) &paramhpp, field_index + 1);
                    if (result != OCI_SUCCESS)
                    {
                        int err_code;
                        char err_desc[512] = { 0 };
                        LOAD_I18N_RESSTR(IDC_MSG_QUERY_STR13, msg_str);
                        on_table_oci_error(pnode, this_oci->errhpp, &err_code, err_desc, _countof(err_desc) - 1);
                        on_result_append_text(msg_str, field_index, err_code, util_make_u16(err_desc, utf_str, MAX_BUFFER));
                        oci_sub->fnOCIHandleFree((dvoid *) stmthpp, OCI_HTYPE_STMT);
                        eu_safe_free(column_data);
                        SQL_EXECUTE_FAIL(1);
                    }
                    oci_sub->fnOCIAttrGet((dvoid *) paramhpp,
                                          OCI_DTYPE_PARAM,
                                          (dvoid *) &column_name,
                                          &column_name_len,
                                          OCI_ATTR_NAME,
                                          this_oci->errhpp);
                    if (!on_table_insert_columns(pnode->hwnd_qrtable, field_index, (const char *)column_name))
                    {
                        MSG_BOX(IDC_MSG_QUERY_STR4, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                        oci_sub->fnOCIHandleFree((dvoid *) stmthpp, OCI_HTYPE_STMT);
                        eu_safe_free(column_data);
                        SQL_EXECUTE_FAIL(1);
                    }
                    oci_sub->fnOCIDefineByPos(stmthpp,
                                              &column_ocid,
                                              this_oci->errhpp,
                                              field_index + 1,
                                              (dvoid *) (column_data[field_index].data),
                                              sizeof(column_data[field_index].data) - 1,
                                              SQLT_STR,
                                              (void *) &(column_data[field_index].data_indicator),
                                              &(column_data[field_index].data_len),
                                              NULL,
                                              OCI_DEFAULT);

                    nfield_width = ((int) column_name_len + 2) * char_width;
                    if (afield_width[field_index] < nfield_width)
                    {
                        afield_width[field_index] = nfield_width;
                    }
                }
                while (1)
                {
                    result = oci_sub->fnOCIStmtFetch2(stmthpp, this_oci->errhpp, 1, OCI_FETCH_NEXT, 1, OCI_DEFAULT);
                    if (result == OCI_NO_DATA)
                    {
                        break;
                    }
                    else if (result != OCI_SUCCESS)
                    {
                        oci_sub->fnOCIHandleFree((dvoid *) stmthpp, OCI_HTYPE_STMT);
                        eu_safe_free(column_data);
                        SQL_EXECUTE_FAIL(1);
                    }
                    for (field_index = 0; field_index < field_count; field_index++)
                    {
                        if (!(on_table_insert_item(pnode->hwnd_qrtable, row_index, field_index, column_data[field_index].data)))
                        {
                            MSG_BOX(IDC_MSG_QUERY_STR5, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                            oci_sub->fnOCIHandleFree((dvoid *) stmthpp, OCI_HTYPE_STMT);
                            eu_safe_free(column_data);
                            SQL_EXECUTE_FAIL(1);
                        }
                        if (column_data[field_index].data_len > 0)
                        {
                            nfield_width = ((int) (column_data[field_index].data_len) + 2) * char_width;
                            if (afield_width[field_index] < nfield_width)
                            {
                                afield_width[field_index] = nfield_width;
                            }
                        }
                    }
                    row_index++;
                }
                eu_safe_free(column_data);
            }
            else
            {
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_STR10, msg_str);
                on_result_append_text(msg_str);
            }
            oci_sub->fnOCIHandleFree((dvoid *) stmthpp, OCI_HTYPE_STMT);
        }
        else if (_stricmp(pnode->db_ptr->config.dbtype, "Sqlite3") == 0)
        {
            int index;
            int row_conut;
            char *errmsg = NULL;
            char **result = NULL;
            sql3_handle *this_sql3 = &(pnode->db_ptr->handles.h_sql3);
            if (is_select_word)
            {
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_STR1, msg_str);
                on_result_append_text(msg_str, util_make_u16(psel, utf_str, MAX_BUFFER));
                if ((nret = eu_sqlite3_get_table(this_sql3->sqlite3, psel, &result, &row_conut, &field_count, &errmsg)))
                {
                    LOAD_I18N_RESSTR(IDC_MSG_QUERY_STR7, msg_str);
                    on_result_append_text(msg_str, nret, util_make_u16(errmsg, utf_str, MAX_BUFFER));
                    eu_sqlite3_free(errmsg);
                    eu_sqlite3_free_table(result);
                    SQL_EXECUTE_FAIL(nret);
                }
                else
                {
                    LOAD_I18N_RESSTR(IDC_MSG_QUERY_STR10, msg_str);
                    on_result_append_text(msg_str);
                }
            }
            else
            {
                if ((nret = eu_sqlite3_exec(this_sql3->sqlite3, psel, NULL, NULL, &errmsg)))
                {
                    eu_logmsg("%s: eu_sqlite3_exec failed\n", __FUNCTION__);
                    LOAD_I18N_RESSTR(IDC_MSG_QUERY_STR7, msg_str);
                    on_result_append_text(msg_str, nret, util_make_u16(errmsg, utf_str, MAX_BUFFER));
                    eu_sqlite3_free(errmsg);
                    SQL_EXECUTE_FAIL(nret);
                }
                else
                {
                    LOAD_I18N_RESSTR(IDC_MSG_QUERY_STR10, msg_str);
                    on_result_append_text(msg_str);
                }
            }
            if (!field_count)
            {
                eu_sqlite3_free(errmsg);
                eu_sqlite3_free_table(result);
                SQL_EXECUTE_FAIL(0);
            }
            if (is_select_word)
            {
                TCHAR *ptr_index = NULL;
                if ((afield_width = (unsigned int *) calloc(1, sizeof(unsigned int) * field_count)) == NULL)
                {
                    MSG_BOX(IDC_MSG_QUERY_STR11, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                    eu_sqlite3_free_table(result);
                    SQL_EXECUTE_FAIL(1);
                }
                on_table_reset_table(pnode->hwnd_qrtable);
                for (field_index = 0, index = 0; field_index < field_count; field_index++, index++)
                {
                    if (!(on_table_insert_columns(pnode->hwnd_qrtable, field_index, result[index])))
                    {
                        MSG_BOX(IDC_MSG_QUERY_STR4, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                        eu_sqlite3_free_table(result);
                        SQL_EXECUTE_FAIL(1);
                    }
                    nfield_width = ((int) strlen(result[index]) + 2) * char_width;
                    if (afield_width[field_index] < nfield_width)
                    {
                        afield_width[field_index] = nfield_width;
                    }
                }
                for (row_index = 0, index = field_count; row_index < row_conut; row_index++)
                {
                    for (field_index = 0; field_index < field_count; field_index++, index++)
                    {
                        if (!(on_table_insert_item(pnode->hwnd_qrtable, row_index, field_index, result[index] ? result[index] : "")))
                        {
                            MSG_BOX(IDC_MSG_QUERY_STR5, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                            eu_sqlite3_free_table(result);
                            SQL_EXECUTE_FAIL(1);
                        }
                        if (result[index])
                        {
                            nfield_width = ((int) strlen(result[index]) + 2) * char_width;
                            if (afield_width[field_index] < nfield_width)
                            {
                                afield_width[field_index] = nfield_width;
                            }
                        }
                    }
                }
                eu_sqlite3_free_table(result);
            }
        }
        else if (_stricmp(pnode->db_ptr->config.dbtype, "PostgreSQL") == 0)
        {
            pq_lib *pq_sub = &(db_funcs.m_pq);
            pq_handle *this_pq = &(pnode->db_ptr->handles.h_pq);
            PGresult *res = NULL;
            int row_conut;
            res = pq_sub->fnPQexec(this_pq->postgres, psel);
            if (pq_sub->fnPQresultStatus(res) == PGRES_COMMAND_OK)
            {
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_STR3, msg_str);
                on_result_append_text(msg_str, pq_sub->fnPQntuples(res));
            }
            else if (pq_sub->fnPQresultStatus(res) == PGRES_TUPLES_OK)
            {
                TCHAR *ptr_index = NULL;
                row_conut = pq_sub->fnPQntuples(res);
                field_count = pq_sub->fnPQnfields(res);
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_STR14, msg_str);
                on_result_append_text(msg_str, row_conut);
                if (!field_count)
                {   // 空表
                    pq_sub->fnPQclear(res);
                    SQL_EXECUTE_FAIL(1);
                }
                if ((afield_width = (unsigned int *) calloc(1, sizeof(unsigned int) * field_count)) == NULL)
                {
                    MSG_BOX(IDC_MSG_QUERY_STR11, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                    pq_sub->fnPQclear(res);
                    SQL_EXECUTE_FAIL(1);
                }
                on_table_reset_table(pnode->hwnd_qrtable);
                for (field_index = 0; field_index < field_count; field_index++)
                {
                    if (!on_table_insert_columns(pnode->hwnd_qrtable, field_index, pq_sub->fnPQfname(res, field_index)))
                    {
                        MSG_BOX(IDC_MSG_QUERY_STR4, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                        pq_sub->fnPQclear(res);
                        SQL_EXECUTE_FAIL(1);
                    }
                    nfield_width = ((int) strlen(pq_sub->fnPQfname(res, field_index)) + 2) * char_width;
                    if (afield_width[field_index] < nfield_width)
                    {
                        afield_width[field_index] = nfield_width;
                    }
                }
                for (row_index = 0; row_index < row_conut; row_index++)
                {
                    for (field_index = 0; field_index < field_count; field_index++)
                    {
                        field_value = pq_sub->fnPQgetvalue(res, row_index, field_index);
                        if (!(on_table_insert_item(pnode->hwnd_qrtable, row_index, field_index, field_value)))
                        {
                            MSG_BOX(IDC_MSG_QUERY_STR5, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                            pq_sub->fnPQclear(res);
                            SQL_EXECUTE_FAIL(1);
                        }
                        if (field_value)
                        {
                            nfield_width = ((int) strlen(field_value) + 2) * char_width;
                            if (afield_width[field_index] < nfield_width)
                            {
                                afield_width[field_index] = nfield_width;
                            }
                        }
                    }
                }
                pq_sub->fnPQclear(res);
            }
            else
            {
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_STR7, err_msg);
                on_result_append_text(err_msg, -1, util_make_u16(pq_sub->fnPQresultErrorMessage(res), utf_str, MAX_BUFFER));
                SQL_EXECUTE_FAIL(1);
            }
        }
        else
        {
            LOAD_I18N_RESSTR(IDC_MSG_QUERY_ERR26, err_msg);
            on_result_append_text(err_msg, util_make_u16(pnode->db_ptr->config.dbtype, utf_str, MAX_BUFFER));
            SQL_EXECUTE_FAIL(1);
        }
        psel = strtok(NULL, ";");
    }
table_clean:
    on_table_fix_columns(pnode->hwnd_qrtable, afield_width);
    eu_sci_call(pnode->presult, SCI_SETREADONLY, 1, 0);
    eu_sci_call(pnode->presult, SCI_GOTOLINE, 1, 0);
    eu_safe_free(sel_sql);
    eu_safe_free(afield_width);
    return nret;
}
