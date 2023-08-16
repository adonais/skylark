require("eu_core")

function isnan(_v) return _v~=_v end

local pstr = arg[1]
local nlen = tonumber(arg[2])
if (pstr ~= nil and nlen ~= nil and 1 < nlen) then
  local ret = eu_core.euapi.eu_te_interp(eu_core.ffi.cast('const char *', pstr), nil)
  if (isnan(ret)) then ret = "NaN" else ret = tostring(ret) end
  if (ret == "inf") then ret = "INFINITY" end
  eu_core.euapi.eu_lua_calltip(eu_core.ffi.new('char [?]', #ret + 1, ret))
end
