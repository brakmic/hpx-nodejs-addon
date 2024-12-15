# Building the Addon

## Table of Contents
- [Building the Addon](#building-the-addon)
  - [Table of Contents](#table-of-contents)
  - [Introduction](#introduction)
  - [Prerequisites](#prerequisites)
  - [binding.gyp and node-gyp](#bindinggyp-and-node-gyp)
  - [Build Steps](#build-steps)
  - [Common Build Issues](#common-build-issues)
  - [Optimizations and Build Variants](#optimizations-and-build-variants)

---

## Introduction

The addon uses N-API and C++17 code integrated with HPX. It’s built using `node-gyp` and a `binding.gyp` file that describes how to compile and link the project. Once built, it produces a `.node` binary (e.g., `hpxaddon.node`) that you can require or import in JavaScript.

---

## Prerequisites

- **Node.js & npm:**  
  Ensure Node.js v20+ and npm are installed.
  
- **HPX Installed:**  
  HPX headers and libraries must be accessible. By default, the `binding.gyp` expects HPX in `/usr/local/hpx`.
  
- **C++17 Toolchain:**  
  A compiler that supports C++17 (e.g., `g++` 9+ or Clang 10+).

- **node-gyp:**  
  Installed globally (`npm install -g node-gyp`) or usable via `npx node-gyp`.

**Note:** Using the provided Docker images can simplify these requirements. See [docs/Docker.md](./Docker.md).

---

## binding.gyp and node-gyp

**`binding.gyp`** is a configuration file for `node-gyp` that:

- Lists source files (`addon.cpp`, `hpx_wrapper.cpp`, `hpx_manager.cpp`, etc.).
- Specifies include directories (e.g., `node_modules/node-addon-api`, `/usr/local/hpx/include`).
- Specifies libraries and link flags (HPX, jemalloc, hwloc).
- Sets compiler flags (`-std=c++17`, `-frtti`, etc.).

`node-gyp` reads `binding.gyp` and invokes the system compiler and linker to produce `hpxaddon.node`.

---

## Build Steps

1. **Install Dependencies:**
   ```bash
   npm install
   ```
   This runs `node-gyp rebuild` as defined in the `package.json` scripts, compiling the addon.

2. **Check Output:**
   After a successful build, you should see a `build/Release/hpxaddon.node` file. This is your native addon binary.

3. **Testing the Build:**
   Import the addon in a simple script:
   ```js
   import { createRequire } from 'module';
   const require = createRequire(import.meta.url);
   const hpxaddon = require('./build/Release/hpxaddon.node');
   console.log(hpxaddon);
   ```
   If it prints an object with function keys (`sort`, `count`, etc.), the build is successful.

---

## Common Build Issues

- **HPX Not Found:**  
  If you see linker errors about missing HPX symbols, ensure `/usr/local/hpx` or the correct HPX installation path is accessible. Update `binding.gyp` or set `LD_LIBRARY_PATH` as needed.

- **Missing node-addon-api:**  
  Ensure `npm install` is done so `node-addon-api` is available.

- **Compiler Compatibility:**  
  If you get C++ compilation errors, check that your compiler supports `-std=c++17`. Try updating `g++` or `clang`.

- **Permission Denied (Docker):**  
  If running inside Docker, ensure you have the correct user permissions or run as root.

---

## Optimizations and Build Variants

- **Debug vs Release:**
  By default, `node-gyp` builds in Release mode. You can specify:
  ```bash
  node-gyp rebuild --debug
  ```
  to get a debug build with more verbose logs and no optimization.

- **Additional Flags:**
  You can add custom defines or compile flags in `binding.gyp` if you want to experiment with optimizations (e.g., `-O3`, `-march=native`).

- **Logging Control:**
  Logging behavior is runtime-configured, not build-configured. But debug builds might provide more helpful stack traces.

---

With these steps, you should have a fully compiled HPX Node.js addon. If you prefer a simpler environment, see [docs/Docker.md](./Docker.md) for running the build inside a provided Docker image.
