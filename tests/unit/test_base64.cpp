#include <catch2/catch_test_macros.hpp>

#include "apostol/base64.hpp"

#include <string>
#include <string_view>

using namespace apostol;

// ─── base64_encode ───────────────────────────────────────────────────────────

TEST_CASE("base64_encode: empty string", "[base64]")
{
    REQUIRE(base64_encode("") == "");
}

TEST_CASE("base64_encode: single byte (1 padding)", "[base64]")
{
    REQUIRE(base64_encode("f") == "Zg==");
}

TEST_CASE("base64_encode: two bytes (1 padding)", "[base64]")
{
    REQUIRE(base64_encode("fo") == "Zm8=");
}

TEST_CASE("base64_encode: three bytes (no padding)", "[base64]")
{
    REQUIRE(base64_encode("foo") == "Zm9v");
}

TEST_CASE("base64_encode: Hello World", "[base64]")
{
    REQUIRE(base64_encode("Hello, World!") == "SGVsbG8sIFdvcmxkIQ==");
}

TEST_CASE("base64_encode: known test vectors", "[base64]")
{
    // RFC 4648 test vectors
    REQUIRE(base64_encode("") == "");
    REQUIRE(base64_encode("f") == "Zg==");
    REQUIRE(base64_encode("fo") == "Zm8=");
    REQUIRE(base64_encode("foo") == "Zm9v");
    REQUIRE(base64_encode("foob") == "Zm9vYg==");
    REQUIRE(base64_encode("fooba") == "Zm9vYmE=");
    REQUIRE(base64_encode("foobar") == "Zm9vYmFy");
}

// ─── base64_decode ───────────────────────────────────────────────────────────

TEST_CASE("base64_decode: empty string", "[base64]")
{
    REQUIRE(base64_decode("") == "");
}

TEST_CASE("base64_decode: known test vectors", "[base64]")
{
    REQUIRE(base64_decode("Zg==") == "f");
    REQUIRE(base64_decode("Zm8=") == "fo");
    REQUIRE(base64_decode("Zm9v") == "foo");
    REQUIRE(base64_decode("Zm9vYg==") == "foob");
    REQUIRE(base64_decode("Zm9vYmE=") == "fooba");
    REQUIRE(base64_decode("Zm9vYmFy") == "foobar");
}

TEST_CASE("base64_decode: Hello World", "[base64]")
{
    REQUIRE(base64_decode("SGVsbG8sIFdvcmxkIQ==") == "Hello, World!");
}

TEST_CASE("base64_decode: ignores whitespace", "[base64]")
{
    REQUIRE(base64_decode("Zm9v\nYmFy") == "foobar");
    REQUIRE(base64_decode("Zm9v YmFy") == "foobar");
    REQUIRE(base64_decode(" Zm9v\r\nYmFy ") == "foobar");
}

TEST_CASE("base64_decode: invalid character throws", "[base64]")
{
    REQUIRE_THROWS_AS(base64_decode("Zm9v!"), std::invalid_argument);
}

// ─── Round-trip ──────────────────────────────────────────────────────────────

TEST_CASE("base64: round-trip ASCII", "[base64]")
{
    std::string input = "The quick brown fox jumps over the lazy dog";
    REQUIRE(base64_decode(base64_encode(input)) == input);
}

TEST_CASE("base64: round-trip binary data", "[base64]")
{
    // Binary data with null bytes and full byte range
    std::string binary;
    for (int i = 0; i < 256; ++i)
        binary += static_cast<char>(i);

    auto encoded = base64_encode(binary);
    auto decoded = base64_decode(encoded);
    REQUIRE(decoded.size() == 256);
    REQUIRE(decoded == binary);
}

TEST_CASE("base64: round-trip all sizes 0..100", "[base64]")
{
    for (int len = 0; len <= 100; ++len) {
        std::string input(static_cast<std::size_t>(len), 'A');
        REQUIRE(base64_decode(base64_encode(input)) == input);
    }
}
