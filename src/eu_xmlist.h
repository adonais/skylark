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

#ifndef _EU_XMLIST_H_
#define _EU_XMLIST_H_

#ifdef __cplusplus
extern "C" {
#endif

int  on_xml_tree(eu_tabpage *pnode);
int  on_xml_format(eu_tabpage *pnode);
bool on_xml_pretty(void *ptr, struct opt_format *opt);

#ifdef __cplusplus
}
#endif

#endif  // _EU_XMLIST_H_
