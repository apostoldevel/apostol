[Русская версия](BENCHMARK.ru-RU.md)

# REST API Benchmark: Apostol v2 vs v1 vs Python vs Node.js vs Go

## Context

[Apostol](https://github.com/apostoldevel/apostol) is a C++ framework for building high-performance Linux server applications. Version 1 was built on C++14 with the libdelphi library. Version 2 is a complete rewrite on **C++20** using the [libapostol](https://github.com/apostoldevel/libapostol) framework -- standard library, `{fmt}`, `nlohmann/json`, `llhttp`, and zero legacy dependencies.

Both versions share the same architecture: asynchronous event-driven model based on `epoll` with direct PostgreSQL integration via `libpq`. The HTTP server and PostgreSQL sockets share a single event loop per worker, eliminating intermediary layers.

This benchmark compares six services under identical conditions:

| Version | Service | Stack |
|---------|---------|-------|
| v0 | Nginx (baseline) | Static `return 200` directives |
| v1 | Apostol v1 | C++14, libdelphi, epoll, libpq async |
| v2 | Python | FastAPI + uvicorn + asyncpg |
| v3 | Node.js | Fastify + pg + PM2 (cluster mode) |
| v4 | Go | net/http + pgx/v5 pool |
| v5 | **Apostol v2** | **C++20, libapostol, epoll, libpq async** |

---

## Test Environment

### Docker Topology

All services run in separate Docker containers on the same host, orchestrated by a single `docker-compose.yaml`. Two isolated networks:

- **bench-lan** (172.19.1.0/24) -- application layer (Nginx, application servers, loadgen)
- **bench-db** (172.19.2.0/24) -- database layer (PostgreSQL, PgBouncer, application servers)

### Infrastructure

**PostgreSQL 17** -- optimized for benchmark throughput (not production safety):

```
fsync = off
synchronous_commit = off
full_page_writes = off
wal_level = minimal
shared_buffers = 256MB
work_mem = 16MB
max_connections = 200
```

**PgBouncer** -- session pooling mode, `default_pool_size=50`, `max_client_conn=500`.

**Nginx** -- serves as the v0 baseline (static JSON responses) and an API gateway for proxy tests. `worker_processes auto`, `listen 80 reuseport`, `keepalive_requests 10000`, access logging disabled.

### Connection Pools

All application services use identical pool parameters: **min=5, max=15** per worker.

With 4 workers:

| Service | Pool Model | Total DB Connections |
|---------|-----------|---------------------|
| Apostol v1/v2 | Per-worker | 15 x 4 = 60 |
| Python (uvicorn) | Per-worker | 15 x 4 = 60 |
| Node.js (PM2) | Per-worker | 15 x 4 = 60 |
| Go | Single shared pool | 15 (always) |

Go's goroutine scheduler multiplexes all requests across a single pool regardless of `GOMAXPROCS`.

### Endpoints

Each service implements identical endpoints returning JSON with `Content-Type: application/json; charset=utf-8`:

| Endpoint | Type | Response |
|----------|------|----------|
| `/api/vN/ping` | Static | `{"ok":true,"message":"OK"}` |
| `/api/vN/time` | Compute | `{"serverTime":<unix_epoch_seconds>}` |
| `/api/vN/db/ping` | DB query | `SELECT json_build_object('ok', true, 'message', 'OK')::text` |
| `/api/vN/db/time` | DB query | `SELECT json_build_object('serverTime', extract(epoch from now())::integer)::text` |

---

## Tuning for Fair Comparison

Before running the benchmark, we equalized critical kernel-level parameters across all services to ensure a fair comparison.

### TCP Listen Backlog

The `listen()` backlog determines how many pending TCP connections the kernel queues before rejecting new ones. Under high concurrency (e.g., `wrk -c1000` with `Connection: close`), a small backlog causes SYN retransmissions and artificially limits throughput.

Default values varied widely across services:

| Service | Default Backlog | Benchmark Setting |
|---------|----------------|-------------------|
| Nginx | 511 | **4096** (`listen 80 backlog=4096 reuseport`) |
| Apostol v1 | 4096 (`SOMAXCONN`) | 4096 (no change) |
| Apostol v2 | 4096 | 4096 (no change) |
| Go | 4096 (`SOMAXCONN`) | 4096 (no change) |
| Node.js | 511 | **4096** (`backlog` in `fastify.listen()`) |
| Python | 2048 | **4096** (`--backlog 4096` in uvicorn) |

All services now use `backlog=4096`, matching the kernel's `net.core.somaxconn`.

### SO_REUSEPORT

`SO_REUSEPORT` allows multiple sockets to bind to the same port. The kernel distributes incoming connections across these sockets, eliminating the single-socket accept bottleneck under high concurrency.

| Service | SO_REUSEPORT | Source |
|---------|:------------:|--------|
| Nginx | Yes | `listen 80 reuseport` |
| Apostol v2 | Yes | `setsockopt(SO_REUSEPORT)` in libapostol |
| Apostol v1 | No | Single accept socket |
| Go | No | Go runtime default |
| Node.js | No | PM2 cluster mode (round-robin) |
| Python | No | uvicorn default |

Both Nginx and Apostol v2 use `SO_REUSEPORT`, ensuring a level playing field on connection handling tests.

### epoll Event Array Size

The `epoll_wait()` `maxevents` parameter controls how many I/O events the kernel delivers per syscall. A small value means more syscalls under load.

| Service | Event Array Size | Source |
|---------|-----------------|--------|
| Nginx | 512 | `epoll_events` directive (default) |
| Apostol v1 | 512 | `EVENT_SIZE` in Sockets.cpp |
| Apostol v2 | 512 | `MAX_EVENTS` in event_loop.hpp |
| Go | 128 | Go runtime (hardcoded) |
| Node.js | 1024 | libuv (hardcoded) |

---

## Methodology

### Load Generator

**wrk** with the following parameters:

- 4 threads (`-t4`)
- 100 or 1000 concurrent connections (`-c100` / `-c1000`)
- 10 second duration (`-d10s`)
- Latency histogram enabled (`--latency`)
- Keep-alive ON (wrk default) and OFF (`-H "Connection: close"`)

### Test Procedure

1. All service containers start and reach healthy state.
2. **Direct tests:** the load generator communicates with each service directly (no proxy).
3. **Proxy tests:** the load generator communicates through Nginx, which proxies requests to each service. Nginx uses `Connection: close` to the upstream, meaning each proxied request creates a new TCP connection to the backend.
4. Services are tested sequentially to avoid resource contention.
5. Each endpoint is tested for each service with a cooldown between runs.

---

## Results: Direct

**Configuration:** 4 workers, wrk with `--latency`, 4 threads, 10 second duration, direct connection (no proxy).

### /ping -- Keep-alive ON

Pure HTTP stack performance. Keep-alive eliminates TCP handshake overhead, isolating request processing.

**4 threads / 100 connections:**

| Service | RPS | Avg Latency | p50 | p99 |
|---------|----:|------------:|----:|----:|
| Nginx (v0) | 566,343 | 127us | 111us | 455us |
| **Apostol v2 (v5)** | **506,814** | **191us** | **170us** | **485us** |
| Go (v4) | 211,121 | 519us | 447us | 1.76ms |
| Apostol v1 (v1) | 127,652 | 787us | 790us | 1.16ms |
| Node.js (v3) | 101,509 | 1.03ms | 0.95ms | 2.19ms |
| Python (v2) | 2,399 | 41.29ms | 41.03ms | 43.12ms |

```
Nginx     ████████████████████████████████████████████████████████  566K
Apostol2  ██████████████████████████████████████████████████        507K
Go        █████████████████████                                    211K
Apostol1  █████████████                                            128K
Node.js   ██████████                                               102K
Python    ▏                                                          2K
```

**4 threads / 1000 connections:**

| Service | RPS | Avg Latency | p50 | p99 |
|---------|----:|------------:|----:|----:|
| Nginx | 595,692 | 1.74ms | 1.72ms | 5.68ms |
| **Apostol v2** | **479,977** | **1.98ms** | **1.81ms** | **4.06ms** |
| Go | 204,109 | 4.88ms | 4.81ms | 8.23ms |
| Apostol v1 | 119,149 | 8.37ms | 8.40ms | 11.60ms |
| Node.js | 100,895 | 24.52ms | 9.43ms | 582.48ms |
| Python | 21,452 | 46.26ms | 44.89ms | 59.96ms |

At 1000 connections, Apostol v2 maintains 480K RPS with p99 = 4.06ms. For comparison: Node.js degrades to p99 = 582ms.

### /ping -- Keep-alive OFF

Disabling keep-alive (`Connection: close`) forces a new TCP connection per request. This tests raw connection handling efficiency.

**4 threads / 100 connections:**

| Service | RPS | Avg Latency | p50 | p99 |
|---------|----:|------------:|----:|----:|
| Nginx | 84,442 | 428us | 365us | 1.18ms |
| **Apostol v2** | **84,262** | **339us** | **318us** | **821us** |
| Go | 41,305 | 1.27ms | 772us | 9.14ms |
| Apostol v1 | 8,013 | 5.56ms | 3.70ms | 18.28ms |
| Node.js | 5,873 | 8.36ms | 6.41ms | 23.70ms |
| Python | 5,841 | 9.17ms | 8.53ms | 21.37ms |

**4 threads / 1000 connections:**

| Service | RPS | Avg Latency | p50 | p99 |
|---------|----:|------------:|----:|----:|
| **Apostol v2** | **81,246** | **2.90ms** | **2.44ms** | **7.46ms** |
| Go | 79,670 | 6.84ms | 6.80ms | 16.20ms |
| Nginx | 74,786 | 3.92ms | 3.08ms | 11.49ms |
| Apostol v1 | 31,333 | 14.53ms | 14.06ms | 36.51ms |
| Node.js | 12,636 | 33.17ms | 32.24ms | 81.53ms |
| Python | 7,115 | 75.64ms | 73.04ms | 166.48ms |

Without keep-alive at c100, Nginx and Apostol v2 deliver **nearly identical throughput** (84.4K vs 84.3K) -- both use `SO_REUSEPORT` for kernel-level connection distribution. However, Apostol v2 achieves lower latency (339us avg vs 428us). At c1000, Apostol v2 pulls ahead (81.2K vs 74.8K), suggesting its event loop handles connection churn more efficiently under extreme concurrency.

### /db/ping -- Keep-alive ON

Database round-trip test with keep-alive enabled. This is the production-relevant scenario.

**4 threads / 100 connections:**

| Service | RPS | Avg Latency | p50 | p99 |
|---------|----:|------------:|----:|----:|
| **Apostol v2** | **112,097** | **0.89ms** | **0.91ms** | **1.37ms** |
| Go | 72,004 | 1.93ms | 1.07ms | 13.53ms |
| Apostol v1 | 60,797 | 1.76ms | 1.61ms | 2.48ms |
| Node.js | 35,521 | 2.94ms | 2.65ms | 7.37ms |
| Python | 2,293 | 46.82ms | 42.00ms | 244.29ms |

Apostol v2 leads with 112K RPS -- **1.6x faster** than Go (72K) and **1.8x faster** than Apostol v1 (61K). The advantage comes from the tight integration between epoll and libpq: HTTP parsing, PostgreSQL query dispatch, and response building all happen in a single event loop iteration with minimal overhead.

### /db/ping -- Keep-alive OFF

**4 threads / 100 connections:**

| Service | RPS | Avg Latency | p50 | p99 |
|---------|----:|------------:|----:|----:|
| **Apostol v2** | **59,713** | **1.31ms** | **0.87ms** | **4.52ms** |
| Go | 11,871 | 4.58ms | 1.88ms | 18.40ms |
| Node.js | 5,251 | 10.13ms | 7.24ms | 26.99ms |
| Python | 5,158 | 11.49ms | 10.84ms | 24.96ms |
| Apostol v1 | 2,797 | 6.03ms | 2.93ms | 19.57ms |

Apostol v2 achieves 60K RPS -- **5.0x faster** than Go and **21.3x faster** than Apostol v1 -- with consistently low latency (p99 = 4.52ms). The combination of `SO_REUSEPORT` and tight event loop integration allows v2 to handle TCP teardown and database round-trip concurrently without latency spikes.

---

## Results: via Nginx Proxy

All proxy tests route traffic through a shared Nginx instance: `loadgen -> Nginx -> service`. The Nginx proxy configuration uses `proxy_set_header Connection "close"`, meaning each proxied request creates a **new TCP connection** to the upstream service. This configuration adds significant overhead but represents a common production deployment pattern.

### /ping via Proxy -- Keep-alive ON

**4 threads / 100 connections:**

| Service | RPS | Avg Latency | p50 | p99 |
|---------|----:|------------:|----:|----:|
| **Apostol v2** | **69,682** | **1.76ms** | **1.07ms** | **5.64ms** |
| Go | 38,370 | 5.77ms | 2.06ms | 43.24ms |
| Apostol v1 | 16,444 | 11.74ms | 3.18ms | 54.16ms |
| Node.js | 5,889 | 21.49ms | 14.29ms | 73.65ms |
| Python | 5,473 | 22.80ms | 14.62ms | 83.66ms |

**4 threads / 1000 connections:**

| Service | RPS | Avg Latency | p50 | p99 |
|---------|----:|------------:|----:|----:|
| **Apostol v2** | **39,090** | **34.92ms** | **20.71ms** | **141.98ms** |
| Go | 21,790 | 61.86ms | 37.46ms | 261.89ms |
| Apostol v1 | 14,621 | 79.48ms | 56.99ms | 274.02ms |
| Node.js | 9,662 | 111.68ms | 102.08ms | 338.13ms |
| Python | 8,809 | 119.80ms | 111.04ms | 314.21ms |

### /ping via Proxy -- Keep-alive OFF

**4 threads / 100 connections:**

| Service | RPS | Avg Latency | p50 | p99 |
|---------|----:|------------:|----:|----:|
| **Apostol v2** | **29,310** | **3.07ms** | **1.22ms** | **22.43ms** |
| Apostol v1 | 5,746 | 24.98ms | 4.93ms | 139.92ms |
| Go | 3,503 | 13.93ms | 11.41ms | 42.03ms |
| Node.js | 3,174 | 15.82ms | 14.18ms | 41.23ms |
| Python | 2,659 | 28.34ms | 15.67ms | 134.27ms |

With keep-alive OFF, each request requires two TCP handshakes (client->Nginx and Nginx->upstream). Apostol v2 maintains a commanding lead at 29.3K RPS -- **5.1x faster** than Apostol v1 and **8.4x faster** than Go.

### /db/ping via Proxy -- Keep-alive ON

**4 threads / 100 connections:**

| Service | RPS | Avg Latency | p50 | p99 |
|---------|----:|------------:|----:|----:|
| **Apostol v2** | **35,382** | **2.83ms** | **1.54ms** | **6.99ms** |
| Go | 13,593 | 12.32ms | 5.21ms | 53.13ms |
| Node.js | 5,337 | 28.34ms | 10.91ms | 108.10ms |
| Python | 4,376 | 31.72ms | 16.11ms | 206.63ms |
| Apostol v1 | 3,140 | 54.71ms | 23.21ms | 293.82ms |

Apostol v2 leads with 35.4K RPS -- **2.6x faster** than Go (13.6K) with consistently lower latency (p99 = 6.99ms vs 53.13ms).

### /db/ping via Proxy -- Keep-alive OFF

**4 threads / 100 connections:**

| Service | RPS | Avg Latency | p50 | p99 |
|---------|----:|------------:|----:|----:|
| **Apostol v2** | **28,428** | **3.30ms** | **1.67ms** | **24.92ms** |
| Apostol v1 | 4,639 | 18.38ms | 10.64ms | 88.35ms |
| Go | 3,255 | 14.85ms | 14.52ms | 35.23ms |
| Node.js | 2,652 | 21.53ms | 18.03ms | 101.76ms |
| Python | 2,404 | 28.18ms | 20.17ms | 124.76ms |

With both proxy and database overhead under connection churn, Apostol v2 achieves 28.4K RPS -- **8.7x faster** than Go and **6.1x faster** than Apostol v1.

---

## Analysis

### Apostol v2 vs Nginx

On `/ping` with keep-alive ON, Apostol v2 reaches **90% of Nginx's throughput** (507K vs 566K). Given that Nginx serves static `return 200` directives with no application logic, this is a remarkable result for a full application server.

On `/ping` with keep-alive OFF, both services use `SO_REUSEPORT` and deliver **nearly identical throughput** at c100: 84.3K vs 84.4K. The difference is within measurement noise. However, Apostol v2 achieves **lower latency** (339us avg vs 428us, p50 = 318us vs 365us). At c1000, Apostol v2 pulls ahead on throughput as well (81.2K vs 74.8K), indicating more efficient event loop handling under extreme connection churn.

### Apostol v2 vs Apostol v1

Both versions use the same architectural pattern (single-threaded epoll event loop per worker), but v2 delivers dramatically higher throughput:

| Test | v2 | v1 | Speedup |
|------|---:|---:|--------:|
| /ping KA-ON c100 | 506,814 | 127,652 | **4.0x** |
| /ping KA-ON c1000 | 479,977 | 119,149 | **4.0x** |
| /ping KA-OFF c100 | 84,262 | 8,013 | **10.5x** |
| /db/ping KA-ON c100 | 112,097 | 60,797 | **1.8x** |
| /db/ping KA-OFF c100 | 59,713 | 2,797 | **21.3x** |

The 4.0x improvement on `/ping` keep-alive ON comes from:

1. **llhttp** parser vs libdelphi's hand-written parser -- llhttp is a highly optimized state machine.
2. **C++20 standard library** -- `std::string_view`, move semantics, and zero-copy patterns eliminate unnecessary allocations.
3. **Minimal response path** -- v2's response builder writes directly to the socket buffer with no intermediate object construction.
4. **Tuned epoll** -- `MAX_EVENTS=512` delivers more events per syscall.

The 10.5x improvement on `/ping` keep-alive OFF additionally benefits from:

5. **`SO_REUSEPORT`** -- kernel-level load balancing of incoming connections across workers, vs v1's single accept socket.
6. **backlog=4096** -- v1 already used `SOMAXCONN` (4096), but v2 was at 128 before tuning.

### Apostol v2 vs Go

Apostol v2 outperforms Go across every direct test:

| Test | Apostol v2 | Go | Ratio |
|------|---:|---:|------:|
| /ping KA-ON c100 | 506,814 | 211,121 | **2.4x** |
| /ping KA-OFF c100 | 84,262 | 41,305 | **2.0x** |
| /db/ping KA-ON c100 | 112,097 | 72,004 | **1.6x** |
| /db/ping KA-OFF c100 | 59,713 | 11,871 | **5.0x** |

The advantage narrows on database tests with keep-alive (1.6x vs 2.4x), confirming that the PostgreSQL round-trip becomes the dominant cost. On `/db/ping` without keep-alive, v2's advantage widens to 5.0x thanks to `SO_REUSEPORT` and efficient TCP teardown handling -- Go does not use `SO_REUSEPORT` by default.

### Proxy Overhead

The Nginx proxy adds significant overhead due to `Connection: close` on the upstream link, which forces a new TCP connection per proxied request:

| Service | Direct RPS | Proxy RPS | Retained |
|---------|---:|---:|---:|
| Apostol v2 | 506,814 | 69,682 | 14% |
| Go | 211,121 | 38,370 | 18% |
| Apostol v1 | 127,652 | 16,444 | 13% |
| Node.js | 101,509 | 5,889 | 6% |

The absolute proxy throughput ranking mirrors direct performance: Apostol v2 leads at 69.7K, followed by Go at 38.4K. Despite retaining only 14% of its direct throughput, Apostol v2's raw speed advantage is large enough that it still outperforms every other service through the proxy.

On `/db/ping` via proxy, Apostol v2 leads with 35.4K RPS vs Go's 13.6K, with dramatically better latency consistency (p99 = 7ms vs 53ms). With keep-alive OFF, Apostol v2 maintains the lead at 28.4K vs 3.3K.

### Keep-alive Impact

| Service | KA-ON (c100) | KA-OFF (c100) | Drop Factor |
|---------|-------------:|---------------:|------------:|
| **Apostol v2** | **506,814** | **84,262** | **6.0x** |
| Nginx | 566,343 | 84,442 | 6.7x |
| Go | 211,121 | 41,305 | 5.1x |
| Apostol v1 | 127,652 | 8,013 | 15.9x |
| Node.js | 101,509 | 5,873 | 17.3x |
| Python | 2,399 | 5,841 | +143% |

Go has the **smallest drop factor** (5.1x), followed by Apostol v2 (6.0x) and Nginx (6.7x) -- all three use efficient connection handling. Apostol v1 and Node.js drop 16-17x. Python improves without keep-alive because its runtime overhead dominates TCP costs.

### Node.js and Python

Node.js (Fastify + PM2) reaches 102K RPS on `/ping` -- competitive for a JavaScript runtime. Python (FastAPI + uvicorn) reaches 2.4K RPS at 100 connections but scales to 21K at 1000 connections as worker utilization improves.

---

## How to Reproduce

### Prerequisites

- Docker and Docker Compose
- Sufficient file descriptor limits on the host (`ulimit -n` should be at least 65536)

### Start the Stack

```bash
cd benchmark
docker compose up -d
docker compose ps  # wait for all services to reach "healthy"
```

### Run the Benchmark

```bash
# Build loadgen
docker compose --profile loadgen build loadgen

# Quick ping benchmark — direct + proxy (10s per test)
docker compose --profile loadgen run --rm loadgen /bench/scripts/bench_ping.sh

# Full benchmark suite
docker compose --profile loadgen run --rm loadgen /bench/scripts/bench.sh --quick
```

### Adjust Worker Count

```bash
WORKERS=4 docker compose up -d
```

This sets the worker/process count for all services simultaneously.

---

## Note

Results collected with 4 worker processes, 10-second test duration, `wrk --latency`. All services use `backlog=4096` and `ulimits.nofile=1048576`. Nginx and Apostol v2 use `SO_REUSEPORT`. Results may vary between runs depending on host load and thermal conditions.
