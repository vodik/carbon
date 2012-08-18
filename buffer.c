#include "buffer.h"

#include <stdlib.h>
#include <stdio.h>
#include "unicode.h"

buffer_t *buffer_new(unsigned rows, unsigned cols)
{
    buffer_t *buf = malloc(sizeof(buffer_t));

    buf->rows = rows;
    buf->cols = cols;
    buf->x = buf->y = 0;
    buf->lines = malloc(sizeof(struct line *) * rows);
    buf->u.state = UTF8_START;

    unsigned i;
    for (i = 0; i < rows; ++i) {
        buf->lines[i] = malloc(sizeof(struct line));
        struct line *line = buf->lines[i];
        line->cp = malloc(sizeof(uint32_t) * cols);
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
    enum utf8_state state;

    for (i = 0; i < len; i++) {
        state = utf8_feed(&buf->u, msg[i]);

        printf("state: %d\n", state);
        if (state != UTF8_ACCEPT)
            continue;

        switch (buf->u.c) {
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
            buf->lines[buf->y]->cp[buf->x] = buf->u.c;
            if (++buf->x > buf->cols - 1) {
                buffer_newline(buf);
            }
        }
    }
}

void dump_buffer(buffer_t *buf)
{
    unsigned i, j;
    for (i = 0; i < 8; ++i) {
        printf("|");
        for (j = 0; j < buf->cols; ++j)
            printf("%c", (char)buf->lines[i]->cp[j]);
        printf("|\n");
    }
}

// vim: et:sts=4:sw=4:cino=(0
