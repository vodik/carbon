#ifndef UNICODE_H
#define UNICODE_H

#include <stdint.h>

enum utf8_state {
    UTF8_START,
    UTF8_EXPECT3,
    UTF8_EXPECT2,
    UTF8_EXPECT1,
    UTF8_ACCEPT,
    UTF8_REJECT
};

struct utf8_t {
    enum utf8_state state;
    uint32_t c;
};

enum utf8_state utf8_feed(struct utf8_t *u, char ci);

#endif
