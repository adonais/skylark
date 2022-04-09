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

function swift.create_bakup(path)
  local swift_code = {
    "user_swift = {}\n",
    "function user_swift.get_keywords()\n",
    "  local keywords0_set = \"class deinit enum extension func import init internal let operator private protocol public static struct subscript typealias var break case continue default do else fallthrough for if in return switch where while as dynamicType false is nil self Self super true __COLUMN__ __FILE__ __FUNCTION__ __LINE__ associativity convenience dynamic didSet final get infix inout lazy left mutating none nonmutating optional override postfix precedence prefix Protocol required right set Type unowned weak willSet\"\n",
    "  return keywords0_set\n",
    "end\n",
    "\n",
    "function user_swift.get_autocomplete()\n",
    "  local autocomplete_set = \"class deinit enum extension func import init internal let operator private protocol public static struct subscript typealias var break case continue default do else fallthrough for if in return switch where while as dynamicType false is nil self Self super true __COLUMN__ __FILE__ __FUNCTION__ __LINE__ associativity convenience dynamic didSet final get infix inout lazy left mutating none nonmutating optional override postfix precedence prefix Protocol required right set Type unowned weak willSet\"\n",
    "  return autocomplete_set\n",
    "end\n",
    "\n",
    "function user_swift.get_reqular()\n",
    "  local symbol_reqular_exp = \"[\\\\*_a-zA-Z]+[_a-zA-Z0-9]*[ \\\\t*]+([_a-zA-Z]+[_a-zA-Z0-9:]*)\\\\([^\\\\(^;]*$\"\n",
    "  return symbol_reqular_exp\n",
    "end\n",
    "return user_swift",
  }
  local shell_code = table.concat(swift_code)
  eu_core.save_file(path, shell_code)
  shell_code = nil
  swift_code = nil
end

return swift