[![en](https://img.shields.io/badge/lang-en-green.svg)](https://github.com/apostoldevel/apostol/blob/version2/README.md)

# Апостол

**Апостол** — минимальный проект на базе C++20 фреймворка [libapostol](https://github.com/apostoldevel/libapostol).

Данная сборка включает три модуля:

| Модуль | Тип | Описание |
|--------|-----|----------|
| [PGHTTP](https://github.com/apostoldevel/module-PGHTTP/tree/version2) | Worker | Диспетчер HTTP-запросов к функциям PostgreSQL |
| [WebServer](https://github.com/apostoldevel/module-WebServer/tree/version2) | Worker | Раздача статических файлов |
| [PGFetch](https://github.com/apostoldevel/module-PGFetch/tree/version2) | Helper | HTTP-запросы по LISTEN/NOTIFY |

С помощью [дополнительных модулей](https://github.com/apostoldevel/libapostol#modules) Апостол можно превратить в полнофункциональный сервер приложений.

---

## Сборка

### Зависимости (Debian/Ubuntu)

```shell
sudo apt-get install build-essential cmake cmake-data g++ libssl-dev libcurl4-openssl-dev libpq-dev zlib1g-dev
```

### Клонирование

```shell
git clone --recursive https://github.com/apostoldevel/apostol.git -b version2
cd apostol
```

### Сборка

```shell
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel $(nproc)
```

### Установка

```shell
sudo cmake --install build
```

### Минимальная сборка (без PostgreSQL, SSL, libcurl)

```shell
cmake -B build -DCMAKE_BUILD_TYPE=Release \
    -DWITH_POSTGRESQL=OFF -DWITH_SSL=OFF -DWITH_CURL=OFF
cmake --build build --parallel $(nproc)
```

---

## Конфигурация

Конфигурация по умолчанию: `conf/default.json`

После установки конфигурация хранится в `/etc/apostol/conf/`.

Основные параметры:

| Параметр | По умолчанию | Описание |
|----------|-------------|----------|
| `server.port` | 4977 | Порт HTTP-сервера |
| `server.listen` | 127.0.0.1 | Адрес прослушивания |
| `daemon.enabled` | false | Запуск как демон |
| `process.master` | false | Режим master/worker |
| `postgres.worker` | — | Подключение БД для worker |
| `postgres.helper` | — | Подключение БД для helper |

---

## Запуск

```shell
# На переднем плане (single process)
./build/apostol -p conf/

# Как демон
./build/apostol -p conf/ -d

# Проверка конфигурации
./build/apostol -p conf/ -t

# Systemd-сервис (после установки)
sudo systemctl start apostol
```

---

## Управление процессом

| Сигнал | Команда | Действие |
|--------|---------|----------|
| SIGTERM | `-s stop` | Быстрая остановка |
| SIGQUIT | `-s quit` | Плавная остановка |
| SIGHUP | `-s reload` | Перезагрузка конфигурации |
| SIGUSR1 | `-s reopen` | Переоткрытие лог-файлов |

---

## Лицензия

[MIT](LICENSE)
