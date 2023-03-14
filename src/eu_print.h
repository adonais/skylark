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

#ifndef _EU_PRINT_H_
#define _EU_PRINT_H_

#ifndef DLGTEMPLATEEX

#ifdef _MSC_VER
#pragma pack(push, 1)
#endif

typedef struct _DLGTEMPLATEEX
{
    WORD    dlgVer;
    WORD    signature;
    DWORD    helpID;
    DWORD    exStyle;
    DWORD    style;
    WORD    cDlgItems;
    short    x;
    short    y;
    short    cx;
    short    cy;
} DLGTEMPLATEEX;

#ifdef _MSC_VER
#pragma pack(pop)
#endif

#endif // DLGTEMPLATEEX

// c++ compiler
#ifdef __cplusplus
extern "C" {
#endif

void on_print_setup(HWND hwnd);
int on_print_file(eu_tabpage *pnode);

#ifdef __cplusplus
}
#endif

#endif  // _EU_PRINT_H_
