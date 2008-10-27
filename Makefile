CC = gcc
MAKE=make
SRC = $(wildcard *.c)

OBJ = $(SRC:.c=.o)
EXE = fwife

ifeq ($(PREFIX),)
	PREFIX = /usr
endif
ifeq ($(DEBUG),)
	DEBUG = no
endif

BINDIR = $(PREFIX)/bin
SHAREDIR = $(PREFIX)/share/fwife
LIBDIR = $(PREFIX)/lib/fwife

ifeq ($(DEBUG),yes)
	CFLAGS += -Wall -g
endif

LDFLAGS += -ldl -rdynamic

CFLAGS += $(shell pkg-config --cflags glib-2.0)
LDFLAGS += $(shell pkg-config --libs glib-2.0)

CFLAGS += $(shell pkg-config --cflags gtk+-2.0)
LDFLAGS += $(shell pkg-config --libs gtk+-2.0)

all: $(EXE)
.PHONY: all

# Compilation d'objets
%.o: %.c
	$(CC) -c $^ -o $@ $(CFLAGS) $(LDFLAGS)
	$(MAKE) -C plugins

# Compilation de l'executable
$(EXE): $(OBJ)
	$(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS)

install:
	install -d -m0755 $(DESTDIR)$(BINDIR)
	install -d -m0755 $(DESTDIR)$(LIBDIR)/plugins
	install -d -m0755 $(DESTDIR)$(SHAREDIR)/images/flags
	install -m0755 fwife $(DESTDIR)$(BINDIR)
	install -m0755 plugins/*.so $(DESTDIR)$(LIBDIR)/plugins
	install -m0644 images/*.png $(DESTDIR)$(SHAREDIR)/images
	install -m0644 images/flags/* $(DESTDIR)$(SHAREDIR)/images/flags

uninstall:
	rm -rf $(SHAREDIR)
	rm -rf $(LIBDIR)
	rm $(BINDIR)/fwife

clean: 
	rm $(EXE) *.o
	$(MAKE) -C plugins clean