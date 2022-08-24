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

#ifndef _EU_TABLECTL_H_
#define _EU_TABLECTL_H_

#define SQL_WD2_SIZE 10 * 1024 * 1024

#ifdef __cplusplus
extern "C"
{
#endif

/* mysql */
typedef MYSQL *p_mysql_init(MYSQL *mysql);
typedef MYSQL *p_mysql_real_connect(MYSQL *, char *, char *, char *, char *, unsigned int, const char *, unsigned long);
typedef int p_mysql_set_character_set(MYSQL *mysql, char *encoding);
typedef int p_mysql_query(MYSQL *mysql, char *sql);
typedef my_ulonglong p_mysql_affected_rows(MYSQL *mysql);
typedef MYSQL_RES *p_mysql_store_result(MYSQL *mysql);
typedef unsigned int p_mysql_num_fields(MYSQL_RES *res);
typedef MYSQL_FIELD *p_mysql_fetch_field(MYSQL_RES *res);
typedef MYSQL_ROW p_mysql_fetch_row(MYSQL_RES *res);
typedef void p_mysql_free_result(MYSQL_RES *res);
typedef MYSQL *p_mysql_close(MYSQL *mysql);

/* Oracle */
typedef void *(*malocfp)(void *ctxp, size_t size);
typedef void *(*ralocfp)(void *ctxp, void *memptr, size_t newsize);
typedef void (*mfreefp)(void *ctxp, void *memptr);
typedef sword pOCIEnvCreate(OCIEnv **, ub4, void *, malocfp, ralocfp, mfreefp, size_t xtramem_sz, void **usrmempp);
typedef sword pOCIHandleAlloc(const void *parenth, void **hndlpp, const ub4 type, const size_t xtramem_sz, void **usrmempp);
typedef sword pOCIHandleFree(void *hndlp, const ub4 type);
typedef sword pOCIServerAttach(OCIServer *srvhp, OCIError *errhp, const OraText *dblink, sb4 dblink_len, ub4 mode);
typedef sword pOCIServerDetach(OCIServer *srvhp, OCIError *errhp, ub4 mode);
typedef sword pOCISessionBegin(OCISvcCtx *svchp, OCIError *errhp, OCISession *usrhp, ub4 credt, ub4 mode);
typedef sword pOCISessionEnd(OCISvcCtx *svchp, OCIError *errhp, OCISession *usrhp, ub4 mode);
typedef sword pOCIAttrGet(const void *trgthndlp, ub4 trghndltyp, void *attributep, ub4 *sizep, ub4 attrtype, OCIError *errhp);
typedef sword pOCIAttrSet(void *trgthndlp, ub4 trghndltyp, void *attributep, ub4 size, ub4 attrtype, OCIError *errhp);
typedef sword pOCIParamGet(const void *hndlp, ub4 htype, OCIError *errhp, void **parmdpp, ub4 pos);
typedef sword pOCIParamSet(void *hdlp, ub4 htyp, OCIError *errhp, const void *dscp, ub4 dtyp, ub4 pos);
typedef sword pOCIStmtPrepare(OCIStmt *stmtp, OCIError *errhp, const OraText *stmt, ub4 stmt_len, ub4 language, ub4 mode);
typedef sword pOCIStmtPrepare2(OCISvcCtx *svchp, OCIStmt **stmtp, OCIError *errhp, const OraText *stmt, ub4 stmt_len, const OraText *key, ub4 key_len, ub4 language, ub4 mode);
typedef sword pOCIDefineByPos(OCIStmt *stmtp, OCIDefine **defnp, OCIError *errhp, ub4 position, void *valuep, sb4 value_sz, ub2 dty, void *indp, ub2 *rlenp, ub2 *rcodep, ub4 mode);
typedef sword pOCIDefineByPos2(OCIStmt *stmtp, OCIDefine **defnp, OCIError *errhp, ub4 position, void *valuep, sb8 value_sz, ub2 dty, void *indp, ub4 *rlenp, ub2 *rcodep, ub4 mode);
typedef sword pOCIStmtExecute(OCISvcCtx *svchp, OCIStmt *stmtp, OCIError *errhp, ub4 iters, ub4 rowoff, const OCISnapshot *snap_in, OCISnapshot *snap_out, ub4 mode);
typedef sword pOCIStmtFetch(OCIStmt *stmtp, OCIError *errhp, ub4 nrows, ub2 orientation, ub4 mode);
typedef sword pOCIStmtFetch2(OCIStmt *stmtp, OCIError *errhp, ub4 nrows, ub2 orientation, sb4 scrollOffset, ub4 mode);
typedef sword pOCIErrorGet(void *hndlp, ub4 recordno, OraText *sqlstate, sb4 *errcodep, OraText *bufp, ub4 bufsiz, ub4 type);
typedef sword pOCITransStart(OCISvcCtx *svchp, OCIError *errhp, uword timeout, ub4 flags);
typedef sword pOCITransCommit(OCISvcCtx *svchp, OCIError *errhp, ub4 flags);
typedef sword pOCITransRollback(OCISvcCtx *svchp, OCIError *errhp, ub4 flags);
typedef sword pOCITransDetach(OCISvcCtx *svchp, OCIError *errhp, ub4 flags);

/* pqsql */
typedef PGconn *pPQsetdbLogin(const char *, const char *, const char *, const char *, const char *, const char *, const char *);
typedef void pPQfinish(PGconn *conn);
typedef PGresult *pPQexec(PGconn *conn, const char *query);
typedef char *pPQcmdTuples(PGresult *res);
typedef int pPQntuples(const PGresult *res);
typedef int pPQnfields(const PGresult *res);
typedef char *pPQfname(const PGresult *res, int field_num);
typedef char *pPQgetvalue(const PGresult *res, int tup_num, int field_num);
typedef int pPQgetisnull(const PGresult *res, int tup_num, int field_num);
typedef void pPQclear(PGresult *res);
typedef ExecStatusType pPQresultStatus(const PGresult *res);
typedef char *pPQresultErrorMessage(const PGresult *res);
typedef int pPQsetClientEncoding(PGconn *conn, const char *encoding);

typedef struct _pq_lib
{
    HMODULE libpq_dll;
    pPQsetdbLogin *fnPQsetdbLogin;
    pPQfinish *fnPQfinish;
    pPQexec *fnPQexec;
    pPQcmdTuples *fnPQcmdTuples;
    pPQntuples *fnPQntuples;
    pPQnfields *fnPQnfields;
    pPQfname *fnPQfname;
    pPQgetvalue *fnPQgetvalue;
    pPQgetisnull *fnPQgetisnull;
    pPQclear *fnPQclear;
    pPQresultStatus *fnPQresultStatus;
    pPQresultErrorMessage *fnPQresultErrorMessage;
    pPQsetClientEncoding *fnPQsetClientEncoding;
} pq_lib;

typedef struct _mysql_lib
{
    HMODULE mysql_dll;
    p_mysql_init *fn_mysql_init;
    p_mysql_real_connect *fn_mysql_real_connect;
    p_mysql_set_character_set *fn_mysql_set_character_set;
    p_mysql_query *fn_mysql_query;
    p_mysql_affected_rows *fn_mysql_affected_rows;
    p_mysql_store_result *fn_mysql_store_result;
    p_mysql_num_fields *fn_mysql_num_fields;
    p_mysql_fetch_field *fn_mysql_fetch_field;
    p_mysql_fetch_row *fn_mysql_fetch_row;
    p_mysql_free_result *fn_mysql_free_result;
    p_mysql_close *fn_mysql_close;
} mysql_lib;

typedef struct _oci_lib
{
    HMODULE oci_dll;
    pOCIEnvCreate *fnOCIEnvCreate;
    pOCIHandleAlloc *fnOCIHandleAlloc;
    pOCIHandleFree *fnOCIHandleFree;
    pOCIServerAttach *fnOCIServerAttach;
    pOCIServerDetach *fnOCIServerDetach;
    pOCISessionBegin *fnOCISessionBegin;
    pOCISessionEnd *fnOCISessionEnd;
    pOCIAttrGet *fnOCIAttrGet;
    pOCIAttrSet *fnOCIAttrSet;
    pOCIParamGet *fnOCIParamGet;
    pOCIParamSet *fnOCIParamSet;
    pOCIStmtPrepare *fnOCIStmtPrepare;
    pOCIStmtPrepare2 *fnOCIStmtPrepare2;
    pOCIDefineByPos *fnOCIDefineByPos;
    pOCIDefineByPos2 *fnOCIDefineByPos2;
    pOCIStmtExecute *fnOCIStmtExecute;
    pOCIStmtFetch *fnOCIStmtFetch;
    pOCIStmtFetch2 *fnOCIStmtFetch2;
    pOCIErrorGet *fnOCIErrorGet;
    pOCITransStart *fnOCITransStart;
    pOCITransCommit *fnOCITransCommit;
    pOCITransRollback *fnOCITransRollback;
    pOCITransDetach *fnOCITransDetach;
} oci_lib;

typedef struct _db_libs
{
    mysql_lib m_mysql;
    oci_lib m_oci;
    pq_lib m_pq;
} db_libs;

typedef struct _db_config
{
    int dbport;
    char dbtype[OVEC_LEN];
    char dbhost[MAX_SIZE];
    char dbuser[ACNAME_LEN];
    char dbpass[ACNAME_LEN];
    char dbname[ACNAME_LEN];
    bool config_pass;
} db_config;

typedef struct _mysql_handle
{
    MYSQL *mysql;
} mysql_handle;

typedef struct _oci_handle
{
    OCIEnv *envhpp;
    OCIServer *servhpp;
    OCIError *errhpp;
    OCISession *usrhpp;
    OCISvcCtx *svchpp;
} oci_handle;

typedef struct _sql3_handle
{
    sqlite3 *sqlite3;
} sql3_handle;

typedef struct _pq_handle
{
    PGconn *postgres;
} pq_handle;

typedef struct _db_conn
{
    union conn_handle
    {
        mysql_handle h_mysql;
        oci_handle h_oci;
        sql3_handle h_sql3;
        pq_handle h_pq;
    } handles;
    db_config config;
    bool connected;
} db_conn;

extern db_libs db_funcs;
int on_table_update_theme(eu_tabpage *pnode);
int on_table_oci_error(eu_tabpage *, OCIError *, int *, char *, size_t);
int on_table_sql_query(eu_tabpage *pnode, const char *pq, bool vcontrol, bool clear);
int on_table_create_query_box(eu_tabpage *pnode);
bool on_table_sql_header(eu_tabpage *pnode);
bool on_table_skip_comment(eu_tabpage *pnode, char **psql);
void on_table_disconnect_database(eu_tabpage *pnode, bool force);

#ifdef __cplusplus
}
#endif

#endif  // _EU_TABLECTL_H_
