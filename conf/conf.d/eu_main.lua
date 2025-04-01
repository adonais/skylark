require("eu_conf")
require("eu_theme")

function switch_theme(name)
    if (not eu_theme.load_default(name)) then
      do return 1 end
    end
    return 0
end

function run(none)
    local name = eu_conf.loadconf()
    if (name == nil) then return 1 end
    if (none == "") then
        if (name == "") then
            name = "default"
        end
        return switch_theme(name)
    else
        return 0
    end
end
