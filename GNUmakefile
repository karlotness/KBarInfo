CC := gcc
CFLAGS += -O3 -std=c23 -flto -Wall -Wextra -Wpedantic\
	-Wformat=2 -fstack-protector-strong -fstack-clash-protection\
	-fcf-protection=full -fPIE -D_FORTIFY_SOURCE=3
LIBS = glib-2.0 gobject-2.0 gio-unix-2.0 libpulse-mainloop-glib libnm json-glib-1.0
COMP_CFLAGS = $(CFLAGS) $(shell pkgconf --cflags $(LIBS))
LDFLAGS += -pie -Wl,--as-needed,--sort-common,-O1,-z,now,-z,relro,-z,noexecstack
LIBFLAGS = $(shell pkgconf --libs $(LIBS))
HEADS := $(wildcard src/*.h)
SRC := $(wildcard src/*.c)

kbarinfo: $(SRC:.c=.o)
	$(CC) $(LDFLAGS) $^ -o $@ $(LIBFLAGS)

%.o: %.c $(HEADS)
	$(CC) -c $(COMP_CFLAGS) $< -o $@

clean:
	rm -f kbarinfo
	rm -f *~ */*~ */*.o *.o

.PHONY: clean
