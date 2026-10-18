# syntax=docker/dockerfile:1.7

# Node.js 22 on the same Ubuntu and HPX version used for the addon and the app.
FROM brakmic/hpx-builder:1.11.0 AS node-base

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        ca-certificates \
        curl \
    && curl -fsSL https://deb.nodesource.com/setup_22.x | bash - \
    && apt-get install -y --no-install-recommends nodejs \
    && rm -rf /var/lib/apt/lists/*

# Build the native addon. HPX template diagnostics are large, so the log is
# written to a file and printed only when the build fails.
FROM node-base AS addon-builder

WORKDIR /addon

COPY addon/package.json addon/package-lock.json ./
COPY addon/binding.gyp ./
COPY addon/src/ src/

RUN npm ci > /tmp/addon-build.log 2>&1 || (tail -n 120 /tmp/addon-build.log; exit 1)

# Application runtime with the same Ubuntu and HPX version as the builder.
FROM node-base AS app-runtime

# HPX links jemalloc. Preload it by soname so the dynamic linker reserves the
# static TLS block before the addon is loaded.
ENV LD_PRELOAD=libjemalloc.so.2

WORKDIR /app

COPY app/package.json app/package-lock.json ./
RUN npm ci

COPY app/index.mjs app/benchmark.mjs ./
COPY app/test ./test
COPY --from=addon-builder /addon/build/Release/hpxaddon.node ./addons/hpxaddon.node

CMD ["node", "index.mjs"]
