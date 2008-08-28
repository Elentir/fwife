CC = gcc
MAKE=make
DEBUG = yes
SRC = $(wildcard *.c)

OBJ = $(SRC:.c=.o)
EXE = fwife

LDFLAGS += -ldl -rdynamic

ifeq ($(DEBUG),yes)
	CFLAGS = -Wall -g
else
	CFLAGS =  -Wall -O2  -pipe
endif

CFLAGS += $(shell pkg-config --cflags glib-2.0)
LDFLAGS += $(shell pkg-config --libs glib-2.0)

CFLAGS += -DGTK
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