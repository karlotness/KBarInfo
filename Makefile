CC ?= gcc
CFLAGS = -Og -std=gnu99 -flto -fuse-linker-plugin -ggdb3
LIBS = glib-2.0 gio-2.0 libpulse-mainloop-glib
COMP_CFLAGS = $(CFLAGS) $(shell pkg-config --cflags $(LIBS))
LDFLAGS = $(CFLAGS) $(shell pkg-config --libs --cflags $(LIBS))
HEADS := $(wildcard */*.h *.h)
SRC := $(wildcard */*.c *.c)

include $(wildcard */sub.mk)

kbarinfo: $(SRC:.c=.o)
	$(CC) $(LDFLAGS) $^ -o $@

%.o: %.c $(HEADS)
	$(CC) -c $(COMP_CFLAGS) $< -o $@

clean: $(CLEANTARGS)
	rm -f kbarinfo
	rm -f *~ */*~ */*.o *.o

.PHONY: clean
