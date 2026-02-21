/**
 * Nebo C SDK — gRPC server (C++ shim).
 *
 * Implements six gRPC service bridges that call through to C handler
 * function pointers. The public SDK API stays 100% C; this file is the
 * only C++ in the library.
 */

#include <cstring>
#include <string>
#include <csignal>
#include <unistd.h>

#include <grpcpp/grpcpp.h>

#include "proto/apps/v0/common.grpc.pb.h"
#include "proto/apps/v0/tool.grpc.pb.h"
#include "proto/apps/v0/channel.grpc.pb.h"
#include "proto/apps/v0/gateway.grpc.pb.h"
#include "proto/apps/v0/ui.grpc.pb.h"
#include "proto/apps/v0/comm.grpc.pb.h"
#include "proto/apps/v0/schedule.grpc.pb.h"

extern "C" {
#include "internal.h"
}

namespace apb = apps::v0;

/* ── Helpers ─────────────────────────────────────────────────────────── */

static grpc::Status health_ok(apb::HealthCheckResponse *resp, const nebo_app_t *app) {
    resp->set_healthy(true);
    resp->set_name(app->name ? app->name : "");
    resp->set_version(app->version ? app->version : "");
    return grpc::Status::OK;
}

static void configure_handler(const apb::SettingsMap &sm, void (*cb)(const nebo_string_map_t *)) {
    if (!cb) return;
    int n = sm.values_size();
    if (n == 0) { cb(nullptr); return; }

    const char **keys = new const char *[n];
    const char **vals = new const char *[n];
    int i = 0;
    for (auto &kv : sm.values()) {
        keys[i] = kv.first.c_str();
        vals[i] = kv.second.c_str();
        i++;
    }
    nebo_string_map_t map = {keys, vals, n};
    cb(&map);
    delete[] keys;
    delete[] vals;
}

static nebo_string_map_t proto_map_to_c(const google::protobuf::Map<std::string, std::string> &m,
                                         const char ***out_keys, const char ***out_vals) {
    int n = (int)m.size();
    *out_keys = new const char *[n];
    *out_vals = new const char *[n];
    int i = 0;
    for (auto &kv : m) {
        (*out_keys)[i] = kv.first.c_str();
        (*out_vals)[i] = kv.second.c_str();
        i++;
    }
    return {*out_keys, *out_vals, n};
}

/* ── ToolBridge ──────────────────────────────────────────────────────── */

class ToolBridge final : public apb::ToolService::Service {
    const nebo_tool_handler_t *h_;
    const nebo_app_t *app_;
public:
    ToolBridge(const nebo_tool_handler_t *h, const nebo_app_t *app) : h_(h), app_(app) {}

    grpc::Status HealthCheck(grpc::ServerContext *, const apb::HealthCheckRequest *,
                             apb::HealthCheckResponse *resp) override {
        return health_ok(resp, app_);
    }

    grpc::Status Name(grpc::ServerContext *, const apb::Empty *,
                      apb::NameResponse *resp) override {
        if (h_->name) resp->set_name(h_->name);
        return grpc::Status::OK;
    }

    grpc::Status Description(grpc::ServerContext *, const apb::Empty *,
                             apb::DescriptionResponse *resp) override {
        if (h_->description) resp->set_description(h_->description);
        return grpc::Status::OK;
    }

    grpc::Status Schema(grpc::ServerContext *, const apb::Empty *,
                        apb::SchemaResponse *resp) override {
        if (h_->schema) resp->set_schema(h_->schema, strlen(h_->schema));
        return grpc::Status::OK;
    }

    grpc::Status Execute(grpc::ServerContext *, const apb::ExecuteRequest *req,
                         apb::ExecuteResponse *resp) override {
        if (!h_->execute) return grpc::Status(grpc::UNIMPLEMENTED, "no execute handler");

        std::string input(req->input().begin(), req->input().end());
        char *output = nullptr;
        int is_error = 0;
        int ret = h_->execute(input.c_str(), &output, &is_error);
        if (ret != 0) {
            return grpc::Status(grpc::INTERNAL, output ? output : "execute failed");
        }
        if (output) {
            resp->set_content(output);
            free(output);
        }
        resp->set_is_error(is_error);
        return grpc::Status::OK;
    }

    grpc::Status RequiresApproval(grpc::ServerContext *, const apb::Empty *,
                                  apb::ApprovalResponse *resp) override {
        resp->set_requires_approval(h_->requires_approval ? h_->requires_approval() : 0);
        return grpc::Status::OK;
    }

    grpc::Status Configure(grpc::ServerContext *, const apb::SettingsMap *req,
                           apb::Empty *) override {
        configure_handler(*req, app_->on_configure);
        return grpc::Status::OK;
    }
};

/* ── ChannelBridge ───────────────────────────────────────────────────── */

struct channel_stream_ctx {
    grpc::ServerWriter<apb::InboundMessage> *writer;
    grpc::ServerContext *ctx;
};

static int channel_push_trampoline(const nebo_inbound_message_t *msg, void *opaque) {
    auto *sc = static_cast<channel_stream_ctx *>(opaque);
    if (sc->ctx->IsCancelled()) return -1;
    apb::InboundMessage im;
    if (msg->channel_id) im.set_channel_id(msg->channel_id);
    if (msg->user_id)    im.set_user_id(msg->user_id);
    if (msg->text)       im.set_text(msg->text);
    if (msg->metadata)   im.set_metadata(msg->metadata);
    /* v1 envelope fields */
    if (msg->message_id) im.set_message_id(msg->message_id);
    if (msg->sender) {
        auto *s = im.mutable_sender();
        if (msg->sender->name)   s->set_name(msg->sender->name);
        if (msg->sender->role)   s->set_role(msg->sender->role);
        if (msg->sender->bot_id) s->set_bot_id(msg->sender->bot_id);
    }
    for (int i = 0; i < msg->attachment_count; i++) {
        auto *a = im.add_attachments();
        if (msg->attachments[i].type)     a->set_type(msg->attachments[i].type);
        if (msg->attachments[i].url)      a->set_url(msg->attachments[i].url);
        if (msg->attachments[i].filename) a->set_filename(msg->attachments[i].filename);
        a->set_size(msg->attachments[i].size);
    }
    if (msg->reply_to) im.set_reply_to(msg->reply_to);
    for (int i = 0; i < msg->action_count; i++) {
        auto *a = im.add_actions();
        if (msg->actions[i].label)       a->set_label(msg->actions[i].label);
        if (msg->actions[i].callback_id) a->set_callback_id(msg->actions[i].callback_id);
    }
    if (msg->platform_data && msg->platform_data_len > 0)
        im.set_platform_data(msg->platform_data, msg->platform_data_len);
    if (msg->timestamp) im.set_timestamp(msg->timestamp);
    return sc->writer->Write(im) ? 0 : -1;
}

class ChannelBridge final : public apb::ChannelService::Service {
    const nebo_channel_handler_t *h_;
    const nebo_app_t *app_;
public:
    ChannelBridge(const nebo_channel_handler_t *h, const nebo_app_t *app) : h_(h), app_(app) {}

    grpc::Status HealthCheck(grpc::ServerContext *, const apb::HealthCheckRequest *,
                             apb::HealthCheckResponse *resp) override {
        return health_ok(resp, app_);
    }

    grpc::Status ID(grpc::ServerContext *, const apb::Empty *,
                    apb::IDResponse *resp) override {
        if (h_->id) resp->set_id(h_->id);
        return grpc::Status::OK;
    }

    grpc::Status Connect(grpc::ServerContext *, const apb::ChannelConnectRequest *req,
                         apb::ChannelConnectResponse *resp) override {
        if (!h_->connect) return grpc::Status::OK;

        const char **keys = nullptr, **vals = nullptr;
        nebo_string_map_t map = proto_map_to_c(req->config(), &keys, &vals);
        int ret = h_->connect(&map);
        delete[] keys;
        delete[] vals;
        if (ret != 0) resp->set_error("connect failed");
        return grpc::Status::OK;
    }

    grpc::Status Disconnect(grpc::ServerContext *, const apb::Empty *,
                            apb::ChannelDisconnectResponse *resp) override {
        if (h_->disconnect) {
            int ret = h_->disconnect();
            if (ret != 0) resp->set_error("disconnect failed");
        }
        return grpc::Status::OK;
    }

    grpc::Status Send(grpc::ServerContext *, const apb::ChannelSendRequest *req,
                      apb::ChannelSendResponse *resp) override {
        if (!h_->send) return grpc::Status::OK;

        /* Map proto sender */
        nebo_message_sender_t sender{};
        if (req->has_sender()) {
            sender.name   = req->sender().name().c_str();
            sender.role   = req->sender().role().c_str();
            sender.bot_id = req->sender().bot_id().c_str();
        }

        /* Map proto attachments */
        int att_count = req->attachments_size();
        auto *atts = att_count > 0 ? new nebo_attachment_t[att_count]() : nullptr;
        for (int i = 0; i < att_count; i++) {
            auto &a = req->attachments(i);
            atts[i].type     = a.type().c_str();
            atts[i].url      = a.url().c_str();
            atts[i].filename = a.filename().c_str();
            atts[i].size     = a.size();
        }

        /* Map proto actions */
        int act_count = req->actions_size();
        auto *acts = act_count > 0 ? new nebo_message_action_t[act_count]() : nullptr;
        for (int i = 0; i < act_count; i++) {
            auto &a = req->actions(i);
            acts[i].label       = a.label().c_str();
            acts[i].callback_id = a.callback_id().c_str();
        }

        nebo_channel_send_envelope_t env{};
        env.channel_id        = req->channel_id().c_str();
        env.text              = req->text().c_str();
        env.message_id        = req->message_id().c_str();
        env.sender            = req->has_sender() ? &sender : nullptr;
        env.attachments       = atts;
        env.attachment_count  = att_count;
        env.reply_to          = req->reply_to().c_str();
        env.actions           = acts;
        env.action_count      = act_count;
        env.platform_data     = req->platform_data().data();
        env.platform_data_len = (int)req->platform_data().size();

        char *out_message_id = nullptr;
        int ret = h_->send(&env, &out_message_id);

        delete[] atts;
        delete[] acts;

        if (ret != 0) {
            resp->set_error("send failed");
        } else if (out_message_id) {
            resp->set_message_id(out_message_id);
            free(out_message_id);
        }
        return grpc::Status::OK;
    }

    grpc::Status Receive(grpc::ServerContext *ctx, const apb::Empty *,
                         grpc::ServerWriter<apb::InboundMessage> *writer) override {
        if (!h_->receive) return grpc::Status(grpc::UNIMPLEMENTED, "no receive handler");
        channel_stream_ctx sc{writer, ctx};
        int ret = h_->receive(channel_push_trampoline, &sc);
        return ret == 0 ? grpc::Status::OK : grpc::Status(grpc::INTERNAL, "receive error");
    }

    grpc::Status Configure(grpc::ServerContext *, const apb::SettingsMap *req,
                           apb::Empty *) override {
        configure_handler(*req, app_->on_configure);
        return grpc::Status::OK;
    }
};

/* ── GatewayBridge ───────────────────────────────────────────────────── */

struct gateway_stream_ctx {
    grpc::ServerWriter<apb::GatewayEvent> *writer;
    grpc::ServerContext *ctx;
};

static int gateway_push_trampoline(const nebo_gateway_event_t *evt, void *opaque) {
    auto *sc = static_cast<gateway_stream_ctx *>(opaque);
    if (sc->ctx->IsCancelled()) return -1;
    apb::GatewayEvent ge;
    if (evt->type)       ge.set_type(evt->type);
    if (evt->content)    ge.set_content(evt->content);
    if (evt->model)      ge.set_model(evt->model);
    if (evt->request_id) ge.set_request_id(evt->request_id);
    return sc->writer->Write(ge) ? 0 : -1;
}

class GatewayBridge final : public apb::GatewayService::Service {
    const nebo_gateway_handler_t *h_;
    const nebo_app_t *app_;
public:
    GatewayBridge(const nebo_gateway_handler_t *h, const nebo_app_t *app) : h_(h), app_(app) {}

    grpc::Status HealthCheck(grpc::ServerContext *, const apb::HealthCheckRequest *,
                             apb::HealthCheckResponse *resp) override {
        return health_ok(resp, app_);
    }

    grpc::Status Stream(grpc::ServerContext *ctx, const apb::GatewayRequest *req,
                        grpc::ServerWriter<apb::GatewayEvent> *writer) override {
        if (!h_->stream) return grpc::Status(grpc::UNIMPLEMENTED, "no stream handler");

        /* Convert proto messages to C structs */
        int msg_count = req->messages_size();
        auto *msgs = new nebo_gateway_message_t[msg_count]();
        for (int i = 0; i < msg_count; i++) {
            auto &m = req->messages(i);
            msgs[i].role = m.role().c_str();
            msgs[i].content = m.content().c_str();
            msgs[i].tool_call_id = m.tool_call_id().c_str();
            msgs[i].tool_calls = m.tool_calls().c_str();
        }

        int tool_count = req->tools_size();
        auto *tools = new nebo_gateway_tool_def_t[tool_count]();
        for (int i = 0; i < tool_count; i++) {
            auto &t = req->tools(i);
            tools[i].name = t.name().c_str();
            tools[i].description = t.description().c_str();
            tools[i].input_schema = t.input_schema().c_str();
            tools[i].input_schema_len = (int)t.input_schema().size();
        }

        nebo_user_context_t user_ctx{};
        const nebo_user_context_t *user_ptr = nullptr;
        if (req->has_user()) {
            user_ctx.token = req->user().token().c_str();
            user_ctx.user_id = req->user().user_id().c_str();
            user_ctx.plan = req->user().plan().c_str();
            user_ptr = &user_ctx;
        }

        nebo_gateway_request_t creq{};
        creq.request_id = req->request_id().c_str();
        creq.messages = msgs;
        creq.message_count = msg_count;
        creq.tools = tools;
        creq.tool_count = tool_count;
        creq.max_tokens = req->max_tokens();
        creq.temperature = req->temperature();
        creq.system = req->system().c_str();
        creq.user = user_ptr;

        gateway_stream_ctx sc{writer, ctx};
        int ret = h_->stream(&creq, gateway_push_trampoline, &sc);

        delete[] msgs;
        delete[] tools;
        return ret == 0 ? grpc::Status::OK : grpc::Status(grpc::INTERNAL, "stream error");
    }

    grpc::Status Poll(grpc::ServerContext *, const apb::PollRequest *,
                      apb::PollResponse *) override {
        return grpc::Status(grpc::UNIMPLEMENTED, "poll not supported in C SDK");
    }

    grpc::Status Cancel(grpc::ServerContext *, const apb::CancelRequest *req,
                        apb::CancelResponse *resp) override {
        if (h_->cancel) {
            int ret = h_->cancel(req->request_id().c_str());
            resp->set_cancelled(ret == 0);
        }
        return grpc::Status::OK;
    }

    grpc::Status Configure(grpc::ServerContext *, const apb::SettingsMap *req,
                           apb::Empty *) override {
        configure_handler(*req, app_->on_configure);
        return grpc::Status::OK;
    }
};

/* ── UIBridge ────────────────────────────────────────────────────────── */

class UIBridge final : public apb::UIService::Service {
    const nebo_ui_handler_t *h_;
    const nebo_app_t *app_;
public:
    UIBridge(const nebo_ui_handler_t *h, const nebo_app_t *app) : h_(h), app_(app) {}

    grpc::Status HealthCheck(grpc::ServerContext *, const apb::HealthCheckRequest *,
                             apb::HealthCheckResponse *resp) override {
        return health_ok(resp, app_);
    }

    grpc::Status HandleRequest(grpc::ServerContext *, const apb::HttpRequest *req,
                               apb::HttpResponse *resp) override {
        if (!h_->handle_request) return grpc::Status(grpc::UNIMPLEMENTED, "no handle_request handler");

        /* Map proto headers to C string map */
        const char **hdr_keys = nullptr, **hdr_vals = nullptr;
        nebo_string_map_t headers{};
        if (req->headers_size() > 0) headers = proto_map_to_c(req->headers(), &hdr_keys, &hdr_vals);

        nebo_http_request_t creq{};
        creq.method   = req->method().c_str();
        creq.path     = req->path().c_str();
        creq.query    = req->query().c_str();
        creq.headers  = req->headers_size() > 0 ? &headers : nullptr;
        creq.body     = req->body().data();
        creq.body_len = (int)req->body().size();

        nebo_http_response_t cresp{};
        int ret = h_->handle_request(&creq, &cresp);

        delete[] hdr_keys;
        delete[] hdr_vals;

        if (ret != 0) return grpc::Status(grpc::INTERNAL, "handle_request failed");

        resp->set_status_code(cresp.status_code);
        if (cresp.body && cresp.body_len > 0)
            resp->set_body(cresp.body, cresp.body_len);
        if (cresp.headers) {
            auto *m = resp->mutable_headers();
            for (int i = 0; i < cresp.headers->count; i++) {
                (*m)[cresp.headers->keys[i]] = cresp.headers->values[i];
            }
        }
        return grpc::Status::OK;
    }

    grpc::Status Configure(grpc::ServerContext *, const apb::SettingsMap *req,
                           apb::Empty *) override {
        configure_handler(*req, app_->on_configure);
        return grpc::Status::OK;
    }
};

/* ── CommBridge ──────────────────────────────────────────────────────── */

struct comm_stream_ctx {
    grpc::ServerWriter<apb::CommMessage> *writer;
    grpc::ServerContext *ctx;
};

static int comm_push_trampoline(const nebo_comm_message_t *msg, void *opaque) {
    auto *sc = static_cast<comm_stream_ctx *>(opaque);
    if (sc->ctx->IsCancelled()) return -1;
    apb::CommMessage cm;
    if (msg->id)              cm.set_id(msg->id);
    if (msg->from)            cm.set_from(msg->from);
    if (msg->to)              cm.set_to(msg->to);
    if (msg->topic)           cm.set_topic(msg->topic);
    if (msg->conversation_id) cm.set_conversation_id(msg->conversation_id);
    if (msg->type)            cm.set_type(msg->type);
    if (msg->content)         cm.set_content(msg->content);
    cm.set_timestamp(msg->timestamp);
    cm.set_human_injected(msg->human_injected);
    if (msg->human_id)        cm.set_human_id(msg->human_id);
    return sc->writer->Write(cm) ? 0 : -1;
}

class CommBridge final : public apb::CommService::Service {
    const nebo_comm_handler_t *h_;
    const nebo_app_t *app_;
public:
    CommBridge(const nebo_comm_handler_t *h, const nebo_app_t *app) : h_(h), app_(app) {}

    grpc::Status HealthCheck(grpc::ServerContext *, const apb::HealthCheckRequest *,
                             apb::HealthCheckResponse *resp) override {
        return health_ok(resp, app_);
    }

    grpc::Status Name(grpc::ServerContext *, const apb::Empty *,
                      apb::CommNameResponse *resp) override {
        if (h_->name) resp->set_name(h_->name());
        return grpc::Status::OK;
    }

    grpc::Status Version(grpc::ServerContext *, const apb::Empty *,
                         apb::CommVersionResponse *resp) override {
        if (h_->version) resp->set_version(h_->version());
        return grpc::Status::OK;
    }

    grpc::Status Connect(grpc::ServerContext *, const apb::CommConnectRequest *req,
                         apb::CommConnectResponse *resp) override {
        if (!h_->connect) return grpc::Status::OK;
        const char **keys = nullptr, **vals = nullptr;
        nebo_string_map_t map = proto_map_to_c(req->config(), &keys, &vals);
        char *err = nullptr;
        int ret = h_->connect(&map, &err);
        delete[] keys;
        delete[] vals;
        if (ret != 0 && err) { resp->set_error(err); free(err); }
        return grpc::Status::OK;
    }

    grpc::Status Disconnect(grpc::ServerContext *, const apb::Empty *,
                            apb::CommDisconnectResponse *resp) override {
        if (!h_->disconnect) return grpc::Status::OK;
        char *err = nullptr;
        int ret = h_->disconnect(&err);
        if (ret != 0 && err) { resp->set_error(err); free(err); }
        return grpc::Status::OK;
    }

    grpc::Status IsConnected(grpc::ServerContext *, const apb::Empty *,
                             apb::CommIsConnectedResponse *resp) override {
        if (h_->is_connected) resp->set_connected(h_->is_connected());
        return grpc::Status::OK;
    }

    grpc::Status Send(grpc::ServerContext *, const apb::CommSendRequest *req,
                      apb::CommSendResponse *resp) override {
        if (!h_->send) return grpc::Status::OK;
        auto &pm = req->message();

        nebo_comm_message_t msg{};
        msg.id = pm.id().c_str();
        msg.from = pm.from().c_str();
        msg.to = pm.to().c_str();
        msg.topic = pm.topic().c_str();
        msg.conversation_id = pm.conversation_id().c_str();
        msg.type = pm.type().c_str();
        msg.content = pm.content().c_str();
        msg.timestamp = pm.timestamp();
        msg.human_injected = pm.human_injected();
        msg.human_id = pm.human_id().c_str();

        char *err = nullptr;
        int ret = h_->send(&msg, &err);
        if (ret != 0 && err) { resp->set_error(err); free(err); }
        return grpc::Status::OK;
    }

    grpc::Status Subscribe(grpc::ServerContext *, const apb::CommSubscribeRequest *req,
                           apb::CommSubscribeResponse *resp) override {
        if (!h_->subscribe) return grpc::Status::OK;
        char *err = nullptr;
        int ret = h_->subscribe(req->topic().c_str(), &err);
        if (ret != 0 && err) { resp->set_error(err); free(err); }
        return grpc::Status::OK;
    }

    grpc::Status Unsubscribe(grpc::ServerContext *, const apb::CommUnsubscribeRequest *req,
                             apb::CommUnsubscribeResponse *resp) override {
        if (!h_->unsubscribe) return grpc::Status::OK;
        char *err = nullptr;
        int ret = h_->unsubscribe(req->topic().c_str(), &err);
        if (ret != 0 && err) { resp->set_error(err); free(err); }
        return grpc::Status::OK;
    }

    grpc::Status Register(grpc::ServerContext *, const apb::CommRegisterRequest *req,
                          apb::CommRegisterResponse *resp) override {
        if (!h_->reg) return grpc::Status::OK;
        int cap_count = req->capabilities_size();
        auto **caps = new const char *[cap_count];
        for (int i = 0; i < cap_count; i++) caps[i] = req->capabilities(i).c_str();
        char *err = nullptr;
        int ret = h_->reg(req->agent_id().c_str(), caps, cap_count, &err);
        delete[] caps;
        if (ret != 0 && err) { resp->set_error(err); free(err); }
        return grpc::Status::OK;
    }

    grpc::Status Deregister(grpc::ServerContext *, const apb::Empty *,
                            apb::CommDeregisterResponse *resp) override {
        if (!h_->dereg) return grpc::Status::OK;
        char *err = nullptr;
        int ret = h_->dereg(&err);
        if (ret != 0 && err) { resp->set_error(err); free(err); }
        return grpc::Status::OK;
    }

    grpc::Status Receive(grpc::ServerContext *ctx, const apb::Empty *,
                         grpc::ServerWriter<apb::CommMessage> *writer) override {
        if (!h_->receive) return grpc::Status(grpc::UNIMPLEMENTED, "no receive handler");
        comm_stream_ctx sc{writer, ctx};
        int ret = h_->receive(comm_push_trampoline, &sc);
        return ret == 0 ? grpc::Status::OK : grpc::Status(grpc::INTERNAL, "receive error");
    }

    grpc::Status Configure(grpc::ServerContext *, const apb::SettingsMap *req,
                           apb::Empty *) override {
        configure_handler(*req, app_->on_configure);
        return grpc::Status::OK;
    }
};

/* ── ScheduleBridge ──────────────────────────────────────────────────── */

static void schedule_c_to_proto(const nebo_schedule_t *cs, apb::Schedule *ps) {
    if (cs->id)         ps->set_id(cs->id);
    if (cs->name)       ps->set_name(cs->name);
    if (cs->expression) ps->set_expression(cs->expression);
    if (cs->task_type)  ps->set_task_type(cs->task_type);
    if (cs->command)    ps->set_command(cs->command);
    if (cs->message)    ps->set_message(cs->message);
    if (cs->deliver)    ps->set_deliver(cs->deliver);
    ps->set_enabled(cs->enabled);
    if (cs->last_run)   ps->set_last_run(cs->last_run);
    if (cs->next_run)   ps->set_next_run(cs->next_run);
    ps->set_run_count(cs->run_count);
    if (cs->last_error) ps->set_last_error(cs->last_error);
    if (cs->created_at) ps->set_created_at(cs->created_at);
    if (cs->metadata) {
        auto *m = ps->mutable_metadata();
        for (int i = 0; i < cs->metadata->count; i++) {
            (*m)[cs->metadata->keys[i]] = cs->metadata->values[i];
        }
    }
}

struct schedule_stream_ctx {
    grpc::ServerWriter<apb::ScheduleTrigger> *writer;
    grpc::ServerContext *ctx;
};

static int schedule_push_trampoline(const nebo_schedule_trigger_t *trigger, void *opaque) {
    auto *sc = static_cast<schedule_stream_ctx *>(opaque);
    if (sc->ctx->IsCancelled()) return -1;
    apb::ScheduleTrigger st;
    if (trigger->schedule_id) st.set_schedule_id(trigger->schedule_id);
    if (trigger->name)        st.set_name(trigger->name);
    if (trigger->task_type)   st.set_task_type(trigger->task_type);
    if (trigger->command)     st.set_command(trigger->command);
    if (trigger->message)     st.set_message(trigger->message);
    if (trigger->deliver)     st.set_deliver(trigger->deliver);
    if (trigger->fired_at)    st.set_fired_at(trigger->fired_at);
    if (trigger->metadata) {
        auto *m = st.mutable_metadata();
        for (int i = 0; i < trigger->metadata->count; i++) {
            (*m)[trigger->metadata->keys[i]] = trigger->metadata->values[i];
        }
    }
    return sc->writer->Write(st) ? 0 : -1;
}

class ScheduleBridge final : public apb::ScheduleService::Service {
    const nebo_schedule_handler_t *h_;
    const nebo_app_t *app_;
public:
    ScheduleBridge(const nebo_schedule_handler_t *h, const nebo_app_t *app) : h_(h), app_(app) {}

    grpc::Status HealthCheck(grpc::ServerContext *, const apb::HealthCheckRequest *,
                             apb::HealthCheckResponse *resp) override {
        return health_ok(resp, app_);
    }

    grpc::Status Create(grpc::ServerContext *, const apb::CreateScheduleRequest *req,
                        apb::ScheduleResponse *resp) override {
        if (!h_->create) return grpc::Status(grpc::UNIMPLEMENTED, "no create handler");

        const char **keys = nullptr, **vals = nullptr;
        nebo_string_map_t meta{};
        if (req->metadata_size() > 0) meta = proto_map_to_c(req->metadata(), &keys, &vals);

        nebo_create_schedule_request_t creq{};
        creq.name = req->name().c_str();
        creq.expression = req->expression().c_str();
        creq.task_type = req->task_type().c_str();
        creq.command = req->command().c_str();
        creq.message = req->message().c_str();
        creq.deliver = req->deliver().c_str();
        creq.metadata = req->metadata_size() > 0 ? &meta : nullptr;

        nebo_schedule_t out{};
        char *err = nullptr;
        int ret = h_->create(&creq, &out, &err);

        delete[] keys;
        delete[] vals;

        if (ret != 0) {
            if (err) { resp->set_error(err); free(err); }
            return grpc::Status::OK;
        }
        schedule_c_to_proto(&out, resp->mutable_schedule());
        return grpc::Status::OK;
    }

    grpc::Status Get(grpc::ServerContext *, const apb::GetScheduleRequest *req,
                     apb::ScheduleResponse *resp) override {
        if (!h_->get) return grpc::Status(grpc::UNIMPLEMENTED, "no get handler");
        nebo_schedule_t out{};
        char *err = nullptr;
        int ret = h_->get(req->name().c_str(), &out, &err);
        if (ret != 0) {
            if (err) { resp->set_error(err); free(err); }
            return grpc::Status::OK;
        }
        schedule_c_to_proto(&out, resp->mutable_schedule());
        return grpc::Status::OK;
    }

    grpc::Status List(grpc::ServerContext *, const apb::ListSchedulesRequest *req,
                      apb::ListSchedulesResponse *resp) override {
        if (!h_->list) return grpc::Status(grpc::UNIMPLEMENTED, "no list handler");
        nebo_schedule_t *schedules = nullptr;
        int count = 0;
        long long total = 0;
        char *err = nullptr;
        int ret = h_->list(req->limit(), req->offset(), req->enabled_only(),
                           &schedules, &count, &total, &err);
        if (ret != 0) {
            if (err) free(err);
            return grpc::Status(grpc::INTERNAL, "list failed");
        }
        for (int i = 0; i < count; i++) {
            schedule_c_to_proto(&schedules[i], resp->add_schedules());
        }
        resp->set_total(total);
        free(schedules);
        return grpc::Status::OK;
    }

    grpc::Status Update(grpc::ServerContext *, const apb::UpdateScheduleRequest *req,
                        apb::ScheduleResponse *resp) override {
        if (!h_->update) return grpc::Status(grpc::UNIMPLEMENTED, "no update handler");

        const char **keys = nullptr, **vals = nullptr;
        nebo_string_map_t meta{};
        if (req->metadata_size() > 0) meta = proto_map_to_c(req->metadata(), &keys, &vals);

        nebo_update_schedule_request_t ureq{};
        ureq.name = req->name().c_str();
        ureq.expression = req->expression().c_str();
        ureq.task_type = req->task_type().c_str();
        ureq.command = req->command().c_str();
        ureq.message = req->message().c_str();
        ureq.deliver = req->deliver().c_str();
        ureq.metadata = req->metadata_size() > 0 ? &meta : nullptr;

        nebo_schedule_t out{};
        char *err = nullptr;
        int ret = h_->update(&ureq, &out, &err);

        delete[] keys;
        delete[] vals;

        if (ret != 0) {
            if (err) { resp->set_error(err); free(err); }
            return grpc::Status::OK;
        }
        schedule_c_to_proto(&out, resp->mutable_schedule());
        return grpc::Status::OK;
    }

    grpc::Status Delete(grpc::ServerContext *, const apb::DeleteScheduleRequest *req,
                        apb::DeleteScheduleResponse *resp) override {
        if (!h_->delete_schedule) return grpc::Status(grpc::UNIMPLEMENTED, "no delete handler");
        char *err = nullptr;
        int ret = h_->delete_schedule(req->name().c_str(), &err);
        resp->set_success(ret == 0);
        if (ret != 0 && err) { resp->set_error(err); free(err); }
        return grpc::Status::OK;
    }

    grpc::Status Enable(grpc::ServerContext *, const apb::ScheduleNameRequest *req,
                        apb::ScheduleResponse *resp) override {
        if (!h_->enable) return grpc::Status(grpc::UNIMPLEMENTED, "no enable handler");
        nebo_schedule_t out{};
        char *err = nullptr;
        int ret = h_->enable(req->name().c_str(), &out, &err);
        if (ret != 0) {
            if (err) { resp->set_error(err); free(err); }
            return grpc::Status::OK;
        }
        schedule_c_to_proto(&out, resp->mutable_schedule());
        return grpc::Status::OK;
    }

    grpc::Status Disable(grpc::ServerContext *, const apb::ScheduleNameRequest *req,
                         apb::ScheduleResponse *resp) override {
        if (!h_->disable) return grpc::Status(grpc::UNIMPLEMENTED, "no disable handler");
        nebo_schedule_t out{};
        char *err = nullptr;
        int ret = h_->disable(req->name().c_str(), &out, &err);
        if (ret != 0) {
            if (err) { resp->set_error(err); free(err); }
            return grpc::Status::OK;
        }
        schedule_c_to_proto(&out, resp->mutable_schedule());
        return grpc::Status::OK;
    }

    grpc::Status Trigger(grpc::ServerContext *, const apb::ScheduleNameRequest *req,
                         apb::TriggerResponse *resp) override {
        if (!h_->trigger) return grpc::Status(grpc::UNIMPLEMENTED, "no trigger handler");
        int success = 0;
        char *output = nullptr;
        char *err = nullptr;
        int ret = h_->trigger(req->name().c_str(), &success, &output, &err);
        resp->set_success(success);
        if (output) { resp->set_output(output); free(output); }
        if (ret != 0 && err) { resp->set_error(err); free(err); }
        return grpc::Status::OK;
    }

    grpc::Status History(grpc::ServerContext *, const apb::ScheduleHistoryRequest *req,
                         apb::ScheduleHistoryResponse *resp) override {
        if (!h_->history) return grpc::Status(grpc::UNIMPLEMENTED, "no history handler");
        nebo_schedule_history_entry_t *entries = nullptr;
        int count = 0;
        long long total = 0;
        char *err = nullptr;
        int ret = h_->history(req->name().c_str(), req->limit(), req->offset(),
                              &entries, &count, &total, &err);
        if (ret != 0) {
            if (err) free(err);
            return grpc::Status(grpc::INTERNAL, "history failed");
        }
        for (int i = 0; i < count; i++) {
            auto *e = resp->add_entries();
            if (entries[i].id)            e->set_id(entries[i].id);
            if (entries[i].schedule_name) e->set_schedule_name(entries[i].schedule_name);
            if (entries[i].started_at)    e->set_started_at(entries[i].started_at);
            if (entries[i].finished_at)   e->set_finished_at(entries[i].finished_at);
            e->set_success(entries[i].success);
            if (entries[i].output)        e->set_output(entries[i].output);
            if (entries[i].error)         e->set_error(entries[i].error);
        }
        resp->set_total(total);
        free(entries);
        return grpc::Status::OK;
    }

    grpc::Status Triggers(grpc::ServerContext *ctx, const apb::Empty *,
                          grpc::ServerWriter<apb::ScheduleTrigger> *writer) override {
        if (!h_->triggers) return grpc::Status(grpc::UNIMPLEMENTED, "no triggers handler");
        schedule_stream_ctx sc{writer, ctx};
        int ret = h_->triggers(schedule_push_trampoline, &sc);
        return ret == 0 ? grpc::Status::OK : grpc::Status(grpc::INTERNAL, "triggers error");
    }

    grpc::Status Configure(grpc::ServerContext *, const apb::SettingsMap *req,
                           apb::Empty *) override {
        configure_handler(*req, app_->on_configure);
        return grpc::Status::OK;
    }
};

/* ── Server entry point ──────────────────────────────────────────────── */

static std::unique_ptr<grpc::Server> g_server;

static void signal_handler(int) {
    if (g_server) g_server->Shutdown();
}

extern "C" int nebo_grpc_serve(nebo_app_t *app) {
    /* Remove stale socket */
    unlink(app->sock_path);

    std::string addr = std::string("unix:") + app->sock_path;

    grpc::ServerBuilder builder;
    builder.AddListeningPort(addr, grpc::InsecureServerCredentials());

    /* Conditionally register bridges for each non-NULL handler */
    ToolBridge     *tool_bridge     = nullptr;
    ChannelBridge  *channel_bridge  = nullptr;
    GatewayBridge  *gateway_bridge  = nullptr;
    UIBridge       *ui_bridge       = nullptr;
    CommBridge     *comm_bridge     = nullptr;
    ScheduleBridge *schedule_bridge = nullptr;

    if (app->tool) {
        tool_bridge = new ToolBridge(app->tool, app);
        builder.RegisterService(tool_bridge);
    }
    if (app->channel) {
        channel_bridge = new ChannelBridge(app->channel, app);
        builder.RegisterService(channel_bridge);
    }
    if (app->gateway) {
        gateway_bridge = new GatewayBridge(app->gateway, app);
        builder.RegisterService(gateway_bridge);
    }
    if (app->ui) {
        ui_bridge = new UIBridge(app->ui, app);
        builder.RegisterService(ui_bridge);
    }
    if (app->comm) {
        comm_bridge = new CommBridge(app->comm, app);
        builder.RegisterService(comm_bridge);
    }
    if (app->schedule) {
        schedule_bridge = new ScheduleBridge(app->schedule, app);
        builder.RegisterService(schedule_bridge);
    }

    g_server = builder.BuildAndStart();
    if (!g_server) {
        fprintf(stderr, "Failed to start gRPC server on %s\n", app->sock_path);
        delete tool_bridge;
        delete channel_bridge;
        delete gateway_bridge;
        delete ui_bridge;
        delete comm_bridge;
        delete schedule_bridge;
        return 1;
    }

    /* Graceful shutdown on SIGTERM/SIGINT */
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);

    g_server->Wait();

    g_server.reset();
    delete tool_bridge;
    delete channel_bridge;
    delete gateway_bridge;
    delete ui_bridge;
    delete comm_bridge;
    delete schedule_bridge;
    return 0;
}
