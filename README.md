[![ru](https://img.shields.io/badge/lang-ru-green.svg)](README.ru-RU.md)

<img width="1422" height="506" alt="apostol-v2" src="https://github.com/user-attachments/assets/90464f33-bfde-459b-8b29-162f4f4f1bc3" />

# Apostol (A-POST-OL)

**Apostol** is a template project (demo build) for the [libapostol](https://github.com/apostoldevel/libapostol) C++20 framework -- a high-performance platform for backend applications and system services on Linux with direct access to PostgreSQL.

> A-POST-OL -> Asynchronous POST Orchestration Loop -- a single event loop for HTTP and PostgreSQL.

---

## Table of Contents

- [Overview](#overview)
- [Benchmark](#benchmark)
- [Modules](#modules)
    - [Modules in this build](#this-build-includes-three-modules)
    - [Additional modules](#additional-modules)
- [Projects](#projects)
- [Architecture](#architecture)
- [Quick start with Docker](#quick-start-with-docker)
- [Build from source](#build-from-source)
    - [Dependencies](#dependencies-debianubuntu)
    - [Clone and build](#clone-and-build)
    - [Install](#install)
    - [Minimal build](#minimal-build-no-postgresql-no-ssl-no-libcurl)
- [Configuration](#configuration)
- [Running](#running)
- [Process control](#process-control)
- [Creating your own project](#creating-your-own-project)
- [Links](#links)
- [License](#license)

---

## Overview

Apostol is written in C++20 using an asynchronous programming model based on the [epoll API](https://man7.org/linux/man-pages/man7/epoll.7.html), with direct access to [PostgreSQL](https://www.postgresql.org/) via `libpq`. The framework is targeted at high-load systems.

The key element is the built-in HTTP server running in a single event loop together with PostgreSQL connections.

**What makes it different**:

- The HTTP server and PostgreSQL sockets live in a **single event loop**.
- Data flows **directly** between the HTTP server and the database -- no intermediate scripting layers (PHP, Python, etc.).
- This minimizes latency and overhead and provides predictable performance.

**Main advantages**:

- **Autonomy** -- after building, you get a ready-to-run Linux daemon binary.
- **Speed** -- handling HTTP requests and executing SQL queries is as fast as your OS and DBMS allow.
- **Connection pool** -- built-in configurable PostgreSQL connection pool.
- **Modularity** -- applications are assembled from independent modules like building blocks.

The framework itself lives in a [separate repository](https://github.com/apostoldevel/libapostol) and is included as a git submodule.

---

## Benchmark

**Apostol v2 vs v1 vs Python vs Node.js vs Go vs Nginx** -- comparative benchmark under identical Docker conditions (wrk, 4 threads, 10s duration, 4 workers).

### /ping -- direct (keep-alive ON, 100 connections)

| Service | RPS | Latency p50 |
|---------|----:|------------:|
| Nginx (static return) | 543,000 | 111us |
| **Apostol v2** | **486,000** | **184us** |
| Go (net/http) | 212,000 | 446us |
| Apostol v1 | 126,000 | 768us |
| Node.js (Fastify) | 102,000 | 0.94ms |
| Python (FastAPI) | 2,400 | 41ms |

### /db/ping -- direct (keep-alive ON, 100 connections)

| Service | RPS | Latency p50 |
|---------|----:|------------:|
| **Apostol v2** | **109,000** | **0.93ms** |
| Go | 73,000 | 1.04ms |
| Apostol v1 | 60,000 | 1.66ms |
| Node.js | 38,000 | 2.46ms |
| Python | 2,300 | 42ms |

### /ping -- via Nginx proxy (keep-alive ON, 100 connections)

| Service | RPS | Latency p50 |
|---------|----:|------------:|
| **Apostol v2** | **60,000** | **1.76ms** |
| Go | 39,000 | 2.02ms |
| Apostol v1 | 13,000 | 5.00ms |
| Node.js | 6,600 | 11.18ms |
| Python | 5,400 | 16.01ms |

**Key findings**:
- Apostol v2 reaches **89%** of Nginx throughput on /ping (486K vs 543K)
- Apostol v2 **surpasses Nginx** on keep-alive OFF (88K vs 86K RPS) thanks to `SO_REUSEPORT`
- On /db/ping, Apostol v2 leads with 109K RPS -- **1.5x faster** than Go, **1.8x faster** than v1
- Through Nginx proxy, Apostol v2 maintains the highest throughput at 60K RPS

> Full results, methodology, and analysis: [REST API Benchmark](doc/BENCHMARK.md).

---

## Modules

The framework has a modular architecture with built-in UDP/TCP/WebSocket/HTTP servers and an asynchronous PostgreSQL client.

### This build includes three modules

| Module | Type | Description |
|--------|------|-------------|
| [PGHTTP](https://github.com/apostoldevel/module-PGHTTP/tree/version2) | Worker | HTTP to PostgreSQL function dispatcher -- implement REST API logic in PL/pgSQL |
| [WebServer](https://github.com/apostoldevel/module-WebServer/tree/version2) | Worker | Static file serving with built-in Swagger UI |
| [PGFetch](https://github.com/apostoldevel/module-PGFetch/tree/version2) | Helper | LISTEN/NOTIFY driven HTTP fetch -- send HTTP requests from PL/pgSQL |

> More details about this build: [article](https://github.com/apostoldevel/apostol/blob/version2/doc/ARTICLE.md).

### Additional modules

With [additional modules](https://github.com/apostoldevel/libapostol#modules) Apostol can be turned into:

- [**AuthServer**](https://github.com/apostoldevel/module-AuthServer) -- OAuth 2.0 authorization server;
- [**AppServer**](https://github.com/apostoldevel/module-AppServer) -- application server;
- [**MessageServer**](https://github.com/apostoldevel/process-MessageServer) -- messaging server (SMTP/FCM/API);
- [**FileServer**](https://github.com/apostoldevel/module-FileServer) -- file server;
- [**StreamServer**](https://github.com/apostoldevel/process-StreamServer) -- streaming data server;
- [**WebSocket API**](https://github.com/apostoldevel/module-WebSocketAPI) -- JSON-RPC and pub/sub via WebSocket.

For a full CRM build with all modules see [apostol-crm](https://github.com/apostoldevel/apostol-crm/tree/version2).

---

## Projects

Real-world projects built with **Apostol**:

- [CopyFrog](https://copyfrog.ai) -- AI-based platform for generating ad copy, video creatives, and marketing descriptions;
- [ChargeMeCar](https://chargemecar.com) -- central system for charging stations (OCPP);
- [Campus Caster & Campus CORS](https://cors.campusagro.com) -- NTRIP caster for real-time GNSS corrections;
- [Talking to AI](https://t.me/TalkingToAIBot) -- ChatGPT integration in a Telegram bot;
- [PlugMe](https://plugme.ru) -- CRM system for charging stations and EV owners;
- [DEBT Master](https://debt-master.ru) -- debt collection automation system;
- [Ship Safety ERP](https://ship-safety.ru) -- ERP system for shipping companies;
- [BitDeals](https://testnet.bitdeals.org) -- bitcoin payment processing service.

---

## Architecture

```
apostol/
├── src/
│   ├── app/main.cpp                  — application entry point
│   ├── lib/libapostol/               — framework (git submodule)
│   └── modules/
│       ├── Workers/                  — HTTP request handlers
│       │   ├── PGHTTP/              — git submodule
│       │   └── WebServer/           — git submodule
│       └── Helpers/                  — background tasks
│           └── PGFetch/             — git submodule
├── conf/default.json                 — configuration
├── www/                              — web files (Swagger UI, etc.)
└── docker-compose.yaml               — Docker environment
```

Each module is a separate git submodule -- add or remove modules to build exactly the application you need.

---

## Quick start with Docker

The fastest way to try Apostol -- Docker Compose with PostgreSQL, PgBouncer, and pgweb:

```shell
git clone --recursive https://github.com/apostoldevel/apostol.git -b version2
cd apostol
./docker-build.sh
docker compose up
```

After startup:
- **Swagger UI** -- http://localhost:8080
- **pgweb** (PostgreSQL web UI) -- http://localhost:8081/pgweb

> Instead of pgweb, you can use any other tool for working with PostgreSQL.
> PostgreSQL inside the container is available on port `5433`.

---

## Build from source

### Dependencies (Debian/Ubuntu)

```shell
sudo apt-get install build-essential cmake cmake-data g++ \
    libssl-dev libcurl4-openssl-dev libpq-dev zlib1g-dev
```

### Clone and build

```shell
git clone --recursive https://github.com/apostoldevel/apostol.git -b version2
cd apostol
./configure
cmake --build cmake-build-release --parallel $(nproc)
```

Or manually:

```shell
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel $(nproc)
```

### Install

```shell
sudo cmake --install cmake-build-release
```

By default, the `apostol` binary is installed into `/usr/sbin`, configuration files into `/etc/apostol/`.

### Minimal build (no PostgreSQL, no SSL, no libcurl)

```shell
./configure --without-postgresql --without-ssl --without-curl
cmake --build cmake-build-release --parallel $(nproc)
```

---

## Configuration

Default config: `conf/default.json`

After install the configuration is stored at `/etc/apostol/apostol.json`.

Key settings:

| Setting | Default | Description |
|---------|---------|-------------|
| `server.port` | 4977 | HTTP listen port |
| `server.listen` | 127.0.0.1 | Listen address |
| `server.backlog` | 4096 | TCP listen backlog |
| `daemon.enabled` | false | Run as daemon |
| `process.master` | true | Master/worker mode |
| `process.helper` | true | Enable helper process |
| `postgres.worker` | — | Worker DB connection |
| `postgres.helper` | — | Helper DB connection |

---

## Running

```shell
# Foreground
./cmake-build-release/apostol

# As a daemon
./cmake-build-release/apostol -d

# Test configuration
./cmake-build-release/apostol -t

# Systemd service (after install)
sudo systemctl start apostol
```

### systemd status example

```text
● apostol.service - Apostol
     Loaded: loaded (/lib/systemd/system/apostol.service; enabled)
     Active: active (running)
   Main PID: 461163 (apostol)
     CGroup: /system.slice/apostol.service
             ├─461163 apostol: master process /usr/sbin/apostol
             └─461164 apostol: worker process ("pq fetch", "web server")
```

## Process control

| Signal | Command | Action |
|--------|---------|--------|
| SIGTERM | `-s stop` | Fast shutdown |
| SIGQUIT | `-s quit` | Graceful shutdown |
| SIGHUP | `-s reload` | Reload config |
| SIGUSR1 | `-s reopen` | Reopen log files |

---

## Creating your own project

This repository is a starting point. To create your own application:

1. Fork or copy this repo
2. Add the modules you need as git submodules (see [available modules](https://github.com/apostoldevel/libapostol#modules))
3. Register them in `Workers.hpp` / `Helpers.hpp`
4. Customize `conf/default.json` and `main.cpp`

For automated project scaffolding see the [install script](https://github.com/apostoldevel/libapostol#quick-start) in libapostol.

---

## Links

- [libapostol](https://github.com/apostoldevel/libapostol) -- the framework
- [apostol-crm](https://github.com/apostoldevel/apostol-crm/tree/version2) -- full CRM build with all modules
- [db-platform](https://github.com/apostoldevel/db-platform) -- PostgreSQL backend framework

## License

**Apostol** is registered as computer software:
[Certificate of State Registration](https://st.fl.ru/users/al/alienufo/portfolio/f_1156277cef5ebb72.pdf) (in Russian).

[MIT](LICENSE)
