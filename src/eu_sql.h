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

#ifndef _EU_SQL_H_
#define _EU_SQL_H_

#define LINE_MAX_LEN 9
#define PATH_MAX_RECENTLY 30

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum _DB_MODE
{
    DB_ALL = 0,
    DB_MEM,
    DB_FILE
}DB_MODE;

//
void on_sql_delete_backup_row(eu_tabpage *pnode);
void on_sql_delete_backup_row_thread(eu_tabpage *pnode);
int  on_sqlite3_sync_session(void);
int  on_sqlite3_do_session(const char *s, sql3_callback callback, void *data);
int  on_sqlite3_post(const char *sql, sql3_callback callback, void *data);

#ifdef __cplusplus
}
#endif

#endif  // _EU_SQL_H_
