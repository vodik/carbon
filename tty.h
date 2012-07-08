#ifndef TTY_H
#define TTY_H

#include <stddef.h>

typedef void (*shellfn)(void);
typedef struct tty tty_t;

typedef struct {
    void (*title)(const char *);
} tty_events_t;

tty_t *tty_new(shellfn shell);
void tty_events(tty_t *tty, const tty_events_t *events);

int tty_pid(tty_t *tty);
void tty_resize(int x, int y);

void tty_poll_ctl(tty_t *tty, int epfd, int op);
int tty_read(tty_t *t, void *buf, size_t nbytes);

#endif
