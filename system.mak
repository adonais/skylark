# -----------------------------------------------
# Detect NMAKE version deducing old MSVC versions
# -----------------------------------------------

!IFNDEF _NMAKE_VER
!  MESSAGE Macro _NMAKE_VER not defined.
!  MESSAGE Use MSVC's NMAKE to process this makefile.
!  ERROR   See previous message.
!ENDIF

!IF     "$(_NMAKE_VER)" == "6.00.8168.0"
CC_VERS_NUM = 60
!ELSEIF "$(_NMAKE_VER)" == "6.00.9782.0"
CC_VERS_NUM = 60
!ELSEIF "$(_NMAKE_VER)" == "7.00.8882"
CC_VERS_NUM = 70
!ELSEIF "$(_NMAKE_VER)" == "7.00.9466"
CC_VERS_NUM = 70
!ELSEIF "$(_NMAKE_VER)" == "7.00.9955"
CC_VERS_NUM = 70
!ELSEIF "$(_NMAKE_VER)" == "14.13.26132.0"
CC_VERS_NUM = 140
!ELSE
# Pick an arbitrary bigger number for all later versions
CC_VERS_NUM = 199
!ENDIF

!IF "$(PLATFORM)"=="x64" || "$(TARGET_CPU)"=="x64" || "$(VSCMD_ARG_HOST_ARCH)"=="x64"
BITS	 = 64
CFLAGS   = $(CFLAGS) -DWIN64 -D_WIN64 -I$(INCD)
!IF "$(CC)" == "cl"
CFLAGS   = $(CFLAGS)
!ENDIF
!ELSEIF "$(PLATFORM)"=="x86"
BITS	 = 32
CFLAGS   = $(CFLAGS) -arch:SSE2 -DWIN32 -I$(INCD)
!ELSE
!ERROR Unknown target processor: $(PLATFORM)
!ENDIF

!IF "$(CC)" == "cl"
LT   = -ltcg
AR   = lib -nologo 
LD   = link -nologo -guard:cf
CFLAGS   = $(CFLAGS) -nologo -GL -Gw -guard:cf
DLLFLAGS = -nologo -debug -incremental:no -opt:ref -opt:icf -dll $(LT) -guard:cf
MAVX2    =
!IF "$(BITS)" == "64"
CFLAGS   = $(CFLAGS) -favor:blend
!ENDIF
USE_CL = 1
!ELSEIF "$(CC)" == "clang-cl"
LT   =
AR   = llvm-lib -nologo -llvmlibthin
LD   = lld-link -nologo -guard:cf
CXX  = clang-cl
CFLAGS   = -nologo -Gw -flto=thin -guard:cf $(CFLAGS) -Wno-unused-variable -Wno-unused-function -Wno-parentheses-equality \
           -Wno-incompatible-pointer-types -Wno-deprecated-declarations -Wno-unused-value -Wno-empty-body -Wno-unused-but-set-variable
DLLFLAGS = -nologo -debug -incremental:no -opt:ref -opt:icf -dll -guard:cf
MAVX2    = -mavx2
!IF "$(BITS)" == "32"
CFLAGS   = --target=i686-pc-windows-msvc $(CFLAGS) 
!ENDIF
USE_CLANG = 1
!ELSE
!ERROR Unknown compiler
!ENDIF

XPCFLAGS = -D "_USING_V110_SDK71_"
XPLFALGS = -subsystem:console,5.01
RELEASE  = -D "NDEBUG" -O2 -MD
DEBUG_L  = -D "DEBUG" -D "_DEBUG" -D "APP_DEBUG=1" -Od -MD
HIDE     = -subsystem:windows,6.01
NO_HIDE  = -subsystem:console

##############################################################################
##
INCD  = $(ROOT)\src
BIND  = $(ROOT)\Release
OBJD  = $(ROOT)\.dep
