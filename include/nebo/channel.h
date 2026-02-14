#ifndef NEBO_CHANNEL_H
#define NEBO_CHANNEL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Channel handler — implement this to bridge an external messaging platform.
 *
 * receive: The bridge calls this with a push function. Call push() from any
 *          thread when messages arrive from the external platform. This
 *          function should block for the lifetime of the receive stream.
 *          Return 0 on clean shutdown, non-zero on error.
 */
typedef struct {
    const char *id;
    int (*connect)(const nebo_string_map_t *config);
    int (*disconnect)(void);
    int (*send)(const char *channel_id, const char *text);
    int (*receive)(nebo_push_inbound_message_fn push, void *stream_ctx);
} nebo_channel_handler_t;

#ifdef __cplusplus
}
#endif

#endif /* NEBO_CHANNEL_H */
