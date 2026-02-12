#ifndef NEBO_UI_H
#define NEBO_UI_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * UI view and block types for structured UI apps.
 * These map to the 8 block types: text, heading, input, button, select,
 * toggle, divider, image.
 */

typedef struct {
    const char *label;
    const char *value;
} nebo_select_option_t;

typedef struct {
    const char *block_id;
    const char *type;        /* text, heading, input, button, select, toggle, divider, image */
    const char *text;
    const char *value;
    const char *placeholder;
    const char *hint;
    const char *variant;     /* primary/secondary/ghost/error for buttons; h1/h2/h3 for headings */
    const char *src;         /* image source URL */
    const char *alt;         /* image alt text */
    int disabled;
    const nebo_select_option_t *options; /* NULL-terminated for select blocks */
    int options_count;
    const char *style;       /* compact, full-width */
} nebo_ui_block_t;

typedef struct {
    const char *view_id;
    const char *title;
    nebo_ui_block_t *blocks;
    int block_count;
} nebo_ui_view_t;

typedef struct {
    const char *view_id;
    const char *block_id;
    const char *action;      /* "click", "change", "submit" */
    const char *value;
} nebo_ui_event_t;

typedef struct {
    nebo_ui_view_t *view;    /* NULL if no view update */
    const char *error;       /* NULL if no error */
    const char *toast;       /* NULL if no toast */
} nebo_ui_event_result_t;

/**
 * UI handler — implement this to render structured UI panels.
 */
typedef struct {
    int (*get_view)(const char *context, nebo_ui_view_t *out_view);
    int (*on_event)(const nebo_ui_event_t *event, nebo_ui_event_result_t *out_result);
} nebo_ui_handler_t;

#ifdef __cplusplus
}
#endif

#endif /* NEBO_UI_H */
