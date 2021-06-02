ruby = {}

function ruby.get_keywords()
    local keywords0_set = "__FILE__ and def end in or self unless __LINE__ begin defined? ensure module redo super until BEGIN break do false next rescue then when END case else for nil retry true while alias class elsif if"
	return keywords0_set
end	

function ruby.get_reqular()
    local symbol_reqular_exp = "[ \\t]*def[ \\t]+([_a-zA-Z]+[_a-zA-Z0-9]*)\\("
    return symbol_reqular_exp
end

return ruby
