/*******************************************************************************
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

#ifndef _H_SKYLARK_FILETREEBAR_
#define _H_SKYLARK_FILETREEBAR_

#define CONFIG_KEY_MATERIAL_FILETREE "EU_FILETR"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _tree_data
{
    remotefs *server;
    HTREEITEM hti;
    TCHAR *pathname;
    TCHAR *filepath;
    TCHAR *filename;
    TCHAR *tagdesc;
    int img_index;
    bool is_load;
}tree_data;

typedef struct _remotefb
{
    char* pbuf;
    size_t size;
}remotefb;

typedef enum _IMG_MOUNT
{
    IMG_FAVORITES = 0,
    IMG_CLOUD,
    IMG_DRIVE,
    IMG_FOLD,
    IMG_CLOSE,
    IMG_TEXT,
    IMG_BULB,
    IMG_SHORTCUT,
    IMG_DBMAX
} IMG_MOUNT;

extern HWND g_treebar;
extern HWND g_filetree;

int  on_treebar_tab_height(void);
int  on_treebar_load_remote(HWND hwnd, remotefs *pserver);
int  on_treebar_locate_path(const TCHAR *pathname);
int  on_treebar_create_box(HWND hwnd);
int  on_treebar_create_dlg(HWND hwnd);
int  on_treebar_refresh_node(HTREEITEM hti_parent);
bool on_treebar_variable_initialized(HWND *pd);
void on_treebar_size(const RECT *prc);
void on_treebar_wait_hwnd(void);
void on_treebar_update_addr(remotefs *pserver);
void on_treebar_data_destoy(tree_data **ptvd);
void on_treebar_update_theme(void);
void on_treebar_adjust_box(const RECT *prc, RECT *ptf);
HTREEITEM on_treebar_get_path(tree_data **ptvd);
tree_data *on_treebar_get_treeview(HTREEITEM hti);
tree_data *on_treebar_add_favorite(HTREEITEM parent, const int index, void *pdata);

#ifdef __cplusplus
}
#endif

#endif
