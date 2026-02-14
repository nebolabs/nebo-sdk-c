/**
 * Nebo C SDK — JSON Schema builder for STRAP pattern tool inputs.
 *
 * Builds JSON manually to avoid requiring a JSON library dependency.
 * The output is a valid JSON Schema string.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "nebo/schema.h"

#define MAX_PROPS 32
#define MAX_ACTIONS 16
#define MAX_ENUM_VALUES 32

typedef struct {
    char *name;
    char *desc;
    char *type;
    int required;
    char **enum_values; /* NULL if not an enum */
    int enum_count;
} schema_prop_t;

struct nebo_schema_builder {
    char *actions[MAX_ACTIONS];
    int action_count;
    schema_prop_t props[MAX_PROPS];
    int prop_count;
};

nebo_schema_builder_t *nebo_schema_new(const char **actions) {
    nebo_schema_builder_t *b = calloc(1, sizeof(nebo_schema_builder_t));
    if (!b) return NULL;

    for (int i = 0; actions && actions[i] && i < MAX_ACTIONS; i++) {
        b->actions[i] = strdup(actions[i]);
        b->action_count++;
    }
    return b;
}

static nebo_schema_builder_t *add_prop(nebo_schema_builder_t *b, const char *name,
                                        const char *desc, const char *type, int required) {
    if (!b || b->prop_count >= MAX_PROPS) return b;
    schema_prop_t *p = &b->props[b->prop_count++];
    p->name = strdup(name);
    p->desc = strdup(desc);
    p->type = strdup(type);
    p->required = required;
    p->enum_values = NULL;
    p->enum_count = 0;
    return b;
}

nebo_schema_builder_t *nebo_schema_string(nebo_schema_builder_t *b, const char *name,
                                           const char *desc, int required) {
    return add_prop(b, name, desc, "string", required);
}

nebo_schema_builder_t *nebo_schema_number(nebo_schema_builder_t *b, const char *name,
                                           const char *desc, int required) {
    return add_prop(b, name, desc, "number", required);
}

nebo_schema_builder_t *nebo_schema_bool(nebo_schema_builder_t *b, const char *name,
                                         const char *desc, int required) {
    return add_prop(b, name, desc, "boolean", required);
}

nebo_schema_builder_t *nebo_schema_enum(nebo_schema_builder_t *b, const char *name,
                                         const char *desc, int required,
                                         const char **values) {
    if (!b || b->prop_count >= MAX_PROPS) return b;
    schema_prop_t *p = &b->props[b->prop_count++];
    p->name = strdup(name);
    p->desc = strdup(desc);
    p->type = strdup("string");
    p->required = required;

    int count = 0;
    for (int i = 0; values && values[i] && i < MAX_ENUM_VALUES; i++) count++;
    p->enum_values = malloc((count + 1) * sizeof(char *));
    for (int i = 0; i < count; i++) p->enum_values[i] = strdup(values[i]);
    p->enum_values[count] = NULL;
    p->enum_count = count;
    return b;
}

nebo_schema_builder_t *nebo_schema_object(nebo_schema_builder_t *b, const char *name,
                                           const char *desc, int required) {
    return add_prop(b, name, desc, "object", required);
}

char *nebo_schema_build(nebo_schema_builder_t *b) {
    if (!b) return NULL;

    size_t bufsize = 8192;
    char *buf = malloc(bufsize);
    if (!buf) return NULL;

    int pos = 0;
    pos += snprintf(buf + pos, bufsize - pos, "{\"type\":\"object\",\"properties\":{");

    /* Action field */
    pos += snprintf(buf + pos, bufsize - pos, "\"action\":{\"type\":\"string\",\"enum\":[");
    for (int i = 0; i < b->action_count; i++) {
        if (i > 0) pos += snprintf(buf + pos, bufsize - pos, ",");
        pos += snprintf(buf + pos, bufsize - pos, "\"%s\"", b->actions[i]);
    }
    pos += snprintf(buf + pos, bufsize - pos, "],\"description\":\"Action to perform: ");
    for (int i = 0; i < b->action_count; i++) {
        if (i > 0) pos += snprintf(buf + pos, bufsize - pos, ", ");
        pos += snprintf(buf + pos, bufsize - pos, "%s", b->actions[i]);
    }
    pos += snprintf(buf + pos, bufsize - pos, "\"}");

    /* Additional properties */
    for (int i = 0; i < b->prop_count; i++) {
        schema_prop_t *p = &b->props[i];
        pos += snprintf(buf + pos, bufsize - pos,
            ",\"%s\":{\"type\":\"%s\",\"description\":\"%s\"",
            p->name, p->type, p->desc);
        if (p->enum_values) {
            pos += snprintf(buf + pos, bufsize - pos, ",\"enum\":[");
            for (int j = 0; j < p->enum_count; j++) {
                if (j > 0) pos += snprintf(buf + pos, bufsize - pos, ",");
                pos += snprintf(buf + pos, bufsize - pos, "\"%s\"", p->enum_values[j]);
            }
            pos += snprintf(buf + pos, bufsize - pos, "]");
        }
        pos += snprintf(buf + pos, bufsize - pos, "}");
    }

    pos += snprintf(buf + pos, bufsize - pos, "},\"required\":[\"action\"");
    for (int i = 0; i < b->prop_count; i++) {
        if (b->props[i].required) {
            pos += snprintf(buf + pos, bufsize - pos, ",\"%s\"", b->props[i].name);
        }
    }
    pos += snprintf(buf + pos, bufsize - pos, "]}");

    return buf;
}

void nebo_schema_free(nebo_schema_builder_t *b) {
    if (!b) return;
    for (int i = 0; i < b->action_count; i++) free(b->actions[i]);
    for (int i = 0; i < b->prop_count; i++) {
        free(b->props[i].name);
        free(b->props[i].desc);
        free(b->props[i].type);
        if (b->props[i].enum_values) {
            for (int j = 0; j < b->props[i].enum_count; j++)
                free(b->props[i].enum_values[j]);
            free(b->props[i].enum_values);
        }
    }
    free(b);
}
