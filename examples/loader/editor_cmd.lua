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
    if (ret ~= str and wordstart == true) then
        flags = flags + SCFIND_WHOLEWORD
        flags = flags - SCFIND_WORDSTART
    end
    if ('?' == (string.sub(ret,1,1))) then
        flags = flags + SCCMD_REVERSE
    end
    ret = string.sub(ret, 2, #ret)
    str = string.gsub(ret, "\\C", "")
    return eu.api.eu_command_search(-1, str, flags)
end

local cmd_matrix =
{
    {"^enc=([%w_-]+)",            encoding_action },
    {"^fenc=([%w_-]+)",           fencoding_action},
    {"^w$",                       savefile_action },
    {"^wa$",                      saveall_action  },
    {"^wq$)",                     qsavefile_action},
    {"^w%s+(.+)",                 ssavefile_action},
    {"^x$",                       xsavefile_action},
    {"^q$",                       close_action    },
    {"^q!$",                      sclose_action   },
    {"^sav%s+(.+)",               saveas_action   },
    {"^saveas%s+(.+)",            saveas_action   },
    {"^([/\\?].+)",               search_action   },
}

function skylark_cmd(p)
    local txt = get_line(p)
    if (txt ~= nil and txt ~= "") then
        for i = 1, #cmd_matrix do
            for j = 1, 1 do
                local v1, v2 = string.match(txt, cmd_matrix[i][j])
                if (v1 ~= nil and v1 ~= "") then
                    return (cmd_matrix[i][j+1])(v1, v2)
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
