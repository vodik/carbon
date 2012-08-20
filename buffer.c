#include "buffer.h"

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "unicode.h"

#define LIMIT(x, max) \
	__extension__ ({ \
		typeof(x) _x = (x); \
		typeof(max) _max = (max); \
		((_x > _max) ? _max : _x); \
	})

#define ESC  033
#define CSI  '['

enum esc_type {
    ESC_CSI,
    ESC_INVALID
};

struct esc_data_t {
    enum esc_state (*feed)(buffer_t *, char);
    void (*apply)(buffer_t *);
};

static enum esc_state esc_feedCSI(buffer_t *e, char c);
static void esc_applyCSI(buffer_t *e);

static const struct esc_data_t Table[ESC_INVALID] = {
    [ESC_CSI] = { esc_feedCSI, esc_applyCSI }
};


buffer_t *buffer_new(unsigned rows, unsigned cols)
{
    unsigned i;
    buffer_t *buf = malloc(sizeof(buffer_t));

    buf->rows = rows;
    buf->cols = cols;
    buf->x = buf->y = 0;
    buf->u.state = UTF8_START;

    buf->esc.state = ESC_WAITING;
    buf->esc.op = NULL;
    buf->esc.mode = 0;
    buf->esc.narg = 0;

    buf->lines = malloc(sizeof(struct line *) * rows);

    for (i = 0; i < rows; ++i) {
        struct line *line = malloc(sizeof(struct line));

        line->up = NULL; /* TODO */
        line->down = NULL; /* TODO */
        line->len = cols;
        line->g = calloc(cols, sizeof(uint32_t));

        buf->lines[i] = line;
    }

    return buf;
}

void buffer_newline(buffer_t *buf)
{
    /* TODO: scroll */
    buf->x = 0;
    buf->y = LIMIT(buf->y + 1, buf->rows - 1);
}

static enum esc_state esc_feed(buffer_t *b, char c)
{
    switch (b->esc.state) {
    case ESC_START:
        switch (c) {
        case CSI:
            printf("CSI\n");
            b->esc.op = &Table[ESC_CSI];
            break;
        default:
            break;
        }
        b->esc.narg = 0;
        b->esc.state = b->esc.op ? ESC_EXPECT : ESC_WAITING;
        break;
    case ESC_EXPECT:
        b->esc.state = b->esc.op->feed(b, c);
    default:
        break;
    }

    switch (b->esc.state) {
        case ESC_ACCEPT:
            b->esc.op->apply(b);
        case ESC_REJECT:
            b->esc.state = ESC_WAITING;
            break;
        default:
            break;
    }

    printf("esc state: %d\n", b->esc.state);
    return b->esc.state;
}


void buffer_write(buffer_t *buf, const char *msg, size_t len)
{
    unsigned i;
    enum utf8_state state;

    for (i = 0; i < len; i++) {
        const char c = msg[i];

        if (c == ESC && buf->esc.state == ESC_WAITING) {
            printf("WE FOUND SOMETHING ESCAPED!\n");
            buf->esc.state = ESC_START;
            continue;
        }

        if (buf->esc.state) {
            printf("WE ARE FEEDING %c\n", c);
            esc_feed(buf, c);
            continue;
        }

        state = utf8_feed(&buf->u, msg[i]);

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
            buf->lines[buf->y]->g[buf->x] = buf->u.c;
            if (++buf->x > buf->cols - 1)
                buffer_newline(buf);
        }
    }
}

static enum esc_state esc_feedCSI(buffer_t *b, char c)
{
    printf("CSI ESCAPE PARSING\n");
    if (isdigit(c)) {
        b->esc.args[b->esc.narg] *= 10;
        b->esc.args[b->esc.narg] += c - '0';
        return ESC_EXPECT;
    } else if (c == ';') {
        ++b->esc.narg;
        return ESC_EXPECT;
    } else if (0x40 <= c && c <= 0x7E) {
        b->esc.mode = c;
        return ESC_ACCEPT;
    }
    printf("CSI FAILED\n");
    return ESC_REJECT;
}

/* apply the escape sequence's effect */
/* static void esc_applyCSI(buffer_t *e, const buf_t *bufimpl, void *buf) */
static void esc_applyCSI(buffer_t *b)
{
    printf("TRYING TO ACCEPT THIS\n");
    switch(b->esc.mode) {
    /* insert [0] blank characters */
    case '@':
        /* if (bufimpl->insertblank) */
        /*     bufimpl->insertblank(buf, e->args[0] ? e->args[0] : 1); */
        break;
    /* move cursor up [0] */
    case 'A':
    case 'e':
        break;
    /* move cursor down [0] */
    case 'B':
        break;
    /* move cursor right [0] */
    case 'C':
    case 'a':
        break;
    /* move cursor left [0] */
    case 'D':
        break;
    /* move cursor down [0] and to start of line */
    case 'E':
        break;
    /* move cursor up [0] and to start of line */
    case 'F':
        break;
    /* more to column [0] */
    case 'G':
    case '`':
        break;
    /* move to row [0] column [1] */
    case 'H':
    case 'f':
        break;
    /* clear screen */
    case 'J':
        break;
    /* clear line */
    case 'K':
        break;
    /* insert [0] blank lines */
    case 'L':
        break;
    /* ??? */
    case 'l':
        break;
    /* delete [0] lines */
    case 'M':
        break;
    /* delete [0] charactes */
    case 'P':
        break;
    /* move to row [0] */
    case 'd':
        break;
    /* set terminal mode */
    case 'h':
        break;
    /* set terminal attributes [] */
    case 'm':
        break;
    /* ??? */
    case 'r':
        break;
    /* save cursor position */
    case 's':
        break;
    /* load cursor position */
    case 'u':
        break;
    default:
        break;
    }
}

// vim: et:sts=4:sw=4:cino=(0
