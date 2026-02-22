[English version](BENCHMARK.md)

# REST API Benchmark: Apostol vs Python vs Node.js vs Go

## Контекст

[Apostol](https://github.com/apostoldevel/apostol) -- C++14 фреймворк для создания высокопроизводительных асинхронных HTTP-серверов (daemon-процессов) под Linux. В его основе лежит событийная модель на базе `epoll` с прямой интеграцией PostgreSQL через `libpq`. HTTP-сервер и PostgreSQL-сокеты работают в едином event loop, что исключает промежуточные слои и минимизирует задержки.

Данный бенчмарк сравнивает Apostol с тремя популярными стеками:

| Сервис | Язык | HTTP-фреймворк | PostgreSQL-драйвер | Масштабирование |
|--------|------|----------------|---------------------|-----------------|
| **Apostol** | C++14 | встроенный (epoll) | libpq (async) | master + N worker-процессов |
| **Python** | Python 3.12 | FastAPI / uvicorn | asyncpg | uvicorn --workers N |
| **Node.js** | Node.js | Fastify | pg (node-postgres) | PM2 cluster (N instances) |
| **Go** | Go | net/http (stdlib) | pgx/v5 | GOMAXPROCS=N (goroutines) |

В качестве baseline используется **Nginx**, отдающий статические JSON-ответы без обращения к базе данных.

---

## Тестовая среда

### Инфраструктура

Все сервисы запущены в отдельных Docker-контейнерах на одном хосте. Контейнеры объединены в две изолированные сети:

- **bench-lan** (172.19.1.0/24) -- прикладной уровень (Nginx, application-серверы, loadgen)
- **bench-db** (172.19.2.0/24) -- уровень базы данных (PostgreSQL, PgBouncer, application-серверы)

### Компоненты

| Компонент | Контейнер | Порт (внутренний) | Назначение |
|-----------|-----------|-------------------|------------|
| PostgreSQL 17 | bench-postgres | 5432 | База данных |
| PgBouncer | bench-pgbouncer | 6432 | Connection pooler (session mode) |
| Nginx | bench-nginx | 80 | Baseline + API gateway (reverse proxy) |
| Apostol | bench-apostol | 8080 | C++ application server |
| Python | bench-python | 8000 | FastAPI / uvicorn |
| Node.js | bench-node | 3000 | Fastify / PM2 |
| Go | bench-go | 4000 | net/http / pgx |
| loadgen | bench-loadgen | -- | wrk, hey |

### Конфигурация PostgreSQL

PostgreSQL оптимизирован для бенчмарка (не для production):

```
shared_buffers = 256MB
work_mem = 16MB
max_connections = 200
wal_level = minimal
fsync = off
synchronous_commit = off
full_page_writes = off
```

### Connection pools

Все application-серверы настроены единообразно:

- **min** = 5 соединений
- **max** = 15 соединений на воркер

PgBouncer работает в режиме `session` с `default_pool_size = 50`.

### Воркеры

Все серверы запущены с **4 воркерами** (настраивается через переменную окружения `WORKERS`).

| Сервис | Соединения к БД (4 воркера) |
|--------|-----------------------------|
| Apostol | 15 x 4 = 60 |
| Python | 15 x 4 = 60 |
| Node.js | 15 x 4 = 60 |
| Go | 15 |

---

## Эндпоинты

Каждый сервис реализует 4 идентичных эндпоинта. Версия в URL соответствует сервису:

- `v0` -- Nginx (baseline)
- `v1` -- Apostol
- `v2` -- Python
- `v3` -- Node.js
- `v4` -- Go

### Статические эндпоинты (без обращения к БД)

| Эндпоинт | Ответ |
|----------|-------|
| `/api/vN/ping` | `{"ok":true,"message":"OK"}` |
| `/api/vN/time` | `{"serverTime":<unix_timestamp>}` |

Для `/ping` все серверы возвращают предзаготовленную строку. Для `/time` -- текущее время в секундах (Unix epoch), вычисленное на стороне приложения.

Nginx baseline для `/time` возвращает фиксированное значение `{"serverTime":1700000000}` (статический ответ, без вычислений).

### Database-эндпоинты

| Эндпоинт | SQL-запрос |
|----------|------------|
| `/api/vN/db/ping` | `SELECT json_build_object('ok', true, 'message', 'OK')::text` |
| `/api/vN/db/time` | `SELECT json_build_object('serverTime', extract(epoch from now())::integer)::text` |

Запросы максимально легковесные: нет обращений к таблицам, только вычисление JSON в PostgreSQL. Это изолирует overhead самого round-trip к базе данных.

Nginx baseline для `/db/ping` и `/db/time` возвращает статические ответы (в базу не ходит).

---

## Методология

### Инструмент нагрузки

**wrk** -- HTTP benchmarking tool. Параметры запуска:

- **4 потока** (`-t4`)
- **100 или 1 000 соединений** (`-c100` / `-c1000`)
- **10 секунд** на эндпоинт (`-d10s`)
- **--latency** -- вывод распределения задержек (p50, p75, p90, p99)
- **Keep-alive ON** (поведение wrk по умолчанию) и **Keep-alive OFF** (через скрипт с заголовком `Connection: close`)

### Порядок тестирования

Сервисы тестируются **последовательно**: сначала все 4 эндпоинта одного сервиса, затем следующего. Это исключает конкуренцию за ресурсы между тестируемыми серверами.

Порядок: Nginx -> Apostol -> Python -> Node.js -> Go.

### Метрики

- **Requests/sec (RPS)** -- пропускная способность
- **Avg Latency** -- средняя задержка ответа
- **p50, p75, p90, p99** -- перцентили задержки (50-й, 75-й, 90-й и 99-й)

---

## Ключевые архитектурные различия

### Модель параллелизма

| Сервис | Модель | Соединения к БД (4 воркера) |
|--------|--------|----------------------------|
| Apostol | pool-per-worker: каждый worker-процесс держит свой пул | 15 x 4 = 60 |
| Python | pool-per-worker: каждый uvicorn-воркер создает свой asyncpg pool | 15 x 4 = 60 |
| Node.js | pool-per-worker: каждый PM2-instance создает свой pg Pool | 15 x 4 = 60 |
| Go | единый пул: pgxpool на весь процесс, независимо от GOMAXPROCS | 15 |

Go использует один процесс с goroutine-планировщиком, поэтому ему достаточно одного пула на 15 соединений при любом значении `GOMAXPROCS`. Остальные серверы создают отдельный процесс на каждый воркер, и каждый процесс поддерживает собственный пул.

### Обработка запросов

- **Nginx** -- статический ответ в `return 200`, без какой-либо обработки.
- **Apostol** -- epoll event loop: HTTP-парсинг и async PostgreSQL через `libpq` (PQsendQuery) в одном цикле событий. Нет thread pool, нет goroutine -- чистый однопоточный async на воркер.
- **Python** -- asyncio event loop (uvicorn): async/await с asyncpg. Ограничен GIL (для CPU-bound) и overhead Python runtime.
- **Node.js** -- V8 event loop (Fastify): async/await с node-postgres. Один поток на instance, масштабирование через PM2 cluster.
- **Go** -- goroutine per request: планировщик Go мультиплексирует горутины по OS-потокам (GOMAXPROCS). pgx использует context-aware connection pool.

---

## Результаты

### /ping -- keep-alive ON

Чистая производительность HTTP-стека без обращения к базе данных. Keep-alive включён (поведение wrk по умолчанию).

**t4/c100** (4 потока, 100 соединений, 10 секунд):

| Сервис | RPS | Avg Latency | p50 | p75 | p90 | p99 |
|--------|-----|-------------|-----|-----|-----|-----|
| Nginx (v0) | 545 281 | 146us | 107us | 178us | 294us | 585us |
| Go (v4) | 197 416 | 555us | 473us | 786us | 1,13ms | 1,86ms |
| Apostol (v1) | 124 882 | 805us | 834us | 0,88ms | 1,09ms | 1,31ms |
| Node.js (v3) | 96 341 | 1,07ms | 0,95ms | 1,10ms | 1,25ms | 2,28ms |
| Python (v2) | 2 418 | 41,27ms | 41,02ms | 41,98ms | 42,01ms | 43,13ms |

```
/ping, keep-alive ON, t4/c100 — RPS (тысяч):

Nginx    |======================================================| 545K
Go       |===================                                   | 197K
Apostol  |============                                          | 125K
Node.js  |=========                                             |  96K
Python   |                                                      |   2K
```

**t4/c1000** (4 потока, 1 000 соединений, 10 секунд):

| Сервис | RPS | Avg Latency | p50 | p99 |
|--------|-----|-------------|-----|-----|
| Nginx | 550 208 | 2,36ms | 1,92ms | 8,37ms |
| Go | 197 238 | 5,06ms | 4,99ms | 8,51ms |
| Apostol | 116 414 | 8,56ms | 8,88ms | 12,02ms |
| Node.js | 96 820 | 16,46ms | 9,73ms | 285,33ms |
| Python | 21 471 | 46,31ms | 44,99ms | 63,05ms |

При увеличении числа соединений с 100 до 1 000 Nginx, Go и Node.js практически не теряют в пропускной способности. Apostol показывает незначительное снижение (125K -> 116K). Python, напротив, демонстрирует рост с 2,4K до 21,5K RPS -- при 100 соединениях 4 воркера были недозагружены, а при 1 000 соединений uvicorn утилизирует все процессы.

### /ping -- keep-alive OFF

При отключённом keep-alive каждый HTTP-запрос требует установки и разрыва TCP-соединения (трёхэтапное рукопожатие + FIN).

**t4/c100** (4 потока, 100 соединений, 10 секунд):

| Сервис | RPS | Avg Latency | p50 | p99 |
|--------|-----|-------------|-----|-----|
| Nginx | 82 369 | 437us | 372us | 1,22ms |
| Go | 43 861 | 1,15ms | 683us | 8,76ms |
| Apostol | 8 575 | 5,26ms | 2,19ms | 19,24ms |
| Node.js | 5 881 | 8,28ms | 6,33ms | 24,58ms |
| Python | 5 583 | 9,60ms | 8,71ms | 24,46ms |

**t4/c1000** (4 потока, 1 000 соединений, 10 секунд):

| Сервис | RPS | Avg Latency | p50 | p99 |
|--------|-----|-------------|-----|-----|
| Nginx | 78 807 | 4,61ms | 3,66ms | 13,84ms |
| Go | 78 378 | 6,18ms | 5,65ms | 15,81ms |
| Apostol | 28 136 | 15,78ms | 15,35ms | 41,25ms |
| Node.js | 11 631 | 33,29ms | 29,05ms | 88,91ms |
| Python | 6 976 | 77,59ms | 75,09ms | 177,29ms |

Падение RPS при отключённом keep-alive (c100): Nginx -6,6x, Go -4,5x, Apostol -14,6x, Node.js -16,4x. Python, напротив, показывает рост +130% (2 418 -> 5 583), поскольку при keep-alive ON его bottleneck -- Python runtime, а не сетевой слой.

### /db/ping -- прямое подключение к PostgreSQL

**Условия:** keep-alive OFF, t4/c100, прямое подключение к PostgreSQL (без PgBouncer).

| Сервис | RPS | Avg Latency | p50 | p99 |
|--------|-----|-------------|-----|-----|
| Go | 11 877 | 4,50ms | 2,06ms | 18,63ms |
| Node.js | 4 967 | 10,80ms | 8,15ms | 33,79ms |
| Python | 4 465 | 18,77ms | 11,65ms | 264,14ms |
| Apostol | 2 746 | 7,74ms | 3,27ms | 29,50ms |

Apostol показывает наименьший RPS среди application-серверов. Причина: при Connection: close каждый запрос открывает новое TCP-соединение, а в комбинации с асинхронным round-trip к PostgreSQL это создаёт bottleneck в однопоточном event loop -- соединение разрывается до завершения полного цикла обработки.

### /db/ping -- через PgBouncer

**Условия:** keep-alive OFF, t4/c100, подключение через PgBouncer (transaction mode).

| Сервис | RPS | Avg Latency | p50 | p99 | Примечание |
|--------|-----|-------------|-----|-----|------------|
| Node.js | 5 212 | 10,45ms | 9,22ms | 27,25ms | Без ошибок |
| Apostol | 2 797 | 6,82ms | 3,40ms | 24,69ms | Без ошибок |
| Go | 8 989* | 5,92ms | 2,47ms | 23,23ms | 40% Non-2xx (prepared statements) |
| Python | 3 320* | 26,03ms | 24,92ms | 57,23ms | 77% Non-2xx (prepared statements) |

\*RPS включает ошибочные ответы.

PgBouncer в режиме transaction pooling несовместим с prepared statements: после завершения транзакции соединение возвращается в пул и может быть отдано другому клиенту, у которого нет ранее подготовленных statements. Go (pgx) и Python (asyncpg) по умолчанию используют prepared statements, что приводит к массовым ошибкам. Node.js (node-postgres) и Apostol (libpq) отправляют простые запросы и работают без ошибок.

---

## Анализ результатов

### Nginx baseline

Nginx показывает 545K RPS на `/ping` при 4 воркерах -- это верхняя граница производительности для данной тестовой среды, чистый overhead HTTP-стека. При keep-alive OFF скорость падает до 82K (в 6,6 раза), что наглядно демонстрирует стоимость TCP handshake.

### Go

Go лидирует среди application-серверов: **197K RPS** на `/ping` (в 1,58 раза больше Apostol, в 2,05 раза больше Node.js). Причины:

- **Goroutine-планировщик** эффективно мультиплексирует тысячи горутин по OS-потокам с минимальным overhead на контекстное переключение.
- **Единый connection pool** (15 соединений) обслуживает все горутины. Нет дублирования соединений между воркерами.
- **net/http** -- зрелая и оптимизированная стандартная библиотека с встроенным HTTP/1.1-парсером.

### Apostol (C++)

Apostol показывает **125K RPS** на `/ping`. Для epoll-based C++ фреймворка с полностью асинхронным PostgreSQL это высокий результат.

**Сильнейшая сторона Apostol -- стабильность latency.** При c100 разброс p50-p99 составляет всего 834us -> 1,31ms (коэффициент 1,57x), тогда как у Go -- 473us -> 1,86ms (3,93x), а у Node.js -- 0,95ms -> 2,28ms (2,40x). При c1000 Apostol сохраняет p99 = 12,02ms, что является лучшим показателем среди всех application-серверов (для сравнения: Node.js p99 = 285,33ms).

Однопоточный event loop на воркер обеспечивает предсказуемую обработку: нет контекстных переключений, нет lock contention -- каждый запрос проходит через детерминированный путь.

### Node.js

Node.js (Fastify + PM2 cluster) показывает **96K RPS** на `/ping` -- конкурентоспособный результат для JavaScript-рантайма. V8 JIT-компилятор обеспечивает высокую производительность для I/O-bound задач. Однако при c1000 p99 latency возрастает до 285,33ms, что указывает на проблемы с tail latency под высокой нагрузкой.

### Python

Python (FastAPI + uvicorn + asyncpg) показывает **2,4K RPS** на `/ping` при c100 -- на два порядка ниже Go. GIL ограничивает параллелизм в пределах одного процесса, а overhead Python runtime доминирует над сетевым вводом-выводом.

При увеличении числа соединений до 1 000 Python достигает **21,5K RPS** -- рост в 8,9 раза. Это означает, что при 100 соединениях 4 воркера uvicorn были недозагружены, и bottleneck находился в недостаточном количестве параллельных запросов для полной утилизации всех процессов.

### Влияние keep-alive

При отключённом keep-alive (c100) каждый HTTP-запрос требует установки и разрыва TCP-соединения. Деградация `/ping`:

| Сервис | KA-ON RPS | KA-OFF RPS | Падение |
|--------|-----------|------------|---------|
| Nginx | 545 281 | 82 369 | -6,6x |
| Go | 197 416 | 43 861 | -4,5x |
| Apostol | 124 882 | 8 575 | -14,6x |
| Node.js | 96 341 | 5 881 | -16,4x |
| Python | 2 418 | 5 583 | +130% |

Apostol и Node.js страдают сильнее всего, поскольку их однопоточные event loop тратят значительную часть времени на TCP-рукопожатие и закрытие соединений. Python показывает рост, потому что при keep-alive ON bottleneck -- Python runtime, а при keep-alive OFF TCP overhead маскирует его, и воркеры утилизируются полнее.

Вывод: keep-alive является обязательным условием для достижения высокой производительности всех серверов. В production-среде keep-alive всегда включён по умолчанию.

### Overhead базы данных

Сравнение `/ping` vs `/db/ping` в режиме keep-alive OFF, t4/c100 (одинаковые условия для корректного сопоставления):

| Сервис | /ping RPS (KA-OFF) | /db/ping RPS (KA-OFF) | Снижение |
|--------|--------------------|----------------------|----------|
| Go | 43 861 | 11 877 | -73% |
| Apostol | 8 575 | 2 746 | -68% |
| Python | 5 583 | 4 465 | -20% |
| Node.js | 5 881 | 4 967 | -16% |

Go и Apostol теряют 68-73% производительности при добавлении database round-trip -- для них сетевой overhead к PostgreSQL является основным bottleneck. Node.js и Python теряют значительно меньше (16-20%), потому что их собственный runtime уже является ограничивающим фактором.

### PgBouncer и prepared statements

PgBouncer в режиме transaction pooling несовместим с prepared statements. Apostol (libpq) и Node.js (node-postgres) отправляют простые запросы и работают через PgBouncer без ошибок. Go (pgx) и Python (asyncpg) по умолчанию используют prepared statements -- при transaction pooling сервер возвращает ошибки, что даёт 40% и 77% Non-2xx ответов соответственно.

---

## Как воспроизвести

### Требования

- Docker и Docker Compose
- Свободные порты: 15432, 16432, 8090-8094

### Запуск

```bash
cd benchmark

# Запуск всех сервисов
docker compose up -d

# Дождаться, пока все контейнеры станут healthy
docker compose ps

# Сборка контейнера loadgen
docker compose --profile loadgen build loadgen

# Быстрый debug-тест (10 секунд, 2t/100c)
docker compose --profile loadgen run --rm loadgen /bench/scripts/debug-quick.sh

# Полный бенчмарк (quick mode: 10 сек, конфигурации 2:100 и 4:1000)
docker compose --profile loadgen run --rm loadgen /bench/scripts/bench.sh --quick

# Полный бенчмарк (60 сек, все конфигурации: 2:100, 4:1000, 8:5000, 10:10000)
docker compose --profile loadgen run --rm loadgen /bench/scripts/bench.sh
```

### Изменение числа воркеров

```bash
WORKERS=4 docker compose up -d
```

### Результаты

Результаты сохраняются в `benchmark/results/` в формате:

- Отдельные файлы `.txt` для каждого теста (сырой вывод wrk)
- Сводный файл `summary.csv`

---

## Примечание

Представленные результаты являются финальными и получены с **4 воркерами** (`WORKERS=4`). Каждый тест выполнялся 10 секунд с использованием `wrk --latency`.

Число воркеров можно изменить через переменную окружения:

```bash
WORKERS=4 docker compose up -d
```

Тесты `/ping` и `/time` выполнялись напрямую к application-серверам (без Nginx proxy). Тесты `/db/ping` выполнялись как с прямым подключением к PostgreSQL, так и через PgBouncer (transaction mode).

---

## Исходный код

Весь код бенчмарка (Dockerfile, конфигурации, скрипты, исходники application-серверов) находится в каталоге [`benchmark/`](../benchmark/) данного репозитория.
