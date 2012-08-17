OUT = carbon
SRC = ${wildcard *.c}
OBJ = ${SRC:.c=.o}

CFLAGS := -std=gnu99 \
	-Wall -Wextra -pedantic \
	${shell pkg-config --cflags cairo} \
	${CFLAGS}

LDFLAGS := ${shell pkg-config --libs cairo} ${LDFLAGS}

${OUT}: ${OBJ}

clean:
	${RM} ${OUT} ${OBJ}

.PHONY: clean install uninstall
