require("eu")

function skylark_mouseup(index)
    local line = eu.cureent_line_number(index)
    local buf = eu.get_line(index, line)
    local res = string.gsub(buf, "[\r\n]", "")
    print(string.format("[Mouseup, line: %d][%s]\n", line, res))
end

function main()
    local ret = eu.lib.register_event(SKYLARK_MOUSEUP, "skylark_mouseup")
    if (ret ~= 1) then
        print("Loader: register skylark_mouseup envent failed")
    end
end
