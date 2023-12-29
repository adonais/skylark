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
#define file_click_close(m) (m != FILE_AUTO_SAVE && m != FILE_SHUTDOWN && mode != FILE_REMOTE_CLOSE)
#define url_has_remote(ll) (_tcslen(ll) > URL_MIN && _tcsnicmp(ll, _T("sftp://"), URL_MIN) == 0)
#define url_has_samba(ll) (_tcslen(ll) > 2 && (ll[1] == L'\\' && ll[0] == L'\\'))
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
    int view;
    TCHAR rel_path[MAX_BUFFER];
    TCHAR bak_path[MAX_BUFFER];
    char mark_id[MAX_BUFFER];
    char fold_id[MAX_BUFFER];
}file_backup;

int on_file_new(HWND htab, eu_tabpage *psrc);
int on_file_load(eu_tabpage *pnode, const bool force);
int on_file_node_init(eu_tabpage **p, file_backup *pbak);
int on_file_open(void);
int on_file_out_open(const HWND htab, const int index, uint32_t *pid);
int on_file_drop(HDROP hdrop);
int on_file_save(eu_tabpage *pnode, const bool save_as);
int on_file_save_as(eu_tabpage *pnode);
int on_file_all_save(void);
int on_file_close(eu_tabpage **ppnode, const CLOSE_MODE mode);
int on_file_all_close(void);
int on_file_left_close(void);
int on_file_right_close(void);
int on_file_exclude_close(eu_tabpage *pnode);
int on_file_unchange_close(eu_tabpage *pnode);
int on_file_open_filename_dlg(HWND hwnd, TCHAR *file_name, int name_len);
int on_file_redirect(file_backup *pm, const size_t vsize);
int on_file_stream_upload(eu_tabpage *pnode, TCHAR *pmsg);
int on_file_load_plugins(eu_tabpage *pnode, bool route_open);
int on_file_after_open(eu_tabpage *pnode);
void on_file_before_open(eu_tabpage *pnode);
void on_file_active_condition(const HWND htab, const int index);
void on_file_update_time(eu_tabpage *pnode, const time_t m, const bool saved);
void on_file_new_eols(eu_tabpage *pnode, const int new_eol);
void on_file_new_encoding(eu_tabpage *pnode, const int new_enc);
void on_file_finish_wait(void);
void on_file_update_recent_menu(void);
void on_file_clear_recent(void);
void on_file_edit_exit(HWND hwnd);
void on_file_edit_restart(HWND hwnd, const bool admin, const bool wait);
void on_file_restore_recent(void);
void on_file_reload_current(eu_tabpage *pnode);
void on_file_auto_backup(void);
void on_file_auto_notify(void);
void on_file_filedb_update(const eu_tabpage *pnode);
void on_file_npp_write(eu_tabpage *pnode, const wchar_t *cache_path, const bool isbak, int *);
void on_file_splite_path(const TCHAR *full_path, TCHAR *pathname, TCHAR *filename, TCHAR *mainname, TCHAR *extname);
bool on_file_get_bakpath(eu_tabpage *pnode);
bool on_file_map_hex(eu_tabpage *pnode, HANDLE hfile, const size_t nbyte);
uint64_t on_file_get_avail_phys(void);

#ifdef __cplusplus
}
#endif

#endif
