#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/epoll.h>

#include "tty.h"

void shell(void)
{
    printf("starting shell %s...\n", getenv("SHELL"));
    char *args[] = { getenv("SHELL"), "-i", NULL };
    /* putenv("TERM=carbon"); */
    putenv("TERM=dumb");
    execv(args[0], args);
}

int main(void)
{
    int epfd;

    epfd = epoll_create1(0);
    if (epfd == -1) {
        perror("epoll_create");
        exit(EXIT_FAILURE);
    }

    tty_t *tty = tty_new(shell);
    if (!tty) {
        perror("tty_new");
        exit(EXIT_FAILURE);
    }

    tty_poll(tty, epfd, EPOLL_CTL_ADD);

    struct epoll_event events[10];
    while (true) {
        int i, n = epoll_wait (epfd, events, 10, -1);

        for (i = 0; i < n; ++i) {
            tty_read(events[i].data.ptr);
        }
    }
    return 0;
}
