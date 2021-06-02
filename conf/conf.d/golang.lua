golang = {}

function golang.get_keywords()
    local keywords0_set = "bool byte break case chan complex64 complex128 const continue defer default else for func go goto fallthrough false float32 float64 if import int interface int8 int16 int32 int64 len map nil package range return select string struct switch true type uint uintptr uint8 uint16 uint32 uint64 var"
	return keywords0_set
end	

function golang.get_autocomplete()
    local autocomplete_set = "bool byte break case chan complex64 complex128 const continue defer default else for func go goto fallthrough false float32 float64 if import int interface int8 int16 int32 int64 len map nil package range return select string struct switch true type uint uintptr uint8 uint16 uint32 uint64 var"
	return autocomplete_set
end	

function golang.get_reqular()
    local symbol_reqular_exp = "func[ \\t]+([_a-zA-Z]+[_a-zA-Z0-9:]*)\\([^\\(^;]*$"
    return symbol_reqular_exp
end

function golang.get_tabattr()
    -- 0, 跟随主配置
    local tab_width = 0
    -- 0, 不转换 1, 转换, -1, 跟随主配置
    local tab_convert_spaces = 0
    return tab_width,tab_convert_spaces
end

return golang
