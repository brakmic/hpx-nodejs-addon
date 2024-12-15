# Internals

## Table of Contents
- [Internals](#internals)
  - [Table of Contents](#table-of-contents)
  - [Introduction](#introduction)
  - [Core Components](#core-components)
  - [HPX Manager \& HPX Lifecycle](#hpx-manager--hpx-lifecycle)
  - [Async Helpers \& Promise Handling](#async-helpers--promise-handling)
  - [Data Conversion Layer](#data-conversion-layer)
  - [TSFNManager and Predicate Helpers](#tsfnmanager-and-predicate-helpers)
  - [Logging Infrastructure](#logging-infrastructure)
  - [Conclusion](#conclusion)

---

## Introduction

This document delves into the internal source code structure of the Node.js addon, elucidating key classes, modules, and their interactions. It is primarily intended for contributors or maintainers seeking a comprehensive understanding of how the addon is architected at the code level, ensuring efficient and stable integrations with HPX for parallel computations.

---

## Core Components

1. **`addon.cpp` and `addon.hpp`**:  
   These files define and register all functions exposed to JavaScript (`sort`, `count`, `copyIf`, `sortComp`, etc.). Each function handles argument validation, data extraction from `Int32Array`s, and leverages `QueueAsyncWork` to execute computations asynchronously, ensuring non-blocking operations in the Node.js environment.

2. **`hpx_wrapper.cpp` and `hpx_wrapper.hpp`**:  
   These provide HPX-based implementations of various algorithms (`hpx_sort`, `hpx_count`, `hpx_copy_if`, `hpx_sort_comp`, etc.). Each function:
   - Accepts C++ pointers and sizes as input.
   - Utilizes `hpx::async` and HPX's parallel algorithms to perform computations.
   - Returns futures containing the results of these computations, facilitating asynchronous execution.

3. **`hpx_manager.cpp` and `hpx_manager.hpp`**:  
   Manages the lifecycle of the HPX runtime. It handles the initialization and finalization of HPX, spawns dedicated threads for HPX operations, and manages promises that signal successful startup or shutdown.

4. **`hpx_config.cpp` and `hpx_config.hpp`**:  
   These files are responsible for storing and parsing user configurations (such as `executionPolicy`, `threshold`, etc.). They also initialize the logging system based on user preferences, ensuring that the addon operates according to specified parameters.

---

## HPX Manager & HPX Lifecycle

**HPXManager** is a singleton class that orchestrates the lifecycle of the HPX runtime within the addon:

- **Initialization**:  
  - Initiates HPX using `hpx::start()` in a dedicated thread.
  - Utilizes `InitHPX` to asynchronously start HPX, returning a future that resolves upon successful initialization.
  - Implements `hpx_main_handler` as the entry point when HPX starts, setting `running_ = true`, resolving the initialization promise, and awaiting a finalize signal.

- **Finalization**:  
  - Through `FinalizeHPX`, signals the HPX runtime to gracefully shutdown using `hpx::finalize()`.
  - Ensures that the HPX thread is properly joined after shutdown.
  - Manages thread-safe state transitions to prevent multiple initializations or finalizations.
  - Cleans up all registered ThreadSafeFunctions (TSFNs) upon finalization to ensure no callbacks remain active.

**Key Points:**
- **Singleton Pattern**: Ensures only one instance of HPXManager exists, preventing multiple initializations.
- **Thread Safety**: Utilizes mutexes and atomic variables to manage state changes safely across threads.
- **Resource Management**: Guarantees that resources are appropriately released during finalization, maintaining addon stability.

---

## Async Helpers & Promise Handling

**`async_helpers.hpp`** defines templated `QueueAsyncWork` functions that streamline the process of executing asynchronous tasks and managing their results:

- **Functionality**:
  - Wraps the creation and scheduling of `napi_async_work`.
  - Separates the execution phase (running HPX algorithms) from the completion phase (fulfilling or rejecting JavaScript Promises).

- **Workflow**:
  1. **JavaScript Invocation**:  
     - A JavaScript function (e.g., `sort`, `copyIf`) is called, which internally invokes `QueueAsyncWork`.
  
  2. **Asynchronous Execution**:  
     - The `execute` lambda runs on a worker thread, performing HPX-based computations.
     - Utilizes HPX's parallel capabilities to handle large datasets efficiently.
  
  3. **Promise Resolution**:  
     - Upon completion, the `complete` lambda is invoked on the main thread.
     - Resolves or rejects the corresponding JavaScript `Promise` based on the outcome of the asynchronous task.

- **Benefits**:
  - **Decoupling**: Separates the computational logic from JavaScript's event loop, ensuring non-blocking operations.
  - **Reusability**: Provides a consistent pattern for all asynchronous functions, reducing code duplication.
  - **Error Handling**: Centralizes error management, ensuring that exceptions in worker threads are appropriately propagated to JavaScript.

---

## Data Conversion Layer

**`data_conversion.cpp` and `data_conversion.hpp`** provide utilities for handling data transformations and validations between C++ and JavaScript:

- **Functionality**:
  - **Argument Extraction**:  
    - Functions like `GetInt32ArrayArgument` validate and extract `Int32Array` data from JavaScript function arguments.
    - Ensures type safety and provides meaningful error messages if validations fail.
  
  - **Batch Processing Helpers**:  
    - **`GetPredicateMaskBatchUsingTSFN`**:  
      - Executes a JavaScript predicate function in batch mode, passing the entire `Int32Array` at once.
      - Receives a `Uint8Array` mask indicating which elements satisfy the predicate (`1` for `true`, `0` for `false`).
  
    - **`GetKeyArrayBatchUsingTSFN`**:  
      - Similar to the predicate mask function but tailored for key extraction in sorting operations.
      - Receives an `Int32Array` of keys corresponding to each element, facilitating efficient sorting in C++.

- **Memory Management**:
  - **Shared Pointers**:  
    - Utilizes `std::shared_ptr` to manage the lifetime of data structures across asynchronous operations.
    - Prevents memory leaks and ensures data remains valid until all operations are complete.
  
  - **Data Copying**:  
    - Copies input arrays into `std::vector<int32_t>` or `std::vector<uint8_t>` to maintain data integrity during asynchronous processing.
  
  - **Output Allocation**:  
    - Results are returned as newly allocated `Int32Array` objects, ensuring that JavaScript receives independent copies of the data.

**Key Points**:
- **Efficiency**: Batch processing minimizes the overhead of multiple cross-boundary calls between C++ and JavaScript.
- **Safety**: Ensures that data passed between threads remains consistent and free from race conditions.
- **Flexibility**: Provides utilities that cater to various operations like filtering (`copyIf`, `countIf`) and sorting (`sortComp`, `partialSortComp`).

---

## TSFNManager and Predicate Helpers

**`TSFNManager`**:
- **Purpose**:  
  - Manages all active ThreadSafeFunctions (TSFNs) within the addon.
  - Ensures that TSFNs are properly aborted and released during the finalization of the HPX runtime.
  
- **Functionality**:
  - **Registration**:  
    - Tracks all created TSFNs, maintaining a registry that can be iterated over for cleanup.
  
  - **Finalization**:  
    - On addon shutdown, it gracefully aborts all registered TSFNs.
    - Ensures that no callbacks remain active, preventing potential memory leaks or undefined behaviors.

**`predicate_helpers.cpp` and `predicate_helpers.hpp`**:
- **Purpose**:  
  - Facilitate the execution of JavaScript predicate and comparator functions from C++ in a thread-safe manner.
  
- **Key Functions**:
  - **`MakeIntPredicate` and `MakeIntComparator`**:  
    - Wrap JavaScript functions to be used as predicates or comparators in HPX algorithms.
    - Convert per-element JavaScript functions into batch operations to reduce overhead.
  
  - **Batch Execution**:  
    - Instead of invoking the JavaScript function for each element individually, the addon calls the function once with the entire dataset.
    - Receives a mask or key array that is then used in C++ for parallel processing.

- **Implementation Details**:
  - **ThreadSafeFunction (TSFN)**:  
    - Utilized to safely call JavaScript functions from worker threads.
    - Ensures that interactions with the JavaScript engine are thread-safe and do not violate V8's single-threaded constraints.
  
  - **Mask and Key Structures**:  
    - **`MaskData` and `MaskPredicate`**:  
      - Used in filtering operations (`copyIf`, `countIf`) to determine which elements satisfy the predicate.
      - `MaskData` holds the mask array and an atomic index for thread-safe access.
      - `MaskPredicate` accesses `MaskData` to evaluate each element based on the mask.
  
    - **`KeyData` and `KeyComparator`**:  
      - Employed in sorting operations (`sortComp`, `partialSortComp`) to determine the order of elements.
      - `KeyData` holds the key array and an atomic index.
      - `KeyComparator` uses `KeyData` to compare elements based on their corresponding keys.

**Key Idea**:  
Batch calls significantly reduce the overhead associated with multiple cross-boundary invocations between C++ and JavaScript. By processing predicates and comparators in bulk, the addon achieves higher performance while maintaining thread safety through TSFNs.

---

## Logging Infrastructure

**`log_macros.hpp` and `logger.cpp`** establish a robust logging system within the addon:

- **Functionality**:
  - **Logging Levels**:  
    - Supports various logging levels (`DEBUG`, `INFO`, `WARN`, `ERROR`) to categorize log messages based on their severity.
  
  - **Configuration**:  
    - Logging behavior is configured based on user settings (`loggingEnabled`, `logLevel`) specified in `hpx_config`.
    - Determines which messages are emitted and their verbosity.
  
  - **Macros**:  
    - Macros like `LOG_DEBUG`, `LOG_INFO`, `LOG_WARN`, `LOG_ERROR` simplify logging throughout the addon code.
    - Enable conditional logging based on the configured log level, ensuring that unnecessary log messages are not generated in production environments.

- **Usage**:
  - **Runtime Diagnostics**:  
    - Logs critical steps in HPX initialization and finalization, aiding in debugging lifecycle issues.
  
  - **TSFN Operations**:  
    - Records the creation, execution, and abortion of TSFNs, providing insights into predicate and comparator executions.
  
  - **Asynchronous Workflows**:  
    - Tracks the scheduling and completion of asynchronous tasks, helping identify performance bottlenecks or unexpected behaviors.
  
  - **Error Reporting**:  
    - Captures and logs exceptions and errors that occur during asynchronous operations, facilitating quicker issue resolution.

- **Best Practices**:
  - **Performance Considerations**:  
    - Logging is designed to have minimal impact on performance, especially at higher log levels like `ERROR`.
  
  - **Security**:  
    - Sensitive information is either omitted or masked in log messages to prevent potential security vulnerabilities.

**When to Check Logs**:
- **HPX Lifecycle Events**:  
  - Monitoring initialization and finalization steps to ensure HPX operates as expected.
  
- **Predicate and Comparator Executions**:  
  - Verifying that batch calls to JavaScript functions are executed correctly and efficiently.
  
- **Asynchronous Task States**:  
  - Ensuring that asynchronous operations are scheduled, executed, and completed without issues.

---

## Conclusion

The internal architecture of the Node.js addon is meticulously designed to integrate HPX's powerful parallel computing capabilities with JavaScript's flexible and asynchronous environment. By leveraging advanced C++ features, ThreadSafeFunctions, and a robust logging system, the addon ensures high performance, thread safety, and ease of use for developers.

**Key Takeaways**:

1. **Batch Operations for Enhanced Performance**:
   - **Predicates and Comparators**:  
     Batch processing minimizes the overhead of multiple cross-boundary calls between C++ and JavaScript, significantly boosting performance.
  
   - **Helper Functions**:  
     Utilities like `GetPredicateMaskBatchUsingTSFN` and `GetKeyArrayBatchUsingTSFN` abstract the complexity of batch operations, allowing developers to write intuitive per-element functions while maintaining high efficiency.

2. **Thread Safety and Efficient Resource Management**:
   - **Atomic Operations**:  
     Structures like `MaskData`, `KeyData`, and atomic counters ensure that concurrent operations do not lead to race conditions or data inconsistencies.
  
   - **Memory Management**:  
     The use of `std::shared_ptr` and careful data copying strategies prevent memory leaks and ensure data validity across asynchronous tasks.

3. **Seamless Asynchronous Integration**:
   - **Promise-Based API**:  
     All exposed functions return Promises, providing a familiar and consistent interface for JavaScript developers.
  
   - **Non-Blocking Operations**:  
     By executing heavy computations on worker threads and using `QueueAsyncWork`, the addon ensures that the Node.js event loop remains responsive, delivering a smooth user experience.

4. **Robust Logging and Diagnostics**:
   - **Comprehensive Logging**:  
     The logging infrastructure captures critical events and errors, aiding in debugging and performance tuning.
  
   - **Configurable Verbosity**:  
     Developers can adjust logging levels based on their needs, balancing between informational insights and performance considerations.

By adhering to these principles and leveraging the strengths of both C++ and JavaScript, the addon provides a powerful toolset for high-performance data processing tasks within the Node.js ecosystem.

For a higher-level overview of the addon's architecture, refer to the [Architecture](./Architecture.md) document. For practical usage examples and demonstrations, consult the [Examples](./Examples.md) section.