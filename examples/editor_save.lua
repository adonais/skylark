require("eu")

function skylark_filesave(index)
    print(string.format("[%s] saved\n", eu.file_path(index)))
end

function main()
    local ret = eu.lib.register_event(SKYLARK_FILESAVE, "skylark_filesave")
    if (ret ~= 1) then
        print("Loader: register skylark_filesave envent failed")
    end
end
