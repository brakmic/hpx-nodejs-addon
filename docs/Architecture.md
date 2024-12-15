# Architecture

**Table of Contents**  
- [Architecture](#architecture)
  - [Introduction](#introduction)
  - [Features \& Capabilities](#features--capabilities)
  - [Architecture Overview](#architecture-overview)
    - [Node-API Integration](#node-api-integration)
    - [Asynchronous Execution with HPX](#asynchronous-execution-with-hpx)
    - [ThreadSafeFunctions and JavaScript Callbacks](#threadsafefunctions-and-javascript-callbacks)
    - [Data Conversion and Memory Management](#data-conversion-and-memory-management)
    - [Logging and Debugging](#logging-and-debugging)
  - [Installation \& Setup](#installation--setup)
  - [Configuration Options](#configuration-options)
  - [Usage Examples (Quick Start)](#usage-examples-quick-start)
  - [Detailed Example Code](#detailed-example-code)
    - [Using Custom Predicates and Comparators](#using-custom-predicates-and-comparators)
    - [Example: countIf (Predicate)](#example-countif-predicate)
    - [Example: copyIf (Predicate)](#example-copyif-predicate)
    - [Example: sortComp (Comparator)](#example-sortcomp-comparator)
    - [Example: partialSortComp (Comparator)](#example-partialsortcomp-comparator)
  - [Troubleshooting](#troubleshooting)

---

## Introduction

The **HPX Node.js Addon** seamlessly integrates the [HPX C++ runtime system](https://hpx-docs.stellar-group.org/latest/html/index.html) into Node.js, enabling high-performance parallel computations directly from JavaScript code. By leveraging HPX's advanced parallel algorithms and execution policies, this addon allows developers to perform CPU-intensive operations efficiently without blocking the Node.js event loop.

**Key Benefits:**
- **Seamless Parallelization:** Offload heavy computations to HPX threads while maintaining a responsive Node.js application.
- **Advanced Execution Policies:** Utilize HPX's parallel, sequential, and unsequenced execution policies to optimize performance based on your specific use case.
- **Customizable Predicates & Comparators:** Define and use custom JavaScript functions for complex data processing tasks.
- **Batch Processing:** Enhance performance by minimizing cross-boundary calls between C++ and JavaScript through batch processing strategies.

For a comprehensive understanding of the addon's architecture and advanced usage scenarios, refer to the [Architecture](./docs/Architecture.md) and [Advanced Topics](./docs/AdvancedTopics.md) documents. For additional examples and practical demonstrations, explore the [Examples](./docs/Examples.md) section.

---

## Features & Capabilities

- **Seamless Parallelization of C++ Algorithms in Node.js:**  
  Offload heavy computations to HPX and retrieve results as typed arrays, ensuring non-blocking operations in your Node.js applications.

- **Configurable Execution Policies:**  
  Choose between sequential (`seq`), parallel (`par`), or parallel unsequenced (`par_unseq`) execution policies. Control the transition from parallel to sequential execution based on a user-defined threshold.

- **Custom Predicates and Comparators:**  
  Define custom JavaScript predicate or comparator functions. The addon employs [ThreadSafeFunctions (TSFNs)](https://github.com/nodejs/node-addon-api/blob/main/doc/threadsafe.md) to safely invoke these functions from C++ threads without hindering the Node.js event loop.

- **Batch Processing of JavaScript Callbacks:**  
  For operations like `countIf`, `copyIf`, `sortComp`, and `partialSortComp`, the addon utilizes batch processing to minimize overhead. This involves converting per-element predicates/comparators into batch versions using provided helper functions (`elementPredicateToMask`, `elementComparatorToKeys`), allowing the addon to handle large datasets efficiently.

- **Flexible Configuration:**  
  Adjust parameters such as `threadCount`, `threshold`, `logLevel`, and more through a JSON configuration object to tailor the addon's performance to your system and application needs.

- **Robust Error Handling:**  
  Receive meaningful error messages and ensure that exceptions within asynchronous operations are properly propagated to JavaScript, facilitating easier debugging and more resilient applications.

- **Scalability:**  
  Designed to handle large arrays efficiently, the addon scales well with data size and system resources, making it suitable for demanding real-time data processing applications.

- **Debugging Tools:**  
  Leverage built-in logging and integrate with debugging tools like `nano` and `gdb` within Docker containers to troubleshoot both JavaScript and native C++ components effectively.

---

## Architecture Overview

The **HPX Node.js Addon** operates through a well-defined architecture that ensures efficient integration between JavaScript and HPX's C++ runtime. Here's an in-depth look at its core components:

### Node-API Integration

- **Node-API (N-API):**  
  The addon is built using [Node-API](https://nodejs.org/api/n-api.html), providing a stable and ABI-compatible interface for building native addons in C++. This ensures that the addon remains compatible across different Node.js versions without requiring recompilation.

- **Function Registration:**  
  Functions like `sort`, `count`, `copyIf`, `sortComp`, etc., are registered as JavaScript-callable functions. Each function returns a `Promise`, enabling asynchronous execution and seamless integration with JavaScript's async/await patterns.

### Asynchronous Execution with HPX

- **HPX Runtime Initialization:**  
  The addon initializes the HPX runtime in a background thread using the `initHPX` function. This ensures that HPX's parallel execution capabilities are available without blocking the main Node.js thread.

- **Parallel Algorithms:**  
  When a JavaScript function like `hpxaddon.sort()` is invoked, the addon schedules the corresponding HPX parallel algorithm (e.g., `hpx::sort`) to run asynchronously. These operations are executed across multiple HPX threads, leveraging multi-core architectures for enhanced performance.

- **Promise-Based API:**  
  Each asynchronous operation returns a `Promise` that resolves with the result once the HPX task completes. This design keeps the Node.js event loop responsive, even during intensive computations.

### ThreadSafeFunctions and JavaScript Callbacks

- **ThreadSafeFunctions (TSFNs):**  
  For operations requiring custom JavaScript predicates or comparators (e.g., `countIf`, `copyIf`, `sortComp`), the addon uses TSFNs to safely call back into JavaScript from C++ threads. This mechanism ensures thread safety and prevents potential data races or corruption.

- **Batch Processing Strategy:**  
  To minimize the overhead of multiple TSFN calls, the addon employs a batch processing approach:
  1. **Conversion:** Per-element JavaScript predicates/comparators are converted into batch versions using helper functions (`elementPredicateToMask`, `elementComparatorToKeys`).
  2. **Single Invocation:** The batch function is called once with the entire data array, returning a mask (`Uint8Array`) or keys (`Int32Array`).
  3. **Efficient Processing:** The addon uses the returned mask or keys in C++ to perform the desired operation without invoking JavaScript functions for each element.

### Data Conversion and Memory Management

- **Typed Arrays:**  
  The addon primarily works with `Int32Array` for input and output, mapping them to `std::vector<int32_t>` in C++. This direct mapping avoids unnecessary data copying, ensuring efficient memory usage.

- **Memory Handling:**  
  - **Input Data:**  
    When a JavaScript `Int32Array` is passed to the addon, it is mapped to a C++ vector for processing.
  
  - **Output Data:**  
    Results are returned as new `Int32Array` instances, created from C++ vectors after processing. This ensures that data flows seamlessly between JavaScript and C++ without manual intervention.

- **Shared Pointers:**  
  The addon uses `std::shared_ptr<std::vector<int32_t>>` to manage data lifetimes, especially when dealing with asynchronous HPX tasks. This ensures that data remains valid throughout the operation's lifecycle.

### Logging and Debugging

- **Custom Logger:**  
  The addon includes a custom logging system controlled by configuration options (`loggingEnabled`, `logLevel`). Logs provide insights into the HPX runtime lifecycle, async operations, and TSFN callbacks.

- **Debugging Tools:**  
  Within Docker containers, developers can use tools like `nano` for editing JavaScript files and `gdb` for debugging native C++ code. Detailed logging combined with these tools facilitates effective troubleshooting.

For a deeper dive into each architectural component, refer to the [Architecture](./docs/Architecture.md) document.

---

## Installation & Setup

**Prerequisites:**

- **Node.js v20 or Newer:**  
  Ensure that you have Node.js version 20 or later installed on your system.

- **HPX Installed:**  
  HPX should be installed in `/usr/local/hpx`. You can use the provided Dockerfiles for a hassle-free setup or follow the [Installation](./docs/Installation.md) guide for manual installation.

- **C++17 Toolchain:**  
  A working C++17 compiler and related build tools are required to compile the addon.

**Steps:**

1. **Clone the Repository:**

   ```bash
   git clone https://github.com/your-repo/hpx-nodejs-addon.git
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

For more extensive examples, including custom predicates and comparators, refer to the [Detailed Example Code](#detailed-example-code) section.

---

## Detailed Example Code

### Using Custom Predicates and Comparators

To perform advanced operations like `countIf` and `copyIf`, you can define custom JavaScript predicates and comparators. Here's how to convert per-element functions into batch-processing functions using helper functions:

```js
/**
 * Converts a per-element predicate function into a batch mask generator.
 *
 * @param {Function} pred - A function that takes a single element and returns a boolean.
 * @returns {Function} - A function that takes an Int32Array and returns a Uint8Array mask.
 */
function elementPredicateToMask(pred) {
  return (inputArr) => {
    const mask = new Uint8Array(inputArr.length);
    for (let i = 0; i < inputArr.length; i++) {
      mask[i] = pred(inputArr[i]) ? 1 : 0;
    }
    return mask;
  };
}

/**
 * Converts a per-element comparator function into a batch key extractor.
 *
 * @param {Function} comp - A comparator function that takes two elements and returns a boolean.
 * @returns {Function} - A function that takes an Int32Array and returns an Int32Array of keys.
 */
function elementComparatorToKeys(comp) {
  // Determine sorting order based on a test comparison
  const testVal = comp(2, 1);
  return (inputArr) => {
    const keys = new Int32Array(inputArr.length);
    if (testVal) {
      // Descending order: negate the values to reverse sorting
      for (let i = 0; i < inputArr.length; i++) {
        keys[i] = -inputArr[i];
      }
    } else {
      // Ascending order: keys are the same as input values
      for (let i = 0; i < inputArr.length; i++) {
        keys[i] = inputArr[i];
      }
    }
    return keys;
  };
}
```

### Example: countIf (Predicate)

Count the number of elements that satisfy a custom predicate.

```js
const arr = Int32Array.from([1, 2, 3, 4, 5, 6, 7, 8, 9]);

// Define a per-element predicate: elements greater than 5
const isGreaterThanFive = (val) => val > 5;

// Convert the per-element predicate to a batch mask generator
const isGreaterThanFiveMask = elementPredicateToMask(isGreaterThanFive);

// Using countIf with the batch mask generator
const countRes = await hpxaddon.countIf(arr, isGreaterThanFiveMask);
console.log(`Elements > 5: ${countRes}`); // Output: Elements > 5: 4
```

### Example: copyIf (Predicate)

Copy elements that satisfy a custom predicate into a new `Int32Array`.

```js
const arr = Int32Array.from([1, 2, 3, 4, 5, 6, 7, 8, 9]);

// Define a per-element predicate: even numbers
const isEven = (val) => val % 2 === 0;

// Convert the per-element predicate to a batch mask generator
const isEvenMask = elementPredicateToMask(isEven);

// Using copyIf with the batch mask generator
const evens = await hpxaddon.copyIf(arr, isEvenMask);
console.log(`Even numbers: ${Array.from(evens)}`); // Output: Even numbers: 2,4,6,8
```

### Example: sortComp (Comparator)

Sort an `Int32Array` based on a custom comparator function.

```js
const input = Int32Array.from([10, 5, 8, 2, 9]);

// Define a per-element comparator for descending order
const descendingComparator = (a, b) => a > b;

// Convert the per-element comparator to a batch key extractor
const descendingKeys = elementComparatorToKeys(descendingComparator);

// Using sortComp with the batch key extractor
const sortedDesc = await hpxaddon.sortComp(input, descendingKeys);
console.log(`Sorted descending: ${Array.from(sortedDesc)}`); // Output: Sorted descending: 10,9,8,5,2
```

### Example: partialSortComp (Comparator)

Partially sort an `Int32Array` so that the first 'mid' elements are the smallest ones in ascending order.

```js
const partArr = Int32Array.from([9, 7, 5, 3, 1]);

// Define a per-element comparator for ascending order
const ascendingComparator = (a, b) => a < b;

// Convert the per-element comparator to a batch key extractor
const ascendingKeys = elementComparatorToKeys(ascendingComparator);

const mid = 3;

// Using partialSortComp with the batch key extractor
const partiallySorted = await hpxaddon.partialSortComp(partArr, mid, ascendingKeys);
console.log(`Partial sort (first ${mid} smallest): ${Array.from(partiallySorted)}`);
// Possible Output: Partial sort (first 3 smallest): 1,3,5,7,9
```

**Note:** The first `mid` elements will be the smallest `mid` elements in ascending order. The remaining elements may not be fully sorted.

For more advanced examples and scenarios, refer to the [Examples](./docs/Examples.md) document.

---

## Troubleshooting

Encountering issues is a natural part of development. Here's how to address common problems with the **HPX Node.js Addon**:

- **Check Logs for Detailed Information:**
  - Enable `loggingEnabled` and set `logLevel` to `debug` to view detailed internal operations.

- **HPX Fails to Start:**
  - Ensure HPX is correctly installed and that the `LD_LIBRARY_PATH` is properly set.
  - Verify that all dependencies are installed and that there are no version mismatches.

- **Missing Libraries at Runtime:**
  - Confirm that `COPY --from=...` steps in the `app.Dockerfile` are correct.
  - Ensure that `LD_LIBRARY_PATH` includes the directories where HPX and other dependencies are installed.

- **Performance Overhead:**
  - Running inside Docker might introduce slight overhead. Allocate sufficient CPU and memory resources to the Docker daemon for accurate benchmarks.

- **Debugger Tools Not Found:**
  - If `nano` or `gdb` are not available within the container, ensure they are installed via the `app.Dockerfile`:
  
    ```dockerfile
    RUN apt-get update && apt-get install -y nano gdb
    ```
  
  - Rebuild the `app.Dockerfile` image after making this change.

- **Invalid Input Types:**
  - Ensure that all inputs to addon functions are of the expected types. For example, `sort` expects an `Int32Array`.
  - Use try-catch blocks to handle and log errors gracefully.

- **Predicate/Comparator Errors:**
  - Ensure that predicate functions return a `Uint8Array` and comparator functions return an `Int32Array` of keys.
  - Verify that batch functions are correctly implemented to avoid mismatched array lengths or types.

For unresolved issues, consider consulting the [Troubleshooting Docs](./docs/Troubleshooting.md).
