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

#define EXEC_VERSION 7
#define SKYLARK_SQLITE_BUSY_TIMEOUT 2000
#define START_TRANSACTION(db) sqlite3_exec(db, "begin transaction;", NULL, NULL, NULL)
#define END_TRANSACTION(db)   sqlite3_exec(db, "commit transaction;", NULL, NULL, NULL)
#define RECENT_TABLE   "create table file_recent(szId INTEGER PRIMARY KEY, szName char, szPos BIGINT, szDate BIGINT, szHex SMALLINT, UNIQUE(szName));"
#define RECENT_INSERT(n1,n2) "insert or replace into "#n1 "(szName,szPos,szDate,szHex) select szName,szPos,szDate,szHex from "#n2";"
#define SESSION_TABLE  "create table skylark_session(szId INTEGER PRIMARY KEY,szTabId INTEGER,szRealPath char,"            \
                       "szBakPath char,szMark char,szFold char,szLine BIGINT,szCp INTEGER,szBakCp INTEGER,szEol SMALLINT," \
                       "szBlank SMALLINT,szHex SMALLINT,szFocus SMALLINT,szZoom SMALLINT,szStatus SMALLINT,szSync SMALLINT,UNIQUE(szRealPath));"
#define SESSION_VAULE  "(szTabId,szRealPath,szBakPath,szMark,szFold,szLine,szCp,szBakCp,szEol,szBlank,szHex,szFocus,szZoom,szStatus,szSync) "
#define SESSION_SELECT "select szTabId,szRealPath,szBakPath,szMark,szFold,szLine,szCp,szBakCp,szEol,szBlank,szHex,szFocus,szZoom,szStatus,szSync from "
#define SESSION_INSERT(n1,n2) "insert or replace into "#n1 SESSION_VAULE SESSION_SELECT#n2";"
#define RECENT_FORMAT  "create trigger delete_till_30 BEFORE INSERT ON file_recent WHEN (select count(*) from file_recent)>%d "\
                       "BEGIN "\
                       "DELETE FROM file_recent WHERE file_recent.szId IN "\
                       "(SELECT file_recent.szId FROM file_recent ORDER BY file_recent.szId limit (select count(*) -%d from file_recent )); "\
                       "END;"
#define SKYVER_FORMAT  "insert or replace into skylar_ver(szName,szVersion,szBUildId,szTrigger,szExtra) values('%s', '%s', %I64u, %d, %I64u);"


#define DO_TRIGGER(f, n)                              \
char f[MAX_BUFFER] = {0};                             \
int  n = eu_get_config()->file_recent_number;         \
n = n > 0 && n < 100 ? n : 29;                        \
_snprintf(buf, MAX_BUFFER - 1, RECENT_FORMAT, n, n)

static CRITICAL_SECTION eu_sql_cs;
static CRITICAL_SECTION_DEBUG critsect_sqlite3 =
{
    0, 0, &eu_sql_cs,
    { &critsect_sqlite3.ProcessLocksList, &critsect_sqlite3.ProcessLocksList },
      0, 0, (DWORD_PTR)0x1,
};
static CRITICAL_SECTION eu_sql_cs = { &critsect_sqlite3, -1, 0, 0, 0, 0 };

static volatile intptr_t eu_memdb = 0;

static bool
sql_format_execute(char *buf, int len, int tirgger, int64_t extra)
{
    bool ret = false;
    char *pname = eu_utf16_utf8(__ORIGINAL_NAME, NULL);
    char *pver = eu_utf16_utf8(__EU_INFO_RELEASE_VERSION, NULL);
    do
    {
        if (!pname)
        {
            break;
        }
        if (!pver)
        {
            break;
        }
        int m = snprintf(buf, len, SKYVER_FORMAT, pname, pver, on_about_build_id(), tirgger, extra);
        ret = (m > 0 && m < len);
    } while(0);
    eu_safe_free(pname);
    eu_safe_free(pver);
    return ret;
}

static int
init_sql_file(const char *sql_path, uintptr_t *pdb)
{
    int rc = 0;
    int m_table = 0;
    sqlite3 *db = NULL;
    const char *test = "SELECT szName FROM skylar_ver;";
    const char *sql[] = \
    {
        RECENT_TABLE,
        "create table file_remote(szId INTEGER PRIMARY KEY, szName char, szProtocol char, szAddress char, "
        "szPort SMALLINT, szArea SMALLINT, szUser char, szPass char, szPrivate char, szPassphrase char, UNIQUE(szName));",
        "create table find_his(szId INTEGER PRIMARY KEY, szName char, UNIQUE(szName));",
        "create table replace_his(szId INTEGER PRIMARY KEY, szName char, UNIQUE(szName));",
        "create table folder_his(szId INTEGER PRIMARY KEY, szName char, UNIQUE(szName));",
        SESSION_TABLE,
        "create table skylar_ver(szName char, szVersion char, szBUildId BIGINT, szTrigger SMALLINT, szExtra BIGINT, UNIQUE(szName));",
        SKYVER_FORMAT,
        "create trigger delete_combo1_30 BEFORE INSERT ON find_his WHEN (select count(*) from find_his)>29 "
        "BEGIN "
        "DELETE FROM find_his WHERE find_his.szId IN (SELECT find_his.szId FROM find_his ORDER BY find_his.szId limit (select count(*) -29 from find_his )); "
        "END;",
        "create trigger delete_combo2_30 BEFORE INSERT ON replace_his WHEN (select count(*) from replace_his)>29 "
        "BEGIN "
        "DELETE FROM replace_his WHERE replace_his.szId IN (SELECT replace_his.szId FROM replace_his ORDER BY replace_his.szId limit (select count(*) -29 from replace_his )); "
        "END;",
        "create trigger delete_combo3_30 BEFORE INSERT ON folder_his WHEN (select count(*) from folder_his)>29 "
        "BEGIN "
        "DELETE FROM folder_his WHERE folder_his.szId IN (SELECT folder_his.szId FROM folder_his ORDER BY folder_his.szId limit (select count(*) -29 from folder_his )); "
        "END;",
        NULL
    };
    if ((rc = sqlite3_open(sql_path, &db)) != SQLITE_OK)
    {
        goto mem_clean;
    }
    if ((rc = sqlite3_busy_timeout(db, SKYLARK_SQLITE_BUSY_TIMEOUT)) != SQLITE_OK)
    {
        goto mem_clean;
    }
    // 测试表是否存在
    if ((rc = sqlite3_exec(db, test, NULL, NULL, NULL)) != SQLITE_OK)
    {
        START_TRANSACTION(db);
        for (int i = 0; sql[i]; ++i)
        {
            if (EXEC_VERSION == i)
            {
                char buffer[ENV_LEN] = {0};
                if (sql_format_execute(buffer, ENV_LEN - 1, 0, 0))
                {
                    rc = sqlite3_exec(db, buffer, 0, 0, NULL);
                }
            }
            else
            {
                rc = sqlite3_exec(db, sql[i], 0, 0, NULL);
            }
            if (rc != SQLITE_OK)
            {
                printf("create table, sql[%d] = [%s] error: %d\n", i, sql[i], rc);
                break;
            }
        }
        END_TRANSACTION(db);
        if (!rc)
        {
            rc = SKYLARK_SQL_END;
        }
    }
mem_clean:
    if (rc != SQLITE_OK)
    {
        sqlite3_close(db);
        db = NULL;
    }
    if (pdb)
    {
        *pdb = (uintptr_t)db;
    }
    return rc;
}

/**************************************************************************************
 * 多线程并发时, 开启用户线程锁, 不使用sqlite3本身的多线程模式, 它的内部实现太繁琐
 * 所以, 尽量使用子线程调用本函数, 防止在多线程模式下一些奇怪的死锁问题
 * sql参数, 为sql查询语句
 * callback参数, 为回调函数
 * data参数, 为回调函数参数
 * 函数成功返回0, 失败返回rc错误码, 返回-1时, 数据库打开错误.
 **************************************************************************************/
int
eu_sqlite3_send(const char *sql, sql3_callback callback, void *data)
{
    int rc = SQLITE_ERROR;
    char pfile[MAX_PATH] = {0};
    TCHAR path[MAX_PATH] = {0};
    EnterCriticalSection(&eu_sql_cs);
    _sntprintf(path, MAX_PATH-1, _T("%s\\conf\\skylark_prefs.sqlite3"), eu_module_path);
    if (util_make_u8(path, pfile, MAX_PATH-1)[0])
    {
        uintptr_t db = 0;
        if ((rc = init_sql_file(pfile, &db)) == SQLITE_OK && db)
        {
            char *err = NULL;
            if ((rc = sqlite3_exec((sqlite3 *)db, sql, callback, data, &err)))
            {
                if (err)
                {
                    printf("%s failed, cause: %s\n", __FUNCTION__, err);
                    sqlite3_free(err);
                }
            }
        }
        sqlite3_close((sqlite3 *)db);
    }
    LeaveCriticalSection(&eu_sql_cs);
    return rc;
}

int
on_sql_post(const char *sql, sql3_callback callback, void *data)
{
    int rc = SQLITE_ERROR;
    TCHAR path[MAX_PATH] = {0};
    char *sql_path = NULL;
    _sntprintf(path, MAX_PATH-1, _T("%s\\conf\\skylark_prefs.sqlite3"), eu_module_path);
    if ((sql_path = eu_utf16_utf8(path, NULL)) != NULL)
    {
        uintptr_t db = 0;
        if ((rc = init_sql_file(sql_path, &db)) == SQLITE_OK && db)
        {
            char *err = NULL;
            if ((rc = sqlite3_exec((sqlite3 *)db, sql, callback, data, &err)))
            {
                if (err)
                {
                    printf("%s failed, cause: %s\n", __FUNCTION__, err);
                    sqlite3_free(err);
                }
            }
        }
        free(sql_path);
        sqlite3_close((sqlite3 *)db);
    }
    return rc;
}

static int
on_sqlite3_session_callback(void *data, int count, char **column, char **names)
{
    bool rel = false;
    bool bk = false;
    int **pv = (int **)(intptr_t)data;
    file_backup filebak = {0};
    filebak.tab_id = -1;
    for (int i = 0; i < count; ++i)
    {
        if (STRCMP(names[i], ==, "szId"))
        {
            filebak.tab_id = (short)atoi(column[i]);
        }
        else if (STRCMP(names[i], ==, "szRealPath"))
        {
            MultiByteToWideChar(CP_UTF8, 0, column[i], -1, filebak.rel_path, MAX_PATH);
        }
        else if (STRCMP(names[i], ==, "szBakPath"))
        {
            MultiByteToWideChar(CP_UTF8, 0, column[i], -1, filebak.bak_path, MAX_PATH);
        }
    }
    if (_tcslen(filebak.rel_path) > 0)
    {
        rel = eu_exist_file(filebak.rel_path);
    }
    if (_tcslen(filebak.bak_path) > 0)
    {
        bk = eu_exist_file(filebak.bak_path);
    }
    if (!(rel || bk) && filebak.tab_id != -1)
    {
        cvector_push_back(*pv, filebak.tab_id);
    }
    return 0;
}

static char *
on_sqlite3_vec_format(const int *pv)
{
    char *pret = NULL;
    if (pv)
    {
        char *pnum = NULL;
        int size = eu_int_cast(cvector_size(pv));
        int buff_len = size * LINE_MAX_LEN;
        const char *sql = "delete from skylark_session where szId in (%s);";
        if ((pnum = (char *)calloc(1, buff_len)))
        {
            int offset = 0;
            for (int i = 0; i < size; ++i)
            {
                snprintf(pnum + offset, buff_len - 1 - offset, "%d,", pv[i]);
                offset = eu_int_cast(strlen(pnum));
            }
            if ((offset = eu_int_cast(strlen(pnum))) > 1)
            {
                if (pnum[offset - 1] == ',')
                {
                    pnum[offset - 1] = 0;
                }
            }
        }
        if (pnum && strlen(pnum) > 0)
        {
            buff_len += eu_int_cast(strlen(sql));
            if ((pret = (char *)calloc(1, buff_len)))
            {
                snprintf(pret, buff_len - 1, sql, pnum);
            }
        }
        eu_safe_free(pnum);
    }
    return pret;
}

/**************************************************************************************
 * 创建内存数据库并同步session表
 **************************************************************************************/
static int
on_sqlite3_create_memdb(const char *pfile)
{
    sqlite3 *memdb = NULL;
    int rc = SQLITE_ERROR;
    if (!pfile)
    {
        return rc;
    }
    if ((rc = sqlite3_open(":memory:", &memdb)))
    {
        fprintf(stderr, "cannot open database, cause: %d", rc);
        sqlite3_close(memdb);
        return rc;
    }
    do
    {
        char sql[ENV_LEN] = {0};
        _snprintf(sql, ENV_LEN, "attach database \"%s\" as filedb;", pfile);
        DO_TRIGGER(buf, n);
        if ((rc = sqlite3_exec(memdb, SESSION_TABLE, NULL, NULL, NULL)))
        {
            fprintf(stderr, "cannot create seesion table, cause: %d\n", rc);
            break;
        }
        if ((rc = sqlite3_exec(memdb, RECENT_TABLE, NULL, NULL, NULL)))
        {
            fprintf(stderr, "cannot create recent table, cause: %d\n", rc);
            break;
        }
        if ((rc = sqlite3_exec(memdb, buf, NULL, NULL, NULL)))
        {
            printf("create trigger error: %d\n", rc);
            break;
        }
        if ((rc = sqlite3_exec(memdb, sql, NULL, NULL, NULL)))
        {
            fprintf(stderr, "attach database error: %d\n", rc);
            break;
        }
        if ((rc = sqlite3_exec(memdb, SESSION_INSERT(skylark_session, filedb.skylark_session), NULL, NULL, NULL)))
        {
            fprintf(stderr, "sql_transfer_session: %d\n", rc);
            break;
        }
        if ((rc = sqlite3_exec(memdb, RECENT_INSERT(file_recent, filedb.file_recent), NULL, NULL, NULL)))
        {
            fprintf(stderr, "sql_transfer_recent[%s]: %d\n", RECENT_INSERT(file_recent, filedb.file_recent), rc);
            break;
        }
        if ((rc = sqlite3_exec(memdb, "detach database 'filedb';", NULL, NULL, NULL)))
        {
            fprintf(stderr, "detach database error: %d\n", rc);
            break;
        }
    } while (0);
    if (!rc)
    {
        inter_atom_exchange(&eu_memdb, (intptr_t)memdb);
    }
    else
    {
        sqlite3_close(memdb);
    }
    return rc;
}

static int
on_sql_skyver_callbak(void *data, int count, char **column, char **names)
{
    UNREFERENCED_PARAMETER(count);
    UNREFERENCED_PARAMETER(names);
    *(int64_t *)data = _atoi64(column[0]);
    return 0;
}

/**************************************************************************************
 * 清理session表中已经不存在于文件系统里的记录并执行sql语句
 **************************************************************************************/
int
on_sql_do_session(const char *s, sql3_callback callback, void *data)
{
    int rc = SQLITE_ERROR;
    char pfile[MAX_PATH];
    TCHAR path[MAX_PATH] = {0};
    EnterCriticalSection(&eu_sql_cs);
    _sntprintf(path, MAX_PATH-1, _T("%s\\conf\\skylark_prefs.sqlite3"), eu_module_path);
    if (util_make_u8(path, pfile, MAX_PATH-1)[0])
    {
        char *psql = NULL;
        uintptr_t db = 0;
        cvector_vector_type(int) v = NULL;
        if ((rc = init_sql_file(pfile, &db)) == SQLITE_OK && db)
        {   // 根据配置重建触发器
            int tri = 0;
            DO_TRIGGER(buf, n);
            const char *sql = "select szid,szrealpath,szbakpath from skylark_session;";
            rc = sqlite3_exec((sqlite3 *)db, "select szTrigger from skylar_ver;", on_sql_skyver_callbak, (void *)&tri, NULL);
            if (tri != n)
            {
                char buffer[ENV_LEN] = {0};
                if (sql_format_execute(buffer, ENV_LEN - 1, n, 0))
                {
                    rc = sqlite3_exec((sqlite3 *)db, buffer, 0, 0, NULL);
                }
                sqlite3_exec((sqlite3 *)db, "DROP TRIGGER delete_till_30;", 0, 0, NULL);
                if ((rc = sqlite3_exec((sqlite3 *)db, buf, 0, 0, NULL)))
                {
                    printf("create trigger error: %d\n", rc);
                }
            }
            rc = sqlite3_exec((sqlite3 *)db, sql, on_sqlite3_session_callback, (void *)(intptr_t)&v, NULL);
        }
        if (cvector_size(v) > 0)
        {
            psql = on_sqlite3_vec_format(v);
            cvector_free(v);
        }
        if (psql && rc == SQLITE_OK)
        {
            rc = sqlite3_exec((sqlite3 *)db, psql, NULL, NULL, NULL);
        }
        if (!eu_memdb && db && rc == SQLITE_OK)
        {
            rc = on_sqlite3_create_memdb(pfile);
        }
        if (db && rc == SQLITE_OK)
        {
            rc = sqlite3_exec((sqlite3 *)db, "delete from skylark_session;", NULL, NULL, NULL);
        }
        if (s && rc == SQLITE_OK && eu_memdb)
        {
            rc = sqlite3_exec((sqlite3 *)eu_memdb, s, callback, data, NULL);
        }
        sqlite3_close((sqlite3 *)db);
        eu_safe_free(psql);
    }
    LeaveCriticalSection(&eu_sql_cs);
    return rc;
}

int
on_sql_sync_session(void)
{
    int rc = SQLITE_ERROR;
    TCHAR path[MAX_PATH] = {0};
    char *sql_path = NULL;
    EnterCriticalSection(&eu_sql_cs);
    _sntprintf(path, MAX_PATH-1, _T("%s\\conf\\skylark_prefs.sqlite3"), eu_module_path);
    if ((sql_path = eu_utf16_utf8(path, NULL)) != NULL)
    {
        uintptr_t db = 0;
        char sql[ENV_LEN] = {0};
        const char *sql_del = "delete from skylark_session where szSync=0;";
        snprintf(sql, ENV_LEN - 1, "attach database '%s' as 'filedb';", sql_path);
        rc = init_sql_file(sql_path, &db);
        if (db && eu_memdb && rc == SQLITE_OK)
        {
            rc = sqlite3_exec((sqlite3 *)eu_memdb, sql, NULL, NULL, NULL);
            if (rc == SQLITE_OK)
            {
                rc = sqlite3_exec((sqlite3 *)eu_memdb, SESSION_INSERT(filedb.skylark_session, skylark_session), NULL, NULL, NULL);
            }
            if (rc == SQLITE_OK)
            {
                rc = sqlite3_exec((sqlite3 *)db, sql_del, NULL, NULL, NULL);
            }
            if (rc == SQLITE_OK)
            {
                rc = sqlite3_exec((sqlite3 *)eu_memdb, RECENT_INSERT(filedb.file_recent, file_recent), NULL, NULL, NULL);
            }
            if (rc == SQLITE_OK)
            {
                rc = sqlite3_exec((sqlite3 *)eu_memdb, "detach database 'filedb';", NULL, NULL, NULL);
            }
        }
        sqlite3_close((sqlite3 *)db);
        sqlite3_close((sqlite3 *)eu_memdb);
        inter_atom_exchange(&eu_memdb, 0);
    }
    LeaveCriticalSection(&eu_sql_cs);
    return rc;
}

int
on_sql_mem_post(const char *sql, sql3_callback callback, void *data)
{
    if (eu_memdb && sql)
    {
        return sqlite3_exec((sqlite3 *)eu_memdb, sql, callback, data, NULL);
    }
    return SQLITE_ERROR;
}

int
on_sql_file_recent_clear(void)
{
    if (eu_memdb)
    {
        return sqlite3_exec((sqlite3 *)eu_memdb, "delete from file_recent;", NULL, NULL, NULL);
    }
    return SQLITE_ERROR;
}

void
on_sql_file_recent_thread(const file_recent *precent)
{
    uint64_t result = util_gen_tstamp();
    if (result && precent)
    {
        char pfile[MAX_PATH+1] = {0};
        char sql[MAX_BUFFER] = {0};
        snprintf(pfile, MAX_BUFFER-1, "%s", precent->path);
        eu_str_replace(pfile, MAX_PATH, "'", "''");
        const char *exp = "insert into file_recent(szName,szPos,szDate,szHex) values('%s', %I64d, %I64u, %d) "
                          "on conflict (szName) do update set szPos=%I64d,szDate=%I64u,szHex=%d;";
        snprintf(sql, MAX_BUFFER-1, exp, pfile, precent->postion, result, precent->hex, precent->postion, result, precent->hex);
        if (sqlite3_exec((sqlite3 *)eu_memdb, sql, NULL, NULL, NULL) != 0)
        {
            printf("%s failed: %s\n", __FUNCTION__, sql);
        }
    }
}

void
eu_push_find_history(const char *key)
{
    char sql[MAX_BUFFER+1] = {0};
    _snprintf(sql, MAX_BUFFER, "insert or ignore into find_his(szName) values('%s');", key);
    if (eu_sqlite3_send(sql, NULL, NULL) != 0)
    {
        printf("eu_sqlite3_send failed in %s\n", __FUNCTION__);
    }
}

void
eu_delete_find_history(const char *key)
{
    char sql[MAX_BUFFER+1] = {0};
    _snprintf(sql, MAX_BUFFER, "delete from find_his where szName='%s';", key);
    if (eu_sqlite3_send(sql, NULL, NULL) != 0)
    {
        printf("eu_sqlite3_send failed in %s\n", __FUNCTION__);
    }
}

static unsigned __stdcall
sql_search_history(void *lp)
{
    if (eu_sqlite3_send("SELECT szName FROM find_his;", (sql3_callback)lp, (void *)IDC_WHAT_FOLDER_CBO) != 0)
    {
        printf("eu_sqlite3_send failed in %s\n", __FUNCTION__);
    }
    return 0;
}

void
eu_get_find_history(sql3_callback pfunc)
{
    CloseHandle((HANDLE) _beginthreadex(NULL, 0, sql_search_history, pfunc, 0, NULL));
}

void
eu_push_replace_history(const char *key)
{
    char sql[MAX_BUFFER+1] = {0};
    _snprintf(sql, MAX_BUFFER, "insert or ignore into replace_his(szName) values('%s');", key);
    if (eu_sqlite3_send(sql, NULL, NULL) != 0)
    {
        printf("eu_sqlite3_send failed in %s\n", __FUNCTION__);
    }
}

void
eu_delete_replace_history(const char *key)
{
    char sql[MAX_BUFFER+1] = {0};
    _snprintf(sql, MAX_BUFFER, "delete from replace_his where szName='%s';", key);
    if (eu_sqlite3_send(sql, NULL, NULL) != 0)
    {
        printf("eu_sqlite3_send failed in %s\n", __FUNCTION__);
    }
}

static unsigned __stdcall
sql_replace_history(void *lp)
{
    if (eu_sqlite3_send("SELECT szName FROM replace_his;", (sql3_callback)lp, (void *)IDC_SEARCH_RP_CBO) != 0)
    {
        printf("eu_sqlite3_send failed in %s\n", __FUNCTION__);
    }
    return 0;
}

void
eu_get_replace_history(sql3_callback pfunc)
{
    CloseHandle((HANDLE) _beginthreadex(NULL, 0, sql_replace_history, pfunc, 0, NULL));
}

void
eu_push_folder_history(const char *key)
{
    char sql[MAX_BUFFER+1] = {0};
    _snprintf(sql, MAX_BUFFER, "insert or ignore into folder_his(szName) values('%s');", key);
    if (eu_sqlite3_send(sql, NULL, NULL) != 0)
    {
        printf("eu_sqlite3_send failed in %s\n", __FUNCTION__);
    }
}

void
eu_delete_folder_history(const char *key)
{
    char sql[MAX_BUFFER+1] = {0};
    _snprintf(sql, MAX_BUFFER, "delete from folder_his where szName='%s';", key);
    if (eu_sqlite3_send(sql, NULL, NULL) != 0)
    {
        printf("eu_sqlite3_send failed in %s\n", __FUNCTION__);
    }
}

static unsigned __stdcall
sql_folder_history(void *lp)
{
    if (eu_sqlite3_send("SELECT szName FROM folder_his;", (sql3_callback)lp, (void *)IDC_SEARCH_DIR_CBO) != 0)
    {
        printf("eu_sqlite3_send failed in %s\n", __FUNCTION__);
    }
    return 0;
}

void
eu_get_folder_history(sql3_callback pfunc)
{
    CloseHandle((HANDLE) _beginthreadex(NULL, 0, sql_folder_history, pfunc, 0, NULL));
}

void
eu_update_backup_table(file_backup *pbak, DB_MODE mode)
{
    char rel_path[MAX_PATH+1] = {0};
    char bak_path[MAX_PATH+1] = {0};
    char sql[MAX_BUFFER*4] = {0};
    const char *fmt = "insert into skylark_session(szTabId,szRealPath,szBakPath,szMark,szFold,szLine,szCp,szBakCp,szEol,szBlank,szHex,szFocus,szZoom,szStatus,szSync) "
                      "values(%d, '%s', '%s', '%s', '%s', %I64d, %d, %d, %d, %d, %d, %d, %d, %d, %d) on conflict (szRealPath) do update set "
                      "szTabId=%d,szBakPath='%s',szMark='%s',szFold='%s',szLine=%I64d,szCp=%d,szBakCp=%d,szEol=%d,szBlank=%d,szHex=%d,szFocus=%d,szZoom=%d,szStatus=%d,szSync=%d;";
    WideCharToMultiByte(CP_UTF8, 0, pbak->rel_path, -1, rel_path, MAX_PATH, NULL, NULL);
    WideCharToMultiByte(CP_UTF8, 0, pbak->bak_path, -1, bak_path, MAX_PATH, NULL, NULL);
    // 文件路径存在特殊符号时进行转义
    eu_str_replace(rel_path, MAX_PATH, "'", "''");
    eu_str_replace(bak_path, MAX_PATH, "'", "''");
    _snprintf(sql, MAX_BUFFER*4-1, fmt, pbak->tab_id, rel_path, bak_path, pbak->mark_id, pbak->fold_id, pbak->postion, pbak->cp, pbak->bakcp,
              pbak->eol, pbak->blank, pbak->hex, pbak->focus, pbak->zoom, pbak->status, pbak->sync, pbak->tab_id,bak_path,pbak->mark_id,pbak->fold_id,pbak->postion,
              pbak->cp, pbak->bakcp,pbak->eol, pbak->blank, pbak->hex, pbak->focus, pbak->zoom, pbak->status, pbak->sync);
    switch (mode)
    {
        case DB_ALL:
            sqlite3_exec((sqlite3 *)eu_memdb, sql, NULL, NULL, NULL);
            eu_sqlite3_send(sql, NULL, NULL);
            break;
        case DB_MEM:
            sqlite3_exec((sqlite3 *)eu_memdb, sql, NULL, NULL, NULL);
            break;
        case DB_FILE:
            eu_sqlite3_send(sql, NULL, NULL);
            break;
        default:
            break;
    }
}

void
eu_clear_backup_table(void)
{
    if (eu_sqlite3_send("delete from skylark_session;", NULL, NULL) != 0)
    {
        printf("eu_sqlite3_send failed in %s\n", __FUNCTION__);
    }
}

static unsigned __stdcall
on_sql_execute_thread(void *lp)
{
    int rc = 0;
    char sql[MAX_BUFFER+1] = {0};
    _snprintf(sql, MAX_BUFFER, "delete from skylark_session where szRealPath='%s';", (char *)lp);
    if (eu_memdb && (rc = sqlite3_exec((sqlite3 *)eu_memdb, sql, NULL, NULL, NULL)))
    {
        printf("on_sql_execute_thread failed in %s\n", __FUNCTION__);
    }
    free(lp);
    return rc;
}

void
on_sql_delete_backup_row(eu_tabpage *pnode)
{
    if (pnode && pnode->pathfile[0])
    {
        char *path = eu_utf16_utf8(pnode->pathfile, NULL);
        if (path)
        {
            on_sql_execute_thread(path);
        }
    }
}

void
on_sql_delete_backup_row_thread(eu_tabpage *pnode)
{
    if (pnode && pnode->pathfile[0])
    {
        char *path = eu_utf16_utf8(pnode->pathfile, NULL);
        if (path)
        {
            CloseHandle((HANDLE)_beginthreadex(NULL, 0, on_sql_execute_thread, (void *)path, 0, NULL));
        }
    }
}

/**************************************************************************************
 * 导出sqlite几个常用函数到euapi
 **************************************************************************************/
int
eu_sqlite3_open(const char *filename, sqlite3 **ppdb)
{
    return sqlite3_open(filename, ppdb);
}

int
eu_sqlite3_exec(sqlite3* db,  const char *sql, sql3_callback pfunc, void *para, char **errmsg)
{
    return sqlite3_exec(db,  sql, pfunc, para, errmsg);
}

int
eu_sqlite3_get_table(sqlite3 *db,const char *psql,char ***presult,int *prow,int *pcolumn,char **pzmsg)
{
    return sqlite3_get_table(db, psql, presult, prow, pcolumn, pzmsg);
}

void
eu_sqlite3_free_table(char **result)
{
    sqlite3_free_table(result);
}

void
eu_sqlite3_free(void *point)
{
    sqlite3_free(point);
}

int
eu_sqlite3_close(sqlite3 * db)
{
    return sqlite3_close(db);
}
