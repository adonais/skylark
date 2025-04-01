require("eu")

function skylark_restored(index)
    print(string.format("Window restored[%s]\n", eu.file_name(index)))
end

function main()
    local ret = eu.lib.register_event(SKYLARK_RESTORED, "skylark_restored")
    if (ret ~= 1) then
        print("Loader: register skylark_restored envent failed")
    end
end
