require("eu")

function get_line(p)
    local txt = nil
    local pos = eu.api.eu_sci_cmd(p, SCI_GETCURRENTPOS, 0, 0)
    if (pos ~= nil and pos >= 0) then
        local line = eu.api.eu_sci_cmd(p, SCI_LINEFROMPOSITION, pos, 1)
        local start = eu.api.eu_sci_cmd(p, SCI_POSITIONFROMLINE, line, 1)
        if (line ~= nil and start ~= nil) then
            local buf = eu.api.eu_sci_range(p, start, pos)
            eu.ffi.gc(buf,(function(self)
                if buf ~= nil then eu.ffi.C.free(buf) end
            end))
            if (buf ~= nil) then
                txt = eu.ffi.string(buf, eu.ffi.C.strlen(buf))
            end
        end
    end
    return txt
end

function encoding_action(enc)
    return eu.api.eu_command_reload(-1, enc)
end

function fencoding_action(enc)
    return eu.api.eu_command_convert(-1, enc)
end

function savefile_action()
    return eu.save_file(-1)
end

function saveall_action()
    return eu.api.eu_file_save_all()
end

function qsavefile_action()
    if (eu.save_file(-1) == 0) then
        return eu.close_file(-1, 1)
    end
    return false
end

function xsavefile_action()
    return eu.api.eu_command_xsave(-1)
end

function close_action()
    -- FILE_ONLY_CLOSE == 1
    return eu.close_file(-1, 1)
end

function sclose_action()
    -- FILE_FORCE_CLOSE == 6
    return eu.close_file(-1, 6)
end

function ssavefile_action(path)
    -- SAVE_SIL == 3
    return eu.api.eu_command_saveas(-1, path, 3)
end

function saveas_action(path)
    -- SAVE_AS == 1
    return eu.api.eu_command_saveas(-1, path, 1)
end

function search_action(str)
    local flags = SCFIND_MATCHCASE + SCFIND_REGEXP
    local ret = string.gsub(str, "\\c", "")
    if (ret ~= str) then
        flags = flags - SCFIND_MATCHCASE
    end
    str = string.gsub(ret, "\\<", "\\b")
    ret = string.gsub(str, "\\>", "\\b")
    if ('?' == (string.sub(ret,1,1))) then
        flags = flags + SCCMD_REVERSE
    end
    ret = string.sub(ret, 2, #ret)
    str = string.gsub(ret, "\\C", "")
    return eu.api.eu_command_search(-1, str, flags)
end

function replace_action(str, n1, n2)
    local ret1, ret2, v1, v2, nocase, ft = nil
    local flags = SCFIND_MATCHCASE + SCFIND_REGEXP
    local ret = string.gsub(str, "\\c", "")
    if (ret ~= str) then
        flags = flags - SCFIND_MATCHCASE
        nocase = true
    end
    if (type(n1) == "boolean") then ft = true end
    str = string.gsub(ret, "\\<", "\\b")
    ret = string.gsub(str, "\\>", "\\b")
    str = string.gsub(ret, "\\C", "")
    v1 = string.find(str, '/')
    if (v1 ~= nil) then
        ret1 = string.sub(str, 1, v1 - 1)
        v2 = string.find(str, '/', v1 + 1)
    end
    if (v2 ~= nil) then ret2 = string.sub(str, v1 + 1, v2 - 1) end
    if (ret1 ~= nil and ret2 ~= nil) then
        if (v2 < #str) then
            ret = string.sub(str, v2 + 1, #str)
            if (ret ~= nil) then
                if (string.find(ret, 'i') ~= nil) then
                    if (nocase == nil) then flags = flags - SCFIND_MATCHCASE end
                end
                if (string.find(ret, 'g') ~= nil) then
                    if (n2 ~= nil) then
                        -- do nothing
                    elseif (ft ~= nil) then
                        flags = flags + SCCMD_TEXT_ALL
                    else
                        flags = flags + SCCMD_LINE_ALL
                    end
                elseif (n2 ~= nil) then
                    -- do nothing
                elseif (ft ~= nil) then
                    flags = flags + SCCMD_TEXT_FIRST
                else
                    flags = flags + SCCMD_LINE_FIRST
                end
            end
        elseif (n2 ~= nil) then
            -- do nothing
        elseif (ft ~= nil) then
            flags = flags + SCCMD_TEXT_FIRST
        else
            flags = flags + SCCMD_LINE_FIRST
        end
        if (n2 == ni1) then
            n1 = -1
            n2 = -1
        end
        return eu.api.eu_command_replace(-1, ret1, ret2, n1, n2, flags)
    end
end

function replace_global(str)
    return replace_action(str, true)
end

function replace_part(v1, v2, str)
    local line = nil
    local n1, n2 = nil
    print(v1, v2, str)
    if (v1 == '.' and #v1 == 1) then
        line = eu.cureent_line_number(-1)
        n1 = eu.line_start_positon(-1, -1)
    elseif (string.sub(v1, 1, 1) ~= '.') then
        line = tonumber(v1) - 1
        if (line <= 0) then line = 0 end
        n1 = eu.line_start_positon(-1, line)
    end
    if (n1 ~= nil and line ~= nil) then
        local line2 = nil
        local ch = string.sub(v2, 1, 1)
        if (ch == '+' and #v2 > 1) then
            line2 = tonumber(string.sub(v2, 2, #v2)) + line
            if (line2 <= 0) then line2 = 0 end
            n2 = eu.line_end_positon(-1, line2)
        elseif (ch == '$' and #v2 == 1) then
            n2 = eu.max_position(-1)
        elseif (v2 ~= "") then
            line2 = tonumber(v2)
            if (line2 <= 0) then line2 = 1 end
            n2 = eu.line_end_positon(-1, line2 - 1)
        end
    end
    if (n1 >= 0 and n2 >= 0) then
        replace_action(str, n1, n2)
    end
end

function search_tabs(str)
    if (str ~= nil and str ~= "") then
        return eu.api.eu_command_tabs_hint(str, true)
    end
end

function search_tabactive(str)
    if (str ~= nil and str ~= "") then
        return eu.api.eu_command_tabs_hint(str, false)
    end
end

function close_tabs(str)
    if (str == "tc") then
        eu.close_tab()
    elseif (str == "tca") then
        -- -- WM_COMMAND == 0x0111
        eu.ffi.C.SendMessageW(eu.api.eu_module_hwnd(), 0x0111, IDM_FILE_CLOSEALL, 0)
    elseif (str == "tce") then
        eu.ffi.C.SendMessageW(eu.api.eu_module_hwnd(), 0x0111, IDM_FILE_CLOSEALL_EXCLUDE, 0)
    elseif (str == "tcr") then
        eu.ffi.C.SendMessageW(eu.api.eu_module_hwnd(), 0x0111, IDM_TAB_CLOSE_RIGHT, 0)
    elseif (str == "tcl") then
        eu.ffi.C.SendMessageW(eu.api.eu_module_hwnd(), 0x0111, IDM_TAB_CLOSE_LEFT, 0)
    end
end

function close_onetab(number)
    local index = tonumber(number)
    if (index ~= nil and index > 0) then
        eu.close_file(index - 1, 1)
    end
end

function switch_onetab(number)
    local index = tonumber(number)
    local cur = eu.api.eu_tab_focus_index()
    if (index ~= nil and index >= 0 and cur >= 0) then
        if (index == 0) then index = 1 end
        index = index - 1
        if (index ~= cur) then
            eu.api.eu_command_launch(-1)
            eu.api.eu_command_launch(index)
            eu.api.eu_tab_select(index)
        end
    end
end

function switch_lasttab()
    local index = eu.api.eu_tab_last_index()
    if (index ~= nil and index >= 0) then
        switch_onetab(index + 1)
    end
end

function goto_linenumber(str)
    local line = nil
    if (str == 'c') then
        line = -1;
    elseif (str == 'g') then
        line = eu.line_total(-1)
    else
        line = tonumber(str)
    end
    if (line ~= nil) then eu.api.eu_command_jump(-1, line) end
end

function which_action(str)
    if (str ~= nil and str ~= "") then
        eu.api.eu_command_which(str)
    end
end

function exec_action(str)
    if (str ~= nil and str ~= "") then
        local s = nil
        local d = string.find(str, ",%d")
        if (d ~= nil) then
            s = string.sub(str, 1, d - 1)
            d = string.sub(str, d + 1, #str)
        else
            s = str
            d = 3
        end
        local handle, pid = eu.create_process(s, nil, tonumber(d))
        if (handle ~= nil) then
            -- print(string.format("we create proces, pid = %u\n", pid))
            eu.ffi.C.CloseHandle(eu.ffi.cast("void *", handle))
        end
    end
end

function run_action(str, fnclose)
    if (str ~= nil and str ~= "") then
        if (fnclose == nil) then fnclose = false end
        eu.api.eu_command_run(-1, str, fnclose)
    end
end

function runq_action(str)
    run_action(str, true)
end

function quit_action()
    eu.ffi.C.SendMessageW(eu.api.eu_module_hwnd(), 0x0111, IDM_EXIT, 0)
end

function restart_action()
    eu.ffi.C.SendMessageW(eu.api.eu_module_hwnd(), 0x0111, IDM_FILE_RESTART_ADMIN, 0)
end

function ai_action(str)
    if (str ~= nil and str ~= "") then
        str = string.gsub(str, "\\t", "\t")
        str = string.gsub(str, "\\r", "\n")
        str = string.gsub(str, "\\n", "\n")
        str = string.gsub(str, "\"", "\\\"")
        eu.api.eu_command_talk(-1, str)
    end
end

local cmd_matrix =
{
    {"^enc=([%w_-]+)",                      encoding_action },
    {"^fenc=([%w_-]+)",                     fencoding_action},
    {"^w$",                                 savefile_action },
    {"^wa$",                                saveall_action  },
    {"^wq$)",                               qsavefile_action},
    {"^w%s+(.+)",                           ssavefile_action},
    {"^x$",                                 xsavefile_action},
    {"^q$",                                 close_action    },
    {"^q!$",                                sclose_action   },
    {"^sav%s+(.+)",                         saveas_action   },
    {"^saveas%s+(.+)",                      saveas_action   },
    {"^([/\\?].+)",                         search_action   },
    {"^s/(.+)",                             replace_action  },
    {"^%%s/(.+)",                           replace_global  },
    {"^([%.%d]?%d*),([%+%$%d]?%d*)s/(.+)",  replace_part    },
    {"^ts%s*(.+)",                          search_tabs     },
    {"^tw%s*(.+)",                          search_tabactive},
    {"^(tc[aelr]-)$",                       close_tabs      },
    {"^tc(%d+)$",                           close_onetab    },
    {"^tt$",                                switch_lasttab  },
    {"^t(%d+)$",                            switch_onetab   },
    {"^g(%d+)$",                            goto_linenumber },
    {"^g([cg])$",                           goto_linenumber },
    {"^which%s+(.+)$",                      which_action    },
    {"^exec%s+(.+)$",                       exec_action     },
    {"^run%s+(.+)$",                        run_action      },
    {"^runq%s+(.+)$",                       runq_action     },
    {"^quit$",                              quit_action     },
    {"^ras$",                               restart_action  },
    {"^ai%s+(.+)$",                         ai_action       },
}

function skylark_cmd(p)
    local txt = get_line(p)
    if (txt ~= nil and txt ~= "") then
        for i = 1, #cmd_matrix do
            for j = 1, 1 do
                local v1, v2, v3 = string.match(txt, cmd_matrix[i][j])
                if (v1 ~= nil and v1 ~= "") then
                    return (cmd_matrix[i][j+1])(v1, v2, v3)
                end
            end
        end
    end
end

function main()
    local ret = eu.lib.register_event(SKYLARK_COMMANDS, "skylark_cmd")
    if (ret ~= 1) then
        print("Loader: register skylark_cmd envent failed")
    end
end
