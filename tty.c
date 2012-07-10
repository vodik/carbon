#include "tty.h"

#define _XOPEN_SOURCE 200809L
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
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
        || (slavedev = ptsname(masterfd)) == NULL) {
        perror("failed to create pts\n");
        exit(EXIT_FAILURE);
    }

    slavefd = open(slavedev, O_RDWR | O_NOCTTY);
    if (slavefd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    tty_t *tty = malloc(sizeof(tty_t));
    if (!tty) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    switch (pid = fork()) {
    case -1:
        perror("fork");
        exit(EXIT_FAILURE);
        break;
    case 0:
        setsid();
        dup2(slavefd, STDIN_FILENO);
        dup2(slavefd, STDOUT_FILENO);
        dup2(slavefd, STDERR_FILENO);
        if (ioctl(slavefd, TIOCSCTTY, NULL) == -1) {
            perror("ioctl");
            exit(EXIT_FAILURE);
        }
        close(slavefd);
        close(masterfd);
        shell();

        /* shouldn't get here, shell should exit */
        exit(0);
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

    if (epoll_ctl(epfd, op, tty->fd, &tty->event) == -1) {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }
}

int tty_read(tty_t *t, void *buf, size_t nbytes)
{
    int ret;

    ret = read(t->fd, buf, nbytes);
    printf("read %d\n", ret);
    if (ret == -1) {
        perror("read");
        exit(EXIT_FAILURE);
    }

    return ret;
}

// vim: et:sts=4:sw=4:cino=(0
