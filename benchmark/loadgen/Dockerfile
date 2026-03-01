FROM debian:bookworm-slim

RUN set -eux; \
    apt update && apt install -y --no-install-recommends \
    curl wget gnupg2 ca-certificates build-essential git unzip jq bc && \
    apt-get clean && rm -rf /var/lib/apt/lists/*;

# Install wrk
RUN set -eux; \
    git clone --depth 1 https://github.com/wg/wrk.git /tmp/wrk && \
    cd /tmp/wrk && make -j$(nproc) && cp wrk /usr/local/bin/ && \
    rm -rf /tmp/wrk;

# Install hey
RUN set -eux; \
    ARCH=$(dpkg --print-architecture); \
    wget -q "https://hey-release.s3.us-east-2.amazonaws.com/hey_linux_${ARCH}" -O /usr/local/bin/hey && \
    chmod +x /usr/local/bin/hey;

# Install k6
RUN set -eux; \
    curl -fsSL https://dl.k6.io/key.gpg | gpg --dearmor -o /usr/share/keyrings/k6-archive-keyring.gpg && \
    echo "deb [signed-by=/usr/share/keyrings/k6-archive-keyring.gpg] https://dl.k6.io/deb stable main" > /etc/apt/sources.list.d/k6.list && \
    apt update && apt install -y --no-install-recommends k6 && \
    apt-get clean && rm -rf /var/lib/apt/lists/*;

WORKDIR /bench

COPY scripts/ /bench/scripts/
RUN chmod +x /bench/scripts/*.sh

VOLUME /results

CMD ["sleep", "infinity"]
