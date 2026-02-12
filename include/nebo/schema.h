#ifndef NEBO_SCHEMA_H
#define NEBO_SCHEMA_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Schema builder for STRAP pattern tool inputs.
 * All functions return the builder for chaining.
 * The returned JSON string from nebo_schema_build() must be freed by the caller.
 */

typedef struct nebo_schema_builder nebo_schema_builder_t;

/** Create a new schema builder with the given actions (NULL-terminated array). */
nebo_schema_builder_t *nebo_schema_new(const char **actions);

/** Add a string parameter. */
nebo_schema_builder_t *nebo_schema_string(nebo_schema_builder_t *b, const char *name, const char *desc, int required);

/** Add a number parameter. */
nebo_schema_builder_t *nebo_schema_number(nebo_schema_builder_t *b, const char *name, const char *desc, int required);

/** Add a boolean parameter. */
nebo_schema_builder_t *nebo_schema_bool(nebo_schema_builder_t *b, const char *name, const char *desc, int required);

/** Build the JSON Schema string. Caller must free() the returned string. */
char *nebo_schema_build(nebo_schema_builder_t *b);

/** Free the schema builder. */
void nebo_schema_free(nebo_schema_builder_t *b);

#ifdef __cplusplus
}
#endif

#endif /* NEBO_SCHEMA_H */
