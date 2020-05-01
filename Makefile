INSTALL       = /usr/bin/env install
PREFIX        = /usr/local
bindir        = $(PREFIX)/bin
DESTDIR       =
mandir        = $(PREFIX)/share/man

install:
		$(INSTALL) -d $(DESTDIR)$(bindir)
		$(INSTALL) -d $(DESTDIR)$(mandir)/man1
		$(INSTALL) -m644 *.1 $(DESTDIR)$(mandir)/man1
