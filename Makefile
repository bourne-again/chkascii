INSTALL       = /usr/bin/env install
PREFIX        = /usr
bindir        = $(PREFIX)/bin
mandir        = $(PREFIX)/share/man

install:
		$(INSTALL) -d $(bindir)
		$(INSTALL) -m755 chkascii $(DESTDIR)$(bindir)
		$(INSTALL) -d $(mandir)/man1
		$(INSTALL) -m644 chkascii.1 $(mandir)/man1
