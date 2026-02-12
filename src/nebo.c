/**
 * Nebo C SDK — app lifecycle and environment.
 *
 * NOTE: The gRPC server implementation requires linking against grpc and protobuf.
 * This file provides the app lifecycle, environment access, and schema builder.
 * The gRPC bridge layer (tool_bridge.c, etc.) is generated from proto files.
 *
 * For a complete working implementation, build with CMake which handles
 * proto generation and linking automatically.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>

#include "nebo/nebo.h"

struct nebo_app {
    char *dir;
    char *sock_path;
    char *id;
    char *name;
    char *version;
    char *data_dir;

    const nebo_tool_handler_t *tool;
    const nebo_channel_handler_t *channel;
    const nebo_ui_handler_t *ui;
    void (*on_configure)(const nebo_string_map_t *settings);
};

static char *safe_getenv(const char *name) {
    const char *val = getenv(name);
    return val ? strdup(val) : strdup("");
}

nebo_app_t *nebo_app_new(void) {
    nebo_app_t *app = calloc(1, sizeof(nebo_app_t));
    if (!app) return NULL;

    app->dir       = safe_getenv("NEBO_APP_DIR");
    app->sock_path = safe_getenv("NEBO_APP_SOCK");
    app->id        = safe_getenv("NEBO_APP_ID");
    app->name      = safe_getenv("NEBO_APP_NAME");
    app->version   = safe_getenv("NEBO_APP_VERSION");
    app->data_dir  = safe_getenv("NEBO_APP_DATA");

    return app;
}

void nebo_app_register_tool(nebo_app_t *app, const nebo_tool_handler_t *handler) {
    if (app) app->tool = handler;
}

void nebo_app_register_channel(nebo_app_t *app, const nebo_channel_handler_t *handler) {
    if (app) app->channel = handler;
}

void nebo_app_register_ui(nebo_app_t *app, const nebo_ui_handler_t *handler) {
    if (app) app->ui = handler;
}

void nebo_app_on_configure(nebo_app_t *app, void (*callback)(const nebo_string_map_t *settings)) {
    if (app) app->on_configure = callback;
}

const char *nebo_app_dir(const nebo_app_t *app) { return app ? app->dir : ""; }
const char *nebo_app_sock(const nebo_app_t *app) { return app ? app->sock_path : ""; }
const char *nebo_app_id(const nebo_app_t *app) { return app ? app->id : ""; }
const char *nebo_app_name(const nebo_app_t *app) { return app ? app->name : ""; }
const char *nebo_app_version(const nebo_app_t *app) { return app ? app->version : ""; }
const char *nebo_app_data_dir(const nebo_app_t *app) { return app ? app->data_dir : ""; }

void nebo_app_free(nebo_app_t *app) {
    if (!app) return;
    free(app->dir);
    free(app->sock_path);
    free(app->id);
    free(app->name);
    free(app->version);
    free(app->data_dir);
    free(app);
}

/*
 * nebo_app_run() — the gRPC server entry point.
 *
 * A full implementation requires the gRPC C++ core library and protobuf-generated
 * service stubs. The CMakeLists.txt handles proto generation and linking.
 *
 * This stub validates the environment and prints a diagnostic message.
 * See the CMakeLists.txt for the complete build that includes gRPC.
 */
int nebo_app_run(nebo_app_t *app) {
    if (!app) return 1;

    if (!app->sock_path || app->sock_path[0] == '\0') {
        fprintf(stderr, "NEBO_APP_SOCK not set\n");
        nebo_app_free(app);
        return 1;
    }

    if (!app->tool && !app->channel && !app->ui) {
        fprintf(stderr, "No handlers registered\n");
        nebo_app_free(app);
        return 1;
    }

    fprintf(stderr, "[%s] C SDK app ready (sock=%s)\n", app->name, app->sock_path);
    fprintf(stderr, "[%s] Note: link against grpc++ and protobuf for full gRPC server\n", app->name);

    /* In a full implementation, this would:
     * 1. Create a gRPC server
     * 2. Register service implementations (tool, channel, ui bridges)
     * 3. Listen on the Unix socket
     * 4. Block until SIGTERM
     */

    nebo_app_free(app);
    return 0;
}
