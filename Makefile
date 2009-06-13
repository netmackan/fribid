
SUBDIRS=main plugin

all clean install uninstall:
	for dir in $(SUBDIRS); do (cd $$dir && $(MAKE) $@); done

distclean: clean
	rm -f common/config.h

.PHONY: all clean install uninstall $(SUBDIRS)

