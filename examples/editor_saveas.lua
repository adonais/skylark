require("eu")

function skylark_filesaveas(index)
    print(string.format("[%s] saveas\n", eu.file_path(index)))
end

function main()
    local ret = eu.lib.register_event(SKYLARK_FILESAVEAS, "skylark_filesaveas")
    if (ret ~= 1) then
        print("Loader: register skylark_filesaveas envent failed")
    end
end
