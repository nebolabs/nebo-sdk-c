#ifndef NEBO_GATEWAY_H
#define NEBO_GATEWAY_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Gateway handler — implement this to provide LLM model routing.
 *
 * stream: Called when Nebo starts a chat completion. Receives the request
 *         and a push function. Call push() for each GatewayEvent (text token,
 *         tool_call, thinking, error, done). Return 0 when done, non-zero on
 *         fatal error. push() returns non-zero if the stream was cancelled.
 *
 * cancel: Called to abort an in-progress stream. Return 0 on success.
 */
typedef struct {
    int (*stream)(const nebo_gateway_request_t *req,
                  nebo_push_gateway_event_fn push,
                  void *stream_ctx);
    int (*cancel)(const char *request_id);
} nebo_gateway_handler_t;

#ifdef __cplusplus
}
#endif

#endif /* NEBO_GATEWAY_H */
