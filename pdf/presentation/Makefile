BASE=slides-a5
TEX=$(BASE).tex
DVI=$(BASE).dvi
PDF=$(BASE).pdf

# MP=youarehere.mp
# M1=$(MP:.mp=.1)

all: $(PDF)

$(PDF): $(TEX) $(PIC_DST) $(M1)
	pdflatex $(TEX)
	# dvipdf $(DVI)

%.1: %.mp
	mpost $<

.PHONY: show clean info
show: $(PDF)
	evince $(PDF)

clean:
	rm -rf $(BASE).aux 
	rm -rf $(BASE).log 
	rm -rf $(BASE).nav 
	rm -rf $(BASE).out
	rm -rf $(BASE).toc
	rm -rf $(BASE).snm 
	rm -rf *.log	
