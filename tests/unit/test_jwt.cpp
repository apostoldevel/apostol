#ifdef WITH_SSL

#include <catch2/catch_test_macros.hpp>

#include "apostol/jwt.hpp"
#include "apostol/oauth_providers.hpp"

#define JWT_DISABLE_PICOJSON
#include "jwt-cpp/traits/nlohmann-json/traits.h"

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

using jwt_traits = jwt::traits::nlohmann_json;

static const fs::path test_dir = fs::temp_directory_path() / "apostol_test_jwt";

// RAII cleanup
struct JwtTestGuard {
    JwtTestGuard() { fs::create_directories(test_dir); }
    ~JwtTestGuard() { fs::remove_all(test_dir); }
};

static void write_provider(const std::string& filename,
                           const std::string& app_name,
                           const std::string& client_id,
                           const std::string& client_secret)
{
    nlohmann::json j;
    j[app_name] = {
        {"client_id", client_id},
        {"client_secret", client_secret}
    };

    std::ofstream f(test_dir / filename);
    f << j.dump();
}

static apostol::OAuthProviders load_providers()
{
    apostol::OAuthProviders providers;
    providers.load(test_dir);
    return providers;
}

static std::string make_token(const std::string& secret,
                              const std::string& sub,
                              const std::string& aud,
                              const std::string& iss = "test-issuer",
                              int expire_secs = 3600)
{
    auto now = std::chrono::system_clock::now();
    return jwt::create<jwt_traits>()
        .set_algorithm("HS256")
        .set_issuer(iss)
        .set_audience(aud)
        .set_subject(sub)
        .set_issued_at(now)
        .set_expires_at(now + std::chrono::seconds(expire_secs))
        .sign(jwt::algorithm::hs256{secret});
}

TEST_CASE("jwt: verify valid token", "[jwt]")
{
    JwtTestGuard guard;
    write_provider("test.json", "web", "my-client-id", "my-secret-key");

    auto providers = load_providers();
    auto token = make_token("my-secret-key", "session-123", "my-client-id");
    auto claims = apostol::verify_jwt(token, providers);

    REQUIRE(claims.sub == "session-123");
    REQUIRE(claims.aud == "my-client-id");
    REQUIRE(claims.iss == "test-issuer");
}

TEST_CASE("jwt: expired token throws JwtExpiredError", "[jwt]")
{
    JwtTestGuard guard;
    write_provider("test.json", "web", "my-client-id", "my-secret-key");

    auto providers = load_providers();
    auto token = make_token("my-secret-key", "session-123", "my-client-id", "iss", -10);

    REQUIRE_THROWS_AS(apostol::verify_jwt(token, providers), apostol::JwtExpiredError);
}

TEST_CASE("jwt: wrong secret throws JwtVerificationError", "[jwt]")
{
    JwtTestGuard guard;
    write_provider("test.json", "web", "my-client-id", "correct-secret");

    auto providers = load_providers();
    auto token = make_token("wrong-secret", "session-123", "my-client-id");

    REQUIRE_THROWS_AS(apostol::verify_jwt(token, providers), apostol::JwtVerificationError);
}

TEST_CASE("jwt: unknown audience throws JwtVerificationError", "[jwt]")
{
    JwtTestGuard guard;
    write_provider("test.json", "web", "known-client", "secret");

    auto providers = load_providers();
    auto token = make_token("secret", "session-123", "unknown-client");

    REQUIRE_THROWS_AS(apostol::verify_jwt(token, providers), apostol::JwtVerificationError);
}

TEST_CASE("jwt: empty providers throws JwtVerificationError", "[jwt]")
{
    apostol::OAuthProviders empty;
    auto token = make_token("secret", "session-123", "any-client");

    REQUIRE_THROWS_AS(apostol::verify_jwt(token, empty), apostol::JwtVerificationError);
}

TEST_CASE("jwt: HS384 algorithm works", "[jwt]")
{
    JwtTestGuard guard;
    write_provider("test.json", "web", "client-384", "secret-384");

    auto providers = load_providers();

    auto now = std::chrono::system_clock::now();
    auto token = jwt::create<jwt_traits>()
        .set_algorithm("HS384")
        .set_issuer("iss")
        .set_audience("client-384")
        .set_subject("sub-384")
        .set_issued_at(now)
        .set_expires_at(now + std::chrono::hours(1))
        .sign(jwt::algorithm::hs384{"secret-384"});

    auto claims = apostol::verify_jwt(token, providers);
    REQUIRE(claims.sub == "sub-384");
}

#endif // WITH_SSL
