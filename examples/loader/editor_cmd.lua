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

function qsavefile_action()
    if (eu.save_file(-1) == 0) then
        return eu.close_file(-1, 1)
    end
    return false
end

function close_action()
    -- FILE_ONLY_CLOSE == 1
    return eu.close_file(-1, 1)
end

function sclose_action()
    -- FILE_FORCE_CLOSE == 6
    return eu.close_file(-1, 6)
end

local cmd_matrix =
{
    {"^enc=([%w_-]+)",            encoding_action },
    {"^fenc=([%w_-]+)",           fencoding_action},
    {"^w$",                       savefile_action },
    {"^wq$)",                     qsavefile_action},
    {"^q$",                       close_action    },
    {"^q!$",                      sclose_action   },
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
