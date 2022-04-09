rust = {}

function rust.get_keywords()
  local keywords0_set = "bool char f32 f64 i16 i32 i64 i8 int str u16 u32 u64 u8 uint as break const continue crate else enum extern false fn for if impl in let loop match mod move mut pub ref return Self self static struct super trait true type unsafe use where while union"
  return keywords0_set
end

function rust.get_reqular()
  local symbol_reqular_exp = "[ \\t]*fn[ \\t]+([_a-zA-Z]+[_a-zA-Z0-9]*)\\("
  return symbol_reqular_exp
end

function rust.create_bakup(path)
  local rust_code = {
    "user_rust = {}\n",
    "function user_rust.get_keywords()\n",
    "  local keywords0_set = \"bool char f32 f64 i16 i32 i64 i8 int str u16 u32 u64 u8 uint as break const continue crate else enum extern false fn for if impl in let loop match mod move mut pub ref return Self self static struct super trait true type unsafe use where while union\"\n",
    "  return keywords0_set\n",
    "end\n",
    "\n",
    "function user_rust.get_reqular()\n",
    "  local symbol_reqular_exp = \"[ \\\\t]*fn[ \\\\t]+([_a-zA-Z]+[_a-zA-Z0-9]*)\\\\(\"\n",
    "  return symbol_reqular_exp\n",
    "end\n",
    "return user_rust",
  }
  local shell_code = table.concat(rust_code)
  eu_core.save_file(path, shell_code)
  shell_code = nil
  rust_code = nil
end

return rust