# Testing

## Table of Contents
- [Testing](#testing)
  - [Table of Contents](#table-of-contents)
  - [Introduction](#introduction)
  - [Prerequisites](#prerequisites)
  - [Running the Tests](#running-the-tests)
  - [What the Tests Cover](#what-the-tests-cover)
  - [Adding New Tests](#adding-new-tests)
  - [Troubleshooting Test Failures](#troubleshooting-test-failures)

---

## Introduction

This addon provides a set of tests (for example, `hpx-addon.test.mjs`) that verify the correctness of its exposed functions. These tests ensure that operations like `sort`, `count`, `copyIf`, and others return expected results and handle various input conditions.

---

## Prerequisites

1. **Addon Built:**  
   The addon (`hpxaddon.node`) must be compiled and available. See [docs/Building.md](./Building.md).

2. **HPX and Environment Setup:**  
   Ensure HPX is installed and accessible. For convenience, you can run tests inside the provided Docker environment as described in [docs/Docker.md](./Docker.md).

3. **Node.js & Test Runner:**  
   The tests are written using native ESM and common Node.js APIs. We use `import` statements and basic assertions (e.g., via `chai` or `assert` module). Make sure you have Node.js v20+.

---

## Running the Tests

If you have a `test` script defined in `package.json` and a file like `hpx-addon.test.mjs`, you can run:

```bash
npm test
```

Or, directly:

```bash
node --loader ts-node/esm app/test/hpx-addon.test.mjs
```

*(Adjust the command if your tests differ or if you’re not using a loader. Check the provided `package.json` and `test` folder for the exact command.)*

**What happens:**
- The tests initialize HPX with a test configuration.
- Run various operations, compare the output to expected results.
- Finalize HPX after tests complete.

Upon completion, you should see either a success message or details about which tests failed.

---

## What the Tests Cover

- **Basic Operations:**  
  `sort`, `count`, `copy`, `fill`, `find`, etc. are tested with small arrays, checking correctness of output.
  
- **Predicates & Comparators:**  
  `countIf`, `copyIf`, `sortComp`, and `partialSortComp` tests ensure that JS predicates and comparators work correctly, verifying that the addon correctly batches calls and returns expected results.

- **Error Handling & Edge Cases:**  
  Some tests try empty arrays, very small arrays, or arrays that exceed the `threshold`. This ensures both sequential and parallel paths are tested.

- **HPX Lifecycle:**  
  Tests also confirm that `initHPX` and `finalizeHPX` work correctly, ensuring no exceptions are thrown if HPX is started and stopped multiple times or if invalid configurations are provided.

---

## Adding New Tests

1. **Create a New Test File:**  
   Add a `.mjs` file in the `app/test/` directory. For example, `app/test/new-feature.test.mjs`.

2. **Require the Addon:**  
   ```js
   import { createRequire } from 'module';
   const require = createRequire(import.meta.url);
   const hpxaddon = require('../../addons/hpxaddon.node');
   ```

3. **Initialize HPX (If Needed):**  
   In a `before` hook, call `await hpxaddon.initHPX({...})` with test config.

4. **Write Assertions:**  
   Use `assert` or `chai` to verify results. Example:
   ```js
   import { strict as assert } from 'assert';
   const arr = Int32Array.from([3,1,2]);
   const sorted = await hpxaddon.sort(arr);
   assert.deepEqual(Array.from(sorted), [1,2,3]);
   ```

5. **Finalize HPX in After Hook:**  
   `await hpxaddon.finalizeHPX();`

6. **Update npm Scripts (If Needed):**  
   Ensure `npm test` runs your new test file as well.

---

## Troubleshooting Test Failures

- **HPX Not Initialized:**  
  Make sure tests call `initHPX` before performing operations.

- **Environment Variables Missing:**  
  If HPX libraries aren’t found, run tests inside the Docker environment or ensure `LD_LIBRARY_PATH` is set correctly.

- **Timeouts or Slow Tests:**  
  If tests run very slowly, enable fewer tests, or reduce data sizes in test arrays. You can also adjust configuration parameters (like `threadCount` or `threshold`) in the test’s init config.

- **Logging for Clues:**  
  Set `loggingEnabled: true` and `logLevel: 'debug'` in the test config to see internal logs that might explain failures.
