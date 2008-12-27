CC = gcc
MAKE=make
SRC = $(wildcard *.c)
DEBUG=yes

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
	CFLAGS += -g
endif

CFLAGS += -Wall

# define arch in src
CFLAGS += -DARCH="\"$(shell arch)\"" 
#define plugin directory
CFLAGS += -DPLUGINDIR="\"$(DESTDIR)$(LIBDIR)/plugins\""
#define image directory
CFLAGS += -DIMAGEDIR="\"$(DESTDIR)$(SHAREDIR)/images\""

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
	$(MAKE) -C plugins SHAREDIR=$(SHAREDIR) LIBDIR=$(LIBDIR) DESTDIR=$(DESTDIR)

# Compilation de l'executable
$(EXE): $(OBJ)
	$(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS)

install:
	install -d -m0755 $(DESTDIR)$(BINDIR)
	install -d -m0755 $(DESTDIR)$(LIBDIR)/plugins
	install -d -m0755 $(DESTDIR)$(SHAREDIR)/images/flags
	install -d -m0755 $(DESTDIR)$(SHAREDIR)/kmconf
	install -d -m0755 $(DESTDIR)$(SHAREDIR)/scripts
	install -m0755 fwife $(DESTDIR)$(BINDIR)
	install -m0755 plugins/*.so $(DESTDIR)$(LIBDIR)/plugins
	install -m0644 images/*.png $(DESTDIR)$(SHAREDIR)/images
	install -m0644 images/flags/* $(DESTDIR)$(SHAREDIR)/images/flags
	install -m0644 plugins/kmconf/* $(DESTDIR)$(SHAREDIR)/kmconf
	install -m0755 plugins/scripts/* $(DESTDIR)$(SHAREDIR)/scripts
	

uninstall:
	rm -rf $(SHAREDIR)
	rm -rf $(LIBDIR)
	rm $(BINDIR)/fwife

clean: 
	rm $(EXE) *.o
	$(MAKE) -C plugins clean