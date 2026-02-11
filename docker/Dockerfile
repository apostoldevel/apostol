# Этап сборки
FROM debian:bookworm-slim AS builder

LABEL project="example"

ENV PROJECT_NAME 'example'
ENV BUILD_MODE 'release'

ENV TZ 'Europe/Moscow'
ENV LANG 'ru_RU.UTF-8'
ENV LC_ALL 'ru_RU.UTF-8'

# Настройка часового пояса и локалей
RUN set -eux; \
    echo $TZ > /etc/timezone; \
    echo "en_US.UTF-8 UTF-8" > /etc/locale.gen; \
    echo "ru_RU.UTF-8 UTF-8" >> /etc/locale.gen; \
    apt update && apt install -y --no-install-recommends \
    tzdata locales && \
    ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && \
    dpkg-reconfigure -f noninteractive tzdata && \
    dpkg-reconfigure -f noninteractive locales && \
    apt-get clean && rm -rf /var/lib/apt/lists/*;

# Установка необходимых программ и библиотек
RUN set -eux; \
    apt update && apt install -y --no-install-recommends \
    apt-utils bash build-essential cmake cmake-data g++ gcc \
    libcurl4-openssl-dev libssl-dev make pkg-config lsb-release curl ca-certificates git && \
    apt-get clean && rm -rf /var/lib/apt/lists/*;

# Установка библиотек PostgreSQL
RUN set -eux; \
    install -d /usr/share/postgresql-common/pgdg && \
    curl -o /usr/share/postgresql-common/pgdg/apt.postgresql.org.asc --fail https://www.postgresql.org/media/keys/ACCC4CF8.asc && \
    sh -c 'echo "deb [signed-by=/usr/share/postgresql-common/pgdg/apt.postgresql.org.asc] https://apt.postgresql.org/pub/repos/apt $(lsb_release -cs)-pgdg main" > /etc/apt/sources.list.d/pgdg.list' && \
    apt update && apt install -y --no-install-recommends libpq-dev postgresql-server-dev-all && \
    apt-get clean && rm -rf /var/lib/apt/lists/*;

# Сборка и установка приложения
WORKDIR /opt/$PROJECT_NAME
COPY . .

RUN set -eux; \
    cp -r .docker/conf . && \
    ./configure --not-pull-self --$BUILD_MODE; \
    cd cmake-build-$BUILD_MODE; \
    make install; \
    cd ..; \
    rm -rf cmake-build-$BUILD_MODE; \
    rm -rf src;

COPY ./.docker/entrypoint.sh /opt/entrypoint.sh
RUN chmod +x /opt/entrypoint.sh

# Этап создания образа приложения
FROM debian:bookworm-slim AS app

ENV PROJECT_NAME 'example'
ENV TZ 'Europe/Moscow'
ENV LANG 'ru_RU.UTF-8'
ENV LC_ALL 'ru_RU.UTF-8'

# Настройка часового пояса и локалей
RUN set -eux; \
    echo $TZ > /etc/timezone; \
    echo "en_US.UTF-8 UTF-8" > /etc/locale.gen; \
    echo "ru_RU.UTF-8 UTF-8" >> /etc/locale.gen; \
    apt update && apt install -y --no-install-recommends \
    tzdata locales && \
    ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && \
    dpkg-reconfigure -f noninteractive tzdata && \
    dpkg-reconfigure -f noninteractive locales && \
    apt clean && rm -rf /var/lib/apt/lists/*;

# Установка необходимых программ и библиотек
RUN set -eux; \
    apt update && apt install -y --no-install-recommends \
    apt-utils bash sudo lsb-release curl ca-certificates git gettext-base htop mc && \
    apt clean && rm -rf /var/lib/apt/lists/*;

# Установка клиента PostgreSQL
RUN set -eux; \
    install -d /usr/share/postgresql-common/pgdg && \
    curl -o /usr/share/postgresql-common/pgdg/apt.postgresql.org.asc --fail https://www.postgresql.org/media/keys/ACCC4CF8.asc && \
    sh -c 'echo "deb [signed-by=/usr/share/postgresql-common/pgdg/apt.postgresql.org.asc] https://apt.postgresql.org/pub/repos/apt $(lsb_release -cs)-pgdg main" > /etc/apt/sources.list.d/pgdg.list' && \
    apt update && apt install -y --no-install-recommends postgresql-client && \
    apt clean && rm -rf /var/lib/apt/lists/*;

# Создание необходимых каталогов
RUN mkdir -p /var/log/$PROJECT_NAME && \
    mkdir -p /var/lib/$PROJECT_NAME && \
    mkdir -p /var/lib/$PROJECT_NAME/report_ready && \
    mkdir -p /var/lib/$PROJECT_NAME/public && \
    mkdir -p /tmp/$PROJECT_NAME;

RUN chmod 777 /var/lib/$PROJECT_NAME/report_ready
RUN chmod 777 /var/lib/$PROJECT_NAME/public

COPY --from=builder /usr/sbin/$PROJECT_NAME /usr/sbin/$PROJECT_NAME
COPY --from=builder /etc/$PROJECT_NAME /etc/$PROJECT_NAME
COPY --from=builder /opt/$PROJECT_NAME /opt/$PROJECT_NAME
COPY --from=builder /opt/entrypoint.sh /opt/entrypoint.sh

EXPOSE 8080

ENTRYPOINT ["/opt/entrypoint.sh"]

STOPSIGNAL SIGTERM