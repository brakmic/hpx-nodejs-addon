import { createRequire } from 'module';
const require = createRequire(import.meta.url);

import chalk from 'chalk';
import figlet from 'figlet';
import ora from 'ora';
import Table from 'cli-table3';
const hpxaddon = require('./addons/hpxaddon.node');

// Helper conversions
function toInt32Array(arr) {
  // Convert normal JS array to Int32Array
  return Int32Array.from(arr);
}

// Convert a simple element-wise predicate to a batch predicate function
function elementPredicateToMask(pred) {
  // pred: (val:number) => boolean
  // return: (inputArr:Int32Array) => Uint8Array
  return (inputArr) => {
    const mask = new Uint8Array(inputArr.length);
    for (let i = 0; i < inputArr.length; i++) {
      mask[i] = pred(inputArr[i]) ? 1 : 0;
    }
    return mask;
  };
}

// Convert a simple comparator (a,b)=>bool to a key extractor
function elementComparatorToKeys(comp) {
  // If comp(a,b)=a>b means descending, we can create keys = -value to simulate descending sort by value
  // For partial/total sorting, we must produce a stable key.
  // Let's assume comp is either ascending or descending:
  // If comp(a,b)=a>b means descending, keys = -value
  // If comp(a,b)=a<b means ascending, keys = value
  // We detect direction by testing comp on a sample:
  const testVal = comp(2,1); // if true, means 2>1 is desired -> descending
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

async function main() {
  try {
    console.log(chalk.cyan(figlet.textSync('HPX Addon Demo', { horizontalLayout: 'fitted' })));
    console.log(chalk.green('Welcome to the HPX Node.js Addon Demonstration!\n'));

    const config = {
      executionPolicy: 'par',
      threshold: 10000,
      threadCount: 4,
      loggingEnabled: true,
      logLevel: 'debug',
      addonName: 'hpxaddon'
    };

    console.log(chalk.blue(`Configuration: ${JSON.stringify(config)}`));

    let spinner = ora('Initializing HPX runtime...\n').start();
    await hpxaddon.initHPX(config);
    spinner.succeed('HPX initialized successfully.');

    const resultsTable = new Table({
      head: [chalk.bold('Operation'), chalk.bold('Input'), chalk.bold('Output/Result')],
      style: { head: ['cyan'] }
    });

    // We must now pass Int32Arrays to the addon
    // Sort
    const unsortedArray = toInt32Array([5, 3, 8, 1, 9]);
    spinner = ora(`Sorting array ${Array.from(unsortedArray)}...`).start();
    const sortedArray = await hpxaddon.sort(unsortedArray);
    const sortedArrayDisp = Array.from(sortedArray);
    spinner.succeed(`Sorted: [${sortedArrayDisp}]`);
    resultsTable.push(['Sort', Array.from(unsortedArray).join(', '), sortedArrayDisp.join(', ')]);

    // Count
    const arrayToCount = toInt32Array([1, 2, 3, 2, 4, 2]);
    const valueToCount = 2;
    spinner = ora(`Counting occurrences of ${valueToCount} in ${Array.from(arrayToCount)}...`).start();
    const countRes = await hpxaddon.count(arrayToCount, valueToCount);
    spinner.succeed(`Count: ${countRes}`);
    resultsTable.push(['Count', Array.from(arrayToCount).join(', ') + ` (val=${valueToCount})`, countRes]);

    // Copy
    const arrayToCopy = toInt32Array([10, 20, 30]);
    spinner = ora(`Copying [${Array.from(arrayToCopy)}]...`).start();
    const copiedArr = await hpxaddon.copy(arrayToCopy);
    const copiedDisp = Array.from(copiedArr);
    spinner.succeed(`Copied: [${copiedDisp}]`);
    resultsTable.push(['Copy', Array.from(arrayToCopy).join(', '), copiedDisp.join(', ')]);

    // EndsWith
    const mainArray = toInt32Array([1, 2, 3, 4]);
    const suffixArray = toInt32Array([3, 4]);
    spinner = ora(`Check endsWith: [${Array.from(mainArray)}] ends with [${Array.from(suffixArray)}]?`).start();
    const endsW = await hpxaddon.endsWith(mainArray, suffixArray);
    spinner.succeed(`EndsWith: ${endsW}`);
    resultsTable.push(['EndsWith', Array.from(mainArray).join(', ') + ` endsWith ${Array.from(suffixArray).join(', ')}`, endsW]);

    // Equal
    const array1 = toInt32Array([1, 2, 3]);
    const array2 = toInt32Array([1, 2, 3]);
    spinner = ora(`Checking equal: [${Array.from(array1)}] vs [${Array.from(array2)}]`).start();
    const eqRes = await hpxaddon.equal(array1, array2);
    spinner.succeed(`Equal: ${eqRes}`);
    resultsTable.push(['Equal', `${Array.from(array1).join(', ')} vs ${Array.from(array2).join(', ')}`, eqRes]);

    // Find
    const arrayToFind = toInt32Array([5, 6, 7, 8]);
    const vFind = 7;
    spinner = ora(`Finding ${vFind} in [${Array.from(arrayToFind)}]`).start();
    const fRes = await hpxaddon.find(arrayToFind, vFind);
    const fMsg = (fRes !== -1) ? `Found at index ${fRes}` : 'Not found';
    spinner.succeed(fMsg);
    resultsTable.push(['Find', Array.from(arrayToFind).join(', ') + ` val=${vFind}`, fMsg]);

    // Merge
    const m1 = toInt32Array([1, 3, 5]);
    const m2 = toInt32Array([2, 4, 6]);
    spinner = ora(`Merging [${Array.from(m1)}] and [${Array.from(m2)}]`).start();
    const merged = await hpxaddon.merge(m1, m2);
    const mergedDisp = Array.from(merged);
    spinner.succeed(`Merged: [${mergedDisp}]`);
    resultsTable.push(['Merge', Array.from(m1).join(', ') + ' & ' + Array.from(m2).join(', '), mergedDisp.join(', ')]);

    // PartialSort
    const arrPartial = toInt32Array([9, 7, 5, 3, 1]);
    const midIdx = 3;
    spinner = ora(`Partial sort [${Array.from(arrPartial)}] up to index ${midIdx}`).start();
    const pSorted = await hpxaddon.partialSort(arrPartial, midIdx);
    const pSortedDisp = Array.from(pSorted);
    spinner.succeed(`Partial Sorted: [${pSortedDisp}]`);
    resultsTable.push(['PartialSort', Array.from(arrPartial).join(', ') + ` (middle=${midIdx})`, pSortedDisp.join(', ')]);

    // copyN
    const arrayToCopyN = toInt32Array([10,10,10,10,10,10,10,10]);
    const countN = 5;
    spinner = ora(`copyN from [${Array.from(arrayToCopyN)}] count=${countN}`).start();
    const copyNRes = await hpxaddon.copyN(arrayToCopyN, countN);
    const copyNDisp = Array.from(copyNRes);
    spinner.succeed(`copyN Result: [${copyNDisp}]`);
    resultsTable.push(['copyN', Array.from(arrayToCopyN).join(', ') + ` (count=${countN})`, copyNDisp.join(', ')]);

    // fill
    const fillArray = toInt32Array([0,0,0,0,0]);
    spinner = ora(`Filling [${Array.from(fillArray)}] with 7...`).start();
    const fillRes = await hpxaddon.fill(fillArray, 7); 
    const fillDisp = Array.from(fillRes);
    spinner.succeed(`Filled: [${fillDisp}]`);
    resultsTable.push(['fill', `0,0,0,0,0 val=7`, fillDisp.join(', ')]);

    // countIf: elements > 5
    // old: predCountIf = (val) => val > 5
    // new: predCountIf = elementPredicateToMask(val => val > 5)
    const arrayForCountIf = toInt32Array([1,2,3,4,5,6,7,8,9]);
    const predCountIf = elementPredicateToMask((val) => val > 5);
    spinner = ora(`countIf [${Array.from(arrayForCountIf)}] with pred=(val>5)`).start();
    const countIfRes = await hpxaddon.countIf(arrayForCountIf, predCountIf);
    spinner.succeed(`countIf Result: ${countIfRes}`);
    resultsTable.push(['countIf', `[${Array.from(arrayForCountIf)}] pred=(>5)`, countIfRes]);

    // copyIf: even numbers
    // old: predCopyIf = (val)=>val%2===0
    // new: batch predicate
    const arrayForCopyIf = toInt32Array([1,2,3,4,5,6,7,8,9]);
    const predCopyIf = elementPredicateToMask((val)=> val%2===0);
    spinner = ora(`copyIf [${Array.from(arrayForCopyIf)}] even?`).start();
    const copyIfRes = await hpxaddon.copyIf(arrayForCopyIf, predCopyIf);
    const copyIfDisp = Array.from(copyIfRes);
    spinner.succeed(`copyIf Result: [${copyIfDisp}]`);
    resultsTable.push(['copyIf', `[${Array.from(arrayForCopyIf)}] pred=(even)`, copyIfDisp.join(', ')]);

    // sortComp: sort descending
    // old: compDesc = (a,b)=> a>b
    // new: we must provide a key extractor from entire array
    // Use elementComparatorToKeys to convert a comparator into keys
    const arrayForSortComp = toInt32Array([10, 5, 8, 2, 9]);
    const compDesc = (a,b)=>a>b;
    const keyExtractorDesc = elementComparatorToKeys(compDesc);
    spinner = ora(`sortComp [${Array.from(arrayForSortComp)}] desc`).start();
    const sortCompRes = await hpxaddon.sortComp(arrayForSortComp, keyExtractorDesc);
    const sortCompDisp = Array.from(sortCompRes);
    spinner.succeed(`sortComp Result: [${sortCompDisp}]`);
    resultsTable.push(['sortComp', `[${Array.from(arrayForSortComp)}] comp=(desc)`, sortCompDisp.join(', ')]);

    // partialSortComp: partial sort descending up to middle=2
    const arrayForPartialComp = toInt32Array([5,4,3,2,1]);
    const midComp = 2;
    const compPartialDesc = (a,b) => a>b;
    const keyExtractorPartialDesc = elementComparatorToKeys(compPartialDesc);
    spinner = ora(`partialSortComp [${Array.from(arrayForPartialComp)}] middle=${midComp} desc`).start();
    const partialSortCompRes = await hpxaddon.partialSortComp(arrayForPartialComp, midComp, keyExtractorPartialDesc);
    const partialSortCompDisp = Array.from(partialSortCompRes);
    spinner.succeed(`partialSortComp Result: [${partialSortCompDisp}]`);
    resultsTable.push(['partialSortComp', `[5,4,3,2,1] middle=${midComp} desc`, partialSortCompDisp.join(', ')]);

    // Large parallel ops (just converting to Int32Array is fine)
    console.log(chalk.magenta('\nDemonstrating large parallel ops...'));
    const largeSize = 100000;
    const largeData = new Int32Array(largeSize);
    for (let i = 0; i < largeSize; i++) {
      largeData[i] = Math.floor(Math.random() * largeSize);
    }
    spinner = ora('Parallel ops on large data...').start();
    const t1 = Date.now();
    const [largeSorted, largeCount] = await Promise.all([
      hpxaddon.sort(largeData),
      hpxaddon.count(largeData, 42)
    ]);
    const t2 = Date.now();
    spinner.succeed(`Completed in ${t2 - t1}ms`);
    resultsTable.push(['Large Parallel Ops', `Size=${largeSize}`, `sort & count done in ${t2 - t1}ms`]);

    // finalize
    spinner = ora('Finalizing HPX...').start();
    await hpxaddon.finalizeHPX();
    spinner.succeed('HPX finalized successfully.');

    console.log(chalk.blue('\n=== Summary of Operations ==='));
    console.log(resultsTable.toString());

    console.log(chalk.green('\nAll done!'));

  } catch (error) {
    console.error(chalk.red('An error occurred:'), error);
  }
}

main();
