CFLAGS = -I../..
ifeq ($(OS),Windows_NT)
LDFLAGS = -ld3d9
endif

demo : demo.cpp
	g++ $^ -s -Os -o $@ $(CFLAGS) $(LDFLAGS)
