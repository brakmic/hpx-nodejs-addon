# Stage 1: Builder
FROM brakmic/hpx-builder:v1.10.0 AS addon-builder

# Install prerequisites for Node.js installation
RUN apt-get update && apt-get install -y \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Install Node.js (version 20)
RUN curl -fsSL https://deb.nodesource.com/setup_20.x | bash - \
    && apt-get install -y nodejs \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /addon

# Copy addon package files first for Docker caching
COPY addon/package.json addon/package-lock.json ./

# Copy binding.gyp after package files
COPY addon/binding.gyp ./

# Copy the HPX addon src
COPY addon/src/ src/

# Install Node.js dependencies, including devDependencies
RUN npm install --include=dev

# Define build argument
#ARG BUILD_VARIANT=debug

# Build the addon using the appropriate build script
#RUN npm run build:${BUILD_VARIANT}
