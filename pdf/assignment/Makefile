BASE=a5
TEX=$(BASE).tex
PDF=$(BASE).pdf

$(PDF): $(TEX) Makefile
	@pdflatex $(BASE)
	@pdflatex $(BASE)

show: $(PDF)
	@evince $(PDF)

clean:
	@rm -f $(BASE).{aux,dvi,log,bbl,blg,pdf}
