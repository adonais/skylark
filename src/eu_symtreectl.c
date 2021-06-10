/******************************************************************************
 * This file is part of Skylark project
 * Copyright ©2021 Hua andy <hua.andy@gmail.com>

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

static WNDPROC symtree_wnd;

static char *
get_symtree_str(eu_tabpage *pnode, bool get_parent)
{
    HTREEITEM hti;
    TVITEM tvi = {0};
    TCHAR file_str[MAXLEN_FILENAME] = {0};
    char *cnv = NULL;
    hti = TreeView_GetSelection(pnode->hwnd_symtree);
    if (hti == NULL)
    {
        return NULL;
    }     
    if (get_parent)
    {
        if (!(hti = TreeView_GetParent(pnode->hwnd_symtree, hti)))
        {
            return NULL;
        }
    }  
    tvi.mask = TVIF_HANDLE | TVIF_TEXT;
    tvi.hItem = hti;
    tvi.cchTextMax = _countof(file_str) - 1;
    tvi.pszText = file_str;
    if (!TreeView_GetItem(pnode->hwnd_symtree, &tvi))
    {
        return NULL;
    }
    if ((cnv = eu_utf16_utf8(file_str, NULL)) == NULL)
    {
        return NULL;
    }
    if (strstr(cnv, " --"))
    {
        strstr(cnv, " --")[0] = 0;
    }
    return cnv;
}

int
on_symtree_add_text(eu_tabpage *pnode)
{
    char *cnv = NULL;
    if (!pnode)
    {
        return 1;
    }
    if (!(cnv = get_symtree_str(pnode, false)))
    {
        return 1;
    }
    eu_sci_call(pnode, SCI_ADDTEXT, strlen(cnv), (sptr_t) cnv);
    free(cnv);
    return 0;
}

int
on_symtree_do_sql(eu_tabpage *pnode, bool reload)
{
    char *words2 = NULL;
    int words2_buf_len;
    int words2_remain_len;
    int words2_len;
    char sql[MAXLEN_FILENAME];
    MYSQL_RES *m_result = NULL;
    MYSQL_RES *m_result2 = NULL;
    MYSQL_ROW row_str;
    MYSQL_ROW row_str2;
    TVITEM tvi;
    TVINSERTSTRUCT tvis;
    HTREEITEM hti_root;
    TCHAR utf_str[MAX_BUFFER+1] = {0};
    int err = 0;
    if (!pnode)
    {
        return -1;
    }
    if (reload && !on_table_sql_header(pnode))
    {
        return -1;
    }
    words2 = (char *) calloc(1, SQL_WD2_SIZE);
    if (!words2)
    {
        return -1;
    }
    words2_buf_len = 0;
    words2_remain_len = SQL_WD2_SIZE - 1;
    if (_stricmp(pnode->db_config.dbtype, "MySQL") == 0)
    {
        mysql_lib *mysql_sub = &(db_funcs.m_mysql);
        mysql_handle *this_mysql = &(pnode->udb.handles.h_mysql);
        snprintf(sql,
                 sizeof(sql) - 1,
                 "SELECT table_name FROM information_schema.TABLES WHERE table_schema='%s'",
                 pnode->db_config.dbname);
        err = mysql_sub->fn_mysql_query(this_mysql->mysql, sql);
        if (err)
        {
            MSG_BOX(IDC_MSG_SYMTREE_ERR2, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
            free(words2);
            return -1;
        }
        m_result = mysql_sub->fn_mysql_store_result(this_mysql->mysql);
        if (m_result)
        {
            TreeView_DeleteAllItems(pnode->hwnd_symtree);
            while ((row_str = mysql_sub->fn_mysql_fetch_row(m_result)))
            {
                memset(&tvi, 0, sizeof(TVITEM));
                memset(&tvis, 0, sizeof(TVINSERTSTRUCT));
                tvi.mask = TVIF_TEXT;
                tvi.pszText = util_make_u16(row_str[0], utf_str, MAX_BUFFER);
                tvis.hParent = TVI_ROOT;
                tvis.hInsertAfter = TVI_LAST;
                tvis.item = tvi;
                hti_root = TreeView_InsertItem(pnode->hwnd_symtree, &tvis);
                words2_len = snprintf(words2 + words2_buf_len, words2_remain_len, "%s ", row_str[0]);
                if (words2_len > 0)
                {
                    words2_buf_len += words2_len;
                    words2_remain_len -= words2_len;
                }
                snprintf(sql,
                         _countof(sql) - 1,
                         "SELECT column_name,data_type,character_maximum_length,"
                         "numeric_precision,numeric_scale FROM information_schema.COLUMNS WHERE table_schema='%s' and "
                         "table_name='%s' ORDER BY ordinal_position ASC",
                         pnode->db_config.dbname,
                         row_str[0]);
                err = mysql_sub->fn_mysql_query(this_mysql->mysql, sql);
                if (err)
                {
                    MSG_BOX(IDC_MSG_SYMTREE_ERR3, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                    free(words2);
                    return -1;
                }
                m_result2 = mysql_sub->fn_mysql_store_result(this_mysql->mysql);
                if (m_result2)
                {
                    while ((row_str2 = mysql_sub->fn_mysql_fetch_row(m_result2)))
                    {
                        char buf[MAX_BUFFER + 1] = {0};
                        memset(&tvi, 0, sizeof(TVITEM));
                        memset(&tvis, 0, sizeof(TVINSERTSTRUCT));
                        if (row_str2[2])
                        {
                            snprintf(buf, MAX_BUFFER, "%s --%s(%s)", row_str2[0], row_str2[1], row_str2[2]);
                        }
                        else if (row_str2[3] && row_str2[4])
                        {
                            snprintf(buf, MAX_BUFFER, "%s --%s(%s,%s)", row_str2[0], row_str2[1], row_str2[3], row_str2[4]);
                        }
                        else
                        {
                            snprintf(buf, MAX_BUFFER, "%s --%s", row_str2[0], row_str2[1]);
                        }
                        tvi.mask = TVIF_TEXT;
                        tvi.pszText = util_make_u16(buf, utf_str, MAX_BUFFER);
                        tvis.hParent = hti_root;
                        tvis.hInsertAfter = TVI_LAST;
                        tvis.item = tvi;
                        TreeView_InsertItem(pnode->hwnd_symtree, &tvis);
                        words2_len = snprintf(words2 + words2_buf_len, words2_remain_len, "%s ", row_str2[0]);
                        if (words2_len > 0)
                        {
                            words2_buf_len += words2_len;
                            words2_remain_len -= words2_len;
                        }
                    }
                }
                mysql_sub->fn_mysql_free_result(m_result2);
            }
        }
        mysql_sub->fn_mysql_free_result(m_result);
    }
    else if (_stricmp(pnode->db_config.dbtype, "Oracle") == 0)
    {
        sword result;
        oci_lib *oci_sub = &(db_funcs.m_oci);
        oci_handle *this_oci = &(pnode->udb.handles.h_oci);
        OCIStmt *stmthpp = NULL;
        OCIStmt *stmthpp2 = NULL;
        OCIDefine *ocid_table_name = NULL;
        OCIDefine *ocid_column_name = NULL;
        OCIDefine *ocid_data_type = NULL;
        OCIDefine *ocid_data_length = NULL;
        OCIDefine *ocid_data_precision = NULL;
        OCIDefine *ocid_data_scale = NULL;
        char table_name[128 + 1];
        char column_name[128 + 1];
        char data_type[128 + 1];
        int data_length = 0;
        int data_precision = 0;
        int data_scale = 0;
        ub2 table_name_len;
        ub2 column_name_len;
        ub2 data_type_len;
        ub2 data_length_len;
        ub2 data_precision_len;
        ub2 data_scale_len;
        sb2 table_name_indicator = 0;
        sb2 column_name_indicator = 0;
        sb2 data_type_indicator = 0;
        sb2 data_length_indicator = 0;
        sb2 data_precision_indicator = 0;
        sb2 data_scale_indicator = 0;
        char sql[MAX_BUFFER] = "SELECT table_name FROM user_tables";
        oci_sub->fnOCIHandleAlloc((dvoid *) (this_oci->envhpp), (dvoid **) &stmthpp, OCI_HTYPE_STMT, (size_t) 0, (dvoid **) 0);
        result =
            oci_sub->fnOCIStmtPrepare(stmthpp, this_oci->errhpp, (text *) sql, (ub4) strlen(sql), (ub4) OCI_NTV_SYNTAX, (ub4) OCI_DEFAULT);
        if (result != OCI_SUCCESS)
        {
            int err_code;
            char err_desc[512] = { 0 };
            LOAD_I18N_RESSTR(IDC_MSG_SYMTREE_ERR4, err4);
            on_table_oci_error(pnode, this_oci->errhpp, &err_code, err_desc, _countof(err_desc) - 1);
            on_result_append_text(pnode->hwnd_qredit, err4, err_code, util_make_u16(err_desc, utf_str, MAX_BUFFER));
            return -1;
        }
        oci_sub->fnOCIDefineByPos(stmthpp,
                                  &ocid_table_name,
                                  this_oci->errhpp,
                                  1,
                                  (dvoid *) table_name,
                                  _countof(table_name) - 1,
                                  SQLT_STR,
                                  (void *)(uintptr_t)table_name_indicator,
                                  &table_name_len,
                                  NULL,
                                  OCI_DEFAULT);
        result = oci_sub->fnOCIStmtExecute(this_oci->svchpp,
                                           stmthpp,
                                           this_oci->errhpp,
                                           (ub4) 1,
                                           (ub4) 0,
                                           NULL,
                                           NULL,
                                           OCI_DEFAULT);
        if (result != OCI_SUCCESS)
        {
            int err_code;
            char err_desc[512] = { 0 };
            LOAD_I18N_RESSTR(IDC_MSG_SYMTREE_ERR4, err4);
            on_table_oci_error(pnode, this_oci->errhpp, &err_code, err_desc, _countof(err_desc) - 1);
            on_result_append_text(pnode->hwnd_qredit, err4, err_code, util_make_u16(err_desc, utf_str, MAX_BUFFER));
            return -1;
        }
        TreeView_DeleteAllItems(pnode->hwnd_symtree);
        do
        {          
            memset(&tvi, 0, sizeof(TVITEM));
            memset(&tvis, 0, sizeof(TVINSERTSTRUCT));
            tvi.mask = TVIF_TEXT;
            tvi.pszText = util_make_u16(table_name, utf_str, MAX_BUFFER);
            tvis.hParent = TVI_ROOT;
            tvis.hInsertAfter = TVI_LAST;
            tvis.item = tvi;
            hti_root = TreeView_InsertItem(pnode->hwnd_symtree, &tvis);
            words2_len = snprintf(words2 + words2_buf_len, words2_remain_len, "%s ", table_name);
            if (words2_len > 0)
            {
                words2_buf_len += words2_len;
                words2_remain_len -= words2_len;
            }
            snprintf(sql,
                     _countof(sql) - 1,
                     "SELECT column_name,data_type,data_length,data_precision,data_scale "
                     "FROM user_tab_columns WHERE table_name='%s' ORDER BY column_id ASC",
                     table_name);

            oci_sub->fnOCIHandleAlloc((dvoid *) (this_oci->envhpp),
                                      (dvoid **) &stmthpp2,
                                      OCI_HTYPE_STMT,
                                      (size_t) 0,
                                      (dvoid **) 0);
            result = oci_sub->fnOCIStmtPrepare(stmthpp2,
                                               this_oci->errhpp,
                                               (text *) sql,
                                               (ub4) strlen(sql),
                                               (ub4) OCI_NTV_SYNTAX,
                                               (ub4) OCI_DEFAULT);
            if (result != OCI_SUCCESS)
            {
                int err_code;
                char err_desc[512] = { 0 };
                LOAD_I18N_RESSTR(IDC_MSG_SYMTREE_ERR5, err5);
                on_table_oci_error(pnode, this_oci->errhpp, &err_code, err_desc, _countof(err_desc) - 1);
                on_result_append_text(pnode->hwnd_qredit, err5, err_code, util_make_u16(err_desc, utf_str, MAX_BUFFER));
                return -1;
            }
            oci_sub->fnOCIDefineByPos(stmthpp2,
                                      &ocid_column_name,
                                      this_oci->errhpp,
                                      1,
                                      (dvoid *) column_name,
                                      sizeof(column_name) - 1,
                                      SQLT_STR,
                                      (void *) &column_name_indicator,
                                      &column_name_len,
                                      NULL,
                                      OCI_DEFAULT);
            oci_sub->fnOCIDefineByPos(stmthpp2,
                                      &ocid_data_type,
                                      this_oci->errhpp,
                                      2,
                                      (dvoid *) data_type,
                                      sizeof(data_type) - 1,
                                      SQLT_STR,
                                      (void *) &data_type_indicator,
                                      &data_type_len,
                                      NULL,
                                      OCI_DEFAULT);
            oci_sub->fnOCIDefineByPos(stmthpp2,
                                      &ocid_data_length,
                                      this_oci->errhpp,
                                      3,
                                      (dvoid *) &data_length,
                                      sizeof(data_length),
                                      SQLT_INT,
                                      (void *) &data_length_indicator,
                                      &data_length_len,
                                      NULL,
                                      OCI_DEFAULT);
            oci_sub->fnOCIDefineByPos(stmthpp2,
                                      &ocid_data_precision,
                                      this_oci->errhpp,
                                      4,
                                      (dvoid *) &data_precision,
                                      sizeof(data_precision),
                                      SQLT_INT,
                                      (void *) &data_precision_indicator,
                                      &data_precision_len,
                                      NULL,
                                      OCI_DEFAULT);
            oci_sub->fnOCIDefineByPos(stmthpp2,
                                      &ocid_data_scale,
                                      this_oci->errhpp,
                                      5,
                                      (dvoid *) &data_scale,
                                      sizeof(data_scale),
                                      SQLT_INT,
                                      (void *) &data_scale_indicator,
                                      &data_scale_len,
                                      NULL,
                                      OCI_DEFAULT);

            result = oci_sub->fnOCIStmtExecute(this_oci->svchpp,
                                               stmthpp2,
                                               this_oci->errhpp,
                                               (ub4) 1,
                                               (ub4) 0,
                                               NULL,
                                               NULL,
                                               OCI_DEFAULT);
            if (result != OCI_SUCCESS)
            {
                int err_code;
                char err_desc[512] = { 0 };
                LOAD_I18N_RESSTR(IDC_MSG_SYMTREE_ERR6, err6);
                on_table_oci_error(pnode, this_oci->errhpp, &err_code, err_desc, _countof(err_desc) - 1);
                on_result_append_text(pnode->hwnd_qredit, err6, err_code, util_make_u16(err_desc, utf_str, MAX_BUFFER));
                return -1;
            }
            do
            {
                char buf[MAX_BUFFER];
                memset(&tvi, 0, sizeof(TVITEM));
                memset(&tvis, 0, sizeof(TVINSERTSTRUCT));
                if (data_precision_indicator == -1)
                {
                    _snprintf(buf, _countof(buf) - 1, "%s --%s(%d)", column_name, data_type, data_length);
                }
                else
                {
                    _snprintf(buf, _countof(buf) - 1, "%s --%s(%d,%d)", column_name, data_type, data_precision, data_scale);
                }              
                tvi.mask = TVIF_TEXT;
                tvi.pszText = util_make_u16(buf, utf_str, MAX_BUFFER);
                tvis.hParent = hti_root;
                tvis.hInsertAfter = TVI_LAST;
                tvis.item = tvi;
                TreeView_InsertItem(pnode->hwnd_symtree, &tvis);
                words2_len = snprintf(words2 + words2_buf_len, words2_remain_len, "%s ", column_name);
                if (words2_len > 0)
                {
                    words2_buf_len += words2_len;
                    words2_remain_len -= words2_len;
                }               
            } while (oci_sub->fnOCIStmtFetch2(stmthpp2, this_oci->errhpp, 1, OCI_FETCH_NEXT, 1, OCI_DEFAULT) != OCI_NO_DATA);
            oci_sub->fnOCIHandleFree((dvoid *) stmthpp2, OCI_HTYPE_STMT);
        } while (oci_sub->fnOCIStmtFetch2(stmthpp, this_oci->errhpp, 1, OCI_FETCH_NEXT, 1, OCI_DEFAULT) != OCI_NO_DATA);
        oci_sub->fnOCIHandleFree((dvoid *) stmthpp, OCI_HTYPE_STMT);
    }
    else if (_stricmp(pnode->db_config.dbtype, "Sqlite3") == 0)
    {
        char *errmsg = NULL;
        char **result = NULL;
        int nrow;
        int ncolumn;
        int row;
        int index;
        char **result2 = NULL;
        int nrow2;
        int ncolumn2;
        int row2;
        int index2;
        sql3_handle *this_sql3 = &(pnode->udb.handles.h_sql3);
        _snprintf(sql, sizeof(sql) - 1, "select name from sqlite_master where type='table' order by name");
        err = eu_sqlite3_get_table(this_sql3->sqlite3, sql, &result, &nrow, &ncolumn, &errmsg);
        if (err)
        {
            if (errmsg)
            {
                LOAD_I18N_RESSTR(IDC_MSG_SYMTREE_ERR7, errbox);
                on_result_append_text(pnode->hwnd_qredit, errbox, util_make_u16(errmsg, utf_str, MAX_BUFFER));
                eu_safe_free(errmsg);
            }
            free(words2);
            return -1;
        }
        TreeView_DeleteAllItems(pnode->hwnd_symtree);
        for (row = 0, index = ncolumn; row < nrow; row++, index++)
        {           
            memset(&tvi, 0, sizeof(TVITEM));
            memset(&tvis, 0, sizeof(TVINSERTSTRUCT));
            tvi.mask = TVIF_TEXT;
            tvi.pszText = util_make_u16(result[index], utf_str, MAX_BUFFER);
            tvis.hParent = TVI_ROOT;
            tvis.hInsertAfter = TVI_LAST;
            tvis.item = tvi;
            hti_root = TreeView_InsertItem(pnode->hwnd_symtree, &tvis);
            words2_len = snprintf(words2 + words2_buf_len, words2_remain_len, "%s ", result[index]);
            if (words2_len > 0)
            {
                words2_buf_len += words2_len;
                words2_remain_len -= words2_len;
            }
            snprintf(sql, _countof(sql) - 1, "pragma table_info ('%s')", result[index]);
            err = eu_sqlite3_get_table(this_sql3->sqlite3, sql, &result2, &nrow2, &ncolumn2, &errmsg);
            if (err)
            {
                printf("sqlite3_get_table2 failed\n");
                if (errmsg)
                {
                    LOAD_I18N_RESSTR(IDC_MSG_SYMTREE_ERR8, errbox);
                    on_result_append_text(pnode->hwnd_qredit, errbox, util_make_u16(errmsg, utf_str, MAX_BUFFER));
                    eu_safe_free(errmsg);
                }
                eu_sqlite3_free_table(result);
                return -1;
            }
            for (row2 = 0, index2 = ncolumn2; row2 < nrow2; row2++, index2 += ncolumn2)
            {
                char buf[MAX_BUFFER] = {0};
                memset(&tvi, 0, sizeof(TVITEM));
                memset(&tvis, 0, sizeof(TVINSERTSTRUCT));
                snprintf(buf, _countof(buf) - 1, "%s --%s", result2[index2 + 1], result2[index2 + 2]);         
                tvi.mask = TVIF_TEXT;
                tvi.pszText = util_make_u16(buf, utf_str, MAX_BUFFER);
                tvis.hParent = hti_root;
                tvis.hInsertAfter = TVI_LAST;
                tvis.item = tvi;
                TreeView_InsertItem(pnode->hwnd_symtree, &tvis);
                words2_len = snprintf(words2 + words2_buf_len, words2_remain_len, "%s ", result2[index2 + 1]);
                if (words2_len > 0)
                {
                    words2_buf_len += words2_len;
                    words2_remain_len -= words2_len;
                }
            }
            eu_sqlite3_free_table(result2);
        }
        eu_sqlite3_free_table(result);
    }
    else if (_stricmp(pnode->db_config.dbtype, "PostgreSQL") == 0)
    {
        int nrow;
        int row;
        int nrow2;
        int row2;  
        PGresult *res = NULL;
        PGresult *res2 = NULL;
        pq_lib *pq_sub = &(db_funcs.m_pq);
        pq_handle *this_pq = &(pnode->udb.handles.h_pq);
        _snprintf(sql,
                  _countof(sql) - 1,
                  "SELECT table_name FROM information_schema.TABLES WHERE table_catalog='%s' AND "
                  "table_schema<>'pg_catalog' AND table_schema<>'information_schema' AND table_type='BASE TABLE' ORDER BY "
                  "table_name ASC",
                  pnode->db_config.dbname);
        res = pq_sub->fnPQexec(this_pq->postgres, sql);
        if (pq_sub->fnPQresultStatus(res) != PGRES_TUPLES_OK)
        {
            char *errmsg = pq_sub->fnPQresultErrorMessage(res);
            if (errmsg)
            {
                LOAD_I18N_RESSTR(IDC_MSG_SYMTREE_ERR7, errbox);
                on_result_append_text(pnode->hwnd_qredit, errbox, util_make_u16(errmsg, utf_str, MAX_BUFFER));
            }
            free(words2);
            pq_sub->fnPQclear(res);
            res = NULL;
            return -1;
        }
        TreeView_DeleteAllItems(pnode->hwnd_symtree);
        nrow = pq_sub->fnPQntuples(res);
        for (row = 0; row < nrow; row++)
        {       
            memset(&tvi, 0, sizeof(TVITEM));
            memset(&tvis, 0, sizeof(TVINSERTSTRUCT));
            tvi.mask = TVIF_TEXT;
            tvi.pszText = util_make_u16(pq_sub->fnPQgetvalue(res, row, 0), utf_str, MAX_BUFFER);
            tvis.hParent = TVI_ROOT;
            tvis.hInsertAfter = TVI_LAST;
            tvis.item = tvi;
            hti_root = TreeView_InsertItem(pnode->hwnd_symtree, &tvis);
            words2_len = snprintf(words2 + words2_buf_len, words2_remain_len, "%s ", pq_sub->fnPQgetvalue(res, row, 0));
            if (words2_len > 0)
            {
                words2_buf_len += words2_len;
                words2_remain_len -= words2_len;
            }           
            snprintf(sql,
                     _countof(sql) - 1,
                     "SELECT column_name,data_type,character_maximum_length,numeric_precision,numeric_scale FROM "
                     "information_schema."
                     "COLUMNS WHERE table_catalog='%s' AND table_name='%s' ORDER BY ordinal_position ASC",
                     pnode->db_config.dbname,
                     pq_sub->fnPQgetvalue(res, row, 0));
            res2 = pq_sub->fnPQexec(this_pq->postgres, sql);
            if (pq_sub->fnPQresultStatus(res2) != PGRES_TUPLES_OK)
            {
                char *errmsg = pq_sub->fnPQresultErrorMessage(res2);
                if (errmsg)
                {
                    LOAD_I18N_RESSTR(IDC_MSG_SYMTREE_ERR8, errbox);
                    on_result_append_text(pnode->hwnd_qredit, errbox, util_make_u16(errmsg, utf_str, MAX_BUFFER));
                }
                free(words2);
                pq_sub->fnPQclear(res2);
                res2 = NULL;
                return -1;
            }
            nrow2 = pq_sub->fnPQntuples(res2);
            for (row2 = 0; row2 < nrow2; row2++)
            {
                char buf[MAX_BUFFER] = {0};
                memset(&tvi, 0, sizeof(TVITEM));
                memset(&tvis, 0, sizeof(TVINSERTSTRUCT));
                
                if (!pq_sub->fnPQgetisnull(res2, row2, 2))
                {
                    snprintf(buf,
                             _countof(buf) - 1,
                             "%s --%s(%s)",
                             pq_sub->fnPQgetvalue(res2, row2, 0),
                             pq_sub->fnPQgetvalue(res2, row2, 1),
                             pq_sub->fnPQgetvalue(res2, row2, 2));
                }
                else
                {
                    snprintf(buf,
                             _countof(buf) - 1,
                             "%s --%s(%s,%s)",
                             pq_sub->fnPQgetvalue(res2, row2, 0),
                             pq_sub->fnPQgetvalue(res2, row2, 1),
                             pq_sub->fnPQgetvalue(res2, row2, 3),
                             pq_sub->fnPQgetvalue(res2, row2, 4));
                }                
                tvi.mask = TVIF_TEXT;
                tvi.pszText = util_make_u16(buf, utf_str, MAX_BUFFER);
                tvis.hParent = hti_root;
                tvis.hInsertAfter = TVI_LAST;
                tvis.item = tvi;
                TreeView_InsertItem(pnode->hwnd_symtree, &tvis);
                words2_len = snprintf(words2 + words2_buf_len, words2_remain_len, "%s ", pq_sub->fnPQgetvalue(res2, row2, 0));
                if (words2_len > 0)
                {
                    words2_buf_len += words2_len;
                    words2_remain_len -= words2_len;
                }
            }
        }
        if (res)
        {
            pq_sub->fnPQclear(res);
        }
        if (res2)
        {
            pq_sub->fnPQclear(res2);
        }
    }
    else
    {
        LOAD_I18N_RESSTR(IDC_MSG_QUERY_ERR26, msg_str);
        on_result_append_text(pnode->hwnd_qredit, msg_str, util_make_u16(pnode->db_config.dbtype, utf_str, MAX_BUFFER));
        return 0;
    }
    eu_sci_call(pnode, SCI_SETKEYWORDS, 1, (sptr_t) words2);
    eu_sci_call(pnode, SCI_STYLESETFORE, SCE_C_WORD2, (sptr_t)(eu_get_theme()->item.keywords1.color));
    eu_sci_call(pnode, SCI_STYLESETBOLD, SCE_C_WORD2, (sptr_t)(eu_get_theme()->item.keywords1.bold));
    free(words2);
    return 0;
}

static HTREEITEM
redis_reply(eu_tabpage *pnode, HTREEITEM hTreeItem, struct redisReply *reply)
{
    char buf[FILESIZE];
    TCHAR *p_buf = NULL;
    TVITEM tvi = {0};
    TVINSERTSTRUCT tvis = {0};
    HTREEITEM hti;
    HTREEITEM hret;
    if (reply->type == REDIS_REPLY_STATUS)
    {
        snprintf(buf, _countof(buf) - 1, "%s (STATUS)", reply->str);
    }
    else if (reply->type == REDIS_REPLY_ERROR)
    {
        snprintf(buf, _countof(buf) - 1, "%s (ERROR)", reply->str);
    }
    else if (reply->type == REDIS_REPLY_STRING)
    {
        if (reply->len > MAX_BUFFER)
        {
            snprintf(buf, _countof(buf) - 1, "%.*s...... (STRING)", FILESIZE - 18, reply->str);
        }
        else
        {
            snprintf(buf, _countof(buf) - 1, "%.*s (STRING)", reply->len, reply->str);
        }
    }
    else if (reply->type == REDIS_REPLY_INTEGER)
    {
        snprintf(buf, _countof(buf) - 1, "%lld (ERROR)", reply->integer);
    }
    else if (reply->type == REDIS_REPLY_NIL)
    {
        snprintf(buf, _countof(buf) - 1, "(null) (NIL)");
    }
    else if (reply->type == REDIS_REPLY_ARRAY)
    {
        snprintf(buf, _countof(buf) - 1, "%d (ELEMENTS)", reply->elements);
    }
#ifdef _UNICODE
    if ((p_buf = eu_utf8_utf16(buf, NULL)) == NULL)
    {
        return NULL;
    }
#else
    p_buf = buf;
#endif
    tvi.mask = TVIF_TEXT;
    tvi.pszText = p_buf;
    tvis.hParent = hTreeItem;
    tvis.hInsertAfter = TVI_LAST;
    tvis.item = tvi;
    hti = TreeView_InsertItem(pnode->hwnd_symtree, &tvis);
#ifdef _UNICODE    
    free(p_buf);
#endif
    if (reply->type == REDIS_REPLY_ARRAY)
    {
        for (size_t i = 0; i < reply->elements; i++)
        {
            hret = redis_reply(pnode, hti, reply->element[i]);
            if (hret == NULL) 
            {
                return hret;
            }
        }
    }
    return hti;
}

void
on_symtree_disconnect_redis(eu_tabpage *pnode)
{
    if (pnode->rs_handle.ctx)
    {
        redis_funcs.fnRedisFree(pnode->rs_handle.ctx);
        pnode->rs_handle.ctx = NULL;
    }
    safe_close_dll(pnode->rs_handle.hiredis_dll);
    pnode->redis_is_connect = false;
}

int
on_symtree_connect_redis(eu_tabpage *pnode)
{
    int nret = 0;
    HTREEITEM hti;
    if (!pnode)
    {
        return -1;
    }
    if (pnode->redis_is_connect)
    {
        return 0;
    }
    if (pnode->redis_config.host[0] == '\0') 
    {
        return -1;
    }
    if (pnode->rs_handle.hiredis_dll == NULL)
    {
        TCHAR dll_path[MAX_PATH+1] = {0};
        _sntprintf(dll_path, MAX_PATH, _T("%s\\%s"), eu_module_path, _T("hiredis.dll"));
        pnode->rs_handle.hiredis_dll = LoadLibrary(dll_path);
        if (pnode->rs_handle.hiredis_dll == NULL)
        {
            MSG_BOX(IDC_MSG_SYMTREE_ERR10, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
            return -1;
        }
        redis_funcs.fnRedisConnectWithTimeout =
            (pRedisConnectWithTimeout *) GetProcAddress(pnode->rs_handle.hiredis_dll, "redisConnectWithTimeout");
        redis_funcs.fnRedisFree = (pRedisFree *) GetProcAddress(pnode->rs_handle.hiredis_dll, "redisFree");
        redis_funcs.fnRedisCommand = (pRedisCommand *) GetProcAddress(pnode->rs_handle.hiredis_dll, "redisCommand");
        redis_funcs.fnFreeReplyObject = (pFreeReplyObject *) GetProcAddress(pnode->rs_handle.hiredis_dll, "freeReplyObject");
        if (redis_funcs.fnRedisConnectWithTimeout == NULL || redis_funcs.fnRedisFree == NULL ||
            redis_funcs.fnRedisCommand == NULL || redis_funcs.fnFreeReplyObject == NULL)
        {
            MSG_BOX(IDC_MSG_SYMTREE_ERR11, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
            on_symtree_disconnect_redis(pnode);
            return -1;
        }
    }
    if (pnode->rs_handle.ctx == NULL)
    {
        struct timeval timeout;
        char command[MAXLEN_FILENAME];
        struct redisReply *reply = NULL;

        timeout.tv_sec = 10;
        timeout.tv_usec = 0;
        pnode->rs_handle.ctx = redis_funcs.fnRedisConnectWithTimeout(pnode->redis_config.host, pnode->redis_config.port, timeout);
        if (pnode->rs_handle.ctx == NULL || pnode->rs_handle.ctx->err)
        {
            MSG_BOX(IDC_MSG_SYMTREE_ERR12, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
            on_symtree_disconnect_redis(pnode);
            return -1;
        }

        if (pnode->redis_config.pass[0])
        {
            memset(command, 0, sizeof(command));
            snprintf(command, sizeof(command) - 1, "AUTH %s", pnode->redis_config.pass);
            reply = (struct redisReply *) redis_funcs.fnRedisCommand(pnode->rs_handle.ctx, command);
            if (reply == NULL || reply->type == REDIS_REPLY_ERROR)
            {
                MSG_BOX(IDC_MSG_SYMTREE_ERR13, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                if (reply)
                {
                    redis_funcs.fnFreeReplyObject(reply);
                }
                on_symtree_disconnect_redis(pnode);
                return -1;
            }
            redis_funcs.fnFreeReplyObject(reply);
        }
        if (pnode->redis_config.dbsl[0])
        {
            memset(command, 0, sizeof(command));
            snprintf(command, sizeof(command) - 1, "SELECT %s", pnode->redis_config.dbsl);
            reply = (struct redisReply *) redis_funcs.fnRedisCommand(pnode->rs_handle.ctx, command);
            if (reply == NULL || reply->type == REDIS_REPLY_ERROR)
            {
                MSG_BOX(IDC_MSG_SYMTREE_ERR14, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
                if (reply)
                {
                    redis_funcs.fnFreeReplyObject(reply);
                }
                on_symtree_disconnect_redis(pnode);
                return -1;
            }
            redis_funcs.fnFreeReplyObject(reply);
        }
        reply = (struct redisReply *) redis_funcs.fnRedisCommand(pnode->rs_handle.ctx, "PING");
        if (reply == NULL || reply->type == REDIS_REPLY_ERROR)
        {
            MSG_BOX(IDC_MSG_SYMTREE_ERR15, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
            if (reply)
            {
                redis_funcs.fnFreeReplyObject(reply);
            }
            on_symtree_disconnect_redis(pnode);
            return -1;
        }
        // 写入侧边栏
        TreeView_DeleteAllItems(pnode->hwnd_symtree);
        hti = redis_reply(pnode, TVI_ROOT, reply);
        redis_funcs.fnFreeReplyObject(reply);
        if (hti)
        {
            TreeView_Expand(pnode->hwnd_symtree, hti, TVE_EXPAND);
        }
    }
    pnode->redis_is_connect = true;
    return 0;
}

static bool
parse_redis_header(eu_tabpage *pnode)
{
    sptr_t file_line;
    sptr_t file_line_count;
    bool in_config_secion = false;
    bool m_change = false;
    redis_conn redis_config = {0};
    file_line_count = eu_sci_call(pnode, SCI_GETLINECOUNT, 0, 0);
    for (file_line = 0; file_line <= file_line_count; ++file_line)
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
            if (!strncmp(util_trim_left_white(line_buf, NULL), BEGIN_REDIS_CONNECTION_CONFIG, strlen(BEGIN_REDIS_CONNECTION_CONFIG)))
            {
                in_config_secion = true;
            }
        }
        else
        {
            if (!strncmp(util_trim_left_white(line_buf, NULL), END_REDIS_CONNECTION_CONFIG, strlen(END_REDIS_CONNECTION_CONFIG)))
            {
                if (redis_config.host[0])
                {
                    if ((pnode->redis_config.host[0] && strcmp(pnode->redis_config.host, redis_config.host)) ||
                        (pnode->redis_config.port && pnode->redis_config.port != redis_config.port) ||
                        (pnode->redis_config.pass[0] && strcmp(pnode->redis_config.pass, redis_config.pass)) ||
                        (pnode->redis_config.dbsl[0] && strcmp(pnode->redis_config.dbsl, redis_config.dbsl)))
                    {
                        m_change = true;
                    }
                    memcpy(&(pnode->redis_config), &redis_config, sizeof(redis_conn));
                }
                break;
            }
            if (sscanf(line_buf, "%s%s%s%s", config_remark, config_key, config_eval, config_value) != 4)
            {
                break;
            }
            if (strcmp(config_remark, "--") != 0 || strcmp(config_eval, ":") != 0)
            {
                continue;
            }
            if (strcmp(config_key, "HOST") == 0)
            {
                strncpy(redis_config.host, config_value, sizeof(redis_config.host) - 1);
            }
            else if (strcmp(config_key, "PORT") == 0)
            {
                redis_config.port = atoi(config_value);
            }
            else if (strcmp(config_key, "PASS") == 0)
            {
                strncpy(redis_config.pass, config_value, sizeof(redis_config.pass) - 1);
            }
            else if (strcmp(config_key, "DBSL") == 0)
            {
                strncpy(redis_config.dbsl, config_value, sizeof(redis_config.dbsl) - 1);
            }
        }
    }
    if (m_change && pnode->redis_is_connect)
    {
        on_symtree_disconnect_redis(pnode);
    }
    return (on_symtree_connect_redis(pnode) == 0);
}

int
on_symtree_query_redis(eu_tabpage *pnode)
{
    sptr_t start_pos;
    sptr_t end_pos;
    sptr_t sel_len;
    char *sel_cmd = NULL;
    char *sel_event = NULL;
    const char *eol = on_encoding_get_eol(pnode);
    struct redisReply *reply = NULL;
    HTREEITEM hti = NULL;
    if (!parse_redis_header(pnode))
    {
        printf("parse_redis_header return false\n");
        return -1;
    }
    start_pos = eu_sci_call(pnode, SCI_GETSELECTIONSTART, 0, 0);
    end_pos = eu_sci_call(pnode, SCI_GETSELECTIONEND, 0, 0);
    sel_len = end_pos - start_pos;
    if (sel_len <= 0)
    {
        sel_cmd = util_strdup_line(pnode, NULL);
    }
    else
    {
        sel_cmd = on_sci_range_text(pnode, start_pos, end_pos);
    }
    if (sel_cmd == NULL)
    {
        printf("memory allocation failed\n");
        return -1;
    }
    sel_event = strtok(sel_cmd, eol);
    while (sel_event && *sel_event)
    {
        printf("sel_envent = %s\n", sel_event);
        if (!strncmp(util_trim_left_white(sel_event, NULL), "--", strlen("--")))
        {
            sel_event = strtok(NULL, eol);
            continue;
        }
        if (sel_event[strlen(sel_event)-1] == ';')
        {
            sel_event[strlen(sel_event)-1] = 0;
        }
        if (strstr(sel_event, "--"))
        {
            strstr(sel_event, "--")[0] = 0;
        }
        reply = (struct redisReply *) redis_funcs.fnRedisCommand(pnode->rs_handle.ctx, sel_event);
        if (reply == NULL)
        {
            on_symtree_disconnect_redis(pnode);
            return -1;
        }
        TreeView_DeleteAllItems(pnode->hwnd_symtree);
        hti = redis_reply(pnode, TVI_ROOT, reply);
        redis_funcs.fnFreeReplyObject(reply);
        if (hti)
        {
            TreeView_Expand(pnode->hwnd_symtree, hti, TVE_EXPAND);
        }
        sel_event = strtok(NULL, eol);
    }
    free(sel_cmd);
    return 0;
}

static HTREEITEM
tvi_insert_str(HWND hwnd, HTREEITEM parent, const char *str, int64_t pos)
{
    HTREEITEM hti = NULL;
    TVITEM tvi = {0};
    TVINSERTSTRUCT tvis = {0};
    TCHAR buf[MAX_PATH] = {0};
    if (!str)
    {
        eu_i18n_load_str(IDC_MSG_SYMTREE_ERR9, buf, MAX_PATH);
    }
    else
    {
        MultiByteToWideChar(CP_UTF8, 0, str, -1, buf, MAX_PATH);
    }
    tvi.mask = TVIF_TEXT | TVIF_PARAM;
    tvi.pszText = buf;
    tvi.lParam = pos;
    tvis.hParent = parent;
    tvis.hInsertAfter = TVI_LAST;
    tvis.item = tvi;
    hti = TreeView_InsertItem(hwnd, &tvis);
    return hti;
}

static HTREEITEM
tvi_inser_object(HWND dlg, HTREEITEM parent, const char *parent_str, const char *text, int64_t pos)
{
    TVITEM tvi = {0};
    HTREEITEM hti = NULL;
    TVINSERTSTRUCT tvis = {0};
    size_t parent_len = parent_str?strlen(parent_str)+3:0;
    size_t len = parent_len+strlen(text)+1;
    char *txt = (char *)calloc(1, len);
    if (txt)
    {
        if (parent_str)
        {
            sprintf(txt, "%s : %s", parent_str, text);
        }
        else
        {
            sprintf(txt, "%s", text);
        }
        TCHAR *u16_txt = eu_utf8_utf16(txt, NULL);
        if (u16_txt)
        {
            tvi.pszText = u16_txt;
            tvi.mask = TVIF_TEXT | TVIF_PARAM;
            tvi.lParam = pos;
            tvis.hParent = parent;
            tvis.hInsertAfter = TVI_LAST;
            tvis.item = tvi;
            hti = TreeView_InsertItem(dlg, &tvis);
            free(u16_txt);
        }
        free(txt);
    }
    return hti;
}

static void
expand_all_child(HWND htree, HTREEITEM hitem)
{
    HTREEITEM hchild = TreeView_GetChild(htree, hitem);
    while (hchild)
    {
        TreeView_Expand(htree,hchild,TVE_EXPAND);
        expand_all_child(htree, hchild);
        hchild = TreeView_GetNextItem(htree, hchild, TVGN_NEXT);
    }
}

static void
process_value(eu_tabpage *pnode, HTREEITEM tree_root, json_value *value, int x);

static void
process_object(eu_tabpage *pnode, HTREEITEM tree_root, json_value *value, int x, int64_t pos)
{
    int length;
    HTREEITEM new_tvi = NULL;
    if (value == NULL)
    {
        return;
    }
    length = value->u.object.length;

    if (value->parent != NULL && value->parent->type != json_array && value->parent->u.object.values[x].name)
    {
        new_tvi = tvi_insert_str(pnode->hwnd_symtree, tree_root, value->parent->u.object.values[x].name, pos);
    }
    else
    {
        new_tvi = tvi_insert_str(pnode->hwnd_symtree, tree_root, "(object)", pos);
    }
    for (int i = 0; i < length; i++)
    {
        process_value(pnode, new_tvi, value->u.object.values[i].value, i);
    }
}

static void
process_array(eu_tabpage *pnode, HTREEITEM tree_root, json_value *value, int x, int64_t pos)
{
    int length;
    HTREEITEM new_tvi = NULL;
    if (value == NULL)
    {
        return;
    }
    length = value->u.array.length;
    if (value->parent != NULL && value->parent->type != json_array && value->parent->u.object.values[x].name)
    {
        new_tvi = tvi_inser_object(pnode->hwnd_symtree, tree_root, value->parent->u.object.values[x].name, " (array)", pos);
    }
    else
    {
        new_tvi = tree_root;
    }
    for (int i = 0; i < length; i++)
    {
        process_value(pnode, new_tvi, value->u.array.values[i], i);
    }
}

static void
process_value(eu_tabpage *pnode, HTREEITEM new_tvi, json_value *json_root, int x)
{
    int64_t pos = 0;
    if (json_root == NULL)
    {
        return;
    }
    if (json_root->line > 0)
    {
        pos = eu_sci_call(pnode, SCI_POSITIONFROMLINE, json_root->line, 0);
    }
    switch (json_root->type)
    {
        case json_none:
            printf("none\n");
            break;
        case json_object:
            process_object(pnode, new_tvi, json_root, x, pos);
            break;
        case json_array:
            process_array(pnode, new_tvi, json_root, x, pos);
            break;
        case json_integer:
        {
            char i2s[64] = { 0 };
            _i64toa(json_root->u.integer, i2s, 10);
            if (json_root->parent->type != json_array)
            {
                tvi_inser_object(pnode->hwnd_symtree, new_tvi, json_root->parent->u.object.values[x].name, i2s, pos);
            }
            else
            {
                tvi_insert_str(pnode->hwnd_symtree, new_tvi, i2s, pos);
            }
            break;
        }
        case json_double:
        {
            char f2s[64] = { 0 };
            _snprintf(f2s, 64, "%g", json_root->u.dbl);
            if (json_root->parent->type != json_array)
            {
                tvi_inser_object(pnode->hwnd_symtree, new_tvi, json_root->parent->u.object.values[x].name, f2s, pos);
            }
            else
            {
                tvi_insert_str(pnode->hwnd_symtree, new_tvi, f2s, pos);
            }
            break;
        }
        case json_string:
            if (json_root->parent->type != json_array)
            {
                tvi_inser_object(pnode->hwnd_symtree, new_tvi, json_root->parent->u.object.values[x].name, json_root->u.string.ptr, pos);
            }
            else
            {
                tvi_insert_str(pnode->hwnd_symtree, new_tvi, json_root->u.string.ptr, pos);
            }
            break;
        case json_boolean:
            if (json_root->parent->type != json_array)
            {
                tvi_inser_object(pnode->hwnd_symtree, new_tvi, json_root->parent->u.object.values[x].name, json_root->u.jbool ? "true" : "false", pos);
            }
            else
            {
                tvi_insert_str(pnode->hwnd_symtree, new_tvi, json_root->u.jbool ? "true" : "false", pos);
            }
            break;
        case json_null:
            if (json_root->parent->type != json_array)
            {
                tvi_inser_object(pnode->hwnd_symtree, new_tvi, json_root->parent->u.object.values[x].name, "null", pos);
            }
            else
            {
                tvi_insert_str(pnode->hwnd_symtree, new_tvi, "null", pos);
            }
            break;
        default:
            break;
    }
}

static bool
init_json_tree(eu_tabpage *pnode, const char *buffer, int64_t len)
{
    bool ret = false;
    HTREEITEM tree_root = NULL;
    json_settings sets = {0};
    json_value *json_root = NULL;
    sets.settings |= json_enable_comments;
    TreeView_DeleteAllItems(pnode->hwnd_symtree);
    if ((json_root = json_parse_ex(&sets, buffer, len, NULL)))
    {
        process_value(pnode, tree_root, json_root, 0);
        json_value_free(json_root);
        ret = true;
    }
    else
    {
        tvi_insert_str(pnode->hwnd_symtree, tree_root, NULL, 0);
    }
    expand_all_child(pnode->hwnd_symtree, tree_root);
    return ret;
}

unsigned WINAPI
cjson_thread(void *lp)
{
    char *text = NULL;
    eu_tabpage *pnode = (eu_tabpage *) lp;
    if (!pnode)
    {
        return 1;
    }
    if (pnode->hwnd_symtree)
    {
        char *text = NULL;
        sptr_t text_len = 0;
        if ((text = util_strdup_content(pnode, (size_t *)&text_len)) != NULL)
        {
            if (!init_json_tree(pnode, text, text_len))
            {
                printf("json parser failed\n");
            }
        }
    }
    if (text)
    {
        free(text);
    }
    _InterlockedExchange(&pnode->json_id, 0);
    return 0;
}

int
on_symtree_json(eu_tabpage *pnode)
{
    if (!pnode)
    {
        return 1;
    }
    if (pnode->raw_size > 0xA00000)
    {   // size > 10MB
        if (pnode->hwnd_symtree)
        {
            DestroyWindow(pnode->hwnd_symtree);
            pnode->hwnd_symtree = NULL;
        }
        return 1;
    }
    CloseHandle((HANDLE) _beginthreadex(NULL, 0, &cjson_thread, pnode, 0, (uint32_t *)&pnode->json_id));
    return 0;
}

int
on_symtree_postion(eu_tabpage *pnode)
{
    HTREEITEM hti;
    TVITEM tvi = { 0 };
    hti = TreeView_GetSelection(pnode->hwnd_symtree);
    if (hti == NULL)
    {
        return 1;
    }
    tvi.mask = TVIF_HANDLE | TVIF_PARAM;
    tvi.hItem = hti;
    if (!TreeView_GetItem(pnode->hwnd_symtree, &tvi))
    {
        return 1;
    }
    eu_sci_call(pnode, SCI_GOTOPOS, tvi.lParam, 0);
    SetFocus(pnode->hwnd_sc);
    return 0;
}

LRESULT CALLBACK
symtree_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    eu_tabpage *pnode = NULL;
    switch (message)
    {
        case WM_COMMAND:
        {
            if (!(pnode = on_tabpage_focus_at()))
            {
                break;
            }
            switch((int)LOWORD(wParam))
            {
                case IDM_RELOAD_SYMBOLTREE:
                    if (pnode->doc_ptr && pnode->doc_ptr->fn_reload_symtree)
                    {
                        pnode->doc_ptr->fn_reload_symtree(pnode);
                    }
                    break;
                case IDM_SELECT_TABLE_SYMBOLTREE:
                {
                    char sql[MAX_PATH+1] = {0};
                    char *cnv = get_symtree_str(pnode, false);
                    if (cnv)
                    {
                        _snprintf(sql, MAX_PATH, "select * from %s;", cnv);
                        on_table_sql_query(pnode, sql);
                        free(cnv);
                    }
                    break;
                }
                case IDM_SELECT_ROW_SYMBOLTREE:
                {
                    char sql[MAX_PATH+1] = {0};
                    char *cnv = get_symtree_str(pnode, false);
                    char *parent = get_symtree_str(pnode, true);
                    if (cnv && parent)
                    {
                        _snprintf(sql, MAX_PATH, "select %s from %s;", cnv, parent);
                        on_table_sql_query(pnode, sql);
                    }
                    if (cnv)
                    {
                        free(cnv);
                    } 
                    if (parent)
                    {
                        free(parent);
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case WM_LBUTTONUP:
        {
            eu_reset_drag_line();
            break;
        }        
        case WM_LBUTTONDBLCLK:
        {
            POINT pt;
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);
            TVHITTESTINFO tvhti;
            memset(&tvhti, 0, sizeof(tvhti));
            memcpy(&(tvhti.pt), &pt, sizeof(POINT));
            if (!(pnode = on_tabpage_focus_at()))
            {
                break;
            }
            TreeView_HitTest(hwnd, &tvhti);
            ClientToScreen(hwnd, &pt);
            if (tvhti.flags & TVHT_ONITEM)
            {
                if (pnode->doc_ptr && pnode->doc_ptr->fn_click_symtree)
                {
                    pnode->doc_ptr->fn_click_symtree(pnode);
                }
                return 1;
            }
            break;
        }
        case WM_RBUTTONDOWN:
        {
            POINT pt;
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);
            TVHITTESTINFO tvhti;
            memset(&tvhti, 0, sizeof(tvhti));
            memcpy(&(tvhti.pt), &pt, sizeof(POINT));
            TreeView_HitTest(hwnd, &tvhti);
            ClientToScreen(hwnd, &pt);
            if (tvhti.flags & TVHT_ONITEM)
            {
                if (!(pnode = (eu_tabpage *) GetWindowLongPtr(hwnd, GWLP_USERDATA)))
                {
                    break;
                }
                if (!pnode->db_is_connect)
                {
                    break;
                }
                TreeView_SelectItem(hwnd, tvhti.hItem);
                if (TreeView_GetChild(hwnd, tvhti.hItem))
                {
                    TrackPopupMenu(pop_symtree_table_menu, 0, pt.x, pt.y, 0, hwnd, NULL);
                }
                else
                {
                    TrackPopupMenu(pop_symtree_row_menu, 0, pt.x, pt.y, 0, hwnd, NULL);
                }
            }
            else if (!(tvhti.flags & TVHT_ONITEM) && tvhti.flags > 0x1 && tvhti.flags < 0x41)
            {
                TrackPopupMenu(pop_symtree_refresh_menu, 0, pt.x, pt.y, 0, hwnd, NULL);
            }
            break;
        }
        case WM_DPICHANGED:
        {
            SendMessage(hwnd, WM_SETFONT, (WPARAM) on_theme_font_hwnd(), 0);   
            break;
        }
        case WM_DESTROY:
        {
            pnode = (eu_tabpage *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
            if (pnode && pnode->json_id)
            {
                util_kill_thread(pnode->json_id);
            }
            printf("symtree WM_DESTROY\n");
            break;
        }
        default:
            break;
    }
    return CallWindowProc(symtree_wnd, hwnd, message, wParam, lParam);
}

int
on_symtree_create(eu_tabpage *pnode)
{
    if (pnode->hwnd_symtree)
    {
        DestroyWindow(pnode->hwnd_symtree);
    }
    pnode->hwnd_symtree =
        CreateWindow(WC_TREEVIEW, NULL, WS_CHILD | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT | WS_TABSTOP, 0, 0, 0, 0, eu_module_hwnd(), NULL, eu_module_handle(), NULL);
    if (pnode->hwnd_symtree == NULL)
    {
        MSG_BOX(IDC_MSG_SYMTREE_ERR1, IDC_MSG_ERROR, MB_ICONERROR | MB_OK);
        return 1;
    }
    if (!(symtree_wnd = (WNDPROC) SetWindowLongPtr(pnode->hwnd_symtree, GWLP_WNDPROC, (LONG_PTR) symtree_proc)))
    {
        printf("SetWindowLongPtr(pnode->hwnd_symtree) failed\n");
        DestroyWindow(pnode->hwnd_symtree);
        pnode->hwnd_symtree = NULL;
        return 1;
    }
    else
    {
        SetWindowLongPtr(pnode->hwnd_symtree, GWLP_USERDATA, (intptr_t) pnode);
    }
    return 0;
}
