require("eu_toolbar")

function run(tname)
    if (not eu_toolbar.loading(tname)) then
        do return 1 end
    end
    return 0
end
