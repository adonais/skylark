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

#ifndef _EU_SETTINGS_H_
#define _EU_SETTINGS_H_

#ifdef __cplusplus
extern "C" {
#endif

HBITMAP on_setting_app_icon(void);
HBITMAP on_setting_customize_icon(void);
HBITMAP on_setting_lua_icon(const int resid);
void    on_setting_update_menu(const HMENU setting_menu);
void    on_setting_execute(HWND hwnd, const int wm_id);
void    on_setting_manager(void);

#ifdef __cplusplus
}
#endif

#endif  // _EU_SETTINGS_H_
