cshape = {}

function cshape.get_keywords()
    local keywords0_set = "abstract as ascending base bool break by byte case catch char checked class const continue decimal default delegate descending do double else enum equals event explicit extern false finally fixed float for foreach from goto group if implicit in int interface internal into is join lock let long namespace new null object on operator orderby out override params private protected public readonly ref return sbyte sealed select short sizeof stackalloc static string struct switch this throw true try typeof uint ulong unchecked unsafe ushort using var virtual void volatile where while"
	return keywords0_set
end	

function cshape.get_autocomplete()
    local autocomplete_set = "abstract as ascending base bool break by byte case catch char checked class const continue decimal default delegate descending do double else enum equals event explicit extern false finally fixed float for foreach from goto group if implicit in int interface internal into is join lock let long namespace new null object on operator orderby out override params private protected public readonly ref return sbyte sealed select short sizeof stackalloc static string struct switch this throw true try typeof uint ulong unchecked unsafe ushort using var virtual void volatile where while"
	return autocomplete_set
end	

function cshape.get_reqular()
    local symbol_reqular_exp = "[ \\t]+[_a-zA-Z]+[_a-zA-Z0-9]*[ \\t]+[_a-zA-Z]+[_a-zA-Z0-9]*[ \\t]+([_a-zA-Z]+[_a-zA-Z0-9:]*)\\([^\\(^;]*$"
    return symbol_reqular_exp
end

return cshape
