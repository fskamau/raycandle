CC = gcc

CFLAGS = -Wextra -Wall  -fPIC -std=c99 -ggdb $(shell pkg-config --cflags raylib)
LDFLAGS = $(shell pkg-config --libs raylib 2>/dev/null || echo "-L/usr/local/lib -lraylib") -lm -lpthread -lGL

TARGET_FOLDER = ./build
PY_PKG_FOLDER = ../raycandle
ASSETS_FOLDER = ./assets

SOURCES = utils.c str.c figure.c node.c axes.c ready_signal.c artist.c
OBJECTS = $(patsubst %.c,$(TARGET_FOLDER)/%.o, $(SOURCES))

all: $(TARGET_FOLDER)/libraycandle.so.1  link_to_local 

link_to_local: $(TARGET_FOLDER)/libraycandle.so.1
	$(shell [ ! -e $(TARGET_FOLDER)/libraycandle.so ] && ln -sr $(TARGET_FOLDER)/libraycandle.so.1  $(TARGET_FOLDER)/libraycandle.so)

cp_h_2_build: install
	cp  raycandle.h node.h  $(TARGET_FOLDER)
	cp -r $(PY_PKG_FOLDER)/$(ASSETS_FOLDER) $(TARGET_FOLDER)/

$(TARGET_FOLDER):
	mkdir -p $(TARGET_FOLDER)

$(TARGET_FOLDER)/%.o: %.c | $(TARGET_FOLDER)
	$(CC) $(CFLAGS) -c -o $@ $<

$(TARGET_FOLDER)/libraycandle.so.1: $(OBJECTS)
	$(CC) -shared -o $@ -Wl,-soname,libraycandle.so.1 $(OBJECTS) $(LDFLAGS)


cffi: $(TARGET_FOLDER)/raycandle.h

$(TARGET_FOLDER)/raycandle.h: $(TARGET_FOLDER)
	python3 -c "import re; lines=open('raycandle.h').readlines(); print(''.join([line for line in lines if not re.search(r'^\s*#', line)]))" \
	| $(CC) -E -C -P -o $@ -

install: all cffi
	@if [ ! -d "$(PY_PKG_FOLDER)" ]; then \
		echo "ERROR: '$(PY_PKG_FOLDER)' does not exist" >&2; \
		exit 1; \
	fi
	cp  $(TARGET_FOLDER)/libraycandle.so.1 $(PY_PKG_FOLDER)/libraycandle.so
	cp  $(TARGET_FOLDER)/raycandle.h $(PY_PKG_FOLDER)/
	cp -r $(ASSETS_FOLDER) $(PY_PKG_FOLDER)/

clean:
	rm -rf $(TARGET_FOLDER)

.PHONY: all cffi install clean
