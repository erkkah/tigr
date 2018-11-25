CFLAGS = -I../..
ifeq ($(OS),Windows_NT)
	LDFLAGS = -ld3d9
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Darwin)
		LDFLAGS = -framework OpenGL -framework Cocoa
	else ifeq ($(UNAME_S),Linux)
		LDFLAGS = -lGLU -lGL -lX11 -g
	endif
endif

hello : hello.c ../../tigr.c
	gcc $^ -s -Os -o $@ $(CFLAGS) $(LDFLAGS)
