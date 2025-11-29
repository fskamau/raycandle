CC=gcc

export PKG_CONFIG_PATH=../../raylib/lib/pkgconfig


CFLAGS=-Wextra -Wall -O3 -fPIC  -std=c99 $(shell pkg-config --cflags raylib)
LDFLAGS=$(shell pkg-config --libs raylib)
TARGET_FOLDER=./build
SOURCES=ready_signal.c utils.c artist.c axes.c figure.c locator.c mouse_updater.c 
OBJECTS=$(patsubst %.c,$(TARGET_FOLDER)/%.o, $(SOURCES))

all: raycandle.so raycandle_for_cffi.h clean mv

$(TARGET_FOLDER):
	mkdir -p $(TARGET_FOLDER)
$(TARGET_FOLDER)/%.o: %.c | $(TARGET_FOLDER)
	$(CC) $(CFLAGS)  -c -o $@  $<
raycandle.so:$(OBJECTS)
	$(CC)  -shared -o $(TARGET_FOLDER)/libraycandle.so.1  -Wl,-soname,libraycandle.so $(OBJECTS) $(LDFLAGS)
raycandle_for_cffi.h:raycandle.so
	python -c "import re;lines=open('raycandle.h').readlines();\
	print(''.join([line for line in lines if  not re.search(r'^\S*?#',line)]))"\
	| gcc  -E -C -o $(TARGET_FOLDER)/raycandle_for_cffi.h -
clean:
	rm $(TARGET_FOLDER)/*.o
mv:
	cp build/* ../





