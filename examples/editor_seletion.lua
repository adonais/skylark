require("eu")

function skylark_seletion(index)
    print(string.format("[%s] seletion\n", eu.get_selection(index)))
end

function main()
    local ret = eu.lib.register_event(SKYLARK_SELETION, "skylark_seletion")
    if (ret ~= 1) then
        print("Loader: register skylark_seletion envent failed")
    end
end
