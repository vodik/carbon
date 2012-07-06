#ifndef TTY_H
#define TTY_H

typedef void (*shellfn)(void);
typedef struct tty tty_t;

tty_t *tty_new(shellfn shell);
void tty_poll(tty_t *tty, int epfd, int op);
void tty_read(tty_t *t);

#endif
