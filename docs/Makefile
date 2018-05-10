# makefile used to build manual graph figures
# author: S. Kramm, 2018
# hosted on  https://github.com/skramm/spaghetti

COLOR_1=-e "\e[1;33m"
COLOR_2=-e "\e[1;34m"
COLOR_3=-e "\e[1;35m"
COLOR_OFF="\e[0m"



DOT_FILES := $(wildcard *.dot)
SVG_FILES := $(DOT_FILES:.dot=.svg)

all: $(SVG_FILES)
	@echo "done"

%.svg: %.dot Makefile
	dot -Tsvg $< >$@

