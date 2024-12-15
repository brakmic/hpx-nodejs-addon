# Configuration

## Table of Contents
1. [Introduction](#introduction)
2. [Supported Configuration Options](#supported-configuration-options)
3. [Examples](#examples)
4. [Logging Details](#logging-details)
5. [Best Practices](#best-practices)

---

## Introduction

The addon’s behavior can be customized by passing a configuration object to `initHPX`. These settings influence how HPX is initialized and how parallel algorithms are executed internally.

**Example:**
```js
await hpxaddon.initHPX({
  executionPolicy: 'par',   // "seq", "par", "par_unseq"
  threshold: 10000,
  threadCount: 4,
  loggingEnabled: true,
  logLevel: 'debug',
  addonName: 'hpxaddon'
});
```

Once HPX is initialized with these options, subsequent calls (like `sort` or `countIf`) use these settings.

---

## Supported Configuration Options

### executionPolicy
- **Type:** string
- **Default:** `"par"`
- **Allowed Values:** `"seq"`, `"par"`, `"par_unseq"`

This determines the execution policy for parallel algorithms:
- `"seq"`: Always run sequentially.
- `"par"`: Use parallel execution when data size >= threshold.
- `"par_unseq"`: Parallel and unsequenced execution (for highly optimized vectorization).

### threshold
- **Type:** number
- **Default:** `10000`

**Meaning:**  
For input sizes smaller than `threshold`, the addon falls back to sequential execution even if `executionPolicy` is `"par"` or `"par_unseq"`. This avoids parallel overhead for small tasks.

**In short:** arrays smaller than `threshold` run sequentially.

### threadCount
- **Type:** number
- **Default:** `4`

Sets the number of HPX threads. Increasing `threadCount` may improve performance on multicore systems. Too high a value can cause oversubscription.

### loggingEnabled
- **Type:** boolean
- **Default:** `true`

Enables or disables logging.

### logLevel
- **Type:** string
- **Default:** `"INFO"`
- **Allowed Values:** `"DEBUG"`, `"INFO"`, `"WARN"`, `"ERROR"`

Controls verbosity of log output. `"DEBUG"` is most verbose, `"ERROR"` is least.

### addonName
- **Type:** string
- **Default:** `"hpxaddon"`

Specifies the name of the addon. Usually no need to change this, but it’s available if you want to differentiate multiple addons or for logging.

---

## Examples

### Minimal Configuration
```js
await hpxaddon.initHPX({ executionPolicy: 'seq' });
// Uses default threshold, threadCount=4, logging enabled at INFO level.
```

### Fully Custom Configuration
```js
await hpxaddon.initHPX({
  executionPolicy: 'par_unseq',
  threshold: 50000,
  threadCount: 8,
  loggingEnabled: true,
  logLevel: 'debug',
  addonName: 'myCustomAddon'
});
```

In this example:
- Large arrays (>= 50,000 elements) run in parallel, potentially using SIMD optimizations.
- 8 HPX threads handle tasks.
- Detailed debug logs are printed.

---

## Logging Details

When `loggingEnabled` is `true`:
- Logs print initialization and finalization steps.
- Internal async work states (init, execute, complete) are logged.
- TSFN usage and batch processing steps are logged at `DEBUG` level.

Use `logLevel: 'debug'` during development to understand what’s happening under the hood. Switch to `logLevel: 'info'` or higher in production to reduce noise.

---

## Best Practices

1. **Set a Reasonable Threshold:**  
   If `threshold` is too low, even small arrays attempt parallel work, causing overhead. If too high, some arrays that could benefit from parallelism remain sequential. Experiment with values based on your data sizes.

2. **Tune threadCount:**  
   Match `threadCount` to the number of CPU cores or a bit less to avoid oversubscription. More threads doesn’t always mean faster performance.

3. **Logging for Development:**  
   Keep `loggingEnabled: true` and `logLevel: 'debug'` while testing. For production, disable or use a higher log level.

4. **Execution Policy Selection:**  
   - Use `"par"` for general parallelization.
   - `"par_unseq"` if you trust HPX to vectorize well and you have a CPU that benefits from it.
   - `"seq"` for debugging or when you know parallelization won’t help.

---

By understanding these configuration options and adjusting them to your environment, you can achieve optimal performance and maintainability for your application.
