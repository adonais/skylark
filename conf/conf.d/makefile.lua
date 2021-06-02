makefile = {}

function makefile.get_keywords()
    local keywords0_set = "break case catch classdef continue else elseif end for function global if ifeq ifneq ifdef ifndef otherwise parfor persistent return switch try while"
    local keywords1_set = ": $ echo error cc cxx cflags cxxflags ldflags define subst strip findstring filter filter-out sort word words wordlist firstword dir notdir suffix basename addsuffix addprefix join foreach call origin shell warning patsubst export include override wildcard all clean vpath .PHONY ARFLAGS ASFLAGS CFLAGS CXXFLAGS COFLAGS CPPFLAGS FFLAGS GFLAGS LDFLAGS LFLAGS PFLAGS RFLAGS YFLAGS TARGET"
	return keywords0_set,keywords1_set
end	

function makefile.get_extname()
    local file_extnames = ";makefile;Makefile;Makefile.gcc;*.mk;*.mak;configure;"
    return file_extnames
end

function makefile.get_tabattr()
    -- 0, 跟随主配置
    local tab_width = 0
    -- 0, 不转换 1, 转换, -1, 跟随主配置
    local tab_convert_spaces = 0
    return tab_width,tab_convert_spaces
end

return makefile
