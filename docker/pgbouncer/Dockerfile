FROM edoburu/pgbouncer:latest

COPY pgbouncer.ini userlist.txt /etc/pgbouncer/

USER root

RUN set -eux; \
    chown postgres:root /etc/pgbouncer/userlist.txt && \
    chmod 640 /etc/pgbouncer/userlist.txt;

USER postgres
