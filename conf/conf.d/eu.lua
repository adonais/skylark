eu = {}

require("eu_core")
require("eu_sci")

eu.ffi = eu_core.ffi
eu.api = eu_core.euapi
eu.lib = eu_core.eulib

function eu.get_buffer()
    info = debug.getinfo(1, 'S')
    print("当前路径:", info.source)
end

return eu
