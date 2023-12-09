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

#ifndef _H_SKYLARK_TABPAGES_
#define _H_SKYLARK_TABPAGES_

#define CONFIG_KEY_MATERIAL_TABPAGES "EU_TABPAG"
#define TABS_SPLIT            (10)
#define TABS_WIDTH_DEFAULT    (120)
#define TABS_HEIGHT_DEFAULT   (23)
#define TABS_MAYBE_RESERVE    (-1)
#define TABS_MAYBE_EIXT       (-2)
#define HMAIN_GET             \
   (eu_get_config() && eu_get_config()->eu_tab.hmain ? \
   (HWND)eu_get_config()->eu_tab.hmain : NULL)
#define HSLAVE_GET            \
   (eu_get_config() && eu_get_config()->eu_tab.hslave ? \
   (HWND)eu_get_config()->eu_tab.hslave : NULL)

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _complete_t *complete_ptr;
typedef struct _capture_set *capture_ptr;
typedef int  (*tab_ptr)(eu_tabpage *p);
typedef void  (*tab_callback)(int index);
typedef void (__cdecl *tab_want)(void *p);

struct _tabpage
{
    HWND hwnd_sc;               // 当前编辑器句柄
    sptr_t eusc;                // 当前编辑器类指针
    HWND hwnd_symlist;          // tab关联的右侧边栏list窗口句柄
    HWND hwnd_symtree;          // tab关联的右侧边栏tree窗口句柄
    HWND hwnd_qrtable;          // tab关联的table窗口, 显示查询结果
    HFONT hwnd_font;            // tab关联的子窗口控件字体句柄
    RECT rect_sc;               // 编辑器矩形区域
    RECT rect_sym;              // 右侧边栏窗口矩形区域
    RECT rect_qrtable;          // table窗口矩形区域
    RECT rect_map;              // 文档结构图矩形区域
    RECT rect_result;           // 文档搜索结果矩形区域
    RECT rect_sidebar;          // 侧边栏矩形区域
    bool sym_show;              // 是否显示右侧边栏
    bool map_show;              // 是否显示文档结构图
    bool result_show;           // 是否显示文档搜索结果窗口
    bool sidebar_show;          // 是否显示侧边栏窗口
    bool foldline;              // 是否存在折叠线
    bool needpre;               // 是否需要bom
    bool is_blank;              // 新建文件,空白标签
    bool at_close;              // 是否绘制了关闭按钮
    bool be_modify;             // 文档是否修改, 同步hex模式
    bool fn_modify;             // 文档打开时的初始状态
    bool last_focus;            // 保存前台焦点
    TCHAR pathfile[MAX_BUFFER]; // 文件带路径名
    TCHAR pathname[MAX_BUFFER]; // 文件所在路径名
    TCHAR bakpath[MAX_BUFFER];  // 备份后的名称
    TCHAR filename[MAX_PATH];   // 文件名
    TCHAR extname[_MAX_EXT];    // 扩展名
    char pre_context[4+1];      // 保存bom
    char eols_undo_str[QW_SIZE];// eols转换保存的数据,用于undo操作
    char icon_undo_str[QW_SIZE];// 编码转换时保存的数据,用于undo操作
    size_t pre_len;             // bom的长度
    size_t bytes_remaining;     // 文件变动后的大小
    size_t bytes_written;       // 文件保存时写入的长度
    time_t st_mtime;            // 文件修改时间
    uint32_t file_attr;         // 文件属性,只读/可写...
    intptr_t match_count;       // 查找时匹配计数
    intptr_t begin_pos;         // 开始选择位置
    intptr_t nc_pos;            // 关闭编辑器时, 光标所处位置
    intptr_t reserved0;         // 保留, 仅供临时使用
    intptr_t reserved1;         // 保留, 仅供临时使用
    uint64_t raw_size;          // 文件初始大小
    volatile long pcre_id;      // pcre线程id
    volatile long json_id;      // 解析json线程id
    volatile long busy_id;      // 标签是否空闲状态
    volatile long lock_id;      // 自动保存时使用的锁
    int tab_id;                 // tab编号,用于保存会话
    int hex_mode;               // 16进制编辑状态, 0, 否. 1,是. 2,插件
    int codepage;               // 真实的文件编码
    int bakcp;                  // 自动保存时的文件编码
    int eol;                    // 换行符
    int zoom_level;             // 标签页的放大倍数
    int ac_mode;                // 是否处于snippet模式
    int reason;                 // 编辑器窗口状态
    int initial;                // 标签初始化状态
    int view;                   // 标签所在视图
    remotefs fs_server;         // SFTP
    PHEXVIEW phex;              // 二进制视图
    uint8_t *write_buffer;      // 文件保存时写入的缓存区
    eu_tabpage *presult;        // 文档搜索结果的节点指针
    doctype_t *doc_ptr;         // 文件类型指针
    db_conn *db_ptr;            // 数据库配置
    redis_conn *redis_ptr;      // redis配置
    result_vec *ret_vec;        // 搜索结果标记
    complete_ptr ac_vec;        // snippet模式下的vec数组
    capture_ptr re_group;       // snippet正则模式下捕获组
    NMM pmod;                   // 插件模块地址
    npdata *plugin;             // 插件动态数据表
    tab_want pwant;             // 回调函数, 需要时使用
};

extern HWND g_tabpages;

int  on_tabpage_create_dlg(const HWND hwnd);
int  on_tabpage_add(eu_tabpage *pnode);
int  on_tabpage_reload_file(eu_tabpage *pnode, int flags, sptr_t *pline);
int  on_tabpage_theme_changed(eu_tabpage *p);
int  on_tabpage_get_height(const int i);
int  on_tabpage_get_index(const eu_tabpage *pnode);
int  on_tabpage_selection(eu_tabpage *pnode, int index);
int  on_tabpage_sel_number(int **pvec, const bool ascending);
int  on_tabpage_sel_path(wchar_t ***pvec, bool *hex);
void on_tabpage_switch_next(HWND hwnd);
void on_tabpage_adjust_box(const RECT *prc, RECT *ptab1, RECT *ptab2);
void on_tabpage_adjust_window(const RECT *prc, eu_tabpage *pnode, RECT *ptab1, RECT *ptab2);
void on_tabpage_set_title(int ntab, TCHAR *title);
void on_tabpage_symlist_click(eu_tabpage *pnode);
void on_tabpage_foreach(tab_ptr fntab);
void on_tabpage_newdoc_reload(void);
void on_tabpage_close_tabs(int);
void on_tabpage_save_files(int);
void on_tabpage_push_editor(int);
void on_tabpage_do_file(tab_callback func);
void on_tabpage_active_tab(eu_tabpage *pnode);
void on_tabpage_active_one(int index);
void on_tabpage_size(const RECT *prc);
void on_tabpage_variable_reset(void);
bool on_tabpage_exist_map(void);
eu_tabpage *on_tabpage_get_handle(void *hwnd_sc);
eu_tabpage *on_tabpage_get_ptr(const int index);
eu_tabpage *on_tabpage_select_index(int index);
eu_tabpage *on_tabpage_focus_at(void);
eu_tabpage *on_tabpage_remove(const eu_tabpage *pnode, const CLOSE_MODE mode);
TCHAR *on_tabpage_generator(TCHAR *filename, const int len);

#ifdef __cplusplus
}
#endif

#endif
