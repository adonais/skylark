eu_theme = {}

require("eu_core")

function eu_theme.get_default(name)
    local theme = nil
    if (name == "black") then
        theme = -- 暗黑主题配置文件
          "linenumber_font = \"Consolas\"\n" ..
          "linenumber_fontsize = 9\n" ..
          "linenumber_color = 0x00A0A0A0\n" ..
          "linenumber_bgcolor = 0x00111111\n" ..
          "linenumber_bold = 0\n" ..
          "foldmargin_font = \"Consolas\"\n" ..
          "foldmargin_fontsize = 9\n" ..
          "foldmargin_color = 0x00000000\n" ..
          "foldmargin_bgcolor = 0x00111111\n" ..
          "foldmargin_bold = 0\n" ..
          "text_font = \"Consolas\"\n" ..
          "text_fontsize = 11\n" ..
          "text_color = 0x00D4D4D4\n" ..
          "text_bgcolor = 0x001E1E1E\n" ..
          "text_bold = 0\n" ..
          "caretline_font = \"Consolas\"\n" ..
          "caretline_fontsize = 11\n" ..
          "caretline_color = 0x00000000\n" ..
          "caretline_bgcolor = 0x5A696969\n" ..
          "caretline_bold = 0\n" ..
          "indicator_font = \"Consolas\"\n" ..
          "indicator_fontsize = 11\n" ..
          "indicator_color = 0x00FFFFFF\n" ..
          "indicator_bgcolor = 0x5A808000\n" ..
          "indicator_bold = 0\n" ..
          "keywords_font = \"Consolas\"\n" ..
          "keywords_fontsize = 11\n" ..
          "keywords_color = 0x00FF8000\n" ..
          "keywords_bgcolor = 0x00000000\n" ..
          "keywords_bold = 1\n" ..
          "keywords2_font = \"Consolas\"\n" ..
          "keywords2_fontsize = 11\n" ..
          "keywords2_color = 0x00C08000\n" ..
          "keywords2_bgcolor = 0x00000000\n" ..
          "keywords2_bold = 1\n" ..
          "string_font = \"Consolas\"\n" ..
          "string_fontsize = 11\n" ..
          "string_color = 0x00C080FF\n" ..
          "string_bgcolor = 0x00000000\n" ..
          "string_bold = 0\n" ..
          "character_font = \"Consolas\"\n" ..
          "character_fontsize = 11\n" ..
          "character_color = 0x00C080FF\n" ..
          "character_bgcolor = 0x00000000\n" ..
          "character_bold = 0\n" ..
          "number_font = \"Consolas\"\n" ..
          "number_fontsize = 11\n" ..
          "number_color = 0x00C080FF\n" ..
          "number_bgcolor = 0x00000000\n" ..
          "number_bold = 0\n" ..
          "operator_font = \"Consolas\"\n" ..
          "operator_fontsize = 11\n" ..
          "operator_color = 0x00B0B0B0\n" ..
          "operator_bgcolor = 0x00000000\n" ..
          "operator_bold = 0\n" ..
          "preprocessor_font = \"Consolas\"\n" ..
          "preprocessor_fontsize = 11\n" ..
          "preprocessor_color = 0x00FF8000\n" ..
          "preprocessor_bgcolor = 0x00000000\n" ..
          "preprocessor_bold = 0\n" ..
          "comment_font = \"Consolas\"\n" ..
          "comment_fontsize = 11\n" ..
          "comment_color = 0x00808080\n" ..
          "comment_bgcolor = 0x00000000\n" ..
          "comment_bold = 0\n" ..
          "commentline_font = \"Consolas\"\n" ..
          "commentline_fontsize = 11\n" ..
          "commentline_color = 0x00808080\n" ..
          "commentline_bgcolor = 0x00000000\n" ..
          "commentline_bold = 0\n" ..
          "commentdoc_font = \"Consolas\"\n" ..
          "commentdoc_fontsize = 11\n" ..
          "commentdoc_color = 0x00808080\n" ..
          "commentdoc_bgcolor = 0x00000000\n" ..
          "commentdoc_bold = 0\n" ..
          "tags_font = \"Consolas\"\n" ..
          "tags_fontsize = 11\n" ..
          "tags_color = 0x00FF8000\n" ..
          "tags_bgcolor = 0x00000000\n" ..
          "tags_bold = 0\n" ..
          "unknowtags_font = \"Consolas\"\n" ..
          "unknowtags_fontsize = 11\n" ..
          "unknowtags_color = 0x00804000\n" ..
          "unknowtags_bgcolor = 0x00000000\n" ..
          "unknowtags_bold = 0\n" ..
          "attributes_font = \"Consolas\"\n" ..
          "attributes_fontsize = 11\n" ..
          "attributes_color = 0x0080FF80\n" ..
          "attributes_bgcolor = 0x00000000\n" ..
          "attributes_bold = 0\n" ..
          "unknowattributes_font = \"Consolas\"\n" ..
          "unknowattributes_fontsize = 11\n" ..
          "unknowattributes_color = 0x00808040\n" ..
          "unknowattributes_bgcolor = 0x00000000\n" ..
          "unknowattributes_bold = 0\n" ..
          "entities_font = \"Consolas\"\n" ..
          "entities_fontsize = 11\n" ..
          "entities_color = 0x00800080\n" ..
          "entities_bgcolor = 0x00000000\n" ..
          "entities_bold = 0\n" ..
          "tagends_font = \"Consolas\"\n" ..
          "tagends_fontsize = 11\n" ..
          "tagends_color = 0x000080FF\n" ..
          "tagends_bgcolor = 0x00000000\n" ..
          "tagends_bold = 0\n" ..
          "cdata_font = \"Consolas\"\n" ..
          "cdata_fontsize = 11\n" ..
          "cdata_color = 0x00008000\n" ..
          "cdata_bgcolor = 0x00000000\n" ..
          "cdata_bold = 0\n" ..
          "phpsection_font = \"Consolas\"\n" ..
          "phpsection_fontsize = 11\n" ..
          "phpsection_color = 0x000080FF\n" ..
          "phpsection_bgcolor = 0x00000000\n" ..
          "phpsection_bold = 0\n" ..
          "aspsection_font = \"Consolas\"\n" ..
          "aspsection_fontsize = 11\n" ..
          "aspsection_color = 0x00808080\n" ..
          "aspsection_bgcolor = 0x00000000\n" ..
          "aspsection_bold = 0\n" ..
          "activetab_font = \"DEFAULT_GUI_FONT\"\n" ..
          "activetab_fontsize = 11\n" ..
          "activetab_color = 0\n" ..
          "activetab_bgcolor = 0x00545454\n" ..
          "activetab_bold = 0\n" ..
          "caret_font = \"Consolas\"\n" ..
          "caret_fontsize = 11\n" ..
          "caret_color = 0x020A99FF\n" ..
          "caret_bgcolor = 0x001E1E1E\n" ..
          "caret_bold = 500\n" ..
          "symbolic_font = \"Consolas\"\n" ..
          "symbolic_fontsize = 13\n" ..
          "symbolic_color = 0x00D4D4D4\n" ..
          "symbolic_bgcolor = 0x001E1E1E\n" ..
          "symbolic_bold = 0\n" ..
          "hyperlink_font = \"Consolas\"\n" ..
          "hyperlink_fontsize = 11\n" ..
          "hyperlink_color = 0x3C80FFFF\n" ..
          "hyperlink_bgcolor = 0xB4FF8C2F\n" ..
          "hyperlink_bold = 1"
    elseif (name == "white") then
        theme = -- 经典白主题配置文件
          "linenumber_font = \"Consolas\"\n" ..
          "linenumber_fontsize = 9\n" ..
          "linenumber_color = 0x00111111\n" ..
          "linenumber_bgcolor = 0x00DDDDDD\n" ..
          "linenumber_bold = 0\n" ..
          "foldmargin_font = \"Consolas\"\n" ..
          "foldmargin_fontsize = 9\n" ..
          "foldmargin_color = 0x00000000\n" ..
          "foldmargin_bgcolor = 0x00EEEEEE\n" ..
          "foldmargin_bold = 0\n" ..
          "text_font = \"Consolas\"\n" ..
          "text_fontsize = 11\n" ..
          "text_color = 0x00000000\n" ..
          "text_bgcolor = 0x00FFFFFF\n" ..
          "text_bold = 0\n" ..
          "caretline_font = \"Consolas\"\n" ..
          "caretline_fontsize = 11\n" ..
          "caretline_color = 0x00000000\n" ..
          "caretline_bgcolor = 0x5A808080\n" ..
          "caretline_bold = 0\n" ..
          "indicator_font = \"Consolas\"\n" ..
          "indicator_fontsize = 11\n" ..
          "indicator_color = 0x00FFFFFF\n" ..
          "indicator_bgcolor = 0x5A808000\n" ..
          "indicator_bold = 0\n" ..
          "keywords_font = \"Consolas\"\n" ..
          "keywords_fontsize = 11\n" ..
          "keywords_color = 0x00FF8000\n" ..
          "keywords_bgcolor = 0x00000000\n" ..
          "keywords_bold = 1\n" ..
          "keywords2_font = \"Consolas\"\n" ..
          "keywords2_fontsize = 11\n" ..
          "keywords2_color = 0x00C08000\n" ..
          "keywords2_bgcolor = 0x00000000\n" ..
          "keywords2_bold = 1\n" ..
          "string_font = \"Consolas\"\n" ..
          "string_fontsize = 11\n" ..
          "string_color = 0x001515A3\n" ..
          "string_bgcolor = 0x00000000\n" ..
          "string_bold = 0\n" ..
          "character_font = \"Consolas\"\n" ..
          "character_fontsize = 11\n" ..
          "character_color = 0x00C080FF\n" ..
          "character_bgcolor = 0x00000000\n" ..
          "character_bold = 0\n" ..
          "number_font = \"Consolas\"\n" ..
          "number_fontsize = 11\n" ..
          "number_color = 0x00588609\n" ..
          "number_bgcolor = 0x00000000\n" ..
          "number_bold = 0\n" ..
          "operator_font = \"Consolas\"\n" ..
          "operator_fontsize = 11\n" ..
          "operator_color = 0x00B0B0B0\n" ..
          "operator_bgcolor = 0x00000000\n" ..
          "operator_bold = 0\n" ..
          "preprocessor_font = \"Consolas\"\n" ..
          "preprocessor_fontsize = 11\n" ..
          "preprocessor_color = 0x00FF8000\n" ..
          "preprocessor_bgcolor = 0x00000000\n" ..
          "preprocessor_bold = 0\n" ..
          "comment_font = \"Consolas\"\n" ..
          "comment_fontsize = 11\n" ..
          "comment_color = 0x00808080\n" ..
          "comment_bgcolor = 0x00000000\n" ..
          "comment_bold = 0\n" ..
          "commentline_font = \"Consolas\"\n" ..
          "commentline_fontsize = 11\n" ..
          "commentline_color = 0x00808080\n" ..
          "commentline_bgcolor = 0x00000000\n" ..
          "commentline_bold = 0\n" ..
          "commentdoc_font = \"Consolas\"\n" ..
          "commentdoc_fontsize = 11\n" ..
          "commentdoc_color = 0x00808080\n" ..
          "commentdoc_bgcolor = 0x00000000\n" ..
          "commentdoc_bold = 0\n" ..
          "tags_font = \"Consolas\"\n" ..
          "tags_fontsize = 11\n" ..
          "tags_color = 0x00FF8000\n" ..
          "tags_bgcolor = 0x00000000\n" ..
          "tags_bold = 0\n" ..
          "unknowtags_font = \"Consolas\"\n" ..
          "unknowtags_fontsize = 11\n" ..
          "unknowtags_color = 0x00804000\n" ..
          "unknowtags_bgcolor = 0x00000000\n" ..
          "unknowtags_bold = 0\n" ..
          "attributes_font = \"Consolas\"\n" ..
          "attributes_fontsize = 11\n" ..
          "attributes_color = 0x00AD5104\n" ..
          "attributes_bgcolor = 0x00000000\n" ..
          "attributes_bold = 0\n" ..
          "unknowattributes_font = \"Consolas\"\n" ..
          "unknowattributes_fontsize = 11\n" ..
          "unknowattributes_color = 0x00808040\n" ..
          "unknowattributes_bgcolor = 0x00000000\n" ..
          "unknowattributes_bold = 0\n" ..
          "entities_font = \"Consolas\"\n" ..
          "entities_fontsize = 11\n" ..
          "entities_color = 0x00800080\n" ..
          "entities_bgcolor = 0x00000000\n" ..
          "entities_bold = 0\n" ..
          "tagends_font = \"Consolas\"\n" ..
          "tagends_fontsize = 11\n" ..
          "tagends_color = 0x00000080\n" ..
          "tagends_bgcolor = 0x00000000\n" ..
          "tagends_bold = 0\n" ..
          "cdata_font = \"Consolas\"\n" ..
          "cdata_fontsize = 11\n" ..
          "cdata_color = 0x00008000\n" ..
          "cdata_bgcolor = 0x00000000\n" ..
          "cdata_bold = 0\n" ..
          "phpsection_font = \"Consolas\"\n" ..
          "phpsection_fontsize = 11\n" ..
          "phpsection_color = 0x00FF0000\n" ..
          "phpsection_bgcolor = 0x00000000\n" ..
          "phpsection_bold = 0\n" ..
          "aspsection_font = \"Consolas\"\n" ..
          "aspsection_fontsize = 11\n" ..
          "aspsection_color = 0x00C0C0C0\n" ..
          "aspsection_bgcolor = 0x00000000\n" ..
          "aspsection_bold = 0\n" ..
          "activetab_font = \"DEFAULT_GUI_FONT\"\n" ..
          "activetab_fontsize = 11\n" ..
          "activetab_color = 0\n" ..
          "activetab_bgcolor = 0x00d77800\n" ..
          "activetab_bold = 0\n" ..
          "caret_font = \"Consolas\"\n" ..
          "caret_fontsize = 11\n" ..
          "caret_color = 0x020A99FF\n" ..
          "caret_bgcolor = 0x00FFFFFF\n" ..
          "caret_bold = 500\n" ..
          "symbolic_font = \"Consolas\"\n" ..
          "symbolic_fontsize = 13\n" ..
          "symbolic_color = 0x00000000\n" ..
          "symbolic_bgcolor = 0x00FFFFFF\n" ..
          "symbolic_bold = 0\n" ..
          "hyperlink_font = \"Consolas\"\n" ..
          "hyperlink_fontsize = 11\n" ..
          "hyperlink_color = 0x3C8C99ED\n" ..
          "hyperlink_bgcolor = 0xB4E26941\n" ..
          "hyperlink_bold = 1"
    else
        theme = -- 默认主题配置文件
          "linenumber_font = \"Consolas\"\n" ..
          "linenumber_fontsize = 9\n" ..
          "linenumber_color = 0x00FFFFFF\n" ..
          "linenumber_bgcolor = 0x004E4C4C\n" ..
          "linenumber_bold = 0\n" ..
          "foldmargin_font = \"Consolas\"\n" ..
          "foldmargin_fontsize = 9\n" ..
          -- 折叠标志(+,-)颜色
          "foldmargin_color = 0x00707070\n" ..
          -- 折叠栏背景色
          "foldmargin_bgcolor = 0x004E4C4C\n" ..
          "foldmargin_bold = 0\n" ..
          "text_font = \"Consolas\"\n" ..
          "text_fontsize = 11\n" ..
          "text_color = 0x00FFFFFF\n" ..
          "text_bgcolor = 0x00444444\n" ..
          "text_bold = 0\n" ..
          "caretline_font = \"Consolas\"\n" ..
          "caretline_fontsize = 11\n" ..
          "caretline_color = 0x00000000\n" ..
          "caretline_bgcolor = 0x5A696969\n" ..
          "caretline_bold = 0\n" ..
          "indicator_font = \"Consolas\"\n" ..
          "indicator_fontsize = 11\n" ..
          "indicator_color = 0x00FFFFFF\n" ..
          "indicator_bgcolor = 0x5A808000\n" ..
          "indicator_bold = 0\n" ..
          "keywords_font = \"Consolas\"\n" ..
          "keywords_fontsize = 11\n" ..
          "keywords_color = 0x0000B050\n" ..
          "keywords_bgcolor = 0x00000000\n" ..
          "keywords_bold = 1\n" ..
          "keywords2_font = \"Consolas\"\n" ..
          "keywords2_fontsize = 11\n" ..
          "keywords2_color = 0x0000B050\n" ..
          "keywords2_bgcolor = 0x00000000\n" ..
          "keywords2_bold = 1\n" ..
          "string_font = \"Consolas\"\n" ..
          "string_fontsize = 11\n" ..
          "string_color = 0x00C080FF\n" ..
          "string_bgcolor = 0x00000000\n" ..
          "string_bold = 0\n" ..
          "character_font = \"Consolas\"\n" ..
          "character_fontsize = 11\n" ..
          "character_color = 0x00C080FF\n" ..
          "character_bgcolor = 0x00000000\n" ..
          "character_bold = 0\n" ..
          "number_font = \"Consolas\"\n" ..
          "number_fontsize = 11\n" ..
          "number_color = 0x00A8CE93\n" ..
          "number_bgcolor = 0x00000000\n" ..
          "number_bold = 0\n" ..
          "operator_font = \"Consolas\"\n" ..
          "operator_fontsize = 11\n" ..
          "operator_color = 0x00FFFFFF\n" ..
          "operator_bgcolor = 0x00000000\n" ..
          "operator_bold = 0\n" ..
          "preprocessor_font = \"Consolas\"\n" ..
          "preprocessor_fontsize = 11\n" ..
          "preprocessor_color = 0x0000B050\n" ..
          "preprocessor_bgcolor = 0x00000000\n" ..
          "preprocessor_bold = 0\n" ..
          "comment_font = \"Consolas\"\n" ..
          "comment_fontsize = 11\n" ..
          "comment_color = 0x00C0C0C0\n" ..
          "comment_bgcolor = 0x00000000\n" ..
          "comment_bold = 0\n" ..
          "commentline_font = \"Consolas\"\n" ..
          "commentline_fontsize = 11\n" ..
          "commentline_color = 0x00C0C0C0\n" ..
          "commentline_bgcolor = 0x00000000\n" ..
          "commentline_bold = 0\n" ..
          "commentdoc_font = \"Consolas\"\n" ..
          "commentdoc_fontsize = 11\n" ..
          "commentdoc_color = 0x00C0C0C0\n" ..
          "commentdoc_bgcolor = 0x00000000\n" ..
          "commentdoc_bold = 0\n" ..
          "tags_font = \"Consolas\"\n" ..
          "tags_fontsize = 11\n" ..
          "tags_color = 0x00FF8000\n" ..
          "tags_bgcolor = 0x00000000\n" ..
          "tags_bold = 0\n" ..
          "unknowtags_font = \"Consolas\"\n" ..
          "unknowtags_fontsize = 11\n" ..
          "unknowtags_color = 0x00FF00FF\n" ..
          "unknowtags_bgcolor = 0x00000000\n" ..
          "unknowtags_bold = 0\n" ..
          "attributes_font = \"Consolas\"\n" ..
          "attributes_fontsize = 11\n" ..
          "attributes_color = 0x0080FF80\n" ..
          "attributes_bgcolor = 0x00000000\n" ..
          "attributes_bold = 0\n" ..
          "unknowattributes_font = \"Consolas\"\n" ..
          "unknowattributes_fontsize = 11\n" ..
          "unknowattributes_color = 0x00808040\n" ..
          "unknowattributes_bgcolor = 0x00000000\n" ..
          "unknowattributes_bold = 0\n" ..
          "entities_font = \"Consolas\"\n" ..
          "entities_fontsize = 11\n" ..
          "entities_color = 0x008000FF\n" ..
          "entities_bgcolor = 0x00000000\n" ..
          "entities_bold = 0\n" ..
          "tagends_font = \"Consolas\"\n" ..
          "tagends_fontsize = 11\n" ..
          "tagends_color = 0x000080FF\n" ..
          "tagends_bgcolor = 0x00000000\n" ..
          "tagends_bold = 0\n" ..
          "cdata_font = \"Consolas\"\n" ..
          "cdata_fontsize = 11\n" ..
          "cdata_color = 0x00008000\n" ..
          "cdata_bgcolor = 0x00000000\n" ..
          "cdata_bold = 0\n" ..
          "phpsection_font = \"Consolas\"\n" ..
          "phpsection_fontsize = 11\n" ..
          "phpsection_color = 0x000080FF\n" ..
          "phpsection_bgcolor = 0x00000000\n" ..
          "phpsection_bold = 0\n" ..
          "aspsection_font = \"Consolas\"\n" ..
          "aspsection_fontsize = 11\n" ..
          "aspsection_color = 0x00808080\n" ..
          "aspsection_bgcolor = 0x00000000\n" ..
          "aspsection_bold = 0\n" ..
          "activetab_font = \"DEFAULT_GUI_FONT\"\n" ..
          "activetab_fontsize = 11\n" ..
          "activetab_color = 0\n" ..
          "activetab_bgcolor = 0x00d77800\n" ..
          "activetab_bold = 0\n" ..
          "caret_font = \"Consolas\"\n" ..
          "caret_fontsize = 11\n" ..
          "caret_color = 0x020A99FF\n" ..
          "caret_bgcolor = 0x00444444\n" ..
          "caret_bold = 500\n" ..
          "symbolic_font = \"Consolas\"\n" ..
          "symbolic_fontsize = 13\n" ..
          "symbolic_color = 0x00FFFFFF\n" ..
          "symbolic_bgcolor = 0x00444444\n" ..
          "symbolic_bold = 0\n" ..
          "hyperlink_font = \"Consolas\"\n" ..
          "hyperlink_fontsize = 11\n" ..
          "hyperlink_color = 0x3C80FFFF\n" ..
          "hyperlink_bgcolor = 0xB4FF8C2F\n" ..
          "hyperlink_bold = 1"
    end
    return theme
end

function eu_theme.write_default(path)
    local m_theme = nil
    local m_default = "default"
    local m_black = "black"
    local m_white = "white"
    local m_file  = (path .. "\\styletheme" .. ".conf")
    if (not eu_core.file_exists(m_file)) then
        m_theme = eu_theme.get_default(m_default)
        eu_core.save_file(m_file, m_theme)
    end
    m_file  = (path .. "\\styletheme_" .. m_black .. ".conf")
    if (not eu_core.file_exists(m_file)) then
        m_theme = eu_theme.get_default(m_black)
        eu_core.save_file(m_file, m_theme)
    end
    m_file  = (path .. "\\styletheme_" .. m_white .. ".conf")
    if (not eu_core.file_exists(m_file)) then
        m_theme = eu_theme.get_default(m_white)
        eu_core.save_file(m_file, m_theme)
    end
end

function eu_theme.load_default(name)
    local tname = ""
    local underline = ""
    local path = eu_core.script_path()
    if (name == nil or name == tname) then
        return false
    elseif (name ~= "default") then
        tname = name
        underline = "_"
    end
    eu_theme.write_default(path)
    local file  = (path .. "\\styletheme" .. underline .. tname .. ".conf")
    if (not eu_core.file_exists(file)) then
        file  = (path .. "\\styletheme" .. ".conf")
        local theme = eu_theme.get_default("default")
        eu_code = assert(loadstring(theme))()
    else
        dofile(file)
        tname = name
    end
    local m_file = eu_core.ffi.new('char[260]')
    eu_core.ffi.C._fullpath(m_file, file, 260)
    local m_theme = eu_core.ffi.new("struct eu_theme", {m_file, tname,
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
        {aspsection_font,aspsection_fontsize,aspsection_color,aspsection_bgcolor,aspsection_bold},
        {activetab_font,activetab_fontsize,activetab_color,activetab_bgcolor,activetab_bold},
        {caret_font,caret_fontsize,caret_color,caret_bgcolor,caret_bold},
        {symbolic_font,symbolic_fontsize,symbolic_color,symbolic_bgcolor,symbolic_bold},
        {hyperlink_font,hyperlink_fontsize,hyperlink_color,hyperlink_bgcolor,hyperlink_bold}
      }
    })
    return eu_core.euapi.eu_theme_ptr(m_theme, true)
end

return eu_theme
