CC := gcc
CFLAGS = -O3 -std=gnu99 -flto -fuse-linker-plugin -Wall -Wextra\
	-Werror -Wformat=2 -fstack-protector-all -fstack-check\
	-fcf-protection=full -fPIE -D_FORTIFY_SOURCE=2
LIBS = glib-2.0 gio-2.0 libpulse-mainloop-glib
COMP_CFLAGS = $(CFLAGS) $(shell pkg-config --cflags $(LIBS))
LDFLAGS = -pie -z now -z relro $(CFLAGS)\
	$(shell pkg-config --libs --cflags $(LIBS))
HEADS := $(wildcard */*.h *.h)
SRC := $(wildcard */*.c *.c)

kbarinfo: $(SRC:.c=.o)
	$(CC) $(LDFLAGS) $^ -o $@

%.o: %.c $(HEADS)
	$(CC) -c $(COMP_CFLAGS) $< -o $@

clean:
	rm -f kbarinfo
	rm -f *~ */*~ */*.o *.o

.PHONY: clean
