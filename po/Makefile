top_srcdir =
PO_DIR=~/git/translations/po
LANGS = $(shell ls $(PO_DIR)/fwife)

pos:
	[ -d $(PO_DIR) ]
	for i in $(LANGS); \
	do \
		cp $(PO_DIR)/fwife/$$i/fwife.po $$i.po; \
		if ! msgfmt -c --statistics -o /dev/null $$i.po; then \
			echo "$$i.po would break your build!"; \
			exit 1; \
		fi; \
	done

mos:
	for i in $(wildcard *.po); do \
		echo -n "$$i: "; \
		mkdir -p ./mofiles/`basename $$i .po`/LC_MESSAGES/; \
		msgfmt -o ./mofiles/`basename $$i .po`/LC_MESSAGES/fwife.mo $$i; \
	done

pot: intltool-extract
	intltool-update --pot --gettext-package=fwife

intltool-extract:
	cp /usr/bin/intltool-extract ./

update-po: pot
	for lang in $(LANGS); do \
		if msgmerge $$lang.po fwife.pot -o $$lang.new.po; then \
			mv -f $$lang.new.po $$lang.po; \
		else \
			echo "msgmerge for $$cat failed!"; \
			rm -f $$lang.new.po; \
		fi; \
	done

distclean:
	rm -f intltool-extract *.pot *.po *.gmo
	rm -rf mofiles