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

#ifndef _EU_OPENAI_H_
#define _EU_OPENAI_H_

#define AI_EOT       (-255)
#define AI_CMD_MODEL (1)
#define THINK_INIT   0x00000001
#define THINK_OUTPUT 0x00000002
#define THINK_ENDDO  0x00000004

// c++ compiler
#ifdef __cplusplus
extern "C" {
#endif

bool on_openai_check(eu_tabpage *presult);
void on_openai_cancel(eu_tabpage *presult);
void on_openai_run(const char *str, const HWND hwnd);

#ifdef __cplusplus
}
#endif

#endif  // _EU_OPENAI_H_
