[![en](https://img.shields.io/badge/lang-en-green.svg)](https://github.com/apostoldevel/apostol/blob/master/README.md)

![image](https://github.com/apostoldevel/apostol/assets/91010417/cf0b289d-185f-4852-a9a9-cd953f4a7199)

# Апостол (Apostol)

**Апостол** — это высокопроизводительная C++‑платформа (framework) для разработки серверных приложений и системных служб под Linux с прямым доступом к PostgreSQL.

---

## Оглавление

- [Описание](#описание)
- [Модули](#модули)
    - [Модули в данной сборке](#данная-сборка-поставляется-двумя-модулями)
    - [Дополнительные модули](#с-помощью-дополнительных-модулей-апостол-можно-превратить-в)
- [Реальные проекты](#проекты)
- [Структура каталогов](#структура-каталогов)
- [Запуск в Docker (docker-compose)](#docker-compose)
- [Сборка и установка](#сборка-и-установка)
    - [Зависимости](#linux-debianubuntu)
    - [Установка PostgreSQL и создание БД](#postgresql)
    - [Установка исходников](#для-установки-апостол-без-git-необходимо)
    - [Сборка и установка бинаря](#сборка)
- [Запуск как сервис](#запуск)
- [Управление процессами](#управление)
- [Лицензии и регистрация](#лицензии-и-регистрация)

---

## Описание

Апостол разработан на C++ с использованием асинхронной модели программирования на базе [epoll API](https://man7.org/linux/man-pages/man7/epoll.7.html) и прямым доступом к СУБД [PostgreSQL](https://www.postgresql.org/) через библиотеку `libpq`. Платформа ориентирована на высоконагруженные системы.

Ключевым элементом платформы является встроенный HTTP‑сервер, работающий в одном цикле обработки событий с соединениями PostgreSQL.

**Уникальность подхода**:

- HTTP‑сервер и сокеты PostgreSQL находятся в **едином event‑loop**.
- Данные идут **напрямую** между HTTP‑сервером и базой данных — без промежуточных скриптовых слоёв (PHP, Python и т.п.).
- Это максимально снижает задержки, уменьшает накладные расходы и даёт предсказуемую производительность.

**Основные преимущества**:

- **Автономность**  
  После сборки вы получаете готовый бинарный файл системной службы (демона) под Linux.

- **Скорость**  
  Обработка HTTP‑запросов и выполнение SQL‑запросов выполняются настолько быстро, насколько это позволяет ОС и СУБД.

- **Пул соединений**  
  Встроенный настраиваемый пул соединений с PostgreSQL.

---

## Модули

Фреймворк имеет модульную архитектуру и включает встроенные UDP/TCP/WebSocket/HTTP‑серверы и клиент для [PostgreSQL](https://www.postgresql.org/).

### Данная сборка поставляется следующими модулями

- [**WebServer**](https://github.com/apostoldevel/module-WebServer) — веб‑сервер
    - Обеспечивает работу [Swagger UI](https://swagger.io/tools/swagger-ui), доступного по адресу:  
      [http://localhost:8080](http://localhost:8080) после запуска **Апостол**.

- [**PGFetch**](https://github.com/apostoldevel/module-PGFetch) — Postgres Fetch
    - Позволяет отправлять HTTP‑запросы на языке `PL/pgSQL` непосредственно из базы данных.  

- [**PGHTTP**](https://github.com/apostoldevel/module-PGHTTP) — Postgres HTTP
    - Позволяет принимать и обрабатывать HTTP‑запросы (REST API) на языке `PL/pgSQL`.  
      Это даёт возможность реализовывать логику API непосредственно в базе данных.

> Подробнее о возможностях этой сборки — в [статье](https://github.com/apostoldevel/apostol/blob/master/doc/ARTICLE.ru-RU.md).

### С помощью дополнительных модулей Апостол можно превратить в

- [**AuthServer**](https://github.com/apostoldevel/module-AuthServer) — сервер авторизации OAuth 2.0;
- [**AppServer**](https://github.com/apostoldevel/module-AppServer) — сервер приложений;
- [**MessageServer**](https://github.com/apostoldevel/process-MessageServer) — сервер сообщений (SMTP/FCM/API);
- [**FileServer**](https://github.com/apostoldevel/module-FileServer) — файловый сервер;
- [**StreamServer**](https://github.com/apostoldevel/process-StreamServer) — сервер потоковых данных.

Отдельно доступен модуль WebSocket: [**WebSocket API**](https://github.com/apostoldevel/module-WebSocketAPI).

---

## Проекты

Реальные проекты, реализованные на **Апостол**:

- [CopyFrog](https://copyfrog.ai) — платформа на базе ИИ для генерации рекламных текстов, видео‑креативов и маркетинговых описаний;
- [ChargeMeCar](https://chargemecar.com) — центральная система для зарядных станций (OCPP);
- [Campus Caster & Campus CORS](https://cors.campusagro.com): NTRIP-кастер (Caster) и коммерческая система для передачи GNSS-поправок в реальном времени по протоколу NTRIP (протокол для потоковой передачи дифференциальных поправок GNSS через Интернет).
- [Talking to AI](https://t.me/TalkingToAIBot) — интеграция ChatGPT в Telegram-бот.
- [PlugMe](https://plugme.ru) — CRM‑система для зарядных станций и владельцев электромобилей;
- [DEBT Master](https://debt-master.ru) — система автоматизации взыскания задолженности;
- [Ship Safety ERP](https://ship-safety.ru) — ERP‑система для судоходных компаний;
- [BitDeals](https://testnet.bitdeals.org) — сервис обработки биткоин‑платежей.

---

## Структура каталогов

```text
auto/                       скрипты автоматизации
cmake-modules/              модули CMake
conf/                       конфигурационные файлы
src/                        исходный код
├─ app/                     исходный код: Apostol (приложение)
├─ core/                    исходный код: Apostol Core (ядро фреймворка)
├─ lib/                     исходный код библиотек
│  └─ delphi/               Delphi classes for C++
└─ modules/                 исходный код модулей (дополнений)
www/                        файлы веб‑интерфейса (сайт, Swagger UI и т.п.)
```

---

## Docker Compose

### Сборка контейнеров

```bash
./docker-build.sh
```

### Запуск контейнеров

```bash
./docker-up.sh
```

После запуска:

- **Swagger UI** ([swagger-ui](https://github.com/swagger-api/swagger-ui))  
  будет доступен по адресу:
    - [http://localhost:8080](http://localhost:8080)
    - или `http://<host-ip>:8080`

- **Pgweb** ([pgweb](https://github.com/sosedoff/pgweb)) — веб‑обозреватель для PostgreSQL  
  будет доступен по адресу:
    - [http://localhost:8081/pgweb](http://localhost:8081/pgweb/)
    - или `http://<host-ip>:8081/pgweb`

> Вместо pgweb можно использовать любой другой инструмент для работы с СУБД.  
> PostgreSQL из контейнера доступен на порту `5433`.

---

## Сборка и установка

Для локальной установки **Апостол** вам потребуются:

1. Компилятор C++;
2. [CMake](https://cmake.org) или IDE с поддержкой CMake;
3. Библиотека [libpq-dev](https://www.postgresql.org/download) (frontend headers для работы с PostgreSQL);
4. Библиотека [postgresql-server-dev-all](https://www.postgresql.org/download) (backend headers PostgreSQL).

### Linux (Debian/Ubuntu)

Установка компилятора C++ и основных библиотек:

```bash
sudo apt-get update
sudo apt-get install \
    build-essential \
    libssl-dev \
    libcurl4-openssl-dev \
    make cmake gcc g++
```

> Подробная установка C++, CMake, IDE и других вспомогательных инструментов не входит в это руководство.

#### PostgreSQL

Установите PostgreSQL, следуя официальной инструкции:  
<https://www.postgresql.org/download/>

#### База данных

Шаги подготовки базы данных:

1. Укажите имя базы данных в файле `db/sql/sets.conf`  
   (по умолчанию: `web`).

2. Укажите пароль для пользователя СУБД в файле `~postgres/.pgpass`:  
   Под пользователем `postgres`:
   ```bash
   sudo -iu postgres -H vim .pgpass
   ```
   Пример содержимого:
   ```text
   *:*:*:http:http
   ```

3. Включите доступ в конфигурации `pg_hba.conf`  
   Файл: `/etc/postgresql/$PG_MAJOR/main/pg_hba.conf`:
   ```text
   # TYPE  DATABASE        USER    ADDRESS  METHOD
   local   web             http            md5
   ```

4. Примените настройки:
   ```bash
   sudo pg_ctlcluster <version> main reload
   ```

5. Инициализируйте базу:
   ```bash
   cd db/
   ./runme.sh
   ```

6. В меню установочного скрипта выберите `First Installation`.

> Параметр `--init` необходим только при первой установке базы данных.  
> Далее установочный скрипт можно запускать без параметров или с `--install`.

---

### Установка исходников

#### Для установки **Апостол** без Git необходимо

1. Скачать архив:  
   <https://github.com/apostoldevel/apostol/archive/master.zip>
2. Распаковать архив;
3. При необходимости отредактировать `CMakeLists.txt`;
4. Собрать проект (см. ниже).

#### Для установки **Апостол** через Git

```bash
git clone https://github.com/apostoldevel/apostol.git
cd apostol
```

---

### Сборка

```bash
./configure
```

### Компиляция и установка

```bash
cd cmake-build-release
make
sudo make install
```

По умолчанию бинарный файл `apostol` устанавливается в:

```text
/usr/sbin
```

Файлы конфигурации и необходимые данные в зависимости от параметров установки будут располагаться в:

```text
/etc/apostol
или
~/apostol
```

---

## Запуск

Если при конфигурации сборки опция `INSTALL_AS_ROOT` установлена в `ON`, то `apostol` устанавливается как системная служба (демон) Linux.

### Управление через systemd

Запуск сервиса:

```bash
sudo systemctl start apostol
```

Проверка статуса:

```bash
sudo systemctl status apostol
```

Ожидаемый вывод (пример):

```text
● apostol.service - Apostol
     Loaded: loaded (/etc/systemd/system/apostol.service; enabled; vendor preset: enabled)
     Active: active (running) since Sat 2019-04-06 00:00:00 MSK; 3y ago
    Process: 461158 ExecStartPre=/usr/bin/rm -f /run/apostol.pid (code=exited, status=0/SUCCESS)
    Process: 461160 ExecStartPre=/usr/sbin/apostol -t (code=exited, status=0/SUCCESS)
    Process: 461162 ExecStart=/usr/sbin/apostol (code=exited, status=0/SUCCESS)
   Main PID: 461163 (apostol)
      Tasks: 2 (limit: 77011)
     Memory: 2.6M
        CPU: 44ms
     CGroup: /system.slice/apostol.service
             ├─461163 apostol: master process /usr/sbin/apostol
             └─461164 apostol: worker process ("pq fetch", "web server")
```

---

## Управление

Управление `apostol` осуществляется с помощью UNIX‑сигналов.  
PID главного процесса по умолчанию записывается в файл:

```text
/run/apostol.pid
```

Имя PID‑файла можно изменить при конфигурации сборки или в `apostol.conf`  
(секция `[daemon]`, ключ `pid`).

### Сигналы главному процессу

| Сигнал     | Действие                                                                                 |
|-----------|-------------------------------------------------------------------------------------------|
| `TERM`    | быстрое завершение                                                                       |
| `INT`     | быстрое завершение                                                                       |
| `QUIT`    | плавное завершение                                                                       |
| `HUP`     | перечитать конфигурацию, запустить новые рабочие процессы, плавно завершить старые       |
| `WINCH`   | плавное завершение рабочих процессов                                                     |

Рабочими процессами отдельно управлять не требуется, однако они поддерживают:

| Сигнал     | Действие          |
|-----------|-------------------|
| `TERM`    | быстрое завершение |
| `INT`     | быстрое завершение |
| `QUIT`    | плавное завершение |

---

## Лицензии и регистрация

**Апостол** зарегистрирован как программа для ЭВМ:  
[Свидетельство о государственной регистрации программы для ЭВМ](https://st.fl.ru/users/al/alienufo/portfolio/f_1156277cef5ebb72.pdf).

Информация о лицензии (если есть отдельный файл `LICENSE`) рекомендуется продублировать здесь.