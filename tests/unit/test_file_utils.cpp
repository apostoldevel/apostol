#include <catch2/catch_test_macros.hpp>

#include "apostol/file_utils.hpp"

#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

static const fs::path test_dir = fs::temp_directory_path() / "apostol_test_file_utils";

// RAII cleanup
struct TestDirGuard {
    TestDirGuard() { fs::create_directories(test_dir); }
    ~TestDirGuard() { fs::remove_all(test_dir); }
};

TEST_CASE("file_utils: create_directories", "[file_utils]")
{
    TestDirGuard guard;

    auto nested = test_dir / "a" / "b" / "c";
    REQUIRE(apostol::create_directories(nested));
    REQUIRE(fs::is_directory(nested));
}

TEST_CASE("file_utils: write_file + read back", "[file_utils]")
{
    TestDirGuard guard;

    auto path = test_dir / "subdir" / "hello.txt";
    std::string data = "Hello, Apostol v2!";

    REQUIRE(apostol::write_file(path, data));
    REQUIRE(fs::exists(path));

    // Read back
    std::ifstream f(path, std::ios::binary);
    REQUIRE(f.is_open());
    std::string content((std::istreambuf_iterator<char>(f)), {});
    REQUIRE(content == data);
}

TEST_CASE("file_utils: write_file binary data", "[file_utils]")
{
    TestDirGuard guard;

    auto path = test_dir / "binary.bin";
    std::string data;
    data.resize(256);
    for (int i = 0; i < 256; ++i)
        data[static_cast<std::size_t>(i)] = static_cast<char>(i);

    REQUIRE(apostol::write_file(path, data));

    std::ifstream f(path, std::ios::binary);
    std::string content((std::istreambuf_iterator<char>(f)), {});
    REQUIRE(content == data);
}

TEST_CASE("file_utils: delete_file", "[file_utils]")
{
    TestDirGuard guard;

    auto path = test_dir / "to_delete.txt";
    apostol::write_file(path, "data");
    REQUIRE(fs::exists(path));

    apostol::delete_file(path);
    REQUIRE_FALSE(fs::exists(path));

    // Deleting non-existent file should not throw
    REQUIRE_NOTHROW(apostol::delete_file(path));
}

TEST_CASE("file_utils: sha256_hex", "[file_utils]")
{
#ifdef WITH_SSL
    // Known test vector: SHA256("") = e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
    REQUIRE(apostol::sha256_hex("") ==
            "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");

    // SHA256("hello") = 2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824
    REQUIRE(apostol::sha256_hex("hello") ==
            "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824");

    // SHA256("abc") = ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad
    REQUIRE(apostol::sha256_hex("abc") ==
            "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
#else
    // Without SSL, sha256_hex returns empty
    REQUIRE(apostol::sha256_hex("hello").empty());
#endif
}

TEST_CASE("file_utils: file_mime_type", "[file_utils]")
{
    REQUIRE(apostol::file_mime_type(".html") == "text/html; charset=utf-8");
    REQUIRE(apostol::file_mime_type(".pdf") == "application/pdf");
    REQUIRE(apostol::file_mime_type(".json") == "application/json");
    REQUIRE(apostol::file_mime_type(".png") == "image/png");
    REQUIRE(apostol::file_mime_type(".unknown") == "application/octet-stream");
    REQUIRE(apostol::file_mime_type("") == "application/octet-stream");
    REQUIRE(apostol::file_mime_type(".csv") == "text/csv");
}

TEST_CASE("file_utils: is_safe_path", "[file_utils]")
{
    REQUIRE(apostol::is_safe_path("/some/path/file.txt"));
    REQUIRE(apostol::is_safe_path("relative/path"));
    REQUIRE(apostol::is_safe_path("file.txt"));

    // Unsafe paths
    REQUIRE_FALSE(apostol::is_safe_path(""));
    REQUIRE_FALSE(apostol::is_safe_path("../etc/passwd"));
    REQUIRE_FALSE(apostol::is_safe_path("/path/../secret"));
    REQUIRE_FALSE(apostol::is_safe_path(std::string_view("has\0null", 8)));
}
