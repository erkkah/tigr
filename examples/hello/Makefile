CFLAGS = -I../..
ifeq ($(OS),Windows_NT)
LDFLAGS = -ld3d9
endif

hello : hello.c ../../tigr.c
	gcc $^ -s -Os -o $@ $(CFLAGS) $(LDFLAGS)
