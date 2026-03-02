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

**Nginx** -- serves as the v0 baseline (static JSON responses) and an API gateway for proxy tests. `worker_processes auto`, `keepalive_requests 10000`, access logging disabled.

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

Before running the benchmark, we equalized two critical kernel-level parameters across all services to ensure a fair comparison.

### TCP Listen Backlog

The `listen()` backlog determines how many pending TCP connections the kernel queues before rejecting new ones. Under high concurrency (e.g., `wrk -c1000` with `Connection: close`), a small backlog causes SYN retransmissions and artificially limits throughput.

Default values varied widely across services:

| Service | Default Backlog | Benchmark Setting |
|---------|----------------|-------------------|
| Nginx | 511 | **4096** (`listen 80 backlog=4096`) |
| Apostol v1 | 4096 (`SOMAXCONN`) | 4096 (no change) |
| Apostol v2 | 128 | **4096** (`server.backlog` in JSON config) |
| Go | 4096 (`SOMAXCONN`) | 4096 (no change) |
| Node.js | 511 | **4096** (`backlog` in `fastify.listen()`) |
| Python | 2048 | **4096** (`--backlog 4096` in uvicorn) |

All services now use `backlog=4096`, matching the kernel's `net.core.somaxconn`.

### epoll Event Array Size

The `epoll_wait()` `maxevents` parameter controls how many I/O events the kernel delivers per syscall. A small value means more syscalls under load.

| Service | Event Array Size | Source |
|---------|-----------------|--------|
| Nginx | 512 | `epoll_events` directive (default) |
| Apostol v1 | 512 | `EVENT_SIZE` in Sockets.cpp |
| Apostol v2 | 64 -> **512** | `MAX_EVENTS` in event_loop.hpp |
| Go | 128 | Go runtime (hardcoded) |
| Node.js | 1024 | libuv (hardcoded) |

Apostol v2's `MAX_EVENTS` was increased from 64 to 512 to match v1. At 64, v2 needed **8x more syscalls** per event loop iteration under high load -- a significant overhead that masked its true performance.

These two changes -- backlog and epoll event array -- had a dramatic impact on Apostol v2's throughput, particularly in `Connection: close` scenarios.

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
| Nginx (v0) | 543,131 | 146us | 111us | 579us |
| **Apostol v2 (v5)** | **485,521** | **199us** | **184us** | **498us** |
| Go (v4) | 211,891 | 517us | 446us | 1.74ms |
| Apostol v1 (v1) | 126,096 | 790us | 768us | 1.16ms |
| Node.js (v3) | 101,891 | 1.03ms | 0.94ms | 2.27ms |
| Python (v2) | 2,418 | 41.25ms | 41.03ms | 42.99ms |

```
Nginx     ████████████████████████████████████████████████████████  543K
Apostol2  ██████████████████████████████████████████████████████     486K
Go        ██████████████████████                                    212K
Apostol1  █████████████                                             126K
Node.js   ██████████                                                102K
Python    ▏                                                           2K
```

**4 threads / 1000 connections:**

| Service | RPS | Avg Latency | p50 | p99 |
|---------|----:|------------:|----:|----:|
| Nginx | 580,145 | 1.83ms | 1.79ms | 5.85ms |
| **Apostol v2** | **515,234** | **1.87ms** | **1.72ms** | **3.69ms** |
| Go | 215,164 | 4.63ms | 4.56ms | 7.88ms |
| Apostol v1 | 117,543 | 8.46ms | 8.78ms | 12.61ms |
| Node.js | 104,400 | 21.81ms | 9.15ms | 510.49ms |
| Python | 21,904 | 45.41ms | 44.97ms | 59.94ms |

At 1000 connections, Apostol v2 maintains 515K RPS with p99 = 3.69ms -- **even lower** than its p99 at 100 connections. For comparison: Node.js degrades to p99 = 510ms.

### /ping -- Keep-alive OFF

Disabling keep-alive (`Connection: close`) forces a new TCP connection per request. This tests raw connection handling efficiency.

**4 threads / 100 connections:**

| Service | RPS | Avg Latency | p50 | p99 |
|---------|----:|------------:|----:|----:|
| **Apostol v2** | **88,327** | **330us** | **312us** | **802us** |
| Nginx | 85,693 | 428us | 367us | 1.18ms |
| Go | 38,892 | 1.31ms | 755us | 9.03ms |
| Apostol v1 | 8,069 | 5.56ms | 4.29ms | 16.89ms |
| Node.js | 6,420 | 7.58ms | 6.12ms | 19.81ms |
| Python | 6,259 | 8.55ms | 8.04ms | 20.10ms |

**4 threads / 1000 connections:**

| Service | RPS | Avg Latency | p50 | p99 |
|---------|----:|------------:|----:|----:|
| **Apostol v2** | **82,492** | **2.88ms** | **2.41ms** | **7.40ms** |
| Go | 79,118 | 6.44ms | 6.11ms | 16.32ms |
| Nginx | 76,846 | 3.97ms | 3.12ms | 11.72ms |
| Apostol v1 | 29,955 | 15.10ms | 14.65ms | 39.10ms |
| Node.js | 12,608 | 33.80ms | 33.44ms | 83.92ms |
| Python | 7,176 | 75.17ms | 72.55ms | 169.60ms |

Without keep-alive, **Apostol v2 surpasses Nginx** at both 100 and 1000 connections. At c100: 88.3K vs 85.7K. At c1000: 82.5K vs 76.8K. This is possible because Apostol v2 uses `SO_REUSEPORT`, which distributes incoming connections across worker processes at the kernel level, while Nginx serializes connection acceptance through its accept mechanism.

### /db/ping -- Keep-alive ON

Database round-trip test with keep-alive enabled. This is the production-relevant scenario.

**4 threads / 100 connections:**

| Service | RPS | Avg Latency | p50 | p99 |
|---------|----:|------------:|----:|----:|
| **Apostol v2** | **109,106** | **0.92ms** | **0.93ms** | **1.56ms** |
| Go | 73,166 | 1.82ms | 1.04ms | 12.07ms |
| Apostol v1 | 60,238 | 1.95ms | 1.66ms | 2.85ms |
| Node.js | 38,229 | 2.73ms | 2.46ms | 6.62ms |
| Python | 2,288 | 47.08ms | 42.01ms | 242.24ms |

Apostol v2 leads with 109K RPS -- **1.5x faster** than Go (73K) and **1.8x faster** than Apostol v1 (60K). The advantage comes from the tight integration between epoll and libpq: HTTP parsing, PostgreSQL query dispatch, and response building all happen in a single event loop iteration with minimal overhead.

### /db/ping -- Keep-alive OFF

**4 threads / 100 connections:**

| Service | RPS | Avg Latency | p50 | p99 |
|---------|----:|------------:|----:|----:|
| **Apostol v2** | **60,210** | **1.31ms** | **0.88ms** | **4.67ms** |
| Go | 11,315 | 4.78ms | 1.90ms | 19.49ms |
| Node.js | 5,092 | 10.43ms | 7.30ms | 29.77ms |
| Python | 4,615 | 13.05ms | 11.73ms | 39.65ms |
| Apostol v1 | 2,796 | 6.05ms | 3.01ms | 19.44ms |

Apostol v2 achieves 60K RPS -- **5.3x faster** than Go and **21.5x faster** than Apostol v1 -- with consistently low latency (p99 = 4.67ms). The combination of `SO_REUSEPORT` and tight event loop integration allows v2 to handle TCP teardown and database round-trip concurrently without latency spikes.

---

## Results: via Nginx Proxy

All proxy tests route traffic through a shared Nginx instance: `loadgen -> Nginx -> service`. The Nginx proxy configuration uses `proxy_set_header Connection "close"`, meaning each proxied request creates a **new TCP connection** to the upstream service. This configuration adds significant overhead but represents a common production deployment pattern.

### /ping via Proxy -- Keep-alive ON

**4 threads / 100 connections:**

| Service | RPS | Avg Latency | p50 | p99 |
|---------|----:|------------:|----:|----:|
| **Apostol v2** | **59,586** | **1.73ms** | **1.76ms** | **4.38ms** |
| Go | 39,360 | 5.27ms | 2.02ms | 49.14ms |
| Apostol v1 | 13,032 | 15.99ms | 5.00ms | 72.04ms |
| Node.js | 6,639 | 22.17ms | 11.18ms | 86.64ms |
| Python | 5,353 | 22.37ms | 16.01ms | 73.23ms |

**4 threads / 1000 connections:**

| Service | RPS | Avg Latency | p50 | p99 |
|---------|----:|------------:|----:|----:|
| **Apostol v2** | **34,277** | **42.58ms** | **24.36ms** | **185.68ms** |
| Go | 21,991 | 59.02ms | 38.31ms | 237.30ms |
| Apostol v1 | 15,420 | 79.10ms | 55.56ms | 285.15ms |
| Node.js | 10,873 | 98.84ms | 83.65ms | 284.41ms |
| Python | 8,406 | 121.82ms | 125.66ms | 303.27ms |

### /ping via Proxy -- Keep-alive OFF

**4 threads / 100 connections:**

| Service | RPS | Avg Latency | p50 | p99 |
|---------|----:|------------:|----:|----:|
| **Apostol v2** | **28,716** | **2.79ms** | **1.27ms** | **17.76ms** |
| Apostol v1 | 6,103 | 24.17ms | 5.11ms | 123.95ms |
| Go | 4,640 | 10.51ms | 9.34ms | 32.18ms |
| Node.js | 4,003 | 13.53ms | 11.32ms | 68.02ms |
| Python | 3,006 | 27.03ms | 14.82ms | 126.41ms |

With keep-alive OFF, each request requires two TCP handshakes (client->Nginx and Nginx->upstream). Apostol v2 maintains a commanding lead at 28.7K RPS -- **4.7x faster** than Apostol v1 and **6.2x faster** than Go.

### /db/ping via Proxy -- Keep-alive ON

**4 threads / 100 connections:**

| Service | RPS | Avg Latency | p50 | p99 |
|---------|----:|------------:|----:|----:|
| Go | 36,310 | 4.28ms | 1.37ms | 51.45ms |
| **Apostol v2** | **31,750** | **2.59ms** | **1.64ms** | **6.99ms** |
| Node.js | 5,675 | 21.52ms | 15.88ms | 69.97ms |
| Python | 5,154 | 25.37ms | 12.18ms | 205.65ms |
| Apostol v1 | 3,356 | 48.13ms | 31.20ms | 223.42ms |

Go edges ahead on raw RPS (36.3K vs 31.8K) through the proxy with database queries. However, Apostol v2 delivers dramatically better latency consistency: p99 = 6.99ms vs Go's 51.45ms.

### /db/ping via Proxy -- Keep-alive OFF

**4 threads / 100 connections:**

| Service | RPS | Avg Latency | p50 | p99 |
|---------|----:|------------:|----:|----:|
| **Apostol v2** | **27,075** | **3.17ms** | **1.78ms** | **20.01ms** |
| Apostol v1 | 4,521 | 18.43ms | 9.61ms | 96.42ms |
| Go | 3,784 | 12.85ms | 12.51ms | 31.01ms |
| Node.js | 2,980 | 21.48ms | 15.83ms | 118.42ms |
| Python | 2,544 | 27.67ms | 18.76ms | 130.32ms |

With both proxy and database overhead under connection churn, Apostol v2 achieves 27K RPS -- **7.2x faster** than Go and **6.0x faster** than Apostol v1.

---

## Analysis

### Apostol v2 vs Nginx

On `/ping` with keep-alive ON, Apostol v2 reaches **89% of Nginx's throughput** (486K vs 543K). Given that Nginx serves static `return 200` directives with no application logic, this is a remarkable result for a full application server.

On `/ping` with keep-alive OFF, Apostol v2 **surpasses Nginx** at both concurrency levels: 88.3K vs 85.7K (c100) and 82.5K vs 76.8K (c1000). The advantage comes from `SO_REUSEPORT` -- the kernel distributes incoming connections directly to worker processes, avoiding Nginx's serialized accept pattern.

### Apostol v2 vs Apostol v1

Both versions use the same architectural pattern (single-threaded epoll event loop per worker), but v2 delivers dramatically higher throughput:

| Test | v2 | v1 | Speedup |
|------|---:|---:|--------:|
| /ping KA-ON c100 | 485,521 | 126,096 | **3.9x** |
| /ping KA-ON c1000 | 515,234 | 117,543 | **4.4x** |
| /ping KA-OFF c100 | 88,327 | 8,069 | **10.9x** |
| /db/ping KA-ON c100 | 109,106 | 60,238 | **1.8x** |
| /db/ping KA-OFF c100 | 60,210 | 2,796 | **21.5x** |

The 3.9x improvement on `/ping` keep-alive ON comes from:

1. **llhttp** parser vs libdelphi's hand-written parser -- llhttp is a highly optimized state machine.
2. **C++20 standard library** -- `std::string_view`, move semantics, and zero-copy patterns eliminate unnecessary allocations.
3. **Minimal response path** -- v2's response builder writes directly to the socket buffer with no intermediate object construction.
4. **Tuned epoll** -- `MAX_EVENTS=512` delivers more events per syscall.

The 10.9x improvement on `/ping` keep-alive OFF additionally benefits from:

5. **`SO_REUSEPORT`** -- kernel-level load balancing of incoming connections across workers, vs v1's single accept socket.
6. **backlog=4096** -- v1 already used `SOMAXCONN` (4096), but v2 was at 128 before tuning.

### Apostol v2 vs Go

Apostol v2 outperforms Go across every direct test:

| Test | Apostol v2 | Go | Ratio |
|------|---:|---:|------:|
| /ping KA-ON c100 | 485,521 | 211,891 | **2.3x** |
| /ping KA-OFF c100 | 88,327 | 38,892 | **2.3x** |
| /db/ping KA-ON c100 | 109,106 | 73,166 | **1.5x** |
| /db/ping KA-OFF c100 | 60,210 | 11,315 | **5.3x** |

The advantage narrows on database tests with keep-alive (1.5x vs 2.3x), confirming that the PostgreSQL round-trip becomes the dominant cost. On `/db/ping` without keep-alive, v2's advantage widens to 5.3x thanks to `SO_REUSEPORT` and efficient TCP teardown handling.

### Proxy Overhead

The Nginx proxy adds significant overhead due to `Connection: close` on the upstream link, which forces a new TCP connection per proxied request:

| Service | Direct RPS | Proxy RPS | Retained |
|---------|---:|---:|---:|
| Apostol v2 | 485,521 | 59,586 | 12% |
| Go | 211,891 | 39,360 | 19% |
| Apostol v1 | 126,096 | 13,032 | 10% |
| Node.js | 101,891 | 6,639 | 7% |

The absolute proxy throughput ranking mirrors direct performance: Apostol v2 leads at 59.6K, followed by Go at 39.4K. Despite retaining only 12% of its direct throughput, Apostol v2's raw speed advantage is large enough that it still outperforms every other service through the proxy.

On `/db/ping` via proxy, Go edges ahead on RPS (36.3K vs 31.8K) but Apostol v2 delivers far better latency consistency (p99 = 7ms vs 51ms). With keep-alive OFF, Apostol v2 regains the throughput lead decisively (27K vs 3.8K).

### Keep-alive Impact

| Service | KA-ON (c100) | KA-OFF (c100) | Drop Factor |
|---------|-------------:|---------------:|------------:|
| **Apostol v2** | **485,521** | **88,327** | **5.5x** |
| Nginx | 543,131 | 85,693 | 6.3x |
| Go | 211,891 | 38,892 | 5.5x |
| Apostol v1 | 126,096 | 8,069 | 15.6x |
| Node.js | 101,891 | 6,420 | 15.9x |
| Python | 2,418 | 6,259 | +159% |

Apostol v2 and Go share the **smallest drop factor** (5.5x), ahead of Nginx (6.3x). For comparison, Apostol v1 and Node.js drop 15-16x. Python improves without keep-alive because its runtime overhead dominates TCP costs.

### Node.js and Python

Node.js (Fastify + PM2) reaches 102K RPS on `/ping` -- competitive for a JavaScript runtime. Python (FastAPI + uvicorn) reaches 2.4K RPS at 100 connections but scales to 22K at 1000 connections as worker utilization improves.

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

Results collected with 4 worker processes, 10-second test duration, `wrk --latency`. All services use `backlog=4096` and `ulimits.nofile=1048576`. Results may vary between runs depending on host load and thermal conditions.
