#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/wait.h>
#include <cairo.h>

#include "tty.h"
#include "term.h"
#include "buffer.h"

const unsigned width  = 240;
const unsigned height = 240;

int epfd;
tty_t *tty;

cairo_surface_t *surface;
term_t *term;

static void sigchld();
static void shell(void);
static void run(void);

void sigchld()
{
    int stat = 0;
    if (waitpid(tty_pid(tty), &stat, 0) == -1) {
        perror("waitpid");
        exit(EXIT_FAILURE);
    }

    exit(WIFEXITED(stat) ? WEXITSTATUS(stat) : EXIT_FAILURE);
}

void shell(void)
{
    char *args[] = { getenv("SHELL"), "-i", NULL };
    /* putenv("TERM=carbon"); */
    putenv("TERM=dumb");
    execv(args[0], args);
}

void run(void)
{
    struct epoll_event events[10];

    while (true) {
        int i, n = epoll_wait(epfd, events, 10, -1);

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
}

void title(const char *t)
{
    (void)t;
}

static const tty_events_t events = {
    .title = title
};

int main(void)
{
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

    tty_events(tty, &events);

    surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    term = term_new(surface, "Monospace", 12);

    signal(SIGCHLD, sigchld);

    tty_poll_ctl(tty, epfd, EPOLL_CTL_ADD);
    run();

    return 0;
}

// vim: et:sts=4:sw=4:cino=(0
