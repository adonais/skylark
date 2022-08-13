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

#ifndef _H_AUTO_COMPLETE_
#define _H_AUTO_COMPLETE_

typedef enum _auto_state
{
    AUTO_NONE = 0,
    AUTO_CODE,
    AUTO_DONE
}auto_state;

typedef struct _auto_postion
{
	intptr_t min;
	intptr_t max;
}auto_postion;

typedef struct _complete_t
{
    int index;
    char value[ACNAME_LEN];
    char word[MAX_ACCELS];
    auto_postion pos[OVEC_LEN];
}complete_t;

#ifdef __cplusplus
extern "C"
{
#endif

bool on_complete_snippet(eu_tabpage *pnode);
bool on_complete_snippet_back(eu_tabpage *pnode);
void on_complete_delay_snippet(void);
void on_complete_set_word(eu_tabpage *pnode);
void on_complete_unset_word(eu_tabpage *pnode);
void on_complete_reset_focus(void);
void on_complete_doc(eu_tabpage *pnode, ptr_notify lpnotify);
void on_complete_html(eu_tabpage *pnode, ptr_notify lpnotify);
const char *on_complete_get_func(eu_tabpage *pnode, const char *key);

#ifdef __cplusplus
}
#endif

#endif  /* End _H_AUTO_COMPLETE_ */
