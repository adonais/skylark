luascript = {}

function luascript.get_keywords()
    local keywords0_set = "and break do else elseif end false for function if in local nil not or repeat return then true until while"
    local keywords1_set = "assert collectgarbage date error gcinfo getfenv getmetatable loadstring next pcall select setfenv setmetatable time type unpack xpcall abs acos asin atan atan2 ceil cos deg exp floor frexp ldexp log log10 max min mod rad random randomseed sin sqrt tan format gsub strbyte strchar strfind strlen strlower strmatch strrep strsub strupper tonumber tostring strtrim strsplit strjoin foreach foreachi getn ipairs pairs sort tinsert tremove"
	return keywords0_set,keywords1_set
end	

function luascript.get_autocomplete()
    local autocomplete_set = "assert collectgarbage date error gcinfo getfenv getmetatable loadstring next pcall select setfenv setmetatable time type unpack xpcall abs acos asin atan atan2 ceil cos deg exp floor frexp ldexp log log10 max min mod rad random randomseed sin sqrt tan format gsub strbyte strchar strfind strlen strlower strmatch strrep strsub strupper tonumber tostring strtrim strsplit strjoin foreach foreachi getn ipairs pairs sort tinsert tremove"
	return autocomplete_set
end

function luascript.get_calltip()
    local calltip_add = 
    {
        "assert|assert(value)",
        "collectgarbage|collectgarbage()",
        "date|date(format, time)",
        "error|error(\"error message\",level)",
        "gcinfo|gcinfo()",
        "getfenv|getfenv(function or integer)",
        "getmetatable|getmetatable(obj, mtable)",
        "loadstring|loadstring(\"Lua code\")",
        "next|next(table, index)",
        "pcall|pcall(func, arg1, arg2, ...)",
        "select|select(index, list)",
        "setfenv|setfenv(function or integer, table)",
        "setmetatable|setmetatable(obj, mtable)",
        "time|time(table)",
        "type|type(var)",
        "unpack|unpack(table)",
        "xpcall|xpcall(func, err)",
        "abs|abs(value)",
        "acos|acos(value)",
        "asin|asin(value)",
        "atan|atan(value)",
        "atan2|atan2(y, x)",
        "ceil|ceil(value)",
        "cos|cos(degrees)",
        "deg|deg(radians)",
        "exp|exp(value)",
        "floor|floor(value)",
        "frexp|frexp(num)",
        "ldexp|ldexp(value, multiple)",
        "log|log(value)",
        "log10|log10(value)",
        "max|max(value[, values...])",
        "min|min(value[,values...])",
        "mod|mod(value,modulus)",
        "rad|rad(degrees)",
        "random|random([ [lower,] upper])",
        "randomseed|randomseed(seed)",
        "sin|sin(degrees)",
        "sqrt|sqrt(value)",
        "tan|tan(degrees)",
        "format|format(formatstring[, value[, ...]])",
        "gsub|gsub(string,pattern,replacement[, limitCount])",
        "strbyte|strbyte(string[, index])",
        "strchar|strchar(asciiCode[, ...])",
        "strfind|strfind(string, pattern[, initpos[, plain]])",
        "strlen|strlen(string)",
        "strlower|strlower(string)",
        "strmatch|strmatch(string, pattern[, initpos])",
        "strrep|strrep(seed,count)",
        "strsub|strsub(string, index[, endIndex])",
        "strupper|strupper(string)",
        "tonumber|tonumber(arg[, base])",
        "tostring|tostring(arg)",
        "strtrim|strtrim(string)",
        "strsplit|strsplit(delimiter, string)",
        "strjoin|strjoin(delimiter, string, string[, ...])",
        "foreach|foreach(table,function)",
        "foreachi|foreachi(table,function)",
        "getn|getn(table)",
        "ipairs|ipairs(table)",
        "pairs|pairs(table)",
        "sort|sort(table[, comp])",
        "tinsert|tinsert(table[, pos], value)",
        "tremove|tremove(table[, pos])"
    }
    return calltip_add
end

function luascript.get_reqular()
    local symbol_reqular_exp = "[ \\t]*function[ \\t]+([_a-zA-Z]+[:_a-zA-Z0-9]*)\\s*\\("
    return symbol_reqular_exp
end

return luascript
