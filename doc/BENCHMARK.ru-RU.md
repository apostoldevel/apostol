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

**Nginx** -- baseline (статические JSON-ответы) и API gateway для proxy-тестов. `worker_processes auto`, `listen 80 reuseport`, `keepalive_requests 10000`, логирование отключено.

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

Перед запуском бенчмарка мы выровняли критические параметры ядра для честного сравнения.

### TCP Listen Backlog

Параметр `backlog` в `listen()` определяет, сколько pending TCP-соединений ядро ставит в очередь. При высокой конкурентности (напр., `wrk -c1000` с `Connection: close`) маленький backlog вызывает SYN retransmit и искусственно ограничивает throughput.

Значения по умолчанию сильно различались:

| Сервис | Backlog по умолчанию | В бенчмарке |
|--------|---------------------|-------------|
| Nginx | 511 | **4096** (`listen 80 backlog=4096 reuseport`) |
| Apostol v1 | 4096 (`SOMAXCONN`) | 4096 (без изменений) |
| Apostol v2 | 4096 | 4096 (без изменений) |
| Go | 4096 (`SOMAXCONN`) | 4096 (без изменений) |
| Node.js | 511 | **4096** (`backlog` в `fastify.listen()`) |
| Python | 2048 | **4096** (`--backlog 4096` в uvicorn) |

Теперь все сервисы используют `backlog=4096`, что соответствует `net.core.somaxconn` ядра.

### SO_REUSEPORT

`SO_REUSEPORT` позволяет нескольким сокетам привязаться к одному порту. Ядро распределяет входящие соединения между этими сокетами, устраняя узкое место единого accept-сокета при высокой конкурентности.

| Сервис | SO_REUSEPORT | Источник |
|--------|:------------:|----------|
| Nginx | Да | `listen 80 reuseport` |
| Apostol v2 | Да | `setsockopt(SO_REUSEPORT)` в libapostol |
| Apostol v1 | Нет | Единый accept-сокет |
| Go | Нет | Стандартное поведение Go runtime |
| Node.js | Нет | PM2 cluster mode (round-robin) |
| Python | Нет | Стандартное поведение uvicorn |

Nginx и Apostol v2 используют `SO_REUSEPORT`, обеспечивая равные условия на тестах обработки соединений.

### Размер массива epoll

Параметр `maxevents` в `epoll_wait()` определяет, сколько I/O-событий ядро возвращает за один syscall. Маленькое значение = больше syscall-ов под нагрузкой.

| Сервис | Размер массива | Источник |
|--------|---------------|----------|
| Nginx | 512 | `epoll_events` (default) |
| Apostol v1 | 512 | `EVENT_SIZE` в Sockets.cpp |
| Apostol v2 | 512 | `MAX_EVENTS` в event_loop.hpp |
| Go | 128 | Go runtime (hardcoded) |
| Node.js | 1024 | libuv (hardcoded) |

---

## Методология

### Нагрузочный генератор

**wrk** с параметрами:

- 4 потока (`-t4`)
- 100 или 1000 соединений (`-c100`/`-c1000`)
- 10 секунд (`-d10s`)
- Гистограмма latency (`--latency`)
- Keep-alive ON (по умолчанию в wrk) и OFF (`-H "Connection: close"`)

### Порядок тестирования

1. Все контейнеры сервисов запускаются и достигают healthy-статуса.
2. **Прямые тесты:** нагрузочный генератор обращается к каждому сервису напрямую (без proxy).
3. **Proxy-тесты:** нагрузочный генератор обращается через Nginx, который проксирует запросы к каждому сервису. Nginx использует `Connection: close` к upstream, т.е. каждый проксируемый запрос создаёт новое TCP-соединение к бэкенду.
4. Сервисы тестируются последовательно для исключения борьбы за ресурсы.
5. Между тестами выдерживается пауза.

---

## Результаты: напрямую

**Конфигурация:** 4 воркера, wrk `--latency`, 4 потока, 10 секунд, прямое подключение (без proxy).

### /ping -- keep-alive ON

Чистая производительность HTTP-стека. Keep-alive исключает overhead TCP handshake, изолируя обработку запросов.

**t4/c100:**

| Сервис | RPS | Avg Latency | p50 | p99 |
|--------|----:|------------:|----:|----:|
| Nginx (v0) | 566 343 | 127us | 111us | 455us |
| **Apostol v2 (v5)** | **506 814** | **191us** | **170us** | **485us** |
| Go (v4) | 211 121 | 519us | 447us | 1,76ms |
| Apostol v1 (v1) | 127 652 | 787us | 790us | 1,16ms |
| Node.js (v3) | 101 509 | 1,03ms | 0,95ms | 2,19ms |
| Python (v2) | 2 399 | 41,29ms | 41,03ms | 43,12ms |

```
Nginx     ████████████████████████████████████████████████████████  566K
Apostol2  ██████████████████████████████████████████████████        507K
Go        █████████████████████                                    211K
Apostol1  █████████████                                            128K
Node.js   ██████████                                               102K
Python    ▏                                                          2K
```

**t4/c1000:**

| Сервис | RPS | Avg Latency | p50 | p99 |
|--------|----:|------------:|----:|----:|
| Nginx | 595 692 | 1,74ms | 1,72ms | 5,68ms |
| **Apostol v2** | **479 977** | **1,98ms** | **1,81ms** | **4,06ms** |
| Go | 204 109 | 4,88ms | 4,81ms | 8,23ms |
| Apostol v1 | 119 149 | 8,37ms | 8,40ms | 11,60ms |
| Node.js | 100 895 | 24,52ms | 9,43ms | 582,48ms |
| Python | 21 452 | 46,26ms | 44,89ms | 59,96ms |

При 1000 соединений Apostol v2 сохраняет 480K RPS с p99 = 4,06ms. Для сравнения: Node.js деградирует до p99 = 582ms.

### /ping -- keep-alive OFF

Каждый запрос создаёт новое TCP-соединение. Тест на эффективность обработки жизненного цикла соединений.

**t4/c100:**

| Сервис | RPS | Avg Latency | p50 | p99 |
|--------|----:|------------:|----:|----:|
| Nginx | 84 442 | 428us | 365us | 1,18ms |
| **Apostol v2** | **84 262** | **339us** | **318us** | **821us** |
| Go | 41 305 | 1,27ms | 772us | 9,14ms |
| Apostol v1 | 8 013 | 5,56ms | 3,70ms | 18,28ms |
| Node.js | 5 873 | 8,36ms | 6,41ms | 23,70ms |
| Python | 5 841 | 9,17ms | 8,53ms | 21,37ms |

**t4/c1000:**

| Сервис | RPS | Avg Latency | p50 | p99 |
|--------|----:|------------:|----:|----:|
| **Apostol v2** | **81 246** | **2,90ms** | **2,44ms** | **7,46ms** |
| Go | 79 670 | 6,84ms | 6,80ms | 16,20ms |
| Nginx | 74 786 | 3,92ms | 3,08ms | 11,49ms |
| Apostol v1 | 31 333 | 14,53ms | 14,06ms | 36,51ms |
| Node.js | 12 636 | 33,17ms | 32,24ms | 81,53ms |
| Python | 7 115 | 75,64ms | 73,04ms | 166,48ms |

Без keep-alive при c100 Nginx и Apostol v2 показывают **практически одинаковый throughput** (84,4K vs 84,3K) -- оба используют `SO_REUSEPORT` для распределения соединений на уровне ядра. При этом Apostol v2 достигает более низкой latency (339us avg vs 428us). При c1000 Apostol v2 выходит вперёд (81,2K vs 74,8K), что свидетельствует о более эффективной работе event loop при экстремальной нагрузке.

### /db/ping -- keep-alive ON

Тест с обращением к PostgreSQL. Наиболее близкий к production сценарий.

**t4/c100:**

| Сервис | RPS | Avg Latency | p50 | p99 |
|--------|----:|------------:|----:|----:|
| **Apostol v2** | **112 097** | **0,89ms** | **0,91ms** | **1,37ms** |
| Go | 72 004 | 1,93ms | 1,07ms | 13,53ms |
| Apostol v1 | 60 797 | 1,76ms | 1,61ms | 2,48ms |
| Node.js | 35 521 | 2,94ms | 2,65ms | 7,37ms |
| Python | 2 293 | 46,82ms | 42,00ms | 244,29ms |

Apostol v2 лидирует с 112K RPS -- **в 1,6 раза быстрее** Go (72K) и **в 1,8 раза быстрее** Apostol v1 (61K). Преимущество обеспечивается тесной интеграцией epoll и libpq: HTTP-парсинг, отправка запроса к PostgreSQL и формирование ответа происходят в одной итерации event loop с минимальным overhead.

### /db/ping -- keep-alive OFF

**t4/c100:**

| Сервис | RPS | Avg Latency | p50 | p99 |
|--------|----:|------------:|----:|----:|
| **Apostol v2** | **59 713** | **1,31ms** | **0,87ms** | **4,52ms** |
| Go | 11 871 | 4,58ms | 1,88ms | 18,40ms |
| Node.js | 5 251 | 10,13ms | 7,24ms | 26,99ms |
| Python | 5 158 | 11,49ms | 10,84ms | 24,96ms |
| Apostol v1 | 2 797 | 6,03ms | 2,93ms | 19,57ms |

Apostol v2 показывает 60K RPS -- **в 5,0 раз быстрее** Go и **в 21,3 раза быстрее** Apostol v1 -- с низкой и стабильной latency (p99 = 4,52ms). Сочетание `SO_REUSEPORT` и тесной интеграции event loop позволяет v2 обрабатывать TCP teardown и database round-trip одновременно без всплесков задержки.

---

## Результаты: через Nginx proxy

Все proxy-тесты маршрутизируют трафик через общий экземпляр Nginx: `loadgen -> Nginx -> сервис`. Конфигурация Nginx proxy использует `proxy_set_header Connection "close"`, что означает: каждый проксируемый запрос создаёт **новое TCP-соединение** к upstream-сервису. Эта конфигурация добавляет значительный overhead, но отражает распространённый production-паттерн деплоя.

### /ping через proxy -- keep-alive ON

**t4/c100:**

| Сервис | RPS | Avg Latency | p50 | p99 |
|--------|----:|------------:|----:|----:|
| **Apostol v2** | **69 682** | **1,76ms** | **1,07ms** | **5,64ms** |
| Go | 38 370 | 5,77ms | 2,06ms | 43,24ms |
| Apostol v1 | 16 444 | 11,74ms | 3,18ms | 54,16ms |
| Node.js | 5 889 | 21,49ms | 14,29ms | 73,65ms |
| Python | 5 473 | 22,80ms | 14,62ms | 83,66ms |

**t4/c1000:**

| Сервис | RPS | Avg Latency | p50 | p99 |
|--------|----:|------------:|----:|----:|
| **Apostol v2** | **39 090** | **34,92ms** | **20,71ms** | **141,98ms** |
| Go | 21 790 | 61,86ms | 37,46ms | 261,89ms |
| Apostol v1 | 14 621 | 79,48ms | 56,99ms | 274,02ms |
| Node.js | 9 662 | 111,68ms | 102,08ms | 338,13ms |
| Python | 8 809 | 119,80ms | 111,04ms | 314,21ms |

### /ping через proxy -- keep-alive OFF

**t4/c100:**

| Сервис | RPS | Avg Latency | p50 | p99 |
|--------|----:|------------:|----:|----:|
| **Apostol v2** | **29 310** | **3,07ms** | **1,22ms** | **22,43ms** |
| Apostol v1 | 5 746 | 24,98ms | 4,93ms | 139,92ms |
| Go | 3 503 | 13,93ms | 11,41ms | 42,03ms |
| Node.js | 3 174 | 15,82ms | 14,18ms | 41,23ms |
| Python | 2 659 | 28,34ms | 15,67ms | 134,27ms |

При keep-alive OFF каждый запрос требует двух TCP handshake (клиент->Nginx и Nginx->upstream). Apostol v2 уверенно лидирует с 29,3K RPS -- **в 5,1 раза быстрее** Apostol v1 и **в 8,4 раза быстрее** Go.

### /db/ping через proxy -- keep-alive ON

**t4/c100:**

| Сервис | RPS | Avg Latency | p50 | p99 |
|--------|----:|------------:|----:|----:|
| **Apostol v2** | **35 382** | **2,83ms** | **1,54ms** | **6,99ms** |
| Go | 13 593 | 12,32ms | 5,21ms | 53,13ms |
| Node.js | 5 337 | 28,34ms | 10,91ms | 108,10ms |
| Python | 4 376 | 31,72ms | 16,11ms | 206,63ms |
| Apostol v1 | 3 140 | 54,71ms | 23,21ms | 293,82ms |

Apostol v2 лидирует с 35,4K RPS -- **в 2,6 раза быстрее** Go (13,6K) со значительно лучшей стабильностью latency (p99 = 6,99ms vs 53,13ms).

### /db/ping через proxy -- keep-alive OFF

**t4/c100:**

| Сервис | RPS | Avg Latency | p50 | p99 |
|--------|----:|------------:|----:|----:|
| **Apostol v2** | **28 428** | **3,30ms** | **1,67ms** | **24,92ms** |
| Apostol v1 | 4 639 | 18,38ms | 10,64ms | 88,35ms |
| Go | 3 255 | 14,85ms | 14,52ms | 35,23ms |
| Node.js | 2 652 | 21,53ms | 18,03ms | 101,76ms |
| Python | 2 404 | 28,18ms | 20,17ms | 124,76ms |

При совмещении proxy-overhead и обращений к БД под нагрузкой с созданием новых соединений Apostol v2 показывает 28,4K RPS -- **в 8,7 раза быстрее** Go и **в 6,1 раза быстрее** Apostol v1.

---

## Анализ

### Apostol v2 vs Nginx

На `/ping` keep-alive ON Apostol v2 достигает **90% от throughput Nginx** (507K vs 566K). Учитывая, что Nginx отдаёт статические `return 200` без какой-либо логики, это впечатляющий результат для полноценного application-сервера.

На `/ping` keep-alive OFF оба сервиса используют `SO_REUSEPORT` и показывают **практически одинаковый throughput** при c100: 84,3K vs 84,4K. Разница находится в пределах погрешности измерений. При этом Apostol v2 достигает **более низкой latency** (339us avg vs 428us, p50 = 318us vs 365us). При c1000 Apostol v2 выходит вперёд и по throughput (81,2K vs 74,8K), что свидетельствует о более эффективной работе event loop при экстремальной нагрузке.

### Apostol v2 vs v1

Обе версии используют один и тот же архитектурный паттерн (однопоточный epoll event loop на воркер), но v2 показывает радикально более высокий throughput:

| Тест | v2 | v1 | Ускорение |
|------|---:|---:|----------:|
| /ping KA-ON c100 | 506 814 | 127 652 | **4,0x** |
| /ping KA-ON c1000 | 479 977 | 119 149 | **4,0x** |
| /ping KA-OFF c100 | 84 262 | 8 013 | **10,5x** |
| /db/ping KA-ON c100 | 112 097 | 60 797 | **1,8x** |
| /db/ping KA-OFF c100 | 59 713 | 2 797 | **21,3x** |

Ускорение в 4,0x на `/ping` keep-alive ON обеспечивается:

1. **llhttp** -- высокооптимизированный конечный автомат (vs рукописный парсер в libdelphi).
2. **C++20** -- `std::string_view`, move-семантика, zero-copy паттерны.
3. **Минимальный путь ответа** -- response builder пишет прямо в буфер сокета.
4. **Настроенный epoll** -- `MAX_EVENTS=512` позволяет получать больше событий за один syscall.

Ускорение в 10,5x на `/ping` keep-alive OFF дополнительно обеспечивается:

5. **`SO_REUSEPORT`** -- ядро распределяет соединения по воркерам (vs единый accept-сокет в v1).
6. **backlog=4096** -- v1 уже использовал `SOMAXCONN` (4096), но v2 был на 128 до выравнивания.

### Apostol v2 vs Go

Apostol v2 опережает Go во всех прямых тестах:

| Тест | Apostol v2 | Go | Разница |
|------|---:|---:|--------:|
| /ping KA-ON c100 | 506 814 | 211 121 | **2,4x** |
| /ping KA-OFF c100 | 84 262 | 41 305 | **2,0x** |
| /db/ping KA-ON c100 | 112 097 | 72 004 | **1,6x** |
| /db/ping KA-OFF c100 | 59 713 | 11 871 | **5,0x** |

Разрыв сужается на database-тестах с keep-alive (1,6x vs 2,4x), что подтверждает: PostgreSQL round-trip становится доминирующей стоимостью. На `/db/ping` без keep-alive преимущество v2 расширяется до 5,0x благодаря `SO_REUSEPORT` и эффективной обработке TCP teardown -- Go по умолчанию не использует `SO_REUSEPORT`.

### Overhead proxy

Nginx proxy добавляет значительный overhead из-за `Connection: close` к upstream, что вынуждает создавать новое TCP-соединение на каждый проксируемый запрос:

| Сервис | RPS напрямую | RPS через proxy | Сохранено |
|--------|---:|---:|---:|
| Apostol v2 | 506 814 | 69 682 | 14% |
| Go | 211 121 | 38 370 | 18% |
| Apostol v1 | 127 652 | 16 444 | 13% |
| Node.js | 101 509 | 5 889 | 6% |

Рейтинг абсолютного proxy-throughput повторяет прямой: Apostol v2 лидирует с 69,7K, за ним Go с 38,4K. Несмотря на сохранение лишь 14% от прямого throughput, запас скорости Apostol v2 достаточно велик, чтобы опережать все остальные сервисы через proxy.

На `/db/ping` через proxy Apostol v2 лидирует с 35,4K RPS vs 13,6K у Go, со значительно лучшей стабильностью latency (p99 = 7ms vs 53ms). При keep-alive OFF Apostol v2 сохраняет лидерство с 28,4K vs 3,3K.

### Влияние keep-alive

| Сервис | KA-ON (c100) | KA-OFF (c100) | Падение |
|--------|-------------:|---------------:|--------:|
| **Apostol v2** | **506 814** | **84 262** | **6,0x** |
| Nginx | 566 343 | 84 442 | 6,7x |
| Go | 211 121 | 41 305 | 5,1x |
| Apostol v1 | 127 652 | 8 013 | 15,9x |
| Node.js | 101 509 | 5 873 | 17,3x |
| Python | 2 399 | 5 841 | +143% |

Go показывает **наименьшее падение** (5,1x), за ним Apostol v2 (6,0x) и Nginx (6,7x) -- все три используют эффективную обработку соединений. Apostol v1 и Node.js теряют в 16--17 раз. Python растёт без keep-alive, потому что overhead Python runtime доминирует над стоимостью TCP.

### Node.js и Python

Node.js (Fastify + PM2) достигает 102K RPS на `/ping` -- конкурентоспособный результат для JavaScript-рантайма. Python (FastAPI + uvicorn) показывает 2,4K RPS при 100 соединениях, но масштабируется до 21K при 1000 соединениях по мере улучшения утилизации воркеров.

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

# Быстрый ping-бенчмарк — напрямую + через proxy (10 сек на тест)
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

Результаты получены с 4 воркерами, 10 секунд на тест, `wrk --latency`. Все сервисы используют `backlog=4096` и `ulimits.nofile=1048576`. Nginx и Apostol v2 используют `SO_REUSEPORT`. Результаты могут незначительно варьироваться между запусками в зависимости от нагрузки хоста.

---

## Исходный код

Весь код бенчмарка (Dockerfile, конфигурации, скрипты, исходники серверов) находится в каталоге [`benchmark/`](../benchmark/) данного репозитория.
