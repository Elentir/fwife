CFLAGS += -g -Wall -Werror
LDFLAGS += -lparted -ldl -lpacman -lvolume_id

CFLAGS += $(shell pkg-config --cflags gtk+-2.0)
LDFLAGS += $(shell pkg-config --libs gtk+-2.0)

CFLAGS += $(shell pkg-config --cflags glib-2.0)
LDFLAGS += $(shell pkg-config --libs glib-2.0)

#fwtimeconfig
CFLAGS += -I../libfwtimeconfig
LDFLAGS += -L../libfwtimeconfig -lfwtimeconfig

# fwgrubconfig
CFLAGS += -I../libfwgrubconfig
LDFLAGS += -L../libfwgrubconfig -lfwgrubconfig

# fwnetconfig
CFLAGS += -I../libfwnetconfig
LDFLAGS += -L../libfwnetconfig -lfwnetconfig

# fwutil
CFLAGS += -I../libfwutil
LDFLAGS += -L../libfwutil -lfwutil

# fwmouseconfig
CFLAGS += -I../libfwmouseconfig
LDFLAGS += -L../libfwmouseconfig -lfwmouseconfig

# fwxconfig
CFLAGS += -I../libfwxconfig
LDFLAGS += -L../libfwxconfig -lfwxconfig

PLUGINS=greet configsource asklang layout install select partdisk timeconf grubconf usersconf timeconf netconf finish

all: $(addsuffix .so,$(PLUGINS))

clean:
	rm -f *.so *.o

test: all
	cd ..; PATH=$$PATH:/sbin:/usr/sbin sudo ./setup

%.so: %.o
	$(CC) $(CFLAGS) -shared -Wl,-soname,$(basename $@).so -o \
		$(basename $@).so $(basename $@).o $(LDFLAGS)
