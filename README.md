# HPX Node.js Addon

**Table of Contents**  
- [HPX Node.js Addon](#hpx-nodejs-addon)
  - [Introduction](#introduction)
  - [Features \& Capabilities](#features--capabilities)
  - [Architecture Overview](#architecture-overview)
  - [Installation \& Setup](#installation--setup)
  - [Configuration Options](#configuration-options)
  - [Usage Examples (Quick Start)](#usage-examples-quick-start)
  - [License](#license)

---

## Introduction

The **HPX Node.js Addon** integrates the [HPX C++ runtime system](https://hpx-docs.stellar-group.org/latest/html/index.html) into Node.js, enabling high-performance parallel computations directly from JavaScript code. By leveraging HPX's advanced parallel algorithms and execution policies, this addon allows developers to perform CPU-intensive operations efficiently without blocking the Node.js event loop.

**Key Benefits:**
- **Seamless Parallelization:** Offload heavy computations to HPX threads while maintaining a responsive Node.js application.
- **Advanced Execution Policies:** Utilize HPX's parallel, sequential, and unsequenced execution policies to optimize performance based on your specific use case.
- **Customizable Predicates & Comparators:** Define and use custom JavaScript functions for complex data processing tasks.
- **Batch Processing:** Enhance performance by minimizing cross-boundary calls between C++ and JavaScript through batch processing strategies.

---

## Features & Capabilities

- **Seamless Parallelization of C++ Algorithms in Node.js:**  
  Offload heavy computations to HPX and retrieve results as typed arrays, ensuring non-blocking operations in your Node.js applications.

- **Configurable Execution Policies:**  
  Choose between sequential (`seq`), parallel (`par`), or parallel unsequenced (`par_unseq`) execution policies. Control the transition from parallel to sequential execution based on a user-defined threshold.

- **Custom Predicates and Comparators:**  
  Define custom JavaScript predicate or comparator functions. The addon employs [ThreadSafeFunctions (TSFNs)](https://github.com/nodejs/node-addon-api/blob/main/doc/threadsafe.md) to safely invoke these functions from C++ threads without hindering the Node.js event loop.

- **Batch Processing of JavaScript Callbacks:**  
  For operations like `countIf`, `copyIf`, `sortComp`, and `partialSortComp`, the addon utilizes batch processing to minimize overhead. This involves converting per-element predicates/comparators into batch versions, allowing the addon to handle large datasets efficiently.

- **Flexible Configuration:**  
  Adjust parameters such as `threadCount`, `threshold`, `logLevel`, and more through a JSON configuration object to tailor the addon's performance to your system and application needs.

- **Robust Error Handling:**  
  Receive meaningful error messages and ensure that exceptions within asynchronous operations are properly propagated to JavaScript, facilitating easier debugging and more resilient applications.

- **Scalability:**  
  Designed to handle large arrays efficiently, the addon scales well with data size and system resources, making it suitable for demanding real-time data processing applications.

---

## Architecture Overview

At a high level, the **HPX Node.js Addon** operates through the following components:

1. **Node-API Integration:**  
   The addon is built using Node.js native addons, registering functions like `sort`, `count`, `copyIf`, etc., as JavaScript-callable functions. Each operation returns a `Promise` to facilitate asynchronous execution.

2. **HPX Runtime:**  
   Upon initialization, the addon starts the HPX runtime in a background thread. Parallel algorithms (e.g., `hpx::sort`, `hpx::merge`) are dispatched as HPX tasks, running concurrently across multiple HPX threads.

3. **Async Helpers & ThreadSafeFunctions:**  
   When a JavaScript predicate or comparator is provided (e.g., for `copyIf`), the addon creates a `ThreadSafeFunction` (TSFN). To reduce overhead, a batch approach is employed:
   - **Batch Conversion:** Convert per-element predicates/comparators into batch-processing functions using provided helper functions (`elementPredicateToMask`, `elementComparatorToKeys`).
   - **Single Invocation:** Call the batch function once with the entire array, receiving a mask or key array in return.
   - **Efficient Processing:** Use the received mask or keys in C++ to perform the desired operation without per-element JS calls.

4. **Data Conversion:**  
   Input and output arrays are typically `Int32Array` on the JavaScript side, mapped to `std::vector<int32_t>` in C++. This mapping avoids unnecessary copies, ensuring efficient data handling.

5. **TSFN Manager & Predicate Helpers:**  
   A TSFN manager oversees all thread-safe functions, ensuring they are properly released when finalizing HPX or shutting down the addon.

For an in-depth understanding of the architecture, refer to the [Architecture](./docs/Architecture.md) document.

---

## Installation & Setup

**Prerequisites:**

- **Node.js v20 or Newer:**  
  Ensure that you have Node.js version 20 or later installed.

- **HPX Installed:**  
  HPX should be installed in `/usr/local/hpx`. You can use the provided Dockerfiles for a hassle-free setup.

- **C++17 Toolchain:**  
  A working C++17 compiler and related build tools are required.

**Steps:**

1. **Clone the Repository:**

   ```bash
   git clone https://github.com/brakmic/hpx-nodejs-addon.git
   cd hpx-nodejs-addon
   ```

2. **Install Dependencies and Build the Addon:**

   ```bash
   npm install
   ```

   This command executes `node-gyp rebuild`, compiling the C++ addon.

3. **Initialize HPX in Your Application:**

   Before using any addon functions, initialize HPX as shown in the [Usage Examples](#usage-examples-quick-start).

**Note:** For detailed instructions, refer to the [Installation](./docs/Installation.md) guide.

---

## Configuration Options

You can configure the HPX runtime by passing a configuration object to `initHPX`. Here's the available configuration options:

```js
await hpxaddon.initHPX({
  executionPolicy: 'par',       // Execution policy: "seq", "par", "par_unseq"
  threshold: 10000,              // Threshold for task granularity (smaller arrays run sequentially)
  threadCount: 4,                // Number of HPX threads to spawn
  loggingEnabled: true,          // Enable or disable logging
  logLevel: 'info',              // Logging level: "debug", "info", "warn", "error"
  addonName: 'hpxaddon'          // Name of the addon (optional)
});
```

**Configuration Details:**

- **executionPolicy:**  
  Determines how algorithms are executed.  
  - `"seq"`: Sequential execution.  
  - `"par"`: Parallel execution.  
  - `"par_unseq"`: Parallel unsequenced execution.

- **threshold:**  
  Sets the threshold for task granularity. Arrays smaller than this value will execute sequentially to avoid parallel overhead.

- **threadCount:**  
  Specifies the number of HPX threads to spawn, typically aligned with the number of CPU cores for optimal performance.

- **loggingEnabled & logLevel:**  
  Control logging behavior. Enable logging and set the desired verbosity level to monitor internal operations and debug issues.

For a comprehensive list and explanation of configuration options, see [Configuration](./docs/Configuration.md).

---

## Usage Examples (Quick Start)

Here's a quick example to get you started with the HPX Node.js Addon:

```js
import { createRequire } from 'module';
const require = createRequire(import.meta.url);
const hpxaddon = require('./addons/hpxaddon.node');

(async () => {
  // Initialize HPX
  await hpxaddon.initHPX({
    executionPolicy: 'par',
    threshold: 10000,
    threadCount: 4,
    loggingEnabled: true,
    logLevel: 'debug'
  });

  // Sorting an Int32Array
  const arr = Int32Array.from([5, 3, 8, 1, 9]);
  const sorted = await hpxaddon.sort(arr);
  console.log('Sorted Array:', sorted); // Int32Array [1, 3, 5, 8, 9]

  // Counting occurrences of a value
  const data = Int32Array.from([1, 2, 2, 3, 2, 4, 2]);
  const countOf2 = await hpxaddon.count(data, 2);
  console.log(`Count of 2: ${countOf2}`); // Output: Count of 2: 4

  // Finalize HPX before exiting
  await hpxaddon.finalizeHPX();
})();
```

**Note:** Ensure that you call `finalizeHPX()` to gracefully shut down the HPX runtime when your application is terminating.

## License

This project is distributed under the MIT License. See [LICENSE](./LICENSE) for full details.

---

**Final Notes:**

The **HPX Node.js Addon** empowers developers to harness the full potential of parallel computing within Node.js applications. By integrating HPX's robust C++ runtime with JavaScript's flexibility, you can build efficient, high-performance applications tailored to complex data processing needs.

**Additional Resources:**
- [Architecture](./docs/Architecture.md)
- [Advanced Topics](./docs/AdvancedTopics.md)
- [Examples](./docs/Examples.md)
- [Benchmarking](./docs/Benchmarking.md)
- [Troubleshooting](./docs/Troubleshooting.md)

Feel free to explore these documents for a deeper understanding and advanced usage scenarios.