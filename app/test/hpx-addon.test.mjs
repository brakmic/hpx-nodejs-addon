import { expect } from 'chai';
import { createRequire } from 'module';
import chalk from 'chalk';
import ora from 'ora';
import figlet from 'figlet';

const require = createRequire(import.meta.url);
const {
  initHPX,
  finalizeHPX,
  sort,
  copy,
  count: _count,
  endsWith,
  equal,
  find,
  merge,
  partialSort,
  copyN,
  fill,
  countIf,
  copyIf,
  sortComp,
  partialSortComp
} = require('../addons/hpxaddon.node');

// Helpers
function toInt32Array(arr) {
  return Int32Array.from(arr);
}
function elementPredicateToMask(pred) {
  const fn = (inputArr) => {
    const mask = new Uint8Array(inputArr.length);
    for (let i = 0; i < inputArr.length; i++) {
      mask[i] = pred(inputArr[i]) ? 1 : 0;
    }
    return mask;
  };
  return fn;
}
function elementComparatorToKeys(comp) {
  const testVal = comp(2,1);
  return (inputArr) => {
    const keys = new Int32Array(inputArr.length);
    if (testVal) { // descending
      for (let i=0; i<inputArr.length; i++) {
        keys[i] = -inputArr[i];
      }
    } else { // ascending
      for (let i=0; i<inputArr.length; i++) {
        keys[i] = inputArr[i];
      }
    }
    return keys;
  };
}

// Print banner once
console.log(
  chalk.cyanBright(
    figlet.textSync('HPX Addon Tests', { horizontalLayout: 'fitted' })
  )
);

describe('HPX Addon Test Suite', function() {
  this.timeout(20000);

  const config = {
    executionPolicy: 'par',
    threshold: 10000,
    threadCount: 4,
    loggingEnabled: true,
    logLevel: 'debug',
    addonName: 'hpxaddon'
  };
  let spinner;

  describe('HPX Running Tests', function() {
    before(async function() {
      spinner = ora(chalk.blue('Initializing HPX for tests...')).start();
      try {
        const initSuccess = await initHPX(config);
        if (!initSuccess) {
          spinner.fail(chalk.red('Failed to initialize HPX'));
          throw new Error('HPX initialization failed');
        }
        spinner.succeed(chalk.green('HPX initialized successfully for tests.'));
      } catch (err) {
        spinner.fail(chalk.red(`Error initializing HPX: ${err.message}`));
        throw err;
      }
    });

    after(async function() {
      spinner = ora(chalk.blue('Finalizing HPX...')).start();
      try {
        const finalizeSuccess = await finalizeHPX();
        if (!finalizeSuccess) {
          spinner.fail(chalk.red('Failed to finalize HPX'));
          throw new Error('HPX finalization failed');
        }
        spinner.succeed(chalk.green('HPX finalized successfully.'));
      } catch (err) {
        spinner.fail(chalk.red(`Error finalizing HPX: ${err.message}`));
        throw err;
      }
    });

    it('should sort an array using HPX sort', async function() {
      const unsorted = toInt32Array([5, 3, 8, 1, 9]);
      const sorted = await sort(unsorted);
      const sortedArray = Array.from(sorted);
      expect(sortedArray).to.deep.equal([1, 3, 5, 8, 9]);
    });

    it('should sort an array using HPX sortComp with custom comparator (descending)', async function() {
      const unsorted = toInt32Array([10, 5, 8, 2, 9]);
      const compDesc = (a,b)=>a>b;
      const keyExtractorDesc = elementComparatorToKeys(compDesc);
      const sorted = await sortComp(unsorted, keyExtractorDesc);
      const sortedArray = Array.from(sorted);
      expect(sortedArray).to.deep.equal([10, 9, 8, 5, 2]);
    });

    it('should copy an array using HPX copy', async function() {
      const original = toInt32Array([1, 2, 3, 4, 5]);
      const copied = await copy(original);
      const copiedArray = Array.from(copied);
      expect(copiedArray).to.deep.equal(Array.from(original));
    });

    it('should copy N elements using HPX copyN', async function() {
      const original = toInt32Array([10,10,10,10,10,10,10,10]);
      const count = 5;
      const copied = await copyN(original, count);
      const copiedArray = Array.from(copied);
      expect(copiedArray).to.deep.equal([10,10,10,10,10]);
      expect(copiedArray.length).to.equal(count);
    });

    it('should fill array using HPX fill', async function() {
      const original = toInt32Array([0,0,0,0,0]);
      const fillValue = 7;
      const filled = await fill(original, fillValue);
      const filledArray = Array.from(filled);
      expect(filledArray).to.deep.equal([7,7,7,7,7]);
    });

    it('should count occurrences using HPX count', async function() {
      const data = toInt32Array([1, 2, 2, 3, 2, 4, 2]);
      const countVal = 2;
      const countRes = await _count(data, countVal);
      expect(countRes).to.equal(4);
    });

    it('should count using HPX countIf (elements > 5)', async function() {
      const arr = toInt32Array([1,2,3,4,5,6,7,8,9]);
      const countIfPred = elementPredicateToMask(val => val > 5);
      const countRes = await countIf(arr, countIfPred);
      expect(countRes).to.equal(4); // 6,7,8,9
    });

    it('should copy elements using HPX copyIf (even numbers)', async function() {
      const arr = toInt32Array([1,2,3,4,5,6,7,8,9]);
      const copyIfPred = elementPredicateToMask(val => val % 2 === 0);
      const copied = await copyIf(arr, copyIfPred);
      const copiedArray = Array.from(copied);
      expect(copiedArray).to.deep.equal([2,4,6,8]);
    });

    it('should check if array ends with a suffix using HPX endsWith', async function() {
      const data = toInt32Array([1, 2, 3, 4, 5]);
      const suffix = toInt32Array([4, 5]);
      const result = await endsWith(data, suffix);
      expect(result).to.be.true;
    });

    it('should check if two arrays are equal using HPX equal', async function() {
      const arr1 = toInt32Array([1, 2, 3]);
      const arr2 = toInt32Array([1, 2, 3]);
      const result = await equal(arr1, arr2);
      expect(result).to.be.true;
    });

    it('should find an element using HPX find', async function() {
      const data = toInt32Array([1, 2, 3, 4, 5]);
      const valueToFind = 3;
      const index = await find(data, valueToFind);
      expect(index).to.equal(2);
    });

    it('should merge two sorted arrays using HPX merge', async function() {
      const arrA = toInt32Array([1, 3, 5]);
      const arrB = toInt32Array([2, 4, 6]);
      const merged = await merge(arrA, arrB);
      const mergedArray = Array.from(merged);
      expect(mergedArray).to.deep.equal([1,2,3,4,5,6]);
    });

    it('should perform partial sort using HPX partialSort', async function() {
      const data = toInt32Array([5, 2, 8, 1, 9]);
      const middle = 3;
      const partiallySorted = await partialSort(data, middle);
      const partiallySortedArray = Array.from(partiallySorted);
      // The first 3 elements should be the smallest 3 in ascending order: [1,2,5]
      expect(partiallySortedArray.slice(0, middle)).to.deep.equal([1,2,5]);
    });

    it('should perform partial sort with comparator using HPX partialSortComp', async function() {
      const arr = toInt32Array([5,4,3,2,1]);
      const middle = 2;
      const compPartialDesc = (a,b)=>a>b;
      const keyExtractor = elementComparatorToKeys(compPartialDesc);
      const partiallySorted = await partialSortComp(arr, middle, keyExtractor);
      const partiallySortedArray = Array.from(partiallySorted);
      // The first 2 elements should be the largest 2 in descending order: [5,4]
      expect(partiallySortedArray.slice(0, middle)).to.deep.equal([5,4]);
    });

    it('should handle incorrect HPX configuration gracefully', async function() {
      const badConfig = {
        foo: 'bar',
        executionPolicy: 'par',
        threshold: 10000,
        threadCount: 2,
        loggingEnabled: false,
        logLevel: 'info'
      };
      try {
        const initSuccess = await initHPX(badConfig);
        if (!initSuccess) {
          throw new Error('HPX initialization failed with bad config');
        }
        throw new Error('HPX initialization should have failed with bad config');
      } catch (err) {
        expect(err).to.exist;
      }
    });
  });
});
