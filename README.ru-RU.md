[![en](https://img.shields.io/badge/lang-en-green.svg)](README.md)

<img width="1422" height="506" alt="apostol-v2" src="https://github.com/user-attachments/assets/90464f33-bfde-459b-8b29-162f4f4f1bc3" />

# Апостол (A-POST-OL)

**Апостол** — шаблон-проект (демо-сборка) для C++20 фреймворка [libapostol](https://github.com/apostoldevel/libapostol) — высокопроизводительной платформы для разработки серверных приложений и системных служб под Linux с прямым доступом к PostgreSQL.

> A-POST-OL -> Asynchronous POST Orchestration Loop — единый цикл обработки событий (event loop) для HTTP и PostgreSQL.

---

## Оглавление

- [Описание](#описание)
- [Бенчмарк](#бенчмарк)
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

## Бенчмарк

**Apostol v2 vs v1 vs Python vs Node.js vs Go vs Nginx** — сравнительное тестирование в идентичных Docker-условиях (wrk, 4 потока, 10 секунд, 4 воркера).

### /ping — напрямую (keep-alive ON, 100 соединений)

| Сервис | RPS | Latency p50 |
|--------|----:|------------:|
| Nginx (static return) | 566 000 | 111us |
| **Apostol v2** | **507 000** | **170us** |
| Go (net/http) | 211 000 | 447us |
| Apostol v1 | 128 000 | 790us |
| Node.js (Fastify) | 102 000 | 0,95ms |
| Python (FastAPI) | 2 400 | 41ms |

### /db/ping — напрямую (keep-alive ON, 100 соединений)

| Сервис | RPS | Latency p50 |
|--------|----:|------------:|
| **Apostol v2** | **112 000** | **0,91ms** |
| Go | 72 000 | 1,07ms |
| Apostol v1 | 61 000 | 1,61ms |
| Node.js | 36 000 | 2,65ms |
| Python | 2 300 | 42ms |

### /ping — через Nginx proxy (keep-alive ON, 100 соединений)

| Сервис | RPS | Latency p50 |
|--------|----:|------------:|
| **Apostol v2** | **70 000** | **1,07ms** |
| Go | 38 000 | 2,06ms |
| Apostol v1 | 16 000 | 3,18ms |
| Node.js | 5 900 | 14,29ms |
| Python | 5 500 | 14,62ms |

**Ключевые результаты**:
- Apostol v2 достигает **90%** от throughput Nginx на /ping (507K vs 566K)
- На /ping keep-alive OFF Apostol v2 **на равных с Nginx** при c100 (84K, оба используют `SO_REUSEPORT`) и лидирует при c1000 (81K vs 75K)
- На /db/ping Apostol v2 лидирует с 112K RPS — **в 1,6 раза быстрее** Go, **в 1,8 раза быстрее** v1
- Через Nginx proxy Apostol v2 сохраняет лидерство с 70K RPS

> Полные результаты, методология и анализ: [REST API Benchmark](doc/BENCHMARK.ru-RU.md).

---

## Модули

Фреймворк имеет модульную архитектуру со встроенными UDP/TCP/WebSocket/HTTP-серверами и асинхронным клиентом PostgreSQL.

### Данная сборка включает три модуля

| Модуль | Тип | Описание |
|--------|-----|----------|
| [PGHTTP](https://github.com/apostoldevel/module-PGHTTP) | Worker | Диспетчер HTTP-запросов к функциям PostgreSQL — реализуйте REST API на PL/pgSQL |
| [WebServer](https://github.com/apostoldevel/module-WebServer) | Worker | Раздача статических файлов со встроенным Swagger UI |
| [PGFetch](https://github.com/apostoldevel/module-PGFetch) | Helper | HTTP-запросы по LISTEN/NOTIFY — отправляйте HTTP-запросы из PL/pgSQL |

> Подробнее о возможностях этой сборки — в [статье](https://github.com/apostoldevel/apostol/blob/master/doc/ARTICLE.ru-RU.md).

### Дополнительные модули

С помощью [дополнительных модулей](https://github.com/apostoldevel/libapostol#modules) Апостол можно превратить в:

- [**AuthServer**](https://github.com/apostoldevel/module-AuthServer) — сервер авторизации OAuth 2.0;
- [**AppServer**](https://github.com/apostoldevel/module-AppServer) — сервер приложений;
- [**MessageServer**](https://github.com/apostoldevel/process-MessageServer) — сервер сообщений (SMTP/FCM/API);
- [**FileServer**](https://github.com/apostoldevel/module-FileServer) — файловый сервер;
- [**StreamServer**](https://github.com/apostoldevel/process-StreamServer) — сервер потоковых данных;
- [**WebSocket API**](https://github.com/apostoldevel/module-WebSocketAPI) — JSON-RPC и pub/sub через WebSocket.

Полная CRM-сборка со всеми модулями: [apostol-crm](https://github.com/apostoldevel/apostol-crm).

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
git clone --recursive https://github.com/apostoldevel/apostol.git
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
git clone --recursive https://github.com/apostoldevel/apostol.git
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
sudo cmake --install cmake-build-release
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
# На переднем плане
./cmake-build-release/apostol

# Как демон
./cmake-build-release/apostol -d

# Проверка конфигурации
./cmake-build-release/apostol -t

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
- [apostol-crm](https://github.com/apostoldevel/apostol-crm) — полная сборка CRM со всеми модулями
- [db-platform](https://github.com/apostoldevel/db-platform) — PostgreSQL-фреймворк для разработки бэкенда

## Лицензия

**Апостол** зарегистрирован как программа для ЭВМ:
[Свидетельство о государственной регистрации](https://st.fl.ru/users/al/alienufo/portfolio/f_1156277cef5ebb72.pdf).

[MIT](LICENSE)
