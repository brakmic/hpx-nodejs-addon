# Predicate-Based Algorithms

## Table of Contents
- [Predicate-Based Algorithms](#predicate-based-algorithms)
  - [Table of Contents](#table-of-contents)
  - [Introduction](#introduction)
  - [Function Overview](#function-overview)
  - [Detailed Breakdown of `CopyIf`](#detailed-breakdown-of-copyif)
    - [Step-by-Step Explanation](#step-by-step-explanation)
  - [Understanding `GetPredicateMaskBatchUsingTSFN`](#understanding-getpredicatemaskbatchusingtsfn)
    - [Step-by-Step Explanation](#step-by-step-explanation-1)
  - [Interplay Between `MaskData` and `MaskPredicate`](#interplay-between-maskdata-and-maskpredicate)
    - [Purpose](#purpose)
    - [Detailed Structure and Functionality](#detailed-structure-and-functionality)
      - [`MaskData` Struct](#maskdata-struct)
      - [`MaskPredicate` Struct](#maskpredicate-struct)
    - [Why Use `MaskData` and `MaskPredicate`?](#why-use-maskdata-and-maskpredicate)
  - [Flowchart](#flowchart)
  - [Usage Example](#usage-example)
    - [Using Helper Functions](#using-helper-functions)
      - [Helper Function: `elementPredicateToMask`](#helper-function-elementpredicatetomask)
  - [Supporting `SortComp` with Comparator Helpers](#supporting-sortcomp-with-comparator-helpers)
    - [Helper Function: `elementComparatorToKeys`](#helper-function-elementcomparatortokeys)
    - [Detailed Breakdown of `SortComp`](#detailed-breakdown-of-sortcomp)
    - [Step-by-Step Explanation](#step-by-step-explanation-2)
  - [Understanding `GetKeyArrayBatchUsingTSFN`](#understanding-getkeyarraybatchusingtsfn)
    - [Step-by-Step Explanation](#step-by-step-explanation-3)
  - [Interplay Between `KeyData` and `KeyComparator`](#interplay-between-keydata-and-keycomparator)
    - [Purpose](#purpose-1)
    - [Detailed Structure and Functionality](#detailed-structure-and-functionality-1)
      - [`KeyData` Struct](#keydata-struct)
      - [`KeyComparator` Struct](#keycomparator-struct)
    - [Why Use `KeyData` and `KeyComparator`?](#why-use-keydata-and-keycomparator)
  - [Flowchart for `SortComp`](#flowchart-for-sortcomp)
  - [Usage Example](#usage-example-1)

---

## Introduction

When developing high-performance Node.js addons using C++, leveraging parallel computing libraries like HPX can significantly enhance performance for large-scale data operations. This document explores the implementation of predicate-based parallel algorithms within such addons, using the `CopyIf` function as a primary example. By understanding how predicates and masking mechanisms collaborate, developers can create efficient and stable functions that harness the full potential of parallelism without compromising the seamless integration with the Node.js environment.

## Function Overview

The `CopyIf` function filters elements from an input `Int32Array` based on a user-provided JavaScript predicate function. It returns a new `Int32Array` containing only the elements that satisfy the predicate. This operation is executed asynchronously to prevent blocking the main Node.js event loop, ensuring efficient performance even with large datasets.

**Function Signature:**

```cpp
Napi::Value CopyIf(const Napi::CallbackInfo& info);
```

**Inputs:**
- **`inputArr`**: A `Int32Array` containing the data to be filtered.
- **`fn`**: A JavaScript function that takes an element and returns `true` (1) if the element should be included or `false` (0) otherwise.

**Output:**
- A new `Int32Array` containing only the elements for which `fn(element)` returned `true`.

## Detailed Breakdown of `CopyIf`

Let's dissect the `CopyIf` function step by step to understand its operation.

```cpp
Napi::Value CopyIf(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    // Step 1: Extract Arguments
    auto inputArr = GetInt32ArrayArgument(info, 0);
    Napi::Function fn = info[1].As<Napi::Function>();
    const int32_t* dataPtr = inputArr.Data();
    size_t dataSize = inputArr.ElementLength();

    // Step 2: Create ThreadSafeFunction (TSFN) for the JS Predicate
    auto tsfn = Napi::ThreadSafeFunction::New(env, fn, "BatchPredicate", 0, 1);
    auto tsfnPtr = std::make_shared<Napi::ThreadSafeFunction>(std::move(tsfn));

    // Step 3: Queue Asynchronous Work
    return QueueAsyncWork<std::shared_ptr<std::vector<int32_t>>>(
        env,
        [dataPtr, dataSize, tsfnPtr](std::shared_ptr<std::vector<int32_t>>& res, std::string &err) {
            try {
                // Step 3a: Obtain Mask from JS Predicate
                auto mask = GetPredicateMaskBatchUsingTSFN(*tsfnPtr, dataPtr, dataSize); // std::shared_ptr<std::vector<uint8_t>>

                // Step 3b: Setup MaskData
                struct MaskData {
                    const uint8_t* maskData;
                    std::atomic<size_t> currentIndex{0};
                };

                auto maskData = std::make_shared<MaskData>();
                auto maskVec = *mask; // Copy the mask vector
                auto maskHolder = std::make_shared<std::vector<uint8_t>>(std::move(maskVec));
                maskData->maskData = maskHolder->data();

                // Step 3c: Define MaskPredicate
                struct MaskPredicate {
                    std::shared_ptr<MaskData> data;
                    std::shared_ptr<std::vector<uint8_t>> maskHolder; 
                    bool operator()(int32_t /*val*/) const {
                        size_t idx = data->currentIndex.fetch_add(1, std::memory_order_relaxed);
                        return (*maskHolder)[idx] == 1;
                    }
                };

                MaskPredicate pred{maskData, maskHolder};

                // Step 3d: Execute hpx_copy_if with MaskPredicate
                auto fut = hpx_copy_if(dataPtr, dataSize, pred);
                res = fut.get();
            } catch(const std::exception &e){
                err = e.what();
            }
        },
        [tsfnPtr](Napi::Env env, Napi::Promise::Deferred& def, std::shared_ptr<std::vector<int32_t>>& res, const std::string &err) {
            // Step 4: Abort TSFN and Resolve/Reject Promise
            tsfnPtr->Abort();
            if(!err.empty()) {
                def.Reject(Napi::String::New(env, err));
            } else {
                Napi::Int32Array arr = Napi::Int32Array::New(env, res->size());
                memcpy(arr.Data(), res->data(), res->size()*sizeof(int32_t));
                def.Resolve(arr);
            }
        }
    );
}
```

### Step-by-Step Explanation

1. **Step 1: Extract Arguments**
   - **`inputArr`**: Retrieves the first argument as an `Int32Array` using a helper function `GetInt32ArrayArgument`.
   - **`fn`**: Retrieves the second argument as a JavaScript function.
   - **`dataPtr` & `dataSize`**: Obtains a pointer to the raw data and its size from the `Int32Array`.

2. **Step 2: Create ThreadSafeFunction (TSFN)**
   - **TSFN Creation**: Initializes a `ThreadSafeFunction` to safely invoke the JavaScript predicate from a worker thread.
   - **`tsfnPtr`**: Wraps the TSFN in a `shared_ptr` to manage its lifetime across asynchronous operations.

3. **Step 3: Queue Asynchronous Work**
   - **`QueueAsyncWork`**: Schedules the provided lambda to run asynchronously, ensuring the main Node.js event loop remains unblocked.

   - **Step 3a: Obtain Mask from JS Predicate**
     - **`GetPredicateMaskBatchUsingTSFN`**: Calls the JavaScript predicate function once with the entire array, expecting a `Uint8Array` mask indicating which elements satisfy the predicate (`1` for `true`, `0` for `false`).

   - **Step 3b: Setup MaskData**
     - **`MaskData` Struct**: Holds a pointer to the mask data and an atomic index to track the current position during predicate evaluation.
     - **`maskHolder`**: Ensures the mask data remains valid and is accessible across threads.

   - **Step 3c: Define MaskPredicate**
     - **`MaskPredicate` Struct**: A functor that serves as the predicate for `hpx_copy_if`.
     - **`operator()`**:
       - Atomically fetches and increments the current index.
       - Checks if the corresponding mask value is `1`.
       - Returns `true` if the element should be copied; otherwise, `false`.

   - **Step 3d: Execute `hpx_copy_if` with `MaskPredicate`**
     - **`hpx_copy_if`**: Utilizes HPX's parallel `copy_if` algorithm to filter and copy elements based on the predicate.
     - **`res`**: Holds the resulting vector after filtering.

4. **Step 4: Abort TSFN and Resolve/Reject Promise**
   - **`tsfnPtr->Abort()`**: Cleans up the TSFN, indicating that no further JS callbacks are needed.
   - **Promise Handling**:
     - If an error occurred, the Promise is rejected with the error message.
     - Otherwise, a new `Int32Array` is created, the filtered data is copied into it, and the Promise is resolved with this array.

## Understanding `GetPredicateMaskBatchUsingTSFN`

The `GetPredicateMaskBatchUsingTSFN` function bridges the gap between C++ and JavaScript by executing the predicate function on the entire array in a single batch operation. Here's a breakdown of its operation.

```cpp
std::shared_ptr<std::vector<uint8_t>> GetPredicateMaskBatchUsingTSFN(const Napi::ThreadSafeFunction& tsfn, const int32_t* data, size_t length) {
    std::atomic<bool> done(false);
    std::string error;
    auto mask = std::make_shared<std::vector<uint8_t>>(length);

    struct CallbackData {
        std::vector<int32_t> dataCopy;
        size_t length;
        std::shared_ptr<std::vector<uint8_t>> mask;
        std::string* error;
        std::atomic<bool>* done;
    };

    // Prepare callback data for JS call
    auto cbData = new CallbackData{
        std::vector<int32_t>(data, data+length),
        length,
        mask,
        &error,
        &done
    };

    // NonBlockingCall invokes the JS function once with the entire array
    napi_status st = tsfn.NonBlockingCall(cbData, [](Napi::Env env, Napi::Function jsFn, void* raw) {
        Napi::HandleScope scope(env);
        std::unique_ptr<CallbackData> args((CallbackData*)raw);

        // We create an Int32Array backed by args->dataCopy
        auto buf = Napi::ArrayBuffer::New(env, (void*)args->dataCopy.data(), args->length * sizeof(int32_t));
        auto inputArr = Napi::Int32Array::New(env, args->length, buf, 0);

        // JS predicate call: must return a Uint8Array of the same length
        Napi::Value ret = jsFn.Call({ inputArr });
        if (!ret.IsTypedArray()) {
            *(args->error) = "Predicate must return a typed array (Uint8Array).";
        } else {
            auto tarr = ret.As<Napi::TypedArray>();
            if (tarr.TypedArrayType() != napi_uint8_array || tarr.ElementLength() != args->length) {
                *(args->error) = "Predicate must return a Uint8Array of same length.";
            } else {
                auto maskArr = tarr.As<Napi::Uint8Array>();
                memcpy(args->mask->data(), maskArr.Data(), args->length);
            }
        }
        args->done->store(true);
    });

    if (st != napi_ok) {
        throw std::runtime_error("Failed NonBlockingCall for predicate.");
    }

    // Wait until JS callback completes
    while (!done.load()) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    if (!error.empty()) throw std::runtime_error(error);
    return mask;
}
```

### Step-by-Step Explanation

1. **Initialization:**
   - **`done`**: An atomic flag to indicate when the JS callback has completed.
   - **`error`**: A string to capture any error messages.
   - **`mask`**: A shared pointer to a `std::vector<uint8_t>` that will store the predicate results.

2. **Preparing Callback Data:**
   - **`CallbackData` Struct**: Contains the data to send to JS and placeholders for the mask and error messages.
     - **`dataCopy`**: A copy of the input data (`Int32Array`) to send to JS.
     - **`length`**: Number of elements.
     - **`mask`**: Pointer to store the resulting mask.
     - **`error`**: Pointer to store error messages.
     - **`done`**: Pointer to the atomic flag.

   - **`cbData`**: Dynamically allocated `CallbackData` instance containing the data to send to JS.

3. **Executing the JS Callback:**
   - **`tsfn.NonBlockingCall`**: Invokes the JavaScript predicate function asynchronously without blocking the main thread.
     - **Arguments:**
       - **`cbData`**: Pointer to `CallbackData`.
       - **Lambda Function**: Handles the execution on the JS thread.

4. **JS Callback Handling (Lambda):**
   - **`Napi::HandleScope scope(env)`**: Manages the lifetime of JS handles.
   - **`args`**: Unique pointer to manage `CallbackData` lifecycle.

   - **Creating JS `Int32Array`:**
     - **`Napi::ArrayBuffer`**: Wraps the `dataCopy` in an ArrayBuffer.
     - **`Napi::Int32Array::New`**: Creates a new `Int32Array` in JS using the ArrayBuffer.

   - **Calling JS Predicate Function:**
     - **`jsFn.Call({ inputArr })`**: Executes the JS predicate with the `Int32Array`.
     - **Validation:**
       - Ensures the returned value is a `Uint8Array` of the same length.
       - If valid, copies the `Uint8Array` data into `mask`.
       - Otherwise, sets an error message.

   - **Marking Completion:**
     - **`args->done->store(true)`**: Signals that the JS callback has completed.

5. **Error Handling:**
   - If `NonBlockingCall` fails, throws an exception.
   - After invoking, the function **busy-waits** until `done` is `true`.
   - If an error occurred during JS execution, throws an exception.

6. **Returning the Mask:**
   - Returns the `mask` vector indicating which elements satisfy the predicate.

## Interplay Between `MaskData` and `MaskPredicate`

### Purpose

- **`MaskData`**: Manages the state required for the predicate during the parallel copy operation.
- **`MaskPredicate`**: A functor that accesses `MaskData` to determine whether an element should be copied.

### Detailed Structure and Functionality

#### `MaskData` Struct

```cpp
struct MaskData {
    const uint8_t* maskData;
    std::atomic<size_t> currentIndex{0};
};
```

**Components:**

- **`maskData`**: Pointer to the mask array (`uint8_t`), where each element is `1` if the corresponding input element satisfies the predicate, or `0` otherwise.
- **`currentIndex`**: An atomic counter to keep track of the current position in the mask array. This ensures thread-safe access when multiple threads evaluate the predicate concurrently.

#### `MaskPredicate` Struct

```cpp
struct MaskPredicate {
    std::shared_ptr<MaskData> data;
    std::shared_ptr<std::vector<uint8_t>> maskHolder; 
    bool operator()(int32_t /*val*/) const {
        size_t idx = data->currentIndex.fetch_add(1, std::memory_order_relaxed);
        return (*maskHolder)[idx] == 1;
    }
};
```

**Components:**

- **`data`**: Shared pointer to `MaskData`, providing access to the mask data and the atomic index.
- **`maskHolder`**: Shared pointer to the actual `mask` vector. This ensures that the mask data remains valid and isn't destroyed while the predicate is in use.

**Functionality of `operator()`:**

1. **Atomic Index Fetch and Increment:**
   - **`fetch_add(1, std::memory_order_relaxed)`**: Atomically retrieves the current index and increments it by `1`. The `memory_order_relaxed` specifies that no memory ordering constraints are enforced, which is sufficient here as we only need atomicity.

2. **Predicate Evaluation:**
   - **`(*maskHolder)[idx] == 1`**: Checks if the mask at the retrieved index is `1`.
     - Returns `true` if the corresponding input element satisfies the predicate.
     - Returns `false` otherwise.

### Why Use `MaskData` and `MaskPredicate`?

- **Thread Safety:** Since `hpx_copy_if` may run the predicate concurrently across multiple threads, it's crucial to manage the current index safely. The atomic `currentIndex` ensures that each thread accesses a unique position in the mask array without race conditions.
  
- **Efficiency:** By precomputing the mask in a single batch operation (`GetPredicateMaskBatchUsingTSFN`), we avoid multiple calls back into JavaScript. The predicate simply reads from the mask, which is fast and efficient.

## Flowchart

To better understand the flow, here's an ASCII-based flowchart illustrating the entire process.

```
[JavaScript Code]
        |
        | Calls CopyIf(Int32Array, predicateFunction)
        |
[CopyIf Function]
        |
        | Extracts Int32Array and JS Function
        |
        | Creates ThreadSafeFunction (TSFN) for predicate
        |
        | Queues Asynchronous Work via QueueAsyncWork
        |
[Asynchronous Task]
        |
        | Calls GetPredicateMaskBatchUsingTSFN
        |   |
        |   |-- Creates CallbackData with dataCopy and mask
        |   |-- Calls TSFN.NonBlockingCall with CallbackData
        |   |-- JS Thread:
        |          |-- Receives dataCopy as Int32Array
        |          |-- Executes predicateFunction(Int32Array)
        |          |-- Expects Uint8Array mask back
        |          |-- Copies mask data into mask vector
        |          |-- Sets done = true
        |   |-- C++ Thread waits until done == true
        |   |-- Returns mask vector
        |
        | Sets up MaskData with maskData pointer and atomic currentIndex
        |
        | Defines MaskPredicate with MaskData and maskHolder
        |
        | Executes hpx_copy_if(dataPtr, dataSize, MaskPredicate)
        |   |
        |   |-- hpx_copy_if iterates over dataPtr
        |   |-- For each element, calls MaskPredicate()
        |          |-- Fetches currentIndex atomically
        |          |-- Checks maskData[currentIndex] == 1
        |          |-- If true, copies element to output vector
        |
        | Waits for hpx_copy_if to complete and retrieves result vector
        |
[Completion Callback]
        |
        | Aborts TSFN
        |-- If error: Rejects Promise with error
        |-- Else: Copies result vector to new Int32Array
        |         Resolves Promise with new Int32Array
        |
[JavaScript Promise]
        |
        | Receives filtered Int32Array
        |
```

## Usage Example

Here's how a developer might use the `CopyIf` function within a Node.js application:

```javascript
const hpxAddon = require('./build/Release/hpxaddon');

// Sample Int32Array
const inputArray = new Int32Array([1, 2, 3, 4, 5, 6, 7, 8, 9, 10]);

// Predicate function: Select even numbers
const isEven = (arr) => {
    const mask = new Uint8Array(arr.length);
    for (let i = 0; i < arr.length; i++) {
        mask[i] = (arr[i] % 2 === 0) ? 1 : 0;
    }
    return mask;
};

// Using CopyIf
hpxAddon.copyIf(inputArray, isEven)
    .then(filteredArray => {
        console.log('Filtered Array:', filteredArray);
    })
    .catch(err => {
        console.error('Error:', err);
    });
```

**Explanation:**

1. **Importing the Addon:**
   - The addon is required from the compiled binary.

2. **Preparing Data:**
   - An `Int32Array` named `inputArray` is created with sample data.

3. **Defining the Predicate:**
   - The `isEven` function serves as the predicate, returning a `Uint8Array` mask where each element is `1` if the corresponding element in `inputArray` is even, and `0` otherwise.

4. **Executing `CopyIf`:**
   - The `copyIf` function is called with `inputArray` and `isEven`.
   - Upon successful completion, it logs the filtered array containing only even numbers.
   - If an error occurs, it logs the error message.

**Expected Output:**
```
Filtered Array: Int32Array(5) [ 2, 4, 6, 8, 10 ]
```

### Using Helper Functions

To simplify the process of creating mask arrays from simple per-element predicate functions, developers can utilize helper functions. This abstracts away the complexity of manually generating mask arrays, making the code cleaner and more intuitive.

#### Helper Function: `elementPredicateToMask`

```javascript
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
```

**Usage Example:**

```javascript
const hpxAddon = require('./build/Release/hpxaddon');

// Sample Int32Array
const inputArray = new Int32Array([1, 2, 3, 4, 5, 6, 7, 8, 9, 10]);

// Simple per-element predicate: Check if a number is even
const isEven = (val) => val % 2 === 0;

// Convert the per-element predicate to a batch mask generator
const isEvenMask = elementPredicateToMask(isEven);

// Using CopyIf with the helper function
hpxAddon.copyIf(inputArray, isEvenMask)
    .then(filteredArray => {
        console.log('Filtered Array:', filteredArray);
    })
    .catch(err => {
        console.error('Error:', err);
    });
```

**Benefits:**

- **Simplified Predicate Definition:** Developers can write straightforward per-element predicates without worrying about mask array generation.
- **Enhanced Readability:** The code is more intuitive and easier to maintain.
- **Reusability:** The helper function can be reused across different filtering operations.

**Expected Output:**
```
Filtered Array: Int32Array(5) [ 2, 4, 6, 8, 10 ]
```

## Supporting `SortComp` with Comparator Helpers

In addition to filtering operations like `CopyIf`, other functions like `SortComp` also benefit from similar predicate and comparator handling. To streamline the use of comparators in sorting operations, helper functions can be employed to convert simple comparator functions into key extraction functions required by the addon.

### Helper Function: `elementComparatorToKeys`

```javascript
/**
 * Converts a per-element comparator function into a batch key extractor.
 *
 * @param {Function} comp - A comparator function that takes two elements and returns a boolean.
 * @returns {Function} - A function that takes an Int32Array and returns an Int32Array of keys.
 */
function elementComparatorToKeys(comp) {
  const testVal = comp(2,1);
  return (inputArr) => {
    const keys = new Int32Array(inputArr.length);
    if (testVal) {
      // descending
      for (let i = 0; i < inputArr.length; i++) {
        keys[i] = -inputArr[i];
      }
    } else {
      // ascending
      for (let i = 0; i < inputArr.length; i++) {
        keys[i] = inputArr[i];
      }
    }
    return keys;
  };
}
```

**Usage Example:**

```javascript
const hpxAddon = require('./build/Release/hpxaddon');

// Sample Int32Array
const inputArray = new Int32Array([5, 3, 8, 1, 9, 2]);

// Comparator function: Descending order
const descendingComparator = (a, b) => a > b;

// Convert the comparator to a key extractor
const descendingKeys = elementComparatorToKeys(descendingComparator);

// Using SortComp with the helper function
hpxAddon.sortComp(inputArray, descendingKeys)
    .then(sortedArray => {
        console.log('Sorted Array (Descending):', sortedArray);
    })
    .catch(err => {
        console.error('Error:', err);
    });
```

**Benefits:**

- **Simplified Comparator Definition:** Developers can write straightforward comparator functions without handling key extraction logic.
- **Flexibility:** Easily switch between ascending and descending order by changing the comparator.
- **Consistency:** Maintains a consistent approach similar to predicate handling in filtering operations.

**Expected Output:**
```
Sorted Array (Descending): Int32Array(6) [ 9, 8, 5, 3, 2, 1 ]
```

### Detailed Breakdown of `SortComp`

To understand how sorting with custom comparators works, let's examine the `SortComp` function and its supporting mechanisms.

```cpp
/**
 * @brief Sorts an Int32Array using a user-provided JavaScript key extraction function.
 *
 * Instead of calling a JS comparator per element, we do a single batch call to get keys, then sort C++ side.
 * Returns a Promise with the sorted array.
 *
 */
Napi::Value SortComp(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    auto inputArr = GetInt32ArrayArgument(info,0);
    Napi::Function fn = info[1].As<Napi::Function>();
    const int32_t* dataPtr = inputArr.Data();
    size_t dataSize = inputArr.ElementLength();

    // Create TSFN for key extraction
    auto tsfn = Napi::ThreadSafeFunction::New(env, fn, "BatchKeyExtractor", 0, 1);
    auto tsfnPtr = std::make_shared<Napi::ThreadSafeFunction>(std::move(tsfn));

    return QueueAsyncWork<std::shared_ptr<std::vector<int32_t>>>(
        env,
        [dataPtr, dataSize, tsfnPtr](std::shared_ptr<std::vector<int32_t>>& res, std::string &err){
            try {
                // Step 3a: Extract Keys from JS Predicate
                auto keys = GetKeyArrayBatchUsingTSFN(*tsfnPtr, dataPtr, dataSize);

                // Step 3b: Setup Sorting Mechanism
                auto inputVec = std::make_shared<std::vector<int32_t>>(dataPtr, dataPtr + dataSize);
                std::vector<int32_t> idx(dataSize);
                for (int32_t i = 0; i < (int32_t)dataSize; i++) idx[i] = i;

                // Step 3c: Define Comparator for Indices
                auto comp = [keys](int32_t a, int32_t b) {
                    return (*keys)[static_cast<size_t>(a)] < (*keys)[static_cast<size_t>(b)];
                };

                // Step 3d: Execute hpx_sort_comp with Comparator
                auto fut = hpx_sort_comp(idx.data(), dataSize, comp);
                auto sortedIdx = fut.get(); // sortedIdx is the final sorted index array

                // Step 3e: Rearrange Data According to Sorted Indices
                std::vector<int32_t> sortedData(dataSize);
                for (size_t i=0; i<dataSize; i++) {
                    sortedData[i] = (*inputVec)[static_cast<size_t>((*sortedIdx)[i])];
                }
                res = std::make_shared<std::vector<int32_t>>(std::move(sortedData));

            } catch (const std::exception &e) { err = e.what(); }
        },
        [tsfnPtr](Napi::Env env, Napi::Promise::Deferred& def,
                  std::shared_ptr<std::vector<int32_t>>& res, const std::string &err) {
            tsfnPtr->Abort();
            if(!err.empty()) def.Reject(Napi::String::New(env, err));
            else {
                Napi::Int32Array arr = Napi::Int32Array::New(env, res->size());
                memcpy(arr.Data(), res->data(), res->size()*sizeof(int32_t));
                def.Resolve(arr);
            }
        }
    );
}
```

### Step-by-Step Explanation

1. **Step 1: Extract Arguments**
   - **`inputArr`**: Retrieves the first argument as an `Int32Array` using a helper function `GetInt32ArrayArgument`.
   - **`fn`**: Retrieves the second argument as a JavaScript function.
   - **`dataPtr` & `dataSize`**: Obtains a pointer to the raw data and its size from the `Int32Array`.

2. **Step 2: Create ThreadSafeFunction (TSFN) for Key Extraction**
   - **TSFN Creation**: Initializes a `ThreadSafeFunction` to safely invoke the JavaScript key extraction function from a worker thread.
   - **`tsfnPtr`**: Wraps the TSFN in a `shared_ptr` to manage its lifetime across asynchronous operations.

3. **Step 3: Queue Asynchronous Work**
   - **`QueueAsyncWork`**: Schedules the provided lambda to run asynchronously, ensuring the main Node.js event loop remains unblocked.

   - **Step 3a: Extract Keys from JS Predicate**
     - **`GetKeyArrayBatchUsingTSFN`**: Calls the JavaScript key extraction function once with the entire array, expecting an `Int32Array` of keys corresponding to each element.

   - **Step 3b: Setup Sorting Mechanism**
     - **`inputVec`**: Constructs a `std::vector<int32_t>` from the input data.
     - **`idx`**: Creates an index array to track the original positions of elements.

   - **Step 3c: Define Comparator for Indices**
     - **`comp`**: A lambda function that compares elements based on their corresponding keys. It accesses the keys using the indices from the `idx` array.

   - **Step 3d: Execute `hpx_sort_comp` with Comparator**
     - **`hpx_sort_comp`**: Utilizes HPX's parallel sort algorithm to sort the `idx` array based on the comparator.
     - **`sortedIdx`**: Holds the sorted indices after the sorting operation.

   - **Step 3e: Rearrange Data According to Sorted Indices**
     - **`sortedData`**: Constructs the final sorted data by mapping the sorted indices back to the original data.

4. **Step 4: Abort TSFN and Resolve/Reject Promise**
   - **`tsfnPtr->Abort()`**: Cleans up the TSFN, indicating that no further JS callbacks are needed.
   - **Promise Handling**:
     - If an error occurred, the Promise is rejected with the error message.
     - Otherwise, a new `Int32Array` is created, the sorted data is copied into it, and the Promise is resolved with this array.

## Understanding `GetKeyArrayBatchUsingTSFN`

The `GetKeyArrayBatchUsingTSFN` function operates similarly to `GetPredicateMaskBatchUsingTSFN` but is tailored for key extraction in sorting operations. It executes the key extraction function in JavaScript and retrieves an array of keys used for sorting in C++.

```cpp
/**
 * @brief Retrieves a key array from a JavaScript callback using a ThreadSafeFunction.
 *
 * Similar to GetPredicateMaskBatchUsingTSFN, but this time we deal with "keys" for sorting.
 * For operations like sortComp or partialSortComp, instead of calling a comparator per element,
 * we call a key-extractor JS function once. This function must return an Int32Array of keys
 * that correspond 1-to-1 with the input data. The keys help us reorder elements efficiently
 * in C++ without multiple JS calls.
 *
 * Steps:
 * - Copy input C++ array into dataCopy.
 * - NonBlockingCall to JS function, passing entire array as Int32Array.
 * - JS must return an Int32Array of the same length, serving as "keys".
 * - We copy these keys into a std::vector<int32_t> 'keys', which is returned for C++ sorting.
 *
 * @param tsfn A Napi::ThreadSafeFunction representing the JS key extractor callback.
 * @param data Pointer to the input int32_t array.
 * @param length Number of elements in 'data'.
 * @return A shared_ptr to a std::vector<int32_t> containing keys for each element.
 * @throws std::runtime_error if JS returns something invalid or if NonBlockingCall fails.
 */
std::shared_ptr<std::vector<int32_t>> GetKeyArrayBatchUsingTSFN(const Napi::ThreadSafeFunction& tsfn, const int32_t* data, size_t length) {
    std::atomic<bool> done(false);
    std::string error;
    auto keys = std::make_shared<std::vector<int32_t>>(length);

    struct CallbackData {
        std::vector<int32_t> dataCopy;
        size_t length;
        std::shared_ptr<std::vector<int32_t>> keys;
        std::string* error;
        std::atomic<bool>* done;
    };

    auto cbData = new CallbackData{
        std::vector<int32_t>(data, data+length),
        length,
        keys,
        &error,
        &done
    };

    napi_status st = tsfn.NonBlockingCall(cbData, [](Napi::Env env, Napi::Function jsFn, void* raw) {
        Napi::HandleScope scope(env);
        std::unique_ptr<CallbackData> args((CallbackData*)raw);

        // Create Int32Array for JS
        auto buf = Napi::ArrayBuffer::New(env, (void*)args->dataCopy.data(), args->length * sizeof(int32_t));
        auto inputArr = Napi::Int32Array::New(env, args->length, buf, 0);

        // JS key extractor call
        Napi::Value ret = jsFn.Call({ inputArr });
        if (!ret.IsTypedArray()) {
            *(args->error) = "Key extractor must return an Int32Array of same length as input.";
        } else {
            auto tarr = ret.As<Napi::TypedArray>();
            if (tarr.TypedArrayType() != napi_int32_array || tarr.ElementLength() != args->length) {
                *(args->error) = "Key extractor must return Int32Array of same length.";
            } else {
                auto keyArr = tarr.As<Napi::Int32Array>();
                memcpy(args->keys->data(), keyArr.Data(), args->length * sizeof(int32_t));
            }
        }
        args->done->store(true);
    });

    if (st != napi_ok) {
        throw std::runtime_error("Failed NonBlockingCall for key extraction.");
    }

    // Wait for JS callback to complete
    while (!done.load()) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    if (!error.empty()) throw std::runtime_error(error);
    return keys;
}
```

### Step-by-Step Explanation

1. **Initialization:**
   - **`done`**: An atomic flag to indicate when the JS callback has completed.
   - **`error`**: A string to capture any error messages.
   - **`keys`**: A shared pointer to a `std::vector<int32_t>` that will store the keys used for sorting.

2. **Preparing Callback Data:**
   - **`CallbackData` Struct**: Contains the data to send to JS and placeholders for the keys and error messages.
     - **`dataCopy`**: A copy of the input data (`Int32Array`) to send to JS.
     - **`length`**: Number of elements.
     - **`keys`**: Pointer to store the resulting keys.
     - **`error`**: Pointer to store error messages.
     - **`done`**: Pointer to the atomic flag.

   - **`cbData`**: Dynamically allocated `CallbackData` instance containing the data to send to JS.

3. **Executing the JS Callback:**
   - **`tsfn.NonBlockingCall`**: Invokes the JavaScript key extractor function asynchronously without blocking the main thread.
     - **Arguments:**
       - **`cbData`**: Pointer to `CallbackData`.
       - **Lambda Function**: Handles the execution on the JS thread.

4. **JS Callback Handling (Lambda):**
   - **`Napi::HandleScope scope(env)`**: Manages the lifetime of JS handles.
   - **`args`**: Unique pointer to manage `CallbackData` lifecycle.

   - **Creating JS `Int32Array`:**
     - **`Napi::ArrayBuffer`**: Wraps the `dataCopy` in an ArrayBuffer.
     - **`Napi::Int32Array::New`**: Creates a new `Int32Array` in JS using the ArrayBuffer.

   - **Calling JS Key Extractor Function:**
     - **`jsFn.Call({ inputArr })`**: Executes the JS key extractor with the `Int32Array`.
     - **Validation:**
       - Ensures the returned value is an `Int32Array` of the same length.
       - If valid, copies the `Int32Array` data into `keys`.
       - Otherwise, sets an error message.

   - **Marking Completion:**
     - **`args->done->store(true)`**: Signals that the JS callback has completed.

5. **Error Handling:**
   - If `NonBlockingCall` fails, throws an exception.
   - After invoking, the function **busy-waits** until `done` is `true`.
   - If an error occurred during JS execution, throws an exception.

6. **Returning the Keys:**
   - Returns the `keys` vector used for sorting.

## Interplay Between `KeyData` and `KeyComparator`

### Purpose

- **`KeyData`**: Manages the state required for the comparator during the parallel sorting operation.
- **`KeyComparator`**: A functor that accesses `KeyData` to determine the sorting order based on the extracted keys.

### Detailed Structure and Functionality

#### `KeyData` Struct

```cpp
struct KeyData {
    const int32_t* keyData;
    std::atomic<size_t> currentIndex{0};
};
```

**Components:**

- **`keyData`**: Pointer to the key array (`int32_t`), where each element represents the sorting key for the corresponding input element.
- **`currentIndex`**: An atomic counter to keep track of the current position in the key array. This ensures thread-safe access when multiple threads evaluate the comparator concurrently.

#### `KeyComparator` Struct

```cpp
struct KeyComparator {
    std::shared_ptr<KeyData> data;
    std::shared_ptr<std::vector<int32_t>> keyHolder; 
    bool operator()(int32_t a, int32_t b) const {
        // Fetch indices atomically
        size_t idxA = data->currentIndex.fetch_add(1, std::memory_order_relaxed);
        size_t idxB = data->currentIndex.fetch_add(1, std::memory_order_relaxed);
        return keyHolder->at(idxA) < keyHolder->at(idxB);
    }
};
```

**Components:**

- **`data`**: Shared pointer to `KeyData`, providing access to the key data and the atomic index.
- **`keyHolder`**: Shared pointer to the actual `keys` vector. This ensures that the key data remains valid and isn't destroyed while the comparator is in use.

**Functionality of `operator()`:**

1. **Atomic Index Fetch and Increment:**
   - **`fetch_add(1, std::memory_order_relaxed)`**: Atomically retrieves the current index and increments it by `1` for each comparator call.

2. **Comparator Evaluation:**
   - **`keyHolder->at(idxA) < keyHolder->at(idxB)`**: Compares the keys at the retrieved indices to determine the sorting order.
     - Returns `true` if the key at `idxA` is less than the key at `idxB`, indicating that element `a` should come before element `b`.
     - Returns `false` otherwise.

### Why Use `KeyData` and `KeyComparator`?

- **Thread Safety:** Since `hpx_sort_comp` may run the comparator concurrently across multiple threads, managing the current indices atomically ensures that each comparison accesses unique key pairs without race conditions.
  
- **Efficiency:** By extracting keys in a single batch operation (`GetKeyArrayBatchUsingTSFN`), the sorting process avoids multiple JS calls, leveraging the speed of C++ for the actual sorting based on precomputed keys.

## Flowchart for `SortComp`

Understanding the sorting process with custom comparators is crucial for implementing efficient sorting algorithms. Below is an ASCII-based flowchart illustrating the entire process of the `SortComp` function.

```
[JavaScript Code]
        |
        | Calls SortComp(Int32Array, comparatorFunction)
        |
[SortComp Function]
        |
        | Extracts Int32Array and JS Function
        |
        | Creates ThreadSafeFunction (TSFN) for key extraction
        |
        | Queues Asynchronous Work via QueueAsyncWork
        |
[Asynchronous Task]
        |
        | Calls GetKeyArrayBatchUsingTSFN
        |   |
        |   |-- Creates CallbackData with dataCopy and keys
        |   |-- Calls TSFN.NonBlockingCall with CallbackData
        |   |-- JS Thread:
        |          |-- Receives dataCopy as Int32Array
        |          |-- Executes comparatorFunction(Int32Array)
        |          |-- Expects Int32Array keys back
        |          |-- Copies keys data into keys vector
        |          |-- Sets done = true
        |   |-- C++ Thread waits until done == true
        |   |-- Returns keys vector
        |
        | Builds inputVec and idx array
        |
        | Defines Comparator using KeyData and KeyComparator
        |
        | Executes hpx_sort_comp(idx.data(), dataSize, comp)
        |   |
        |   |-- hpx_sort_comp iterates over idx array
        |   |-- For each comparison, calls KeyComparator()
        |          |-- Fetches idxA and idxB atomically
        |          |-- Compares keys[idxA] and keys[idxB]
        |          |-- Determines sorting order based on comparator
        |
        | Waits for hpx_sort_comp to complete and retrieves sortedIdx
        |
        | Rearranges inputVec based on sortedIdx to form sortedData
        |
[Completion Callback]
        |
        | Aborts TSFN
        |-- If error: Rejects Promise with error
        |-- Else: Copies sortedData to new Int32Array
        |         Resolves Promise with sorted Int32Array
        |
[JavaScript Promise]
        |
        | Receives sorted Int32Array
        |
```

## Usage Example

To demonstrate the usage of helper functions in simplifying predicate and comparator definitions, consider the following example integrating both `CopyIf` and `SortComp`.

```javascript
const hpxAddon = require('./build/Release/hpxaddon');

// Sample Int32Array
const inputArray = new Int32Array([5, 3, 8, 1, 9, 2]);

// Helper Function: Convert per-element predicate to mask generator
function elementPredicateToMask(pred) {
  return (inputArr) => {
    const mask = new Uint8Array(inputArr.length);
    for (let i = 0; i < inputArr.length; i++) {
      mask[i] = pred(inputArr[i]) ? 1 : 0;
    }
    return mask;
  };
}

// Helper Function: Convert per-element comparator to key extractor
function elementComparatorToKeys(comp) {
  const testVal = comp(2,1);
  return (inputArr) => {
    const keys = new Int32Array(inputArr.length);
    if (testVal) {
      // descending
      for (let i = 0; i < inputArr.length; i++) {
        keys[i] = -inputArr[i];
      }
    } else {
      // ascending
      for (let i = 0; i < inputArr.length; i++) {
        keys[i] = inputArr[i];
      }
    }
    return keys;
  };
}

// Predicate Function: Select even numbers
const isEven = (val) => val % 2 === 0;

// Comparator Function: Descending order
const descendingComparator = (a, b) => a > b;

// Convert per-element predicate and comparator using helper functions
const isEvenMask = elementPredicateToMask(isEven);
const descendingKeys = elementComparatorToKeys(descendingComparator);

// Using CopyIf with the helper function
hpxAddon.copyIf(inputArray, isEvenMask)
    .then(filteredArray => {
        console.log('Filtered Array:', filteredArray);
        
        // Using SortComp on the filtered array
        return hpxAddon.sortComp(filteredArray, descendingKeys);
    })
    .then(sortedArray => {
        console.log('Sorted Array (Descending):', sortedArray);
    })
    .catch(err => {
        console.error('Error:', err);
    });
```

**Explanation:**

1. **Importing the Addon:**
   - The addon is required from the compiled binary.

2. **Preparing Data:**
   - An `Int32Array` named `inputArray` is created with sample data.

3. **Defining Predicate and Comparator:**
   - **`isEven`**: A simple per-element predicate that checks if a number is even.
   - **`descendingComparator`**: A per-element comparator that defines descending order.

4. **Using Helper Functions:**
   - **`elementPredicateToMask`**: Converts the `isEven` predicate into a mask generator.
   - **`elementComparatorToKeys`**: Converts the `descendingComparator` into a key extractor.

5. **Executing `CopyIf`:**
   - Filters the `inputArray` to include only even numbers.
   - Logs the filtered array.

6. **Executing `SortComp`:**
   - Sorts the filtered array in descending order using the extracted keys.
   - Logs the sorted array.

**Expected Output:**
```
Filtered Array: Int32Array(5) [ 2, 4, 6, 8, 10 ]
Sorted Array (Descending): Int32Array(5) [ 10, 8, 6, 4, 2 ]
```
