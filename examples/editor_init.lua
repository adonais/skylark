require("eu")

function skylark_init()
    print(string.format("[pid: %u] initialization\n", eu.process_id()))
end

function main()
    local ret = eu.lib.register_event(SKYLARK_INIT, "skylark_init")
    if (ret ~= 1) then
        print("Loader: register skylark_init envent failed")
    end
end
