CFLAGS = -I../..
ifeq ($(OS),Windows_NT)
	LDFLAGS = -lopengl32 -lgdi32
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Darwin)
		LDFLAGS = -framework OpenGL -framework Cocoa
	endif
endif

opengl: opengl.c ../../tigr.c
	gcc $^ -s -Os -o $@ $(CFLAGS) $(LDFLAGS)