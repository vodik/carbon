#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <cairo.h>

#include "tty.h"
#include "term.h"

const unsigned width  = 240;
const unsigned height = 240;

void shell(void)
{
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

    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    term_t *term = term_new(surface, "Monospace", 12);

    tty_poll(tty, epfd, EPOLL_CTL_ADD);

    struct epoll_event events[10];
    while (true) {
        int i, n = epoll_wait (epfd, events, 10, -1);

        for (i = 0; i < n; ++i) {
            char buf[BUFSIZ];
            int ret;

            ret = tty_read(events[i].data.ptr, buf, BUFSIZ);
            if (ret == -1) {
                perror("tty_read");
                exit(EXIT_FAILURE);
            }

            buf[ret] = '\0';
            term_print(term, buf);

            cairo_surface_write_to_png(surface, "terminal.png");
        }
    }
    return 0;
}
