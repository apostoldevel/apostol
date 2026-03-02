// tests/unit/test_route_manager.cpp
#include <catch2/catch_test_macros.hpp>
#include "apostol/route_manager.hpp"

using namespace apostol;

// --- PathParams --------------------------------------------------------------

TEST_CASE("PathParams: empty params", "[route_manager]")
{
    PathParams p;
    REQUIRE_FALSE(p.has("id"));
    REQUIRE(p["id"].empty());
}

TEST_CASE("PathParams: set and get", "[route_manager]")
{
    PathParams p;
    p.params["id"] = "42";
    p.params["name"] = "alice";

    REQUIRE(p.has("id"));
    REQUIRE(p["id"] == "42");
    REQUIRE(p["name"] == "alice");
    REQUIRE_FALSE(p.has("missing"));
    REQUIRE(p["missing"].empty());
}

// --- Path matching -----------------------------------------------------------

TEST_CASE("RouteManager: exact path match", "[route_manager]")
{
    RouteManager rm;
    bool called = false;
    rm.add_route("GET", "/ping", [&](auto&, auto&, auto&) { called = true; });

    HttpRequest req;
    req.method = "GET";
    req.path = "/ping";
    HttpResponse resp;

    REQUIRE(rm.dispatch(req, resp));
    REQUIRE(called);
}

TEST_CASE("RouteManager: exact path no match", "[route_manager]")
{
    RouteManager rm;
    rm.add_route("GET", "/ping", [](auto&, auto&, auto&) {});

    HttpRequest req;
    req.method = "GET";
    req.path = "/other";
    HttpResponse resp;

    REQUIRE_FALSE(rm.dispatch(req, resp));
}

TEST_CASE("RouteManager: method mismatch", "[route_manager]")
{
    RouteManager rm;
    rm.add_route("GET", "/ping", [](auto&, auto&, auto&) {});

    HttpRequest req;
    req.method = "POST";
    req.path = "/ping";
    HttpResponse resp;

    REQUIRE_FALSE(rm.dispatch(req, resp));
}

TEST_CASE("RouteManager: path params extraction", "[route_manager]")
{
    RouteManager rm;
    PathParams captured;
    rm.add_route("GET", "/users/{id}", [&](auto&, auto&, auto& p) {
        captured = p;
    });

    HttpRequest req;
    req.method = "GET";
    req.path = "/users/42";
    HttpResponse resp;

    REQUIRE(rm.dispatch(req, resp));
    REQUIRE(captured["id"] == "42");
}

TEST_CASE("RouteManager: multiple path params", "[route_manager]")
{
    RouteManager rm;
    PathParams captured;
    rm.add_route("GET", "/users/{uid}/posts/{pid}",
        [&](auto&, auto&, auto& p) { captured = p; });

    HttpRequest req;
    req.method = "GET";
    req.path = "/users/alice/posts/99";
    HttpResponse resp;

    REQUIRE(rm.dispatch(req, resp));
    REQUIRE(captured["uid"] == "alice");
    REQUIRE(captured["pid"] == "99");
}

TEST_CASE("RouteManager: wildcard match", "[route_manager]")
{
    RouteManager rm;
    bool called = false;
    rm.add_route("GET", "/files/*", [&](auto&, auto&, auto&) { called = true; });

    HttpRequest req;
    req.method = "GET";
    req.path = "/files/images/logo.png";
    HttpResponse resp;

    REQUIRE(rm.dispatch(req, resp));
    REQUIRE(called);
}

TEST_CASE("RouteManager: wildcard does not match shorter", "[route_manager]")
{
    RouteManager rm;
    rm.add_route("GET", "/files/*", [](auto&, auto&, auto&) {});

    HttpRequest req;
    req.method = "GET";
    req.path = "/files";
    HttpResponse resp;

    REQUIRE_FALSE(rm.dispatch(req, resp));
}

TEST_CASE("RouteManager: exact beats parametric", "[route_manager]")
{
    RouteManager rm;
    std::string which;
    rm.add_route("GET", "/users/me", [&](auto&, auto&, auto&) { which = "exact"; });
    rm.add_route("GET", "/users/{id}", [&](auto&, auto&, auto&) { which = "param"; });

    HttpRequest req;
    req.method = "GET";
    req.path = "/users/me";
    HttpResponse resp;

    REQUIRE(rm.dispatch(req, resp));
    REQUIRE(which == "exact");
}

TEST_CASE("RouteManager: parametric beats wildcard", "[route_manager]")
{
    RouteManager rm;
    std::string which;
    rm.add_route("GET", "/files/{name}", [&](auto&, auto&, auto&) { which = "param"; });
    rm.add_route("GET", "/files/*", [&](auto&, auto&, auto&) { which = "wild"; });

    HttpRequest req;
    req.method = "GET";
    req.path = "/files/readme.txt";
    HttpResponse resp;

    REQUIRE(rm.dispatch(req, resp));
    REQUIRE(which == "param");
}

TEST_CASE("RouteManager: base_path prepended", "[route_manager]")
{
    RouteManager rm;
    rm.set_base_path("/api/v1");
    bool called = false;
    rm.add_route("GET", "/ping", [&](auto&, auto&, auto&) { called = true; });

    HttpRequest req;
    req.method = "GET";
    req.path = "/api/v1/ping";
    HttpResponse resp;

    REQUIRE(rm.dispatch(req, resp));
    REQUIRE(called);
}

TEST_CASE("RouteManager: has_route checks any method", "[route_manager]")
{
    RouteManager rm;
    rm.add_route("GET", "/ping", [](auto&, auto&, auto&) {});
    rm.add_route("POST", "/data", [](auto&, auto&, auto&) {});

    REQUIRE(rm.has_route("/ping"));
    REQUIRE(rm.has_route("/data"));
    REQUIRE_FALSE(rm.has_route("/other"));
}

TEST_CASE("RouteManager: trailing slash normalization", "[route_manager]")
{
    RouteManager rm;
    bool called = false;
    rm.add_route("GET", "/ping", [&](auto&, auto&, auto&) { called = true; });

    HttpRequest req;
    req.method = "GET";
    req.path = "/ping/";
    HttpResponse resp;

    REQUIRE(rm.dispatch(req, resp));
    REQUIRE(called);
}

// --- RouteBuilder ------------------------------------------------------------

TEST_CASE("RouteBuilder: fluent metadata", "[route_manager]")
{
    RouteManager rm;

    rm.add_route("GET", "/ping", [](auto&, auto&, auto&) {})
        .summary("Health check")
        .description("Returns 200 if service is alive")
        .tag("Health")
        .response(200, "OK");

    rm.add_route("POST", "/users", [](auto&, auto&, auto&) {})
        .summary("Create user")
        .tag("Users")
        .param("name", "query", "string", true, "User name")
        .request_body("application/json", {{"type", "object"}})
        .response(201, "Created")
        .response(400, "Bad request");

    REQUIRE(rm.size() == 2);
}

TEST_CASE("RouteBuilder: deprecated flag", "[route_manager]")
{
    RouteManager rm;

    rm.add_route("GET", "/old", [](auto&, auto&, auto&) {})
        .summary("Old endpoint")
        .deprecated();

    REQUIRE(rm.size() == 1);
}

// --- OpenAPI spec ------------------------------------------------------------

TEST_CASE("RouteManager: openapi_spec basic structure", "[route_manager]")
{
    RouteManager rm;
    rm.set_info("Test API", "1.0.0", "A test API");
    rm.add_server("http://localhost:8080", "Dev server");

    rm.add_route("GET", "/ping", [](auto&, auto&, auto&) {})
        .summary("Health check")
        .tag("Health")
        .response(200, "OK");

    auto spec = rm.openapi_spec();

    REQUIRE(spec["openapi"] == "3.0.0");
    REQUIRE(spec["info"]["title"] == "Test API");
    REQUIRE(spec["info"]["version"] == "1.0.0");
    REQUIRE(spec["info"]["description"] == "A test API");
    REQUIRE(spec["servers"].size() == 1);
    REQUIRE(spec["servers"][0]["url"] == "http://localhost:8080");

    REQUIRE(spec["paths"].contains("/ping"));
    auto& ping_get = spec["paths"]["/ping"]["get"];
    REQUIRE(ping_get["summary"] == "Health check");
    REQUIRE(ping_get["tags"] == nlohmann::json::array({"Health"}));
    REQUIRE(ping_get["responses"].contains("200"));
    REQUIRE(ping_get["responses"]["200"]["description"] == "OK");
}

TEST_CASE("RouteManager: openapi_spec with params and body", "[route_manager]")
{
    RouteManager rm;
    rm.set_info("API", "1.0.0");

    rm.add_route("POST", "/users/{id}", [](auto&, auto&, auto&) {})
        .summary("Update user")
        .tag("Users")
        .param("id", "path", "string", true, "User ID")
        .param("verbose", "query", "boolean", false, "Verbose output")
        .request_body("application/json", {{"type", "object"}})
        .response(200, "Updated", {{"type", "object"}})
        .response(404, "Not found");

    auto spec = rm.openapi_spec();
    auto& op = spec["paths"]["/users/{id}"]["post"];

    REQUIRE(op["parameters"].size() == 2);
    REQUIRE(op["parameters"][0]["name"] == "id");
    REQUIRE(op["parameters"][0]["in"] == "path");
    REQUIRE(op["parameters"][0]["required"] == true);

    REQUIRE(op["requestBody"]["required"] == true);
    REQUIRE(op["requestBody"]["content"].contains("application/json"));

    REQUIRE(op["responses"].contains("200"));
    REQUIRE(op["responses"].contains("404"));
}

TEST_CASE("RouteManager: openapi_spec with base_path", "[route_manager]")
{
    RouteManager rm;
    rm.set_base_path("/api/v1");
    rm.set_info("API", "1.0.0");

    rm.add_route("GET", "/ping", [](auto&, auto&, auto&) {});

    auto spec = rm.openapi_spec();
    REQUIRE(spec["paths"].contains("/api/v1/ping"));
}

TEST_CASE("RouteManager: openapi_spec deprecated", "[route_manager]")
{
    RouteManager rm;
    rm.set_info("API", "1.0.0");

    rm.add_route("GET", "/old", [](auto&, auto&, auto&) {})
        .deprecated();

    auto spec = rm.openapi_spec();
    REQUIRE(spec["paths"]["/old"]["get"]["deprecated"] == true);
}

TEST_CASE("RouteManager: openapi_spec collects unique tags", "[route_manager]")
{
    RouteManager rm;
    rm.set_info("API", "1.0.0");

    rm.add_route("GET", "/a", [](auto&, auto&, auto&) {}).tag("Users");
    rm.add_route("GET", "/b", [](auto&, auto&, auto&) {}).tag("Users");
    rm.add_route("GET", "/c", [](auto&, auto&, auto&) {}).tag("Health");

    auto spec = rm.openapi_spec();
    REQUIRE(spec["tags"].size() == 2);  // Users + Health (deduplicated)
}

// --- YAML writer -------------------------------------------------------------

#include "apostol/yaml_writer.hpp"

TEST_CASE("json_to_yaml: simple object", "[yaml_writer]")
{
    nlohmann::json j = {{"name", "test"}, {"version", "1.0"}};
    auto yaml = json_to_yaml(j);

    REQUIRE(yaml.find("name: test") != std::string::npos);
    REQUIRE(yaml.find("version: '1.0'") != std::string::npos);
}

TEST_CASE("json_to_yaml: nested object", "[yaml_writer]")
{
    nlohmann::json j = {{"info", {{"title", "API"}, {"version", "1.0"}}}};
    auto yaml = json_to_yaml(j);

    REQUIRE(yaml.find("info:") != std::string::npos);
    REQUIRE(yaml.find("  title: API") != std::string::npos);
}

TEST_CASE("json_to_yaml: array", "[yaml_writer]")
{
    nlohmann::json j = {{"tags", {"Health", "Users"}}};
    auto yaml = json_to_yaml(j);

    REQUIRE(yaml.find("tags:") != std::string::npos);
    REQUIRE(yaml.find("- Health") != std::string::npos);
    REQUIRE(yaml.find("- Users") != std::string::npos);
}

TEST_CASE("json_to_yaml: boolean and number", "[yaml_writer]")
{
    nlohmann::json j = {{"required", true}, {"code", 200}};
    auto yaml = json_to_yaml(j);

    REQUIRE(yaml.find("required: true") != std::string::npos);
    REQUIRE(yaml.find("code: 200") != std::string::npos);
}

TEST_CASE("json_to_yaml: string requiring quotes", "[yaml_writer]")
{
    nlohmann::json j = {{"desc", "yes"}, {"port", "8080"}};
    auto yaml = json_to_yaml(j);

    // "yes" is a YAML reserved word, must be quoted
    REQUIRE(yaml.find("desc: 'yes'") != std::string::npos);
    // "8080" looks numeric, must be quoted to stay string
    REQUIRE(yaml.find("port: '8080'") != std::string::npos);
}

TEST_CASE("json_to_yaml: empty object and array", "[yaml_writer]")
{
    nlohmann::json j = {{"obj", nlohmann::json::object()}, {"arr", nlohmann::json::array()}};
    auto yaml = json_to_yaml(j);

    REQUIRE(yaml.find("obj: {}") != std::string::npos);
    REQUIRE(yaml.find("arr: []") != std::string::npos);
}

// --- RoutedModule ------------------------------------------------------------

#include "apostol/routed_module.hpp"

namespace
{

class TestRoutedModule final : public apostol::RoutedModule
{
public:
    using RoutedModule::RoutedModule;

    std::string_view name() const override { return "TestRouted"; }
    bool enabled() const override { return true; }

protected:
    void init_routes() override
    {
        routes_.set_base_path("/api/v1");
        routes_.set_info("Test", "1.0.0");

        routes_.add_route("GET", "/ping",
            [](auto&, auto& resp, auto&) {
                resp.set_status(apostol::HttpStatus::ok)
                    .set_body(R"({"status":"ok"})", "application/json");
            })
            .summary("Ping")
            .tag("Health")
            .response(200, "OK");

        routes_.add_route("GET", "/users/{id}",
            [](auto&, auto& resp, auto& params) {
                resp.set_status(apostol::HttpStatus::ok)
                    .set_body("{\"id\":\"" + params["id"] + "\"}", "application/json");
            })
            .summary("Get user")
            .tag("Users")
            .param("id", "path", "string", true);
    }
};

} // anonymous namespace

TEST_CASE("RoutedModule: dispatch to exact route", "[routed_module]")
{
    TestRoutedModule mod;

    HttpRequest req;
    req.method = "GET";
    req.path = "/api/v1/ping";
    HttpResponse resp;

    REQUIRE(mod.execute(req, resp));
    auto s = resp.serialize();
    REQUIRE(s.find("200 OK") != std::string::npos);
    REQUIRE(s.find("\"status\":\"ok\"") != std::string::npos);
}

TEST_CASE("RoutedModule: dispatch with path params", "[routed_module]")
{
    TestRoutedModule mod;

    HttpRequest req;
    req.method = "GET";
    req.path = "/api/v1/users/42";
    HttpResponse resp;

    REQUIRE(mod.execute(req, resp));
    auto s = resp.serialize();
    REQUIRE(s.find("\"id\":\"42\"") != std::string::npos);
}

TEST_CASE("RoutedModule: returns false for unknown path", "[routed_module]")
{
    TestRoutedModule mod;

    HttpRequest req;
    req.method = "GET";
    req.path = "/unknown";
    HttpResponse resp;

    REQUIRE_FALSE(mod.execute(req, resp));
}

TEST_CASE("RoutedModule: OPTIONS returns allowed methods", "[routed_module]")
{
    TestRoutedModule mod;

    HttpRequest req;
    req.method = "OPTIONS";
    req.path = "/api/v1/ping";
    HttpResponse resp;

    REQUIRE(mod.execute(req, resp));
    auto s = resp.serialize();
    REQUIRE(s.find("204") != std::string::npos);
}

TEST_CASE("RoutedModule: method not allowed", "[routed_module]")
{
    TestRoutedModule mod;

    HttpRequest req;
    req.method = "DELETE";
    req.path = "/api/v1/ping";
    HttpResponse resp;

    REQUIRE(mod.execute(req, resp));
    auto s = resp.serialize();
    REQUIRE(s.find("405") != std::string::npos);
}

TEST_CASE("RoutedModule: openapi_spec accessible", "[routed_module]")
{
    TestRoutedModule mod;
    // Trigger lazy init
    HttpRequest req;
    req.method = "GET";
    req.path = "/api/v1/ping";
    HttpResponse resp;
    mod.execute(req, resp);

    auto spec = mod.routes().openapi_spec();
    REQUIRE(spec["info"]["title"] == "Test");
    REQUIRE(spec["paths"].contains("/api/v1/ping"));
    REQUIRE(spec["paths"].contains("/api/v1/users/{id}"));
}

// --- Docs endpoints ----------------------------------------------------------

TEST_CASE("RoutedModule: GET /docs returns HTML", "[routed_module]")
{
    TestRoutedModule mod;

    HttpRequest req;
    req.method = "GET";
    req.path = "/docs";
    HttpResponse resp;

    REQUIRE(mod.execute(req, resp));
    auto s = resp.serialize();
    REQUIRE(s.find("200 OK") != std::string::npos);
    REQUIRE(s.find("text/html") != std::string::npos);
    REQUIRE(s.find("swagger-ui") != std::string::npos);
}

TEST_CASE("RoutedModule: GET /docs/api.json returns spec", "[routed_module]")
{
    TestRoutedModule mod;

    HttpRequest req;
    req.method = "GET";
    req.path = "/docs/api.json";
    HttpResponse resp;

    REQUIRE(mod.execute(req, resp));
    auto s = resp.serialize();
    REQUIRE(s.find("200 OK") != std::string::npos);
    REQUIRE(s.find("application/json") != std::string::npos);
    REQUIRE(s.find("\"openapi\"") != std::string::npos);
    REQUIRE(s.find("\"3.0.0\"") != std::string::npos);
}

TEST_CASE("RoutedModule: GET /docs/api.yaml returns YAML", "[routed_module]")
{
    TestRoutedModule mod;

    HttpRequest req;
    req.method = "GET";
    req.path = "/docs/api.yaml";
    HttpResponse resp;

    REQUIRE(mod.execute(req, resp));
    auto s = resp.serialize();
    REQUIRE(s.find("200 OK") != std::string::npos);
    REQUIRE(s.find("openapi:") != std::string::npos);
}
