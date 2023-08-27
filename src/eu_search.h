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

#ifndef _H_SKYLARK_SEARCH_
#define _H_SKYLARK_SEARCH_

#define SELECTION_ZOOM_LEVEEL -99

#define ON_LOOP_FLAGS          0x00000010
#define ON_HEX_STRINGS         0x00000020
#define NO_REGXP_FLAGS         0x00000040

#define INCLUDE_FOLDER_SUB     0x00000100
#define INCLUDE_FOLDER_HIDDEN  0x00000200
#define INCLUDE_FILE_UTF8      0x00000400
#define INCLUDE_CURRENT_FOLDER 0x00000800

#define ON_OTHER_PAGE          0x00001000
#define ON_REPLACE_THIS        0x00002000
#define ON_REPLACE_ALL         0x00004000
#define ON_REPLACE_SELECTION   0x00008000

#ifdef __cplusplus
extern "C"
{
#endif

struct navigate_trace
{
    eu_tabpage *pnode;
    int64_t last_pos;
    struct list_head ng_node;
};

typedef enum _replace_event
{
    FULL_HALF = 0,
    HALF_FULL,
    TAB_SPACE,
    SPACE_TAB
}replace_event;

typedef struct _btn_state
{
    int id;
    uint32_t mask;
}btn_state;

typedef struct _file_trace
{
    TCHAR path[MAX_BUFFER];
    int count;
    struct list_head node_file;
}file_trace;

typedef struct _folder_trace
{
    TCHAR dir[MAX_BUFFER];
    struct list_head node_folder;
}folder_trace;

int on_search_find_thread(eu_tabpage *pnode);
int on_search_find_pre(eu_tabpage *pnode);
int on_search_find_next(eu_tabpage *pnode);
int on_search_replace_thread(eu_tabpage *pnode);
int on_search_file_thread(const TCHAR *path);
int on_search_jmp_premark_all(eu_tabpage *pnode);
int on_search_jmp_next_mark_all(eu_tabpage *pnode);
int on_search_update_navigate_list(eu_tabpage *pnode, int64_t pos);
int on_search_back_navigate_this(const eu_tabpage *pnode);
int on_search_back_navigate_all(void);
int on_search_add_navigate_list(eu_tabpage *pnode, int64_t pos);
int on_search_combo_callback(void *data, int count, char **column, char **names);

sptr_t on_search_process_find(eu_tabpage *, const char *, size_t, size_t, size_t);
bool on_search_create_box(void);

void on_search_toggle_mark(eu_tabpage *pnode, sptr_t lineno);
void on_search_remove_marks_all(eu_tabpage *pnode);
void on_search_jmp_premark_this(eu_tabpage *pnode, const int mask);
void on_search_jmp_next_mark_this(eu_tabpage *pnode, const int mask);
void on_search_jmp_previous_history(eu_tabpage *pnode);
void on_search_jmp_next_history(eu_tabpage *pnode);
void on_search_jmp_specified_line(eu_tabpage *pnode);
void on_search_move_to_lgroup(eu_tabpage *pnode);
void on_search_move_to_rgroup(eu_tabpage *pnode);
void on_search_move_to_lword(eu_tabpage *pnode);
void on_search_move_to_rword(eu_tabpage *pnode);
void on_search_move_to_top_block(eu_tabpage *pnode);
void on_search_move_to_bottom_block(eu_tabpage *pnode);
void on_search_select_all(eu_tabpage *pnode);
void on_search_select_word(eu_tabpage *pnode);
void on_search_select_line(eu_tabpage *pnode);
void on_search_select_se(eu_tabpage *pnode, uint16_t id);
void on_search_select_left_word(eu_tabpage *pnode);
void on_search_select_right_word(eu_tabpage *pnode);
void on_search_select_left_group(eu_tabpage *pnode);
void on_search_select_right_group(eu_tabpage *pnode);
void on_search_cumulative_previous_block(eu_tabpage *pnode);
void on_search_cumulative_next_block(eu_tabpage *pnode);
void on_search_jmp_home(eu_tabpage *pnode);
void on_search_jmp_end(eu_tabpage *pnode);
void on_search_jmp_pos(eu_tabpage *pnode);
void on_search_jmp_line(eu_tabpage *pnode, sptr_t goto_num, sptr_t current_num);
void on_search_clean_navigate_this(eu_tabpage *pnode);
void on_search_select_matching_all(eu_tabpage *pnode);
void on_search_page_mark(eu_tabpage *pnode, char *, int);
void on_search_fold_kept(eu_tabpage *pnode, char *szfold, int size);
void on_search_update_mark(eu_tabpage *pnode, char *szmark);
void on_search_update_fold(eu_tabpage *pnode, char *szfold);
void on_search_set_selection(eu_tabpage *pnode);
void on_search_set_rectangle(eu_tabpage *pnode);
void on_search_finish_wait(void);
void on_search_dark_mode_release(void);
void on_search_repalce_event(eu_tabpage *p, replace_event docase);
void on_search_jmp_matching_brace(eu_tabpage *pnode, int *pres);
void on_search_turn_select(eu_tabpage *pnode);
void on_search_regxp_error(void);
void
on_search_jmp_next_history(eu_tabpage *pnode);

#ifdef __cplusplus
}
#endif

#endif
