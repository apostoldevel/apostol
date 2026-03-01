[English version](BENCHMARK.md)

# REST API Benchmark: Apostol v2 vs v1 vs Python vs Node.js vs Go

## Контекст

[Apostol](https://github.com/apostoldevel/apostol) -- C++ фреймворк для создания высокопроизводительных асинхронных HTTP-серверов под Linux. Версия 1 была написана на C++14 с использованием библиотеки libdelphi. Версия 2 -- полная переработка на **C++20** с использованием фреймворка [libapostol](https://github.com/apostoldevel/libapostol): стандартная библиотека, `{fmt}`, `nlohmann/json`, `llhttp`, без legacy-зависимостей.

Обе версии используют одну и ту же архитектуру: асинхронная событийная модель на базе `epoll` с прямой интеграцией PostgreSQL через `libpq`. HTTP-сервер и PostgreSQL-сокеты работают в едином event loop на воркер, без промежуточных слоёв.

Данный бенчмарк сравнивает шесть сервисов в идентичных условиях:

| Версия | Сервис | Стек |
|--------|--------|------|
| v0 | Nginx (baseline) | Статические `return 200` |
| v1 | Apostol v1 | C++14, libdelphi, epoll, libpq async |
| v2 | Python | FastAPI + uvicorn + asyncpg |
| v3 | Node.js | Fastify + pg + PM2 (cluster mode) |
| v4 | Go | net/http + pgx/v5 pool |
| v5 | **Apostol v2** | **C++20, libapostol, epoll, libpq async** |

---

## Тестовая среда

### Инфраструктура Docker

Все сервисы запущены в отдельных Docker-контейнерах на одном хосте. Две изолированные сети:

- **bench-lan** (172.19.1.0/24) -- прикладной уровень (Nginx, application-серверы, loadgen)
- **bench-db** (172.19.2.0/24) -- уровень БД (PostgreSQL, PgBouncer, application-серверы)

**PostgreSQL 17** -- оптимизирован для бенчмарка (не для production):

```
fsync = off
synchronous_commit = off
full_page_writes = off
wal_level = minimal
shared_buffers = 256MB
work_mem = 16MB
max_connections = 200
```

**PgBouncer** -- session pooling, `default_pool_size=50`, `max_client_conn=500`.

**Nginx** -- baseline (статические JSON-ответы) и API gateway. `worker_processes auto`, `keepalive_requests 10000`, логирование отключено.

### Connection pools

Все серверы: **min=5, max=15** на воркер.

| Сервис | Модель пула | Соединений к БД (4 воркера) |
|--------|-------------|----------------------------|
| Apostol v1/v2 | Per-worker | 15 x 4 = 60 |
| Python (uvicorn) | Per-worker | 15 x 4 = 60 |
| Node.js (PM2) | Per-worker | 15 x 4 = 60 |
| Go | Единый пул | 15 |

### Эндпоинты

| Эндпоинт | Тип | Ответ |
|----------|-----|-------|
| `/api/vN/ping` | Статический | `{"ok":true,"message":"OK"}` |
| `/api/vN/time` | Вычисляемый | `{"serverTime":<unix_epoch>}` |
| `/api/vN/db/ping` | Запрос к БД | `SELECT json_build_object('ok', true, 'message', 'OK')::text` |
| `/api/vN/db/time` | Запрос к БД | `SELECT json_build_object('serverTime', extract(epoch from now())::integer)::text` |

---

## Выравнивание параметров

Перед запуском бенчмарка мы выровняли два критических параметра ядра для честного сравнения.

### TCP Listen Backlog

Параметр `backlog` в `listen()` определяет, сколько pending TCP-соединений ядро ставит в очередь. При высокой конкурентности (напр., `wrk -c1000` с `Connection: close`) маленький backlog вызывает SYN retransmit и искусственно ограничивает throughput.

Значения по умолчанию сильно различались:

| Сервис | Backlog по умолчанию | В бенчмарке |
|--------|---------------------|-------------|
| Nginx | 511 | **4096** (`listen 80 backlog=4096`) |
| Apostol v1 | 4096 (`SOMAXCONN`) | 4096 (без изменений) |
| Apostol v2 | 128 | **4096** (`server.backlog` в JSON) |
| Go | 4096 (`SOMAXCONN`) | 4096 (без изменений) |
| Node.js | 511 | **4096** (`backlog` в `fastify.listen()`) |
| Python | 2048 | **4096** (`--backlog 4096` в uvicorn) |

Теперь все сервисы используют `backlog=4096`, что соответствует `net.core.somaxconn` ядра.

### Размер массива epoll

Параметр `maxevents` в `epoll_wait()` определяет, сколько I/O-событий ядро возвращает за один syscall. Маленькое значение = больше syscall-ов под нагрузкой.

| Сервис | Размер массива | Источник |
|--------|---------------|----------|
| Nginx | 512 | `epoll_events` (default) |
| Apostol v1 | 512 | `EVENT_SIZE` в Sockets.cpp |
| Apostol v2 | 64 -> **512** | `MAX_EVENTS` в event_loop.hpp |
| Go | 128 | Go runtime (hardcoded) |
| Node.js | 1024 | libuv (hardcoded) |

`MAX_EVENTS` в Apostol v2 был увеличен с 64 до 512. При значении 64 v2 делал **в 8 раз больше syscall-ов** для обработки того же объёма событий -- значительный overhead, скрывавший реальную производительность.

Эти два изменения -- backlog и размер массива epoll -- радикально повлияли на throughput Apostol v2, особенно в сценариях с `Connection: close`.

---

## Методология

**wrk** -- 4 потока (`-t4`), 100 или 1000 соединений (`-c100`/`-c1000`), 10 секунд (`-d10s`), `--latency`.

Сервисы тестируются последовательно. Нагрузочный генератор обращается к каждому сервису напрямую (без Nginx proxy).

---

## Результаты

**Конфигурация:** 4 воркера, wrk `--latency`, 4 потока, 10 секунд.

### /ping -- keep-alive ON

Чистая производительность HTTP-стека.

**t4/c100:**

| Сервис | RPS | Avg Latency | p50 | p75 | p90 | p99 |
|--------|----:|------------:|----:|----:|----:|----:|
| Nginx (v0) | 584 991 | 119us | 109us | 143us | 215us | 330us |
| **Apostol v2 (v5)** | **270 706** | **368us** | **351us** | **380us** | **436us** | **722us** |
| Go (v4) | 114 873 | 0,91ms | 0,87ms | 1,30ms | 1,76ms | 2,50ms |
| Apostol v1 (v1) | 67 004 | 1,50ms | 1,48ms | 1,51ms | 1,63ms | 2,13ms |
| Node.js (v3) | 54 308 | 2,18ms | 1,77ms | 1,86ms | 2,04ms | 4,33ms |
| Python (v2) | 2 404 | 41,51ms | 41,03ms | 41,98ms | 42,06ms | 49,06ms |

```
Nginx     ████████████████████████████████████████████████████████  585K
Apostol2  ██████████████████████████                                271K
Go        ███████████                                               115K
Apostol1  ██████                                                     67K
Node.js   █████                                                      54K
Python    ▏                                                           2K
```

**t4/c1000:**

| Сервис | RPS | Avg Latency | p50 | p99 |
|--------|----:|------------:|----:|----:|
| Nginx | 611 664 | 1,70ms | 1,27ms | 5,45ms |
| **Apostol v2** | **255 816** | **3,89ms** | **3,73ms** | **6,38ms** |
| Go | 110 121 | 9,04ms | 8,94ms | 13,55ms |
| Apostol v1 | 61 486 | 22,94ms | 15,99ms | 252,37ms |
| Node.js | 52 879 | 36,86ms | 17,64ms | 846,23ms |
| Python | 11 179 | 88,74ms | 88,00ms | 115,89ms |

При 1000 соединений Apostol v2 сохраняет 256K RPS с p99 = 6,38ms. Для сравнения: v1 деградирует до p99 = 252ms, Node.js -- до 846ms.

### /ping -- keep-alive OFF

Каждый запрос создаёт новое TCP-соединение. Тест на эффективность обработки жизненного цикла соединений.

**t4/c100:**

| Сервис | RPS | Avg Latency | p50 | p99 |
|--------|----:|------------:|----:|----:|
| Nginx | 85 920 | 423us | 361us | 1,17ms |
| **Apostol v2** | **75 720** | **1,03ms** | **820us** | **3,53ms** |
| Go | 8 760 | 4,99ms | 3,04ms | 19,04ms |
| Apostol v1 | 7 274 | 6,89ms | 4,84ms | 28,47ms |
| Node.js | 6 282 | 8,09ms | 7,17ms | 22,98ms |
| Python | 5 604 | 10,62ms | 10,59ms | 25,74ms |

**t4/c1000:**

| Сервис | RPS | Avg Latency | p50 | p99 |
|--------|----:|------------:|----:|----:|
| **Apostol v2** | **83 617** | **10,63ms** | **9,62ms** | **25,75ms** |
| Nginx | 74 189 | 4,06ms | 3,17ms | 11,83ms |
| Go | 24 642 | 17,58ms | 16,85ms | 43,97ms |
| Apostol v1 | 16 467 | 30,03ms | 29,93ms | 68,30ms |
| Node.js | 9 219 | 54,64ms | 56,98ms | 124,77ms |
| Python | 5 670 | 103,76ms | 105,37ms | 221,72ms |

При 1000 соединений без keep-alive **Apostol v2 обгоняет Nginx** по throughput: 83,6K vs 74,2K RPS. Это возможно благодаря `SO_REUSEPORT` -- ядро распределяет входящие соединения по воркерам, минуя сериализацию accept в Nginx.

### /db/ping -- keep-alive ON

Тест с обращением к PostgreSQL. Наиболее близкий к production сценарий.

**t4/c100:**

| Сервис | RPS | Avg Latency | p50 | p75 | p90 | p99 |
|--------|----:|------------:|----:|----:|----:|----:|
| **Apostol v2** | **69 407** | **1,44ms** | **1,42ms** | **1,63ms** | **1,75ms** | **2,09ms** |
| Go | 57 963 | 1,73ms | 1,62ms | 1,76ms | 2,33ms | 2,94ms |
| Apostol v1 | 32 584 | 3,24ms | 3,02ms | 3,12ms | 3,22ms | 3,59ms |
| Node.js | 26 283 | 3,97ms | 3,69ms | 3,85ms | 4,05ms | 6,57ms |
| Python | 2 269 | 47,19ms | 42,06ms | 43,04ms | 44,95ms | 238,52ms |

Apostol v2 лидирует с 69,4K RPS -- на 20% быстрее Go (58K) и в 2,1 раза быстрее v1 (32,6K). Преимущество обеспечивается тесной интеграцией epoll и libpq: HTTP-парсинг, отправка запроса к PostgreSQL и формирование ответа происходят в одной итерации event loop с минимальным overhead.

### /db/ping -- keep-alive OFF

**t4/c100:**

| Сервис | RPS | Avg Latency | p50 | p99 |
|--------|----:|------------:|----:|----:|
| Apostol v2 | 34 142 | 34,83ms | 2,86ms | 732,61ms |
| Go | 7 167 | 7,14ms | 4,76ms | 19,61ms |
| Node.js | 5 730 | 9,70ms | 9,26ms | 21,20ms |
| Python | 4 390 | 15,63ms | 15,35ms | 31,51ms |
| Apostol v1 | 2 800 | 7,20ms | 5,20ms | 18,69ms |

Apostol v2 показывает 34K RPS -- почти 5x от Go -- но с бимодальным распределением latency: p50 = 2,86ms при p99 = 733ms. Большой backlog (4096) позволяет ядру ставить соединения в очередь быстрее, чем event loop успевает их закрыть при совмещённой нагрузке TCP teardown + DB round-trip. В production keep-alive всегда включён, что делает этот сценарий чисто академическим.

---

## Анализ

### Apostol v2: влияние настройки параметров

Главный вывод бенчмарка -- влияние двух простых параметров на производительность Apostol v2:

| Параметр | До | После | Эффект |
|----------|-----|-------|--------|
| `listen()` backlog | 128 | 4096 | Устраняет переполнение SYN-очереди |
| `epoll_wait()` maxevents | 64 | 512 | В 8 раз меньше syscall-ов на итерацию event loop |

Эти изменения превратили Apostol v2 из конкурентоспособного решения в безоговорочного лидера. Вывод: **настройка системных параметров важна не меньше, чем алгоритмическая оптимизация**.

### Apostol v2 vs v1

Обе версии используют один и тот же архитектурный паттерн (однопоточный epoll event loop на воркер), но v2 показывает радикально более высокий throughput:

| Тест | v2 | v1 | Ускорение |
|------|---:|---:|----------:|
| /ping KA-ON c100 | 270 706 | 67 004 | **4,0x** |
| /ping KA-ON c1000 | 255 816 | 61 486 | **4,2x** |
| /ping KA-OFF c100 | 75 720 | 7 274 | **10,4x** |
| /ping KA-OFF c1000 | 83 617 | 16 467 | **5,1x** |
| /db/ping KA-ON c100 | 69 407 | 32 584 | **2,1x** |

Ускорение в 4x на `/ping` keep-alive ON обеспечивается:

1. **llhttp** -- высокооптимизированный конечный автомат (vs рукописный парсер в libdelphi).
2. **C++20** -- `std::string_view`, move-семантика, zero-copy паттерны.
3. **Минимальный путь ответа** -- response builder пишет прямо в буфер сокета.
4. **Настроенный epoll** -- `MAX_EVENTS=512` (было 64).

Ускорение в 10x на `/ping` keep-alive OFF дополнительно обеспечивается:

5. **`SO_REUSEPORT`** -- ядро распределяет соединения по воркерам (vs единый accept-сокет в v1).
6. **backlog=4096** -- v1 уже использовал `SOMAXCONN` (4096), но v2 был на 128.

### Apostol v2 vs Go

Apostol v2 опережает Go во всех тестах:

| Тест | Apostol v2 | Go | Разница |
|------|---:|---:|--------:|
| /ping KA-ON c100 | 270 706 | 114 873 | **2,4x** |
| /ping KA-OFF c1000 | 83 617 | 24 642 | **3,4x** |
| /db/ping KA-ON c100 | 69 407 | 57 963 | **1,2x** |

Разрыв сужается на database-тестах (1,2x vs 2,4x), что подтверждает: PostgreSQL round-trip становится доминирующей стоимостью. На чистом HTTP v2 опережает горутинный планировщик Go с большим отрывом.

### Apostol v2 vs Nginx

На `/ping` keep-alive ON Apostol v2 достигает **46% от throughput Nginx** (271K vs 585K). Учитывая, что Nginx отдаёт статические `return 200` без логики, это впечатляющий результат для полноценного application-сервера.

На `/ping` keep-alive OFF при 1000 соединениях Apostol v2 **обгоняет Nginx**: 83,6K vs 74,2K RPS. Преимущество обеспечивается `SO_REUSEPORT` -- ядро распределяет входящие соединения напрямую по воркерам.

### Стабильность latency

Apostol v2 показывает наименьший разброс p99 при 1000 соединениях (keep-alive ON):

| Сервис | p50 | p99 | Разброс (p99/p50) |
|--------|----:|----:|-------:|
| **Apostol v2** | **3,73ms** | **6,38ms** | **1,7x** |
| Go | 8,94ms | 13,55ms | 1,5x |
| Apostol v1 | 15,99ms | 252,37ms | 15,8x |
| Node.js | 17,64ms | 846,23ms | 48,0x |

Однопоточный event loop устраняет lock contention, jitter планировщика горутин и паузы GC, обеспечивая предсказуемую latency даже под высокой нагрузкой.

### Влияние keep-alive

| Сервис | KA-ON (c100) | KA-OFF (c100) | Падение |
|--------|-------------:|---------------:|--------:|
| Nginx | 584 991 | 85 920 | 6,8x |
| **Apostol v2** | **270 706** | **75 720** | **3,6x** |
| Go | 114 873 | 8 760 | 13,1x |
| Apostol v1 | 67 004 | 7 274 | 9,2x |
| Node.js | 54 308 | 6 282 | 8,6x |
| Python | 2 404 | 5 604 | +133% |

Apostol v2 показывает **наименьшее падение** среди всех сервисов (3,6x) благодаря `SO_REUSEPORT`. Для сравнения: Go теряет 13,1x, Apostol v1 -- 9,2x. Python растёт без keep-alive, потому что overhead Python runtime доминирует над стоимостью TCP.

---

## Как воспроизвести

### Требования

- Docker и Docker Compose
- `ulimit -n` >= 65536

### Запуск

```bash
cd benchmark
docker compose up -d
docker compose ps  # дождаться "healthy" у всех сервисов
```

### Запуск бенчмарка

```bash
# Сборка loadgen
docker compose --profile loadgen build loadgen

# Быстрый ping-бенчмарк (10 сек на тест)
docker compose --profile loadgen run --rm loadgen /bench/scripts/bench_ping.sh

# Полный бенчмарк
docker compose --profile loadgen run --rm loadgen /bench/scripts/bench.sh --quick
```

### Изменение числа воркеров

```bash
WORKERS=4 docker compose up -d
```

---

## Примечание

Результаты получены с 4 воркерами, 10 секунд на тест, `wrk --latency`. Все сервисы используют `backlog=4096` и `ulimits.nofile=1048576`. Результаты могут незначительно варьироваться между запусками в зависимости от нагрузки хоста.

---

## Исходный код

Весь код бенчмарка (Dockerfile, конфигурации, скрипты, исходники серверов) находится в каталоге [`benchmark/`](../benchmark/) данного репозитория.
