FROM debian AS builder

LABEL project="apostol"

ENV PROJECT_NAME 'apostol'

ENV PG_VERSION '16'
ENV PG_CLUSTER 'main'

ENV TZ 'UTC'

ENV LANG 'en_US.UTF-8'
ENV LC_ALL 'en_US.UTF-8'

ENV PGSSLCERT /tmp/postgresql.crt

RUN set -eux; \
    echo $TZ > /etc/timezone;  \
    echo "en_US.UTF-8 UTF-8" > /etc/locale.gen; \
    echo "ru_RU.UTF-8 UTF-8" >> /etc/locale.gen; \
    apt-get update && apt-get install -y tzdata locales; \
    rm /etc/localtime;  \
    ln -snf /usr/share/zoneinfo/$TZ /etc/localtime;  \
    dpkg-reconfigure -f noninteractive tzdata; \
    dpkg-reconfigure -f noninteractive locales; \
    update-locale; \
    apt-get clean; \
    apt-get update -y;

RUN set -eux; \
    apt-get install apt-utils bash build-essential cmake cmake-data g++ gcc libcurl4-openssl-dev libssl-dev make pkg-config sudo wget curl git htop mc lsb-release -y; \
    sh -c 'echo "deb http://apt.postgresql.org/pub/repos/apt $(lsb_release -cs)-pgdg main" > /etc/apt/sources.list.d/pgdg.list'; \
    wget --quiet -O - https://www.postgresql.org/media/keys/ACCC4CF8.asc | sudo apt-key add -; \
    apt-get update -y; \
    apt-get install postgresql libpq-dev postgresql-server-dev-all -y;

RUN curl -s https://api.github.com/repos/sosedoff/pgweb/releases/latest | grep linux_amd64.zip | grep download | cut -d '"' -f 4 | wget -qi -; \
    unzip pgweb_linux_amd64.zip; \
    rm pgweb_linux_amd64.zip; \
    mv pgweb_linux_amd64 /usr/bin/pgweb;

WORKDIR /opt/$PROJECT_NAME

COPY . .

RUN set -eux; \
    cp docker/default.conf conf/default.conf; \
    cp docker/default.json conf/sites/default.json; \
    cmake -DCMAKE_BUILD_TYPE=Release . -B cmake-build-release; \
    cd cmake-build-release; \
    make install;

RUN set -eux; \
    pg_dropcluster $PG_VERSION $PG_CLUSTER;

COPY ./docker/run.sh /opt/run.sh

EXPOSE 8080
EXPOSE 8081

CMD ["bash", "/opt/run.sh"]
