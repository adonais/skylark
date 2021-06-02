swift = {}

function swift.get_keywords()
    local keywords0_set = "class deinit enum extension func import init internal let operator private protocol public static struct subscript typealias var break case continue default do else fallthrough for if in return switch where while as dynamicType false is nil self Self super true __COLUMN__ __FILE__ __FUNCTION__ __LINE__ associativity convenience dynamic didSet final get infix inout lazy left mutating none nonmutating optional override postfix precedence prefix Protocol required right set Type unowned weak willSet"
	return keywords0_set
end	

function swift.get_autocomplete()
    local autocomplete_set = "class deinit enum extension func import init internal let operator private protocol public static struct subscript typealias var break case continue default do else fallthrough for if in return switch where while as dynamicType false is nil self Self super true __COLUMN__ __FILE__ __FUNCTION__ __LINE__ associativity convenience dynamic didSet final get infix inout lazy left mutating none nonmutating optional override postfix precedence prefix Protocol required right set Type unowned weak willSet"
	return autocomplete_set
end	

function swift.get_reqular()
    local symbol_reqular_exp = "[\\*_a-zA-Z]+[_a-zA-Z0-9]*[ \\t*]+([_a-zA-Z]+[_a-zA-Z0-9:]*)\\([^\\(^;]*$"
    return symbol_reqular_exp
end

return swift
