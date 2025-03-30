require("eu")

function skylark_shutdown()
    print(string.format("[pid: %u] shutdown\n", eu.process_id()))
end

function main()
    local ret = eu.lib.register_event(SKYLARK_SHUTDOWN, "skylark_shutdown")
    if (ret ~= 1) then
        print("Loader: register skylark_shutdown envent failed")
    end
end
