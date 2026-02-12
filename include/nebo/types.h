#ifndef NEBO_TYPES_H
#define NEBO_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * String map for config/settings key-value pairs.
 * Backed by a simple array of key/value pairs.
 */
typedef struct {
    const char **keys;
    const char **values;
    int count;
} nebo_string_map_t;

/**
 * Inbound message from a channel.
 */
typedef struct {
    const char *channel_id;
    const char *user_id;
    const char *text;
    const char *metadata; /* JSON string */
} nebo_inbound_message_t;

/**
 * Gateway event streamed back to Nebo.
 */
typedef struct {
    const char *type;      /* "text", "tool_call", "thinking", "error", "done" */
    const char *content;
    const char *model;
    const char *request_id;
} nebo_gateway_event_t;

/**
 * Inter-agent communication message.
 */
typedef struct {
    const char *id;
    const char *from;
    const char *to;
    const char *topic;
    const char *conversation_id;
    const char *type;
    const char *content;
    long long timestamp;
    int human_injected;
    const char *human_id;
} nebo_comm_message_t;

#ifdef __cplusplus
}
#endif

#endif /* NEBO_TYPES_H */
