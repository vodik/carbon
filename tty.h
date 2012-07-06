#ifndef TTY_H
#define TTY_H

#include <stddef.h>

typedef void (*shellfn)(void);
typedef struct tty tty_t;

tty_t *tty_new(shellfn shell);

void tty_poll(tty_t *tty, int epfd, int op);
int tty_read(tty_t *t, void *buf, size_t nbytes);

#endif
