/******************************************************************************
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

db_libs db_funcs = {0};
redis_lib redis_funcs = {0};
LONG_PTR orgi_hearder_proc = 0;

int
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
        return 0;
    }
    return 1;
}

static LRESULT CALLBACK
listview_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR uIdSubClass, DWORD_PTR dwRefData)
{
    switch (msg)
    {
        case WM_LBUTTONUP:
        {
            eu_reset_drag_line();
            break;
        }
        case WM_THEMECHANGED:
        {
            printf("qrtable recv WM_THEMECHANGED\n");
            const intptr_t style = GetWindowLongPtr(hwnd, GWL_STYLE);
            if (on_dark_enable())
            {
                SetWindowLongPtr(hwnd, GWL_STYLE, style | LVS_NOCOLUMNHEADER);
            }
            else
            {
                SetWindowLongPtr(hwnd, GWL_STYLE, style & ~LVS_NOCOLUMNHEADER);
            }
            UpdateWindow(hwnd);
            break;
        }
        case WM_DPICHANGED:
        {
            SendMessage(hwnd, WM_SETFONT, (WPARAM) on_theme_font_hwnd(), 0);
            break;
        }
        case WM_DESTROY:
        {
            RemoveWindowSubclass(hwnd, listview_proc, uIdSubClass);
            printf("qrtable listview WM_DESTROY\n");
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
    const uint32_t style = WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | LVS_REPORT;
    pnode->hwnd_qrtable = CreateWindow(WC_LISTVIEW, _T(""), style, 0, 0, 0, 0, eu_module_hwnd(), (HMENU)IDM_TABLE_BAR, eu_module_handle(), NULL);
    if (pnode->hwnd_qrtable == NULL)
    {
        MSG_BOX(IDC_MSG_QUERY_ERR1, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
        return 1;
    }
    SetWindowSubclass(pnode->hwnd_qrtable, listview_proc, 0, 0);
    ListView_SetExtendedListViewStyle(pnode->hwnd_qrtable, LVS_EX_FLATSB | LVS_EX_DOUBLEBUFFER | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT);
    if (on_dark_supports())
    {
        on_dark_set_theme(pnode->hwnd_qrtable, L"Explorer", NULL);
    }
    return on_table_update_theme(pnode);
}

int
on_table_oci_error(eu_tabpage *pnode, OCIError *errhpp, int *err_code, char *err_desc, size_t err_size)
{
    sb4 err;
    sword result;
    EU_VERIFY(pnode != NULL);
    oci_lib *oci_sub = &(db_funcs.m_oci);
    err = 0;
    err_code[0] = '\0';
    result = oci_sub->fnOCIErrorGet((dvoid *) errhpp, (ub4) 1, (text *) NULL, &err, (ub1 *) err_desc, (ub4) err_size, OCI_HTYPE_ERROR);
    if (result != OCI_SUCCESS)
    {
        LOAD_I18N_RESSTR(IDC_MSG_QUERY_ERR2, err_msg);
        on_result_append_text(pnode->hwnd_qredit, err_msg);
        return -1;
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
        ret = ListView_InsertColumn(hwnd, index, &lvc);
        free(ptr_name);
    }
    return (ret >= 0);
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

int
on_table_disconnect_database(eu_tabpage *pnode)
{
    EU_VERIFY(pnode != NULL);
    if (_stricmp(pnode->db_config.dbtype, "MySQL") == 0)
    {
        mysql_lib *mysql_sub = &(db_funcs.m_mysql);
        mysql_handle *this_mysql = &(pnode->udb.handles.h_mysql);
        if (this_mysql->mysql)
        {
            mysql_sub->fn_mysql_close(this_mysql->mysql);
            this_mysql->mysql = NULL;
        }
        safe_close_dll(mysql_sub->mysql_dll);
    }
    else if (_stricmp(pnode->db_config.dbtype, "Oracle") == 0)
    {
        oci_lib *oci_sub = &(db_funcs.m_oci);
        oci_handle *this_oci = &(pnode->udb.handles.h_oci);
        if (this_oci->usrhpp)
        {
            oci_sub->fnOCIHandleFree((dvoid *) (this_oci->usrhpp), OCI_HTYPE_SESSION);
            this_oci->usrhpp = NULL;
        }
        if (this_oci->servhpp)
        {
            oci_sub->fnOCIHandleFree((dvoid *) (this_oci->servhpp), OCI_HTYPE_SERVER);
            this_oci->servhpp = NULL;
        }
        safe_close_dll(oci_sub->oci_dll);
    }
    else if (_stricmp(pnode->db_config.dbtype, "Sqlite3") == 0)
    {
        sql3_handle *this_sql3 = &(pnode->udb.handles.h_sql3);
        if (this_sql3->sqlite3)
        {
            eu_sqlite3_close(this_sql3->sqlite3);
            this_sql3->sqlite3 = NULL;
        }
    }
    else if (_stricmp(pnode->db_config.dbtype, "PostgreSQL") == 0)
    {
        pq_lib *pq_sub = &(db_funcs.m_pq);
        pq_handle *this_pq = &(pnode->udb.handles.h_pq);
        if (this_pq->postgres)
        {
            pq_sub->fnPQfinish(this_pq->postgres);
            this_pq->postgres = NULL;
        }
        safe_close_dll(pq_sub->libpq_dll);
    }
    else if (pnode->hwnd_qredit)
    {
        TCHAR utf_str[16+1] = {0};
        LOAD_I18N_RESSTR(IDC_MSG_QUERY_ERR3, err_msg);
        on_result_append_text(pnode->hwnd_qredit, err_msg, util_make_u16(pnode->db_config.dbtype, utf_str, 16));
    }
    if (pnode->db_is_connect)
    {
        pnode->db_is_connect = false;
    }
    return 0;
}

int
on_table_connect_database(eu_tabpage *pnode)
{
    char ip[ACNAME_LEN+1] = {0};
    TCHAR utf_str[ACNAME_LEN+1] = {0};
    TCHAR user[ACNAME_LEN+1] = {0};
    TCHAR name[ACNAME_LEN+1] = {0};
    TCHAR dll_path[MAX_PATH+1] = {0};
    EU_VERIFY(pnode != NULL);
    if (pnode->db_is_connect)
    {
        return 0;
    }
    if (!pnode->db_config.dbtype[0])
    {
        return -1;
    }
    if (util_query_hostname(pnode->db_config.dbhost, ip, ACNAME_LEN))
    {
        printf("util_query_hostname error\n");
        return -1;
    }
    if (_stricmp(pnode->db_config.dbtype, "MySQL") == 0)
    {
        mysql_lib *mysql_sub = &(db_funcs.m_mysql);
        mysql_handle *this_mysql = &(pnode->udb.handles.h_mysql);

        if (mysql_sub->mysql_dll == NULL)
        {
            _sntprintf(dll_path, MAX_PATH, _T("%s\\%s"), eu_module_path, _T("libmysql.dll"));
            mysql_sub->mysql_dll = LoadLibrary(dll_path);
            if (mysql_sub->mysql_dll == NULL)
            {
                MSG_BOX(IDC_MSG_QUERY_ERR4, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                return -1;
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
                safe_close_dll(mysql_sub->mysql_dll);
                return -1;
            }
        }
        if (this_mysql->mysql == NULL)
        {
            this_mysql->mysql = mysql_sub->fn_mysql_init(NULL);
            if (this_mysql->mysql == NULL)
            {
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_ERR6, err_msg);
                on_result_append_text(pnode->hwnd_qredit, err_msg);
                return -1;
            }
            pnode->db_is_connect = false;
        }
        if (!pnode->db_is_connect)
        {
            if (pnode->db_config.dbpass[0] == '\0')
            {
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_ERR7, m_pass);
                TCHAR dbpass[64] = {0};
                eu_input(m_pass, dbpass, _countof(dbpass));
                if (dbpass[0] == '\0')
                {
                    LOAD_I18N_RESSTR(IDC_MSG_QUERY_ERR8, err_msg);
                    on_result_append_text(pnode->hwnd_qredit, err_msg);
                    mysql_sub->fn_mysql_close(this_mysql->mysql);
                    this_mysql->mysql = NULL;
                    return -1;
                }
                else
                {
                    WideCharToMultiByte(CP_UTF8, 0, dbpass, -1, pnode->db_config.dbpass, 64, NULL, NULL);
                }
            }
            this_mysql->mysql = mysql_sub->fn_mysql_real_connect(this_mysql->mysql,
                                                                 ip,
                                                                 pnode->db_config.dbuser,
                                                                 pnode->db_config.dbpass,
                                                                 pnode->db_config.dbname,
                                                                 pnode->db_config.dbport,
                                                                 NULL,
                                                                 0);
            if (this_mysql->mysql == NULL)
            {
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_ERR9, err_msg);
                on_result_append_text(pnode->hwnd_qredit,
                                 err_msg,
                                 util_make_u16(ip, utf_str, ACNAME_LEN),
                                 pnode->db_config.dbport,
                                 util_make_u16(pnode->db_config.dbuser, user, ACNAME_LEN),
                                 util_make_u16(pnode->db_config.dbname, name, ACNAME_LEN));
                mysql_sub->fn_mysql_close(this_mysql->mysql);
                this_mysql->mysql = NULL;
                if (!pnode->db_config.config_pass)
                {
                    pnode->db_config.dbpass[0] = '\0';
                }
                return -1;
            }
            else
            {
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_ERR10, err_msg);
                on_result_append_text(pnode->hwnd_qredit,
                                 err_msg,
                                 util_make_u16(ip, utf_str, ACNAME_LEN),
                                 pnode->db_config.dbport,
                                 util_make_u16(pnode->db_config.dbuser, user, ACNAME_LEN),
                                 util_make_u16(pnode->db_config.dbname, name, ACNAME_LEN));
            }
            if (mysql_sub->fn_mysql_set_character_set(this_mysql->mysql, "utf8mb4"))
            {
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_ERR11, err_msg);
                on_result_append_text(pnode->hwnd_qredit, err_msg);
                mysql_sub->fn_mysql_close(this_mysql->mysql);
                this_mysql->mysql = NULL;
                return -1;
            }
            pnode->db_is_connect = true;
        }
    }
    else if (_stricmp(pnode->db_config.dbtype, "Oracle") == 0)
    {
        oci_lib *oci_sub = &(db_funcs.m_oci);
        oci_handle *this_oci = &(pnode->udb.handles.h_oci);
        sword result;

        if (oci_sub->oci_dll == NULL)
        {
            _sntprintf(dll_path, MAX_PATH, _T("%s\\%s"), eu_module_path, _T("oci.dll"));
            oci_sub->oci_dll = LoadLibrary(dll_path);
            if (oci_sub->oci_dll == NULL)
            {
                MSG_BOX(IDC_MSG_QUERY_ERR12, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                return -1;
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
                safe_close_dll(db_funcs.m_mysql.mysql_dll);
                return -1;
            }
        }
        if (this_oci->envhpp == NULL)
        {
            result = oci_sub->fnOCIEnvCreate(&(this_oci->envhpp), OCI_DEFAULT, NULL, NULL, NULL, NULL, 0, NULL);
            if (result != OCI_SUCCESS && result != OCI_SUCCESS_WITH_INFO)
            {
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_ERR14, err_msg);
                on_result_append_text(pnode->hwnd_qredit, err_msg);
                return -1;
            }
            oci_sub->fnOCIHandleAlloc((dvoid *) (this_oci->envhpp), (dvoid **) &(this_oci->errhpp), OCI_HTYPE_ERROR, (size_t) 0, (dvoid **) 0);
            pnode->db_is_connect = false;
        }
        if (!pnode->db_is_connect)
        {
            if (pnode->db_config.dbpass[0] == '\0')
            {
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_ERR7, m_pass);
                TCHAR dbpass[64] = {0};
                eu_input(m_pass, dbpass, _countof(dbpass));
                if (pnode->db_config.dbpass[0] == '\0')
                {
                    LOAD_I18N_RESSTR(IDC_MSG_QUERY_ERR8, err_msg);
                    on_result_append_text(pnode->hwnd_qredit, err_msg);
                    return -1;
                }
                else
                {
                    WideCharToMultiByte(CP_UTF8, 0, dbpass, -1, pnode->db_config.dbpass, 64, NULL, NULL);
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
                on_result_append_text(pnode->hwnd_qredit,
                                 err_msg,
                                 util_make_u16(ip, utf_str, ACNAME_LEN),
                                 err_code,
                                 util_make_u16(err_desc, user, ACNAME_LEN));
                oci_sub->fnOCIHandleFree((dvoid *) (this_oci->servhpp), OCI_HTYPE_SERVER);
                this_oci->servhpp = NULL;
                if (!pnode->db_config.config_pass)
                {
                    pnode->db_config.dbpass[0] = '\0';
                }
                return -1;
            }
            else
            {
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_ERR16, err_msg);
                on_result_append_text(pnode->hwnd_qredit, err_msg, util_make_u16(ip, utf_str, ACNAME_LEN));
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
                                  (dvoid *) (pnode->db_config.dbuser),
                                  (ub4) strlen(pnode->db_config.dbuser),
                                  (ub4) OCI_ATTR_USERNAME,
                                  this_oci->errhpp);
            oci_sub->fnOCIAttrSet((dvoid *) (this_oci->usrhpp),
                                  (ub4) OCI_HTYPE_SESSION,
                                  (dvoid *) (pnode->db_config.dbpass),
                                  (ub4) strlen(pnode->db_config.dbpass),
                                  (ub4) OCI_ATTR_PASSWORD,
                                  this_oci->errhpp);

            result = oci_sub->fnOCISessionBegin(this_oci->svchpp, this_oci->errhpp, this_oci->usrhpp, OCI_CRED_RDBMS, (ub4) OCI_DEFAULT);
            if (result != OCI_SUCCESS)
            {
                int err_code;
                char err_desc[512] = { 0 };
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_ERR17, err_msg);
                on_table_oci_error(pnode, this_oci->errhpp, &err_code, err_desc, _countof(err_desc) - 1);
                on_result_append_text(pnode->hwnd_qredit, err_msg, err_code, err_desc, pnode->db_config.dbuser);
                oci_sub->fnOCIHandleFree((dvoid *) (this_oci->usrhpp), OCI_HTYPE_SESSION);
                this_oci->usrhpp = NULL;
                oci_sub->fnOCIHandleFree((dvoid *) (this_oci->servhpp), OCI_HTYPE_SERVER);
                this_oci->servhpp = NULL;
                if (!pnode->db_config.config_pass)
                {
                    pnode->db_config.dbpass[0] = '\0';
                }
                return -1;
            }
            else
            {
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_ERR18, err_msg);
                on_result_append_text(pnode->hwnd_qredit, err_msg, util_make_u16(pnode->db_config.dbuser, user, ACNAME_LEN));
            }
            oci_sub->fnOCIAttrSet((dvoid *) (this_oci->svchpp),
                                  (ub4) OCI_HTYPE_SVCCTX,
                                  (dvoid *) (this_oci->usrhpp),
                                  (ub4) 0,
                                  (ub4) OCI_ATTR_SESSION,
                                  this_oci->errhpp);
            pnode->db_is_connect = true;
        }
    }
    else if (_stricmp(pnode->db_config.dbtype, "Sqlite3") == 0)
    {
        sql3_handle *this_sql3 = &(pnode->udb.handles.h_sql3);
        if (!pnode->db_is_connect)
        {
            if (eu_sqlite3_open(pnode->db_config.dbname, &(this_sql3->sqlite3)) != SQLITE_OK)
            {
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_ERR19, err_msg);
                on_result_append_text(pnode->hwnd_qredit, err_msg);
                return -1;
            }
            LOAD_I18N_RESSTR(IDC_MSG_QUERY_ERR20, err_msg);
            on_result_append_text(pnode->hwnd_qredit, err_msg, util_make_u16(pnode->db_config.dbname, name, ACNAME_LEN));
            pnode->db_is_connect = true;
        }
    }
    else if (_stricmp(pnode->db_config.dbtype, "PostgreSQL") == 0)
    {
        pq_lib *pq_sub = &(db_funcs.m_pq);
        pq_handle *this_pq = &(pnode->udb.handles.h_pq);

        if (pq_sub->libpq_dll == NULL)
        {
            _sntprintf(dll_path, MAX_PATH, _T("%s\\%s"), eu_module_path, _T("libpq.dll"));
            pq_sub->libpq_dll = LoadLibrary(dll_path);
            if (pq_sub->libpq_dll == NULL)
            {
                MSG_BOX(IDC_MSG_QUERY_ERR21, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                return -1;
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
                safe_close_dll(pq_sub->libpq_dll);
                return -1;
            }
        }
        if (this_pq->postgres == NULL || pnode->db_is_connect == FALSE)
        {
            char src_port[20 + 1];
            memset(src_port, 0, sizeof(src_port));
            snprintf(src_port, sizeof(src_port) - 1, "%d", pnode->db_config.dbport);
            this_pq->postgres = pq_sub->fnPQsetdbLogin(ip,
                                                       src_port,
                                                       NULL,
                                                       NULL,
                                                       pnode->db_config.dbname,
                                                       pnode->db_config.dbuser,
                                                       pnode->db_config.dbpass);
            if (this_pq->postgres == NULL)
            {
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_ERR23, err_msg);
                on_result_append_text(pnode->hwnd_qredit, err_msg);
                return -1;
            }
            if (pq_sub->fnPQsetClientEncoding(this_pq->postgres, eu_query_encoding_name(pnode->codepage)))
            {
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_ERR24, err_msg);
                on_result_append_text(pnode->hwnd_qredit, err_msg);
                pq_sub->fnPQfinish(this_pq->postgres);
                this_pq->postgres = NULL;
                return -1;
            }
            LOAD_I18N_RESSTR(IDC_MSG_QUERY_ERR25, err_msg);
            on_result_append_text(pnode->hwnd_qredit,
                             err_msg,
                             util_make_u16(ip, utf_str, ACNAME_LEN),
                             atoi(src_port),
                             util_make_u16(pnode->db_config.dbname, user, ACNAME_LEN),
                             util_make_u16(pnode->db_config.dbuser, name, ACNAME_LEN));
            pnode->db_is_connect = true;
        }
    }
    else
    {
        LOAD_I18N_RESSTR(IDC_MSG_QUERY_ERR26, err_msg);
        on_result_append_text(pnode->hwnd_qredit, err_msg, util_make_u16(pnode->db_config.dbtype, utf_str, ACNAME_LEN));
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
    db_conn db_config = {0};
    EU_VERIFY(pnode != NULL);
    file_line_count = eu_sci_call(pnode, SCI_GETLINECOUNT, 0, 0);
    for (file_line = 0; file_line <= file_line_count; file_line++)
    {
        char line_buf[FILESIZE+1] = {0};
        char config_remark[ACNAME_LEN] = {0};
        char config_key[ACNAME_LEN] = {0};
        char config_eval[ACNAME_LEN] = {0};
        char config_value[ACNAME_LEN] = {0};
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
                if (db_config.dbtype[0])
                {
                    if ((pnode->db_config.dbtype[0] && strcmp(pnode->db_config.dbtype, db_config.dbtype)) ||
                        (pnode->db_config.dbhost[0] && strcmp(pnode->db_config.dbhost, db_config.dbhost)) ||
                        (pnode->db_config.dbport && pnode->db_config.dbport != db_config.dbport) ||
                        (pnode->db_config.dbuser[0] && strcmp(pnode->db_config.dbuser, db_config.dbuser)) ||
                        (pnode->db_config.dbpass[0] && strcmp(pnode->db_config.dbpass, db_config.dbpass)) ||
                        (pnode->db_config.dbname[0] && strcmp(pnode->db_config.dbname, db_config.dbname)))
                    {
                        m_change = true;
                    }
                    if (db_config.dbpass[0])
                    {
                        db_config.config_pass = true;
                    }
                    else
                    {
                        db_config.config_pass = false;
                    }
                    memcpy(&(pnode->db_config), &db_config, sizeof(db_conn));
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
                strncpy(db_config.dbtype, config_value, sizeof(db_config.dbtype) - 1);
            }
            else if (strcmp(config_key, "DBHOST") == 0)
            {
                strncpy(db_config.dbhost, config_value, sizeof(db_config.dbhost) - 1);
            }
            else if (strcmp(config_key, "DBPORT") == 0)
            {
                db_config.dbport = atoi(config_value);
            }
            else if (strcmp(config_key, "DBUSER") == 0)
            {
                strncpy(db_config.dbuser, config_value, sizeof(db_config.dbuser) - 1);
            }
            else if (strcmp(config_key, "DBPASS") == 0)
            {
                strncpy(db_config.dbpass, config_value, sizeof(db_config.dbpass) - 1);
            }
            else if (strcmp(config_key, "DBNAME") == 0)
            {
                strncpy(db_config.dbname, config_value, sizeof(db_config.dbname) - 1);
            }
        }
    }
    if (m_change && pnode->db_is_connect)
    {
        on_table_disconnect_database(pnode);
    }
    return (on_table_connect_database(pnode) == 0);
}

static int
strnspace(const char *s1, const char *s2)
{
    if (!(s1 && s2))
    {
        return -1;
    }
    if (s1 == s2)
    {
        return 0;
    }
    if (strncmp(s1, s2, strlen(s2)) == 0)
    {
        return 0;
    }
    for (int i = 0; i < (int)strlen(s1); ++i)
    {
        if (isspace(s1[i]))
        {
            continue;
        }
        if (strncmp(&s1[i], s2, strlen(s2)) == 0)
        {
            return 0;
        }
    }
    return 1;
}

static bool
skip_sql_comment(eu_tabpage *pnode, const char *sql, char *out)
{
    bool ret = false;
    const char *s = NULL;
    int s_len = eu_int_cast(strlen(sql));
    const int m_eol = on_encoding_eol_char(pnode);
    if (!(sql && *sql))
    {
        return false;
    }
    if (strncmp(sql, "--", strlen("--")))
    {
        return (sscanf(sql, "%s", out) == 1);
    }
    for (s = sql; *s; ++s)
    {
        if (*s == m_eol && s - sql < s_len && strnspace(&s[1], "--"))
        {
            if (s[1] != m_eol)
            {
                ret = (sscanf(&s[1], "%s", out) == 1);
            }
            break;
        }
    }
    return ret;
}

int
on_table_sql_query(eu_tabpage *pnode, const char *pq)
{
    int sel_start;
    int sel_end;
    int sel_len;
    char *sel_sql = NULL;
    char *psel = NULL;
    HWND hwnd_view_header;
    int view_count;
    int field_count;
    int field_index;
    char *field_value = NULL;
    int row_index;
    unsigned int *afield_width = NULL;
    unsigned int nfield_width;
    TCHAR utf_str[MAX_BUFFER+1] = {0};
    int nret = 0;
    EU_VERIFY(pnode != NULL);
    int char_width = (int)eu_sci_call(pnode, SCI_TEXTWIDTH, STYLE_DEFAULT, (sptr_t) "X");
    if (!on_table_sql_header(pnode))
    {
        printf("on_table_sql_header failed\n");
        return -1;
    }
    if (on_symtree_do_sql(pnode, false))
    {
        printf("on_symtree_do_sql failed\n");
        return -1;
    }
    if (!pq)
    {
        sel_start = (int) eu_sci_call(pnode, SCI_GETSELECTIONSTART, 0, 0);
        sel_end = (int) eu_sci_call(pnode, SCI_GETSELECTIONEND, 0, 0);
        sel_len = sel_end - sel_start;
        if (sel_len <= 0)
        {
            sel_sql = util_strdup_line(pnode, NULL);
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
        printf("failed to allocate memory\n");
        return -1;
    }
    psel = strtok(sel_sql, ";");
    while (psel)
    {
        char first_word[256] = {0};
        while ((*psel))
        {
            if ((*psel) == on_encoding_eol_char(pnode))
            {
                psel++;
            }
            else
            {
                break;
            }
        }
        if (psel[0] == '\0')
        {
            break;
        }
        skip_sql_comment(pnode, psel, first_word);
        printf("psel = %s\nfirst_word = %s\n", psel, first_word);
        if (_stricmp(pnode->db_config.dbtype, "MySQL") == 0)
        {
            mysql_lib *mysql_sub = &(db_funcs.m_mysql);
            mysql_handle *this_mysql = &(pnode->udb.handles.h_mysql);
            MYSQL_RES *presult = NULL;
            if (strncmp(psel, "--", strlen("--")))
            {
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_STR1, msg_str);
                on_result_append_text(pnode->hwnd_qredit, msg_str, util_make_u16(psel, utf_str, MAX_BUFFER));
            }
            nret = mysql_sub->fn_mysql_query(this_mysql->mysql, psel);
            if (nret)
            {
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_STR2, msg_str);
                on_result_append_text(pnode->hwnd_qredit, msg_str, nret);
                free(sel_sql);
                return -1;
            }
            else
            {
                if (_strnicmp(psel, "select", strlen("select")))
                {
                    LOAD_I18N_RESSTR(IDC_MSG_QUERY_STR3, msg_str);
                    on_result_append_text(pnode->hwnd_qredit,
                                     msg_str,
                                     (int)(mysql_sub->fn_mysql_affected_rows(this_mysql->mysql)));
                }
            }
            if ((presult = mysql_sub->fn_mysql_store_result(this_mysql->mysql)))
            {
                MYSQL_FIELD *mysql_field = NULL;
                MYSQL_ROW mysqlrow;
                ListView_DeleteAllItems(pnode->hwnd_qrtable);
                hwnd_view_header = ListView_GetHeader(pnode->hwnd_qrtable);
                view_count = (int) SendMessage(hwnd_view_header, HDM_GETITEMCOUNT, 0, 0);
                for (view_count--; view_count >= 0; view_count--)
                {
                    ListView_DeleteColumn(pnode->hwnd_qrtable, view_count);
                }
                field_count = mysql_sub->fn_mysql_num_fields(presult);
                afield_width = (unsigned int *) calloc(1, sizeof(unsigned int) * field_count);
                if (afield_width == NULL)
                {
                    printf("Failed to allocate memory\n");
                    mysql_sub->fn_mysql_free_result(presult);
                    free(sel_sql);
                    return -1;
                }
                for (field_index = 0; field_index < field_count; field_index++)
                {
                    mysql_field = mysql_sub->fn_mysql_fetch_field(presult);
                    if (!on_table_insert_columns(pnode->hwnd_qrtable, field_index, mysql_field->name))
                    {
                        MSG_BOX(IDC_MSG_QUERY_STR4, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                        mysql_sub->fn_mysql_free_result(presult);
                        free(afield_width);
                        free(sel_sql);
                        return -1;
                    }
                    nfield_width = ((int) strlen(mysql_field->name) + 2) * char_width;
                    if (afield_width[field_index] < nfield_width)
                    {
                        afield_width[field_index] = nfield_width;
                    }
                }
                row_index = 0;
                while ((mysqlrow = mysql_sub->fn_mysql_fetch_row(presult)))
                {
                    for (field_index = 0; field_index < field_count; field_index++)
                    {
                        if (!(on_table_insert_item(pnode->hwnd_qrtable, row_index, field_index, mysqlrow[field_index])))
                        {
                            MSG_BOX(IDC_MSG_QUERY_STR5, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                            mysql_sub->fn_mysql_free_result(presult);
                            free(afield_width);
                            free(sel_sql);
                            return -1;
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
                for (field_index = 0; field_index < field_count; field_index++)
                {
                    ListView_SetColumnWidth(pnode->hwnd_qrtable, field_index, afield_width[field_index]);
                }
                mysql_sub->fn_mysql_free_result(presult);
                free(afield_width);
            }
        }
        else if (_stricmp(pnode->db_config.dbtype, "Oracle") == 0)
        {
            oci_lib *oci_sub = &(db_funcs.m_oci);
            oci_handle *this_oci = &(pnode->udb.handles.h_oci);
            sword result;

            OCIStmt *stmthpp = NULL;
            ub2 stmt_type;
            OCITrans *txnhpp = NULL;
            OCIParam *paramhpp = NULL;
            text *column_name = NULL;
            ub4 column_name_len = 0;
            OCIDefine *column_ocid = NULL;
            struct ColumnData
            {
                char data[4000 + 1];
                ub2 data_len;
                sb2 data_indicator;
            } *column_data;
            if (_strnicmp(psel, "-- ", strlen("-- ")))
            {
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_STR1, msg_str);
                on_result_append_text(pnode->hwnd_qredit, msg_str, util_make_u16(psel, utf_str, MAX_BUFFER));
            }
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
                on_result_append_text(pnode->hwnd_qredit, msg_str, err_code, util_make_u16(err_desc, utf_str, MAX_BUFFER));
                free(sel_sql);
                return -1;
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
                on_result_append_text(pnode->hwnd_qredit, msg_str, err_code, util_make_u16(err_desc, utf_str, MAX_BUFFER));
                if (stmt_type != OCI_STMT_SELECT)
                {
                    LOAD_I18N_RESSTR(IDC_MSG_QUERY_STR8, msg_str);
                    oci_sub->fnOCITransRollback(this_oci->svchpp, this_oci->errhpp, OCI_DEFAULT);
                    on_result_append_text(pnode->hwnd_qredit, msg_str);
                }
                free(sel_sql);
                return -1;
            }
            else
            {
                if (stmt_type != OCI_STMT_SELECT)
                {
                    LOAD_I18N_RESSTR(IDC_MSG_QUERY_STR9, msg_str);
                    oci_sub->fnOCITransCommit(this_oci->svchpp, this_oci->errhpp, OCI_DEFAULT);
                    on_result_append_text(pnode->hwnd_qredit, msg_str);
                    oci_sub->fnOCIHandleFree((void *) txnhpp, OCI_HTYPE_TRANS);
                }
            }
            if (stmt_type == OCI_STMT_SELECT)
            {
                ListView_DeleteAllItems(pnode->hwnd_qrtable);
                hwnd_view_header = ListView_GetHeader(pnode->hwnd_qrtable);
                view_count = (int) SendMessage(hwnd_view_header, HDM_GETITEMCOUNT, 0, 0);
                for (view_count--; view_count >= 0; view_count--)
                {
                    ListView_DeleteColumn(pnode->hwnd_qrtable, view_count);
                }
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_STR10, msg_str);
                on_result_append_text(pnode->hwnd_qredit, msg_str);

                ub4 nFieldCount2;
                oci_sub->fnOCIAttrGet((dvoid *) stmthpp, OCI_HTYPE_STMT, &nFieldCount2, 0, OCI_ATTR_PARAM_COUNT, this_oci->errhpp);
                field_count = (int) nFieldCount2;

                afield_width = (unsigned int *) calloc(1, sizeof(unsigned int) * field_count);
                if (afield_width == NULL)
                {
                    MSG_BOX(IDC_MSG_QUERY_STR11, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                    oci_sub->fnOCIHandleFree((dvoid *) stmthpp, OCI_HTYPE_STMT);
                    free(sel_sql);
                    return -1;
                }
                column_data = (struct ColumnData *) calloc(1, sizeof(struct ColumnData) * field_count);
                if (column_data == NULL)
                {
                    MSG_BOX(IDC_MSG_QUERY_STR12, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                    oci_sub->fnOCIHandleFree((dvoid *) stmthpp, OCI_HTYPE_STMT);
                    free(afield_width);
                    free(sel_sql);
                    return -1;
                }
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
                        on_result_append_text(pnode->hwnd_qredit, msg_str, field_index, err_code, util_make_u16(err_desc, utf_str, MAX_BUFFER));
                        oci_sub->fnOCIHandleFree((dvoid *) stmthpp, OCI_HTYPE_STMT);
                        free(afield_width);
                        free(column_data);
                        free(sel_sql);
                        return -1;
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
                        free(afield_width);
                        free(column_data);
                        free(sel_sql);
                        return -1;
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
                row_index = 0;
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
                        free(afield_width);
                        free(column_data);
                        free(sel_sql);
                        return -1;
                    }
                    for (field_index = 0; field_index < field_count; field_index++)
                    {
                        if (!(on_table_insert_item(pnode->hwnd_qrtable, row_index, field_index, column_data[field_index].data)))
                        {
                            MSG_BOX(IDC_MSG_QUERY_STR5, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                            oci_sub->fnOCIHandleFree((dvoid *) stmthpp, OCI_HTYPE_STMT);
                            free(afield_width);
                            free(column_data);
                            free(sel_sql);
                            return -1;
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
                for (field_index = 0; field_index < field_count; field_index++)
                {
                    ListView_SetColumnWidth(pnode->hwnd_qrtable, field_index, afield_width[field_index]);
                }
                free(afield_width);
                free(column_data);
            }
            else
            {
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_STR10, msg_str);
                on_result_append_text(pnode->hwnd_qredit, msg_str);
            }
            oci_sub->fnOCIHandleFree((dvoid *) stmthpp, OCI_HTYPE_STMT);
        }
        else if (_stricmp(pnode->db_config.dbtype, "Sqlite3") == 0)
        {
            sql3_handle *this_sql3 = &(pnode->udb.handles.h_sql3);
            char *errmsg = NULL;
            char **result = NULL;
            int row_conut;
            int index;
            if (_strnicmp(first_word, "SELECT", strlen("SELECT")) == 0)
            {
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_STR1, msg_str);
                on_result_append_text(pnode->hwnd_qredit, msg_str, util_make_u16(psel, utf_str, MAX_BUFFER));
                nret = eu_sqlite3_get_table(this_sql3->sqlite3, psel, &result, &row_conut, &field_count, &errmsg);
                if (nret)
                {
                    LOAD_I18N_RESSTR(IDC_MSG_QUERY_STR7, msg_str);
                    on_result_append_text(pnode->hwnd_qredit, msg_str, nret, util_make_u16(errmsg, utf_str, MAX_BUFFER));
                    if (errmsg)
                    {
                        free(errmsg);
                    }
                    eu_sqlite3_free_table(result);
                    free(sel_sql);
                    return -1;
                }
                else
                {
                    LOAD_I18N_RESSTR(IDC_MSG_QUERY_STR10, msg_str);
                    on_result_append_text(pnode->hwnd_qredit, msg_str);

                }
            }
            else
            {
                if (_strnicmp(psel, "-- ", strlen("-- ")))
                {
                    LOAD_I18N_RESSTR(IDC_MSG_QUERY_STR1, msg_str);
                    on_result_append_text(pnode->hwnd_qredit, msg_str, util_make_u16(psel, utf_str, MAX_BUFFER));
                }
                nret = eu_sqlite3_exec(this_sql3->sqlite3, psel, NULL, NULL, &errmsg);
                if (nret)
                {
                    printf("eu_sqlite3_exec failed\n");
                    LOAD_I18N_RESSTR(IDC_MSG_QUERY_STR7, msg_str);
                    on_result_append_text(pnode->hwnd_qredit, msg_str, nret, util_make_u16(errmsg, utf_str, MAX_BUFFER));
                    if (errmsg)
                    {
                        free(errmsg);
                    }
                    free(sel_sql);
                    return -1;
                }
                else
                {
                    LOAD_I18N_RESSTR(IDC_MSG_QUERY_STR10, msg_str);
                    on_result_append_text(pnode->hwnd_qredit, msg_str);
                }
            }
            if (_strnicmp(first_word, "SELECT", strlen("SELECT")) == 0)
            {
                TCHAR *ptr_index = NULL;
                ListView_DeleteAllItems(pnode->hwnd_qrtable);
                hwnd_view_header = ListView_GetHeader(pnode->hwnd_qrtable);
                view_count = (int) SendMessage(hwnd_view_header, HDM_GETITEMCOUNT, 0, 0);
                for (view_count--; view_count >= 0; view_count--)
                {
                    ListView_DeleteColumn(pnode->hwnd_qrtable, view_count);
                }
                afield_width = (unsigned int *) calloc(1, sizeof(unsigned int) * field_count);
                if (afield_width == NULL)
                {
                    MSG_BOX(IDC_MSG_QUERY_STR11, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                    eu_sqlite3_free_table(result);
                    free(sel_sql);
                    return -1;
                }
                index = 0;
                for (field_index = 0; field_index < field_count; field_index++, index++)
                {
                    if (!on_table_insert_columns(pnode->hwnd_qrtable, field_index, result[index]))
                    {
                        MSG_BOX(IDC_MSG_QUERY_STR4, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                        eu_sqlite3_free_table(result);
                        free(afield_width);
                        free(sel_sql);
                        return -1;
                    }
                    nfield_width = ((int) strlen(result[index]) + 2) * char_width;
                    if (afield_width[field_index] < nfield_width)
                    {
                        afield_width[field_index] = nfield_width;
                    }
                }
                index = field_count;
                for (row_index = 0; row_index < row_conut; row_index++)
                {
                    for (field_index = 0; field_index < field_count; field_index++, index++)
                    {
                        if (!(on_table_insert_item(pnode->hwnd_qrtable, row_index, field_index, result[index] ? result[index] : "")))
                        {
                            MSG_BOX(IDC_MSG_QUERY_STR5, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                            eu_sqlite3_free_table(result);
                            free(afield_width);
                            free(sel_sql);
                            return -1;
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
                for (field_index = 0; field_index < field_count; field_index++)
                {
                    ListView_SetColumnWidth(pnode->hwnd_qrtable, field_index, afield_width[field_index]);
                }
                free(afield_width);
                eu_sqlite3_free_table(result);
            }
        }
        else if (_stricmp(pnode->db_config.dbtype, "PostgreSQL") == 0)
        {
            pq_lib *pq_sub = &(db_funcs.m_pq);
            pq_handle *this_pq = &(pnode->udb.handles.h_pq);
            PGresult *res = NULL;
            int row_conut;
            if (_strnicmp(psel, "-- ", strlen("-- ")))
            {
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_STR1, msg_str);
                on_result_append_text(pnode->hwnd_qredit, msg_str, util_make_u16(psel, utf_str, MAX_BUFFER));
            }
            res = pq_sub->fnPQexec(this_pq->postgres, psel);
            if (pq_sub->fnPQresultStatus(res) == PGRES_COMMAND_OK)
            {
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_STR3, msg_str);
                on_result_append_text(pnode->hwnd_qredit, msg_str, pq_sub->fnPQntuples(res));
            }
            else if (pq_sub->fnPQresultStatus(res) == PGRES_TUPLES_OK)
            {
                TCHAR *ptr_index = NULL;
                row_conut = pq_sub->fnPQntuples(res);
                field_count = pq_sub->fnPQnfields(res);
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_STR14, msg_str);
                on_result_append_text(pnode->hwnd_qredit, msg_str, row_conut);
                ListView_DeleteAllItems(pnode->hwnd_qrtable);
                hwnd_view_header = ListView_GetHeader(pnode->hwnd_qrtable);
                view_count = (int) SendMessage(hwnd_view_header, HDM_GETITEMCOUNT, 0, 0);
                for (view_count--; view_count >= 0; view_count--)
                {
                    ListView_DeleteColumn(pnode->hwnd_qrtable, view_count);
                }
                afield_width = (unsigned int *) calloc(1, sizeof(unsigned int) * field_count);
                if (afield_width == NULL)
                {
                    MSG_BOX(IDC_MSG_QUERY_STR11, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                    pq_sub->fnPQclear(res);
                    free(sel_sql);
                    return -1;
                }
                for (field_index = 0; field_index < field_count; field_index++)
                {
                    if (!on_table_insert_columns(pnode->hwnd_qrtable, field_index, pq_sub->fnPQfname(res, field_index)))
                    {
                        MSG_BOX(IDC_MSG_QUERY_STR4, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                        pq_sub->fnPQclear(res);
                        free(afield_width);
                        free(sel_sql);
                        return -1;
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
                            free(afield_width);
                            free(sel_sql);
                            return -1;
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
                for (field_index = 0; field_index < field_count; field_index++)
                {
                    ListView_SetColumnWidth(pnode->hwnd_qrtable, field_index, afield_width[field_index]);
                }
                free(afield_width);
                pq_sub->fnPQclear(res);
            }
            else
            {
                LOAD_I18N_RESSTR(IDC_MSG_QUERY_STR7, err_msg);
                on_result_append_text(pnode->hwnd_qredit, err_msg, -1, util_make_u16(pq_sub->fnPQresultErrorMessage(res), utf_str, MAX_BUFFER));
                free(sel_sql);
                return -1;
            }
        }
        else
        {
            LOAD_I18N_RESSTR(IDC_MSG_QUERY_ERR26, err_msg);
            on_result_append_text(pnode->hwnd_qredit, err_msg, util_make_u16(pnode->db_config.dbtype, utf_str, MAX_BUFFER));
            free(sel_sql);
            return -1;
        }
        psel = strtok(NULL, ";");
    }
    free(sel_sql);
    return 0;
}
