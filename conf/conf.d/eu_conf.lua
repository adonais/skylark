eu_conf = {}

require("eu_core")

function eu_conf.fill_actions(s)
    local pconfig = eu_core.ffi.cast('struct eu_config *', s)
    if (eu_core.table_is_empty(process_actions)) then
        eu_core.ffi.fill(pconfig.m_actions, 100 * 260)
    else
        local actions_size = eu_core.ffi.sizeof(pconfig.m_actions)/260;
        for i=0,actions_size-1 do
          if (process_actions[i] ~= nil and #process_actions[i] < 260) then
              eu_core.ffi.copy(pconfig.m_actions[i], process_actions[i])
          else
              eu_core.ffi.fill(pconfig.m_actions[i], 260)
          end
        end
    end
end

function eu_conf.fill_customize(s)
    local pconfig = eu_core.ffi.cast('struct eu_config *', s)
    local msize = eu_core.ffi.sizeof(pconfig.m_customize)/eu_core.ffi.sizeof(pconfig.m_customize[0]);
    if (eu_core.table_is_empty(process_customized)) then
        process_customized[1] = {['hide'] = false, ['name'] = "44500", ['path'] = "conf/conf.d/eu_evaluation.lua",
                                 ['param'] = "%CURRENT_SELSTR% %NUM_SELSTR%", ['micon'] = 44305, ['posid'] = 0, ['hbmp'] = 0}
        process_customized[2] = {['hide'] = false, ['name'] = "44501", ['path'] = "",
                                 ['param'] = "", ['micon'] = 0, ['posid'] = 0, ['hbmp'] = 0}
        if (eu_core.euapi.eu_under_wine()) then
          process_customized[2].path = "calc"
        elseif (eu_core.euapi.eu_which("win32calc.exe")) then
          process_customized[2].path = "%windir%/system32/win32calc.exe"
        else
          process_customized[2].path = "%windir%/system32/calc.exe"
        end
    end
    for i=0,msize-1 do
      if process_customized[i+1] ~= nil then
        if process_customized[i+1].hide ~= nil then
          pconfig.m_customize[i].hide = process_customized[i+1].hide
        else
          pconfig.m_customize[i].hide = false
        end
        if process_customized[i+1].name ~= nil then
          eu_core.ffi.copy(pconfig.m_customize[i].name, process_customized[i+1].name, 64)
        else
          eu_core.ffi.fill(pconfig.m_customize[i].name, 64)
        end
        if process_customized[i+1].path ~= nil then
          eu_core.ffi.copy(pconfig.m_customize[i].path, process_customized[i+1].path, 260)
        else
          eu_core.ffi.fill(pconfig.m_customize[i].path, 260)
        end
        if process_customized[i+1].param ~= nil then
          eu_core.ffi.copy(pconfig.m_customize[i].param, process_customized[i+1].param, 260)
        else
          eu_core.ffi.fill(pconfig.m_customize[i].param, 260)
        end
        if process_customized[i+1].micon ~= nil then
          pconfig.m_customize[i].micon = process_customized[i+1].micon
        else
          pconfig.m_customize[i].micon = 0
        end
        pconfig.m_customize[i].posid = 0
        pconfig.m_customize[i].hbmp = 0
      else
        eu_core.ffi.fill(pconfig.m_customize[i], eu_core.ffi.sizeof(pconfig.m_customize)/32)
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
        "enable_auto_identation = true\n" ..
        "window_theme = \"default\"\n" ..
        "window_full_screen = false\n" ..
        "window_menubar_visiable = true\n" ..
        "window_toolbar_visiable = 30031\n" ..
        "window_statusbar_visiable = true\n" ..
        "line_number_visiable = true\n" ..
        "last_search_flags = 0x000044\n" ..
        "history_mask = 44711\n" ..
        "white_space_visiable = false\n" ..
        "white_space_size = 2\n" ..
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
        "sidebar_width = 320\n" ..
        "document_map_width = 140\n" ..
        "sqlquery_result_edit_height = 80\n" ..
        "sqlquery_result_listview_height = 270\n" ..
        "file_recent_number = 29\n" ..
        "scroll_to_cursor = false\n" ..
        "inter_reserved_0 = 0\n" ..
        "inter_reserved_1 = 0\n" ..
        "inter_reserved_2 = 0\n" ..
        "block_fold_visiable = true\n" ..
        "tabs_tip_show_enable = true\n" ..
        "code_hint_show_enable = true\n" ..
        "tab_split_show = false\n" ..
        "tab_close_way = 0\n" ..
        "tab_close_draw = 43004\n" ..
        "tab_new_way = 0\n" ..
        "tab_switch_forward = 42991\n" ..
        "edit_font_quality = 42552\n" ..
        "edit_rendering_technology = 42560\n" ..
        "update_file_mask = 0\n" ..
        "update_file_notify = 0\n" ..
        "light_all_find_str = true\n" ..
        "backup_on_file_write = false\n" ..
        "save_last_session = true\n" ..
        "exit_when_close_last_tab = false\n" ..
        "allow_multiple_instance = false\n" ..
        "enable_runtime_logging = false\n" ..
        "save_last_placement = \"\"\n" ..
        "ui_language = \"auto\"\n" ..
        "-- bookmark default setting\n" ..
        "bookmark = {\n" ..
        "    visable = true,\n" ..
        "    shape = 32,\n" ..
        "    argb = 0x28408040\n" ..
        "}\n" ..
        "-- brace default setting\n" ..
        "brace = {\n" ..
        "    matching = true,\n" ..
        "    autoc = true,\n" ..
        "    rgb = 0x000000FF\n" ..
        "}\n" ..
        "-- calltip default setting\n" ..
        "calltip = {\n" ..
        "    enable = true,\n" ..
        "    rgb = 0x00768465\n" ..
        "}\n" ..
        "-- auto complete default setting\n" ..
        "complete = {\n" ..
        "    enable = true,\n" ..
        "    characters = 1,\n" ..
        "    snippet = 44014\n" ..
        "}\n" ..
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
        "-- titlebar default setting\n" ..
        "titlebar = {\n" ..
        "    icon = true,\n" ..
        "    name = true,\n" ..
        "    path = true\n" ..
        "}\n" ..
        "-- Master Slave view default setting\n" ..
        "mstab = {\n" ..
        "    vertical = false,\n" ..
        "    horizontal = false,\n" ..
        "    slave_focus = false,\n" ..
        "    show = false,\n" ..
        "    main_size = 0,\n" ..
        "    slave_size = 0\n" ..
        "}\n" ..
        "-- hyperlink hotspot default setting\n" ..
        "hyperlink_detection = true\n" ..
        "-- automatically cached file (size < 200MB)\n" ..
        "cache_limit_size = 200\n" ..
        "app_upgrade = {\n" ..
        "    enable = true,\n" ..
        "    flags = 0,\n" ..
        "    msg_id = 44054,\n" ..
        "    last_check = 0,\n" ..
        "    url = 'https://sourceforge.net/projects/libportable/files/Skylark/update_info.txt/download',\n" ..
        "}\n" ..
        "-- uses the backslash ( / ) to separate directories in file path. default value: cmd.exe\n" ..
        "process_path = \"\"\n" ..
        "other_editor_path = \"\"\n" ..
        "m_reserved_0 = \"\"\n" ..
        "m_reserved_1 = \"\"\n" ..
        "process_actions = {}\n" ..
        "-- setup custom processes\n" ..
        "process_customized = {}\n"
        eu_code = assert(loadstring(code))()
    else
        eu_code = dofile(file)
    end
    -- Add new preference
    if (titlebar == nil) then
        titlebar = {["icon"] = true, ["name"] = true, ["path"] = true}
    end
    if (tab_split_show == nil) then
        tab_split_show = false
    end
    if (mstab == nil) then
        mstab = {["vertical"] = false, ["horizontal"] = false, ["slave_focus"] = false, ["show"] = false, ["main_size"] = 0, ["slave_size"] = 0}
    end
    local m_config = eu_core.ffi.new("struct eu_config", {
        newfile_eols,
        newfile_encoding,
        enable_auto_identation,
        window_theme,
        window_full_screen,
        window_menubar_visiable,
        window_toolbar_visiable,
        window_statusbar_visiable,
        line_number_visiable,
        last_search_flags,
        history_mask,
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
        sidebar_width,
        document_map_width,
        sqlquery_result_edit_height,
        sqlquery_result_listview_height,
        file_recent_number,
        scroll_to_cursor,
        inter_reserved_0,
        inter_reserved_1,
        inter_reserved_2,
        block_fold_visiable,
        tabs_tip_show_enable,
        code_hint_show_enable,
        tab_split_show,
        tab_close_way,
        tab_close_draw,
        tab_new_way,
        tab_switch_forward,
        edit_font_quality,
        edit_rendering_technology,
        update_file_mask,
        update_file_notify,
        light_all_find_str,
        backup_on_file_write,
        save_last_session,
        exit_when_close_last_tab,
        allow_multiple_instance,
        enable_runtime_logging,
        save_last_placement,
        ui_language,
        {bookmark.visable, bookmark.shape, bookmark.argb},
        {brace.matching, brace.autoc, brace.rgb},
        {calltip.enable, calltip.rgb},
        {complete.enable, complete.characters, complete.snippet},
        {printer.header, printer.footer, printer.color_mode, printer.zoom,{printer.margin_left, printer.margin_top, printer.margin_right, printer.margin_bottom}},
        {titlebar.icon, titlebar.name, titlebar.path},
        {mstab.vertical, mstab.horizontal, mstab.slave_focus, mstab.show, mstab.main_size, mstab.slave_size, 0, 0},
        hyperlink_detection,
        cache_limit_size,
        {app_upgrade.enable, app_upgrade.flags, app_upgrade.msg_id, app_upgrade.last_check, app_upgrade.url},
        process_path,
        other_editor_path,
        m_reserved_0,
        m_reserved_1
    })
    eu_conf.fill_actions(m_config)
    eu_conf.fill_customize(m_config)
    if (not eu_core.euapi.eu_config_ptr(m_config)) then
        do return nil end
    end
    printer = nil
    return window_theme
end

return eu_conf
