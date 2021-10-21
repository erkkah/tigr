CFLAGS = -I..
ifeq ($(OS),Windows_NT)
	LDFLAGS = -s -lopengl32 -lgdi32
	EXT = .exe
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Darwin)
		LDFLAGS = -framework OpenGL -framework Cocoa
	else ifeq ($(UNAME_S),Linux)
		LDFLAGS = -s -lGLU -lGL -lX11
	endif
endif

ci : ci.c ../tigr.c
	gcc $^ -o $@$(EXT) $(CFLAGS) $(LDFLAGS)
