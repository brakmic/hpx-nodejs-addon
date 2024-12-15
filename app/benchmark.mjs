import { createRequire } from 'module';
import Benchmark from 'benchmark';
import chalk from 'chalk';
import ora from 'ora';
import figlet from 'figlet';
import Table from 'cli-table3';

const require = createRequire(import.meta.url);
const addon = require('./addons/hpxaddon.node');

// Helpers (same logic as in index.mjs)
function elementPredicateToMask(pred) {
  return (inputArr) => {
    const mask = new Uint8Array(inputArr.length);
    for (let i = 0; i < inputArr.length; i++) {
      mask[i] = pred(inputArr[i]) ? 1 : 0;
    }
    return mask;
  };
}

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

// Predicates and comparators:
const predicateFunction = (x) => x > 50000;
const batchPredicateFunction = elementPredicateToMask(predicateFunction);

const comparatorFunction = (a, b) => a < b;
const batchComparatorFunction = elementComparatorToKeys(comparatorFunction);

/**
 * Initializes HPX asynchronously.
 */
const initializeHPX = async (config) => {
  const spinner = ora(chalk.blue('Initializing HPX for benchmarking...')).start();
  try {
    const initSuccess = await addon.initHPX(config);
    if (!initSuccess) {
      spinner.fail(chalk.red('Failed to initialize HPX'));
      throw new Error('Failed to initialize HPX');
    }
    spinner.succeed(chalk.green('HPX initialized for benchmarking.'));
  } catch (err) {
    spinner.fail(chalk.red(`Error initializing HPX: ${err.message}`));
    throw err;
  }
};

/**
 * Finalizes HPX asynchronously.
 */
const finalizeHPXAsync = async () => {
  const spinner = ora(chalk.blue('Finalizing HPX...')).start();
  try {
    const finalizeSuccess = await addon.finalizeHPX();
    if (!finalizeSuccess) {
      spinner.fail(chalk.red('Failed to finalize HPX'));
      throw new Error('Failed to finalize HPX');
    }
    spinner.succeed(chalk.green('HPX finalized successfully.'));
  } catch (err) {
    spinner.fail(chalk.red(`Error finalizing HPX: ${err.message}`));
    throw err;
  }
};

/**
 * Configuration for benchmarking tests.
 */
const benchmarkTests = [
  {
    operation: 'Sort',
    active: true,
    tests: [
      {
        name: 'HPX Sort',
        fn: (deferred, { cloneDataForTest, batchSize }) => {
          const promises = [];
          for (let i = 0; i < batchSize; i++) {
            const testData = cloneDataForTest();
            promises.push(addon.sort(testData));
          }
          Promise.all(promises)
            .then(() => deferred.resolve())
            .catch(() => deferred.resolve());
        },
      },
      {
        name: 'JavaScript Sort',
        fn: (deferred, { cloneDataForTest, batchSize }) => {
          try {
            for (let i = 0; i < batchSize; i++) {
              const testData = cloneDataForTest();
              testData.sort((a, b) => a - b);
            }
            deferred.resolve();
          } catch {
            deferred.resolve();
          }
        },
      },
    ],
  },
  {
    operation: 'Count',
    active: true,
    tests: [
      {
        name: 'HPX Count',
        fn: (deferred, { data, valueToCount, batchSize }) => {
          const promises = [];
          for (let i = 0; i < batchSize; i++) {
            promises.push(addon.count(data, valueToCount));
          }
          Promise.all(promises)
            .then(() => deferred.resolve())
            .catch(() => deferred.resolve());
        },
      },
      {
        name: 'JavaScript Count',
        fn: (deferred, { data, valueToCount, batchSize }) => {
          try {
            for (let i = 0; i < batchSize; i++) {
              let c = 0;
              for (let val of data) {
                if (val === valueToCount) c++;
              }
            }
            deferred.resolve();
          } catch {
            deferred.resolve();
          }
        },
      },
    ],
  },
  {
    operation: 'Copy',
    active: true,
    tests: [
      {
        name: 'HPX Copy',
        fn: (deferred, { data, batchSize }) => {
          const promises = [];
          for (let i = 0; i < batchSize; i++) {
            promises.push(addon.copy(data));
          }
          Promise.all(promises)
            .then(() => deferred.resolve())
            .catch(() => deferred.resolve());
        },
      },
      {
        name: 'JavaScript Copy',
        fn: (deferred, { data, batchSize }) => {
          try {
            for (let i = 0; i < batchSize; i++) {
              data.slice();
            }
            deferred.resolve();
          } catch {
            deferred.resolve();
          }
        },
      },
    ],
  },
  {
    operation: 'EndsWith',
    active: true,
    tests: [
      {
        name: 'HPX EndsWith',
        fn: (deferred, { data, suffixArray, batchSize }) => {
          const promises = [];
          for (let i = 0; i < batchSize; i++) {
            promises.push(addon.endsWith(data, suffixArray));
          }
          Promise.all(promises)
            .then(() => deferred.resolve())
            .catch(() => deferred.resolve());
        },
      },
      {
        name: 'JavaScript EndsWith',
        fn: (deferred, { data, suffixArray, batchSize }) => {
          try {
            for (let i = 0; i < batchSize; i++) {
              const endSegment = data.slice(-suffixArray.length);
              endSegment.every((val, idx) => val === suffixArray[idx]);
            }
            deferred.resolve();
          } catch {
            deferred.resolve();
          }
        },
      },
    ],
  },
  {
    operation: 'Equal',
    active: true,
    tests: [
      {
        name: 'HPX Equal',
        fn: (deferred, { array1, array2, batchSize }) => {
          const promises = [];
          for (let i = 0; i < batchSize; i++) {
            promises.push(addon.equal(array1, array2));
          }
          Promise.all(promises)
            .then(() => deferred.resolve())
            .catch(() => deferred.resolve());
        },
      },
      {
        name: 'JavaScript Equal',
        fn: (deferred, { array1, array2, batchSize }) => {
          try {
            for (let i = 0; i < batchSize; i++) {
              if (array1.length === array2.length) {
                for (let j = 0; j < array1.length; j++) {
                  if (array1[j] !== array2[j]) break;
                }
              }
            }
            deferred.resolve();
          } catch {
            deferred.resolve();
          }
        },
      },
    ],
  },
  {
    operation: 'Find',
    active: true,
    tests: [
      {
        name: 'HPX Find',
        fn: (deferred, { data, valueToFind, batchSize }) => {
          const promises = [];
          for (let i = 0; i < batchSize; i++) {
            promises.push(addon.find(data, valueToFind));
          }
          Promise.all(promises)
            .then(() => deferred.resolve())
            .catch(() => deferred.resolve());
        },
      },
      {
        name: 'JavaScript Find',
        fn: (deferred, { data, valueToFind, batchSize }) => {
          try {
            for (let i = 0; i < batchSize; i++) {
              let index = -1;
              for (let j = 0; j < data.length; j++) {
                if (data[j] === valueToFind) {
                  index = j;
                  break;
                }
              }
            }
            deferred.resolve();
          } catch {
            deferred.resolve();
          }
        },
      },
    ],
  },
  {
    operation: 'Merge',
    active: true,
    tests: [
      {
        name: 'HPX Merge',
        fn: (deferred, { sortedArray1, sortedArray2, batchSize }) => {
          const promises = [];
          for (let i = 0; i < batchSize; i++) {
            promises.push(addon.merge(sortedArray1, sortedArray2));
          }
          Promise.all(promises)
            .then(() => deferred.resolve())
            .catch(() => deferred.resolve());
        },
      },
      {
        name: 'JavaScript Merge',
        fn: (deferred, { sortedArray1, sortedArray2, batchSize }) => {
          try {
            for (let i = 0; i < batchSize; i++) {
              const merged = new Int32Array(sortedArray1.length + sortedArray2.length);
              let idx1 = 0,
                idx2 = 0,
                idx = 0;
              while (idx1 < sortedArray1.length && idx2 < sortedArray2.length) {
                if (sortedArray1[idx1] < sortedArray2[idx2]) {
                  merged[idx++] = sortedArray1[idx1++];
                } else {
                  merged[idx++] = sortedArray2[idx2++];
                }
              }
              while (idx1 < sortedArray1.length) merged[idx++] = sortedArray1[idx1++];
              while (idx2 < sortedArray2.length) merged[idx++] = sortedArray2[idx2++];
            }
            deferred.resolve();
          } catch {
            deferred.resolve();
          }
        },
      },
    ],
  },
  {
    operation: 'PartialSort',
    active: true,
    tests: [
      {
        name: 'HPX PartialSort',
        fn: (deferred, { cloneDataForTest, middleIndex, batchSize }) => {
          const promises = [];
          for (let i = 0; i < batchSize; i++) {
            const testData = cloneDataForTest();
            promises.push(addon.partialSort(testData, middleIndex));
          }
          Promise.all(promises)
            .then(() => deferred.resolve())
            .catch(() => deferred.resolve());
        },
      },
      {
        name: 'JavaScript PartialSort',
        fn: (deferred, { cloneDataForTest, middleIndex, batchSize }) => {
          try {
            for (let i = 0; i < batchSize; i++) {
              const testData = cloneDataForTest();
              testData.sort((a, b) => a - b); 
            }
            deferred.resolve();
          } catch {
            deferred.resolve();
          }
        },
      },
    ],
  },
  {
    operation: 'CopyN',
    active: true,
    tests: [
      {
        name: 'HPX CopyN',
        fn: (deferred, { data, batchSize }) => {
          const promises = [];
          const count = 50000; 
          for (let i = 0; i < batchSize; i++) {
            promises.push(addon.copyN(data, count));
          }
          Promise.all(promises)
            .then(() => deferred.resolve())
            .catch(() => deferred.resolve());
        },
      },
      {
        name: 'JavaScript CopyN',
        fn: (deferred, { data, batchSize }) => {
          try {
            const count = 50000; 
            for (let i = 0; i < batchSize; i++) {
              data.slice(0, count);
            }
            deferred.resolve();
          } catch {
            deferred.resolve();
          }
        },
      },
    ],
  },
  {
    operation: 'Fill',
    active: true,
    tests: [
      {
        name: 'HPX Fill',
        fn: (deferred, { data, batchSize }) => {
          const promises = [];
          const fillValue = 7;
          for (let i = 0; i < batchSize; i++) {
            promises.push(addon.fill(data, fillValue));
          }
          Promise.all(promises)
            .then(() => deferred.resolve())
            .catch(() => deferred.resolve());
        },
      },
      {
        name: 'JavaScript Fill',
        fn: (deferred, { data, batchSize }) => {
          try {
            const fillValue = 7;
            for (let i = 0; i < batchSize; i++) {
              const filledArray = new Int32Array(data.length);
              filledArray.fill(fillValue);
            }
            deferred.resolve();
          } catch {
            deferred.resolve();
          }
        },
      },
    ],
  },
  {
    operation: 'CountIf',
    active: true,
    tests: [
      {
        name: 'HPX CountIf',
        fn: (deferred, { data, predicateFunction, batchSize }) => {
          const promises = [];
          for (let i = 0; i < batchSize; i++) {
            promises.push(addon.countIf(data, predicateFunction));
          }
          Promise.all(promises)
            .then(() => deferred.resolve())
            .catch(() => deferred.resolve());
        },
      },
      {
        name: 'JavaScript CountIf',
        fn: (deferred, { data, predicateFunction, batchSize }) => {
          try {
            for (let i = 0; i < batchSize; i++) {
              let c = 0;
              for (let val of data) {
                if (predicateFunction(val)) c++;
              }
            }
            deferred.resolve();
          } catch {
            deferred.resolve();
          }
        },
      },
    ],
  },
  {
    operation: 'CopyIf',
    active: true,
    tests: [
      {
        name: 'HPX CopyIf',
        fn: (deferred, { data, predicateFunction, batchSize }) => {
          const promises = [];
          for (let i = 0; i < batchSize; i++) {
            promises.push(addon.copyIf(data, predicateFunction));
          }
          Promise.all(promises)
            .then(() => deferred.resolve())
            .catch(() => deferred.resolve());
        },
      },
      {
        name: 'JavaScript CopyIf',
        fn: (deferred, { data, predicateFunction, batchSize }) => {
          try {
            for (let i = 0; i < batchSize; i++) {
              data.filter(x => predicateFunction(x));
            }
            deferred.resolve();
          } catch {
            deferred.resolve();
          }
        },
      },
    ],
  },
  {
    operation: 'SortComp',
    active: true,
    tests: [
      {
        name: 'HPX SortComp',
        fn: (deferred, { cloneDataForTest, comparatorFunction, batchSize }) => {
          const promises = [];
          for (let i = 0; i < batchSize; i++) {
            const testData = cloneDataForTest();
            promises.push(addon.sortComp(testData, comparatorFunction));
          }
          Promise.all(promises)
            .then(() => deferred.resolve())
            .catch(() => deferred.resolve());
        },
      },
      {
        name: 'JavaScript SortComp',
        fn: (deferred, { cloneDataForTest, comparatorFunction, batchSize }) => {
          try {
            for (let i = 0; i < batchSize; i++) {
              const testData = cloneDataForTest();
              testData.sort(comparatorFunction);
            }
            deferred.resolve();
          } catch {
            deferred.resolve();
          }
        },
      },
    ],
  },
  {
    operation: 'PartialSortComp',
    active: true,
    tests: [
      {
        name: 'HPX PartialSortComp',
        fn: (deferred, { cloneDataForTest, middleIndex, comparatorFunction, batchSize }) => {
          const promises = [];
          for (let i = 0; i < batchSize; i++) {
            const testData = cloneDataForTest();
            promises.push(addon.partialSortComp(testData, middleIndex, comparatorFunction));
          }
          Promise.all(promises)
            .then(() => deferred.resolve())
            .catch(() => deferred.resolve());
        },
      },
      {
        name: 'JavaScript PartialSortComp',
        fn: (deferred, { cloneDataForTest, middleIndex, comparatorFunction, batchSize }) => {
          try {
            for (let i = 0; i < batchSize; i++) {
              const testData = cloneDataForTest();
              testData.sort(comparatorFunction);
            }
            deferred.resolve();
          } catch {
            deferred.resolve();
          }
        },
      },
    ],
  },
];

async function runBenchmarks() {
  console.log(
    chalk.cyanBright(
      figlet.textSync('HPX Benchmark', { horizontalLayout: 'fitted' })
    )
  );

  const spinner = ora(chalk.blue('Preparing data for benchmarking...')).start();

  const arraySize = 1_000_000;
  const suffixSize = 100_000;
  const batchSize = 1;

  const config = {
    executionPolicy: 'par',
    threshold: 10000,
    threadCount: 4,
    loggingEnabled: true,
    logLevel: 'debug',
    addonName: 'hpxaddon'
  };

  let policyDescription = '';
  if (config.executionPolicy === 'par') {
    policyDescription = 'Parallel Execution Policy';
  } else if (config.executionPolicy === 'seq') {
    policyDescription = 'Sequential Execution Policy';
  } else if (config.executionPolicy === 'par_unseq') {
    policyDescription = 'Parallel Unsequenced Execution Policy';
  } else {
    policyDescription = `Unknown Execution Policy (${config.executionPolicy})`;
  }

  const data = new Int32Array(arraySize);
  for (let i = 0; i < arraySize; i++) {
    data[i] = Math.floor(Math.random() * arraySize);
  }

  const valueToCount = 42;
  const valueToFind = 123456;
  const suffixArray = data.slice(-suffixSize);

  const array1 = data.slice();
  const array2 = data.slice();
  const sortedArray1 = array1.slice().sort((a, b) => a - b);
  const sortedArray2 = array2.slice().sort((a, b) => a - b);
  const middleIndex = Math.floor(arraySize / 2);

  spinner.succeed(chalk.green('Data prepared successfully.'));
  console.log(chalk.yellow(`Data size: ${arraySize.toLocaleString()} elements`));
  console.log(chalk.yellow(`Suffix array size: ${suffixSize.toLocaleString()} elements`));
  console.log(chalk.yellow(`Partial sort middle index: ${middleIndex.toLocaleString()}`));
  console.log(chalk.yellow('HPX Configuration:'));
  console.log(chalk.yellow(`- ${policyDescription}`));
  console.log(chalk.yellow(`- Sequential Threshold: ${config.threshold.toLocaleString()}`));
  console.log(chalk.yellow(`- Thread Count: ${config.threadCount.toLocaleString()}`));
  console.log(chalk.yellow(`- Logging: ${config.loggingEnabled ? 'Enabled' : 'Disabled'} (Level: ${config.logLevel})`));
  console.log(chalk.yellow(`- Addon Name: ${config.addonName}\n`));

  const suite = new Benchmark.Suite;

  function cloneDataForTest() {
    return data.slice();
  }

  const context = {
    cloneDataForTest,
    data,
    valueToCount,
    valueToFind,
    suffixArray,
    array1,
    array2,
    sortedArray1,
    sortedArray2,
    middleIndex,
    batchSize,
    predicateFunction: batchPredicateFunction,
    comparatorFunction: batchComparatorFunction,
  };

  for (const benchmark of benchmarkTests) {
    if (!benchmark.active) continue;
    for (const test of benchmark.tests) {
      suite.add(test.name, {
        defer: true,
        fn: (deferred) => {
          test.fn(deferred, context);
        },
      });
    }
  }

  suite
    .on('cycle', function(event) {
      console.log(String(event.target));
    })
    .on('complete', function() {
      console.log(chalk.magenta('\nBenchmark completed.'));

      const results = this.filter('successful').map(bench => ({
        name: bench.name,
        hz: bench.hz,
      }));

      const operations = [...new Set(benchmarkTests.flatMap(test => test.operation))];

      const table = new Table({
        head: [
          chalk.bold('Operation'), 
          chalk.bold('HPX ops/sec'), 
          chalk.bold('JS ops/sec'), 
          chalk.bold('Winner'), 
          chalk.bold('Speedup')
        ],
        style: { head: ['cyan'] },
      });

      for (const operation of operations) {
        const hpxRes = results.find(r => r.name === `HPX ${operation}`);
        const jsRes = results.find(r => r.name === `JavaScript ${operation}`);

        let hpXHz = hpxRes ? hpxRes.hz : 0;
        let jsHz = jsRes ? jsRes.hz : 0;

        let winner = 'Tie';
        let hpXDisplay = hpXHz.toFixed(2);
        let jsDisplay = jsHz.toFixed(2);
        let speedupDisplay = 'N/A';

        if (hpXHz > jsHz) {
          // HPX is winner
          hpXDisplay = chalk.green(hpXDisplay); // winner ops/sec green
          winner = chalk.blue('HPX'); // HPX winner color: blue
          if (jsHz > 0) {
            let factor = (hpXHz / jsHz).toFixed(2);
            speedupDisplay = chalk.green(`${factor}x`); // speedup green
          }
        } else if (jsHz > hpXHz) {
          // JS is winner
          jsDisplay = chalk.green(jsDisplay); // winner ops/sec green
          winner = chalk.magenta('JS'); // JS winner color: magenta
          if (hpXHz > 0) {
            let factor = (jsHz / hpXHz).toFixed(2);
            speedupDisplay = chalk.green(`${factor}x`); // speedup green
          }
        } else {
          // tie
          winner = 'Tie';
          speedupDisplay = 'N/A';
        }

        table.push([operation, hpXDisplay, jsDisplay, winner, speedupDisplay]);
      }

      console.log(chalk.blue('\nDetailed Results:'));
      console.log(table.toString());

      finalizeHPXAsync().catch(err => {
        console.error(chalk.red('Failed to finalize HPX:'), err);
      });
    });

  suite.run({ async: true, maxTime: 10 });
}

async function main() {
  const config = {
    executionPolicy: 'par', 
    threshold: 10000,
    threadCount: 4,
    loggingEnabled: true,
    logLevel: 'debug',
    addonName: 'hpxaddon',
  };

  try {
    await initializeHPX(config);
    await runBenchmarks();
  } catch (err) {
    console.error(chalk.red('Error during benchmarking:'), err);
  }
}

main();
