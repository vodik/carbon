#include "buffer.h"

#include <stdlib.h>
#include <stdio.h>

buffer_t *buffer_new(unsigned rows, unsigned cols)
{
    buffer_t *buf = malloc(sizeof(buffer_t));

    buf->rows = rows;
    buf->cols = cols;
    buf->x = buf->y = 0;
    buf->lines = malloc(sizeof(struct line *) * rows);

    unsigned i;
    for (i = 0; i < rows; ++i) {
        buf->lines[i] = malloc(sizeof(struct line));
        struct line *line = buf->lines[i];
        line->ch = malloc(cols);
    }

    return buf;
}

void buffer_newline(buffer_t *buf)
{
    buf->x = 0;
    ++buf->y;
    if (buf->y > buf->rows - 1) {
        /* TODO scroll */
        buf->y = buf->rows - 1;
    }
}

void buffer_write(buffer_t *buf, const char *msg, size_t len)
{
    unsigned i;

    for (i = 0; i < len; i++) {
        switch (msg[i]) {
        case '\n':
            buffer_newline(buf);
            break;
        case '\r':
            buf->x = 0;
            break;
        case '\t':
            /* TODO */
            break;
        default:
            buf->lines[buf->y]->ch[buf->x] = msg[i];
            if (++buf->x > buf->cols - 1) {
                buffer_newline(buf);
            }
        }
    }
}

void dump_buffer(buffer_t *buf)
{
    unsigned i = buf->rows;
    for (i = 0; i < buf->rows; ++i) {
        printf("|%-10s|\n", buf->lines[i]->ch);
    }
}

// vim: et:sts=4:sw=4:cino=(0
