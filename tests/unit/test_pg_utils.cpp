#ifdef WITH_POSTGRESQL

#include <catch2/catch_test_macros.hpp>

#include "apostol/pg_utils.hpp"

#include <nlohmann/json.hpp>

using namespace apostol;

// ─── pq_quote_literal ───────────────────────────────────────────────────────

TEST_CASE("pq_quote_literal — simple string", "[pg_utils]")
{
    CHECK(pq_quote_literal("hello") == "E'hello'");
}

TEST_CASE("pq_quote_literal — single quotes escaped", "[pg_utils]")
{
    CHECK(pq_quote_literal("it's") == "E'it''s'");
}

TEST_CASE("pq_quote_literal — backslashes escaped", "[pg_utils]")
{
    CHECK(pq_quote_literal("a\\b") == "E'a\\\\b'");
}

TEST_CASE("pq_quote_literal — empty string", "[pg_utils]")
{
    CHECK(pq_quote_literal("") == "E''");
}

TEST_CASE("pq_quote_literal — mixed special chars", "[pg_utils]")
{
    CHECK(pq_quote_literal("O'Bri\\en") == "E'O''Bri\\\\en'");
}

// ─── headers_to_json ────────────────────────────────────────────────────────

TEST_CASE("headers_to_json — basic", "[pg_utils]")
{
    std::vector<std::pair<std::string, std::string>> h = {
        {"Content-Type", "application/json"},
        {"Accept", "text/html"}
    };
    auto j = nlohmann::json::parse(headers_to_json(h));
    CHECK(j["Content-Type"] == "application/json");
    CHECK(j["Accept"] == "text/html");
}

TEST_CASE("headers_to_json — empty", "[pg_utils]")
{
    std::vector<std::pair<std::string, std::string>> h;
    CHECK(headers_to_json(h) == "{}");
}

// ─── params_to_json ─────────────────────────────────────────────────────────

TEST_CASE("params_to_json — basic", "[pg_utils]")
{
    std::vector<std::pair<std::string, std::string>> p = {
        {"page", "1"},
        {"limit", "10"}
    };
    auto j = nlohmann::json::parse(params_to_json(p));
    CHECK(j["page"] == "1");
    CHECK(j["limit"] == "10");
}

TEST_CASE("params_to_json — empty", "[pg_utils]")
{
    std::vector<std::pair<std::string, std::string>> p;
    CHECK(params_to_json(p) == "{}");
}

// ─── form_to_json ───────────────────────────────────────────────────────────

TEST_CASE("form_to_json — basic", "[pg_utils]")
{
    auto j = nlohmann::json::parse(form_to_json("name=John&age=30"));
    CHECK(j["name"] == "John");
    CHECK(j["age"] == "30");
}

TEST_CASE("form_to_json — url-encoded values", "[pg_utils]")
{
    auto j = nlohmann::json::parse(form_to_json("msg=hello+world&path=%2Fapi%2Fv1"));
    CHECK(j["msg"] == "hello world");
    CHECK(j["path"] == "/api/v1");
}

TEST_CASE("form_to_json — empty", "[pg_utils]")
{
    CHECK(form_to_json("") == "{}");
}

// ─── check_pg_error ─────────────────────────────────────────────────────────

TEST_CASE("check_pg_error — no error", "[pg_utils]")
{
    std::string msg;
    CHECK(check_pg_error(R"({"result": "ok"})", msg) == 0);
    CHECK(msg.empty());
}

TEST_CASE("check_pg_error — with error", "[pg_utils]")
{
    std::string msg;
    int code = check_pg_error(R"({"error":{"code":40100,"message":"Unauthorized"}})", msg);
    CHECK(code == 40100);
    CHECK(msg == "Unauthorized");
}

TEST_CASE("check_pg_error — invalid json", "[pg_utils]")
{
    std::string msg;
    CHECK(check_pg_error("not json", msg) == 0);
}

TEST_CASE("check_pg_error — error without message", "[pg_utils]")
{
    std::string msg;
    int code = check_pg_error(R"({"error":{"code":500}})", msg);
    CHECK(code == 500);
    CHECK(msg.empty());
}

// ─── error_code_to_status ───────────────────────────────────────────────────

TEST_CASE("error_code_to_status — direct codes", "[pg_utils]")
{
    CHECK(error_code_to_status(401) == HttpStatus::unauthorized);
    CHECK(error_code_to_status(403) == HttpStatus::forbidden);
    CHECK(error_code_to_status(404) == HttpStatus::not_found);
    CHECK(error_code_to_status(500) == HttpStatus::internal_server_error);
}

TEST_CASE("error_code_to_status — high codes divided by 100", "[pg_utils]")
{
    CHECK(error_code_to_status(40100) == HttpStatus::unauthorized);
    CHECK(error_code_to_status(40300) == HttpStatus::forbidden);
    CHECK(error_code_to_status(40400) == HttpStatus::not_found);
    CHECK(error_code_to_status(50000) == HttpStatus::internal_server_error);
}

TEST_CASE("error_code_to_status — unknown code → 400", "[pg_utils]")
{
    CHECK(error_code_to_status(42) == HttpStatus::bad_request);
    CHECK(error_code_to_status(-1) == HttpStatus::bad_request);
}

#endif // WITH_POSTGRESQL
