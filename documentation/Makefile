XELATEX := xelatex
SOURCE := main.tex

.PHONY: all clean

all:
	$(XELATEX) -interaction=nonstopmode -halt-on-error $(SOURCE)
	$(XELATEX) -interaction=nonstopmode -halt-on-error $(SOURCE)

clean:
	$(RM) main.aux main.log main.out main.toc main.pdf
