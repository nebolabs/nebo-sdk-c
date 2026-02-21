#ifndef NEBO_CHANNEL_H
#define NEBO_CHANNEL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Channel handler — implement this to bridge an external messaging platform.
 *
 * send: Takes a full envelope and writes the platform-assigned message ID
 *       to *out_message_id. Caller frees out_message_id.
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
    int (*send)(const nebo_channel_send_envelope_t *env, char **out_message_id);
    int (*receive)(nebo_push_inbound_message_fn push, void *stream_ctx);
} nebo_channel_handler_t;

#ifdef __cplusplus
}
#endif

#endif /* NEBO_CHANNEL_H */
