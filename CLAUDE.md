# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Apostol is a C++14 framework for building high-performance Linux server applications (daemons). It uses an asynchronous event-driven model based on `epoll` with direct PostgreSQL integration via `libpq`. The HTTP server and PostgreSQL sockets share a single event loop, eliminating intermediary layers for minimal latency.

## Build Commands

```bash
# First-time setup (clones dependencies from GitHub, generates CMake build files)
./configure                    # Release build (default)
./configure --debug            # Debug build
./configure --not-pull-github  # Skip pulling submodule repos

# Compile
cd cmake-build-release && make        # or cmake-build-debug for debug
sudo make install                     # Install binary to /usr/sbin/apostol, config to /etc/apostol/

# Test configuration without starting daemon
apostol -t

# Docker
docker-compose up --build      # Full stack: postgres + pgbouncer + pgweb + backend
./docker-build.sh              # Build image only
./docker-up.sh / ./docker-down.sh
```

## Database Setup

```bash
cd db/
./runme.sh --init       # First install: creates users, database, schemas, tables, views, routines
./runme.sh --install    # Reinstall database and objects (destructive)
./runme.sh --update     # Update routines and views only (safe)
```

Default credentials: database=`web`, user=`http`, password=`http`. SQL scripts are in `db/sql/`.

## Service Management

```bash
sudo systemctl start apostol
sudo systemctl status apostol
apostol -s stop|quit|reopen|reload   # Send signal to running master process
```

Signals: TERM/INT=fast shutdown, QUIT=graceful shutdown, HUP=reload config + restart workers, WINCH=graceful worker shutdown.

## Architecture

**Process model:** Master process supervises worker processes (handle HTTP/DB requests) and a helper process (background tasks like PGFetch).

**Source layout:**
- `src/app/` — Main application entry point (`CApostol` extends `CApplication`)
- `src/core/` — Framework core: HTTP server, PostgreSQL client, config, logging, process management, crypto/JWT
- `src/lib/delphi/` — Custom C++ utility library ("Delphi classes for C++") providing base abstractions
- `src/lib/` — Header-only libs: `rapidxml`, `picojson`, `jwt-cpp`
- `src/common/` — Shared utilities
- `src/modules/` — Plugin modules (the extensibility system)
  - `Workers/` — Long-lived: `WebServer` (HTTP), `PGHTTP` (PostgreSQL HTTP bridge)
  - `Helpers/` — Short-lived: `PGFetch` (HTTP requests from PL/pgSQL)
- `src/processes/` — Process definitions
- `conf/` — Configuration files (INI format for main config, JSON for OAuth2/sites)
- `db/sql/` — PostgreSQL schema, tables, routines
- `www/` — Static web content, Swagger UI, OpenAPI spec (`api.yaml`)

**Module system:** Modules inherit from `CModule`. Register new modules in `src/modules/Modules.hpp` (includes `Workers.hpp` and `Helpers.hpp`). Each module type has its own aggregator header.

**Dependencies are fetched by `./configure`** which clones from GitHub:
- `src/lib/delphi/` ← ufocomp/libdelphi
- `src/core/` ← apostoldevel/apostol-core
- `src/common/` ← apostoldevel/apostol-common
- `src/modules/Workers/WebServer/` ← apostoldevel/module-WebServer
- `src/modules/Workers/PGHTTP/` ← apostoldevel/module-PGHTTP
- `src/modules/Helpers/PGFetch/` ← apostoldevel/module-PGFetch
- `db/sql/http/` ← apostoldevel/db-http

## Key Conventions

- **Class naming:** `C` prefix (e.g., `CApostol`, `CConfig`, `CModule`)
- **Namespaces:** `Apostol::Application`, `Apostol::Module`, `Delphi`
- **Header guards:** `#ifndef APOSTOL_MODULENAME_HPP`
- **C++ standard:** C++14, statically linked libgcc/libstdc++
- **Config format:** INI sections in `conf/default.conf` (installed as `apostol.conf`), env var substitution supported in Docker

## CMake Build Options

| Option | Default | Purpose |
|--------|---------|---------|
| `INSTALL_AS_ROOT` | ON | Install to `/usr/sbin` + systemd service |
| `WITH_POSTGRESQL` | ON | PostgreSQL support (libpq) |
| `WITH_SSL` | ON | OpenSSL support |
| `WITH_CURL` | ON | cURL for outbound HTTP |
| `WITH_SQLITE3` | OFF | SQLite support |

## Docker Stack

`docker-compose.yaml` runs 4 services on two isolated networks (lan-network, db-network):
- **postgres** (port 5433 → 5432) — PostgreSQL with auto-initialized schema
- **pgbouncer** (port 6432) — Connection pooler
- **pgweb** (port 8081) — Web database explorer
- **backend** (port 8080) — Apostol application

Environment configured via `.env` (app) and `docker/{service}/.env` files.