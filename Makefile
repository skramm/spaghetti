# makefile for Spaghetti
# author: S. Kramm, 2018 - 2025
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

ifeq "$(DEBUG)" "Y"
	CFLAGS += -DSPAG_PRINT_STATES
endif

# build options

OPTIONS:= \
SPAG_PRINT_STATES \
SPAG_ENABLE_LOGGING \
SPAG_FRIENDLY_CHECKING \
SPAG_ENUM_STRINGS \
SPAG_EXTERNAL_EVENT_LOOP \
SPAG_EMBED_ASIO_WRAPPER \
SPAG_USE_ASIO_WRAPPER \
SPAG_USE_SIGNALS



# this is needed for the demo programs
LDFLAGS += -lboost_system -lboost_thread -pthread

# needed, so object files that are inner part of successive pattern rules don't get erased at the end of build
#.PRECIOUS: obj/demo/%.o obj/lib/release/%.o obj/lib/debug/%.o
.SECONDARY:

# disable implicit rules
.SUFFIXES:

# list of targets that are NOT files
.PHONY: all clean cleanall doc show diff test

SHELL=/bin/bash

BIN_DIR=BUILD/bin
SRC_DIR=src
SRC_DIR_T=tests
OBJ_DIR=BUILD/obj

# suffix _T is for the test files
HEADER_FILES := $(wildcard $(SRC_DIR)/*.h*)
SRC_FILES    := $(wildcard $(SRC_DIR)/*.cpp)
SRC_FILES_T  := $(wildcard $(SRC_DIR_T)/testA_*.cpp)
OBJ_FILES    := $(patsubst $(SRC_DIR)/%.cpp,   $(OBJ_DIR)/%.o, $(SRC_FILES))
OBJ_FILES_T  := $(patsubst $(SRC_DIR_T)/%.cpp, $(OBJ_DIR)/%.o, $(SRC_FILES_T))
EXEC_FILES   := $(patsubst $(SRC_DIR)/%.cpp,   $(BIN_DIR)/%,   $(SRC_FILES))
EXEC_FILES_T := $(patsubst $(SRC_DIR_T)/%.cpp, $(BIN_DIR)/%,   $(SRC_FILES_T))

DOT_FILES := $(wildcard *.dot)
DOT_FILES += $(wildcard src/*.dot)
SVG_FILES := $(DOT_FILES:.dot=.svg)


# default target
help:
	@echo "This is not a program but a header-only library. Therefore, it is not supposed to be build."
	@echo -e "If you want to build the sample programs, try target 'demo'.\n"
	@echo "* Available targets:"
	@echo " - demo: builds the provided demo programs (needs Boost installed)"
	@echo " - clean: erases demo object files"
	@echo " - cleandoc: erases doxygen-produced files"
	@echo " - cleanall: the two above, and also erases the build demo programs"
	@echo " - doc: build ref. manual, using Doxygen (needs to be installed)"
	@echo " - install: copies single file header to $(DEST_PATH)"
	@echo " - test: builds and run the test code"

demo: $(EXEC_FILES)
	@echo "- Done target $@"

# build and run the tests apps, and compare to the expected output
test: $(EXEC_FILES_T) nobuild
	cd $(BIN_DIR); for f in $(EXEC_FILES_T); \
		do \
			echo -e "\n***********************************\nRunning test program $$f:"; \
			./$$(basename $$f) >$$(basename $$f).stdout; \
			cmp ../../tests/$$(basename $$f).stdout $$(basename $$f).stdout; \
		done;

#	mv *.dot BUILD/


NOBUILD_SRC_FILES := $(wildcard tests/nobuild_*.cpp)
NOBUILD_OBJ_FILES := $(patsubst %.cpp, %.o, $(NOBUILD_SRC_FILES))

nobuild: $(NOBUILD_OBJ_FILES)
	@echo "done target $@"

tests/nobuild_%.o: tests/nobuild_%.cpp
	@echo "Checking build failure of $<" >>BUILD/no_build.stdout
	@echo -e "-----------------------------\nChecking build failure of $<\n" >>BUILD/no_build.stderr
	@! $(CXX) -o $@ -c $< 1>>BUILD/no_build.stdout 2>>BUILD/no_build.stderr


doc: BUILD/doc_html/index.html BUILD/doc_samples_html/index.html
	-xdg-open $<
	@echo "- Done target $@"

BUILD/doc_html/index.html: $(THE_FILE) misc/doxyfile README.md misc/spaghetti.css docs/*
	@echo "* Processing Doxygen on main file"
	mkdir -p BUILD/doc_html
	doxygen misc/doxyfile

BUILD/doc_samples_html/index.html: $(SRC_FILES) $(HEADER_FILES) src/doxyfile
	@echo "* Processing Doxygen on sample programs"
	mkdir -p BUILD/doc_samples_html
	doxygen src/doxyfile

install:
	cp spaghetti.hpp $(DEST_PATH)

dot: $(SVG_FILES)
	@echo "- Done target $@"

# dot, circo, ...
GRAPHIZ_APP = dot
#GRAPHIZ_APP = circo

%.svg: %.dot Makefile
	$(GRAPHIZ_APP) -Tsvg $< >$@

show:
	@echo HEADER_FILES=$(HEADER_FILES)
	@echo SRC_FILES=$(SRC_FILES)
	@echo SRC_FILES_T=$(SRC_FILES_T)
	@echo OBJ_FILES=$(OBJ_FILES)
	@echo OBJ_FILES_T=$(OBJ_FILES_T)
	@echo EXEC_FILES=$(EXEC_FILES)
	@echo EXEC_FILES_T=$(EXEC_FILES_T)
	@echo DOT_FILES=$(DOT_FILES)
	@echo OPTIONS=$(OPTIONS)
	@echo SVG_FILES=$(SVG_FILES)

# special target, for dev use only
diff:
	git diff | colordiff | aha > /tmp/diff.html
	-xdg-open /tmp/diff.html

clean:
	-rm $(OBJ_DIR)/*
	-rm *.dot *.svg *.stdout
	-rm diff.html

cleandoc:
	@-rm -r BUILD/doc_html/*
	@-rm -r BUILD/doc_samples_html/*

cleanbin:
	-rm $(BIN_DIR)/*

cleanall: clean cleandoc cleanbin
	@echo "done"

mkfolders:
	mkdir -p $(OBJ_DIR)
	mkdir -p $(BIN_DIR)

# generic compile rule
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADER_FILES) $(THE_FILE) Makefile mkfolders
	@echo $(COLOR_2) " - Compiling app file $<." $(COLOR_OFF)
	$(CXX) -o $@ -c $< $(CFLAGS)

# for test files
$(OBJ_DIR)/%.o: $(SRC_DIR_T)/%.cpp $(HEADER_FILES) $(THE_FILE) mkfolders
	@echo $(COLOR_2) " - Compiling app file $<." $(COLOR_OFF)
	@$(CXX) -o $@ -c $< $(CFLAGS)

# linking
$(BIN_DIR)/%: $(OBJ_DIR)/%.o $(THE_FILE)
	@echo $(COLOR_3) " - Link demo $@." $(COLOR_OFF)
	@$(CXX) -o $@ -s $(subst $(BIN_DIR)/,$(OBJ_DIR)/,$@).o  $(LDFLAGS)

#$(BIN_DIR)/%: $(OBJ_DIR)/%.o $(THE_FILE)
#	@echo $(COLOR_3) " - Link demo $@." $(COLOR_OFF)
#	$(CXX) -o $@ -s $(subst $(BIN_DIR)/,$(OBJ_DIR_T)/,$@).o  $(LDFLAGS)


