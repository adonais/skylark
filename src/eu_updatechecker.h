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

#ifndef _EU_UPDATECHECKER_H_
#define _EU_UPDATECHECKER_H_

// 启动时运行标识符
#define UPCHECK_INDENT_MAIN  1
// 菜单点击时运行标识符
#define UPCHECK_INDENT_ABOUT 2

// c++ compiler
#ifdef __cplusplus
extern "C" {
#endif

void on_update_run(const int indent);
void on_update_sql(void);
void on_update_cancel(void);
bool on_update_check(void);
bool on_update_excute(void);

#ifdef __cplusplus
}
#endif

#endif // _EU_UPDATECHECKER_H_
