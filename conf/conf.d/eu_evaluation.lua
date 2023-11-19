require("eu_core")

function isnan(_v) return _v~=_v end

local pstr = arg[1]
local nlen = tonumber(arg[2])
if (pstr ~= nil and nlen ~= nil and 1 < nlen) then
  local ret = eu_core.euapi.eu_te_interp(eu_core.ffi.cast('const char *', pstr), nil)
  if (ret ~= nil) then
    if (isnan(ret)) then ret = "NaN" else ret = tostring(ret) end
    if (ret == "inf") then
      ret = "INFINITY"
    elseif (ret == "-inf") then
      ret = "-INFINITY"
    else
      local x, y = math.modf(ret)
      if (math.abs(x) == 0 and math.abs(y) < 1.0E-12) then ret = "0" else ret = tostring(ret) end
    end
    eu_core.euapi.eu_lua_calltip(ret)
  end
end
