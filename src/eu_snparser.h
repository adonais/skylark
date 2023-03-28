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

#ifndef _EU_SNIPPET_PARSER_H_
#define _EU_SNIPPET_PARSER_H_

#ifndef PARAM_LEN
#define PARAM_LEN 8
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
#define ftruncate(a, b) _chsize(a, b)
#endif

typedef struct _snippet_t
{
    intptr_t start;
    intptr_t end;
    char name[QW_SIZE];
    char comment[QW_SIZE];
    char parameter[PARAM_LEN];
    char body[LARGER_LEN];
} snippet_t;

bool on_parser_vector_new(const TCHAR *path, snippet_t **ptr_vec, int dimension, int eol);
bool on_parser_vector_modify(const TCHAR *path, snippet_t **ptr_vec, int dimension);
bool on_parser_vector_erase(const TCHAR *path, snippet_t **ptr_vec, int dimension);
bool on_parser_init(const TCHAR *path, snippet_t **ptr_vec, int *peol);

#ifdef __cplusplus
}
#endif

#endif  // _EU_SNIPPET_PARSER_H_
