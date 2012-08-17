#ifndef BUFFER_H
#define BUFFER_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct buffer buffer_t;

struct font_attr {
    size_t ref;

    uint8_t fg[3];
    uint8_t bg[3];
    bool bold      : 1;
    bool underline : 1;
    bool inverse   : 1;
};

struct cell {
    struct font_attr *attr;
    char ch;
};


struct line {
    char* ch;
    struct line *next, *prev;
};

struct buffer {
    unsigned rows, cols;
    unsigned x, y;
    struct line **lines;
};

/* struct line { */
/*     size_t size; */
/*     struct cell *cells; */

/*     struct line *next; */
/*     struct line *prev; */
/* }; */

/* struct buffer { */
/*     struct font_attr defaults; */
/*     struct font_attr *attr; */

/*     struct line **lines; */
/*     struct line *line; */

/*     unsigned rows, cols; */
/*     unsigned cursor_x, cursor_y; */
/* }; */

void buffer_write(buffer_t *buf, const char *msg, size_t len);

#endif
