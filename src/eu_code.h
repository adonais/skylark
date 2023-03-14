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

#ifndef _EU_CODE_H_
#define _EU_CODE_H_

#define BEGIN_DATABASE_CONNECTION_CONFIG    "-- SKYLARK BEGIN DATABASE CONNECTION CONFIG"
#define END_DATABASE_CONNECTION_CONFIG      "-- SKYLARK END DATABASE CONNECTION CONFIG"
#define BEGIN_REDIS_CONNECTION_CONFIG       "-- SKYLARK BEGIN REDIS CONNECTION CONFIG"
#define END_REDIS_CONNECTION_CONFIG         "-- SKYLARK END REDIS CONNECTION CONFIG"

#ifdef __cplusplus
extern "C" {
#endif

void on_code_switch_fold(eu_tabpage *pnode, sptr_t line_number);
void on_code_block_contract_all(eu_tabpage *pnode);
void on_code_block_expand_all(eu_tabpage *pnode);
void on_code_set_complete_chars(eu_tabpage *pnode);
void on_code_insert_config(eu_tabpage *pnode);
void on_code_do_fold(eu_tabpage *pnode, int code, sptr_t line_number, bool do_wrap);

#ifdef __cplusplus
}
#endif

#endif  // _EU_CODE_H_
