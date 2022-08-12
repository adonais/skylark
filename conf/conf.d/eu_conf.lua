eu_conf = {}

require("eu_core")

function eu_conf.fill_actions(s)
    local pconfig = eu_core.ffi.cast('struct eu_config *', s)
    if (eu_core.table_is_empty(process_actions)) then
        eu_core.ffi.fill(pconfig.m_actions, 100 * 260)
    else
        local actions_size = eu_core.ffi.sizeof(pconfig.m_actions)/260;
        for i=0,actions_size-1 do
          if (process_actions[i]) then
              eu_core.ffi.copy(pconfig.m_actions[i], process_actions[i], 259)
          else
              eu_core.ffi.fill(pconfig.m_actions[i], 260)
          end
        end
    end
end

function eu_conf.loadconf()
    local file = (eu_core.script_path() .. "\\skylark.conf")
    if (not eu_core.file_exists(file)) then
        local code = -- 默认配置文件
        "-- if you edit the file, please keep the encoding correct(utf-8 nobom)\n" ..
        "newfile_eols = 2\n" ..
        "newfile_encoding = 10014\n" ..
        "enable_auto_add_close_char = true\n" ..
        "enable_auto_identation = true\n" ..
        "window_theme = \"default\"\n" ..
        "window_full_screen = false\n" ..
        "window_menubar_visiable = true\n" ..
        "window_toolbar_visiable = true\n" ..
        "window_statusbar_visiable = true\n" ..
        "line_number_visiable = true\n" ..
        "bookmark_visiable = true\n" ..
        "bookmark_shape = 32\n" ..
        "bookmark_argb = 0x28408040\n" ..
        "matching_brace_color = 0x0000FF\n" ..
        "function_calltip_color = 0x768465\n" ..
        "last_search_flags = 0x000044\n" ..
        "white_space_visiable = false\n" ..
        "white_space_size = 3\n" ..
        "newline_visiable = false\n" ..
        "indentation_guides_visiable = true\n" ..
        "tab_width = 4\n" ..
        "onkeydown_tab_convert_spaces = true\n" ..
        "light_fold = true\n" ..
        "wrapline_mode = false\n" ..
        "enable_filetree_show = false\n" ..
        "file_treebar_width = 253\n" ..
        "symbol_list_width = 210\n" ..
        "symbol_tree_width = 210\n" ..
        "document_map_width = 310\n" ..
        "sqlquery_result_edit_height = 80\n" ..
        "sqlquery_result_listview_height = 270\n" ..
        "file_recent_number = 29\n" ..
        "inter_reserved_0 = 0\n" ..
        "inter_reserved_1 = 0\n" ..
        "inter_reserved_2 = 0\n" ..
        "block_fold_visiable = true\n" ..
        "auto_completed_show_enable = true\n" ..
        "auto_completed_show_after_input_characters = 0\n" ..
        "call_tip_show_enable = true\n" ..
        "tabs_tip_show_enable = true\n" ..
        "tab_close_way = 0\n" ..
        "tab_switch_forward = 42991\n" ..
        "edit_font_quality = 42552\n" ..
        "edit_rendering_technology = 42560\n" ..
        "update_file_mask = 0\n" ..
        "light_all_find_str = true\n" ..
        "backup_on_file_write = false\n" ..
        "save_last_session = true\n" ..
        "exit_when_close_last_tab = false\n" ..
        "allow_multiple_instance = false\n" ..
        "save_last_placement = \"\"\n" ..
        "ui_language = \"auto\"\n" ..
        "-- printer default setting\n" ..
        "printer = {\n" ..
        "    header = 1,\n" ..
        "    footer = 0,\n" ..
        "    color_mode = 3,\n" ..
        "    zoom = 0,\n" ..
        "    margin_left = 2000,\n" ..
        "    margin_top = 2000,\n" ..
        "    margin_right = 2000,\n" ..
        "    margin_bottom = 2000\n" ..
        "}\n" ..
        "-- automatically cached file (size < 200MB)\n" ..
        "cache_limit_size = 200\n" ..
        "snippet_enable = 44014\n" ..
        "app_build_id = 0\n" ..
        "-- uses the backslash ( / ) to separate directories in file path. default value: cmd.exe\n" ..
        "process_path = \"\"\n" ..
        "other_editor_path = \"\"\n" ..
        "m_reserved_0 = \"\"\n" ..
        "m_reserved_1 = \"\"\n" ..
        "process_actions = {}\n"
        eu_code = assert(loadstring(code))()
    else
        eu_code = dofile(file)
    end
    local m_bookmark_shape = 32
    local m_bookmark_argb = 0x2408040
    if (bookmark_shape ~= nil and bookmark_shape >=0) then
        m_bookmark_shape = bookmark_shape
    end
    if (bookmark_argb ~= nil and bookmark_argb >=0) then
        m_bookmark_argb = bookmark_argb
    end
    local m_config = eu_core.ffi.new("struct eu_config", {
        newfile_eols,
        newfile_encoding,
        enable_auto_add_close_char,
        enable_auto_identation,
        window_theme,
        window_full_screen,
        window_menubar_visiable,
        window_toolbar_visiable,
        window_statusbar_visiable,
        line_number_visiable,
        bookmark_visiable,
        bookmark_shape,
        bookmark_argb,
        matching_brace_color,
        function_calltip_color,
        last_search_flags,
        white_space_visiable,
        white_space_size,
        newline_visiable,
        indentation_guides_visiable,
        tab_width,
        onkeydown_tab_convert_spaces,
        light_fold,
        wrapline_mode,
        enable_filetree_show,
        file_treebar_width,
        symbol_list_width,
        symbol_tree_width,
        document_map_width,
        sqlquery_result_edit_height,
        sqlquery_result_listview_height,
        file_recent_number,
        inter_reserved_0,
        inter_reserved_1,
        inter_reserved_2,
        block_fold_visiable,
        auto_completed_show_enable,
        auto_completed_show_after_input_characters,
        call_tip_show_enable,
        tabs_tip_show_enable,
        tab_close_way,
        tab_switch_forward,
        edit_font_quality,
        edit_rendering_technology,
        update_file_mask,
        light_all_find_str,
        backup_on_file_write,
        save_last_session,
        exit_when_close_last_tab,
        allow_multiple_instance,
        save_last_placement,
        ui_language,
        {printer.header, printer.footer, printer.color_mode, printer.zoom,{printer.margin_left, printer.margin_top, printer.margin_right, printer.margin_bottom}},
        cache_limit_size,
        snippet_enable,
        app_build_id,
        process_path,
        other_editor_path,
        m_reserved_0,
        m_reserved_1
    })
    eu_conf.fill_actions(m_config)
    if (not eu_core.euapi.eu_config_ptr(m_config)) then
        do return nil end
    end
    printer = nil
    return window_theme
end

return eu_conf
