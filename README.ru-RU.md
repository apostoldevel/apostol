[![en](https://img.shields.io/badge/lang-en-green.svg)](README.md)

<img width="1322" height="506" alt="apostol" src="https://github.com/user-attachments/assets/fffd8315-9d97-4624-b949-57f31519a8da" />

# Апостол (A-POST-OL)

**Апостол** — шаблон-проект (демо-сборка) для C++20 фреймворка [libapostol](https://github.com/apostoldevel/libapostol) — высокопроизводительной платформы для разработки серверных приложений и системных служб под Linux с прямым доступом к PostgreSQL.

> A-POST-OL -> Asynchronous POST Orchestration Loop — единый цикл обработки событий (event loop) для HTTP и PostgreSQL.

---

## Оглавление

- [Описание](#описание)
- [Модули](#модули)
    - [Модули в данной сборке](#данная-сборка-включает-три-модуля)
    - [Дополнительные модули](#дополнительные-модули)
- [Проекты](#проекты)
- [Архитектура](#архитектура)
- [Быстрый старт с Docker](#быстрый-старт-с-docker)
- [Сборка из исходников](#сборка-из-исходников)
    - [Зависимости](#зависимости-debianubuntu)
    - [Клонирование и сборка](#клонирование-и-сборка)
    - [Установка](#установка)
    - [Минимальная сборка](#минимальная-сборка-без-postgresql-ssl-libcurl)
- [Конфигурация](#конфигурация)
- [Запуск](#запуск)
- [Управление процессом](#управление-процессом)
- [Бенчмарк](#бенчмарк)
- [Создание своего проекта](#создание-своего-проекта)
- [Ссылки](#ссылки)
- [Лицензия](#лицензия)

---

## Описание

Апостол написан на C++20 с использованием асинхронной модели программирования на базе [epoll API](https://man7.org/linux/man-pages/man7/epoll.7.html) и прямым доступом к [PostgreSQL](https://www.postgresql.org/) через `libpq`. Платформа ориентирована на высоконагруженные системы.

Ключевой элемент — встроенный HTTP-сервер, работающий в одном цикле обработки событий с соединениями PostgreSQL.

**Уникальность подхода**:

- HTTP-сервер и сокеты PostgreSQL находятся в **едином event loop**.
- Данные идут **напрямую** между HTTP-сервером и базой данных — без промежуточных скриптовых слоёв (PHP, Python и т.п.).
- Это максимально снижает задержки, уменьшает накладные расходы и даёт предсказуемую производительность.

**Основные преимущества**:

- **Автономность** — после сборки вы получаете готовый бинарный файл системной службы (демона) под Linux.
- **Скорость** — обработка HTTP-запросов и выполнение SQL-запросов выполняются настолько быстро, насколько это позволяет ОС и СУБД.
- **Пул соединений** — встроенный настраиваемый пул соединений с PostgreSQL.
- **Модульность** — приложения собираются из независимых модулей как конструктор.

Сам фреймворк находится в [отдельном репозитории](https://github.com/apostoldevel/libapostol) и подключается как git submodule.

---

## Модули

Фреймворк имеет модульную архитектуру со встроенными UDP/TCP/WebSocket/HTTP-серверами и асинхронным клиентом PostgreSQL.

### Данная сборка включает три модуля

| Модуль | Тип | Описание |
|--------|-----|----------|
| [PGHTTP](https://github.com/apostoldevel/module-PGHTTP/tree/version2) | Worker | Диспетчер HTTP-запросов к функциям PostgreSQL — реализуйте REST API на PL/pgSQL |
| [WebServer](https://github.com/apostoldevel/module-WebServer/tree/version2) | Worker | Раздача статических файлов со встроенным Swagger UI |
| [PGFetch](https://github.com/apostoldevel/module-PGFetch/tree/version2) | Helper | HTTP-запросы по LISTEN/NOTIFY — отправляйте HTTP-запросы из PL/pgSQL |

> Подробнее о возможностях этой сборки — в [статье](https://github.com/apostoldevel/apostol/blob/version2/doc/ARTICLE.ru-RU.md).

### Дополнительные модули

С помощью [дополнительных модулей](https://github.com/apostoldevel/libapostol#modules) Апостол можно превратить в:

- [**AuthServer**](https://github.com/apostoldevel/module-AuthServer) — сервер авторизации OAuth 2.0;
- [**AppServer**](https://github.com/apostoldevel/module-AppServer) — сервер приложений;
- [**MessageServer**](https://github.com/apostoldevel/process-MessageServer) — сервер сообщений (SMTP/FCM/API);
- [**FileServer**](https://github.com/apostoldevel/module-FileServer) — файловый сервер;
- [**StreamServer**](https://github.com/apostoldevel/process-StreamServer) — сервер потоковых данных;
- [**WebSocket API**](https://github.com/apostoldevel/module-WebSocketAPI) — JSON-RPC и pub/sub через WebSocket.

Полная CRM-сборка со всеми модулями: [apostol-crm](https://github.com/apostoldevel/apostol-crm/tree/version2).

---

## Проекты

Реальные проекты, реализованные на **Апостол**:

- [CopyFrog](https://copyfrog.ai) — платформа на базе ИИ для генерации рекламных текстов, видео-креативов и маркетинговых описаний;
- [ChargeMeCar](https://chargemecar.com) — центральная система для зарядных станций (OCPP);
- [Campus Caster & Campus CORS](https://cors.campusagro.com) — NTRIP-кастер для передачи GNSS-поправок в реальном времени;
- [Talking to AI](https://t.me/TalkingToAIBot) — интеграция ChatGPT в Telegram-бот;
- [PlugMe](https://plugme.ru) — CRM-система для зарядных станций и владельцев электромобилей;
- [DEBT Master](https://debt-master.ru) — система автоматизации взыскания задолженности;
- [Ship Safety ERP](https://ship-safety.ru) — ERP-система для судоходных компаний;
- [BitDeals](https://testnet.bitdeals.org) — сервис обработки биткоин-платежей.

---

## Архитектура

```
apostol/
├── src/
│   ├── app/main.cpp                  — точка входа приложения
│   ├── lib/libapostol/               — фреймворк (git submodule)
│   └── modules/
│       ├── Workers/                  — обработчики HTTP-запросов
│       │   ├── PGHTTP/              — git submodule
│       │   └── WebServer/           — git submodule
│       └── Helpers/                  — фоновые задачи
│           └── PGFetch/             — git submodule
├── conf/default.json                 — конфигурация
├── www/                              — веб-файлы (Swagger UI и т.п.)
└── docker-compose.yaml               — Docker-окружение
```

Каждый модуль — отдельный git submodule. Добавляйте или убирайте модули, чтобы собрать именно то приложение, которое вам нужно.

---

## Быстрый старт с Docker

Самый быстрый способ попробовать Apostol — Docker Compose с PostgreSQL, PgBouncer и pgweb:

```shell
git clone --recursive https://github.com/apostoldevel/apostol.git -b version2
cd apostol
./docker-build.sh
docker compose up
```

После запуска:
- **Swagger UI** — http://localhost:8080
- **pgweb** (веб-интерфейс PostgreSQL) — http://localhost:8081/pgweb

> Вместо pgweb можно использовать любой другой инструмент для работы с СУБД.
> PostgreSQL из контейнера доступен на порту `5433`.

---

## Сборка из исходников

### Зависимости (Debian/Ubuntu)

```shell
sudo apt-get install build-essential cmake cmake-data g++ \
    libssl-dev libcurl4-openssl-dev libpq-dev zlib1g-dev
```

### Клонирование и сборка

```shell
git clone --recursive https://github.com/apostoldevel/apostol.git -b version2
cd apostol
./configure
cmake --build cmake-build-release --parallel $(nproc)
```

Или вручную:

```shell
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel $(nproc)
```

### Установка

```shell
sudo cmake --install build
```

По умолчанию бинарный файл `apostol` устанавливается в `/usr/sbin`, конфигурация — в `/etc/apostol/`.

### Минимальная сборка (без PostgreSQL, SSL, libcurl)

```shell
./configure --without-postgresql --without-ssl --without-curl
cmake --build cmake-build-release --parallel $(nproc)
```

---

## Конфигурация

Конфигурация по умолчанию: `conf/default.json`

После установки конфигурация хранится в `/etc/apostol/apostol.json`.

Основные параметры:

| Параметр | По умолчанию | Описание |
|----------|-------------|----------|
| `server.port` | 4977 | Порт HTTP-сервера |
| `server.listen` | 127.0.0.1 | Адрес прослушивания |
| `server.backlog` | 4096 | TCP listen backlog |
| `daemon.enabled` | false | Запуск как демон |
| `process.master` | true | Режим master/worker |
| `process.helper` | true | Включить helper-процесс |
| `postgres.worker` | — | Подключение БД для worker |
| `postgres.helper` | — | Подключение БД для helper |

---

## Запуск

```shell
# На переднем плане (single process)
./build/apostol

# Как демон
./build/apostol -d

# Проверка конфигурации
./build/apostol -t

# Systemd-сервис (после установки)
sudo systemctl start apostol
```

### Пример статуса systemd

```text
● apostol.service - Apostol
     Loaded: loaded (/lib/systemd/system/apostol.service; enabled)
     Active: active (running)
   Main PID: 461163 (apostol)
     CGroup: /system.slice/apostol.service
             ├─461163 apostol: master process /usr/sbin/apostol
             └─461164 apostol: worker process ("pq fetch", "web server")
```

## Управление процессом

| Сигнал | Команда | Действие |
|--------|---------|----------|
| SIGTERM | `-s stop` | Быстрая остановка |
| SIGQUIT | `-s quit` | Плавная остановка |
| SIGHUP | `-s reload` | Перезагрузка конфигурации |
| SIGUSR1 | `-s reopen` | Переоткрытие лог-файлов |

---

## Бенчмарк

**Apostol v2 vs v1 vs Python vs Node.js vs Go vs Nginx** — сравнительное тестирование в идентичных Docker-условиях (wrk, 4 потока, 10 секунд).

### /ping — без базы данных (keep-alive ON, 100 соединений)

| Сервис | RPS | Latency p50 |
|--------|----:|------------:|
| Nginx (static return) | 585,000 | 119us |
| **Apostol v2** | **271,000** | **253us** |
| Go (net/http) | 115,000 | 746us |
| Apostol v1 | 67,000 | 1.29ms |
| Node.js (Fastify) | 54,000 | 1.46ms |
| Python (FastAPI) | 2,400 | 41ms |

### /db/ping — PostgreSQL round-trip (keep-alive ON, 100 соединений)

| Сервис | RPS | Latency p50 |
|--------|----:|------------:|
| **Apostol v2** | **69,000** | **1.18ms** |
| Go | 58,000 | 1.55ms |
| Apostol v1 | 33,000 | 2.69ms |
| Node.js | 28,000 | 3.20ms |
| Python | 2,200 | 43ms |

**Ключевые результаты**:
- Apostol v2 в **4 раза быстрее** v1 и в **2.4 раза быстрее** Go на /ping
- Apostol v2 **опережает Nginx** при keep-alive OFF на 1000 соединениях (84K vs 74K RPS) благодаря `SO_REUSEPORT`
- Минимальный разброс p99 latency среди всех сервисов (1.7x при c1000)

> Полные результаты, методология и анализ: [REST API Benchmark](https://github.com/apostoldevel/apostol/blob/version2/doc/BENCHMARK.ru-RU.md).

---

## Создание своего проекта

Этот репозиторий — отправная точка. Чтобы создать своё приложение:

1. Форкните или скопируйте этот репозиторий
2. Добавьте нужные модули как git submodules (см. [список модулей](https://github.com/apostoldevel/libapostol#modules))
3. Зарегистрируйте их в `Workers.hpp` / `Helpers.hpp`
4. Настройте `conf/default.json` и `main.cpp`

Для автоматического создания проекта из шаблона см. [install-скрипт](https://github.com/apostoldevel/libapostol#quick-start) в libapostol.

---

## Ссылки

- [libapostol](https://github.com/apostoldevel/libapostol) — фреймворк
- [apostol-crm](https://github.com/apostoldevel/apostol-crm/tree/version2) — полная сборка CRM со всеми модулями
- [db-platform](https://github.com/apostoldevel/db-platform) — PostgreSQL-фреймворк для разработки бэкенда

## Лицензия

**Апостол** зарегистрирован как программа для ЭВМ:
[Свидетельство о государственной регистрации](https://st.fl.ru/users/al/alienufo/portfolio/f_1156277cef5ebb72.pdf).

[MIT](LICENSE)
