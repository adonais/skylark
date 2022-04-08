--[=[ euapi.dll调用示例
local ffi = require('ffi')
local euapi = ffi.load(ffi.os == "Windows" and "euapi.dll")

ffi.cdef[[
wchar_t * __stdcall utf8_to_utf16(const char *utf8, size_t *out_len);
void free(void *);
int MessageBoxW(void *w, const wchar_t *txt, const wchar_t *cap, int type);
]]

-- UTF-8 to UTF-16
local function U16(input)
    local wstr = euapi.utf8_to_utf16(input, nil)
	ffi.gc(wstr,(function(self)
	    if wstr ~= nil then ffi.C.free(wstr) end
	    print("call gc, goodbye!")
	end))
    return wstr
end
ffi.C.MessageBoxW(nil, U16("你好, Skylark Edit!"), U16("测试:"), 0)
]=]
