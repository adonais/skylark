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

#ifndef _EU_CHANGLES_H_
#define _EU_CHANGLES_H_

#ifdef __cplusplus
extern "C" {
#endif

enum
{
    FILE_CHANGE_FILE_YES = 1,
    FILE_CHANGE_FILE_NO,
    FILE_CHANGE_ALL_YES,
    FILE_CHANGE_ALL_NO ,
    FILE_CHANGE_SEC_YES,
    FILE_CHANGE_SEC_NO
};

void on_changes_window(HWND hwnd);
int eu_i18n_msgbox(HWND hwnd, uint16_t contents_id, uint16_t title_id, uint32_t type);

#ifdef __cplusplus
}
#endif

#endif  // _EU_CHANGLES_H_
