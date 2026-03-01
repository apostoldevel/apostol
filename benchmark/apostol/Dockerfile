FROM debian:bookworm-slim AS builder

ENV PROJECT_NAME='apostol'
ENV BUILD_MODE='release'

RUN set -eux; \
    apt update && apt install -y --no-install-recommends \
    apt-utils bash build-essential cmake cmake-data g++ gcc \
    libcurl4-openssl-dev libssl-dev make pkg-config lsb-release curl ca-certificates git && \
    apt-get clean && rm -rf /var/lib/apt/lists/*;

RUN set -eux; \
    install -d /usr/share/postgresql-common/pgdg && \
    curl -o /usr/share/postgresql-common/pgdg/apt.postgresql.org.asc --fail https://www.postgresql.org/media/keys/ACCC4CF8.asc && \
    sh -c 'echo "deb [signed-by=/usr/share/postgresql-common/pgdg/apt.postgresql.org.asc] https://apt.postgresql.org/pub/repos/apt $(lsb_release -cs)-pgdg main" > /etc/apt/sources.list.d/pgdg.list' && \
    apt update && apt install -y --no-install-recommends libpq-dev postgresql-server-dev-all && \
    apt-get clean && rm -rf /var/lib/apt/lists/*;

WORKDIR /opt/$PROJECT_NAME

# Clone the project and configure
RUN set -eux; \
    git clone --depth 1 --branch version1  https://github.com/apostoldevel/apostol.git . && \
    ./configure --not-pull-self --$BUILD_MODE;

# Overlay benchmark WebServer sources
COPY WebServer.hpp src/modules/Workers/WebServer/WebServer.hpp
COPY WebServer.cpp src/modules/Workers/WebServer/WebServer.cpp

# Build and install
RUN set -eux; \
    cd cmake-build-$BUILD_MODE && \
    make -j$(nproc) && \
    make install && \
    cd .. && \
    rm -rf cmake-build-$BUILD_MODE src;

# --- Runtime stage ---
FROM debian:bookworm-slim AS app

ENV PROJECT_NAME='apostol'

RUN set -eux; \
    apt update && apt install -y --no-install-recommends \
    bash gettext-base && \
    apt-get clean && rm -rf /var/lib/apt/lists/*;

RUN set -eux; \
    install -d /usr/share/postgresql-common/pgdg && \
    apt update && apt install -y --no-install-recommends \
    curl ca-certificates lsb-release && \
    curl -o /usr/share/postgresql-common/pgdg/apt.postgresql.org.asc --fail https://www.postgresql.org/media/keys/ACCC4CF8.asc && \
    sh -c 'echo "deb [signed-by=/usr/share/postgresql-common/pgdg/apt.postgresql.org.asc] https://apt.postgresql.org/pub/repos/apt $(lsb_release -cs)-pgdg main" > /etc/apt/sources.list.d/pgdg.list' && \
    apt update && apt install -y --no-install-recommends postgresql-client && \
    apt-get clean && rm -rf /var/lib/apt/lists/*;

RUN mkdir -p /var/log/$PROJECT_NAME && \
    mkdir -p /var/lib/$PROJECT_NAME && \
    mkdir -p /tmp/$PROJECT_NAME;

COPY --from=builder /usr/sbin/$PROJECT_NAME /usr/sbin/$PROJECT_NAME
COPY --from=builder /etc/$PROJECT_NAME /etc/$PROJECT_NAME

COPY apostol.conf /etc/$PROJECT_NAME/$PROJECT_NAME.conf.tpl
COPY default.json /etc/$PROJECT_NAME/conf/sites/default.json
COPY entrypoint.sh /opt/entrypoint.sh
RUN chmod +x /opt/entrypoint.sh

EXPOSE 8080

ENTRYPOINT ["/opt/entrypoint.sh"]

STOPSIGNAL SIGTERM
