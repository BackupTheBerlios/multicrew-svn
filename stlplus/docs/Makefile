################################################################################
# Makefile for updating colourised HTML versions of header files
# This only needs to be run by developers, not end-uers

HPP_FILES = $(wildcard ../source/*.hpp)
HTML_FILES = $(patsubst %.hpp,%_hpp.html,$(notdir $(HPP_FILES)))

################################################################################
# rules

all: $(HTML_FILES)

%_hpp.html: ../source/%.hpp
	ccolour -noversion -html -nortf -title $(notdir $<) $<

tidy:

clean:
	rm *_hpp.html
