[![ru](https://img.shields.io/badge/lang-ru-green.svg)](https://github.com/apostoldevel/apostol/blob/master/README.ru-RU.md)

![image](https://github.com/apostoldevel/apostol/assets/91010417/cf0b289d-185f-4852-a9a9-cd953f4a7199)

# Apostol

**Apostol** is a high‑performance C++ framework for developing backend applications and system services on Linux with direct access to PostgreSQL.

---

## Table of Contents

- [Overview](#overview)
- [Modules](#modules)
    - [Modules in this build](#this-build-comes-with-two-modules)
    - [Additional modules](#with-additional-modules-apostol-can-be-turned-into)
- [Real‑world projects](#projects)
- [Directory structure](#directory-structure)
- [Running in Docker (docker-compose)](#docker-compose)
- [Build and installation](#build-and-installation)
    - [Dependencies](#linux-debianubuntu)
    - [Installing PostgreSQL and creating the database](#postgresql)
    - [Installing sources](#installing-the-sources)
    - [Building and installing the binary](#build)
- [Running as a service](#running)
- [Process control](#control)
- [Licenses and registration](#licenses-and-registration)

---

## Overview

Apostol is written in C++ using an asynchronous programming model based on the [epoll API](https://man7.org/linux/man-pages/man7/epoll.7.html), with direct access to the [PostgreSQL](https://www.postgresql.org/) DBMS via the `libpq` library. The framework is targeted at high‑load systems.

The key element of the framework is the built‑in HTTP server, which runs in a single event loop together with PostgreSQL connections.

**What makes it different**:

- The HTTP server and PostgreSQL sockets live in a **single event loop**.
- Data flows **directly** between the HTTP server and the database – no intermediate scripting layers (PHP, Python, etc.).
- This minimizes latency and overhead and provides predictable performance.

**Main advantages**:

- **Autonomy**  
  After building, you get a ready‑to‑run Linux system service (daemon) binary.

- **Speed**  
  Handling HTTP requests and executing SQL queries is as fast as your OS and DBMS allow.

- **Connection pool**  
  Built‑in configurable PostgreSQL connection pool.

---

## Modules

The framework has a modular architecture and includes built‑in UDP/TCP/WebSocket/HTTP servers and a client for [PostgreSQL](https://www.postgresql.org/).

### This distribution is supplied with the following modules

- **WebServer** (https://github.com/apostoldevel/module-WebServer) — web server
    - Provides Swagger UI (https://swagger.io/tools/swagger-ui), available at  
      http://localhost:8080 after starting Apostol.

- **PGFetch** (https://github.com/apostoldevel/module-PGFetch) — Postgres Fetch
    - Allows you to send HTTP requests in PL/pgSQL directly from the database.

- **PGHTTP** (https://github.com/apostoldevel/module-PGHTTP) — Postgres HTTP
    - Allows you to receive and process HTTP requests (REST API) in PL/pgSQL.  
      This makes it possible to implement API logic directly in the database.

> More details about this build can be found in the [article](https://github.com/apostoldevel/apostol/blob/master/doc/ARTICLE.md).

### With additional modules Apostol can be turned into

- [**AuthServer**](https://github.com/apostoldevel/module-AuthServer) — OAuth 2.0 authorization server (in Russian);
- [**AppServer**](https://github.com/apostoldevel/module-AppServer) — application server (in Russian);
- [**MessageServer**](https://github.com/apostoldevel/process-MessageServer) — messaging server (SMTP/FCM/API) (in Russian);
- [**FileServer**](https://github.com/apostoldevel/module-FileServer) — file server (in Russian);
- [**StreamServer**](https://github.com/apostoldevel/process-StreamServer) — streaming data server (in Russian).

A separate WebSocket module is available: [**WebSocket API**](https://github.com/apostoldevel/module-WebSocketAPI) (in Russian).

---

## Projects

Real‑world projects built with **Apostol**:

- [CopyFrog](https://copyfrog.ai) — an AI‑based platform for generating ad copy, video creatives, and marketing descriptions;
- [ChargeMeCar](https://chargemecar.com) — central system for charging stations (OCPP);
- [Talking to AI](https://t.me/TalkingToAIBot) — ChatGPT integration in a Telegram bot.
- [Campus Caster & Campus CORS](https://cors.campusagro.com): an NTRIP caster and commercial system for transmitting real‑time GNSS corrections using the NTRIP protocol (a protocol for streaming differential GNSS corrections over the Internet).
- [PlugMe](https://plugme.ru) — CRM system for charging stations and EV owners;
- [DEBT Master](https://debt-master.ru) — debt collection automation system;
- [Ship Safety ERP](https://ship-safety.ru) — an ERP system for shipping companies;
- [BitDeals](https://testnet.bitdeals.org) — bitcoin payment processing service;

---

## Directory structure

```text
auto/                       automation scripts
cmake-modules/              CMake modules
conf/                       configuration files
src/                        source code
├─ app/                     source: Apostol (application)
├─ core/                    source: Apostol Core (framework core)
├─ lib/                     source code of libraries
│  └─ delphi/               Delphi classes for C++
└─ modules/                 source code of modules (extensions)
www/                        web UI files (site, Swagger UI, etc.)
```

---

## Docker Compose

### Building containers

```bash
./docker-build.sh
```

### Starting containers

```bash
./docker-up.sh
```

After startup:

- **Swagger UI** ([swagger-ui](https://github.com/swagger-api/swagger-ui))  
  will be available at:
    - [http://localhost:8080](http://localhost:8080)
    - or `http://<host-ip>:8080`

- **Pgweb** ([pgweb](https://github.com/sosedoff/pgweb)) — web‑based PostgreSQL browser  
  will be available at:
    - [http://localhost:8081/pgweb](http://localhost:8081/pgweb/)
    - or `http://<host-ip>:8081/pgweb`

> Instead of pgweb, you can use any other tool for working with the DBMS.  
> PostgreSQL inside the container is available on port `5433`.

---

## Build and installation

To install **Apostol** locally, you need:

1. A C++ compiler;
2. [CMake](https://cmake.org) or an IDE with CMake support;
3. [libpq-dev](https://www.postgresql.org/download) (frontend headers for PostgreSQL);
4. [postgresql-server-dev-all](https://www.postgresql.org/download) (PostgreSQL backend headers).

### Linux (Debian/Ubuntu)

Install a C++ compiler and basic libraries:

```bash
sudo apt-get update
sudo apt-get install \
    build-essential \
    libssl-dev \
    libcurl4-openssl-dev \
    make cmake gcc g++
```

> Detailed installation of C++, CMake, IDEs, and other auxiliary tools is beyond the scope of this guide.

#### PostgreSQL

Install PostgreSQL following the official instructions:  
<https://www.postgresql.org/download/>

#### Database

Database preparation steps:

1. Specify the database name in `db/sql/sets.conf`  
   (default: `web`).

2. Specify the DB user password in `~postgres/.pgpass`:  
   Under the `postgres` user:
   ```bash
   sudo -iu postgres -H vim .pgpass
   ```
   Example content:
   ```text
   *:*:*:http:http
   ```

3. Allow access in `pg_hba.conf`  
   File: `/etc/postgresql/$PG_MAJOR/main/pg_hba.conf`:
   ```text
   # TYPE  DATABASE        USER    ADDRESS  METHOD
   local   web             http            md5
   ```

4. Apply settings:
   ```bash
   sudo pg_ctlcluster <version> main reload
   ```

5. Initialize the database:
   ```bash
   cd db/
   ./runme.sh
   ```

6. In the installer menu, select `First Installation`.

> The `--init` parameter is required only for the first database installation.  
> Later you can run the installer without parameters or with `--install`.

---

### Installing the sources

#### Installing **Apostol** without Git

1. Download the archive:  
   <https://github.com/apostoldevel/apostol/archive/master.zip>
2. Unpack the archive;
3. Edit `CMakeLists.txt` if necessary;
4. Build the project (see below).

#### Installing **Apostol** via Git

```bash
git clone https://github.com/apostoldevel/apostol.git
cd apostol
```

---

### Build

```bash
./configure
```

### Compilation and installation

```bash
cd cmake-build-release
make
sudo make install
```

By default, the `apostol` binary is installed into:

```text
/usr/sbin
```

Configuration files and required data, depending on installation options, will be located in:

```text
/etc/apostol
or
~/apostol
```

---

## Running

If the `INSTALL_AS_ROOT` option was set to `ON` at configuration time, `apostol` is installed as a Linux system service (daemon).

### Managing via systemd

Start the service:

```bash
sudo systemctl start apostol
```

Check status:

```bash
sudo systemctl status apostol
```

Expected output (example):

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

## Control

You control `apostol` using UNIX signals.  
The master process PID is written by default to:

```text
/run/apostol.pid
```

The PID file name can be changed at build configuration time or in `apostol.conf`  
(section `[daemon]`, key `pid`).

### Signals to the master process

| Signal   | Action                                                                                     |
|----------|--------------------------------------------------------------------------------------------|
| `TERM`   | fast shutdown                                                                              |
| `INT`    | fast shutdown                                                                              |
| `QUIT`   | graceful shutdown                                                                          |
| `HUP`    | reload configuration, start new workers, gracefully shut down old ones                    |
| `WINCH`  | gracefully shut down worker processes                                                      |

You don’t need to control worker processes separately, but they support:

| Signal   | Action            |
|----------|-------------------|
| `TERM`   | fast shutdown     |
| `INT`    | fast shutdown     |
| `QUIT`   | graceful shutdown |

---

## Licenses and registration

**Apostol** is registered as computer software:  
[Certificate of State Registration of a Computer Program](https://st.fl.ru/users/al/alienufo/portfolio/f_1156277cef5ebb72.pdf) (in Russian).

If there is a separate `LICENSE` file, it is recommended to duplicate the license information here.