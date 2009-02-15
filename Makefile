MAKE=make

ifeq ($(PREFIX),)
	PREFIX = /usr
endif

export BINDIR = $(PREFIX)/bin
export SHAREDIR = $(PREFIX)/share/fwife
export LIBDIR = $(PREFIX)/lib/fwife
export LOCALEDIR = $(PREFIX)/share/locale

all:
	$(MAKE) -C src
	$(MAKE) -C po pos
	$(MAKE) -C po mos

clean:
	$(MAKE) -C src clean
	$(MAKE) -C po distclean

install:
	install -d -m0755 $(DESTDIR)$(BINDIR)
	install -d -m0755 $(DESTDIR)$(LIBDIR)/plugins
	install -d -m0755 $(DESTDIR)$(SHAREDIR)/images/flags
	install -d -m0755 $(DESTDIR)$(SHAREDIR)/kmconf
	install -d -m0755 $(DESTDIR)$(SHAREDIR)/scripts
	install -m0755 src/fwife $(DESTDIR)$(BINDIR)
	install -m0755 src/plugins/*.so $(DESTDIR)$(LIBDIR)/plugins
	install -m0644 src/images/*.png $(DESTDIR)$(SHAREDIR)/images
	install -m0644 src/images/flags/* $(DESTDIR)$(SHAREDIR)/images/flags
	install -m0644 src/plugins/kmconf/* $(DESTDIR)$(SHAREDIR)/kmconf
	install -m0755 src/plugins/scripts/* $(DESTDIR)$(SHAREDIR)/scripts
	cp -r po/mofiles/* $(DESTDIR)$(LOCALEDIR)

uninstall:
	rm -rf $(SHAREDIR)
	rm -rf $(LIBDIR)
	rm $(BINDIR)/fwife
	rm `find $(LOCALEDIR) -name fwife.mo`
	


