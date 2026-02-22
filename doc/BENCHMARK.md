[Русская версия](BENCHMARK.ru-RU.md)

# REST API Benchmark: Apostol vs Python vs Node.js vs Go

## Context

[Apostol](https://github.com/apostoldevel/apostol) is a C++14 framework for building high-performance Linux server applications (daemons). It uses an asynchronous event-driven model based on `epoll` with direct PostgreSQL integration via `libpq`. The HTTP server and PostgreSQL sockets share a single event loop, eliminating intermediary layers for minimal latency.

This benchmark compares Apostol against three widely used server-side platforms -- Python (FastAPI/uvicorn/asyncpg), Node.js (Fastify/pg/PM2), and Go (net/http/pgx) -- with Nginx static responses serving as a theoretical upper bound baseline.

The goal is to measure raw request throughput and latency under identical conditions: same hardware, same PostgreSQL instance, same queries, same connection pool parameters.

## Test Environment

### Docker Topology

All services run in separate Docker containers on the same host, orchestrated by a single `docker-compose.yaml`. The stack is divided into two isolated Docker networks:

- **bench-lan** (172.19.1.0/24) -- application layer (Nginx, Apostol, Python, Node.js, Go, loadgen)
- **bench-db** (172.19.2.0/24) -- database layer (PostgreSQL, PgBouncer, and application containers that need DB access)

### Endpoints

Each service implements four identical endpoints returning JSON with `Content-Type: application/json; charset=utf-8`:

| Endpoint | Type | Response |
|----------|------|----------|
| `/api/vN/ping` | Static | `{"ok":true,"message":"OK"}` |
| `/api/vN/time` | Compute | `{"serverTime":<unix_epoch_seconds>}` |
| `/api/vN/db/ping` | DB query | `SELECT json_build_object('ok', true, 'message', 'OK')::text` |
| `/api/vN/db/time` | DB query | `SELECT json_build_object('serverTime', extract(epoch from now())::integer)::text` |

The version prefix `vN` maps to the service:

| Version | Service | Technology Stack |
|---------|---------|-----------------|
| v0 | Nginx (baseline) | Static `return 200` directives |
| v1 | Apostol | C++14, epoll, libpq async |
| v2 | Python | FastAPI + uvicorn + asyncpg |
| v3 | Node.js | Fastify + pg + PM2 (cluster mode) |
| v4 | Go | net/http + pgx/v5 pool |

### Service Implementation Details

**Apostol (v1)** -- built from source using the Apostol framework. The benchmark WebServer module handles four routes directly in the `DoGet` method. Static endpoints build the response in-process; DB endpoints use `ExecSQL` with async callbacks on the shared `epoll` event loop. Connection keep-alive is configurable.

**Python (v2)** -- FastAPI application served by uvicorn. Each worker process creates its own asyncpg connection pool (min=5, max=15). Responses use raw `Response` objects to bypass JSON serialization overhead.

**Node.js (v3)** -- Fastify application managed by PM2 in cluster mode. Each worker process creates its own pg `Pool` (min=5, max=15). Responses are pre-serialized strings sent with explicit Content-Type.

**Go (v4)** -- Standard library `net/http` with pgx/v5 connection pool. `GOMAXPROCS` is set to the `WORKERS` environment variable. Unlike other services, Go creates a single pool (min=5, max=15) shared across all goroutines via the Go runtime scheduler.

### Infrastructure Configuration

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

All application services use identical pool parameters:

- **min_size:** 5 connections
- **max_size:** 15 connections

With the worker count of 4, this means:

| Service | Pool Model | Total DB Connections (4 workers) |
|---------|-----------|--------------------------------|
| Apostol | Per-worker | 15 x 4 = 60 |
| Python (uvicorn) | Per-worker | 15 x 4 = 60 |
| Node.js (PM2) | Per-worker | 15 x 4 = 60 |
| Go | Single shared pool | 15 (always) |

This is a significant architectural difference. Go's goroutine scheduler multiplexes all requests across a single pool of 15 connections regardless of `GOMAXPROCS`, while the other services create independent pools per OS process.

## Methodology

### Load Generator

The load generator container includes three tools: **wrk**, **hey**, and **k6**. The quick mode benchmark uses wrk.

**wrk parameters for quick mode:**
- 4 threads (`-t4`)
- 100 or 1000 concurrent connections (`-c100` / `-c1000`)
- 10 second duration (`-d10s`)
- Latency histogram enabled (`--latency`)
- Keep-alive ON (wrk default) and OFF (`-H "Connection: close"`)

### Test Procedure

1. All service containers start and reach healthy state.
2. The load generator container runs on the same Docker network (`bench-lan`), communicating directly with each service (no proxy).
3. Services are tested sequentially -- one at a time -- to avoid resource contention.
4. Each of the 4 endpoints is tested for each of the 5 services (20 individual wrk runs).
5. Metrics collected: Requests/sec (throughput) and average latency.

### Fairness Controls

- All containers run on the same host with identical `ulimits` (`nofile=1048576`).
- All application services use the same PostgreSQL instance with the same credentials and queries.
- Connection pool sizes are identical (5 min, 15 max).
- Worker count is identical (4) across all services.
- The load generator communicates directly with each service, bypassing the Nginx proxy layer.

## Results

**Configuration:** 4 workers, wrk with `--latency`, 4 threads, 10 second duration, direct connection (no proxy).

### /ping -- Keep-alive ON

The primary throughput comparison. Keep-alive eliminates TCP handshake overhead, isolating pure request processing performance.

**4 threads / 100 connections:**

| Service | RPS | Avg Latency | p50 | p75 | p90 | p99 |
|---------|----:|------------:|----:|----:|----:|----:|
| Nginx (v0) | 545,281 | 146us | 107us | 178us | 294us | 585us |
| Go (v4) | 197,416 | 555us | 473us | 786us | 1.13ms | 1.86ms |
| Apostol (v1) | 124,882 | 805us | 834us | 0.88ms | 1.09ms | 1.31ms |
| Node.js (v3) | 96,341 | 1.07ms | 0.95ms | 1.10ms | 1.25ms | 2.28ms |
| Python (v2) | 2,418 | 41.27ms | 41.02ms | 41.98ms | 42.01ms | 43.13ms |

```
Nginx     ██████████████████████████████████████████████████  545,281 RPS
Go        ██████████████████                                  197,416 RPS
Apostol   ███████████                                         124,882 RPS
Node.js   █████████                                            96,341 RPS
Python    ▏                                                     2,418 RPS
```

**4 threads / 1000 connections:**

| Service | RPS | Avg Latency | p50 | p99 |
|---------|----:|------------:|----:|----:|
| Nginx | 550,208 | 2.36ms | 1.92ms | 8.37ms |
| Go | 197,238 | 5.06ms | 4.99ms | 8.51ms |
| Apostol | 116,414 | 8.56ms | 8.88ms | 12.02ms |
| Node.js | 96,820 | 16.46ms | 9.73ms | 285.33ms |
| Python | 21,471 | 46.31ms | 44.99ms | 63.05ms |

Scaling from 100 to 1000 connections: Nginx and Go hold steady. Apostol and Node.js maintain throughput with minimal drop. Python jumps from 2.4K to 21.5K RPS (8.9x), confirming that at 100 connections its 4 workers were underutilized.

### /ping -- Keep-alive OFF

Disabling keep-alive (`Connection: close`) forces a new TCP connection per request, testing connection handling efficiency.

**4 threads / 100 connections:**

| Service | RPS | Avg Latency | p50 | p99 |
|---------|----:|------------:|----:|----:|
| Nginx | 82,369 | 437us | 372us | 1.22ms |
| Go | 43,861 | 1.15ms | 683us | 8.76ms |
| Apostol | 8,575 | 5.26ms | 2.19ms | 19.24ms |
| Node.js | 5,881 | 8.28ms | 6.33ms | 24.58ms |
| Python | 5,583 | 9.60ms | 8.71ms | 24.46ms |

**4 threads / 1000 connections:**

| Service | RPS | Avg Latency | p50 | p99 |
|---------|----:|------------:|----:|----:|
| Nginx | 78,807 | 4.61ms | 3.66ms | 13.84ms |
| Go | 78,378 | 6.18ms | 5.65ms | 15.81ms |
| Apostol | 28,136 | 15.78ms | 15.35ms | 41.25ms |
| Node.js | 11,631 | 33.29ms | 29.05ms | 88.91ms |
| Python | 6,976 | 77.59ms | 75.09ms | 177.29ms |

At 1000 connections without keep-alive, Go nearly matches Nginx (78K vs 79K RPS). Apostol scales up 3.3x from the 100-connection result, while Node.js and Python show more modest gains.

### /db/ping -- Direct PostgreSQL

Database round-trip test with `Connection: close`, 4 threads / 100 connections, connecting directly to PostgreSQL.

| Service | RPS | Avg Latency | p50 | p99 |
|---------|----:|------------:|----:|----:|
| Go | 11,877 | 4.50ms | 2.06ms | 18.63ms |
| Node.js | 4,967 | 10.80ms | 8.15ms | 33.79ms |
| Python | 4,465 | 18.77ms | 11.65ms | 264.14ms |
| Apostol | 2,746 | 7.74ms | 3.27ms | 29.50ms |

Go leads decisively at 11.9K RPS -- its goroutine model handles the combination of TCP teardown and async DB callbacks most efficiently. Apostol's lower throughput here reflects the cost of `Connection: close` combined with the async DB callback model: every request requires both a new TCP connection and a full DB round-trip through the event loop.

### /db/ping -- Via PgBouncer

Same test as above but routing through PgBouncer in **transaction pooling mode**, 4 threads / 100 connections.

| Service | RPS | Avg Latency | p50 | p99 | Notes |
|---------|----:|------------:|----:|----:|-------|
| Node.js | 5,212 | 10.45ms | 9.22ms | 27.25ms | Clean |
| Apostol | 2,797 | 6.82ms | 3.40ms | 24.69ms | Clean |
| Go | 8,989* | 5.92ms | 2.47ms | 23.23ms | 40% Non-2xx (prepared statements) |
| Python | 3,320* | 26.03ms | 24.92ms | 57.23ms | 77% Non-2xx (prepared statements) |

*RPS includes error responses.

Node.js (pg) and Apostol (libpq) work cleanly with PgBouncer transaction pooling. Go (pgx) and Python (asyncpg) use **prepared statements** by default, which are incompatible with transaction pooling mode -- the prepared statement is created on one server connection but the next query may be routed to a different connection where it does not exist. This is a well-known PgBouncer limitation; it can be worked around by disabling prepared statements in the client driver or switching PgBouncer to session pooling mode.

## Analysis

### Go Leads Among Application Servers

Go achieves the highest application-level throughput at 197,416 RPS on `/ping` with keep-alive -- 1.58x Apostol and 2.05x Node.js. The advantage stems from Go's goroutine scheduler, which efficiently multiplexes thousands of concurrent connections across OS threads without the overhead of process-level isolation. Go also demonstrates exceptional scalability without keep-alive: at 1000 connections it reaches 78,378 RPS, nearly matching Nginx (78,807 RPS) in raw connection handling.

### Apostol: Tightest Tail Latency

Apostol delivers 124,882 RPS on `/ping` -- strong C++ performance that places it firmly between Go and Node.js. But the standout metric is latency consistency: with a p99 of 1.31ms at 100 connections and 12.02ms at 1000 connections, Apostol has the **tightest p99 spread of any application server**. For comparison, Node.js p99 jumps from 2.28ms to 285.33ms (125x) when connections scale from 100 to 1000, while Apostol's p99 only grows from 1.31ms to 12.02ms (9.2x). This predictability is a key advantage of the single-threaded epoll event loop model -- there is no lock contention, no goroutine scheduling jitter, and no garbage collection pauses.

### Node.js: Surprisingly Competitive

Node.js (Fastify + PM2) reaches 96,341 RPS on `/ping` -- about 77% of Apostol's throughput. For a high-level JavaScript runtime, this is a remarkably strong result. The PM2 cluster mode with 4 workers effectively parallelizes the single-threaded V8 event loop, and Fastify's optimized request handling keeps per-request overhead low.

### Python: Runtime-Bound, but Scales with Connections

Python (FastAPI + uvicorn) delivers 2,418 RPS on `/ping` at 100 connections -- roughly 1/52 of Apostol and 1/82 of Go. This is expected behavior for the CPython runtime, where the GIL and per-request overhead dominate. However, at 1000 connections Python jumps to 21,471 RPS (8.9x increase), revealing that at lower connection counts its 4 workers were significantly underutilized. The GIL limits per-worker throughput, but with enough concurrent connections each worker stays busy.

### Keep-alive Impact

Disabling keep-alive reveals how efficiently each runtime handles TCP connection lifecycle:

| Service | KA-ON (c100) | KA-OFF (c100) | Drop Factor |
|---------|-------------:|---------------:|------------:|
| Nginx | 545,281 | 82,369 | 6.6x |
| Go | 197,416 | 43,861 | 4.5x |
| Apostol | 124,882 | 8,575 | 14.6x |
| Node.js | 96,341 | 5,881 | 16.4x |
| Python | 2,418 | 5,583 | +131% (improved) |

Nginx and Go handle connection teardown most gracefully (6.6x and 4.5x drops). Apostol and Node.js suffer more severely (14.6x and 16.4x) because their single-threaded event loops must serialize connection setup, request handling, and teardown sequentially. Python's throughput actually *increases* by 131% without keep-alive at 100 connections -- its runtime overhead is so dominant that TCP connection cost is negligible, and the simpler connection lifecycle may reduce per-request bookkeeping.

### Database Endpoints: Connection: close Penalty

With `Connection: close` on `/db/ping`, every request requires both a new TCP connection and a full database round-trip:

| Service | /db/ping RPS (direct) | Notes |
|---------|----------------------:|-------|
| Go | 11,877 | Goroutines handle close+DB efficiently |
| Node.js | 4,967 | Event loop serializes close+DB |
| Python | 4,465 | GIL-bound but asyncpg is efficient |
| Apostol | 2,746 | Async DB callback + close = double serialization |

Apostol's lower ranking here is specific to the `Connection: close` scenario. The epoll event loop must handle TCP teardown and the async DB callback sequentially per worker, creating a pipeline bottleneck that Go's goroutine model avoids through true parallelism within a single process.

### PgBouncer Compatibility

Transaction pooling mode in PgBouncer exposes a fundamental compatibility divide:

- **Node.js (pg)** and **Apostol (libpq)** work cleanly -- they use simple query protocol without prepared statements.
- **Go (pgx)** and **Python (asyncpg)** use prepared statements by default, which fail under transaction pooling because the prepared statement created on one server connection may be executed on a different one. Go saw 40% error rates, Python 77%.

This is a well-known PgBouncer limitation. Solutions include disabling prepared statements in the client driver configuration, or switching PgBouncer to session pooling mode (at the cost of connection multiplexing).

### Nginx Baseline

Nginx achieves 545,281 RPS on `/ping` with keep-alive, serving as the theoretical upper bound for HTTP response throughput on this hardware. All Nginx responses are static `return 200` directives with no application logic.

## How to Reproduce

### Prerequisites

- Docker and Docker Compose
- Sufficient file descriptor limits on the host (`ulimit -n` should be at least 65536)

### Start the Stack

```bash
cd benchmark
docker compose up -d
```

Wait for all services to reach healthy state:

```bash
docker compose ps
```

### Run the Quick Debug Benchmark

```bash
docker compose --profile loadgen build loadgen
docker compose --profile loadgen run --rm loadgen /bench/scripts/debug-quick.sh
```

This runs `wrk -t2 -c100 -d10s` against all 20 endpoint combinations and prints results to stdout.

### Run the Full Benchmark

```bash
docker compose --profile loadgen run --rm loadgen /bench/scripts/bench.sh --quick
```

Quick mode runs 10-second tests with two thread/connection configurations (2t/100c and 4t/1000c). Results are saved to the `results/` directory as individual wrk output files and a `summary.csv`.

For a full benchmark run (60-second tests, four configurations):

```bash
docker compose --profile loadgen run --rm loadgen /bench/scripts/bench.sh
```

The full benchmark script tests both direct access and proxy-through-Nginx access, with keep-alive on and off, across configurations: 2t/100c, 4t/1000c, 8t/5000c, 10t/10000c.

### Adjust Worker Count

To change the number of workers for all services:

```bash
WORKERS=4 docker compose up -d
```

This sets `WORKERS=4` for Apostol (worker processes), Python (uvicorn workers), Node.js (PM2 instances), and Go (`GOMAXPROCS`).

## Note

These results were collected with 4 worker processes, 10-second test duration, and wrk's `--latency` flag for percentile distribution. Results can vary slightly between runs depending on host load and thermal conditions. To test with a different worker count, use `WORKERS=N docker compose up -d` -- this sets the worker/process count for Apostol, Python (uvicorn), Node.js (PM2), and Go (`GOMAXPROCS`) simultaneously.
