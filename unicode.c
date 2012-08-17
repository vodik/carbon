#include "unicode.h"

enum utf8_state utf8_feed(struct utf8_t *u, char ci)
{
    uint32_t c = ci;

    switch (u->state) {
    case UTF8_START:
    case UTF8_ACCEPT:
    case UTF8_REJECT:
        if (c == 0xC0 || c == 0xC1) {
            /* overlong encoding for ASCII, reject */
            u->state = UTF8_REJECT;
        } else if ((c & 0x80) == 0) {
            /* single byte, accept */
            u->c = c;
            u->state = UTF8_ACCEPT;
        } else if ((c & 0xC0) == 0x80) {
            /* parser out of sync, ignore byte */
            u->state = UTF8_START;
        } else if ((c & 0xE0) == 0xC0) {
            /* start of two byte sequence */
            u->c = (c & 0x1F) << 6;
            u->state = UTF8_EXPECT1;
        } else if ((c & 0xF0) == 0xE0) {
            /* start of three byte sequence */
            u->c = (c & 0x0F) << 12;
            u->state = UTF8_EXPECT2;
        } else if ((c & 0xF8) == 0xF0) {
            /* start of four byte sequence */
            u->c = (c & 0x07) << 18;
            u->state = UTF8_EXPECT3;
        } else {
            /* overlong encoding, reject */
            u->state = UTF8_REJECT;
        }
        break;
    case UTF8_EXPECT3:
        u->c |= (c & 0x3F) << 12;
        if ((c & 0xC0) == 0x80)
            u->state = UTF8_EXPECT2;
        else
            u->state = UTF8_REJECT;
        break;
    case UTF8_EXPECT2:
        u->c |= (c & 0x3F) << 6;
        if ((c & 0xC0) == 0x80)
            u->state = UTF8_EXPECT1;
        else
            u->state = UTF8_REJECT;
        break;
    case UTF8_EXPECT1:
        u->c |= c & 0x3F;
        if ((c & 0xC0) == 0x80)
            u->state = UTF8_ACCEPT;
        else
            u->state = UTF8_REJECT;
        break;
    default:
        break;
    }

    return u->state;
}
