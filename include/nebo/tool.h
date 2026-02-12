#ifndef NEBO_TOOL_H
#define NEBO_TOOL_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Tool handler — implement this to provide a tool capability.
 *
 * execute: receives JSON input, writes result to *output (caller frees).
 *          Set *is_error to 1 if the result is an error message.
 *          Return 0 on success, non-zero on fatal error.
 *
 * requires_approval: optional, NULL means false.
 *          Return 1 if this tool needs user confirmation, 0 otherwise.
 */
typedef struct {
    const char *name;
    const char *description;
    const char *schema; /* JSON Schema string */
    int (*execute)(const char *input_json, char **output, int *is_error);
    int (*requires_approval)(void); /* optional, NULL = false */
} nebo_tool_handler_t;

#ifdef __cplusplus
}
#endif

#endif /* NEBO_TOOL_H */
