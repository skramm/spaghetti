# makefile for Spaghetti
# author: S. Kramm, 2018
# hosted on  https://github.com/skramm/spaghetti

COLOR_1=-e "\e[1;33m"
COLOR_2=-e "\e[1;34m"
COLOR_3=-e "\e[1;35m"
COLOR_OFF="\e[0m"

DEST_PATH:=/usr/local/include/
THE_FILE:=spaghetti.hpp

#--------------------------------
# general compiler flags
CFLAGS = -std=c++11 -Wall -O2 -I.

# build options

OPTIONS:= \
SPAG_PRINT_STATES \
SPAG_ENABLE_LOGGING \
SPAG_FRIENDLY_CHECKING

LIST:=file1 file2
OPT:=A B


# this is needed for the demo programs
LDFLAGS += -lboost_system -lboost_thread -pthread

# needed, so object files that are inner part of successive pattern rules don't get erased at the end of build
#.PRECIOUS: obj/demo/%.o obj/lib/release/%.o obj/lib/debug/%.o
.SECONDARY:

# disable implicit rules
.SUFFIXES:

# list of targets that are NOT files
.PHONY: all clean cleanall doc show

SHELL=/bin/bash

BIN_DIR=bin
SRC_DIR=src
OBJ_DIR=obj


HEADER_FILES := $(wildcard $(SRC_DIR)/*.h*)
SRC_FILES    := $(wildcard $(SRC_DIR)/*.cpp)
OBJ_FILES    := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC_FILES))
EXEC_FILES   := $(patsubst $(SRC_DIR)/%.cpp,$(BIN_DIR)/%,$(SRC_FILES))

FILES=$(basename $(SRC_FILES))

OPT_ALL:= \
	$(foreach a, $(FILES), \
		$(foreach b, $(OPT), \
			$(foreach c, $(OPT), \
				$(foreach d, $(OPT), $(a)_$(b)$(c)$(d).obj ) \
			) \
		) \
	)


opt:
	@for a in 0 1; do \
		for b in 0 1; do \
			for c in 0 1; do \
				echo "$$a$$b$$c"; \
			done \
		done \
	done

help:
	@echo "This is not a program but a header-only library. Therefore, it is not supposed to be build"
	@echo -e "If you want to build the sample programs, try target 'demo'.\n"
	@echo "* Available targets:"
	@echo " - demo"
	@echo " - clean: erases demo object files"
	@echo " - cleandoc: erases doxygen-produced files"
	@echo " - cleanall: the two above, and also erases the build demo programs"
	@echo " - demo"
	@echo " - doc (assumes doxygen installed)"
	@echo " - install: copies single file to $(DEST_PATH)"


# default target
demo: $(EXEC_FILES)
	@echo "- Done target $@"

doc: html/index.html src/html/index.html
	@echo "- Done target $@"

html/index.html: $(THE_FILE) doxyfile README.md src/spaghetti.css
	doxygen doxyfile

src/html/index.html: $(SRC_FILES)
	cd src; doxygen doxyfile

install:
	cp spaghetti.hpp $(DEST_PATH)



#dot: $(SVG_FILES)
#	@echo "- Done target $@"

# dot, circo, ...
#GRAPHIZ_APP = dot

#%.svg: %.dot
#	$(GRAPHIZ_APP) -Tsvg -Grankdir=LR -Nfontsize=24 $< >$@
#	$(GRAPHIZ_APP) -Tsvg $< >$@


show:
	@echo HEADER_FILES=$(HEADER_FILES)
	@echo SRC_FILES=$(SRC_FILES)
	@echo OBJ_FILES=$(OBJ_FILES)
	@echo OPT_ALL=$(OPT_ALL)
	@echo EXEC_FILES=$(EXEC_FILES)
	@echo DOT_FILES=$(DOT_FILES)
	@echo OPTIONS=$(OPTIONS)


clean:
	-rm $(OBJ_DIR)/*

cleandoc:
	-rm -r html/*
	-rm -r src/html/*

cleanbin:
	-rm $(BIN_DIR)/*

cleanall: clean cleandoc cleanbin
	@echo "done"



# generic compile rule
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADER_FILES) $(THE_FILE)
	@echo $(COLOR_2) " - Compiling app file $<." $(COLOR_OFF)
	$(CXX) -o $@ -c $< $(CFLAGS)

# linking
$(BIN_DIR)/%: $(OBJ_DIR)/%.o $(THE_FILE)
	@echo $(COLOR_3) " - Link demo $@." $(COLOR_OFF)
	$(CXX) -o $@ -s $(subst $(BIN_DIR)/,$(OBJ_DIR)/,$@).o  $(LDFLAGS)


