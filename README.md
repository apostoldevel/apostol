[![ru](https://img.shields.io/badge/lang-ru-green.svg)](https://github.com/apostoldevel/apostol/blob/version2/README.ru-RU.md)

# Apostol

**Apostol** is a minimal project built on the [libapostol](https://github.com/apostoldevel/libapostol) C++20 framework.

This build includes three modules:

| Module | Type | Description |
|--------|------|-------------|
| [PGHTTP](https://github.com/apostoldevel/module-PGHTTP/tree/version2) | Worker | HTTP to PostgreSQL function dispatcher |
| [WebServer](https://github.com/apostoldevel/module-WebServer/tree/version2) | Worker | Static file serving |
| [PGFetch](https://github.com/apostoldevel/module-PGFetch/tree/version2) | Helper | LISTEN/NOTIFY driven HTTP fetch |

With [additional modules](https://github.com/apostoldevel/libapostol#modules) Apostol can be turned into a full-featured application server.

---

## Build

### Dependencies (Debian/Ubuntu)

```shell
sudo apt-get install build-essential cmake cmake-data g++ libssl-dev libcurl4-openssl-dev libpq-dev zlib1g-dev
```

### Clone

```shell
git clone --recursive https://github.com/apostoldevel/apostol.git -b version2
cd apostol
```

### Build

```shell
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel $(nproc)
```

### Install

```shell
sudo cmake --install build
```

### Minimal build (no PostgreSQL, no SSL, no libcurl)

```shell
cmake -B build -DCMAKE_BUILD_TYPE=Release \
    -DWITH_POSTGRESQL=OFF -DWITH_SSL=OFF -DWITH_CURL=OFF
cmake --build build --parallel $(nproc)
```

---

## Configuration

Default config: `conf/default.json`

After install the configuration is stored at `/etc/apostol/conf/`.

Key settings:

| Setting | Default | Description |
|---------|---------|-------------|
| `server.port` | 4977 | HTTP listen port |
| `server.listen` | 127.0.0.1 | Listen address |
| `daemon.enabled` | false | Run as daemon |
| `process.master` | false | Master/worker mode |
| `postgres.worker` | — | Worker DB connection |
| `postgres.helper` | — | Helper DB connection |

---

## Running

```shell
# Foreground (single process)
./build/apostol -p conf/

# As a daemon
./build/apostol -p conf/ -d

# Test configuration
./build/apostol -p conf/ -t

# Systemd service (after install)
sudo systemctl start apostol
```

---

## Process control

| Signal | Command | Action |
|--------|---------|--------|
| SIGTERM | `-s stop` | Fast shutdown |
| SIGQUIT | `-s quit` | Graceful shutdown |
| SIGHUP | `-s reload` | Reload config |
| SIGUSR1 | `-s reopen` | Reopen log files |

---

## License

[MIT](LICENSE)
