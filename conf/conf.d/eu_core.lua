eu_core = {}
eu_core.ffi = require("ffi")
eu_core.eulib = require("euapi")
eu_core.euapi = eu_core.ffi.load(eu_core.ffi.os == "Windows" and "euapi.dll")

eu_core.ffi.cdef[[

typedef struct tagRECT 
{
    long left;
    long top;
    long right;
    long bottom;
}RECT;

typedef struct tagACCEL 
{
    unsigned short fVirt;
    unsigned short key;
    unsigned short cmd;
}ACCEL;

typedef struct _print_set
{
    int header;
    int footer;
    int color_mode;
    int zoom;
    RECT rect;
}print_set;

typedef struct _caret_set
{
    int blink;
    int width;
    uint32_t rgb;
}caret_set;

typedef struct _bookmark_set
{
    bool visable;
    int  shape;
    uint32_t argb;
}bookmark_set;

typedef struct _brace_set
{
    bool matching;
    bool autoc;
    uint32_t rgb;
}brace_set;

typedef struct _calltip_set
{
    bool enable;
    uint32_t rgb;
}calltip_set;

typedef struct _complete_set
{
    bool enable;
    int  characters;
    int  snippet;
}complete_set;

struct eu_config
{
    int new_file_eol;
    int new_file_enc;
    
    bool m_ident;
    char window_theme[64];
    bool m_fullscreen;
    bool m_menubar;
    bool m_toolbar;
    bool m_statusbar;
    bool m_linenumber;

    uint32_t last_flags;
    bool ws_visiable;
    int ws_size;
    bool newline_visialbe;
    
    bool m_indentation;
    int tab_width;
    bool tab2spaces;
    bool light_fold;
    bool line_mode;
    bool m_ftree_show;

    int file_tree_width;
    int sym_list_width;
    int sym_tree_width;
    int sidebar_width;
    int document_map_width;
    int result_edit_height;
    int result_list_height;
    int file_recent_number;
    int inter_reserved_0;
    int inter_reserved_1;
    int inter_reserved_2;
    
    bool block_fold;
    bool m_tab_tip;
    
    int m_close_way;
    int m_new_way;
    int m_tab_active;
    int m_quality;
    int m_render;
    int  m_upfile;
    bool m_light_str;
    bool m_write_copy;
    bool m_session;
    bool m_exit;
    bool m_instance;
    char m_placement[1024];
    char m_language[64];
    bookmark_set eu_bookmark;
    brace_set eu_brace;
    caret_set eu_caret;
    calltip_set eu_calltip;
    complete_set eu_complete;
    print_set eu_print;
    int m_limit;
    uint64_t m_id;
    char m_path[260];
    char editor[260];
    char m_reserved_0[260];
    char m_reserved_1[260];
    char m_actions[100][260];
};
    
struct styleclass
{
    char font[32];
    int fontsize;
    uint32_t color;
    uint32_t bgcolor;
    int bold;
};

struct styletheme
{
    struct styleclass linenumber;
    struct styleclass foldmargin;

    struct styleclass text;
    struct styleclass caretline;
    struct styleclass indicator;

    struct styleclass keywords0;
    struct styleclass keywords1;
    struct styleclass string;
    struct styleclass character;
    struct styleclass number;
    struct styleclass operators;
    struct styleclass preprocessor;
    struct styleclass comment;
    struct styleclass commentline;
    struct styleclass commentdoc;

    struct styleclass tags;
    struct styleclass unknowtags;
    struct styleclass attributes;
    struct styleclass unknowattributes;
    struct styleclass entities;
    struct styleclass tagends;
    struct styleclass cdata;
    struct styleclass phpsection;
    struct styleclass aspsection;
    
    struct styleclass activetab;
};

struct eu_theme
{
    char pathfile[260];
    char name[64];
    struct styletheme item;
};

struct rb_node
{
    uintptr_t rb_parent_color;
    struct rb_node *rb_right;
    struct rb_node *rb_left;
};

struct rb_root
{
    struct rb_node *rb_node;
};

typedef struct rb_root eutype_t;
typedef int (*init_before_ptr)(void *pnode);
typedef int (*init_after_ptr)(void *pnode);
typedef int (*parse_header_ptr)(void *pnode);
typedef int (*add_ptr)(void *pnode, void *lpnotify);
typedef int (*key_ptr)(void *pnode, intptr_t wParam, intptr_t lParam);
typedef int (*reload_list_ptr)(void *pnode);
typedef int (*click_list_ptr)(void *pnode);
typedef int (*reload_tree_ptr)(void *pnode);
typedef int (*click_tree_ptr)(void *pnode);

typedef struct _doc_styles
{
    int type[32];
    uint32_t fgcolor[32];
    uint32_t bkcolor[32];
    uint32_t mask;
} doc_styles;

typedef struct _doc_comments
{
    const char *line;
    const char *block;
    bool initialized;
} doc_comments;

typedef struct _snippet_t
{
    intptr_t start;
    intptr_t end;
    char name[64];
    char comment[64];
    char parameter[8];
    char body[2048];
} snippet_t;

typedef struct _doc_data
{
    int doc_type;                             // 文档类型编号,自行添加请从末尾数字开始递增
    const char *filetypename;                 // 文档类型名称
    const char *extname;                      // 文档扩展名
    const char *filedesc;                     // 文档类型描述
    const char *snippet;                      // 文档的代码片段文件所在
    int tab_width;                            // tab键宽度, default = 0, 跟随主配置
    int tab_convert_spaces;                   // tab键是否转换为空格, default = -1: 跟随主配置, 0: false, 1: true
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
    snippet_t *ptrv;                          // 解析后的代码片段关键字
    const char *keywords0;                    // 需要高亮的关键字, 分6类高亮着色
    const char *keywords1;
    const char *keywords2;
    const char *keywords3;
    const char *keywords4;
    const char *keywords5;
    const char *reqular_exp;                  // 根据此正则表达式初始化list控件
    eutype_t acshow_tree;                     // 自动补全hash表
    eutype_t ctshow_tree;                     // 函数提示hash表
    doc_styles style;                         // 文档关键字类型与高亮颜色
    doc_comments comment;                     // 文档注释
} doctype_t;

bool __stdcall eu_config_ptr(struct eu_config *pconfig);
bool __stdcall eu_theme_ptr(struct eu_theme *ptheme, bool init);
bool __stdcall eu_accel_ptr(ACCEL *paccel);
bool __stdcall eu_exist_path(const char *path);
char *_fullpath(char *buf, const char *path, size_t maxlen);

// all doctype callbacks
bool eu_init_calltip_tree(doctype_t *p, const char *key, const char *val);
bool eu_init_completed_tree(doctype_t *p, const char *val);
int64_t eu_sci_call(void *p, int m, int64_t w, int64_t l);

/* 默认的 init_before_ptr 回调函数入口 */
int on_doc_init_list(void *pnode);
int on_doc_init_tree(void *pnode);
int on_doc_init_result(void *pnode);
int on_doc_init_list_sh(void *pnode);

/* 默认的 init_after_ptr 回调函数入口 */
int on_doc_init_after_cpp(void *pnode);
int on_doc_init_after_cs(void *pnode);
int on_doc_init_after_java(void *pnode);
int on_doc_init_after_go(void *pnode);
int on_doc_init_after_swift(void *pnode);
int on_doc_init_after_sql(void *pnode);
int on_doc_init_after_redis(void *pnode);
int on_doc_init_after_python(void *pnode);
int on_doc_init_after_lua(void *pnode);
int on_doc_init_after_perl(void *pnode);
int on_doc_init_after_shell(void *pnode);
int on_doc_init_after_rust(void *pnode);
int on_doc_init_after_ruby(void *pnode);
int on_doc_init_after_lisp(void *pnode);
int on_doc_init_after_asm(void *pnode);
int on_doc_init_after_cobol(void *pnode);
int on_doc_init_after_html(void *pnode);
int on_doc_init_after_css(void *pnode);
int on_doc_init_after_js(void *pnode);
int on_doc_init_after_xml(void *pnode);
int on_doc_init_after_json(void *pnode);
int on_doc_init_after_yaml(void *pnode);
int on_doc_init_after_makefile(void *pnode);
int on_doc_init_after_cmake(void *pnode);
int on_doc_init_after_log(void *pnode);
int on_doc_init_after_nim(void *pnode);
int on_doc_init_after_shell_sh(void *pnode);
int on_doc_init_after_properties(void *pnode);
int on_doc_init_after_diff(void *pnode);

/* 默认的 key_ptr 回调函数入口 */
int on_doc_keydown_jmp(void *pnode, intptr_t wParam, intptr_t lParam);
int on_doc_keydown_sql(void *pnode, intptr_t wParam, intptr_t lParam);
int on_doc_keydown_redis(void *pnode, intptr_t wParam, intptr_t lParam);
int on_doc_keyup_general(void *pnode, intptr_t wParam, intptr_t lParam);
int on_doc_keyup_general_sh(void *, intptr_t wParam, intptr_t lParam);

/* 默认的 add_ptr 回调函数入口 */
int on_doc_identation(void *pnode, void *lpnotify);
int on_doc_cpp_like(void *pnode, void *lpnotify);
int on_doc_sql_like(void *pnode, void *lpnotify);
int on_doc_redis_like(void *pnode, void *lpnotify);
int on_doc_html_like(void *pnode, void *lpnotify);
int on_doc_xml_like(void *pnode, void *lpnotify);
int on_doc_css_like(void *pnode, void *lpnotify);
int on_doc_json_like(void *pnode, void *lpnotify);
int on_doc_makefile_like(void *pnode, void *lpnotify);
int on_doc_cmake_like(void *pnode, void *lpnotify);

/* 默认的 reload_list_ptr,reload_tree_ptr 回调函数入口 */
int on_doc_reload_list_reqular(void *pnode);
int on_doc_reload_list_sh(void *pnode);
int on_doc_reload_tree_xml(void *pnode);
int on_doc_reload_tree_json(void *pnode);
int on_doc_reload_tree_sql(void *pnode);
int on_doc_reload_tree_redis(void *pnode);
int on_doc_click_list_jmp(void *pnode);
int on_doc_click_list_jump_sh(void *pnode);
int on_doc_click_tree_sql(void *pnode);
int on_doc_click_tree_json(void *pnode);
int on_doc_click_tree_redis(void *pnode);

/* lua脚本接口支持, 对各类关键字着色 */
int on_doc_init_after_scilexer(void *p, const  char *name);
void on_doc_enable_foldline(void *p);
void on_doc_default_light(void *p, int lex, intptr_t bg_rgb, intptr_t bk_rgb, bool force);
void on_doc_keyword_light(void *p, int lex, int index, intptr_t rgb);
void on_doc_function_light(void *p, int lex, int index, intptr_t rgb);
void on_doc_preprocessor_light(void *p, int lex, int index, intptr_t rgb);
void on_doc_marcro_light(void *p, int lex, int index, intptr_t rgb);
void on_doc_variable_light(void *p, int lex, intptr_t rgb);
void on_doc_string_light(void *p, int lex, intptr_t rgb);
void on_doc_operator_light(void **pnode, int lex, intptr_t rgb);
void on_doc_char_light(void *p, int lex, intptr_t rgb);
void on_doc_number_light(void *p, int lex, intptr_t rgb);
void on_doc_special_light(void *p, int lex, intptr_t rgb);
void on_doc_send_light(void *p, int lex, int index, intptr_t rgb);
void on_doc_tags_light(void *p, int lex, intptr_t rgb);
void on_doc_comment_light(void *, int lex, intptr_t rgb);
void on_doc_commentblock_light(void *, int lex, intptr_t rgb);
void on_doc_commentdoc_light(void *, int lex, intptr_t rgb);

]]

function eu_core.script_path()
    return eu_core.eulib.lconfdir()
end

function eu_core.mkdir(path)
    return eu_core.eulib.lmkdir(path)
end

function eu_core.file_exists(path)
   local f=io.open(path,"r")
   if f~=nil then io.close(f) return true else return false end
end

function eu_core.table_is_empty(t)
    if t == nil then return true end
    return _G.next(t) == nil
end

function eu_core.enum(tbl)
    local i = 0
    if tbl == nil or type(tbl) ~= "table" then 
        return nil
    end 
    tbl.__index = tbl 
    tbl.__newindex = function() 
        print("can not modify enum")
    end
    for key,value in pairs(tbl) do i=i+1 end
    if (i >= 2) then i=i-2 end
    return setmetatable({}, tbl),i
end

function eu_core.save_file(filename, buffer)
    local f = assert(io.open(filename, 'wb+'))
    io.output(f)
    io.write(buffer)
    io.close(f)
end

function eu_core.strncat(dest, src, num)
    return eu_core.ffi.C.strncat(dest, src, num)
end

return eu_core
