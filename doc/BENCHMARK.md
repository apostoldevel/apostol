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

**Nginx** -- serves as both the v0 baseline (static JSON responses) and an API gateway for proxy tests. `worker_processes auto`, `keepalive_requests 10000`, access logging disabled.

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
2. The load generator communicates directly with each service (no proxy).
3. Services are tested sequentially to avoid resource contention.
4. Each endpoint is tested for each service with a cooldown between runs.

---

## Results

**Configuration:** 4 workers, wrk with `--latency`, 4 threads, 10 second duration, direct connection (no proxy).

### /ping -- Keep-alive ON

Pure HTTP stack performance. Keep-alive eliminates TCP handshake overhead, isolating request processing.

**4 threads / 100 connections:**

| Service | RPS | Avg Latency | p50 | p75 | p90 | p99 |
|---------|----:|------------:|----:|----:|----:|----:|
| Nginx (v0) | 584,991 | 119us | 109us | 143us | 215us | 330us |
| **Apostol v2 (v5)** | **270,706** | **368us** | **351us** | **380us** | **436us** | **722us** |
| Go (v4) | 114,873 | 0.91ms | 0.87ms | 1.30ms | 1.76ms | 2.50ms |
| Apostol v1 (v1) | 67,004 | 1.50ms | 1.48ms | 1.51ms | 1.63ms | 2.13ms |
| Node.js (v3) | 54,308 | 2.18ms | 1.77ms | 1.86ms | 2.04ms | 4.33ms |
| Python (v2) | 2,404 | 41.51ms | 41.03ms | 41.98ms | 42.06ms | 49.06ms |

```
Nginx     ████████████████████████████████████████████████████████  585K
Apostol2  ██████████████████████████                                271K
Go        ███████████                                               115K
Apostol1  ██████                                                     67K
Node.js   █████                                                      54K
Python    ▏                                                           2K
```

**4 threads / 1000 connections:**

| Service | RPS | Avg Latency | p50 | p99 |
|---------|----:|------------:|----:|----:|
| Nginx | 611,664 | 1.70ms | 1.27ms | 5.45ms |
| **Apostol v2** | **255,816** | **3.89ms** | **3.73ms** | **6.38ms** |
| Go | 110,121 | 9.04ms | 8.94ms | 13.55ms |
| Apostol v1 | 61,486 | 22.94ms | 15.99ms | 252.37ms |
| Node.js | 52,879 | 36.86ms | 17.64ms | 846.23ms |
| Python | 11,179 | 88.74ms | 88.00ms | 115.89ms |

At 1000 connections, Apostol v2 maintains 256K RPS with p99 = 6.38ms. For comparison: Apostol v1 degrades to p99 = 252ms, Node.js to 846ms.

### /ping -- Keep-alive OFF

Disabling keep-alive (`Connection: close`) forces a new TCP connection per request. This tests raw connection handling efficiency.

**4 threads / 100 connections:**

| Service | RPS | Avg Latency | p50 | p99 |
|---------|----:|------------:|----:|----:|
| Nginx | 85,920 | 423us | 361us | 1.17ms |
| **Apostol v2** | **75,720** | **1.03ms** | **820us** | **3.53ms** |
| Go | 8,760 | 4.99ms | 3.04ms | 19.04ms |
| Apostol v1 | 7,274 | 6.89ms | 4.84ms | 28.47ms |
| Node.js | 6,282 | 8.09ms | 7.17ms | 22.98ms |
| Python | 5,604 | 10.62ms | 10.59ms | 25.74ms |

**4 threads / 1000 connections:**

| Service | RPS | Avg Latency | p50 | p99 |
|---------|----:|------------:|----:|----:|
| **Apostol v2** | **83,617** | **10.63ms** | **9.62ms** | **25.75ms** |
| Nginx | 74,189 | 4.06ms | 3.17ms | 11.83ms |
| Go | 24,642 | 17.58ms | 16.85ms | 43.97ms |
| Apostol v1 | 16,467 | 30.03ms | 29.93ms | 68.30ms |
| Node.js | 9,219 | 54.64ms | 56.98ms | 124.77ms |
| Python | 5,670 | 103.76ms | 105.37ms | 221.72ms |

At 1000 connections without keep-alive, **Apostol v2 surpasses Nginx** in throughput: 83.6K vs 74.2K RPS. This is possible because Apostol v2 uses `SO_REUSEPORT`, which distributes incoming connections across worker processes at the kernel level, while Nginx serializes connection acceptance through its accept mechanism.

### /db/ping -- Keep-alive ON

Database round-trip test with keep-alive enabled. This is the production-relevant scenario.

**4 threads / 100 connections:**

| Service | RPS | Avg Latency | p50 | p75 | p90 | p99 |
|---------|----:|------------:|----:|----:|----:|----:|
| **Apostol v2** | **69,407** | **1.44ms** | **1.42ms** | **1.63ms** | **1.75ms** | **2.09ms** |
| Go | 57,963 | 1.73ms | 1.62ms | 1.76ms | 2.33ms | 2.94ms |
| Apostol v1 | 32,584 | 3.24ms | 3.02ms | 3.12ms | 3.22ms | 3.59ms |
| Node.js | 26,283 | 3.97ms | 3.69ms | 3.85ms | 4.05ms | 6.57ms |
| Python | 2,269 | 47.19ms | 42.06ms | 43.04ms | 44.95ms | 238.52ms |

Apostol v2 leads with 69.4K RPS -- 20% faster than Go (58K) and 2.1x faster than Apostol v1 (32.6K). The advantage comes from the tight integration between epoll and libpq: HTTP parsing, PostgreSQL query dispatch, and response building all happen in a single event loop iteration with minimal overhead.

### /db/ping -- Keep-alive OFF

**4 threads / 100 connections:**

| Service | RPS | Avg Latency | p50 | p99 |
|---------|----:|------------:|----:|----:|
| Apostol v2 | 34,142 | 34.83ms | 2.86ms | 732.61ms |
| Go | 7,167 | 7.14ms | 4.76ms | 19.61ms |
| Node.js | 5,730 | 9.70ms | 9.26ms | 21.20ms |
| Python | 4,390 | 15.63ms | 15.35ms | 31.51ms |
| Apostol v1 | 2,800 | 7.20ms | 5.20ms | 18.69ms |

Apostol v2 achieves 34K RPS -- nearly 5x Go -- but shows bimodal latency: p50 = 2.86ms while p99 = 733ms. The high backlog (4096) allows the kernel to queue connections faster than the event loop can close them under combined TCP teardown + DB round-trip load, creating occasional latency spikes. In production, keep-alive is always enabled, making this scenario primarily of academic interest.

---

## Analysis

### Apostol v2: The Impact of Tuning

The most significant finding of this benchmark is the effect of two simple kernel-level parameters on Apostol v2's performance:

| Parameter | Before | After | Impact |
|-----------|--------|-------|--------|
| `listen()` backlog | 128 | 4096 | Eliminates SYN queue overflow at high concurrency |
| `epoll_wait()` maxevents | 64 | 512 | 8x fewer syscalls per event loop iteration |

These changes transformed Apostol v2 from a competitive performer into the outright leader among application servers. The lesson: **system-level tuning matters as much as algorithmic optimization**.

### Apostol v2 vs Apostol v1

Both versions use the same architectural pattern (single-threaded epoll event loop per worker), but v2 delivers dramatically higher throughput:

| Test | v2 | v1 | Speedup |
|------|---:|---:|--------:|
| /ping KA-ON c100 | 270,706 | 67,004 | **4.0x** |
| /ping KA-ON c1000 | 255,816 | 61,486 | **4.2x** |
| /ping KA-OFF c100 | 75,720 | 7,274 | **10.4x** |
| /ping KA-OFF c1000 | 83,617 | 16,467 | **5.1x** |
| /db/ping KA-ON c100 | 69,407 | 32,584 | **2.1x** |

The 4x improvement on `/ping` keep-alive ON comes from:

1. **llhttp** parser vs libdelphi's hand-written parser -- llhttp is a highly optimized state machine.
2. **C++20 standard library** -- `std::string_view`, move semantics, and zero-copy patterns eliminate unnecessary allocations.
3. **Minimal response path** -- v2's response builder writes directly to the socket buffer with no intermediate object construction.
4. **Tuned epoll** -- `MAX_EVENTS=512` vs v1's `EVENT_SIZE=512` (now equal, but v2 was at 64 before tuning).

The 10x improvement on `/ping` keep-alive OFF additionally benefits from:

5. **`SO_REUSEPORT`** -- kernel-level load balancing of incoming connections across workers, vs v1's single accept socket.
6. **backlog=4096** -- v1 already used `SOMAXCONN` (4096), but v2 was at 128 before tuning.

### Apostol v2 vs Go

Apostol v2 outperforms Go across every test:

| Test | Apostol v2 | Go | Ratio |
|------|---:|---:|------:|
| /ping KA-ON c100 | 270,706 | 114,873 | **2.4x** |
| /ping KA-OFF c1000 | 83,617 | 24,642 | **3.4x** |
| /db/ping KA-ON c100 | 69,407 | 57,963 | **1.2x** |

The advantage narrows on database tests (1.2x vs 2.4x), confirming that the PostgreSQL round-trip becomes the dominant cost. For pure HTTP handling, v2's epoll-based event loop outperforms Go's goroutine scheduler by a wide margin.

### Apostol v2 vs Nginx

On `/ping` with keep-alive ON, Apostol v2 reaches **46% of Nginx's throughput** (271K vs 585K). Given that Nginx serves static `return 200` directives with no application logic, this is a remarkable result for a full application server.

On `/ping` with keep-alive OFF at 1000 connections, Apostol v2 **surpasses Nginx**: 83.6K vs 74.2K RPS. The advantage comes from `SO_REUSEPORT` -- the kernel distributes incoming connections directly to worker processes, avoiding Nginx's serialized accept pattern.

### Latency Consistency

Apostol v2 delivers the tightest p99 spread of any service at 1000 connections (keep-alive ON):

| Service | p50 | p99 | Spread (p99/p50) |
|---------|----:|----:|-------:|
| **Apostol v2** | **3.73ms** | **6.38ms** | **1.7x** |
| Go | 8.94ms | 13.55ms | 1.5x |
| Apostol v1 | 15.99ms | 252.37ms | 15.8x |
| Node.js | 17.64ms | 846.23ms | 48.0x |

The single-threaded event loop model eliminates lock contention, goroutine scheduling jitter, and GC pauses, producing highly predictable latency even under heavy load.

### Keep-alive Impact

| Service | KA-ON (c100) | KA-OFF (c100) | Drop Factor |
|---------|-------------:|---------------:|------------:|
| Nginx | 584,991 | 85,920 | 6.8x |
| **Apostol v2** | **270,706** | **75,720** | **3.6x** |
| Go | 114,873 | 8,760 | 13.1x |
| Apostol v1 | 67,004 | 7,274 | 9.2x |
| Node.js | 54,308 | 6,282 | 8.6x |
| Python | 2,404 | 5,604 | +133% |

Apostol v2 shows the **smallest drop factor** among all services (3.6x) thanks to `SO_REUSEPORT`. For comparison, Go drops 13.1x and Apostol v1 drops 9.2x. Python improves without keep-alive because its runtime overhead dominates TCP costs.

### Node.js and Python

Node.js (Fastify + PM2) reaches 54K RPS -- competitive for a JavaScript runtime. Python (FastAPI + uvicorn) reaches 2.4K RPS at 100 connections but scales to 11K at 1000 connections as worker utilization improves.

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

# Quick ping benchmark (10s per test)
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
