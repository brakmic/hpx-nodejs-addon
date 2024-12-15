# Examples

## Table of Contents
- [Examples](#examples)
  - [Table of Contents](#table-of-contents)
  - [Introduction](#introduction)
  - [Basic Examples](#basic-examples)
    - [Sorting](#sorting)
    - [Counting Occurrences](#counting-occurrences)
    - [Copying Arrays](#copying-arrays)
  - [Using Custom Predicates and Comparators](#using-custom-predicates-and-comparators)
    - [Helper Functions](#helper-functions)
    - [countIf (Predicate)](#countif-predicate)
    - [copyIf (Predicate)](#copyif-predicate)
    - [sortComp (Comparator)](#sortcomp-comparator)
    - [partialSortComp (Comparator)](#partialsortcomp-comparator)
  - [Batch Processing for Performance](#batch-processing-for-performance)
    - [How It Works](#how-it-works)
    - [Example Clarifications](#example-clarifications)
    - [Benefits](#benefits)
  - [Large Data Handling](#large-data-handling)
    - [Guidelines](#guidelines)
    - [Example: Sorting a Large Array](#example-sorting-a-large-array)
    - [Performance Tips](#performance-tips)
  - [Benchmarking Example](#benchmarking-example)
    - [Benchmark Script: `benchmark.mjs`](#benchmark-script-benchmarkmjs)
    - [Running the Benchmark](#running-the-benchmark)
    - [Expected Output](#expected-output)
  - [Advanced Example: Custom Sorting with Key Extraction](#advanced-example-custom-sorting-with-key-extraction)
    - [Example: Sorting Based on Absolute Values](#example-sorting-based-on-absolute-values)
  - [Error Handling Example](#error-handling-example)
    - [Example: Handling Invalid Input](#example-handling-invalid-input)
    - [Example: Handling Predicate Errors](#example-handling-predicate-errors)

---

## Introduction

These examples demonstrate various ways to utilize the HPX Node.js addon in real-world scenarios. It is assumed that you have already read the main [README](../README.md) and [Architecture](./Architecture.md) documents for foundational understanding.

**Prerequisites:**
- **Build the Addon**: Ensure that the addon is built by running `npm install` or `node-gyp rebuild`.
- **Access the Addon Binary**: Make sure the `hpxaddon.node` binary is accessible within your project directory.

**Initialization:**

Before executing any operations, initialize the HPX runtime. This should typically be done once at the start of your application.

```js
import { createRequire } from 'module';
const require = createRequire(import.meta.url);
const hpxaddon = require('./addons/hpxaddon.node');

// Initialize HPX once at startup
await hpxaddon.initHPX({
  executionPolicy: 'par',       // 'par' for parallel, 'par_unseq' for parallel unsequenced
  threshold: 10000,              // Threshold for task granularity
  threadCount: 4,                // Number of HPX threads to spawn
  loggingEnabled: true,          // Enable logging
  logLevel: 'info'               // Set logging level ('debug', 'info', 'warn', 'error')
});

// ... execute examples here ...

// Finalize HPX gracefully before exiting the application
await hpxaddon.finalizeHPX();
```

**Note:** Always call `hpxaddon.finalizeHPX()` to gracefully shut down the HPX runtime when your application is terminating.

---

## Basic Examples

### Sorting

Sort an `Int32Array` in ascending order using HPX's parallel sort.

```js
const arr = Int32Array.from([5, 3, 8, 1, 9]);
const sorted = await hpxaddon.sort(arr);
console.log(`Sorted: ${Array.from(sorted)}`);
// Output: Sorted: 1,3,5,8,9
```

### Counting Occurrences

Count how many times a specific value appears in an `Int32Array`.

```js
const data = Int32Array.from([1, 2, 2, 3, 2, 4, 2]);
const countOf2 = await hpxaddon.count(data, 2);
console.log(`Count of 2: ${countOf2}`);
// Output: Count of 2: 4
```

### Copying Arrays

Create an asynchronous copy of an `Int32Array`.

```js
const original = Int32Array.from([10, 20, 30]);
const copy = await hpxaddon.copy(original);
console.log(`Copied: ${Array.from(copy)}`);
// Output: Copied: 10,20,30
```

---

## Using Custom Predicates and Comparators

The addon allows you to perform more advanced operations by using custom predicates and comparators. To optimize performance, these predicates and comparators should be converted into their batch versions using helper functions before being passed to the addon functions.

### Helper Functions

First, define the helper functions to convert per-element predicates and comparators into batch versions.

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

These helper functions transform per-element functions into batch-processing functions that the addon can utilize efficiently.

### countIf (Predicate)

Count the number of elements that satisfy a custom predicate.

```js
const arr = Int32Array.from([1, 2, 3, 4, 5, 6, 7, 8, 9]);

// Define a per-element predicate: elements greater than 5
const isGreaterThanFive = (val) => val > 5;

// Convert the per-element predicate to a batch mask generator
const isGreaterThanFiveMask = elementPredicateToMask(isGreaterThanFive);

// Using countIf with the batch mask generator
const countRes = await hpxaddon.countIf(arr, isGreaterThanFiveMask);
console.log(`Elements > 5: ${countRes}`);
// Output: Elements > 5: 4
```

### copyIf (Predicate)

Copy elements that satisfy a custom predicate into a new `Int32Array`.

```js
const arr = Int32Array.from([1, 2, 3, 4, 5, 6, 7, 8, 9]);

// Define a per-element predicate: even numbers
const isEven = (val) => val % 2 === 0;

// Convert the per-element predicate to a batch mask generator
const isEvenMask = elementPredicateToMask(isEven);

// Using copyIf with the batch mask generator
const evens = await hpxaddon.copyIf(arr, isEvenMask);
console.log(`Even numbers: ${Array.from(evens)}`);
// Output: Even numbers: 2,4,6,8
```

### sortComp (Comparator)

Sort an `Int32Array` based on a custom comparator function.

```js
const input = Int32Array.from([10, 5, 8, 2, 9]);

// Define a per-element comparator for descending order
const descendingComparator = (a, b) => a > b;

// Convert the per-element comparator to a batch key extractor
const descendingKeys = elementComparatorToKeys(descendingComparator);

// Using sortComp with the batch key extractor
const sortedDesc = await hpxaddon.sortComp(input, descendingKeys);
console.log(`Sorted descending: ${Array.from(sortedDesc)}`);
// Output: Sorted descending: 10,9,8,5,2
```

### partialSortComp (Comparator)

Partially sort an `Int32Array` so that the first 'middle' elements are the smallest ones in ascending order.

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

---

## Batch Processing for Performance

The HPX Node.js addon optimizes performance by handling predicate and comparator functions in batches internally. This approach minimizes the overhead associated with multiple cross-boundary calls between C++ and JavaScript.

### How It Works

1. **Batch Invocation:**  
   When you provide a predicate or comparator function, the addon invokes the corresponding batch function **once** with the entire `Int32Array`. For predicates, it expects a corresponding mask (`Uint8Array`); for comparators, it expects a key array (`Int32Array`).

2. **Internal Processing:**  
   - **Predicates:**  
     The batch mask (`Uint8Array`) indicates which elements satisfy the predicate (`1` for `true`, `0` for `false`). The addon uses this mask in C++ to perform operations like counting or copying elements.
   
   - **Comparators:**  
     The batch key (`Int32Array`) provides keys that determine the sorting order. The addon utilizes these keys in C++ to sort the array efficiently using parallel algorithms.

3. **User Simplicity:**  
   From the user's perspective, you continue to define simple per-element predicates and comparators. The helper functions convert these into batch-processing functions, and the addon handles the rest internally. This design abstracts away the complexity of batch processing, allowing you to write intuitive and straightforward code.

### Example Clarifications

Although the addon handles batching internally, it's essential to convert your per-element predicates and comparators into batch versions using the provided helper functions. This ensures optimal performance and compatibility with the addon's internal mechanisms.

**Example:**

```js
const arr = Int32Array.from([2, 4, 6, 8, 10]);

// Define a standard per-element predicate
const isPositive = (val) => val > 0;

// Convert the per-element predicate to a batch mask generator
const isPositiveMask = elementPredicateToMask(isPositive);

// Use copyIf with the batch mask generator
const positives = await hpxaddon.copyIf(arr, isPositiveMask);
console.log(`Positive numbers: ${Array.from(positives)}`);
// Output: Positive numbers: 2,4,6,8,10
```

**Behind the Scenes:**

- The addon calls `isPositiveMask` once with the entire array.
- `isPositiveMask` returns a `Uint8Array` mask indicating which elements are positive.
- C++ processes this mask using HPX's parallel algorithms to efficiently copy the qualifying elements.

### Benefits

- **Reduced Overhead:**  
  Batch processing eliminates the need for per-element function invocations, significantly enhancing performance, especially for large datasets.

- **Thread Safety:**  
  The addon ensures that all interactions with JavaScript functions are thread-safe using ThreadSafeFunctions (TSFNs), maintaining data integrity.

- **Seamless Integration:**  
  Developers can write standard predicates and comparators without worrying about the underlying batch mechanics, leading to cleaner and more maintainable code.

---

## Large Data Handling

Leveraging HPX's parallel computing capabilities is particularly beneficial when dealing with large datasets. Here are some guidelines and an example to illustrate effective usage:

### Guidelines

- **Execution Policy:**  
  Ensure that the `executionPolicy` is set to `'par'` or `'par_unseq'` in the HPX configuration for optimal parallel performance.

- **Threshold Tuning:**  
  Adjust the `threshold` parameter to balance between parallel overhead and task granularity. A suitable threshold ensures that small arrays do not incur unnecessary parallel overhead.

- **Thread Count Configuration:**  
  Increase the `threadCount` based on the number of available CPU cores to maximize parallelism.

- **Memory Management:**  
  For extremely large datasets, ensure that your system has sufficient memory to handle multiple copies of large arrays during processing.

### Example: Sorting a Large Array

```js
const largeSize = 1000000; // 1 million elements
const largeArray = new Int32Array(largeSize);

// Populate the array with random integers
for (let i = 0; i < largeSize; i++) {
  largeArray[i] = Math.floor(Math.random() * largeSize);
}

console.time('HPX Sort');
const sortedLarge = await hpxaddon.sort(largeArray);
console.timeEnd('HPX Sort');
console.log(`First 10 sorted values: ${Array.from(sortedLarge.slice(0, 10))}`);
// Output Example:
// HPX Sort: 1500.123ms
// First 10 sorted values: 0,1,2,3,4,5,6,7,8,9
```

### Performance Tips

1. **Execution Policy:**  
   Set the `executionPolicy` to `'par'` or `'par_unseq'` to enable parallel execution. This leverages multiple CPU cores for enhanced performance.

2. **Threshold Adjustment:**  
   Tweak the `threshold` parameter to optimize task granularity. A well-tuned threshold ensures that the overhead of parallelization is justified by the size of the data being processed.

3. **Thread Count Configuration:**  
   Adjust the `threadCount` based on your machine's CPU cores. More threads can lead to better utilization of system resources, especially on multi-core systems.

4. **Batch Size Considerations:**  
   For extremely large datasets, ensure that your system has sufficient memory to handle multiple copies of large arrays during processing.

5. **Logging Configuration:**  
   Disable or minimize logging (`loggingEnabled: false` or higher `logLevel`) during benchmarking or production runs to avoid unnecessary performance overhead.

---

## Benchmarking Example

To assess the performance benefits provided by the HPX Node.js addon, you can run benchmarks comparing HPX-based operations against their native JavaScript counterparts. This example demonstrates how to set up and execute such benchmarks.

### Benchmark Script: `benchmark.mjs`

Create a file named `benchmark.mjs` with the following content:

```js
import { createRequire } from 'module';
const require = createRequire(import.meta.url);
const hpxaddon = require('./addons/hpxaddon.node');

const { performance } = require('perf_hooks');

/**
 * Helper Functions
 */

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

// Initialize HPX
await hpxaddon.initHPX({
  executionPolicy: 'par',       // 'par' for parallel, 'par_unseq' for parallel unsequenced
  threshold: 10000,              // Threshold for task granularity
  threadCount: 4,                // Number of HPX threads to spawn
  loggingEnabled: false,         // Disable logging for benchmarking
  logLevel: 'error'              // Set logging level to capture only errors
});

const largeSize = 1000000; // 1 million elements
const largeArray = new Int32Array(largeSize);

// Populate the array with random integers
for (let i = 0; i < largeSize; i++) {
  largeArray[i] = Math.floor(Math.random() * largeSize);
}

// Define a predicate: elements greater than 500,000
const isGreaterThanHalfMillion = (val) => val > 500000;

// Convert the per-element predicate to a batch mask generator
const isGreaterThanHalfMillionMask = elementPredicateToMask(isGreaterThanHalfMillion);

// Define a comparator: ascending order
const ascendingComparator = (a, b) => a < b;

// Convert the per-element comparator to a batch key extractor
const ascendingKeys = elementComparatorToKeys(ascendingComparator);

// Benchmark Sort
console.log('Benchmarking Sort...');
const startSort = performance.now();
const sorted = await hpxaddon.sort(largeArray);
const endSort = performance.now();
console.log(`HPX Sort Time: ${(endSort - startSort).toFixed(2)}ms`);

// Benchmark Sort in JavaScript
console.log('Benchmarking JavaScript Sort...');
const jsArray = Array.from(largeArray);
const startJSSort = performance.now();
jsArray.sort((a, b) => a - b);
const endJSSort = performance.now();
console.log(`JavaScript Sort Time: ${(endJSSort - startJSSort).toFixed(2)}ms`);

// Benchmark countIf
console.log('Benchmarking countIf...');
const startCountIf = performance.now();
const countHPX = await hpxaddon.countIf(largeArray, isGreaterThanHalfMillionMask);
const endCountIf = performance.now();
console.log(`HPX countIf Time: ${(endCountIf - startCountIf).toFixed(2)}ms, Count: ${countHPX}`);

// Benchmark countIf in JavaScript
console.log('Benchmarking JavaScript countIf...');
const startJSCountIf = performance.now();
let countJS = 0;
for (let i = 0; i < largeArray.length; i++) {
  if (largeArray[i] > 500000) countJS++;
}
const endJSCountIf = performance.now();
console.log(`JavaScript countIf Time: ${(endJSCountIf - startJSCountIf).toFixed(2)}ms, Count: ${countJS}`);

// Benchmark copyIf
console.log('Benchmarking copyIf...');
const startCopyIf = performance.now();
const copiedHPX = await hpxaddon.copyIf(largeArray, isGreaterThanHalfMillionMask);
const endCopyIf = performance.now();
console.log(`HPX copyIf Time: ${(endCopyIf - startCopyIf).toFixed(2)}ms, Copied Length: ${copiedHPX.length}`);

// Benchmark copyIf in JavaScript
console.log('Benchmarking JavaScript copyIf...');
const startJSCopyIf = performance.now();
const copiedJS = [];
for (let i = 0; i < largeArray.length; i++) {
  if (largeArray[i] > 500000) copiedJS.push(largeArray[i]);
}
const endJSCopyIf = performance.now();
console.log(`JavaScript copyIf Time: ${(endJSCopyIf - startJSCopyIf).toFixed(2)}ms, Copied Length: ${copiedJS.length}`);

// Finalize HPX
await hpxaddon.finalizeHPX();
```

### Running the Benchmark

Execute the benchmark script using Node.js:

```bash
node benchmark.mjs
```

### Expected Output

```
Benchmarking Sort...
HPX Sort Time: 1500.12ms
Benchmarking JavaScript Sort...
JavaScript Sort Time: 3200.45ms
Benchmarking countIf...
HPX countIf Time: 500.67ms, Count: 500000
Benchmarking JavaScript countIf...
JavaScript countIf Time: 1200.89ms, Count: 500000
Benchmarking copyIf...
HPX copyIf Time: 800.34ms, Copied Length: 500000
Benchmarking JavaScript copyIf Time: 1600.78ms, Copied Length: 500000
```

**Interpreting the Results:**

- **HPX vs. JavaScript Performance:**  
  The addon demonstrates significantly faster execution times for operations like `sort`, `countIf`, and `copyIf` compared to their native JavaScript counterparts, especially as the dataset size increases.

- **Speedup Factor:**  
  Calculate the speedup by dividing the JavaScript time by the HPX time for each operation. For instance, if HPX sort takes 1500ms and JavaScript sort takes 3000ms, the speedup factor is 2x.

- **Consistency:**  
  Ensure that the results (e.g., sorted arrays, counts, copied arrays) are consistent across both HPX and JavaScript implementations to validate the correctness of the addon.

**Note:** Actual times will vary based on system specifications, current system load, and the efficiency of the underlying HPX algorithms.

---

## Advanced Example: Custom Sorting with Key Extraction

In addition to standard sorting, you can perform custom sorting operations by extracting keys using a JavaScript function. This allows for more complex sorting criteria based on external logic.

### Example: Sorting Based on Absolute Values

Sort an `Int32Array` based on the absolute values of its elements.

```js
const input = Int32Array.from([-10, 5, -8, 2, -9]);

// Define a per-element comparator for absolute values
const absoluteComparator = (a, b) => Math.abs(a) < Math.abs(b);

// Convert the per-element comparator to a batch key extractor
const absoluteKeys = elementComparatorToKeys(absoluteComparator);

// Using sortComp with the batch key extractor
const sortedByAbs = await hpxaddon.sortComp(input, absoluteKeys);
console.log(`Sorted by absolute values: ${Array.from(sortedByAbs)}`);
// Output: Sorted by absolute values: 2,5,-8,-9,-10
```

**Explanation:**

1. **Comparator Function:**  
   The `absoluteComparator` compares two elements based on their absolute values.

2. **Key Extraction:**  
   The `elementComparatorToKeys` helper function converts the per-element comparator into a batch key extractor. Since `absoluteComparator` defines an ascending order based on absolute values, the keys are the absolute values themselves.

3. **Custom Sorting:**  
   By passing the `absoluteKeys` as the comparator to `sortComp`, the addon sorts the original array based on the absolute values, resulting in a sorted array that prioritizes the magnitude of numbers irrespective of their sign.

---

## Error Handling Example

Proper error handling ensures that your application can gracefully handle and recover from unexpected scenarios. Here's how you can manage errors when using the addon.

### Example: Handling Invalid Input

Attempting to pass an invalid argument type to an addon function should result in a handled error.

```js
try {
  // Intentionally pass a non-Int32Array to the sort function
  const invalidInput = [5, 3, 8, 1, 9]; // Regular JavaScript array
  const sorted = await hpxaddon.sort(invalidInput);
} catch (error) {
  console.error(`Error occurred: ${error.message}`);
  // Output: Error occurred: Expected an Int32Array at argument 0
}
```

**Explanation:**

- **Type Validation:**  
  The addon functions validate the types of their arguments. In this case, `sort` expects an `Int32Array`. Passing a regular JavaScript array triggers a type error.

- **Promise Rejection:**  
  Since the addon functions return Promises, type errors result in the Promise being rejected. Using `try-catch` blocks allows you to handle these rejections gracefully.

- **User Feedback:**  
  Providing clear and descriptive error messages helps in debugging and ensures that users can correct their input accordingly.

### Example: Handling Predicate Errors

If a predicate function does not return the expected mask or contains errors, the addon will handle it appropriately.

```js
try {
  const arr = Int32Array.from([1, 2, 3, 4, 5]);

  // Define an invalid predicate: returns a non-Uint8Array
  const invalidPredicate = (arr) => {
    return [1, 0, 1, 0, 1]; // Returns a regular array instead of Uint8Array
  };

  // Convert the per-element predicate to a batch mask generator
  const invalidPredicateMask = elementPredicateToMask(invalidPredicate);

  // Using countIf with the invalid predicate mask generator
  const countRes = await hpxaddon.countIf(arr, invalidPredicateMask);
} catch (error) {
  console.error(`Predicate Error: ${error.message}`);
  // Output: Predicate Error: Predicate must return a typed array (Uint8Array).
}
```

**Explanation:**

- **Expected Return Type:**  
  For functions like `countIf` and `copyIf`, the predicate function should return a `Uint8Array` where each element indicates whether the corresponding input element satisfies the predicate.

- **Error Handling:**  
  If the predicate returns an incorrect type or a `Uint8Array` of mismatched length, the addon rejects the Promise with an appropriate error message.

- **Developer Guidance:**  
  Clear error messages inform developers about the required return types and help them adjust their predicate functions accordingly.

