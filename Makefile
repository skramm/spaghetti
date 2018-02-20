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
SPAG_FRIENDLY_CHECKING \
SPAG_ENUM_STRINGS \
SPAG_EXTERNAL_EVENT_LOOP \
SPAG_GENERATE_DOTFILE

#LIST:=file1 file2
#OPT:=A B


# this is needed for the demo programs
LDFLAGS += -lboost_system -lboost_thread -pthread

# needed, so object files that are inner part of successive pattern rules don't get erased at the end of build
#.PRECIOUS: obj/demo/%.o obj/lib/release/%.o obj/lib/debug/%.o
.SECONDARY:

# disable implicit rules
.SUFFIXES:

# list of targets that are NOT files
.PHONY: all clean cleanall doc show diff

SHELL=/bin/bash

BIN_DIR=bin
SRC_DIR=src
SRC_DIR_T=tests
OBJ_DIR=obj


HEADER_FILES := $(wildcard $(SRC_DIR)/*.h*)
SRC_FILES    := $(wildcard $(SRC_DIR)/*.cpp)
SRC_FILES_T  := $(wildcard $(SRC_DIR_T)/*.cpp)
OBJ_FILES    := $(patsubst $(SRC_DIR)/%.cpp,   $(OBJ_DIR)/%.o, $(SRC_FILES))
OBJ_FILES_T  := $(patsubst $(SRC_DIR_T)/%.cpp, $(OBJ_DIR)/%.o, $(SRC_FILES_T))
EXEC_FILES   := $(patsubst $(SRC_DIR)/%.cpp,   $(BIN_DIR)/%,   $(SRC_FILES))
EXEC_FILES_T := $(patsubst $(SRC_DIR_T)/%.cpp, $(BIN_DIR)/%,   $(SRC_FILES_T))

DOT_FILES := $(wildcard *.dot)
DOT_FILES += $(wildcard src/*.dot)
SVG_FILES := $(DOT_FILES:.dot=.svg)

#FILES=$(basename $(SRC_FILES))

#OPT_ALL:= \
#	$(foreach a, $(FILES), \
#		$(foreach b, $(OPT), \
#			$(foreach c, $(OPT), \
#				$(foreach d, $(OPT), $(a)_$(b)$(c)$(d).obj ) \
#			) \
#		) \
#	)


# default target
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

demo: $(EXEC_FILES) $(EXEC_FILES_T)
	@echo "- Done target $@"

tests: $(EXEC_FILES_T)
	@echo "- Done target $@"

doc: html/index.html src/html/index.html
	@echo "- Done target $@"

html/index.html: $(THE_FILE) doxyfile README.md src/spaghetti.css
	@echo "* Processing Doxygen on main file"
	doxygen doxyfile

src/html/index.html: $(SRC_FILES) $(HEADER_FILES) src/doxyfile
	@echo "* Processing Doxygen on sample programs"
	cd src; doxygen doxyfile

install:
	cp spaghetti.hpp $(DEST_PATH)

dot: $(SVG_FILES)
	@echo "- Done target $@"

# dot, circo, ...
GRAPHIZ_APP = dot

%.svg: %.dot
	$(GRAPHIZ_APP) -Tsvg $< >$@

show:
	@echo HEADER_FILES=$(HEADER_FILES)
	@echo SRC_FILES=$(SRC_FILES)
	@echo SRC_FILES_T=$(SRC_FILES_T)
	@echo OBJ_FILES=$(OBJ_FILES)
	@echo OBJ_FILES_T=$(OBJ_FILES_T)
#	@echo OPT_ALL=$(OPT_ALL)
	@echo EXEC_FILES=$(EXEC_FILES)
	@echo EXEC_FILES_T=$(EXEC_FILES_T)
	@echo DOT_FILES=$(DOT_FILES)
	@echo OPTIONS=$(OPTIONS)
	@echo SVG_FILES=$(SVG_FILES)

diff:
	git diff | colordiff | aha > diff.html
	xdg-open diff.html
#	rm diff.html

clean:
	-rm $(OBJ_DIR)/*
	-rm *.dot *.svg
	-rm diff.html

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

$(OBJ_DIR)/%.o: $(SRC_DIR_T)/%.cpp $(HEADER_FILES) $(THE_FILE)
	@echo $(COLOR_2) " - Compiling app file $<." $(COLOR_OFF)
	$(CXX) -o $@ -c $< $(CFLAGS)

# linking
$(BIN_DIR)/%: $(OBJ_DIR)/%.o $(THE_FILE)
	@echo $(COLOR_3) " - Link demo $@." $(COLOR_OFF)
	$(CXX) -o $@ -s $(subst $(BIN_DIR)/,$(OBJ_DIR)/,$@).o  $(LDFLAGS)

#$(BIN_DIR)/%: $(OBJ_DIR)/%.o $(THE_FILE)
#	@echo $(COLOR_3) " - Link demo $@." $(COLOR_OFF)
#	$(CXX) -o $@ -s $(subst $(BIN_DIR)/,$(OBJ_DIR_T)/,$@).o  $(LDFLAGS)


