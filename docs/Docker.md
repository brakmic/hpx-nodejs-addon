# Docker

## Table of Contents
- [Docker](#docker)
  - [Table of Contents](#table-of-contents)
  - [Introduction](#introduction)
  - [Dockerfile Overview](#dockerfile-overview)
    - [hpx-builder.Dockerfile](#hpx-builderdockerfile)
    - [addon.Dockerfile](#addondockerfile)
    - [app.Dockerfile](#appdockerfile)
  - [Building the Docker Images](#building-the-docker-images)
  - [Running the Containers](#running-the-containers)
    - [Running the JavaScript Application (index.mjs)](#running-the-javascript-application-indexmjs)
    - [Running the Benchmark](#running-the-benchmark)
    - [Running the Tests](#running-the-tests)
    - [Debugging the Application](#debugging-the-application)
      - [Editing Files with Nano](#editing-files-with-nano)
      - [Debugging with GDB](#debugging-with-gdb)
  - [Typical Use Cases](#typical-use-cases)
  - [Troubleshooting Docker Issues](#troubleshooting-docker-issues)

---

## Introduction

To simplify the environment setup, building, and running of the HPX Node.js addon, three Dockerfiles are provided. Each has a distinct purpose:

- **hpx-builder.Dockerfile:** Builds and installs the HPX runtime environment.
- **addon.Dockerfile:** Uses the HPX environment to compile the HPX Node.js addon.
- **app.Dockerfile:** Sets up a runtime environment for running JavaScript code (e.g., `index.mjs`, `benchmark.mjs`, tests).

By using these Dockerfiles, you can avoid manual installation of HPX, Node.js, and other dependencies on your host machine. They provide a reproducible environment for development, testing, and benchmarking.

---

## Dockerfile Overview

### hpx-builder.Dockerfile

**Purpose:**  
Builds HPX from source on a clean Ubuntu 22.04 base image.

**Key Steps:**
- Installs system dependencies (cmake, boost, jemalloc, hwloc, etc.).
- Clones the HPX repository at a specified tag (e.g., v1.10.0).
- Configures and compiles HPX with specified CMake options.
- Installs HPX into `/usr/local/hpx`.
- After build, `/usr/local/hpx` contains all the HPX headers and libraries needed by the addon.

**Resulting Image:**  
A base image with HPX fully installed, tagged as `brakmic/hpx-builder:latest` (or a similar tag).

### addon.Dockerfile

**Purpose:**  
Uses the HPX environment from `hpx-builder` to compile the HPX Node.js addon.

**Key Steps:**
- Inherits from `brakmic/hpx-builder:v1.10.0`.
- Installs Node.js v20 and `node-gyp` dependencies.
- Copies the Node.js addon’s `package.json`, `binding.gyp`, and `src` directory.
- Runs `npm install` to build the addon using `node-gyp` and `binding.gyp`.
- Results in a builder image that contains `hpxaddon.node` after the build step.

**Resulting Image:**  
A builder image tagged as `brakmic/hpx-nodejs-addon-builder:v0.1` (or similar) that has the compiled `hpxaddon.node` binary ready.

### app.Dockerfile

**Purpose:**  
Sets up an environment to run the addon’s JavaScript applications (like `index.mjs`, `benchmark.mjs`, and tests).

**Key Steps:**
- Uses a fresh Ubuntu 22.04 base and installs Node.js 20.
- Copies HPX libraries from `hpx-builder` image and the compiled `hpxaddon.node` from `addon-builder` image.
- Installs application-level dependencies (from `app/package.json`).
- Copies `index.mjs`, `benchmark.mjs`, and test files (`hpx-addon.test.mjs`).
- Installs debugging tools (`nano` and `gdb`).
- Sets `LD_LIBRARY_PATH`, `LD_PRELOAD`, etc., so that HPX and jemalloc are available at runtime.

**Resulting Image:**  
A runtime environment tagged as `brakmic/hpx-nodejs-app:v0.1` (or similar). You can run `node index.mjs`, `npm run benchmark`, or the test runner directly inside this container.

---

## Building the Docker Images

The typical build flow is:

1. **Build HPX Image:**
   ```bash
   docker build -t brakmic/hpx-builder:latest -f hpx-builder.Dockerfile .
   ```
   This creates the base HPX environment image.

2. **Build Addon Image:**
   ```bash
   docker build -t brakmic/hpx-nodejs-addon-builder:v0.1 -f addon.Dockerfile .
   ```
   This uses the hpx-builder image to build the addon. After this step, you have an image with `hpxaddon.node`.

3. **Build App Image:**
   ```bash
   docker build -t brakmic/hpx-nodejs-app:latest -f app.Dockerfile .
   ```
   This final image can run the JavaScript code using the compiled addon.

---

## Running the Containers

### Running the JavaScript Application (index.mjs)

```bash
docker run --rm brakmic/hpx-nodejs-app:latest
```

This starts the app container, runs `index.mjs` using the already compiled addon, and prints results.

### Running the Benchmark

```bash
docker run --rm brakmic/hpx-nodejs-app:latest npm run benchmark
```

This runs the benchmarking script inside the app environment.

### Running the Tests

```bash
docker run --rm brakmic/hpx-nodejs-app:latest npm test
```

Or directly:

```bash
docker run --rm brakmic/hpx-nodejs-app:latest node test/hpx-addon.test.mjs
```

### Debugging the Application

Developers can debug the application by accessing the container's shell, editing files with `nano`, and using the GNU Debugger (`gdb`) to troubleshoot issues.

#### Editing Files with Nano

To modify JavaScript files within the container, you can use the `nano` editor as follows:

1. **Access the Container's Shell:**

   Launch a bash shell inside the `brakmic/hpx-nodejs-app` container:

   ```bash
   docker run -it brakmic/hpx-nodejs-app:latest /bin/bash
   ```

2. **Edit the `index.mjs` File:**

   Open `index.mjs` with `nano` to make changes:

   ```bash
   nano index.mjs
   ```

3. **Edit the `benchmark.mjs` File:**

   Similarly, modify `benchmark.mjs` as needed:

   ```bash
   nano benchmark.mjs
   ```

   After editing, save your changes by pressing `Ctrl + O`, then exit `nano` with `Ctrl + X`.

#### Debugging with GDB

To debug JavaScript scripts, especially when dealing with crashes or unexpected behavior in the native addon, you can use `gdb` as follows:

1. **Access the Container's Shell:**

   Launch a bash shell inside the `brakmic/hpx-nodejs-app` container:

   ```bash
   docker run -it brakmic/hpx-nodejs-app:latest /bin/bash
   ```

2. **Start GDB with the Desired Script:**

   To debug `index.mjs`:

   ```bash
   gdb --args node index.mjs
   ```

   Or to debug `benchmark.mjs`:

   ```bash
   gdb --args node benchmark.mjs
   ```

3. **Run the Script Within GDB:**

   Inside the GDB prompt, start execution by typing:

   ```gdb
   run
   ```

4. **Handling Crashes:**

   If the application crashes, GDB will catch the signal. To get a detailed backtrace, enter:

   ```gdb
   bt full
   ```

   This command provides a comprehensive stack trace, helping you identify where the crash occurred.

5. **Additional GDB Commands:**

   - **Set Breakpoints:**
     ```gdb
     break main
     ```
   
   - **Step Through Code:**
     ```gdb
     step
     ```
   
   - **Continue Execution:**
     ```gdb
     continue
     ```
   
   - **Inspect Variables:**
     ```gdb
     print variableName
     ```

   Refer to GDB documentation for more advanced debugging techniques.

**Note:** Debugging native addons with GDB can help identify issues in the C++ code that interfaces with JavaScript. Ensure that symbols are available and that the addon is built with debugging information for more effective debugging sessions.

---

## Typical Use Cases

- **Development:**  
  You can rebuild the `addon.Dockerfile` image after making changes to C++ code and quickly test in the `app` container.

- **CI/CD:**  
  Integrate these Docker steps into CI pipelines so each commit is tested against a known-good HPX environment.

- **Benchmarking on Clean Environments:**  
  Build images on different machines/servers and run benchmarks to compare performance in a standardized environment.

- **Debugging:**  
  Utilize `nano` and `gdb` within the container to modify scripts and debug native addon issues without leaving the Docker environment.

---

## Troubleshooting Docker Issues

- **Build Failures:**  
  Ensure your host has network access, and that `git`, `cmake`, and other tools are properly installed in the builder image. Check for typos in Docker commands.

- **Missing Libraries at Runtime:**  
  If runtime complains about missing HPX libraries, verify that `COPY --from=...` steps in `app.Dockerfile` are correct and that `LD_LIBRARY_PATH` is set properly.

- **Performance Overhead:**  
  Running inside Docker might introduce slight overhead. For accurate benchmarks, ensure you have allocated sufficient CPU and memory resources to the Docker daemon.

- **Debugger Not Found:**  
  If you encounter errors related to `nano` or `gdb` not being found, ensure that the `app.Dockerfile` installs these tools. You can modify the `app.Dockerfile` to include them:

  ```dockerfile
  RUN apt-get update && apt-get install -y nano gdb
  ```

  Rebuild the `app.Dockerfile` image after making this change.
