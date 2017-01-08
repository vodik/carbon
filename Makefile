CFLAGS := -std=gnu11 -g \
	-Wall -Wextra -pedantic \
	-Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes \
	-Wno-missing-field-initializers \
	$(shell pkg-config --cflags cairo) \
	$(CFLAGS)

LDLIBS := $(shell pkg-config --libs cairo)

carbon: buffer.o carbon.o term.o tty.o unicode.o

clean:
	$(RM) carbon *.o

.PHONY: clean
