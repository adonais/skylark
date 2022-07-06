/*******************************************************************************
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

#ifndef _EU_MENU_H_
#define _EU_MENU_H_

#define THEME_MENU 6
#define THEME_MENU_SUB 7
#define LOCALE_MENU 6
#define LOCALE_MENU_SUB 5
#define TAB_MENU_SUB 27

#ifdef __cplusplus
extern "C" {
#endif

void menu_switch_theme(void);
void menu_update_item(HMENU menu);
HMENU menu_load(uint16_t mid);
int menu_height();
int menu_pop_track(HWND hwnd, uint16_t mid, int64_t lparam);

#ifdef __cplusplus
}
#endif

#endif  // _EU_MENU_H_
