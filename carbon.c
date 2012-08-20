#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <err.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>
#include <sys/wait.h>
#include <cairo.h>

#include "tty.h"
#include "term.h"
#include "buffer.h"

int epfd;

tty_t *tty;
buffer_t *buff;

static void sigchld();
static void shell(void);
static void run(void);

#define COLOR_RESET    "\033[0m"
#define COLOR_BOLD     "\033[1m"
#define COLOR_RED      "\033[31m"
#define COLOR_GREEN    "\033[32m"
#define COLOR_YELLOW   "\033[33m"
#define COLOR_BLUE     "\033[34m"

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
    putenv("TERM=dumb");

    char *args[] = { "/bin/bash", "-i", NULL };
    execv(args[0], args);
}

void dump_buffer(buffer_t *buf)
{
    unsigned i, j;
    printf("\033[H\033[2J");
    for (i = 0; i < buf->rows; ++i) {
        struct line *line = buf->lines[i];

        printf("|");
        for (j = 0; j < line->len; ++j) {
            uint32_t cp = line->g[j];

            if (cp == 033)
                printf(COLOR_BOLD COLOR_RED "!" COLOR_RESET);
            else if (cp == 0)
                printf(COLOR_BLUE "." COLOR_RESET);
            else if (cp > 0x1f && cp < 0x7f)
                printf("%c", (char)cp);
            else
                printf(COLOR_YELLOW "*" COLOR_RESET);
        }
        printf("|");
    }
    printf("> ");
}

void run(void)
{
    struct epoll_event events[10];

    while (true) {
        int i, n = epoll_wait(epfd, events, 10, -1);

        for (i = 0; i < n; ++i) {
            char buf[BUFSIZ];
            int ret;

            if (events[i].data.ptr == 0) {
                ret = read(0, buf, BUFSIZ);
                if (ret == -1) {
                    perror("read");
                    exit(EXIT_FAILURE);
                }

                buf[ret++] = '\r';
                buf[ret] = '\0';
                tty_write(tty, buf, ret);
            } else {
                ret = tty_read(events[i].data.ptr, buf, BUFSIZ);
                printf("read: %d\n", ret);
                if (ret == -1) {
                    perror("tty_read");
                    exit(EXIT_FAILURE);
                }

                buf[ret] = '\0';
                buffer_write(buff, buf, ret);

                dump_buffer(buff);
            }
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
    setbuf(stdout, NULL);

    struct winsize w;
    int fd = open("/dev/tty", O_RDONLY);
    if (ioctl(fd, TIOCGWINSZ, &w) == -1) {
        fprintf(stderr, "couldn't get terminal size: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    signal(SIGCHLD, sigchld);

    epfd = epoll_create1(0);
    if (epfd == -1) {
        perror("epoll_create");
        exit(EXIT_FAILURE);
    }

    tty = tty_new(shell);
    if (!tty) {
        perror("tty_new");
        exit(EXIT_FAILURE);
    }

    tty_events(tty, &events);

    buff = buffer_new(w.ws_row - 1, w.ws_col - 2);
    if (!buff) {
        perror("buff_new");
        exit(EXIT_FAILURE);
    }

    tty_poll_ctl(tty, epfd, EPOLL_CTL_ADD);

    struct epoll_event event = {
        .events = EPOLLIN | EPOLLET,
        .data = { .ptr = 0 }
    };

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, 0, &event) == -1)
        err(EXIT_FAILURE, "epoll_ctl");

    printf("running buffer %dx%d\n", buff->rows, buff->cols);
    run();

    return 0;
}

// vim: et:sts=4:sw=4:cino=(0
