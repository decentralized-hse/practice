TEX = pdflatex
ARTICLE = article

.PHONY: all clean view

all: $(ARTICLE).pdf

$(ARTICLE).pdf: $(ARTICLE).tex
	$(TEX) $(ARTICLE).tex
	$(TEX) $(ARTICLE).tex

clean:
	rm -f $(ARTICLE).{aux,log,out,toc,pdf,bbl,blg,fls,fdb_latexmk,synctex.gz}

view: $(ARTICLE).pdf
	open $(ARTICLE).pdf
