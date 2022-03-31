require("emod")

local ffi = emod.ffi
local euapi = emod.euapi

function string:split(delimiter)
  local result = {}
  local from  = 1
  local delim_from, delim_to = string.find(self, delimiter, from)
  while delim_from do
    table.insert(result, string.sub(self, from, delim_from-1))
    from = delim_to + 1
    delim_from, delim_to = string.find(self, delimiter, from)
  end
  table.insert(result, string.sub(self, from))
  return result
end

function fetch_doctype(s)
  local m_config = ffi.cast('doctype_t *', s)
  local m_req = nil
  local m_key0,m_key1,m_key2,m_key3,m_key4,m_key5
  local m_set = nil
  local calltip = nil
  local reqular = nil
  local tab_width = nil
  local tab_convert_spaces = nil
  
  if (m_config.filetypename ~= nil) then
    m_req = require(ffi.string(m_config.filetypename));
  end
  if (m_req ~= nil) then
    if (m_req.get_tabattr ~= nil) then
      tab_width,tab_convert_spaces = m_req.get_tabattr()
    end
    if (tab_width == nil) then
      tab_width = 0
    end
    if (tab_convert_spaces == nil) then
      tab_convert_spaces = -1
    end
    m_config.tab_width = tab_width
    m_config.tab_convert_spaces = tab_convert_spaces    
    if (m_req.get_calltip ~= nil) then
      calltip = m_req.get_calltip()
    end
    if (calltip ~= nil) then
      for key, value in ipairs(calltip) do
        local ss = string.split(value, "|")
        local val = string.gsub(ss[2], "\\n", "\n")
        euapi.eu_init_calltip_tree(m_config, ss[1], val)
      end
    end
    if (m_req.get_keywords ~= nil) then
      m_key0,m_key1,m_key2,m_key3,m_key4,m_key5 = m_req.get_keywords()
    end
    if (m_key0 ~= nil) then
      m_config.keywords0 = ffi.cast('char *', m_key0)
    end
    if (m_key1 ~= nil) then
      m_config.keywords1 = ffi.cast('char *', m_key1)
    end
    if (m_key2 ~= nil) then
      m_config.keywords2 = ffi.cast('char *', m_key2)
    end
    if (m_key3 ~= nil) then
      m_config.keywords3 = ffi.cast('char *', m_key3)
    end
    if (m_key4 ~= nil) then
      m_config.keywords4 = ffi.cast('char *', m_key4)
    end
    if (m_key5 ~= nil) then
      m_config.keywords5 = ffi.cast('char *', m_key5)
    end
    if (m_req.get_reqular ~= nil) then
      reqular = m_req.get_reqular()
    end
    if (reqular ~= nil) then
      m_config.reqular_exp = ffi.cast('char *', reqular)
    end
    if (m_req.get_autocomplete ~= nil) then
      m_set = m_req.get_autocomplete()
    end        
    if (m_set ~= nil) then
      local dst = string.split(m_set, " +")
	  for i = 1, #dst do
	    euapi.eu_init_completed_tree(m_config, dst[i])
	  end
    end
  end
end

function lua_init_after_au3(p)
  local pnode = ffi.cast("void *", p)
  local res = euapi.on_doc_enable_scilexer(pnode, 60)                -- 60, SCLEX_AU3
  if (res ~= 1) then
    euapi.on_doc_comment_light(pnode, 1, 0)                          -- 1, SCE_AU3_COMMENT
    euapi.on_doc_commentblock_light(pnode, 2, 0)                     -- 2, SCE_AU3_COMMENTBLOCK
    euapi.on_doc_keyword_light(pnode, 5, 0, 0)                       -- 5, SCE_AU3_KEYWORD, keywords0
    euapi.on_doc_marcro_light(pnode, 6, 2, 0x0080FF)                 -- 6, SCE_AU3_MACRO, keywords2
    euapi.on_doc_string_light(pnode, 7, 0x008080)                    -- 7, SCE_AU3_STRING
    -- euapi.on_doc_operator_light(pnode, 8, 0xC000C0)               -- 8, SCE_AU3_OPERATOR
    euapi.on_doc_variable_light(pnode, 9, 0x808000)                  -- 9, SCE_AU3_VARIABLE
    euapi.on_doc_send_light(pnode, 10, 3, 0xFF0000)                  -- 10, SCE_AU3_SENT, keywords3
    euapi.on_doc_preprocessor_light(pnode, 11, 4, 0xFF8000)          -- 11, SCE_AU3_PREPROCESSOR, keywords4
    euapi.on_doc_special_light(pnode, 10, 0xFF0000)                  -- 12, SCE_AU3_SPECIAL
  end
  return res
end

function lua_init_after_fortran(p)
  local pnode = ffi.cast("void *", p)
  local res = euapi.on_doc_enable_scilexer(pnode, 36)                -- 36, SCLEX_FORTRAN
  if (res ~= 1) then
    euapi.on_doc_keyword_light(pnode, 8, 0, 0)                       -- 8, SCE_F_WORD, keywords0
    euapi.on_doc_keyword_light(pnode, 9, 1, 0)                       -- 9, SCE_F_WORD2, keywords1
    euapi.on_doc_keyword_light(pnode, 10, 2, 0xB000B0)               -- 10, SCE_F_WORD3, keywords2
    euapi.on_doc_commentblock_light(pnode, 1, 0)                     -- 1, SCE_F_COMMENT
    euapi.on_doc_number_light(pnode, 2, 0)                           -- 2, SCE_F_NUMBER
    euapi.on_doc_preprocessor_light(pnode, 11, -1, 0xB000B0)         -- 11, SCE_F_PREPROCESSOR
  end
  return res;
end

function lua_init_after_julia(p)
  local pnode = ffi.cast("void *", p)
  local res = euapi.on_doc_enable_scilexer(pnode, 133)               -- 133, SCLEX_JULIA
  if (res ~= 1) then
    -- euapi.on_doc_default_light(pnode, 0, 0)
    euapi.on_doc_keyword_light(pnode, 3, 0, 0)                       -- 3, SCE_JULIA_KEYWORD1, keywords0
    euapi.on_doc_keyword_light(pnode, 4, 1, 0x0080FF)                -- 4, SCE_JULIA_KEYWORD2, keywords1
    euapi.on_doc_keyword_light(pnode, 5, 2, 0x307300)                -- 5, SCE_JULIA_KEYWORD3, keywords2
    euapi.on_doc_keyword_light(pnode, 20, 3, 0)                      -- 20, SCE_JULIA_KEYWORD4, keywords3
    euapi.on_doc_marcro_light(pnode, 12, 4, 0xFF8000)                -- 12, SCE_JULIA_MACRO, keywords4
    
    euapi.on_doc_comment_light(pnode, 1, 0)                          -- 1, SCE_JULIA_COMMENT
    euapi.on_doc_string_light(pnode, 14, 0x008080)                   -- 14, SCE_JULIA_DOCSTRING
    euapi.on_doc_string_light(pnode, 10, 0x008000)                   -- 10, SCE_JULIA_STRING
  end
  return res
end

function fill_my_docs()
  local ffi_null = ffi.cast("void *", nil)
  local my_doc_config = ffi.new ("doctype_t [34]",
    {
      {
          1,
          "asm",
          ";*.asm;",
          "Assembly",
          0,
          -1,
          euapi.on_doc_init_list,
          euapi.on_doc_init_after_asm,
          ffi_null,
          ffi_null,
          euapi.on_doc_keyup_general,
          euapi.on_doc_cpp_like,
          euapi.on_doc_reload_list_reqular,
          euapi.on_doc_click_list_jmp,
          ffi_null,
          ffi_null,
      },
      {
          2,
          "au3",
          ";*.au3;",
          "Autoit3 Script",
          0,
          -1,
          ffi_null,
          lua_init_after_au3,
          ffi_null,
          ffi_null,
          euapi.on_doc_keyup_general,
          euapi.on_doc_cpp_like,
          ffi_null,
          ffi_null,
          ffi_null,
          ffi_null,
      },
      {
          3,
          "cshape",
          ";*.cs;",
          "C#",
          0,
          -1,
          euapi.on_doc_init_list,
          euapi.on_doc_init_after_cs,
          ffi_null,
          euapi.on_doc_keydown_jmp,
          euapi.on_doc_keyup_general,
          euapi.on_doc_cpp_like,
          euapi.on_doc_reload_list_reqular,
          euapi.on_doc_click_list_jmp,
          ffi_null,
          ffi_null,
      },
      {
          4,
          "cpp",
          ";*.h;*.hh;*.hpp;*.c;*.cc;*.cpp;*.cxx;*.rc;*.rc2;*.dlg;",
          "C/C++",
          0,
          -1,
          euapi.on_doc_init_list,
          euapi.on_doc_init_after_cpp,
          ffi_null,
          euapi.on_doc_keydown_jmp,
          euapi.on_doc_keyup_general,
          euapi.on_doc_cpp_like,
          euapi.on_doc_reload_list_reqular,
          euapi.on_doc_click_list_jmp,
          ffi_null,
          ffi_null,
      },
      {
          5,
          "cmake",
          ";CMakeLists.txt;*.cmake;*.ctest;CMakeLists;",
          "CMake",
          0,
          -1,
          ffi_null,
          euapi.on_doc_init_after_cmake,
          ffi_null,
          ffi_null,
          euapi.on_doc_keyup_general,
          euapi.on_doc_cmake_like,
          ffi_null,
          ffi_null,
          ffi_null,
          ffi_null,
      },
      {
          6,
          "css",
          ";*.css;*.scss;*.less;*.hss;",
          "CSS",
          0,
          -1,
          ffi_null,
          euapi.on_doc_init_after_css,
          ffi_null,
          ffi_null,
          euapi.on_doc_keyup_general,
          euapi.on_doc_css_like,
          ffi_null,
          ffi_null,
          ffi_null,
          ffi_null,
      },
      {
          7,
          "cobol",
          ";*.cobol;*.cob;",
          "Cobol",
          0,
          -1,
          euapi.on_doc_init_list,
          euapi.on_doc_init_after_cobol,
          ffi_null,
          ffi_null,
          euapi.on_doc_keyup_general,
          euapi.on_doc_cpp_like,
          euapi.on_doc_reload_list_reqular,
          euapi.on_doc_click_list_jmp,
          ffi_null,
          ffi_null,
      },
      {
          8,
          ffi_null,
          ";*.diff;*.patch;",
          "Diff File",
          0,
          -1,
          ffi_null,
          euapi.on_doc_init_after_diff,
          ffi_null,
          ffi_null,
          euapi.on_doc_keyup_general,
          euapi.on_doc_identation,
          ffi_null,
          ffi_null,
          ffi_null,
          ffi_null,
      },
      {
          9,
          "fortran",
          ";*.f;*.for;*.ftn;*.fpp;*.f90;*.f95;*.f03;*.f08;*.f2k;*.hf;",
          "Fortran Source",
          0,
          -1,
          euapi.on_doc_init_list,
          lua_init_after_fortran,
          ffi_null,
          ffi_null,
          euapi.on_doc_keyup_general,
          euapi.on_doc_cpp_like,
          euapi.on_doc_reload_list_reqular,
          euapi.on_doc_click_list_jmp,
          ffi_null,
          ffi_null,
      },
      {
          10,
          "golang",
          ";*.go;",
          "Golang",
          0,
          -1,
          euapi.on_doc_init_list,
          euapi.on_doc_init_after_go,
          ffi_null,
          euapi.on_doc_keydown_jmp,
          euapi.on_doc_keyup_general,
          euapi.on_doc_cpp_like,
          euapi.on_doc_reload_list_reqular,
          euapi.on_doc_click_list_jmp,
          ffi_null,
          ffi_null,
      },
      {
          11,
          "html",
          ";*.html;*.htm;*.shtml;*.xhtml;*.phtml;*.htt;*.htd;*.hta;*.asp;*.php;",
          "HTML",
          0,
          -1,
          ffi_null,
          euapi.on_doc_init_after_html,
          ffi_null,
          ffi_null,
          euapi.on_doc_keyup_general,
          euapi.on_doc_html_like,
          ffi_null,
          ffi_null,
          ffi_null,
          ffi_null,
      },
      {
          12,
          "json",
          ";*.json;*.eslintrc;*.jshintrc;*.jsonld;*.ipynb;*.babelrc;*.prettierrc;*.stylelintrc;*.jsonc;*jscop;",
          "JSON",
          0,
          -1,
          euapi.on_doc_init_tree,
          euapi.on_doc_init_after_json,
          ffi_null,
          ffi_null,
          euapi.on_doc_keyup_general,
          euapi.on_doc_json_like,
          ffi_null,
          ffi_null,
          euapi.on_doc_reload_tree_json,
          euapi.on_doc_click_tree_json,
      },
      {
          13,
          "java",
          ";*.java;*.jad;*.pde;",
          "Java",
          0,
          -1,
          euapi.on_doc_init_list,
          euapi.on_doc_init_after_java,
          ffi_null,
          euapi.on_doc_keydown_jmp,
          euapi.on_doc_keyup_general,
          euapi.on_doc_cpp_like,
          euapi.on_doc_reload_list_reqular,
          euapi.on_doc_click_list_jmp,
          ffi_null,
          ffi_null,
      },
      {
          14,
          "javascript",
          ";*.js;*.es;*.ts;*.jse;*.jsm;*.mjs;*.qs;",
          "JavaScript",
          0,
          -1,
          euapi.on_doc_init_list,
          euapi.on_doc_init_after_js,
          ffi_null,
          euapi.on_doc_keydown_jmp,
          euapi.on_doc_keyup_general,
          euapi.on_doc_cpp_like,
          euapi.on_doc_reload_list_reqular,
          euapi.on_doc_click_list_jmp,
          ffi_null,
          ffi_null,
      },
      {
          15,
          "julia",
          ";*.jl;",
          "Julia Script",
          0,
          -1,
          euapi.on_doc_init_list,
          lua_init_after_julia,
          ffi_null,
          ffi_null,
          euapi.on_doc_keyup_general,
          euapi.on_doc_cpp_like,
          euapi.on_doc_reload_list_reqular,
          euapi.on_doc_click_list_jmp,
          ffi_null,
          ffi_null,
      },
      {
          16,
          "lisp",
          ";*.lsp;",
          "Lisp",
          0,
          -1,
          euapi.on_doc_init_list,
          euapi.on_doc_init_after_lisp,
          ffi_null,
          ffi_null,
          euapi.on_doc_keyup_general,
          euapi.on_doc_cpp_like,
          euapi.on_doc_reload_list_reqular,
          euapi.on_doc_click_list_jmp,
          ffi_null,
          ffi_null,
      },
      {
          17,
          "log",
          ";*.log;changelog;",
          "Log File",
          0,
          -1,
          ffi_null,
          euapi.on_doc_init_after_log,
          ffi_null,
          ffi_null,
          ffi_null,
          euapi.on_doc_identation,
          ffi_null,
          ffi_null,
          ffi_null,
          ffi_null,
      },
      {
          18,
          "luascript",
          ";*.lua;",
          "Lua",
          0,
          -1,
          euapi.on_doc_init_result_list,
          euapi.on_doc_init_after_lua,
          ffi_null,
          ffi_null,
          euapi.on_doc_keyup_general,
          euapi.on_doc_cpp_like,
          euapi.on_doc_reload_list_reqular,
          euapi.on_doc_click_list_jmp,
          ffi_null,
          ffi_null,
      },
      {
          19,
          "makefile",
          ";*.gcc;*.msvc;*.msc;*.mk;*.mak;*.configure;*.mozbuild;*.build;Makefile;configure;",
          "Makefile",
          0,
          -1,
          ffi_null,
          euapi.on_doc_init_after_makefile,
          ffi_null,
          ffi_null,
          euapi.on_doc_keyup_general,
          euapi.on_doc_makefile_like,
          ffi_null,
          ffi_null,
          ffi_null,
          ffi_null,
      },
      {
          20,
          ffi_null,
          ";*.md;*.markdown;readme;",
          "Markdown",
          0,
          -1,
          ffi_null,
          euapi.on_doc_init_after_markdown,
          ffi_null,
          ffi_null,
          euapi.on_doc_keyup_general,
          euapi.on_doc_markdown_like,
          ffi_null,
          ffi_null,
          ffi_null,
          ffi_null,
      },
      {
          21,
          "nim",
          ";*.nim;",
          "Nim File",
          0,
          -1,
          euapi.on_doc_init_list,
          euapi.on_doc_init_after_nim,
          ffi_null,
          ffi_null,
          euapi.on_doc_keyup_general,
          euapi.on_doc_cpp_like,
          euapi.on_doc_reload_list_reqular,
          euapi.on_doc_click_list_jmp,
          ffi_null,
          ffi_null,
      },
      {
          22,
          "perl",
          ";*.pl;*.perl;",
          "Perl",
          0,
          -1,
          euapi.on_doc_init_list,
          euapi.on_doc_init_after_perl,
          ffi_null,
          ffi_null,
          euapi.on_doc_keyup_general,
          euapi.on_doc_cpp_like,
          euapi.on_doc_reload_list_reqular,
          euapi.on_doc_click_list_jmp,
          ffi_null,
          ffi_null,
      },
      {
          23,
          ffi_null,
          ";*.properties;*.ini;*.inf;*.cfg;*.cnf;*.conf;",
          "Properties File",
          0,
          -1,
          ffi_null,
          euapi.on_doc_init_after_properties,
          ffi_null,
          ffi_null,
          ffi_null,
          ffi_null,
          ffi_null,
          ffi_null,
          ffi_null,
          ffi_null,
      },
      {
          24,
          "python",
          ";*.py;*.pyw;*.pyx;*.pxd;*.pxi;",
          "Python",
          0,
          -1,
          euapi.on_doc_init_list,
          euapi.on_doc_init_after_python,
          ffi_null,
          ffi_null,
          euapi.on_doc_keyup_general,
          euapi.on_doc_cpp_like,
          euapi.on_doc_reload_list_reqular,
          euapi.on_doc_click_list_jmp,
          ffi_null,
          ffi_null,
      },
      {
          25,
          "redis",
          ";*.redis;",
          "Redis",
          0,
          -1,
          euapi.on_doc_init_tree,
          euapi.on_doc_init_after_redis,
          ffi_null,
          euapi.on_doc_keydown_redis,
          euapi.on_doc_keyup_general,
          euapi.on_doc_redis_like,
          ffi_null,
          ffi_null,
          euapi.on_doc_reload_tree_redis,
          euapi.on_doc_click_tree_redis,
      },
      {
          26,
          "ruby",
          ";*.rb;",
          "Ruby",
          0,
          -1,
          euapi.on_doc_init_list,
          euapi.on_doc_init_after_ruby,
          ffi_null,
          ffi_null,
          euapi.on_doc_keyup_general,
          euapi.on_doc_cpp_like,
          euapi.on_doc_reload_list_reqular,
          euapi.on_doc_click_list_jmp,
          ffi_null,
          ffi_null,
      },
      {
          27,
          "rust",
          ";*.rs;",
          "Rust",
          0,
          -1,
          euapi.on_doc_init_list,
          euapi.on_doc_init_after_rust,
          ffi_null,
          ffi_null,
          euapi.on_doc_keyup_general,
          euapi.on_doc_cpp_like,
          euapi.on_doc_reload_list_reqular,
          euapi.on_doc_click_list_jmp,
          ffi_null,
          ffi_null,
      },
      {
          28,
          "sql",
          ";*.sql;*.prc;",
          "SQL",
          0,
          -1,
          euapi.on_doc_init_result,
          euapi.on_doc_init_after_sql,
          ffi_null,
          euapi.on_doc_keydown_sql,
          euapi.on_doc_keyup_general,
          euapi.on_doc_sql_like,
          ffi_null,
          ffi_null,
          euapi.on_doc_reload_tree_sql,
          euapi.on_doc_click_tree_sql,
      },
      {
          29,
          "shell",
          ";*.bat;*.cmd;*.nt;*.ps1;*.psc1;*.psd1;*.psm1;*.sh;*.mozconfig;",
          "Shell",
          0,
          -1,
          euapi.on_doc_init_list_sh,
          euapi.on_doc_init_after_shell_sh,
          ffi_null,
          ffi_null,
          euapi.on_doc_keyup_general_sh,
          euapi.on_doc_cpp_like,
          euapi.on_doc_reload_list_sh,
          euapi.on_doc_click_list_jump_sh,
          ffi_null,
          ffi_null,
      },
      {
          30,
          "swift",
          ";*.swift;",
          "Swift",
          0,
          -1,
          euapi.on_doc_init_list,
          euapi.on_doc_init_after_swift,
          ffi_null,
          euapi.on_doc_keydown_jmp,
          euapi.on_doc_keyup_general,
          euapi.on_doc_cpp_like,
          euapi.on_doc_reload_list_reqular,
          euapi.on_doc_click_list_jmp,
          ffi_null,
          ffi_null,
      },                                                                                          
      {
          31,
          ffi_null,
          ";*.txt;",
          "Text",
          0,
          -1,
          ffi_null,
          ffi_null,
          ffi_null,
          ffi_null,
          ffi_null,
          euapi.on_doc_identation,
          ffi_null,
          ffi_null,
          ffi_null,
          ffi_null,
      },
      {
          32,
          ffi_null,
          ";*.xml;*.xsl;*.svg;*.xul;*.xsd;*.dtd;*.xslt;*.axl;*.xrc;*.rdf;*.manifest;",
          "XML",
          0,
          -1,
          ffi_null,
          euapi.on_doc_init_after_xml,
          ffi_null,
          ffi_null,
          euapi.on_doc_keyup_general,
          euapi.on_doc_xml_like,
          ffi_null,
          ffi_null,
          ffi_null,
          ffi_null,
      },
      {
          33,
          ffi_null,
          ";*.yaml;*.yml;*.clang-tidy;*.mir;*.apinotes;*.ifs;*.clang-format;_clang-format;",
          "YAML",
          0,
          -1,
          ffi_null,
          euapi.on_doc_init_after_yaml,
          ffi_null,
          ffi_null,
          euapi.on_doc_keyup_general,
          euapi.on_doc_identation,
          ffi_null,
          ffi_null,
          ffi_null,
          ffi_null,
      },
      {
          0,
          ffi_null,
      }
    })
  local my_size = ffi.sizeof(my_doc_config)/ffi.sizeof("doctype_t")
  for i=0,my_size-1 do
    fetch_doctype(my_doc_config[i])
  end
  local doc_ptr = tonumber(ffi.cast("uintptr_t", my_doc_config))
  return doc_ptr
end

--[=[
Using Lua script, you can add syntax parser by yourself. 
The above is an example.
]=]
