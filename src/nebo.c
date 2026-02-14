/**
 * Nebo C SDK — app lifecycle and environment.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>

#include "internal.h"

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

void nebo_app_register_gateway(nebo_app_t *app, const nebo_gateway_handler_t *handler) {
    if (app) app->gateway = handler;
}

void nebo_app_register_ui(nebo_app_t *app, const nebo_ui_handler_t *handler) {
    if (app) app->ui = handler;
}

void nebo_app_register_comm(nebo_app_t *app, const nebo_comm_handler_t *handler) {
    if (app) app->comm = handler;
}

void nebo_app_register_schedule(nebo_app_t *app, const nebo_schedule_handler_t *handler) {
    if (app) app->schedule = handler;
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

int nebo_app_run(nebo_app_t *app) {
    if (!app) return 1;

    if (!app->sock_path || app->sock_path[0] == '\0') {
        fprintf(stderr, "NEBO_APP_SOCK not set\n");
        nebo_app_free(app);
        return 1;
    }

    if (!app->tool && !app->channel && !app->gateway &&
        !app->ui && !app->comm && !app->schedule) {
        fprintf(stderr, "No handlers registered\n");
        nebo_app_free(app);
        return 1;
    }

    fprintf(stderr, "[%s] listening on %s\n", app->name, app->sock_path);

    int ret = nebo_grpc_serve(app);

    nebo_app_free(app);
    return ret;
}
