#include "term.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cairo.h>
#include <assert.h>

struct term {
	cairo_t *cr;
	cairo_font_extents_t fe;
	int x, y;
	int w, h;
	int px, py;
};

term_t *
term_new(cairo_surface_t *surface, const char *font, unsigned size)
{
	assert(surface);

	/* create our terminal */
	int width  = cairo_image_surface_get_width(surface);
	int height = cairo_image_surface_get_height(surface);
	cairo_t *cr = cairo_create(surface);

	/* set a nice background */
	cairo_rectangle(cr, 0.0, 0.0, width, height);
	cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);
	cairo_fill(cr);

	cairo_select_font_face(cr, font, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(cr, size);
	cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);

	term_t *term = malloc(sizeof(term_t));
	term->cr = cr;
	cairo_font_extents(cr, &term->fe);

	term->x = 0;
	term->y = 0;
	term->w = width / term->fe.max_x_advance;
	term->h = height / term->fe.max_y_advance;
	term->px = 0;
	term->py = term->fe.height;

	return term;
}

void
term_destroy(term_t *term)
{
	cairo_destroy(term->cr);
	free(term);
}

void
term_print(term_t *term, const char *msg)
{
	int len = strlen(msg), i;
	char str[2] = { 0 };

	for (i = 0; i < len; i++) {
		if (msg[i] == '\n' || ++term->x > term->w - 1) {
			if (msg[i] == '\n')
				++i;
			term->x = 0;
			term->px = 0;
			++term->y;
			term->py += term->fe.height;
		}
		str[0] = msg[i];
		cairo_move_to(term->cr, term->px, term->py);
		cairo_show_text(term->cr, str);
		term->px += term->fe.max_x_advance;
	}
}
