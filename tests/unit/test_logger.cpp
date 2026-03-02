#include <catch2/catch_test_macros.hpp>

#include "apostol/logger.hpp"

#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <zlib.h>

namespace fs = std::filesystem;
using namespace apostol;

// ─── Level helpers ────────────────────────────────────────────────────────────

TEST_CASE("level_name returns correct strings", "[logger]")
{
    REQUIRE(level_name(LogLevel::emerg) == "emerg");
    REQUIRE(level_name(LogLevel::alert) == "alert");
    REQUIRE(level_name(LogLevel::crit) == "crit");
    REQUIRE(level_name(LogLevel::error) == "error");
    REQUIRE(level_name(LogLevel::warn) == "warn");
    REQUIRE(level_name(LogLevel::notice) == "notice");
    REQUIRE(level_name(LogLevel::info) == "info");
    REQUIRE(level_name(LogLevel::debug) == "debug");
}

TEST_CASE("level_from_string parses correctly", "[logger]")
{
    REQUIRE(level_from_string("emerg") == LogLevel::emerg);
    REQUIRE(level_from_string("error") == LogLevel::error);
    REQUIRE(level_from_string("warn") == LogLevel::warn);
    REQUIRE(level_from_string("warning") == LogLevel::warn);
    REQUIRE(level_from_string("info") == LogLevel::info);
    REQUIRE(level_from_string("debug") == LogLevel::debug);
}

TEST_CASE("level_from_string throws on unknown level", "[logger]")
{
    REQUIRE_THROWS_AS(level_from_string("unknown"), std::invalid_argument);
    REQUIRE_THROWS_AS(level_from_string(""), std::invalid_argument);
}

// ─── Logger level filtering ───────────────────────────────────────────────────

struct CapturingTarget : LogTarget
{
    std::vector<std::string> messages;

    void write(LogLevel /*level*/, std::string_view message) override { messages.emplace_back(message); }
};

TEST_CASE("Logger filters messages below configured level", "[logger]")
{
    Logger logger;
    auto* cap = new CapturingTarget();
    logger.add_target(std::unique_ptr<LogTarget>(cap));
    logger.set_level(LogLevel::warn);

    logger.info("should be filtered");
    logger.debug("also filtered");
    logger.warn("should appear");
    logger.error("should appear");

    REQUIRE(cap->messages.size() == 2);
}

TEST_CASE("Logger passes messages at or above configured level", "[logger]")
{
    Logger logger;
    auto* cap = new CapturingTarget();
    logger.add_target(std::unique_ptr<LogTarget>(cap));
    logger.set_level(LogLevel::debug);

    logger.debug("debug msg");
    logger.info("info msg");
    logger.warn("warn msg");

    REQUIRE(cap->messages.size() == 3);
}

TEST_CASE("Logger message contains level name and text", "[logger]")
{
    Logger logger;
    auto* cap = new CapturingTarget();
    logger.add_target(std::unique_ptr<LogTarget>(cap));
    logger.set_level(LogLevel::debug);

    logger.error("something broke");

    REQUIRE(cap->messages.size() == 1);
    REQUIRE(cap->messages[0].find("error:") != std::string::npos);
    REQUIRE(cap->messages[0].find("something broke") != std::string::npos);
}

TEST_CASE("Logger supports std::format arguments", "[logger]")
{
    Logger logger;
    auto* cap = new CapturingTarget();
    logger.add_target(std::unique_ptr<LogTarget>(cap));

    logger.info("value is {}", 42);
    logger.info("x={} y={}", 1.5, "hello");

    REQUIRE(cap->messages.size() == 2);
    REQUIRE(cap->messages[0].find("value is 42") != std::string::npos);
    REQUIRE(cap->messages[1].find("x=1.5 y=hello") != std::string::npos);
}

// ─── FileTarget ───────────────────────────────────────────────────────────────

TEST_CASE("FileTarget writes to file", "[logger][file]")
{
    auto tmp = fs::temp_directory_path() / fmt::format("apostol_test_logger_{}.log", ::getpid());
    fs::remove(tmp);

    {
        Logger logger;
        logger.add_target(std::make_unique<FileTarget>(tmp.string()));
        logger.set_level(LogLevel::debug);
        logger.info("hello from file");
        logger.flush();
    }

    std::ifstream f(tmp);
    REQUIRE(f.is_open());
    std::string content((std::istreambuf_iterator<char>(f)), {});
    REQUIRE(content.find("hello from file") != std::string::npos);

    fs::remove(tmp);
}

TEST_CASE("FileTarget rotates when max size exceeded (uncompressed)", "[logger][file]")
{
    auto tmp  = fs::temp_directory_path() / fmt::format("apostol_test_rotate_{}.log", ::getpid());
    auto tmp1 = fs::temp_directory_path() / fmt::format("apostol_test_rotate_{}.log.1", ::getpid());
    fs::remove(tmp);
    fs::remove(tmp1);

    {
        Logger logger;
        // Very small max size to force rotation; compress=false
        logger.add_target(std::make_unique<FileTarget>(tmp.string(), 50, 2, false));
        logger.set_level(LogLevel::debug);

        for (int i = 0; i < 20; ++i)
            logger.info("line {}", i);

        logger.flush();
    }

    // At least one rotation should have happened
    REQUIRE(fs::exists(tmp1));

    fs::remove(tmp);
    fs::remove(tmp1);
}

TEST_CASE("FileTarget rotates with gzip compression", "[logger][file]")
{
    auto base = fs::temp_directory_path() / fmt::format("apostol_test_gz_{}.log", ::getpid());
    auto gz1  = fs::path(base.string() + ".1.gz");
    auto gz2  = fs::path(base.string() + ".2.gz");
    fs::remove(base);
    fs::remove(gz1);
    fs::remove(gz2);

    {
        Logger logger;
        // Small max size, 3 backups, compress=true
        logger.add_target(std::make_unique<FileTarget>(base.string(), 50, 3, true));
        logger.set_level(LogLevel::debug);

        for (int i = 0; i < 40; ++i)
            logger.info("gzline {}", i);

        logger.flush();
    }

    // At least one .gz backup should exist
    REQUIRE(fs::exists(gz1));

    // Verify it's a valid gzip file by reading it
    gzFile gz = ::gzopen(gz1.c_str(), "rb");
    REQUIRE(gz != nullptr);
    char buf[4096];
    int bytes = ::gzread(gz, buf, sizeof(buf) - 1);
    ::gzclose(gz);
    REQUIRE(bytes > 0);
    buf[bytes] = '\0';
    // Should contain log lines
    REQUIRE(std::string(buf).find("gzline") != std::string::npos);

    // Cleanup
    fs::remove(base);
    fs::remove(gz1);
    fs::remove(gz2);
    fs::remove(fs::path(base.string() + ".3.gz"));
}

TEST_CASE("FileTarget respects max_backups limit", "[logger][file]")
{
    auto base = fs::temp_directory_path() / fmt::format("apostol_test_maxbak_{}.log", ::getpid());
    // Cleanup
    for (int i = 1; i <= 5; ++i) {
        fs::remove(fs::path(fmt::format("{}.{}", base.string(), i)));
        fs::remove(fs::path(fmt::format("{}.{}.gz", base.string(), i)));
    }
    fs::remove(base);

    {
        Logger logger;
        // max_backups=2, compress=false
        logger.add_target(std::make_unique<FileTarget>(base.string(), 50, 2, false));
        logger.set_level(LogLevel::debug);

        // Write enough to trigger multiple rotations
        for (int i = 0; i < 60; ++i)
            logger.info("maxbak line {}", i);

        logger.flush();
    }

    auto f1 = fs::path(fmt::format("{}.1", base.string()));
    auto f2 = fs::path(fmt::format("{}.2", base.string()));
    auto f3 = fs::path(fmt::format("{}.3", base.string()));

    // .1 and .2 should exist, .3 should NOT (max_backups=2)
    CHECK(fs::exists(f1));
    CHECK(fs::exists(f2));
    CHECK(!fs::exists(f3));

    // Cleanup
    fs::remove(base);
    fs::remove(f1);
    fs::remove(f2);
}
