# makefile for Spaghetti
# author: S. Kramm, 2018
# hosted on  https://github.com/skramm/spaghetti

COLOR_1=-e "\e[1;33m"
COLOR_2=-e "\e[1;34m"
COLOR_3=-e "\e[1;35m"
COLOR_OFF="\e[0m"


#--------------------------------
# general compiler flags
CFLAGS = -std=c++11 -Wall -O2 -I.

#LDFLAGS += -L/usr/lib/x86_64-linux-gnu/ -lboost_system -lboost_thread -pthread -lboost_iostreams -lboost_serialization
LDFLAGS += -L/usr/lib/x86_64-linux-gnu/ -lboost_system -lboost_thread -pthread -lboost_serialization

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

# default target
all: $(EXEC_FILES)
	@echo "- Done target $@"

doc: html/index.html
	@echo "- Done target $@"

html/index.html: $(SRC_FILES) $(HEADER_FILES) doxyfile
	doxygen doxyfile

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
	@echo EXEC_FILES=$(EXEC_FILES)
	@echo DOT_FILES=$(DOT_FILES)
	@echo SVG_FILES=$(SVG_FILES)


clean:
	-rm $(OBJ_DIR)/*

cleandoc:
	-rm -r html/*

cleanall: clean cleandoc
	-rm $(BIN_DIR)/*


# generic compile rule
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADER_FILES)
	@echo $(COLOR_2) " - Compiling app file $<." $(COLOR_OFF)
	$(CXX) -o $@ -c $< $(CFLAGS)

# linking
$(BIN_DIR)/%: $(OBJ_DIR)/%.o
	@echo $(COLOR_3) " - Link demo $@." $(COLOR_OFF)
	$(CXX) -o $@ -s $(subst $(BIN_DIR)/,$(OBJ_DIR)/,$@).o  $(LDFLAGS)


