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

#ifndef _H_SKYLARK_FILE_
#define _H_SKYLARK_FILE_

#define BUFF_SIZE (8 * 1024 * 1024)                // 8M
#define BUFF_32K (32 * 1024)                       // 32K
#define ENABLE_MMAP(x) (x > (uint64_t) 0x8000000)  //128M

#ifdef __cplusplus
extern "C"
{
#endif

#define WM_BACKUP_OVER (WM_USER+10001)
#define WM_SYSLIST_OVER (WM_USER+10002)
#define url_has_remote(ll) (_tcsnicmp(ll, _T("sftp://"), 7) == 0)
#define safe_close_handle(h)                    \
    if (NULL != h && INVALID_HANDLE_VALUE != h) \
    {                                           \
        CloseHandle(h);                         \
        h = NULL;                               \
    }

typedef enum _CLOSE_MODE
{
    FILE_SHUTDOWN = 0,
    FILE_ONLY_CLOSE,
    FILE_EXCLUDE_CLOSE,
    FILE_ALL_CLOSE
}CLOSE_MODE;

typedef struct _file_backup
{
    TCHAR rel_path[MAX_PATH];
    TCHAR bak_path[MAX_PATH];
    char mark_id[MAX_BUFFER];
    size_t lineno;
    int tab_id;
    int cp;
    int bakcp;
    int eol;
    int blank;
    int hex;
    int focus;
    int zoom;
    int status;
}file_backup;

extern HANDLE hwnd_backup;

int on_file_new(void);
int on_file_to_tab(eu_tabpage *pnode, file_backup *pbak, bool force);
int on_file_only_open(file_backup *pbak, bool selection);
int on_file_open(void);
int on_file_drop(HDROP hdrop);
int on_file_open_remote(remotefs *pserver, file_backup *pbak, bool selection);
int on_file_save(eu_tabpage *pnode, bool save_as);
int on_file_save_as(eu_tabpage *pnode);
int on_file_all_save(void);
int on_file_close(eu_tabpage *pnode, CLOSE_MODE mode);
int on_file_all_close(void);
int on_file_exclude_close(eu_tabpage *pnode);
int on_file_opendlg_filename(HWND hwnd, TCHAR *file_name, int name_len);
int on_file_redirect(HWND hwnd, file_backup *pm);
void on_file_backup_menu(void);
void on_file_session_menu(void);
void on_file_new_eols(eu_tabpage *pnode, int new_eol);
void on_file_new_encoding(eu_tabpage *pnode, int new_enc);
void on_file_finish_wait(void);
void on_file_update_recent(void);
void on_file_clear_recent(void);
void on_file_edit_exit(HWND hwnd);
void on_file_edit_restart(HWND hwnd);
void on_file_restore_recent(void);
void on_file_close_last_tab(void);
void on_file_splite_path(const TCHAR *full_path, TCHAR *dri_name, TCHAR *pathname, TCHAR *filename, TCHAR *mainname, TCHAR *extname);
uint64_t __stdcall on_file_get_avail_phys(void);

#ifdef __cplusplus
}
#endif

#endif
