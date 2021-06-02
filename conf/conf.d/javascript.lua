javascript = {}

function javascript.get_keywords()
    local keywords0_set = "abstract boolean break byte case catch char class const continue debugger default delete do double else enum export extends final finally float for function goto if implements import in instanceof int interface long native new package private protected public return short static super switch synchronized this throw throws transient try typeof var void volatile while with"
	return keywords0_set
end	

function javascript.get_autocomplete()
    local autocomplete_set = "abstract boolean break byte case catch char class const continue debugger default delete do double else enum export extends final finally float for function goto if implements import in instanceof int interface long native new package private protected public return short static super switch synchronized this throw throws transient try typeof var void volatile while with"
	return autocomplete_set
end	

function javascript.get_reqular()
    local symbol_reqular_exp = "[\\*_a-zA-Z]+[_a-zA-Z0-9]*[ \\t*]+([_a-zA-Z]+[_a-zA-Z0-9:]*)\\([^\\(^;]*$"
    return symbol_reqular_exp
end

return javascript
