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

#ifndef _EU_SESSION_H_
#define _EU_SESSION_H_

#define EU_SESSION_INTERVAL 400

typedef enum _SESSION_STATUS
{
    SESSION_THEME = 0,
    SESSION_CONFIG,
    SESSION_BOTH,
    SESSION_ALL
} SESSION_STATUS;

// c++ compiler
#ifdef __cplusplus
extern "C" {
#endif

void     on_session_run(const int indent);
void     on_session_cancel(void);
bool     on_session_check(void);
uint32_t on_session_thread_id(void);

#ifdef __cplusplus
}
#endif

#endif // _SESSION_H_
