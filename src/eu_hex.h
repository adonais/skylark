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

#ifndef _H_SKYLARK_HEX_
#define _H_SKYLARK_HEX_

#define NUMBEROF_CHARS_IN_FIRST_COLUMN_64BIT 16 + 2
#define NUMBEROF_CHARS_IN_FIRST_COLUMN_32BIT 8 + 2
#define NUMBEROF_CHARS_IN_SECOND_COLUMN 16 * 3 + 1
#define NUMBEROF_CHARS_IN_THIRD_COLUMN 16
//
// mask of an item
//

#define HVIF_ADDRESS 0x0001
#define HVIF_BYTE 0x0002

//
// state of an item
//

#define HVIS_MODIFIED 0x0001

//
// Extended styles
//

#define HVS_ADDRESS64 0x0001
#define HVS_READONLY 0x0002

//
// States of the control
//

#define HVF_FONTCREATED 0x0001
#define HVF_CARETVISIBLE 0x0002
#define HVF_SELECTED 0x0004

enum HEX_COLUMN
{
    COLUMN_DATA = 1,            // 在16进制区域
    COLUMN_VALUE                // 在文本字符区域
};

typedef struct _HEXVIEW
{
    int line1;                  // 16进制区域开始像素
    int line2;                  // 文本区域开始像素

    int visiblelines;           // 显示区域可见行
    int visiblechars;           // 显示区域可见字符数
    int longest_line;           // 行长度

    int hscroll_pos;            // 水平滚动条位置
    int hscroll_max;

    int height_char;            // 字符高
    int width_char;             // 字符宽
    int height_view;            // 可见区域高
    int width_view;             // 可见区域宽
    int active_column;          // COLUMN_DATA or COLUMN_VALUE

    uint32_t hl_position;       // 0 = HINIBBLE(上半字节); 1 = LONIBBLE(下半字节)
    uint32_t ct_flags;          // 光标状态掩码
    uint32_t ex_style;          // 地址栏掩码等...

    intptr_t vscroll_pos;       // 垂直滚动条位置, 即光标所在行
    intptr_t vscroll_max;
    intptr_t totallines;        // 十六进制行数

    size_t total_items;         // 总的字节数
    size_t number_items;        // 当前光标所在位置, 跳转时必须先设置此值
    size_t select_start;        // 光标所在字符起始地址
    size_t select_end;          // 光标所在字符结束地址

    HFONT hfont;                // 字体句柄
    HANDLE hmap;                // 二进制文件内存映射句柄
    uint8_t *pbase;             // 文件映射基地址或内存地址, 由hmap是否为NULL决定
    intptr_t *hex_point;        // 保留, 以后用来保存原始文件增加的偏移量
    bool hex_ascii;             // 原始二进制, 非转码后的utf8

    COLORREF clr_text;          // 文本字体颜色
    COLORREF clr_bg_text;       // 文本背景色
    COLORREF clr_bg_selected;   // 选中文本背景色
    COLORREF clr_modify_text;   // 修改后文本色
} HEXVIEW, *PHEXVIEW;

typedef struct _HVITEM
{
    uint32_t mask;
    uint32_t state;
    uint64_t address;
    size_t number_items;
    uint8_t value;
} HVITEM, *PHVITEM;

typedef struct _NMHVDISPINFO
{
    NMHDR nmhdr;
    HVITEM item;
} NMHVDISPINFO, *PNMHVDISPINFO;

typedef struct _NMHEXVIEW
{
    NMHDR nmhdr;
    HVITEM item;
} NMHEXVIEW, *PNMHEXVIEW;

typedef struct _BYTERANGE
{
    size_t min;
    size_t max;
} BYTERANGE, *PBYTERANGE;

#ifdef __cplusplus
extern "C"
{
#endif

int hexview_switch_mode(eu_tabpage *pnode);
int hexview_save_data(eu_tabpage *pnode, const TCHAR *bakfile);
int hexview_update_theme(eu_tabpage *pnode);
void hexview_updata(intptr_t *p, intptr_t m);
void hexview_destoy(eu_tabpage *pnode);
void hexview_set_area(int value);
void hexview_switch_item(eu_tabpage *pnode);
bool hexview_init(eu_tabpage *pnode);
uint8_t *hexview_strdup_data(eu_tabpage *pnode, size_t *out);

#ifdef __cplusplus
}
#endif

#endif // _H_SKYLARK_HEX_
