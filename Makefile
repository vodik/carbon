OUT = carbon
SRC = ${wildcard *.c}
OBJ = ${SRC:.c=.o}

CFLAGS:=-std=gnu99 \
	-Wall -Wextra -pedantic \
	${CFLAGS}

${OUT}: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	${RM} ${OUT} ${OBJ}

.PHONY: clean install uninstall
