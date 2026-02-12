#ifndef NEBO_CHANNEL_H
#define NEBO_CHANNEL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Channel handler — implement this to bridge an external messaging platform.
 *
 * receive_callback: called by the SDK when Nebo opens the Receive stream.
 *                   Your app should call nebo_channel_push_message() when
 *                   messages arrive from the external platform.
 */
typedef struct {
    const char *id;
    int (*connect)(const nebo_string_map_t *config);
    int (*disconnect)(void);
    int (*send)(const char *channel_id, const char *text);
    void (*receive_callback)(const nebo_inbound_message_t *msg, void *userdata);
} nebo_channel_handler_t;

#ifdef __cplusplus
}
#endif

#endif /* NEBO_CHANNEL_H */
