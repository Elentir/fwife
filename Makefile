CC = gcc
MAKE=make
DEBUG = no
SRC = $(wildcard *.c)

OBJ = $(SRC:.c=.o)
EXE = fwife

BINDIR = /usr/bin
SHAREDIR = /usr/share/fwife

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
	install -d -m0755 $(DESTDIR)$(PREFIX)$(SHAREDIR)
	install -d -m0755 $(DESTDIR)$(PREFIX)$(SHAREDIR)/plugins
	install -d -m0755 $(DESTDIR)$(PREFIX)$(SHAREDIR)/images
	install -d -m0755 $(DESTDIR)$(PREFIX)$(SHAREDIR)/images/flags
	install -m0755 fwife $(DESTDIR)$(PREFIX)$(BINDIR)/fwife
	install -m0644 plugins/*.so $(DESTDIR)$(PREFIX)$(SHAREDIR)/plugins
	install -m0644 images/*.png $(DESTDIR)$(PREFIX)$(SHAREDIR)/images
	install -m0644 images/flags/* $(DESTDIR)$(PREFIX)$(SHAREDIR)/images/flags

uninstall:
	rm -rf $(PREFIX)$(SHAREDIR)
	rm $(PREFIX)$(BINDIR)/fwife

clean: 
	rm $(EXE) *.o
	$(MAKE) -C plugins clean