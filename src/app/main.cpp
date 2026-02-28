#include "apostol/application.hpp"
#include "apostol/http.hpp"
#include "apostol/module.hpp"
#ifdef WITH_POSTGRESQL
#include "apostol/pg.hpp"
#endif

#include "Modules.hpp"    // create_workers() + create_helpers()

#include <chrono>
#include <memory>

using namespace apostol;

// ─── HeartbeatModule ─────────────────────────────────────────────────────────

class HeartbeatModule final : public Module
{
public:
    explicit HeartbeatModule(bool enabled = true) : enabled_(enabled) {}

    std::string_view name()    const override { return "heartbeat"; }
    bool             enabled() const override { return enabled_; }

    bool execute(const HttpRequest& req, HttpResponse& resp) override
    {
        if (req.path != "/healthz")
            return false;
        resp.set_status(200, "OK").set_body("ok", "text/plain");
        return true;
    }

private:
    bool enabled_;
};

// ─── ApostolApp ──────────────────────────────────────────────────────────────

class ApostolApp final : public Application
{
public:
    ApostolApp() : Application("apostol") {}

protected:
    void on_helper_start(EventLoop& loop) override
    {
#ifdef WITH_POSTGRESQL
        const auto& conninfo = settings().pg_conninfo_helper;
        if (!conninfo.empty()) {
            setup_db(loop, conninfo,
                static_cast<std::size_t>(settings().pg_pool_min),
                static_cast<std::size_t>(settings().pg_pool_max));
        }
#endif

        // Register all Helper modules (PGFetch, etc.)
        create_helpers(*this, loop);
    }

    void on_worker_start(EventLoop& loop) override
    {
#ifdef WITH_POSTGRESQL
        const auto& conninfo = settings().pg_conninfo_worker;
        if (conninfo.empty())
        {
            logger().warn("postgres.worker.conninfo not configured — skipping DB setup");
        }
        else
        {
            setup_db(loop, conninfo,
                static_cast<std::size_t>(settings().pg_pool_min),
                static_cast<std::size_t>(settings().pg_pool_max));
        }
#endif

        module_manager().add_module(
            std::make_unique<HeartbeatModule>(module_enabled("heartbeat")));

        create_workers(*this);

        start_http_server(loop, settings().server_port);
        logger().info("worker ready (pid={}, port={})", ::getpid(), http_port());
    }
};

int main(int argc, char* argv[])
{
    ApostolApp app;

    app.set_info(APP_NAME, APP_VERSION, APP_DESCRIPTION);

    return app.run(argc, argv);
}
