#ifndef NEBO_TYPES_H
#define NEBO_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * String map for config/settings key-value pairs.
 */
typedef struct {
    const char **keys;
    const char **values;
    int count;
} nebo_string_map_t;

/**
 * Identifies who sent a message.
 */
typedef struct {
    const char *name;
    const char *role;
    const char *bot_id;
} nebo_message_sender_t;

/**
 * A file or media attachment.
 */
typedef struct {
    const char *type;
    const char *url;
    const char *filename;
    long long size;
} nebo_attachment_t;

/**
 * An interactive element (button, keyboard row).
 */
typedef struct {
    const char *label;
    const char *callback_id;
} nebo_message_action_t;

/**
 * Inbound message from a channel.
 */
typedef struct {
    const char *channel_id;
    const char *user_id;
    const char *text;
    const char *metadata; /* JSON string (legacy) */
    /* v1 envelope fields */
    const char *message_id;
    const nebo_message_sender_t *sender;
    const nebo_attachment_t *attachments;
    int attachment_count;
    const char *reply_to;
    const nebo_message_action_t *actions;
    int action_count;
    const void *platform_data;
    int platform_data_len;
    const char *timestamp;
} nebo_inbound_message_t;

/**
 * Outbound channel message envelope (for Send).
 */
typedef struct {
    const char *channel_id;
    const char *text;
    const char *message_id;
    const nebo_message_sender_t *sender;
    const nebo_attachment_t *attachments;
    int attachment_count;
    const char *reply_to;
    const nebo_message_action_t *actions;
    int action_count;
    const void *platform_data;
    int platform_data_len;
} nebo_channel_send_envelope_t;

/**
 * Gateway message in a conversation.
 */
typedef struct {
    const char *role;       /* "user", "assistant", "tool" */
    const char *content;
    const char *tool_call_id;
    const char *tool_calls; /* JSON-encoded array */
} nebo_gateway_message_t;

/**
 * Tool definition for gateway requests.
 */
typedef struct {
    const char *name;
    const char *description;
    const char *input_schema; /* JSON Schema bytes */
    int input_schema_len;
} nebo_gateway_tool_def_t;

/**
 * User context passed to gateway requests.
 */
typedef struct {
    const char *user_id;
    const char *plan;
    const char *token; /* JWT if app has "user:token" permission */
} nebo_user_context_t;

/**
 * Gateway request for LLM chat completion.
 */
typedef struct {
    const char *request_id;
    const nebo_gateway_message_t *messages;
    int message_count;
    const nebo_gateway_tool_def_t *tools;
    int tool_count;
    int max_tokens;
    double temperature;
    const char *system;
    const nebo_user_context_t *user; /* NULL if no user context */
} nebo_gateway_request_t;

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
    const char *type;    /* "message", "mention", "proposal", "command", "info", "task" */
    const char *content;
    long long timestamp;
    int human_injected;
    const char *human_id;
} nebo_comm_message_t;

/**
 * Schedule definition.
 */
typedef struct {
    const char *id;
    const char *name;
    const char *expression;    /* cron expression */
    const char *task_type;     /* "bash" or "agent" */
    const char *command;
    const char *message;
    const char *deliver;       /* JSON */
    int enabled;
    const char *last_run;      /* RFC3339 */
    const char *next_run;      /* RFC3339 */
    long long run_count;
    const char *last_error;
    const char *created_at;    /* RFC3339 */
    const nebo_string_map_t *metadata;
} nebo_schedule_t;

/**
 * Schedule trigger event (fired when a schedule executes).
 */
typedef struct {
    const char *schedule_id;
    const char *name;
    const char *task_type;
    const char *command;
    const char *message;
    const char *deliver;
    const char *fired_at; /* RFC3339 */
    const nebo_string_map_t *metadata;
} nebo_schedule_trigger_t;

/**
 * Schedule execution history entry.
 */
typedef struct {
    const char *id;
    const char *schedule_name;
    const char *started_at;  /* RFC3339 */
    const char *finished_at; /* RFC3339 */
    int success;
    const char *output;
    const char *error;
} nebo_schedule_history_entry_t;

/**
 * Request to create a schedule.
 */
typedef struct {
    const char *name;
    const char *expression;
    const char *task_type;
    const char *command;
    const char *message;
    const char *deliver;
    const nebo_string_map_t *metadata;
} nebo_create_schedule_request_t;

/**
 * Request to update a schedule.
 */
typedef struct {
    const char *name;
    const char *expression;
    const char *task_type;
    const char *command;
    const char *message;
    const char *deliver;
    const nebo_string_map_t *metadata;
} nebo_update_schedule_request_t;

/**
 * Push callback typedefs for server-streaming RPCs.
 * The gRPC bridge passes a push function and opaque context to the handler.
 * The handler calls push() to send data to the stream.
 * Returns 0 on success, non-zero if the stream is closed/cancelled.
 */
typedef int (*nebo_push_gateway_event_fn)(const nebo_gateway_event_t *event, void *stream_ctx);
typedef int (*nebo_push_inbound_message_fn)(const nebo_inbound_message_t *msg, void *stream_ctx);
typedef int (*nebo_push_comm_message_fn)(const nebo_comm_message_t *msg, void *stream_ctx);
typedef int (*nebo_push_schedule_trigger_fn)(const nebo_schedule_trigger_t *trigger, void *stream_ctx);

#ifdef __cplusplus
}
#endif

#endif /* NEBO_TYPES_H */
