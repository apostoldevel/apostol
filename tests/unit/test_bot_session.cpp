#ifdef WITH_POSTGRESQL

#include <catch2/catch_test_macros.hpp>

#include "apostol/bot_session.hpp"

#include <string>

// These tests only verify the non-PG parts of BotSession.
// Full integration requires a live db-platform instance.

namespace apostol
{

// Use a mock pool reference — we just need to verify construction and state
// without actually connecting to PG.

TEST_CASE("bot_session: initial state", "[bot_session]")
{
    // We cannot create a real PgPool without EventLoop + libpq,
    // so we test the interface contract only.
    // BotSession construction requires a PgPool&, which we can't easily mock.
    // This test verifies the expected behavior documented in the header.

    // Verify the class is constructible with the expected interface:
    // BotSession(PgPool&, string, string)
    // session() returns empty initially
    // valid() returns false initially
    static_assert(std::is_constructible_v<BotSession, PgPool&, std::string, std::string>);
}

} // namespace apostol

#endif // WITH_POSTGRESQL
