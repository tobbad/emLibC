# Minimal Makefile for LaTeX project

LATEX=pdflatex
MAIN=main

all: pdf dot

pdf:
	$(LATEX) $(MAIN).tex
	$(LATEX) $(MAIN).tex
	#$(LATEX) $(MAIN)_en.tex
	#$(LATEX) $(MAIN)_en.tex

clean:
	rm -f *.aux *.log *.toc *.out *.lof *.lot *.fls *.fdb_latexmk

distclean: clean
	rm -f $(MAIN).pdf

tar:
	tar  -cvf $(MAIN).tar *.tex
	tar  -rf  $(MAIN).tar  Makefile
	tar  -rf  $(MAIN).tar  pic/*
	gzip -f $(MAIN).tar  

dot: 	statemachine.png
	cat dot/radio_statemachine.dot | dot -Tpng > statemachine.png
