require("eu_accel")

function run(none)
    if (not eu_accel.loadaccel()) then
        do return 1 end
    end
    return 0
end
