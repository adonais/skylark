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

#ifndef _EU_UPDATECHECKER_H_
#define _EU_UPDATECHECKER_H_

#define UPCHECK_INDENT_MAIN  1
#define UPCHECK_INDENT_ABOUT 2
#define EU_UPGRADE_OK        2

// c++ compiler
#ifdef __cplusplus
extern "C" {
#endif

void on_update_check(const int ident);
void on_update_thread_wait(void);
void on_update_sql(void);
long on_update_thread_id(void);
bool on_update_do(void);

#ifdef __cplusplus
}
#endif

#endif // _EU_UPDATECHECKER_H_
