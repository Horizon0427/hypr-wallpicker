CC = gcc
CPPFLAGS = -Iinclude

CFLAGS = -O2 -Wall -Wextra -Werror
DEBUG_CFLAGS = -g -O0 -Wall -Wextra -Werror

LDFLAGS =
LDLIBS = -lraylib -lm -ldl -lpthread -lrt

SRC = src/main.c src/apply.c src/fs.c
TARGET = wallpicker

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS) $(LDLIBS)

debug: CFLAGS = $(DEBUG_CFLAGS)
debug: clean $(TARGET)

install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)

debug-install: debug
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)

clean:
	rm -f $(TARGET)

reinstall: clean uninstall install

.PHONY: all debug install debug-install uninstall clean reinstall
