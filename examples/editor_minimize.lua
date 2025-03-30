require("eu")

function skylark_minimize(index)
    print(string.format("Window minimized[%s]\n", eu.window_title()))
end

function main()
    local ret = eu.lib.register_event(SKYLARK_MINIMIZED, "skylark_minimize")
    if (ret ~= 1) then
        print("Loader: register skylark_minimized envent failed")
    end
end
