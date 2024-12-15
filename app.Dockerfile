# Use Ubuntu 22.04 as the base image
FROM ubuntu:22.04 AS app-runner

# Install prerequisites and Node.js (version 20)
RUN apt-get update && apt-get install -y \
    curl \
    libjemalloc-dev \
    libasio-dev \
    libboost-dev \
    libhwloc-dev \
    nano \
    gdb \
    valgrind \
    && rm -rf /var/lib/apt/lists/*

# Install Node.js (version 20)
RUN curl -fsSL https://deb.nodesource.com/setup_20.x | bash - \
    && apt-get install -y nodejs \
    && rm -rf /var/lib/apt/lists/*

# Set environment variables for HPX
ENV HPX_INCLUDE_DIR=/usr/local/include
ENV HPX_LIB_DIR=/usr/local/lib

# Set LD_LIBRARY_PATH to include HPX libraries and standard libraries
ENV LD_LIBRARY_PATH=/usr/local/hpx/lib:/usr/local/lib:/usr/lib

# Preload jemalloc to avoid TLS allocation issues
ENV LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libjemalloc.so.2

# Create application directory
WORKDIR /app

# Copy the HPX libraries (if any additional are needed)
COPY --from=brakmic/hpx-builder /usr/local/hpx/ /usr/local/hpx/
COPY --from=brakmic/hpx-builder /usr/local/lib/ /usr/local/lib/

# Define build argument
ARG BUILD_VARIANT=Release

# Copy the compiled addon from the Addon Builder stage
COPY --from=brakmic/hpx-nodejs-addon-builder /addon/build/${BUILD_VARIANT}/hpxaddon.node /app/addons/hpxaddon.node

# Copy package.json and package-lock.json
COPY app/package.json app/package-lock.json ./

# Install Node.js dependencies
RUN npm install --include=dev

# Copy application source code
COPY app/index.mjs ./index.mjs
COPY app/benchmark.mjs ./benchmark.mjs
COPY app/test/hpx-addon.test.mjs ./test/hpx-addon.test.mjs

# Update dynamic linker configuration to include /usr/local/hpx/lib
RUN echo "/usr/local/hpx/lib" > /etc/ld.so.conf.d/hpx.conf && ldconfig

# Define the default command to run the application
CMD ["node", "index.mjs"]
