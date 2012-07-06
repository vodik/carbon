#include "tty.h"

#define _XOPEN_SOURCE 200809L
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>

struct tty {
    pid_t pid;
    int fd;
    struct epoll_event event;
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
        shell();
        exit(-1); /* shouldn't get here */
        break;
    }

    close(slavefd);
    tty->pid = pid;
    tty->fd  = masterfd;
    return tty;
}

void tty_poll(tty_t *tty, int epfd, int op)
{
    tty->event.events = EPOLLIN | EPOLLET;
    tty->event.data.ptr = tty;

    if (epoll_ctl(epfd, op, tty->fd, &tty->event) == -1) {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }
}

void tty_read(tty_t *t)
{
    char buf[BUFSIZ];
    int ret;

    ret = read(t->fd, buf, BUFSIZ);
    printf("read %d\n", ret);
    if (ret == -1) {
        perror("read");
        exit(EXIT_FAILURE);
    }

    /* tty_put(t, buf, ret); */
    ret = write(STDOUT_FILENO, buf, ret);
    if (ret == -1) {
        perror("write");
        exit(EXIT_FAILURE);
    }
}
