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

#ifndef _H_SKYLARK_FILE_
#define _H_SKYLARK_FILE_

#define URL_MIN 7
#define BUFF_SIZE (8 * 1024 * 1024)                // 8M
#define BUFF_32K (32 * 1024)                       // 32K
#define ENABLE_MMAP(x) (x > (uint64_t) 0x8000000)  //128M
#define file_click_close(m) (m != FILE_AUTO_SAVE && m != FILE_SHUTDOWN && mode != FILE_REMOTE_CLOSE)
#define url_has_remote(ll) (_tcslen(ll) > URL_MIN && _tcsnicmp(ll, _T("sftp://"), URL_MIN) == 0)
#define url_has_file(ll) (_tcslen(ll) > (URL_MIN+1) && _tcsnicmp(ll, _T("file:///"), (URL_MIN+1)) == 0)

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum _CLOSE_MODE
{
    FILE_SHUTDOWN = 0,
    FILE_ONLY_CLOSE,
    FILE_EXCLUDE_CLOSE,
    FILE_ALL_CLOSE,
    FILE_REMOTE_CLOSE,
    FILE_AUTO_SAVE
}CLOSE_MODE;

typedef struct _file_backup
{
    intptr_t x;
    intptr_t y;
    intptr_t postion;
    int tab_id;
    int cp;
    int bakcp;
    int eol;
    int blank;
    int hex;
    int focus;
    int zoom;
    int status;
    int sync;
    TCHAR rel_path[MAX_BUFFER];
    TCHAR bak_path[MAX_BUFFER];
    char mark_id[MAX_BUFFER];
    char fold_id[MAX_BUFFER];
}file_backup;

int on_file_new(void);
int on_file_load(eu_tabpage *pnode, file_backup *pbak, const bool force);
int on_file_only_open(file_backup *pbak, const bool selection);
int on_file_open(void);
int on_file_out_open(const int index, uint32_t *pid);
int on_file_drop(HDROP hdrop);
int on_file_open_remote(remotefs *pserver, file_backup *pbak, const bool selection);
int on_file_save(eu_tabpage *pnode, const bool save_as);
int on_file_save_as(eu_tabpage *pnode);
int on_file_all_save(void);
int on_file_close(eu_tabpage *pnode, CLOSE_MODE mode);
int on_file_all_close(void);
int on_file_left_close(void);
int on_file_right_close(void);
int on_file_exclude_close(eu_tabpage *pnode);
int on_file_open_filename_dlg(HWND hwnd, TCHAR *file_name, int name_len);
int on_file_redirect(HWND hwnd, file_backup *pm);
int on_file_stream_upload(eu_tabpage *pnode, TCHAR *pmsg);
void on_file_update_time(eu_tabpage *pnode, time_t m);
void on_file_new_eols(eu_tabpage *pnode, const int new_eol);
void on_file_new_encoding(eu_tabpage *pnode, const int new_enc);
void on_file_finish_wait(void);
void on_file_update_recent_menu(void);
void on_file_clear_recent(void);
void on_file_edit_exit(HWND hwnd);
void on_file_edit_restart(HWND hwnd, const bool admin);
void on_file_restore_recent(void);
void on_file_reload_current(eu_tabpage *pnode);
void on_file_auto_backup(void);
void on_file_auto_notify(void);
uint64_t on_file_get_avail_phys(void);

#ifdef __cplusplus
}
#endif

#endif
