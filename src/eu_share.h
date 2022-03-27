/******************************************************************************
 * This file is part of Skylark project
 * Copyright ©2021 Hua andy <hua.andy@gmail.com>

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

#ifndef _EU_SHARE_H_
#define _EU_SHARE_H_

#include <stdint.h>

#define SKYLARK_LOCK_NAME _T("_ue_handle_lock_")
#define SKYLARK_LOCK_LANG _T("_ue_lang_lock_")
#define safe_close_dll(h)                       \
    if (NULL != h)                              \
    {                                           \
        FreeLibrary(h);                         \
        h = NULL;                               \
    }

#ifdef __cplusplus
extern "C" {
#endif

extern HMODULE g_skylark_lang;

bool __stdcall share_open_file(LPCTSTR, bool, uint32_t, HANDLE *);
bool __stdcall share_envent_create_close_sem(HANDLE *phandle);
uint32_t __stdcall share_envent_wait(uint32_t milliseconds);
uint8_t* __stdcall share_map_section(HANDLE, uint64_t, size_t, bool read_only);
HWND __stdcall share_get_hwnd(void);

#ifdef __cplusplus
}
#endif

#endif  // _EU_SHARE_H_
