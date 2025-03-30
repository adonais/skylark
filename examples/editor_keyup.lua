require("eu")

function skylark_keyup(index)
    print(string.format("Keyup, current word[%s]\n", eu.current_word(index)))
end

function main()
    local ret = eu.lib.register_event(SKYLARK_KEYUP, "skylark_keyup")
    if (ret ~= 1) then
        print("Loader: register skylark_keyup envent failed")
    end
end
