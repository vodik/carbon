#ifndef BUFFER_H
#define BUFFER_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "unicode.h"

/* TODO: set to something sane */
#define ESC_MAX 6

typedef struct buffer buffer_t;

enum esc_state {
    ESC_WAITING,
    ESC_START,
    ESC_ACCEPT,
    ESC_EXPECT,
    ESC_REJECT
};

struct font_attr {
    uint8_t fg;
    uint8_t bg;
    bool bold      : 1;
    bool underline : 1;
    bool inverse   : 1;
};

struct cell_t {
    struct font_attr attr;
    uint32_t cp;
};

struct line_t {
    unsigned len;
    struct cell_t *cell;
    struct line_t *next, *prev;
};

struct esc_t {
    enum esc_state state;
    const struct esc_data_t *op;

    char mode;
    int narg;
    int args[ESC_MAX];
};

struct buffer {
    unsigned rows, cols;
    unsigned x, y;
    struct line_t *lines;
    struct line_t **mapped;

    struct font_attr attr;

    struct esc_t esc;
    struct utf8_t utf;
};

buffer_t *buffer_new(unsigned rows, unsigned cols);
void buffer_write(buffer_t *buf, const char *msg, size_t len);

#endif
