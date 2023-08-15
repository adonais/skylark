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

#ifndef _EU_FRAMEWORK_H_
#define _EU_FRAMEWORK_H_

#include "targetver.h"
// 不加载MFC以及一些不常用的模块
#define WIN32_LEAN_AND_MEAN
// Windows 头文件
#include <windows.h>
#include <richedit.h>
#include <Commdlg.h>
#include <commctrl.h>
#include <windowsx.h>
#include <shellapi.h>
#include <process.h>
// C 运行时头文件
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <memory.h>
#include <tchar.h>
#include <wchar.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#define VIEW_STYLETHEME_MAXCOUNT     20
#define VIEW_FILETYPE_MAXCOUNT       100

#define FILETREE_MARGIN_LEFT         2
#define FILETREE_MARGIN_RIGHT        2
#define FILETREE_MARGIN_TOP          2
#define FILETREE_MARGIN_BOTTOM       0

#define SCINTILLA_MARGIN_LEFT        2
#define SCINTILLA_MARGIN_RIGHT       2
#define SCINTILLA_MARGIN_TOP         2
#define SCINTILLA_MARGIN_BOTTOM      0

#define MARGIN_LINENUMBER_INDEX      0
#define MARGIN_LINENUMBER_WIDTH      40

#define MARGIN_BOOKMARK_INDEX        1
#define MARGIN_BOOKMARK_WIDTH        16
#define MARGIN_BOOKMARK_MASKN        1
#define MARGIN_BOOKMARK_VALUE        0

#define MARGIN_FOLD_INDEX            2
#define MARGIN_FOLD_WIDTH            14

#define SYMBOLLIST_MARGIN_LEFT       2
#define SYMBOLLIST_MARGIN_RIGHT      2
#define SYMBOLLIST_MARGIN_TOP        2
#define SYMBOLLIST_MARGIN_BOTTOM     0

#define SYMBOLTREE_MARGIN_LEFT       2
#define SYMBOLTREE_MARGIN_RIGHT      2
#define SYMBOLTREE_MARGIN_TOP        2
#define SYMBOLTREE_MARGIN_BOTTOM     0

#define FILETREEBAR_WIDTH_MIN        100
#define SYMBOLLIST_WIDTH_MIN         100
#define TREEVIEW_WIDTH_MIN           100
#define DOCUMENTMAP_WIDTH_MIN        32

#define SPLIT_WIDTH                  2

// 第三方依赖的头文件
#include "Scintilla.h"
#include "SciLexer.h"

#include "pcre.h"

#include "curl/curl.h"

#include "openssl/sha.h"
#include "openssl/aes.h"
#include "openssl/evp.h"
#include "openssl/md5.h"
#include "openssl/des.h"

#include "iconv.h"
#include "mysql.h"
#include "oci.h"
#include "hiredis/hiredis.h"
#include "sqlite3.h"
#include "libpq-fe.h"
#include "chardet.h"
#include "tinyexpr.h"

// skylark 源目录下的头文件
#include "eu_cvector.h"
#include "eu_list.h"
#include "eu_rbtree.h"

#include "nanosvg.h"
#include "nanosvgrast.h"

#include "eu_nphost.h"
#include "eu_api.h"
#include "eu_hook.h"
#include "eu_share.h"
#include "eu_script.h"
#include "eu_splitter.h"
#include "eu_log.h"
#include "eu_sql.h"
#include "eu_input.h"
#include "eu_changes.h"
#include "eu_toolbar.h"
#include "eu_statusbar.h"
#include "eu_json.h"
#include "eu_format.h"
#include "eu_qrgen.h"

#include "eu_hex.h"
#include "eu_util.h"
#include "eu_tablectl.h"
#include "eu_encoding.h"
#include "eu_remotefs.h"
#include "eu_theme.h"
#include "eu_theme_dark.h"
#include "eu_theme_menu.h"
#include "eu_symlistctl.h"
#include "eu_symtreectl.h"
#include "eu_resultctl.h"
#include "eu_config.h"
#include "eu_treebar.h"
#include "eu_scintilla.h"
#include "eu_file.h"
#include "eu_edit.h"
#include "eu_search.h"
#include "eu_view.h"
#include "eu_registry.h"
#include "eu_about.h"
#include "eu_code.h"
#include "eu_doctype.h"
#include "eu_menu.h"
#include "eu_proc.h"
#include "eu_tabpage.h"
#include "eu_print.h"
#include "eu_locale.h"
#include "eu_document_map.h"
#include "eu_updatechecker.h"
#include "eu_snippet.h"
#include "eu_snparser.h"
#include "eu_complete.h"
#include "eu_pixmap.h"
#include "eu_hyperlink.h"
#include "eu_exporter.h"
#include "eu_session.h"
#include "eu_favorites.h"

#endif  // _EU_FRAMEWORK_H_
