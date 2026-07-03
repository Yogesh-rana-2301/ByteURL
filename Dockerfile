# ─── Builder ────────────────────────────────────────────────────────────────
FROM debian:bookworm-slim AS builder

RUN apt-get update && apt-get install -y \
    build-essential \
    git \
    wget \
    curl \
    uuid-dev \
    zlib1g-dev \
    libssl-dev \
    libpq-dev \
    libhiredis-dev \
    pkg-config \
    python3 \
    && rm -rf /var/lib/apt/lists/*

# Install a recent CMake (Drogon requires >= 3.20)
RUN wget -q https://github.com/Kitware/CMake/releases/download/v4.1.2/cmake-4.1.2-linux-x86_64.sh \
    && bash cmake-4.1.2-linux-x86_64.sh --skip-license --prefix=/usr/local \
    && rm cmake-4.1.2-linux-x86_64.sh

WORKDIR /app
COPY . /app

# cmake/drogon.cmake sets BUILD_SHARED_LIBS=OFF so drogon, trantor, and jsoncpp
# are all statically linked into the binary. No .so files needed at runtime.
RUN mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DENABLE_DOXYGEN=OFF && \
    cmake --build . --target byteURL --parallel "$(nproc)"

# ─── Runtime image ──────────────────────────────────────────────────────────
FROM debian:bookworm-slim

# Runtime dependencies (drogon/trantor/jsoncpp are statically linked):
#   libssl3        — TLS
#   libpq5         — PostgreSQL client
#   libhiredis0.14 — Redis client
#   zlib1g         — compression
#   libuuid1       — UUID generation
#   gettext-base   — provides envsubst (used by entrypoint.sh to generate config.json)
RUN apt-get update && apt-get install -y \
    libssl3 \
    libpq5 \
    libhiredis0.14 \
    zlib1g \
    libuuid1 \
    gettext-base \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /app/build/byteURL   /app/byteURL
COPY config.template.json              /app/config.template.json
COPY public/                           /app/public/
COPY entrypoint.sh                     /app/entrypoint.sh

RUN chmod +x /app/entrypoint.sh

EXPOSE 8080

# entrypoint.sh generates config.json at startup from environment variables,
# then exec's the byteURL binary. This ensures LISTEN_PORT / LISTEN_ADDR are
# substituted at runtime, not baked in at build time.
CMD ["/app/entrypoint.sh"]
