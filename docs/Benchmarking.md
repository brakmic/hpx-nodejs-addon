# Benchmarking

## Table of Contents
- [Benchmarking](#benchmarking)
  - [Table of Contents](#table-of-contents)
  - [Introduction](#introduction)
  - [Prerequisites](#prerequisites)
  - [Running the Benchmark](#running-the-benchmark)
  - [Interpreting Results](#interpreting-results)
  - [Adjusting Benchmark Configuration](#adjusting-benchmark-configuration)
  - [Example Real-World Output](#example-real-world-output)
  - [Troubleshooting](#troubleshooting)

---

## Introduction

The addon provides a `benchmark.mjs` script that compares HPX-based implementations of algorithms with their native JavaScript counterparts. This helps you understand when HPX can give a speed advantage and how different configurations affect performance.

The benchmark uses [Benchmark.js](https://github.com/bestiejs/benchmark.js) and prints a table showing operations per second (ops/sec) for both HPX and JS implementations, along with a calculated speedup factor.

---

## Prerequisites

1. **Addon Built:**  
   Ensure the addon is compiled and `hpxaddon.node` is available. See [docs/Building.md](./Building.md) for instructions.

2. **HPX Installed & Environment Set:**  
   Confirm HPX is installed and `LD_LIBRARY_PATH` is set, or run inside the provided Docker environment. See [docs/Docker.md](./Docker.md) for details.

3. **Node.js:**  
   The script uses ESM modules and recommends Node.js v20+.

---

## Running the Benchmark

From your project directory:

```bash
node benchmark.mjs
```

**What happens:**
- The script initializes HPX with a given configuration.
- It creates large arrays and measures performance of various operations under HPX and native JS.
- After completion, it prints a results table.

---

## Interpreting Results

- **ops/sec (Operations per second):**  
  Higher is better. It shows how many iterations per second the benchmark can run.

- **Winner & Speedup:**  
  The "Winner" column indicates which side (HPX or JS) was faster on average.  
  The "Speedup" column indicates how many times faster the winner is. For example, "2.5x" means HPX ran 2.5 times as many ops/sec as JS.

A tie scenario would show "Winner: Tie" and "N/A" for speedup.

---

## Adjusting Benchmark Configuration

Inside `benchmark.mjs`, you’ll find configuration options that mirror those in the addon’s initialization:

- **executionPolicy:** Choose `"seq"`, `"par"`, or `"par_unseq"`.
- **threshold:** Set a size at which operations switch from sequential to parallel mode.
- **threadCount:** Adjust how many HPX threads are used.
- **loggingEnabled & logLevel:** Enable logging to see more details about what the benchmark does internally.
- **Data Size & Tests:**  
  You can modify the `arraySize`, `suffixSize`, or the tests that run by enabling/disabling certain operations in the `benchmarkTests` array.  
  Each operation listed in `benchmark.mjs` can be toggled by setting `active: true` or `active: false`.

**Example (modifying benchmark config):**

```js
// Inside benchmark.mjs
const config = {
  executionPolicy: 'par_unseq',
  threshold: 50000,
  threadCount: 8,
  loggingEnabled: true,
  logLevel: 'debug',
  addonName: 'hpxaddon',
};

// Modify array sizes or batchSize if needed:
const arraySize = 200000; // Increase data size to stress parallel execution
```

By experimenting with these settings, you can see how performance changes. After adjusting, just rerun `node benchmark.mjs`.

---

## Example Real-World Output

Below is sample output from a real test run of the benchmark:

```
HPX Sort x 5.20 ops/sec ±24.20% (28 runs sampled)
JavaScript Sort x 0.75 ops/sec ±66.61% (7 runs sampled)
HPX Count x 20.41 ops/sec ±43.73% (31 runs sampled)
JavaScript Count x 33.39 ops/sec ±139.31% (63 runs sampled)
HPX Copy x 7.36 ops/sec ±55.31% (17 runs sampled)
JavaScript Copy x 26.41 ops/sec ±51.46% (17 runs sampled)
HPX EndsWith x 17.96 ops/sec ±102.52% (36 runs sampled)
JavaScript EndsWith x 128 ops/sec ±42.58% (54 runs sampled)
HPX Equal x 28.67 ops/sec ±40.28% (41 runs sampled)
JavaScript Equal x 58.31 ops/sec ±37.45% (35 runs sampled)
HPX Find x 39.01 ops/sec ±50.38% (36 runs sampled)
JavaScript Find x 46.24 ops/sec ±68.48% (29 runs sampled)
HPX Merge x 1.29 ops/sec ±88.48% (13 runs sampled)
JavaScript Merge x 4.46 ops/sec ±24.98% (24 runs sampled)
HPX PartialSort x 2.18 ops/sec ±28.02% (16 runs sampled)
JavaScript PartialSort x 0.35 ops/sec ±28.63% (6 runs sampled)
HPX CopyN x 34.56 ops/sec ±24.87% (30 runs sampled)
JavaScript CopyN x 342 ops/sec ±98.17% (38 runs sampled)
HPX Fill x 23.20 ops/sec ±34.59% (39 runs sampled)
JavaScript Fill x 96.26 ops/sec ±31.56% (35 runs sampled)
HPX CountIf x 40.07 ops/sec ±29.77% (39 runs sampled)
JavaScript CountIf x 3.51 ops/sec ±43.99% (24 runs sampled)
HPX CopyIf x 33.27 ops/sec ±9.86% (54 runs sampled)
JavaScript CopyIf x 2.65 ops/sec ±21.31% (18 runs sampled)
HPX SortComp x 1.62 ops/sec ±23.96% (12 runs sampled)
JavaScript SortComp x 0.10 ops/sec ±18.81% (5 runs sampled)
HPX PartialSortComp x 1.93 ops/sec ±72.80% (14 runs sampled)
JavaScript PartialSortComp x 0.12 ops/sec ±80.29% (6 runs sampled)

Benchmark completed.

Detailed Results:
┌─────────────────┬─────────────┬────────────┬────────┬─────────┐
│ Operation       │ HPX ops/sec │ JS ops/sec │ Winner │ Speedup │
├─────────────────┼─────────────┼────────────┼────────┼─────────┤
│ Sort            │ 5.20        │ 0.75       │ HPX    │ 6.96x   │
├─────────────────┼─────────────┼────────────┼────────┼─────────┤
│ Count           │ 20.41       │ 33.39      │ JS     │ 1.64x   │
├─────────────────┼─────────────┼────────────┼────────┼─────────┤
│ Copy            │ 7.36        │ 26.41      │ JS     │ 3.59x   │
├─────────────────┼─────────────┼────────────┼────────┼─────────┤
│ EndsWith        │ 17.96       │ 127.76     │ JS     │ 7.11x   │
├─────────────────┼─────────────┼────────────┼────────┼─────────┤
│ Equal           │ 28.67       │ 58.31      │ JS     │ 2.03x   │
├─────────────────┼─────────────┼────────────┼────────┼─────────┤
│ Find            │ 39.01       │ 46.24      │ JS     │ 1.19x   │
├─────────────────┼─────────────┼────────────┼────────┼─────────┤
│ Merge           │ 1.29        │ 4.46       │ JS     │ 3.45x   │
├─────────────────┼─────────────┼────────────┼────────┼─────────┤
│ PartialSort     │ 2.18        │ 0.35       │ HPX    │ 6.22x   │
├─────────────────┼─────────────┼────────────┼────────┼─────────┤
│ CopyN           │ 34.56       │ 341.95     │ JS     │ 9.89x   │
├─────────────────┼─────────────┼────────────┼────────┼─────────┤
│ Fill            │ 23.20       │ 96.26      │ JS     │ 4.15x   │
├─────────────────┼─────────────┼────────────┼────────┼─────────┤
│ CountIf         │ 40.07       │ 3.51       │ HPX    │ 11.42x  │
├─────────────────┼─────────────┼────────────┼────────┼─────────┤
│ CopyIf          │ 33.27       │ 2.65       │ HPX    │ 12.57x  │
├─────────────────┼─────────────┼────────────┼────────┼─────────┤
│ SortComp        │ 1.62        │ 0.10       │ HPX    │ 16.74x  │
├─────────────────┼─────────────┼────────────┼────────┼─────────┤
│ PartialSortComp │ 1.93        │ 0.12       │ HPX    │ 16.10x  │
└─────────────────┴─────────────┴────────────┴────────┴─────────┘
```

In this example:
- HPX dominates some operations (e.g., `Sort`, `PartialSortComp`) showing large speedups.
- JS wins in others (e.g., `Count`, `Copy`, `Fill`) due to lower overhead for that specific scenario.
- The variance (`±%`) suggests you may want to run more samples or adjust environment conditions for stable results.

---

## Troubleshooting

- **No Speedup or Lower HPX Performance:**  
  It might be that your dataset is too small or the `threshold` is not tuned. Increase data size or adjust `threshold`.

- **HPX Initialization Errors:**  
  Ensure environment variables and paths for HPX libraries are set, or use the provided Docker environment. See [docs/Docker.md](./Docker.md).

- **Enable Debug Logs:**  
  Set `loggingEnabled: true` and `logLevel: 'debug'` in `benchmark.mjs` to see what’s happening behind the scenes.
