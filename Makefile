PREFIX = /usr/local
GTK = gtk+-3.0
VTE = vte-2.90
TERMINFO = ${PREFIX}/share/terminfo

CFLAGS := -std=c99 -O3 \
	  -Wall -Wextra -pedantic \
	  -Winit-self \
	  -Wshadow \
	  -Wformat=2 \
	  -Wmissing-declarations \
	  -Wstrict-overflow=5 \
	  -Wcast-align \
	  -Wcast-qual \
	  -Wconversion \
	  -Wunused-macros \
	  -Wwrite-strings \
	  -DNDEBUG \
	  -D_POSIX_C_SOURCE=200809L \
	  ${shell pkg-config --cflags ${GTK} ${VTE}} \
	  ${CXXFLAGS}

LDFLAGS := -s -Wl,--as-needed ${LDFLAGS}
LDLIBS := ${shell pkg-config --libs ${GTK} ${VTE}}

zvte: zvte.c config.h
	${CC} ${CFLAGS} ${LDFLAGS} $< ${LDLIBS} -o $@

install: zvte zvte.desktop
	mkdir -p ${DESTDIR}${TERMINFO}
	install -Dm755 zvte ${DESTDIR}${PREFIX}/bin/zvte
	install -Dm644 zvte.desktop ${DESTDIR}${PREFIX}/share/applications/zvte.desktop

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/zvte

clean:
	rm zvte

.PHONY: clean install uninstall
