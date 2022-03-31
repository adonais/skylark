# This makefile should be included in the main scintilla.mak file,
# just after where LOBJS is defined (around line 
# 
# The following line should be added around line 211 of scintilla.mak
# !INCLUDE nppSpecifics.mak

# Set your boost path (the root of where you've unpacked your boost zip).
# Boost is available from www.boost.org

SRC_OBJS=\
	$(SRC_OBJS)\
	$(DIR_O)\BoostRegexSearch.obj\
	$(DIR_O)\UTF8DocumentIterator.obj

INCLUDES=$(INCLUDES) -I../boost/include

CXXFLAGS=$(CXXFLAGS) -DNO_CXX11_REGEX -DSCI_OWNREGEX


$(DIR_O)\UTF8DocumentIterator.obj:: src/boostregex/UTF8DocumentIterator.cxx
	$(CC) $(CXXFLAGS) -D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS -c $(NAME)$(DIR_O)\ src/boostregex/UTF8DocumentIterator.cxx	
	
$(DIR_O)\BoostRegexSearch.obj:: src/boostregex/BoostRegexSearch.cxx src/src/CharClassify.h src/src/RESearch.h	
	$(CC) $(CXXFLAGS) -D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS -c $(NAME)$(DIR_O)\ src/boostregex/BoostRegexSearch.cxx

