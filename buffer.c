#include "buffer.h"

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <ctype.h>
#include "unicode.h"

#define CLAMP(x, low, high) \
	__extension__ ({ \
		typeof(x) _x = (x); \
		typeof(low) _low = (low); \
		typeof(high) _high = (high); \
		((_x > _high) ? _high : ((_x < _low) ? _low : _x)); \
	})

#define TAB  8
#define ESC  033
#define CSI  '['

#define COLOR_RESET    "\033[0m"
#define COLOR_RED      "\033[31m"

enum esc_type {
    ESC_CSI,
    ESC_INVALID
};

struct esc_data_t {
    enum esc_state (*feed)(struct esc_t *, char);
    void (*apply)(buffer_t *);
};

static enum esc_state esc_feedCSI(struct esc_t *esc, char c);
static void esc_applyCSI(buffer_t *esc);

static const struct esc_data_t Table[ESC_INVALID] = {
    [ESC_CSI] = { esc_feedCSI, esc_applyCSI }
};


static struct line_t *alloc_line(unsigned len)
{
    struct line_t *line = malloc(sizeof(struct line_t));
    line->len = len;
    line->cell = calloc(len, sizeof(struct cell_t));
    line->next = line->prev = NULL;
    return line;
}

static void map_scrollbuffer(struct line_t *map[], struct line_t *buf, unsigned size)
{
    unsigned i;

    for (i = size; i > 0; --i) {
        map[i - 1] = buf->prev;
        buf = buf->prev;
    }
}

static void rotate_map(struct line_t *map[], unsigned size)
{
    unsigned i;

    for (i = 0; i < size; ++i)
        map[i] = map[i]->next;
}

static struct line_t *alloc_scrollbuffer(unsigned lines, unsigned len)
{
    struct line_t *head = NULL;

    while (lines--) {
        struct line_t *line = alloc_line(len);

        if (head == NULL)
            head = line;
        else {
            head->prev->next = line;
            line->prev = head->prev;
        }

        head->prev = line;
    }

    head->prev->next = head;
    return head;
}

buffer_t *buffer_new(unsigned rows, unsigned cols)
{
    buffer_t *buf = malloc(sizeof(buffer_t));

    buf->rows = rows;
    buf->cols = cols;
    buf->x = buf->y = 0;

    memset(&buf->utf,  0, sizeof(buf->utf));
    memset(&buf->esc,  0, sizeof(buf->esc));
    memset(&buf->attr, 0, sizeof(buf->attr));

    buf->mapped = calloc(rows, sizeof(struct line_t *));
    buf->lines = alloc_scrollbuffer(rows + 100, cols);
    map_scrollbuffer(buf->mapped, buf->lines, rows);

    return buf;
}

void buffer_newline(buffer_t *buf)
{
    buf->x = 0;

    if (buf->y + 1 == buf->rows)
        rotate_map(buf->mapped, buf->rows);
    else
        ++buf->y;
}

static enum esc_state esc_feed(buffer_t *b, char c)
{
    const struct esc_data_t *op = NULL;
    struct esc_t *esc = &b->esc;

    switch (b->esc.state) {
    case ESC_START:
        switch (c) {
        case CSI:
            printf("CSI\n");
            op = &Table[ESC_CSI];
            break;
        default:
            break;
        }

        if (op) {
            memset(esc, 0, sizeof(struct esc_t));
            esc->op = op;
            esc->state = ESC_EXPECT;
        } else
            esc->state = ESC_WAITING;

        break;
    case ESC_EXPECT:
        esc->state = esc->op->feed(esc, c);
    default:
        break;
    }

    switch (b->esc.state) {
        case ESC_ACCEPT:
            esc->op->apply(b);
        case ESC_REJECT:
            esc->state = ESC_WAITING;
        default:
            break;
    }

    printf("esc state: %d\n", esc->state);
    return esc->state;
}

void buffer_move(buffer_t *buf, unsigned x, unsigned y)
{
    CLAMP(x, 0u, buf->cols - 1);
	CLAMP(y, 0u, buf->rows - 1);
	buf->x = x;
	buf->y = y;
}

void buffer_tab(buffer_t *buf)
{
    unsigned spaces = TAB - buf->x % TAB;
    buffer_move(buf, buf->x + spaces, buf->y);
}

void buffer_write(buffer_t *buf, const char *msg, size_t len)
{
    unsigned i;
    enum utf8_state state;

    for (i = 0; i < len; i++) {
        const char c = msg[i];

        /* TODO: shitty hacky order */
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

        state = utf8_feed(&buf->utf, msg[i]);

        if (state != UTF8_ACCEPT)
            continue;

        switch (buf->utf.cp) {
        case '\n':
            buffer_newline(buf);
            break;
        case '\r':
            buf->x = 0;
            break;
        case '\t':
            buffer_tab(buf);
            break;
        default:
            buf->mapped[buf->y]->cell[buf->x].cp   = buf->utf.cp;
            buf->mapped[buf->y]->cell[buf->x].attr = buf->attr;
            if (++buf->x > buf->cols - 1)
                buffer_newline(buf);
        }
    }
}

static enum esc_state esc_feedCSI(struct esc_t *esc, char c)
{
    printf("CSI ESCAPE PARSING\n");
    if (isdigit(c)) {
        esc->args[esc->narg] *= 10;
        esc->args[esc->narg] += c - '0';
        return ESC_EXPECT;
    } else if (c == ';') {
        ++esc->narg;
        return ESC_EXPECT;
    } else if (0x40 <= c && c <= 0x7E) {
        esc->mode = c;
        return ESC_ACCEPT;
    }
    printf("CSI FAILED\n");
    return ESC_REJECT;
}

/* apply the escape sequence's effect */
/* static void esc_applyCSI(buffer_t *e, const buf_t *bufimpl, void *buf) */
static void esc_applyCSI(buffer_t *b)
{
    unsigned temp;
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
        /* HACK TASTIC */
        temp = b->esc.args[b->esc.narg];
        printf(COLOR_RED "COLOR: %d (@ %d)\n" COLOR_RESET, temp, b->esc.narg);
        temp -= 30;
        b->attr.fg = temp;
        if (b->esc.narg)
            b->attr.bold = b->esc.args[b->esc.narg - 1];
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
