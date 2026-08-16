LATEXMK := latexmk
SOURCE := main.tex

.PHONY: all clean

all:
	$(LATEXMK) -xelatex -interaction=nonstopmode -halt-on-error $(SOURCE)

clean:
	$(LATEXMK) -C $(SOURCE)
