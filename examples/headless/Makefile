CFLAGS += -I../.. -Wall -DTIGR_HEADLESS

headless : headless.c ../../tigr.c
	gcc $^ -Os -o $@ $(CFLAGS)
