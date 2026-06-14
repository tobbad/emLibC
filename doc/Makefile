# Minimal Makefile for LaTeX project

LATEX=pdflatex -shell-escape
MAIN=main

all: pdf pdf_en

pdf:
	$(LATEX) $(MAIN).tex
	$(LATEX) $(MAIN).tex

pdf_en:
	$(LATEX) $(MAIN)_en.tex
	$(LATEX) $(MAIN)_en.tex

clean:
	rm -f *.aux *.log *.toc *.out *.lof *.lot *.lol *.idx *.ilg *.ind *.fls *.fdb_latexmk

distclean: clean
	rm -f $(MAIN).pdf $(MAIN)_en.pdf

tar:
	tar  -cvf $(MAIN).tar *.tex
	tar  -rf  $(MAIN).tar  Makefile
	tar  -rf  $(MAIN).tar  pic/*
	gzip -f $(MAIN).tar

#dot: 	statemachine.png
#	cat dot/radio_statemachine.dot | dot -Tpng > statemachine.png
