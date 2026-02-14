#ifndef NEBO_COMM_H
#define NEBO_COMM_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Comm handler — implement this to provide inter-agent communication.
 *
 * Error-returning functions: return 0 on success, non-zero on error.
 * On error, set *error to a heap-allocated string (caller frees).
 *
 * receive: The bridge calls this with a push function. Store push and call
 *          it from any thread when messages arrive. This function should block
 *          for the lifetime of the receive stream. Return 0 on clean shutdown.
 *
 * Note: reg/dereg instead of register/deregister (register is a C keyword).
 */
typedef struct {
    const char *(*name)(void);
    const char *(*version)(void);
    int (*connect)(const nebo_string_map_t *config, char **error);
    int (*disconnect)(char **error);
    int (*is_connected)(void);
    int (*send)(const nebo_comm_message_t *msg, char **error);
    int (*subscribe)(const char *topic, char **error);
    int (*unsubscribe)(const char *topic, char **error);
    int (*reg)(const char *agent_id, const char **capabilities, int cap_count, char **error);
    int (*dereg)(char **error);
    int (*receive)(nebo_push_comm_message_fn push, void *stream_ctx);
} nebo_comm_handler_t;

#ifdef __cplusplus
}
#endif

#endif /* NEBO_COMM_H */
