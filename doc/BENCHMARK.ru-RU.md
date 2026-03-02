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

**Nginx** -- baseline (статические JSON-ответы) и API gateway для proxy-тестов. `worker_processes auto`, `keepalive_requests 10000`, логирование отключено.

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
| Nginx (v0) | 543 131 | 146us | 111us | 579us |
| **Apostol v2 (v5)** | **485 521** | **199us** | **184us** | **498us** |
| Go (v4) | 211 891 | 517us | 446us | 1,74ms |
| Apostol v1 (v1) | 126 096 | 790us | 768us | 1,16ms |
| Node.js (v3) | 101 891 | 1,03ms | 0,94ms | 2,27ms |
| Python (v2) | 2 418 | 41,25ms | 41,03ms | 42,99ms |

```
Nginx     ████████████████████████████████████████████████████████  543K
Apostol2  ██████████████████████████████████████████████████████     486K
Go        ██████████████████████                                    212K
Apostol1  █████████████                                             126K
Node.js   ██████████                                                102K
Python    ▏                                                           2K
```

**t4/c1000:**

| Сервис | RPS | Avg Latency | p50 | p99 |
|--------|----:|------------:|----:|----:|
| Nginx | 580 145 | 1,83ms | 1,79ms | 5,85ms |
| **Apostol v2** | **515 234** | **1,87ms** | **1,72ms** | **3,69ms** |
| Go | 215 164 | 4,63ms | 4,56ms | 7,88ms |
| Apostol v1 | 117 543 | 8,46ms | 8,78ms | 12,61ms |
| Node.js | 104 400 | 21,81ms | 9,15ms | 510,49ms |
| Python | 21 904 | 45,41ms | 44,97ms | 59,94ms |

При 1000 соединений Apostol v2 сохраняет 515K RPS с p99 = 3,69ms -- **ещё ниже**, чем p99 при 100 соединениях. Для сравнения: Node.js деградирует до p99 = 510ms.

### /ping -- keep-alive OFF

Каждый запрос создаёт новое TCP-соединение. Тест на эффективность обработки жизненного цикла соединений.

**t4/c100:**

| Сервис | RPS | Avg Latency | p50 | p99 |
|--------|----:|------------:|----:|----:|
| **Apostol v2** | **88 327** | **330us** | **312us** | **802us** |
| Nginx | 85 693 | 428us | 367us | 1,18ms |
| Go | 38 892 | 1,31ms | 755us | 9,03ms |
| Apostol v1 | 8 069 | 5,56ms | 4,29ms | 16,89ms |
| Node.js | 6 420 | 7,58ms | 6,12ms | 19,81ms |
| Python | 6 259 | 8,55ms | 8,04ms | 20,10ms |

**t4/c1000:**

| Сервис | RPS | Avg Latency | p50 | p99 |
|--------|----:|------------:|----:|----:|
| **Apostol v2** | **82 492** | **2,88ms** | **2,41ms** | **7,40ms** |
| Go | 79 118 | 6,44ms | 6,11ms | 16,32ms |
| Nginx | 76 846 | 3,97ms | 3,12ms | 11,72ms |
| Apostol v1 | 29 955 | 15,10ms | 14,65ms | 39,10ms |
| Node.js | 12 608 | 33,80ms | 33,44ms | 83,92ms |
| Python | 7 176 | 75,17ms | 72,55ms | 169,60ms |

Без keep-alive **Apostol v2 обгоняет Nginx** при обоих уровнях конкурентности. При c100: 88,3K vs 85,7K. При c1000: 82,5K vs 76,8K. Это возможно благодаря `SO_REUSEPORT` -- ядро распределяет входящие соединения по воркерам, минуя сериализацию accept в Nginx.

### /db/ping -- keep-alive ON

Тест с обращением к PostgreSQL. Наиболее близкий к production сценарий.

**t4/c100:**

| Сервис | RPS | Avg Latency | p50 | p99 |
|--------|----:|------------:|----:|----:|
| **Apostol v2** | **109 106** | **0,92ms** | **0,93ms** | **1,56ms** |
| Go | 73 166 | 1,82ms | 1,04ms | 12,07ms |
| Apostol v1 | 60 238 | 1,95ms | 1,66ms | 2,85ms |
| Node.js | 38 229 | 2,73ms | 2,46ms | 6,62ms |
| Python | 2 288 | 47,08ms | 42,01ms | 242,24ms |

Apostol v2 лидирует с 109K RPS -- **в 1,5 раза быстрее** Go (73K) и **в 1,8 раза быстрее** Apostol v1 (60K). Преимущество обеспечивается тесной интеграцией epoll и libpq: HTTP-парсинг, отправка запроса к PostgreSQL и формирование ответа происходят в одной итерации event loop с минимальным overhead.

### /db/ping -- keep-alive OFF

**t4/c100:**

| Сервис | RPS | Avg Latency | p50 | p99 |
|--------|----:|------------:|----:|----:|
| **Apostol v2** | **60 210** | **1,31ms** | **0,88ms** | **4,67ms** |
| Go | 11 315 | 4,78ms | 1,90ms | 19,49ms |
| Node.js | 5 092 | 10,43ms | 7,30ms | 29,77ms |
| Python | 4 615 | 13,05ms | 11,73ms | 39,65ms |
| Apostol v1 | 2 796 | 6,05ms | 3,01ms | 19,44ms |

Apostol v2 показывает 60K RPS -- **в 5,3 раза быстрее** Go и **в 21,5 раза быстрее** Apostol v1 -- с низкой и стабильной latency (p99 = 4,67ms). Сочетание `SO_REUSEPORT` и тесной интеграции event loop позволяет v2 обрабатывать TCP teardown и database round-trip одновременно без всплесков задержки.

---

## Результаты: через Nginx proxy

Все proxy-тесты маршрутизируют трафик через общий экземпляр Nginx: `loadgen -> Nginx -> сервис`. Конфигурация Nginx proxy использует `proxy_set_header Connection "close"`, что означает: каждый проксируемый запрос создаёт **новое TCP-соединение** к upstream-сервису. Эта конфигурация добавляет значительный overhead, но отражает распространённый production-паттерн деплоя.

### /ping через proxy -- keep-alive ON

**t4/c100:**

| Сервис | RPS | Avg Latency | p50 | p99 |
|--------|----:|------------:|----:|----:|
| **Apostol v2** | **59 586** | **1,73ms** | **1,76ms** | **4,38ms** |
| Go | 39 360 | 5,27ms | 2,02ms | 49,14ms |
| Apostol v1 | 13 032 | 15,99ms | 5,00ms | 72,04ms |
| Node.js | 6 639 | 22,17ms | 11,18ms | 86,64ms |
| Python | 5 353 | 22,37ms | 16,01ms | 73,23ms |

**t4/c1000:**

| Сервис | RPS | Avg Latency | p50 | p99 |
|--------|----:|------------:|----:|----:|
| **Apostol v2** | **34 277** | **42,58ms** | **24,36ms** | **185,68ms** |
| Go | 21 991 | 59,02ms | 38,31ms | 237,30ms |
| Apostol v1 | 15 420 | 79,10ms | 55,56ms | 285,15ms |
| Node.js | 10 873 | 98,84ms | 83,65ms | 284,41ms |
| Python | 8 406 | 121,82ms | 125,66ms | 303,27ms |

### /ping через proxy -- keep-alive OFF

**t4/c100:**

| Сервис | RPS | Avg Latency | p50 | p99 |
|--------|----:|------------:|----:|----:|
| **Apostol v2** | **28 716** | **2,79ms** | **1,27ms** | **17,76ms** |
| Apostol v1 | 6 103 | 24,17ms | 5,11ms | 123,95ms |
| Go | 4 640 | 10,51ms | 9,34ms | 32,18ms |
| Node.js | 4 003 | 13,53ms | 11,32ms | 68,02ms |
| Python | 3 006 | 27,03ms | 14,82ms | 126,41ms |

При keep-alive OFF каждый запрос требует двух TCP handshake (клиент->Nginx и Nginx->upstream). Apostol v2 уверенно лидирует с 28,7K RPS -- **в 4,7 раза быстрее** Apostol v1 и **в 6,2 раза быстрее** Go.

### /db/ping через proxy -- keep-alive ON

**t4/c100:**

| Сервис | RPS | Avg Latency | p50 | p99 |
|--------|----:|------------:|----:|----:|
| Go | 36 310 | 4,28ms | 1,37ms | 51,45ms |
| **Apostol v2** | **31 750** | **2,59ms** | **1,64ms** | **6,99ms** |
| Node.js | 5 675 | 21,52ms | 15,88ms | 69,97ms |
| Python | 5 154 | 25,37ms | 12,18ms | 205,65ms |
| Apostol v1 | 3 356 | 48,13ms | 31,20ms | 223,42ms |

Go обходит по RPS (36,3K vs 31,8K) при проксировании с запросами к БД. Однако Apostol v2 показывает радикально лучшую стабильность latency: p99 = 6,99ms vs 51,45ms у Go.

### /db/ping через proxy -- keep-alive OFF

**t4/c100:**

| Сервис | RPS | Avg Latency | p50 | p99 |
|--------|----:|------------:|----:|----:|
| **Apostol v2** | **27 075** | **3,17ms** | **1,78ms** | **20,01ms** |
| Apostol v1 | 4 521 | 18,43ms | 9,61ms | 96,42ms |
| Go | 3 784 | 12,85ms | 12,51ms | 31,01ms |
| Node.js | 2 980 | 21,48ms | 15,83ms | 118,42ms |
| Python | 2 544 | 27,67ms | 18,76ms | 130,32ms |

При совмещении proxy-overhead и обращений к БД под нагрузкой с созданием новых соединений Apostol v2 показывает 27K RPS -- **в 7,2 раза быстрее** Go и **в 6,0 раз быстрее** Apostol v1.

---

## Анализ

### Apostol v2 vs Nginx

На `/ping` keep-alive ON Apostol v2 достигает **89% от throughput Nginx** (486K vs 543K). Учитывая, что Nginx отдаёт статические `return 200` без какой-либо логики, это впечатляющий результат для полноценного application-сервера.

На `/ping` keep-alive OFF Apostol v2 **обгоняет Nginx** при обоих уровнях конкурентности: 88,3K vs 85,7K (c100) и 82,5K vs 76,8K (c1000). Преимущество обеспечивается `SO_REUSEPORT` -- ядро распределяет входящие соединения напрямую по воркерам, минуя сериализацию accept в Nginx.

### Apostol v2 vs v1

Обе версии используют один и тот же архитектурный паттерн (однопоточный epoll event loop на воркер), но v2 показывает радикально более высокий throughput:

| Тест | v2 | v1 | Ускорение |
|------|---:|---:|----------:|
| /ping KA-ON c100 | 485 521 | 126 096 | **3,9x** |
| /ping KA-ON c1000 | 515 234 | 117 543 | **4,4x** |
| /ping KA-OFF c100 | 88 327 | 8 069 | **10,9x** |
| /db/ping KA-ON c100 | 109 106 | 60 238 | **1,8x** |
| /db/ping KA-OFF c100 | 60 210 | 2 796 | **21,5x** |

Ускорение в 3,9x на `/ping` keep-alive ON обеспечивается:

1. **llhttp** -- высокооптимизированный конечный автомат (vs рукописный парсер в libdelphi).
2. **C++20** -- `std::string_view`, move-семантика, zero-copy паттерны.
3. **Минимальный путь ответа** -- response builder пишет прямо в буфер сокета.
4. **Настроенный epoll** -- `MAX_EVENTS=512` позволяет получать больше событий за один syscall.

Ускорение в 10,9x на `/ping` keep-alive OFF дополнительно обеспечивается:

5. **`SO_REUSEPORT`** -- ядро распределяет соединения по воркерам (vs единый accept-сокет в v1).
6. **backlog=4096** -- v1 уже использовал `SOMAXCONN` (4096), но v2 был на 128.

### Apostol v2 vs Go

Apostol v2 опережает Go во всех прямых тестах:

| Тест | Apostol v2 | Go | Разница |
|------|---:|---:|--------:|
| /ping KA-ON c100 | 485 521 | 211 891 | **2,3x** |
| /ping KA-OFF c100 | 88 327 | 38 892 | **2,3x** |
| /db/ping KA-ON c100 | 109 106 | 73 166 | **1,5x** |
| /db/ping KA-OFF c100 | 60 210 | 11 315 | **5,3x** |

Разрыв сужается на database-тестах с keep-alive (1,5x vs 2,3x), что подтверждает: PostgreSQL round-trip становится доминирующей стоимостью. На `/db/ping` без keep-alive преимущество v2 расширяется до 5,3x благодаря `SO_REUSEPORT` и эффективной обработке TCP teardown.

### Overhead proxy

Nginx proxy добавляет значительный overhead из-за `Connection: close` к upstream, что вынуждает создавать новое TCP-соединение на каждый проксируемый запрос:

| Сервис | RPS напрямую | RPS через proxy | Сохранено |
|--------|---:|---:|---:|
| Apostol v2 | 485 521 | 59 586 | 12% |
| Go | 211 891 | 39 360 | 19% |
| Apostol v1 | 126 096 | 13 032 | 10% |
| Node.js | 101 891 | 6 639 | 7% |

Рейтинг абсолютного proxy-throughput повторяет прямой: Apostol v2 лидирует с 59,6K, за ним Go с 39,4K. Несмотря на сохранение лишь 12% от прямого throughput, запас скорости Apostol v2 достаточно велик, чтобы опережать все остальные сервисы через proxy.

На `/db/ping` через proxy Go обходит по RPS (36,3K vs 31,8K), но Apostol v2 показывает значительно лучшую стабильность latency (p99 = 7ms vs 51ms). При keep-alive OFF Apostol v2 возвращает лидерство по throughput с большим отрывом (27K vs 3,8K).

### Влияние keep-alive

| Сервис | KA-ON (c100) | KA-OFF (c100) | Падение |
|--------|-------------:|---------------:|--------:|
| **Apostol v2** | **485 521** | **88 327** | **5,5x** |
| Nginx | 543 131 | 85 693 | 6,3x |
| Go | 211 891 | 38 892 | 5,5x |
| Apostol v1 | 126 096 | 8 069 | 15,6x |
| Node.js | 101 891 | 6 420 | 15,9x |
| Python | 2 418 | 6 259 | +159% |

Apostol v2 и Go показывают **наименьшее падение** (5,5x), обогнав Nginx (6,3x). Для сравнения: Apostol v1 и Node.js теряют в 15--16 раз. Python растёт без keep-alive, потому что overhead Python runtime доминирует над стоимостью TCP.

### Node.js и Python

Node.js (Fastify + PM2) достигает 102K RPS на `/ping` -- конкурентоспособный результат для JavaScript-рантайма. Python (FastAPI + uvicorn) показывает 2,4K RPS при 100 соединениях, но масштабируется до 22K при 1000 соединениях по мере улучшения утилизации воркеров.

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

Результаты получены с 4 воркерами, 10 секунд на тест, `wrk --latency`. Все сервисы используют `backlog=4096` и `ulimits.nofile=1048576`. Результаты могут незначительно варьироваться между запусками в зависимости от нагрузки хоста.

---

## Исходный код

Весь код бенчмарка (Dockerfile, конфигурации, скрипты, исходники серверов) находится в каталоге [`benchmark/`](../benchmark/) данного репозитория.
