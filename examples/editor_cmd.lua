require("eu")

function get_word(p)
    local txt = nil
    local pos = eu.api.eu_sci_cmd(p, SCI_GETCURRENTPOS, 0, 0)
    if (pos ~= nil and pos >= 0) then
        local s1 = eu.api.eu_sci_cmd(p, SCI_WORDSTARTPOSITION, pos, 1)
        local s2 = eu.api.eu_sci_cmd(p, SCI_WORDENDPOSITION, pos, 1)
        if (s1 ~= nil and s2 ~= nil) then
            local buf = eu.api.eu_sci_range(p, s1, s2)
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

function skylark_cmd(p)
    print(string.format("Cmds [%s]\n", get_word(p)))
end

function main()
    local ret = eu.lib.register_event(SKYLARK_COMMANDS, "skylark_cmd")
    if (ret ~= 1) then
        print("Loader: register skylark_cmd envent failed")
    end
end
