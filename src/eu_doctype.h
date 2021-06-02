/*******************************************************************************
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

#ifndef _H_SKYLARK_DOCTYPE_
#define _H_SKYLARK_DOCTYPE_

#define CONFIG_KEY_MATERIAL_LEXER "EU_DOCTYP"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct rb_root eutype_t;
typedef int (*init_before_ptr)(eu_tabpage *pnode);
typedef int (*init_after_ptr)(eu_tabpage *pnode);
typedef int (*parse_header_ptr)(eu_tabpage *pnode);
typedef int (*add_ptr)(eu_tabpage *pnode, SCNotification *lpnotify);
typedef int (*key_ptr)(eu_tabpage *pnode, WPARAM wParam, LPARAM lParam);
typedef int (*reload_list_ptr)(eu_tabpage *pnode);
typedef int (*click_list_ptr)(eu_tabpage *pnode);
typedef int (*reload_tree_ptr)(eu_tabpage *pnode);
typedef int (*click_tree_ptr)(eu_tabpage *pnode);

enum dctype
{
    DOCTYPE_END = 0,
    DOCTYPE_TXT,
    DOCTYPE_CPP,
    DOCTYPE_CS,
    DOCTYPE_JAVA,
    DOCTYPE_JAVASCRIPT,
    DOCTYPE_GO,
    DOCTYPE_SWIFT,
    DOCTYPE_SQL,
    DOCTYPE_REDIS,
    DOCTYPE_PYTHON,
    DOCTYPE_LUA,
    DOCTYPE_PERL,
    DOCTYPE_SH,
    DOCTYPE_RUST,
    DOCTYPE_RUBY,
    DOCTYPE_LISP,
    DOCTYPE_ASM,
    DOCTYPE_COBOL,
    DOCTYPE_HTML,
    DOCTYPE_XML,
    DOCTYPE_CSS,
    DOCTYPE_JSON,
    DOCTYPE_YAML,
    DOCTYPE_MAKEFILE,
    DOCTYPE_CMAKE,
    DOCTYPE_MARKDOWN,
    DOCTYPE_LOG,
    DOCTYPE_PROPERTIES,
    DOCTYPE_NIM,
    DOCTYPE_AU3,
    DOCTYPE_FORTRAN
};

typedef struct _doc_data
{
    enum dctype doc_type;                     // 文档类型编号
    const char *filetypename;                 // 文档类型名称
    const char *extname;                      // 文档扩展名
    const char *filedesc;                     // 文档类型描述
    int tab_width;                            // tab键宽度, default = 0, 跟随主配置
    int tab_convert_spaces;                   // tab键是否转换为空格, default = -1, 跟随主配置
    init_before_ptr fn_init_before;           // 回调函数, 在文档初始化前运行
    init_after_ptr fn_init_after;             // 回调函数, 在文档初始化后运行
    parse_header_ptr fn_parse_header;         // 回调函数, 分析文件头, 用于sql
    key_ptr fn_keydown;                       // 回调函数, 当编辑器内按下某键
    key_ptr fn_keyup;                         // 回调函数, 当编辑器内释放某键
    add_ptr fn_on_char;                       // 回调函数, 当编辑器输入字符
    reload_list_ptr fn_reload_symlist;        // 回调函数, 右侧边栏list控件初始化
    click_list_ptr fn_click_symlist;          // 回调函数, 右侧边栏list控件被点击
    reload_tree_ptr fn_reload_symtree;        // 回调函数, 右侧边栏tree控件初始化
    click_tree_ptr fn_click_symtree;          // 回调函数, 右侧边栏tree控件被点击
    char *keywords0;                          // 需要高亮的关键字, 分6类高亮着色
    char *keywords1;
    char *keywords2;
    char *keywords3;
    char *keywords4;
    char *keywords5;
    char *reqular_exp;                        // 根据此正则表达式初始化list控件
    eutype_t acshow_tree;                     // 自动补全hash表
    eutype_t ctshow_tree;                     // 函数提示hash表
} doctype_t;

doctype_t *eu_doc_get_ptr(void);
int on_doc_count(void);
void eu_doc_set_ptr(doctype_t *ptr);
doctype_t *on_doc_get_type(const TCHAR *pfile);
TCHAR *on_doc_get_ext(eu_tabpage *pnode);

#ifdef __cplusplus
}
#endif

#endif
