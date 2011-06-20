OUTPUT=024冯时2007013247
MAIN=main.pdf
SCAN1=scan/scan1.pdf
SCAN2=scan/scan2.pdf
SCAN3=scan/scan3.pdf
SCAN4=scan/scan4.pdf
JOINFLAGS=--outfile $(OUTPUT).pdf --a4paper --rotateoversize 'false'

all:
	xelatex main
	bibtex main
	xelatex main
	xelatex main
	pdfjoin $(JOINFLAGS) $(MAIN) '1,2'  $(SCAN1) '-'\
                             $(MAIN) '4-55' $(SCAN2) '-'\
                             $(MAIN) '57-' $(SCAN3) '-' $(SCAN4) '-'
	zip $(OUTPUT) $(OUTPUT).pdf main.tex data/*.tex
