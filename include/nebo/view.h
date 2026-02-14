#ifndef NEBO_VIEW_H
#define NEBO_VIEW_H

#include "ui.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * ViewBuilder constructs a nebo_ui_view_t with a fluent API.
 *
 * Usage:
 *   nebo_view_builder_t *vb = nebo_view_new("main", "Dashboard");
 *   nebo_view_heading(vb, "h1", "Welcome", "h1");
 *   nebo_view_text(vb, "desc", "Hello world");
 *   nebo_view_button(vb, "btn1", "Click Me", "primary");
 *   nebo_ui_view_t *view = nebo_view_build(vb);
 *   // ... use view ...
 *   nebo_view_free_view(view);
 */

typedef struct nebo_view_builder nebo_view_builder_t;

/** Create a new view builder. */
nebo_view_builder_t *nebo_view_new(const char *view_id, const char *title);

/** Add a heading block. variant: "h1", "h2", "h3". */
nebo_view_builder_t *nebo_view_heading(nebo_view_builder_t *vb, const char *block_id,
                                        const char *text, const char *variant);

/** Add a text block. */
nebo_view_builder_t *nebo_view_text(nebo_view_builder_t *vb, const char *block_id,
                                     const char *text);

/** Add a button block. variant: "primary", "secondary", "ghost", "error". */
nebo_view_builder_t *nebo_view_button(nebo_view_builder_t *vb, const char *block_id,
                                       const char *text, const char *variant);

/** Add an input block. */
nebo_view_builder_t *nebo_view_input(nebo_view_builder_t *vb, const char *block_id,
                                      const char *value, const char *placeholder);

/** Add a select block. */
nebo_view_builder_t *nebo_view_select(nebo_view_builder_t *vb, const char *block_id,
                                       const char *value,
                                       const nebo_select_option_t *options, int count);

/** Add a toggle block. on: 1 = true, 0 = false. */
nebo_view_builder_t *nebo_view_toggle(nebo_view_builder_t *vb, const char *block_id,
                                       const char *text, int on);

/** Add a divider block. */
nebo_view_builder_t *nebo_view_divider(nebo_view_builder_t *vb, const char *block_id);

/** Add an image block. */
nebo_view_builder_t *nebo_view_image(nebo_view_builder_t *vb, const char *block_id,
                                      const char *src, const char *alt);

/** Build the view. Returns a heap-allocated nebo_ui_view_t. Free with nebo_view_free_view(). */
nebo_ui_view_t *nebo_view_build(nebo_view_builder_t *vb);

/** Free the builder (does not free the built view). */
void nebo_view_free(nebo_view_builder_t *vb);

/** Free a view returned by nebo_view_build(). */
void nebo_view_free_view(nebo_ui_view_t *view);

#ifdef __cplusplus
}
#endif

#endif /* NEBO_VIEW_H */
