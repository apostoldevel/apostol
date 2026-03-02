# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Apostol is a **v2 template project** (demo build) for the [libapostol](https://github.com/apostoldevel/libapostol) C++20 framework — a high-performance asynchronous backend for Linux with direct PostgreSQL access via a single epoll event loop. This is an **Apostol alone** project (no db-platform / no CRM layer).

## Build Commands

```bash
# Configure (updates submodules automatically)
./configure              # release → cmake-build-release/
./configure --debug      # debug   → cmake-build-debug/

# Build
cmake --build cmake-build-release --parallel $(nproc)
cmake --build cmake-build-debug --parallel $(nproc)

# Install (root — binary to /usr/sbin, config to /etc/apostol/)
sudo cmake --install cmake-build-release

# Minimal build (no PostgreSQL, no SSL, no libcurl)
./configure --without-postgresql --without-ssl --without-curl

# clang-tidy
cmake -B build -DENABLE_CLANG_TIDY=ON && cmake --build build
```

No linter config beyond optional clang-tidy.

## Tests

32 unit tests on Catch2 v3.7.1 covering the full libapostol framework (HTTP, WebSocket, TCP/UDP, PostgreSQL, JWT, modules, process model, etc.). Tests are built by default (`WITH_TEST=ON`).

```bash
# Build and run tests (debug)
./configure --debug
cmake --build cmake-build-debug --parallel $(nproc)
ctest --test-dir cmake-build-debug -V

# Build without tests
./configure --debug -- -DWITH_TEST=OFF

# Run specific test tag
./cmake-build-debug/apostol_tests "[http]"
```

Some tests require a local PostgreSQL server (`dbname=web user=http password=http`). Use `[integration]` tag to run only integration tests.

## Run Locally

```bash
# Change "prefix" to "." in conf/default.json first
mkdir -p logs
./cmake-build-debug/apostol -p . -c conf/default.json

# Endpoints:
#   http://localhost:4977/api/v1/ping   (via PGHTTP, needs PG)
#   http://localhost:4977/healthz       (built-in heartbeat)
#   http://localhost:4977/docs          (Swagger UI)
```

## Docker

```bash
./docker-build.sh                  # build image
docker compose up                  # full stack: postgres + pgbouncer + pgweb + backend
# Swagger UI: http://localhost:8080
# pgweb:      http://localhost:8081/pgweb
# PostgreSQL: localhost:5433
```

## Database

```bash
cd db/
./runme.sh --update    # safe: routines/views only
./runme.sh --init      # DESTRUCTIVE: first-time setup (users + database + schema)
./runme.sh --install   # DESTRUCTIVE: drop/recreate database
```

DB has a minimal `http` schema only (no db-platform). Config in `db/sql/.env.psql`.

## Architecture

**Process model:** Master → N Workers (HTTP) + 1 Helper (background tasks).

**Request flow:** HTTP request → Worker process → Module chain → PostgreSQL (via libpq in same event loop) → HTTP response.

**Key source files:**

| File | Role |
|------|------|
| `src/app/main.cpp` | Entry point, `ApostolApp` class, `HeartbeatModule`, worker/helper lifecycle |
| `src/modules/Workers/Workers.hpp` | Worker module registration (`create_workers()`) |
| `src/modules/Helpers/Helpers.hpp` | Helper module registration (`create_helpers()`) |
| `src/modules/Modules.hpp` | Combines Workers.hpp + Helpers.hpp |
| `conf/default.json` | All runtime configuration (server, modules, postgres pool) |
| `CMakeLists.txt` | Build config, feature flags, APP_* compile definitions |

**Module types:**
- **Workers** (`src/modules/Workers/`) — handle HTTP/WebSocket requests, run in worker processes
- **Helpers** (`src/modules/Helpers/`) — persistent background tasks, run in helper process

**Included modules (all git submodules):**
- `PGHTTP` — dispatches HTTP to PL/pgSQL functions (Worker, requires `WITH_POSTGRESQL`)
- `WebServer` — static file serving + Swagger UI (Worker, always last in chain as fallback)
- `PGFetch` — LISTEN/NOTIFY → outgoing HTTP requests (Helper, requires `WITH_POSTGRESQL`)

## Adding Modules

1. Add submodule: `git submodule add <url> src/modules/Workers/<Name>/` (or `Helpers/`)
2. Include the header in `Workers.hpp` or `Helpers.hpp`
3. Add instantiation in `create_workers()` or `create_helpers()`
4. Enable in `conf/default.json` under `"module": { "<Name>": { "enable": true } }`
5. CMake auto-discovers new `.cpp` files via `GLOB_RECURSE CONFIGURE_DEPENDS`

## Feature Flags

| CMake flag | Default | Controls |
|------------|---------|----------|
| `WITH_POSTGRESQL` | ON | libpq, PgPool, PGHTTP, PGFetch |
| `WITH_SSL` | ON | TLS, JWT, HTTPS |
| `WITH_CURL` | ON | FetchClient uses libcurl backend |
| `WITH_TEST` | ON | Unit tests (Catch2 via FetchContent) |

Guards in code: `#ifdef WITH_POSTGRESQL` / `#ifdef WITH_SSL` / `#ifdef WITH_CURL`.

## Configuration

`conf/default.json` — JSON format (v2). Key sections: `server`, `daemon`, `process`, `workers`, `module`, `postgres`. Module enable/disable: `"module": { "ModName": { "enable": true/false } }`.

Docker config override: `docker/conf/default.json` (listen `0.0.0.0:8080`, 4 workers, pgbouncer on port 6432).

## Process Control (daemon mode)

| Signal | CLI flag | Action |
|--------|----------|--------|
| SIGTERM | `-s stop` | Fast shutdown |
| SIGQUIT | `-s quit` | Graceful shutdown |
| SIGHUP | `-s reload` | Reload config |
| SIGUSR1 | `-s reopen` | Reopen log files |

## Code Conventions

- C++20, namespace `apostol`, no threads (single event loop per process)
- Framework types: `HttpRequest`, `HttpResponse`, `EventLoop`, `Application`, `Module`
- Formatting: `fmt::format()` (not printf/CString::Format)
- Memory: RAII, `std::unique_ptr` for module ownership
- Strings: `std::string`, `std::string_view`
- Compiler warnings: `-Wall -Wextra -Wpedantic -Wno-unused-parameter`
