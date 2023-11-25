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

#ifndef _H_SKYLARK_STYLETHEME_
#define _H_SKYLARK_STYLETHEME_

typedef struct _theme_desc
{
    TCHAR desc[QW_SIZE];
    TCHAR name[QW_SIZE];
}theme_desc;

typedef struct _theme_query
{
    uint16_t res_id;
    TCHAR desc[QW_SIZE];
    TCHAR name[QW_SIZE];
}theme_query;

typedef enum _control_id
{
    all_id = 0,
    tabbar_id,
    filebar_id,
    filetree_id,
    btn_id
} control_id;

#ifdef __cplusplus
extern "C" {
#endif

TCHAR* on_theme_query_name(TCHAR *str);
HFONT  on_theme_font_hwnd(void);

int  on_theme_copy_style(TCHAR *ac_theme);
int  on_theme_load_script(const TCHAR *name);
void on_theme_update_item(void);
void on_theme_update_font(const control_id id);
bool on_theme_setup_font(HWND hwnd);
bool on_theme_create_dlg(void);

#ifdef __cplusplus
}
#endif

#endif
