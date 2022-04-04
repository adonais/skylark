# The toplevel directory of the source tree.
#
ROOT = ..\..\..
!include "$(ROOT)\system.mak"

SRC = .\src
BIN_O = .\bin
DIR_O = $(OBJD)\luajit

# variables
STATICLIB = $(BIND)\luajit.lib
MINILUA = $(BIN_O)\minilua.exe
BUILDVM = $(BIN_O)\buildvm.exe

CFLAGS  = -Zi -Oy -Ob1 -DHAVE_CONFIG_H -I$(SRC) -D_CRT_SECURE_NO_DEPRECATE -D_CRT_SECURE_NO_WARNINGS \
          -DLUA_BUILD_AS_DLL $(RELEASE) $(CFLAGS)
          
MIN_OBJS =                          \
    $(DIR_O)\minilua.obj            \
    
BUILDVM_OBJS =                      \
    $(DIR_O)\host\buildvm.obj       \
    $(DIR_O)\host\buildvm_asm.obj   \
    $(DIR_O)\host\buildvm_fold.obj  \
    $(DIR_O)\host\buildvm_lib.obj   \
    $(DIR_O)\host\buildvm_peobj.obj \

ALL_LIB = $(SRC)\lib_base.c         \
          $(SRC)\lib_math.c         \
          $(SRC)\lib_bit.c          \
          $(SRC)\lib_string.c       \
          $(SRC)\lib_table.c        \
	      $(SRC)\lib_io.c           \
	      $(SRC)\lib_os.c           \
	      $(SRC)\lib_package.c      \
	      $(SRC)\lib_debug.c        \
	      $(SRC)\lib_jit.c          \
	      $(SRC)\lib_ffi.c          \
	      $(SRC)\lib_buffer.c       \
	          
OBJS =                            \
    $(DIR_O)\lj_alloc.obj         \
    $(DIR_O)\lj_api.obj           \
    $(DIR_O)\lj_asm.obj           \
    $(DIR_O)\lj_assert.obj        \
    $(DIR_O)\lj_bc.obj            \
    $(DIR_O)\lj_bcread.obj        \
    $(DIR_O)\lj_bcwrite.obj       \
    $(DIR_O)\lj_buf.obj           \
    $(DIR_O)\lj_carith.obj        \
    $(DIR_O)\lj_ccall.obj         \
    $(DIR_O)\lj_ccallback.obj     \
    $(DIR_O)\lj_cconv.obj         \
    $(DIR_O)\lj_cdata.obj         \
    $(DIR_O)\lj_char.obj          \
    $(DIR_O)\lj_clib.obj          \
    $(DIR_O)\lj_cparse.obj        \
    $(DIR_O)\lj_crecord.obj       \
    $(DIR_O)\lj_ctype.obj         \
    $(DIR_O)\lj_debug.obj         \
    $(DIR_O)\lj_dispatch.obj      \
    $(DIR_O)\lj_err.obj           \
    $(DIR_O)\lj_ffrecord.obj      \
    $(DIR_O)\lj_func.obj          \
    $(DIR_O)\lj_gc.obj            \
    $(DIR_O)\lj_gdbjit.obj        \
    $(DIR_O)\lj_ir.obj            \
    $(DIR_O)\lj_lex.obj           \
    $(DIR_O)\lj_lib.obj           \
    $(DIR_O)\lj_load.obj          \
    $(DIR_O)\lj_mcode.obj         \
    $(DIR_O)\lj_meta.obj          \
    $(DIR_O)\lj_obj.obj           \
    $(DIR_O)\lj_opt_dce.obj       \
    $(DIR_O)\lj_opt_fold.obj      \
    $(DIR_O)\lj_opt_loop.obj      \
    $(DIR_O)\lj_opt_mem.obj       \
    $(DIR_O)\lj_opt_narrow.obj    \
    $(DIR_O)\lj_opt_sink.obj      \
    $(DIR_O)\lj_opt_split.obj     \
    $(DIR_O)\lj_parse.obj         \
    $(DIR_O)\lj_prng.obj          \
    $(DIR_O)\lj_profile.obj       \
    $(DIR_O)\lj_record.obj        \
    $(DIR_O)\lj_serialize.obj     \
    $(DIR_O)\lj_snap.obj          \
    $(DIR_O)\lj_state.obj         \
    $(DIR_O)\lj_str.obj           \
    $(DIR_O)\lj_strfmt.obj        \
    $(DIR_O)\lj_strfmt_num.obj    \
    $(DIR_O)\lj_strscan.obj       \
    $(DIR_O)\lj_tab.obj           \
    $(DIR_O)\lj_trace.obj         \
    $(DIR_O)\lj_udata.obj         \
    $(DIR_O)\lj_vm.obj            \
    $(DIR_O)\lj_vmevent.obj       \
    $(DIR_O)\lj_vmmath.obj        \
    $(DIR_O)\lib_aux.obj          \
    $(DIR_O)\lib_base.obj         \
    $(DIR_O)\lib_bit.obj          \
    $(DIR_O)\lib_buffer.obj       \
    $(DIR_O)\lib_debug.obj        \
    $(DIR_O)\lib_ffi.obj          \
    $(DIR_O)\lib_init.obj         \
    $(DIR_O)\lib_io.obj           \
    $(DIR_O)\lib_jit.obj          \
    $(DIR_O)\lib_math.obj         \
    $(DIR_O)\lib_os.obj           \
    $(DIR_O)\lib_package.obj      \
    $(DIR_O)\lib_string.obj       \
    $(DIR_O)\lib_table.obj        \
    
################################################################################
#$(CC) $(CFLAGS) -Idynasm $(SRC)\host\buildvm*.c -link -out:$@
.SUFFIXES: .c .h .obj

{$(SRC)}.c{$(DIR_O)}.obj:
    $(CC) $(CFLAGS) -Fd$(BIND)\euapi.pdb -Fo$(DIR_O)\ /c $<
    
################################################################################    
# targets
all: dirs $(MINILUA) $(BUILDVM) $(STATICLIB)

dirs:
    @if not exist "$(BIND)" mkdir "$(BIND)" && echo.   Created $(BIND)
    @if not exist "$(DIR_O)" mkdir "$(DIR_O)" && echo.   Created $(DIR_O)
    @if not exist "$(DIR_O)\host" mkdir "$(DIR_O)\host" && echo.   Created $(DIR_O)\host
    @if not exist "$(BIN_O)" mkdir "$(BIN_O)" && echo.   Created $(BIN_O)
    
$(MINILUA): $(MIN_OBJS)
	$(LD) $(LDFLAGS) -out:$@ $(MIN_OBJS)
	$(MINILUA) dynasm\dynasm.lua -LN -D WIN -D JIT -D FFI -D P64 -o $(SRC)\host\buildvm_arch.h $(SRC)\vm_x64.dasc
	
$(BUILDVM): $(BUILDVM_OBJS) $(MINILUA)
	$(LD) $(LDFLAGS) -out:$@ $(BUILDVM_OBJS)
	$(BUILDVM) -m peobj -o $(DIR_O)\lj_vm.obj
	$(BUILDVM) -m bcdef -o $(SRC)\lj_bcdef.h $(ALL_LIB)
	$(BUILDVM) -m ffdef -o $(SRC)\lj_ffdef.h $(ALL_LIB)
	$(BUILDVM) -m libdef -o $(SRC)\lj_libdef.h $(ALL_LIB)
	$(BUILDVM) -m recdef -o $(SRC)\lj_recdef.h $(ALL_LIB)
	$(BUILDVM) -m vmdef -o $(SRC)\jit\vmdef.lua $(ALL_LIB)
	$(BUILDVM) -m folddef -o $(SRC)\lj_folddef.h $(SRC)\lj_opt_fold.c
	
$(STATICLIB): $(OBJS) $(BUILDVM)
	$(AR) -out:$@ $(OBJS)
    @if not exist "$(BIND)\\lua\\jit" mkdir "$(BIND)\\lua\\jit" && echo. Created $(BIND)\\lua\\jit
    xcopy "$(SRC)\\jit" "$(BIND)\\lua\\jit" /s /e /q /y 2>&1>NUL

# cleanup
clean:
	-del /q $(MINILUA) 2>nul 1>nul
	-del /q $(BUILDVM) 2>nul 1>nul
	-del /q $(STATICLIB) 2>nul 1>nul
	-del /q $(OBJS) 2>nul 1>nul
	-del /q $(SRC)\host\buildvm_arch.h  $(SRC)\lj_vm.obj $(SRC)\lj_bcdef.h $(SRC)\lj_ffdef.h $(SRC)\lj_libdef.h \
	        $(SRC)\lj_recdef.h $(SRC)\lj_recdef.h $(SRC)\jit\vmdef.lua $(SRC)\lj_folddef.h
	-rd /s/q $(DIR_O)  2>nul 1>nul
	-rd /s/q $(BIN_O)  2>nul 1>nul

$(MIN_OBJS)     : $(SRC)\host\minilua.c
	$(CC) $(CFLAGS) -Fd$(BIN_O)\minilua.pdb -Fo$(DIR_O)\ -c $(SRC)\host\minilua.c
	
$(BUILDVM_OBJS) : $(SRC)\host\buildvm*.c
	$(CC) $(CFLAGS) -Fd$(BIN_O)\buildvm.pdb -Fo$(DIR_O)\host\ -c $(SRC)\host\buildvm*.c