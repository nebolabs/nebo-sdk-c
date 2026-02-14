#ifndef NEBO_INTERNAL_H
#define NEBO_INTERNAL_H

#include "nebo/nebo.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Full definition of nebo_app — shared between nebo.c and grpc_server.cc.
 */
struct nebo_app {
    char *dir;
    char *sock_path;
    char *id;
    char *name;
    char *version;
    char *data_dir;

    const nebo_tool_handler_t *tool;
    const nebo_channel_handler_t *channel;
    const nebo_gateway_handler_t *gateway;
    const nebo_ui_handler_t *ui;
    const nebo_comm_handler_t *comm;
    const nebo_schedule_handler_t *schedule;
    void (*on_configure)(const nebo_string_map_t *settings);
};

/**
 * Start the gRPC server. Implemented in grpc_server.cc.
 * Blocks until SIGTERM/SIGINT. Returns 0 on clean shutdown.
 */
int nebo_grpc_serve(nebo_app_t *app);

#ifdef __cplusplus
}
#endif

#endif /* NEBO_INTERNAL_H */
