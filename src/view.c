/**
 * Nebo C SDK — ViewBuilder for fluent UI construction.
 */

#include <stdlib.h>
#include <string.h>

#include "nebo/view.h"

#define MAX_BLOCKS 64

struct nebo_view_builder {
    char *view_id;
    char *title;
    nebo_ui_block_t blocks[MAX_BLOCKS];
    int block_count;
};

static const char *safe_strdup(const char *s) {
    return s ? strdup(s) : NULL;
}

nebo_view_builder_t *nebo_view_new(const char *view_id, const char *title) {
    nebo_view_builder_t *vb = calloc(1, sizeof(nebo_view_builder_t));
    if (!vb) return NULL;
    vb->view_id = strdup(view_id);
    vb->title = strdup(title);
    return vb;
}

nebo_view_builder_t *nebo_view_heading(nebo_view_builder_t *vb, const char *block_id,
                                        const char *text, const char *variant) {
    if (!vb || vb->block_count >= MAX_BLOCKS) return vb;
    nebo_ui_block_t *blk = &vb->blocks[vb->block_count++];
    memset(blk, 0, sizeof(*blk));
    blk->block_id = strdup(block_id);
    blk->type = strdup("heading");
    blk->text = strdup(text);
    blk->variant = strdup(variant);
    return vb;
}

nebo_view_builder_t *nebo_view_text(nebo_view_builder_t *vb, const char *block_id,
                                     const char *text) {
    if (!vb || vb->block_count >= MAX_BLOCKS) return vb;
    nebo_ui_block_t *blk = &vb->blocks[vb->block_count++];
    memset(blk, 0, sizeof(*blk));
    blk->block_id = strdup(block_id);
    blk->type = strdup("text");
    blk->text = strdup(text);
    return vb;
}

nebo_view_builder_t *nebo_view_button(nebo_view_builder_t *vb, const char *block_id,
                                       const char *text, const char *variant) {
    if (!vb || vb->block_count >= MAX_BLOCKS) return vb;
    nebo_ui_block_t *blk = &vb->blocks[vb->block_count++];
    memset(blk, 0, sizeof(*blk));
    blk->block_id = strdup(block_id);
    blk->type = strdup("button");
    blk->text = strdup(text);
    blk->variant = strdup(variant);
    return vb;
}

nebo_view_builder_t *nebo_view_input(nebo_view_builder_t *vb, const char *block_id,
                                      const char *value, const char *placeholder) {
    if (!vb || vb->block_count >= MAX_BLOCKS) return vb;
    nebo_ui_block_t *blk = &vb->blocks[vb->block_count++];
    memset(blk, 0, sizeof(*blk));
    blk->block_id = strdup(block_id);
    blk->type = strdup("input");
    blk->value = (const char *)safe_strdup(value);
    blk->placeholder = (const char *)safe_strdup(placeholder);
    return vb;
}

nebo_view_builder_t *nebo_view_select(nebo_view_builder_t *vb, const char *block_id,
                                       const char *value,
                                       const nebo_select_option_t *options, int count) {
    if (!vb || vb->block_count >= MAX_BLOCKS) return vb;
    nebo_ui_block_t *blk = &vb->blocks[vb->block_count++];
    memset(blk, 0, sizeof(*blk));
    blk->block_id = strdup(block_id);
    blk->type = strdup("select");
    blk->value = (const char *)safe_strdup(value);
    if (options && count > 0) {
        nebo_select_option_t *opts = calloc(count, sizeof(nebo_select_option_t));
        for (int i = 0; i < count; i++) {
            opts[i].label = strdup(options[i].label);
            opts[i].value = strdup(options[i].value);
        }
        blk->options = opts;
        blk->options_count = count;
    }
    return vb;
}

nebo_view_builder_t *nebo_view_toggle(nebo_view_builder_t *vb, const char *block_id,
                                       const char *text, int on) {
    if (!vb || vb->block_count >= MAX_BLOCKS) return vb;
    nebo_ui_block_t *blk = &vb->blocks[vb->block_count++];
    memset(blk, 0, sizeof(*blk));
    blk->block_id = strdup(block_id);
    blk->type = strdup("toggle");
    blk->text = strdup(text);
    blk->value = strdup(on ? "true" : "false");
    return vb;
}

nebo_view_builder_t *nebo_view_divider(nebo_view_builder_t *vb, const char *block_id) {
    if (!vb || vb->block_count >= MAX_BLOCKS) return vb;
    nebo_ui_block_t *blk = &vb->blocks[vb->block_count++];
    memset(blk, 0, sizeof(*blk));
    blk->block_id = strdup(block_id);
    blk->type = strdup("divider");
    return vb;
}

nebo_view_builder_t *nebo_view_image(nebo_view_builder_t *vb, const char *block_id,
                                      const char *src, const char *alt) {
    if (!vb || vb->block_count >= MAX_BLOCKS) return vb;
    nebo_ui_block_t *blk = &vb->blocks[vb->block_count++];
    memset(blk, 0, sizeof(*blk));
    blk->block_id = strdup(block_id);
    blk->type = strdup("image");
    blk->src = strdup(src);
    blk->alt = (const char *)safe_strdup(alt);
    return vb;
}

nebo_ui_view_t *nebo_view_build(nebo_view_builder_t *vb) {
    if (!vb) return NULL;
    nebo_ui_view_t *view = calloc(1, sizeof(nebo_ui_view_t));
    if (!view) return NULL;

    view->view_id = strdup(vb->view_id);
    view->title = strdup(vb->title);
    view->block_count = vb->block_count;

    if (vb->block_count > 0) {
        view->blocks = calloc(vb->block_count, sizeof(nebo_ui_block_t));
        memcpy(view->blocks, vb->blocks, vb->block_count * sizeof(nebo_ui_block_t));
        /* Ownership of strdup'd strings transfers to the view */
        memset(vb->blocks, 0, sizeof(vb->blocks));
        vb->block_count = 0;
    }

    return view;
}

static void free_block(nebo_ui_block_t *blk) {
    if (!blk) return;
    free((void *)blk->block_id);
    free((void *)blk->type);
    free((void *)blk->text);
    free((void *)blk->value);
    free((void *)blk->placeholder);
    free((void *)blk->hint);
    free((void *)blk->variant);
    free((void *)blk->src);
    free((void *)blk->alt);
    free((void *)blk->style);
    if (blk->options) {
        for (int i = 0; i < blk->options_count; i++) {
            free((void *)blk->options[i].label);
            free((void *)blk->options[i].value);
        }
        free((void *)blk->options);
    }
}

void nebo_view_free(nebo_view_builder_t *vb) {
    if (!vb) return;
    free(vb->view_id);
    free(vb->title);
    for (int i = 0; i < vb->block_count; i++) {
        free_block(&vb->blocks[i]);
    }
    free(vb);
}

void nebo_view_free_view(nebo_ui_view_t *view) {
    if (!view) return;
    free((void *)view->view_id);
    free((void *)view->title);
    for (int i = 0; i < view->block_count; i++) {
        free_block(&view->blocks[i]);
    }
    free(view->blocks);
    free(view);
}
