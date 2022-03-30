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

#ifndef _H_ABOUT_UI_
#define _H_ABOUT_UI_

// c++ compiler
#ifdef __cplusplus
extern "C" {
#endif

bool on_about_dialog(void);
bool on_about_donation(void);
uint64_t on_about_build_id(void);

#ifdef __cplusplus
}
#endif

#endif  //_H_ABOUT_UI_
