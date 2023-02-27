/*******************************************************************************
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

#ifndef _EU_PIXMAP_H_
#define _EU_PIXMAP_H_

// c++ compiler
#ifdef __cplusplus
extern "C" {
#endif

HBITMAP on_pixmap_from_svg(const char *buf, const int w, const int h, const char *color);
HBITMAP on_pixmap_icons(const int w, const int h, const char *color, int *pout);
const int on_pixmap_svg_count(void);
const int on_pixmap_vec_count(void);

#ifdef __cplusplus
}
#endif

#endif  // _EU_PIXMAP_H_
