require("eu_toolbar")

function run(none)
    if (not eu_toolbar.loading()) then
        do return 1 end
    end
    return 0
end
