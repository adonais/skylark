/******************************************************************************
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

#ifndef _EU_REGISTRY_H_
#define _EU_REGISTRY_H_

#define REGFILE         (_T("-regfile"))
#define REGFILE_BLACK   (_T("-regfile=1"))
#define REGFOLDER       (_T("-regfolder"))
#define REGFOLDER_BLACK (_T("-regfolder=1"))
#define REGASSOC        (_T("-regassoc"))
#define REGASSOC_BLACK  (_T("-regassoc=1"))

#ifdef __cplusplus
extern "C" {
#endif

int on_reg_files_association(void);
bool on_reg_admin(void);
void on_reg_update_menu(void);

#ifdef __cplusplus
}
#endif

#endif  // _EU_REGISTRY_H_
