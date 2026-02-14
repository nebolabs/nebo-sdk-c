#ifndef NEBO_SCHEDULE_H
#define NEBO_SCHEDULE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Schedule handler — implement this to provide scheduling capabilities.
 *
 * All functions return 0 on success, non-zero on error.
 * Error-returning functions set *error to a heap-allocated string (caller frees).
 *
 * list: Sets *out_schedules (caller frees array), *out_count, *out_total.
 * history: Sets *out_entries (caller frees array), *out_count, *out_total.
 * trigger: Manual trigger. Sets *success and *output.
 * triggers: Block for server lifetime, call push() when schedules fire.
 *
 * Note: delete_schedule instead of delete (delete is a C++ keyword).
 */
typedef struct {
    int (*create)(const nebo_create_schedule_request_t *req,
                  nebo_schedule_t *out, char **error);
    int (*get)(const char *name, nebo_schedule_t *out, char **error);
    int (*list)(int limit, int offset, int enabled_only,
                nebo_schedule_t **out_schedules, int *out_count,
                long long *out_total, char **error);
    int (*update)(const nebo_update_schedule_request_t *req,
                  nebo_schedule_t *out, char **error);
    int (*delete_schedule)(const char *name, char **error);
    int (*enable)(const char *name, nebo_schedule_t *out, char **error);
    int (*disable)(const char *name, nebo_schedule_t *out, char **error);
    int (*trigger)(const char *name, int *success, char **output, char **error);
    int (*history)(const char *name, int limit, int offset,
                   nebo_schedule_history_entry_t **out_entries, int *out_count,
                   long long *out_total, char **error);
    int (*triggers)(nebo_push_schedule_trigger_fn push, void *stream_ctx);
} nebo_schedule_handler_t;

#ifdef __cplusplus
}
#endif

#endif /* NEBO_SCHEDULE_H */
