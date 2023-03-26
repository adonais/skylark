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

#ifndef _H_SKYLARK_EXPORTER_
#define _H_SKYLARK_EXPORTER_

#define CF_RTF TEXT("Rich Text Format")
// size definitions for memory allocation
// header + fonttbl + fonttbl end + colortbl + colortbl end + default style + eof
#define EXPORT_SIZE_RTF_STATIC (33 + 11 + 5 + 12 + 5 + 22 + 3)
//font decl + color decl fg + color decl bg
#define EXPORT_SIZE_RTF_STYLE (11 + 27 + 27)
// '\f127\fs56\highlight254\cf255\b0\i0\ul0 '
#define EXPORT_SIZE_RTF_SWITCH (39)

#define SCINTILLA_FONTNAME_SIZE  (DW_SIZE + 1)
#define NRSTYLES                 (STYLE_MAX + 1)

typedef struct _style_export
{
    int ifont;
    int size;
    int bold;
    int italic;
    int underlined;
    int fgcolor;
    int bgcolor;
    int fg_clr;
    int bg_clr;
    bool eol_ext;
    char fontname[DW_SIZE];
} style_export;

typedef struct _sci_export
{
    int tabsize;
    int nr_ustyles;
    int nrstyle_switches;
    int font_length;
    int twips;
    long nschar;
    bool ustyles[NRSTYLES];
    char* data;
    style_export *pstyle;
} sci_export;

typedef union _utf16struct
{
    short value;
    struct
    {
        char byte1;
        char byte2;
    } bytes;
} utf16;

#ifdef __cplusplus
extern "C" {
#endif

char* on_exporter_rtf(eu_tabpage *pnode, const sptr_t start, const sptr_t end);

#ifdef __cplusplus
}
#endif

#endif  // _H_SKYLARK_EXPORTER_
