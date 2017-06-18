CC ?= gcc
CFLAGS = -O0 -std=gnu99 -flto -fuse-linker-plugin
LIBS = glib-2.0
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
