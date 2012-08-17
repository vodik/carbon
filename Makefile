OUT = carbon
SRC = ${wildcard *.c}
OBJ = ${SRC:.c=.o}

CFLAGS := -std=c99 -Wall -Wextra -pedantic ${CFLAGS}

${OUT}: ${OBJ}

clean:
	${RM} ${OUT} ${OBJ}

.PHONY: clean install uninstall
