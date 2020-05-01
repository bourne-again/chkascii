install:
		c++ -o chkascii chkascii.cc
		cp chkascii /usr/bin/
		cp chkascii.1 /usr/share/man/man1/
