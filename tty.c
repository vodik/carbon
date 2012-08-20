#include "tty.h"

#define _XOPEN_SOURCE 200809L
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <err.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>

struct tty {
    pid_t pid;
    int fd;
    struct epoll_event event;
    const tty_events_t *events;
};

tty_t *tty_new(shellfn shell)
{
    int masterfd, slavefd;
    char *slavedev;
    pid_t pid;

    if ((masterfd = posix_openpt(O_RDWR | O_NOCTTY)) == -1
        || grantpt(masterfd) == -1
        || unlockpt(masterfd) == -1
        || (slavedev = ptsname(masterfd)) == NULL)
        err(EXIT_FAILURE, "failed to create pts\n");

    slavefd = open(slavedev, O_RDWR | O_NOCTTY);
    if (slavefd == -1)
        err(EXIT_FAILURE, "open");

    tty_t *tty = malloc(sizeof(tty_t));
    if (!tty)
        err(EXIT_FAILURE, "malloc");

    switch (pid = fork()) {
    case -1:
        err(EXIT_FAILURE, "fork");
        break;
    case 0:
        setsid();
        dup2(slavefd, STDIN_FILENO);
        dup2(slavefd, STDOUT_FILENO);
        dup2(slavefd, STDERR_FILENO);
        if (ioctl(slavefd, TIOCSCTTY, NULL) == -1)
            err(EXIT_FAILURE, "ioctl");

        close(slavefd);
        close(masterfd);
        shell();
        errx(EXIT_FAILURE, "shellfn didn't exit");
        break;
    }

    close(slavefd);
    tty->pid = pid;
    tty->fd  = masterfd;
    return tty;
}

void tty_events(tty_t *tty, const tty_events_t *events)
{
    tty->events = events;
}

int tty_pid(tty_t *tty)
{
    return tty->pid;
}

void tty_resize(tty_t *tty, int rows, int cols)
{
    struct winsize w = {
        .ws_row = rows,
        .ws_col = cols
    };

    if (ioctl(tty->fd, TIOCSWINSZ, &w) == -1) {
        fprintf(stderr, "couldn't set terminal size: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void tty_poll_ctl(tty_t *tty, int epfd, int op)
{
    tty->event.events = EPOLLIN | EPOLLET;
    tty->event.data.ptr = tty;

    if (epoll_ctl(epfd, op, tty->fd, &tty->event) == -1)
        err(EXIT_FAILURE, "epoll_ctl");
}

int tty_read(tty_t *t, void *buf, size_t nbytes)
{
    int ret;

    ret = read(t->fd, buf, nbytes);
    if (ret == -1)
        err(EXIT_FAILURE, "read");

    return ret;
}

int tty_write(tty_t *t, void *buf, size_t nbytes)
{
    int ret;

    ret = write(t->fd, buf, nbytes);
    if (ret == -1)
        err(EXIT_FAILURE, "write");

    return ret;
}

// vim: et:sts=4:sw=4:cino=(0
