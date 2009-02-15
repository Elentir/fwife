CC = gcc
MAKE=make
SRC = $(wildcard *.c)
DEBUG=yes

OBJ = $(SRC:.c=.o)
EXE = fwife

ifeq ($(DEBUG),)
	CFLAGS += -DNDEBUG
endif

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
	$(MAKE) -C plugins

# Compilation de l'executable
$(EXE): $(OBJ)
	$(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS)

clean: 
	rm $(EXE) *.o
	$(MAKE) -C plugins clean