emod = {}
emod.ffi = require('ffi')
emod.euapi = emod.ffi.load(emod.ffi.os == "Windows" and "euapi.dll")

function emod.script_path()
    local function sum(a, b)
            return a + b
    end
    local info = debug.getinfo(sum)
    --解析出来当前目录
    local path = info.source
    -- 去掉开头的"@"
    path = string.sub(path, 2, -1)
    -- 捕获最后一个 "/" 之前的部分 就是我们最终要的目录部分
    path = string.match(path, "^(.*)\\")
    if path == nil then return "." end
    return path
end

function emod.file_exists(cpath)
   local f=io.open(cpath,"r")
   if f~=nil then io.close(f) return true else return false end
end

function emod.table_is_empty(t)
  if t == nil then return true end
  return _G.next(t) == nil
end

emod.ffi.cdef[[

typedef struct tagRECT 
{
    long left;
    long top;
    long right;
    long bottom;
}RECT;

typedef struct _print_set
{
    int header;
    int footer;
    int color_mode;
    int zoom;
    RECT rect;
}print_set;

struct eu_config
{
    int new_file_eol;
    int new_file_enc;
    
    bool auto_close_chars;
    bool m_ident;
    char window_theme[64];
    bool m_fullscreen;
    bool m_menubar;
    bool m_toolbar;
    bool m_statusbar;
    bool m_linenumber;
    
    bool bookmark_visable;
    bool ws_visiable;
    int ws_size;
    bool newline_visialbe;
    
    bool m_indentation;
    int tab_width;
    bool tab2spaces;
    bool light_fold;
    bool line_mode;
    
    bool m_ftree_show;
    bool m_sym_show;

    int file_tree_width;
    int sym_list_width;
    int sym_tree_width;
    int result_edit_height;
    
    int result_list_height;
    bool block_fold;
    bool m_acshow;
    int acshow_chars;
    
    bool m_ctshow;
    int m_quality;
    int m_render;
    int  m_upfile;
    bool m_light_str;
    bool m_write_copy;
    bool m_session;
    char m_placement[1024];
    char m_language[64];
    print_set eu_print;
    int m_limit;
    uint64_t m_id;
    char m_path[260];
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

enum dctype
{
    DOCTYPE_END = 0,
    DOCTYPE_ASM = 1,
    DOCTYPE_AU3 = 2,
    DOCTYPE_CS = 3,
    DOCTYPE_CPP = 4,
    DOCTYPE_CMAKE = 5,
    DOCTYPE_CSS = 6,
    DOCTYPE_COBOL = 7,
    DOCTYPE_FORTRAN = 8,
    DOCTYPE_GO = 9,
    DOCTYPE_HTML = 10,
    DOCTYPE_JSON = 11,
    DOCTYPE_JAVA = 12,
    DOCTYPE_JAVASCRIPT = 13,
    DOCTYPE_JULIA = 14,
    DOCTYPE_LISP = 15,
    DOCTYPE_LOG = 16,
    DOCTYPE_LUA = 17,
    DOCTYPE_MAKEFILE = 18,
    DOCTYPE_MARKDOWN = 19,
    DOCTYPE_NIM = 20,
    DOCTYPE_PERL = 21,
    DOCTYPE_PROPERTIES = 22,
    DOCTYPE_PYTHON = 23,
    DOCTYPE_REDIS = 24,
    DOCTYPE_RUBY = 25,
    DOCTYPE_RUST = 26,
    DOCTYPE_SQL = 27,
    DOCTYPE_SH = 28,
    DOCTYPE_SWIFT = 29,
    DOCTYPE_TXT = 30,
    DOCTYPE_XML = 31,
    DOCTYPE_YAML = 32
} ;

typedef struct _doc_data
{
    int doc_type;                             // 文档类型编号,自行添加请从DOCTYPE_FORTRAN开始递增
    const char *filetypename;                 // 文档类型名称
    const char *extname;                      // 文档扩展名
    const char *filedesc;                     // 文档类型描述
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

bool __stdcall eu_config_ptr(struct eu_config *pconfig);
bool __stdcall eu_theme_ptr(struct eu_theme *ptheme, bool init);
char *_fullpath(char *buf, const char *path, size_t maxlen);

// all doctype callbacks
bool eu_init_calltip_tree(doctype_t *p, const char *key, const char *val);
bool eu_init_completed_tree(doctype_t *p, const char *val);

/* 默认的 init_before_ptr 回调函数入口 */
int on_doc_init_list(void *pnode);
int on_doc_init_tree(void *pnode);
int on_doc_init_result(void *pnode);
int on_doc_init_result_list(void *pnode);
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
int on_doc_init_after_markdown(void *pnode);
int on_doc_init_after_log(void *pnode);
int on_doc_init_after_nim(void *pnode);
int on_doc_init_after_shell_sh(void *pnode);
int on_doc_init_after_properties(void *pnode);

/* 默认的 key_ptr 回调函数入口 */
int on_doc_keydown_jmp(void *pnode, intptr_t wParam, intptr_t lParam);
int on_doc_keydown_light(void *pnode);
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
int on_doc_yaml_like(void *pnode, void *lpnotify);
int on_doc_makefile_like(void *pnode, void *lpnotify);
int on_doc_cmake_like(void *pnode, void *lpnotify);
int on_doc_markdown_like(void *pnode, void *lpnotify);

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
int on_doc_init_sci_lexer(void *p, int, int, int, int, int, int, int, int);
int on_doc_enable_scilexer(void *p, int lex);
void on_doc_default_light(void *p, int lex, int64_t rgb);
void on_doc_keyword_light(void *p, int lex, int index, int64_t rgb);
void on_doc_function_light(void *p, int lex, int index, int64_t rgb);
void on_doc_preprocessor_light(void *p, int lex, int index, int64_t rgb);
void on_doc_marcro_light(void *p, int lex, int index, int64_t rgb);
void on_doc_variable_light(void *p, int lex, int64_t rgb);
void on_doc_string_light(void *p, int lex, int64_t rgb);
void on_doc_operator_light(void **pnode, int lex, int64_t rgb);
void on_doc_char_light(void *p, int lex, int64_t rgb);
void on_doc_number_light(void *p, int lex, int64_t rgb);
void on_doc_special_light(void *p, int lex, int64_t rgb);
void on_doc_send_light(void *p, int lex, int index, int64_t rgb);
void on_doc_tags_light(void *p, int lex, int64_t rgb);
void on_doc_comment_light(void *, int lex, int64_t rgb);
void on_doc_commentblock_light(void *, int lex, int64_t rgb);
void on_doc_commentdoc_light(void *, int lex, int64_t rgb);

]]

function emod.strncat(dest, src, num)
	return emod.ffi.C.strncat(dest, src, num)
end

function emod.load_theme(name)
	local file = " "
	local tname = "default"
	local path = emod.script_path()
	if (name == tname) then
	  file = (path .. "\\..\\styletheme.conf")
	else
	  file = (path .. "\\..\\styletheme_" .. name .. ".conf")
	end

	if (not emod.file_exists(file)) then
	  file = (path .. "\\..\\styletheme.conf")
	  local theme = -- 默认主题配置文件
	    "linenumber_font = \"Courier New\"\n" ..
	    "linenumber_fontsize = 9\n" ..
	    "linenumber_color = 0x00FFFFFF\n" ..
	    "linenumber_bgcolor = 0x00888888\n" ..
	    "linenumber_bold = 0\n" ..
	    "foldmargin_font = \"Courier New\"\n" ..
	    "foldmargin_fontsize = 9\n" ..
	    "foldmargin_color = 0x00666666\n" ..
	    "foldmargin_bgcolor = 0x00666666\n" ..
	    "foldmargin_bold = 0\n" ..
	    "text_font = \"Courier New\"\n" ..
	    "text_fontsize = 11\n" ..
	    "text_color = 0x00FFFFFF\n" ..
	    "text_bgcolor = 0x00444444\n" ..
	    "text_bold = 0\n" ..
	    "caretline_font = \"Courier New\"\n" ..
	    "caretline_fontsize = 11\n" ..
	    "caretline_color = 0x00000000\n" ..
	    "caretline_bgcolor = 0x00696969\n" ..
	    "caretline_bold = 0\n" ..
	    "indicator_font = \"Courier New\"\n" ..
	    "indicator_fontsize = 11\n" ..
	    "indicator_color = 0x00FFFFFF\n" ..
	    "indicator_bgcolor = 0x00EEEEEE\n" ..
	    "indicator_bold = 0\n" ..
	    "keywords_font = \"Courier New\"\n" ..
	    "keywords_fontsize = 11\n" ..
	    "keywords_color = 0x0000B050\n" ..
	    "keywords_bgcolor = 0x00000000\n" ..
	    "keywords_bold = 1\n" ..
	    "keywords2_font = \"Courier New\"\n" ..
	    "keywords2_fontsize = 11\n" ..
	    "keywords2_color = 0x0000B050\n" ..
	    "keywords2_bgcolor = 0x00000000\n" ..
	    "keywords2_bold = 1\n" ..
	    "string_font = \"Courier New\"\n" ..
	    "string_fontsize = 11\n" ..
	    "string_color = 0x00C080FF\n" ..
	    "string_bgcolor = 0x00000000\n" ..
	    "string_bold = 0\n" ..
	    "character_font = \"Courier New\"\n" ..
	    "character_fontsize = 11\n" ..
	    "character_color = 0x00C080FF\n" ..
	    "character_bgcolor = 0x00000000\n" ..
	    "character_bold = 0\n" ..
	    "number_font = \"Courier New\"\n" ..
	    "number_fontsize = 11\n" ..
	    "number_color = 0x00C080FF\n" ..
	    "number_bgcolor = 0x00000000\n" ..
	    "number_bold = 0\n" ..
	    "operator_font = \"Courier New\"\n" ..
	    "operator_fontsize = 11\n" ..
	    "operator_color = 0x00FFFFFF\n" ..
	    "operator_bgcolor = 0x00000000\n" ..
	    "operator_bold = 0\n" ..
	    "preprocessor_font = \"Courier New\"\n" ..
	    "preprocessor_fontsize = 11\n" ..
	    "preprocessor_color = 0x00A349A4\n" ..
	    "preprocessor_bgcolor = 0x00000000\n" ..
	    "preprocessor_bold = 0\n" ..
	    "comment_font = \"Courier New\"\n" ..
	    "comment_fontsize = 11\n" ..
	    "comment_color = 0x00C0C0C0\n" ..
	    "comment_bgcolor = 0x00000000\n" ..
	    "comment_bold = 0\n" ..
	    "commentline_font = \"Courier New\"\n" ..
	    "commentline_fontsize = 11\n" ..
	    "commentline_color = 0x00C0C0C0\n" ..
	    "commentline_bgcolor = 0x00000000\n" ..
	    "commentline_bold = 0\n" ..
	    "commentdoc_font = \"Courier New\"\n" ..
	    "commentdoc_fontsize = 11\n" ..
	    "commentdoc_color = 0x0000C800\n" ..
	    "commentdoc_bgcolor = 0x00000000\n" ..
	    "commentdoc_bold = 0\n" ..
	    "tags_font = \"Courier New\"\n" ..
	    "tags_fontsize = 11\n" ..
	    "tags_color = 0x00FF8000\n" ..
	    "tags_bgcolor = 0x00000000\n" ..
	    "tags_bold = 0\n" ..
	    "unknowtags_font = \"Courier New\"\n" ..
	    "unknowtags_fontsize = 11\n" ..
	    "unknowtags_color = 0x00804000\n" ..
	    "unknowtags_bgcolor = 0x00000000\n" ..
	    "unknowtags_bold = 0\n" ..
	    "attributes_font = \"Courier New\"\n" ..
	    "attributes_fontsize = 11\n" ..
	    "attributes_color = 0x0080FF80\n" ..
	    "attributes_bgcolor = 0x00000000\n" ..
	    "attributes_bold = 0\n" ..
	    "unknowattributes_font = \"Courier New\"\n" ..
	    "unknowattributes_fontsize = 11\n" ..
	    "unknowattributes_color = 0x00808040\n" ..
	    "unknowattributes_bgcolor = 0x00000000\n" ..
	    "unknowattributes_bold = 0\n" ..
	    "entities_font = \"Courier New\"\n" ..
	    "entities_fontsize = 11\n" ..
	    "entities_color = 0x00800080\n" ..
	    "entities_bgcolor = 0x00000000\n" ..
	    "entities_bold = 0\n" ..
	    "tagends_font = \"Courier New\"\n" ..
	    "tagends_fontsize = 11\n" ..
	    "tagends_color = 0x00000080\n" ..
	    "tagends_bgcolor = 0x00000000\n" ..
	    "tagends_bold = 0\n" ..
	    "cdata_font = \"Courier New\"\n" ..
	    "cdata_fontsize = 11\n" ..
	    "cdata_color = 0x00008000\n" ..
	    "cdata_bgcolor = 0x00000000\n" ..
	    "cdata_bold = 0\n" ..
	    "phpsection_font = \"Courier New\"\n" ..
	    "phpsection_fontsize = 11\n" ..
	    "phpsection_color = 0x00FFFFFF\n" ..
	    "phpsection_bgcolor = 0x00000000\n" ..
	    "phpsection_bold = 0\n" ..
	    "aspsection_font = \"Courier New\"\n" ..
	    "aspsection_fontsize = 11\n" ..
	    "aspsection_color = 0x00808080\n" ..
	    "aspsection_bgcolor = 0x00000000\n" ..
	    "aspsection_bold = 0"
	  eu_code = loadstring(theme)
	  eu_code()
	else
	  dofile(file)
	  tname = name
	end
	local m_file = emod.ffi.new('char[260]')
	emod.ffi.C._fullpath(m_file, file, 260)
	local m_theme = emod.ffi.new("struct eu_theme", {m_file, tname, 
	  {
		{linenumber_font,linenumber_fontsize,linenumber_color,linenumber_bgcolor,linenumber_bold},
		{foldmargin_font,foldmargin_fontsize,foldmargin_color,foldmargin_bgcolor,foldmargin_bold},
		{text_font,text_fontsize,text_color,text_bgcolor,text_bold},
		{caretline_font,caretline_fontsize,caretline_color,caretline_bgcolor,caretline_bold},
		{indicator_font,indicator_fontsize,indicator_color,indicator_bgcolor,indicator_bold},
		{keywords_font,keywords_fontsize,keywords_color,keywords_bgcolor,keywords_bold},
		{keywords2_font,keywords2_fontsize,keywords2_color,keywords2_bgcolor,keywords2_bold},
		{string_font,string_fontsize,string_color,string_bgcolor,string_bold},
		{character_font,character_fontsize,character_color,character_bgcolor,character_bold},
		{number_font,number_fontsize,number_color,number_bgcolor,number_bold},
		{operator_font,operator_fontsize,operator_color,operator_bgcolor,operator_bold},
		{preprocessor_font,preprocessor_fontsize,preprocessor_color,preprocessor_bgcolor,preprocessor_bold},
		{comment_font,comment_fontsize,comment_color,comment_bgcolor,comment_bold},
		{commentline_font,commentline_fontsize,commentline_color,commentline_bgcolor,commentline_bold},
		{commentdoc_font,commentdoc_fontsize,commentdoc_color,commentdoc_bgcolor,commentdoc_bold},
		{tags_font,tags_fontsize,tags_color,tags_bgcolor,tags_bold},
		{unknowtags_font,unknowtags_fontsize,unknowtags_color,unknowtags_bgcolor,unknowtags_bold},
		{attributes_font,attributes_fontsize,attributes_color,attributes_bgcolor,attributes_bold},
		{unknowattributes_font,unknowattributes_fontsize,unknowattributes_color,unknowattributes_bgcolor,unknowattributes_bold},
		{entities_font,entities_fontsize,entities_color,entities_bgcolor,entities_bold},
		{tagends_font,tagends_fontsize,tagends_color,tagends_bgcolor,tagends_bold},
		{cdata_font,cdata_fontsize,cdata_color,cdata_bgcolor,cdata_bold},
		{phpsection_font,phpsection_fontsize,phpsection_color,phpsection_bgcolor,phpsection_bold},
		{aspsection_font,aspsection_fontsize,aspsection_color,aspsection_bgcolor,aspsection_bold}
	  }
	})
	return emod.euapi.eu_theme_ptr(m_theme, true)
end

return emod
