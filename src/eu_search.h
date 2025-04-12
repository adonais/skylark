/*******************************************************************************
 * This file is part of Skylark project
 * Copyright ©2025 Hua andy <hua.andy@gmail.com>

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
#define LINE_NOT_FOUND ((sptr_t)-1)

#define INCLUDE_FOLDER_SUB     0x00000100
#define INCLUDE_FOLDER_HIDDEN  0x00000200
#define INCLUDE_FILE_UTF8      0x00000400
#define INCLUDE_CURRENT_FOLDER 0x00000800

#define ON_OTHER_PAGE          0x00001000
#define ON_REPLACE_THIS        0x00002000
#define ON_REPLACE_ALL         0x00004000
#define ON_REPLACE_SELECTION   0x00008000

#define SCCMD_LOOP             0x00000010
#define SCCMD_HEX              0x00000020
#define SCCMD_NOREGXP          0x00000040
#define SCCMD_REVERSE          0x00000080
#define SCCMD_LINE_FIRST       0x00010000
#define SCCMD_LINE_ALL         0x00020000
#define SCCMD_TEXT_FIRST       0x00040000
#define SCCMD_TEXT_ALL         0x00080000

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum _replace_event
{
    FULL_HALF = 0,
    HALF_FULL,
    TAB_SPACE,
    SPACE_TAB,
    RE_REGXP
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
int on_search_jmp_premark_all(const eu_tabpage *pnode);
int on_search_jmp_next_mark_all(const eu_tabpage *pnode);
int on_search_combo_callback(void *data, int count, char **column, char **names);

sptr_t on_search_marker_next(const eu_tabpage *pnode, const sptr_t line, sptr_t last, const int bitmask);
sptr_t on_search_process_find(const eu_tabpage *, const char *, size_t, size_t, size_t);
bool on_search_create_box(void);
bool on_search_caller(void);
bool on_search_replace_target(eu_tabpage *pnode, const char *replace_str, sptr_t *poffset);
bool on_search_next(eu_tabpage *pnode, const char *key, const sptr_t end_pos, const uint32_t flags);

void on_search_set_fsm(void);
void on_search_clear_fsm(void);
void on_search_toggle_mark(const eu_tabpage *pnode, const sptr_t lineno);
void on_search_remove_marks_all(void);
void on_search_jmp_premark_this(const eu_tabpage *pnode, const int mask);
void on_search_jmp_next_mark_this(const eu_tabpage *pnode, const int mask);
void on_search_jmp_previous_history(const eu_tabpage *pnode);
void on_search_jmp_next_history(const eu_tabpage *pnode);
void on_search_jmp_specified_line(const eu_tabpage *pnode);
void on_search_move_to_lgroup(const eu_tabpage *pnode);
void on_search_move_to_rgroup(const eu_tabpage *pnode);
void on_search_move_to_lword(const eu_tabpage *pnode);
void on_search_move_to_rword(const eu_tabpage *pnode);
void on_search_move_to_top_block(const eu_tabpage *pnode);
void on_search_move_to_bottom_block(const eu_tabpage *pnode);
void on_search_select_all(const eu_tabpage *pnode);
void on_search_select_word(const eu_tabpage *pnode);
void on_search_select_line(const eu_tabpage *pnode);
void on_search_select_se(const eu_tabpage *pnode, uint16_t id);
void on_search_select_left_word(const eu_tabpage *pnode);
void on_search_select_right_word(const eu_tabpage *pnode);
void on_search_select_left_group(const eu_tabpage *pnode);
void on_search_select_right_group(const eu_tabpage *pnode);
void on_search_cumulative_previous_block(const eu_tabpage *pnode);
void on_search_cumulative_next_block(const eu_tabpage *pnode);
void on_search_jmp_home(const eu_tabpage *pnode);
void on_search_jmp_end(const eu_tabpage *pnode);
void on_search_jmp_pos(const eu_tabpage *pnode);
void on_search_jmp_line(const eu_tabpage *pnode, const sptr_t goto_num, const sptr_t current_num);
void on_search_select_matching_all(eu_tabpage *pnode);
void on_search_page_mark(const eu_tabpage *pnode, char *, int);
void on_search_fold_kept(const eu_tabpage *pnode, char *szfold, int size);
void on_search_update_mark(const eu_tabpage *pnode, char *szmark);
void on_search_update_fold(const eu_tabpage *pnode, char *szfold);
void on_search_set_selection(eu_tabpage *pnode);
void on_search_set_rectangle(eu_tabpage *pnode);
void on_search_finish_wait(void);
void on_search_dark_mode_release(void);
void on_search_repalce_event(eu_tabpage *p, replace_event docase);
void on_search_jmp_matching_brace(eu_tabpage *pnode, int *pres);
void on_search_turn_select(eu_tabpage *pnode);
void on_search_regxp_error(void);
void on_search_jmp_next_history(const eu_tabpage *pnode);
void on_search_do_space(const eu_tabpage *pnode, const char *key, const char *str_replace, replace_event docase);

#ifdef __cplusplus
}
#endif

#endif
