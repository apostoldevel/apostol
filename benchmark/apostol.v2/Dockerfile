# benchmark/apostol.v2/Dockerfile
#
# Build apostol.v2 for benchmarking (serves /api/v5/{*}).
# Build context: project root (set via docker-compose build.context: ..)
#
# Enables PostgreSQL for /db/ping benchmark. No SSL, no Curl.
#
# Builder:  debian:bookworm-slim + GCC 12 (default in bookworm)
#   {fmt} replaces std::format — works with GCC 12 + C++20.
#   Compiled against glibc 2.36 (bookworm) — no GLIBC_2.38 requirement.
#
# Runtime:  debian:bookworm-slim + libstdc++6 + libpq5
#   Same OS and glibc as apostol v1 — fair comparison.

FROM debian:bookworm-slim AS builder

ENV DEBIAN_FRONTEND=noninteractive
ENV PROJECT_NAME=apostol

RUN set -eux; \
    apt-get update && apt-get install -y --no-install-recommends \
        g++ cmake cmake-data make ca-certificates git zlib1g-dev libpq-dev && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

WORKDIR /opt/$PROJECT_NAME

# Clone the project and configure
RUN set -eux; \
    git clone --depth 1 --branch version2 https://github.com/apostoldevel/apostol.git . && \
    ./configure --without-ssl --without-curl

# Overlay benchmark WebServer — replaces the default WebServer module
COPY WebServer.hpp src/modules/Workers/WebServer/WebServer.hpp
COPY WebServer.cpp src/modules/Workers/WebServer/WebServer.cpp

# Build and install
RUN set -eux; \
    mkdir -p conf/sites && \
    cd cmake-build-release && \
    make -j$(nproc) && \
    make install && \
    cd .. && \
    rm -rf cmake-build-release src

# --- Runtime stage ---
FROM debian:bookworm-slim AS app

ENV DEBIAN_FRONTEND=noninteractive
ENV PROJECT_NAME=apostol

RUN set -eux; \
    apt-get update && apt-get install -y --no-install-recommends \
        bash curl libstdc++6 libpq5 && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

# Copy installed binary + directory skeleton
COPY --from=builder /usr/sbin/$PROJECT_NAME   /usr/sbin/$PROJECT_NAME
COPY --from=builder /etc/$PROJECT_NAME        /etc/$PROJECT_NAME

# Overwrite config with benchmark version
COPY $PROJECT_NAME.json /etc/$PROJECT_NAME/$PROJECT_NAME.json
COPY default.json /etc/$PROJECT_NAME/conf/sites/default.json
COPY entrypoint.sh /opt/entrypoint.sh
RUN chmod +x /opt/entrypoint.sh

EXPOSE 8080

ENTRYPOINT ["/opt/entrypoint.sh"]
STOPSIGNAL SIGTERM
