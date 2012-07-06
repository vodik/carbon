#ifndef TERM_H
#define TERM_H

#include <cairo.h>

typedef struct term term_t;

term_t *term_new(cairo_surface_t *surface, const char *font, unsigned size);
void term_destroy(term_t *term);

void term_print(term_t *term, const char *msg);

#endif
