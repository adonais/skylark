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

#ifndef _EU_MENU_H_
#define _EU_MENU_H_

/* 子菜单所在位置 */
#define THEME_MENU            6
#define THEME_MENU_SUB        7
#define LOCALE_MENU           6
#define LOCALE_MENU_SUB       5

/* 子菜单弹出菜单位置 */
#define TAB_MENU_PANELS_SUB   4
#define TAB_MENU_TITLEBAR_SUB 5
#define TAB_MENU_TOOLBAR_SUB  6
#define TAB_MENU_HILIGHT_SUB  13
#define TAB_DISPLAY_SUB       16
#define TAB_HISTORY_SUB       17
#define TAB_MENU_ACTIVE_SUB   19
#define TAB_MENU_CLOSE_SUB    20
#define TAB_MENU_NEW_SUB      21
#define TAB_MENU_CBUTTON_SUB  22
#define TAB_MENU_OTHER        23
#define PROGRAM_SNIPPET_SUB   15
#define CUSTOMIZED_CMD_SUB    14

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ptr_menu_callback)(HMENU hpop, void *p);
int  menu_height(void);
int  menu_pop_track(const HWND hwnd, const uint16_t mid, LPARAM lparam, const uint32_t, ptr_menu_callback fn, void *param);
void menu_switch_theme(void);
void menu_destroy(const HWND hwnd);
void menu_bmp_destroy(void);
void menu_update_item(const HMENU menu, const bool init);
void menu_setup(HWND hwnd);
HMENU menu_load(const uint16_t mid);

#ifdef __cplusplus
}
#endif

#endif  // _EU_MENU_H_
