all : ../tigr.c

.PHONY : ../tigr.c

tigr_font.h : font.png incbin.pl
	perl incbin.pl $@ $< tigr_font

../tigr.c : tigr_font.h bundle.pl
	perl bundle.pl $@ tigr_amalgamated.c
