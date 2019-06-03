VERSION=1.6.5.2
ARCHNAME=sqlupdate-$(VERSION)
ARCHDIR=archives/
ARCHFILES=sqlupdate.cc virhefiltteri.c \
          Makefile.sets Makefile.sets.in configure \
          COPYING progdesc.php

INSTALL=install
INSTALLPROGS=sqlupdate
BINDIR=/usr/local/bin

include Makefile.sets

all: sqlupdate

sqlupdate: sqlupdate.o
	$(CXX) -o $@ $(CXXFLAGS) $^ $(LDFLAGS)
sqlupdate.o: sqlupdate.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

sqlupdate.wo: sqlupdate.cc
	/opt/xmingw/bin/i386-mingw32msvc-g++ $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<
sqlupdate.exe: sqlupdate.wo
	/opt/xmingw/bin/i386-mingw32msvc-g++ -static $(CXXFLAGS) $(CPPFLAGS) -o $@ $^
	/opt/xmingw/bin/i386-mingw32msvc-strip $@
	- upx --best $@

clean:
	rm -f *.o sqlupdate
distclean: clean
	
realclean: distclean
	
include depfun.mak

install: install${DEPFUN_INSTALL}
uninstall: uninstall${DEPFUN_INSTALL}

.PHONY: install uninstall clean distclean realclean
