rust = {}

function rust.get_keywords()
    local keywords0_set = "bool char f32 f64 i16 i32 i64 i8 int str u16 u32 u64 u8 uint as break const continue crate else enum extern false fn for if impl in let loop match mod move mut pub ref return Self self static struct super trait true type unsafe use where while union"
	return keywords0_set
end	

function rust.get_reqular()
    local symbol_reqular_exp = "[ \\t]*fn[ \\t]+([_a-zA-Z]+[_a-zA-Z0-9]*)\\("
    return symbol_reqular_exp
end

return rust
