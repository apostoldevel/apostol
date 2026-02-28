FROM debian:bookworm-slim AS builder

ARG PROJECT_NAME=apostol
ARG BUILD_MODE=release

LABEL project="${PROJECT_NAME}"

ENV TZ=Europe/Moscow \
    LANG=en_US.UTF-8 \
    LC_ALL=en_US.UTF-8

RUN set -eux; \
    echo "$TZ" > /etc/timezone; \
    echo "en_US.UTF-8 UTF-8" > /etc/locale.gen; \
    apt-get update && apt-get install -y --no-install-recommends \
        tzdata locales && \
    ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && \
    dpkg-reconfigure -f noninteractive tzdata && \
    dpkg-reconfigure -f noninteractive locales && \
    rm -rf /var/lib/apt/lists/*

RUN set -eux; \
    apt-get update && apt-get install -y --no-install-recommends \
        bash build-essential cmake g++ \
        libcurl4-openssl-dev libssl-dev zlib1g-dev \
        pkg-config ca-certificates git curl lsb-release && \
    rm -rf /var/lib/apt/lists/*

RUN set -eux; \
    install -d /usr/share/postgresql-common/pgdg && \
    curl -o /usr/share/postgresql-common/pgdg/apt.postgresql.org.asc --fail \
        https://www.postgresql.org/media/keys/ACCC4CF8.asc && \
    sh -c 'echo "deb [signed-by=/usr/share/postgresql-common/pgdg/apt.postgresql.org.asc] https://apt.postgresql.org/pub/repos/apt $(lsb_release -cs)-pgdg main" > /etc/apt/sources.list.d/pgdg.list' && \
    apt-get update && apt-get install -y --no-install-recommends libpq-dev && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /opt/${PROJECT_NAME}
COPY . .

RUN set -eux; \
    cp -r docker/conf . && \
    ./configure --${BUILD_MODE} --with-postgresql; \
    cmake --build cmake-build-${BUILD_MODE} --parallel $(nproc); \
    cmake --install cmake-build-${BUILD_MODE}; \
    rm -rf cmake-build-${BUILD_MODE} src

COPY ./docker/entrypoint.sh /opt/entrypoint.sh
RUN chmod +x /opt/entrypoint.sh

# ─────────────────────────────────────────────────────────────────────────────

FROM debian:bookworm-slim AS app

ARG PROJECT_NAME=apostol

ENV PROJECT_NAME="${PROJECT_NAME}" \
    TZ=Europe/Moscow \
    LANG=en_US.UTF-8 \
    LC_ALL=en_US.UTF-8

RUN set -eux; \
    echo "$TZ" > /etc/timezone; \
    echo "en_US.UTF-8 UTF-8" > /etc/locale.gen; \
    apt-get update && apt-get install -y --no-install-recommends \
        tzdata locales && \
    ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && \
    dpkg-reconfigure -f noninteractive tzdata && \
    dpkg-reconfigure -f noninteractive locales && \
    rm -rf /var/lib/apt/lists/*

RUN set -eux; \
    apt-get update && apt-get install -y --no-install-recommends \
        bash sudo ca-certificates curl lsb-release git gettext-base htop mc && \
    rm -rf /var/lib/apt/lists/*

RUN set -eux; \
    install -d /usr/share/postgresql-common/pgdg && \
    curl -o /usr/share/postgresql-common/pgdg/apt.postgresql.org.asc --fail \
        https://www.postgresql.org/media/keys/ACCC4CF8.asc && \
    sh -c 'echo "deb [signed-by=/usr/share/postgresql-common/pgdg/apt.postgresql.org.asc] https://apt.postgresql.org/pub/repos/apt $(lsb_release -cs)-pgdg main" > /etc/apt/sources.list.d/pgdg.list' && \
    apt-get update && apt-get install -y --no-install-recommends postgresql-client && \
    rm -rf /var/lib/apt/lists/*

RUN mkdir -p /var/log/${PROJECT_NAME} /var/lib/${PROJECT_NAME} /tmp/${PROJECT_NAME}

COPY --from=builder /usr/sbin/${PROJECT_NAME}    /usr/sbin/${PROJECT_NAME}
COPY --from=builder /etc/${PROJECT_NAME}         /etc/${PROJECT_NAME}
COPY --from=builder /opt/${PROJECT_NAME}         /opt/${PROJECT_NAME}
COPY --from=builder /opt/entrypoint.sh           /opt/entrypoint.sh

EXPOSE 4977

ENTRYPOINT ["/opt/entrypoint.sh"]

STOPSIGNAL SIGTERM
