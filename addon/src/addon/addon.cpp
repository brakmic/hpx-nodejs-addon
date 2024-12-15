#include "addon.hpp"
#include "hpx_wrapper.hpp"
#include "hpx_manager.hpp"
#include "hpx_config.hpp"
#include "async_helpers.hpp"
#include "data_conversion.hpp"
#include "tsfn_manager.hpp"
#include "log_macros.hpp"

#include <napi.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <memory>
#include <vector>
#include <string>
#include <atomic>
#include <condition_variable>
#include <cstring> // for memcpy

/**
 * @brief This file defines the Node.js exposed functions that interface with HPX-based algorithms.
 *
 * All of these functions follow a similar pattern:
 * 1. Extract and validate arguments from JavaScript (e.g., Int32Array or Numbers).
 * 2. Use QueueAsyncWork(...) to run HPX-based computations off the main thread.
 * 
 * QueueAsyncWork is crucial because:
 * - The HPX operations run asynchronously and may be long-running.
 * - We must not block the Node.js main event loop.
 * - QueueAsyncWork sets up a Napi::Promise that is resolved or rejected when the async work completes.
 * 
 * The general pattern:
 * - In the "execute" lambda, we call HPX functions (e.g., hpx_sort, hpx_count).
 * - In the "complete" lambda, we resolve or reject the promise with the result.
 * 
 * This approach ensures fully asynchronous, promise-based API for JS consumers.
 */

/**
 * @brief Initializes the HPX runtime with given configuration.
 *
 * Expects a config object with fields like 'executionPolicy', 'threadCount', etc.
 * This runs HPX initialization off the main thread. Once HPX is ready, we resolve
 * a Promise returning true. If initialization fails, we reject the Promise.
 *
 */
Napi::Value InitHPX(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsObject()) {
        Napi::TypeError::New(env, "Expected config object").ThrowAsJavaScriptException();
        return env.Null();
    }
    Napi::Object configObj = info[0].As<Napi::Object>();
    SetUserConfigFromNapiObject(configObj);

    std::vector<std::string> hpx_config_params;
    hpx_config_params.emplace_back("hpx.os_threads=" + std::to_string(GetUserConfig().threadCount));
    std::vector<std::string> argv_strings = { GetUserConfig().addonName };
    int argc = (int)argv_strings.size();

    return QueueAsyncWork<int>(
        env,
        [argc, argv_strings, hpx_config_params](int& res, std::string& err) {
            try {
                HPXManager& manager = getHPXManager();
                auto fut = manager.InitHPX(argc, argv_strings, hpx_config_params);
                int init_res = fut.get();
                if (init_res != 0) err = "Failed to init HPX.";
                res = init_res;
            } catch (const std::exception& e) { err = e.what(); }
        },
        [](Napi::Env env, Napi::Promise::Deferred& def, int& res, const std::string& err) {
            if (!err.empty()) def.Reject(Napi::String::New(env, err));
            else def.Resolve(Napi::Boolean::New(env, true));
        }
    );
}

/**
 * @brief Shuts down the HPX runtime.
 *
 * This is the counterpart to InitHPX. It ensures that the HPX runtime stops cleanly.
 * On success, returns a Promise resolved to true. On failure, rejects the Promise.
 * Also releases all registered ThreadSafeFunctions to ensure no callbacks remain.
 *
 */
Napi::Value FinalizeHPX(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    return QueueAsyncWork<int>(
        env,
        [](int& res, std::string& err) {
            HPXManager& manager = getHPXManager();
            try {
                auto fut = manager.FinalizeHPX();
                int fin_res = fut.get();
                if (fin_res != 0) err = "Failed to finalize HPX.";
                res = fin_res;
                resetHPXManager();
                TSFNManager::GetInstance().ReleaseAllTSFNs();
            } catch(const std::exception& e) { err = e.what(); }
        },
        [](Napi::Env env, Napi::Promise::Deferred& def, int& res, const std::string& err) {
            if (!err.empty()) def.Reject(Napi::String::New(env, err));
            else def.Resolve(Napi::Boolean::New(env, true));
        }
    );
}

/**
 * @brief Sorts the given Int32Array in ascending order using HPX (async & parallel).
 *
 * Extracts an Int32Array from JS, then calls hpx_sort.
 * The sorted result is returned as a new Int32Array via a resolved Promise.
 * 
 */
Napi::Value Sort(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    auto inputArr = GetInt32ArrayArgument(info, 0);
    const int32_t* dataPtr = inputArr.Data();
    size_t dataSize = inputArr.ElementLength();

    return QueueAsyncWork<std::shared_ptr<std::vector<int32_t>>>(
        env,
        [dataPtr, dataSize](std::shared_ptr<std::vector<int32_t>>& res, std::string& err) {
            try {
                auto fut = hpx_sort(dataPtr, dataSize);
                res = fut.get();
            } catch(const std::exception& e){ err = e.what(); }
        },
        [](Napi::Env env, Napi::Promise::Deferred& def, std::shared_ptr<std::vector<int32_t>>& res, const std::string& err){
            if (!err.empty()) def.Reject(Napi::String::New(env, err));
            else {
                Napi::Int32Array outArr = Napi::Int32Array::New(env, res->size());
                memcpy(outArr.Data(), res->data(), res->size() * sizeof(int32_t));
                def.Resolve(outArr);
            }
        }
    );
}

/**
 * @brief Counts how many elements in the Int32Array equal a given value.
 *
 * Takes an Int32Array and a value. Calls hpx_count. On completion, returns a Promise
 * resolved with the count (as a Number).
 *
 */
Napi::Value Count(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    auto inputArr = GetInt32ArrayArgument(info, 0);
    int32_t value = info[1].As<Napi::Number>().Int32Value();
    const int32_t* dataPtr = inputArr.Data(); 
    size_t dataSize = inputArr.ElementLength();

    return QueueAsyncWork<int64_t>(
        env,
        [dataPtr, dataSize, value](int64_t& res, std::string& err){
            try {
                auto fut = hpx_count(dataPtr, dataSize, value);
                res = fut.get();
            } catch(const std::exception& e){ err = e.what();}
        },
        [](Napi::Env env, Napi::Promise::Deferred& def, int64_t &res, const std::string& err) {
            if(!err.empty()) def.Reject(Napi::String::New(env,err));
            else def.Resolve(Napi::Number::New(env,(double)res));
        }
    );
}

/**
 * @brief Copies the entire Int32Array to a new Int32Array.
 *
 * Async copy operation that calls hpx_copy and returns a Promise
 * resolved with the copied data.
 *
 */
Napi::Value Copy(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    auto inputArr = GetInt32ArrayArgument(info, 0);
    const int32_t* dataPtr = inputArr.Data();
    size_t dataSize = inputArr.ElementLength();

    return QueueAsyncWork<std::shared_ptr<std::vector<int32_t>>>(
        env,
        [dataPtr, dataSize](std::shared_ptr<std::vector<int32_t>>& res, std::string& err){
            try{
                auto fut = hpx_copy(dataPtr, dataSize);
                res = fut.get();
            } catch(const std::exception& e){ err = e.what(); }
        },
        [](Napi::Env env, Napi::Promise::Deferred& def, std::shared_ptr<std::vector<int32_t>>& res, const std::string& err) {
            if(!err.empty()) def.Reject(Napi::String::New(env, err));
            else {
                Napi::Int32Array arr = Napi::Int32Array::New(env, res->size());
                memcpy(arr.Data(), res->data(), res->size()*sizeof(int32_t));
                def.Resolve(arr);
            }
        }
    );
}

/**
 * @brief Checks if one Int32Array (main) ends with another Int32Array (suffix).
 *
 * Uses hpx_ends_with. Returns a boolean indicating if 'main' ends with 'suffix'.
 *
 */
Napi::Value EndsWith(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    auto mainArr = info[0].As<Napi::Int32Array>();
    auto suffixArr = info[1].As<Napi::Int32Array>();
    int32_t* mainPtr = mainArr.Data(); size_t mainSize = mainArr.ElementLength();
    int32_t* suffixPtr = suffixArr.Data(); size_t suffixSize = suffixArr.ElementLength();

    return QueueAsyncWork<bool>(
        env,
        [mainPtr, mainSize, suffixPtr, suffixSize](bool &res, std::string &err){
            try {
                auto fut = hpx_ends_with(mainPtr, mainSize, suffixPtr, suffixSize);
                res = fut.get();
            } catch(const std::exception &e){ err = e.what();}
        },
        [](Napi::Env env, Napi::Promise::Deferred& def, bool &res, const std::string &err){
            if(!err.empty()) def.Reject(Napi::String::New(env, err));
            else def.Resolve(Napi::Boolean::New(env, res));
        }
    );
}

/**
 * @brief Checks if two Int32Arrays are equal element-wise.
 *
 * Uses hpx_equal. Returns a boolean Promise. True if same length and same elements, false otherwise.
 *
 * Function: Equal
 */
Napi::Value Equal(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    auto v1 = info[0].As<Napi::Int32Array>();
    auto v2 = info[1].As<Napi::Int32Array>();
    int32_t* v1Ptr = v1.Data(); size_t v1Size = v1.ElementLength();
    int32_t* v2Ptr = v2.Data(); size_t v2Size = v2.ElementLength();

    return QueueAsyncWork<bool>(
        env,
        [v1Ptr, v1Size, v2Ptr, v2Size](bool &res, std::string &err){
            try{
                auto fut = hpx_equal(v1Ptr, v1Size, v2Ptr, v2Size);
                res = fut.get();
            } catch(const std::exception &e){ err = e.what(); }
        },
        [](Napi::Env env, Napi::Promise::Deferred& def, bool &res, const std::string &err){
            if(!err.empty()) def.Reject(Napi::String::New(env, err));
            else def.Resolve(Napi::Boolean::New(env, res));
        }
    );
}

/**
 * @brief Finds the first occurrence of a given value in an Int32Array.
 *
 * Uses hpx_find. Returns the index as a Promise (Number). -1 if not found.
 *
 */
Napi::Value Find(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    auto arr = info[0].As<Napi::Int32Array>();
    int32_t value = info[1].As<Napi::Number>().Int32Value();
    int32_t* dataPtr = arr.Data();
    size_t dataSize = arr.ElementLength();

    return QueueAsyncWork<int64_t>(
        env,
        [dataPtr, dataSize, value](int64_t &res, std::string &err){
            try {
                auto fut = hpx_find(dataPtr, dataSize, value);
                res = fut.get();
            } catch(const std::exception &e){ err = e.what(); }
        },
        [](Napi::Env env, Napi::Promise::Deferred& def, int64_t &res, const std::string &err){
            if(!err.empty()) def.Reject(Napi::String::New(env, err));
            else def.Resolve(Napi::Number::New(env,(double)res));
        }
    );
}

/**
 * @brief Merges two sorted Int32Arrays into a single sorted Int32Array.
 *
 * Uses hpx_merge. Both input arrays must be pre-sorted. The result is a Promise resolved with the merged array.
 *
 */
Napi::Value Merge(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    auto v1 = GetInt32ArrayArgument(info,0);
    auto v2 = GetInt32ArrayArgument(info,1);
    const int32_t* v1Ptr = v1.Data(); size_t v1Size = v1.ElementLength();
    const int32_t* v2Ptr = v2.Data(); size_t v2Size = v2.ElementLength();

    return QueueAsyncWork<std::shared_ptr<std::vector<int32_t>>>(
        env,
        [v1Ptr, v1Size, v2Ptr, v2Size](std::shared_ptr<std::vector<int32_t>> &res, std::string &err){
            try{
                auto fut = hpx_merge(v1Ptr, v1Size, v2Ptr, v2Size);
                res = fut.get();
            }catch(const std::exception &e){ err = e.what();}
        },
        [](Napi::Env env, Napi::Promise::Deferred& def, std::shared_ptr<std::vector<int32_t>>& res, const std::string &err){
            if(!err.empty()) def.Reject(Napi::String::New(env,err));
            else {
                Napi::Int32Array arr = Napi::Int32Array::New(env, res->size());
                memcpy(arr.Data(), res->data(), res->size()*sizeof(int32_t));
                def.Resolve(arr);
            }
        }
    );
}

/**
 * @brief Partially sorts the array so that the first 'middle' elements are the smallest ones in ascending order.
 *
 * Uses hpx_partial_sort. Returns a Promise with the partially sorted array.
 *
 */
Napi::Value PartialSort(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    auto inputArr = GetInt32ArrayArgument(info,0);
    uint32_t middle = info[1].As<Napi::Number>().Uint32Value();
    const int32_t* dataPtr = inputArr.Data();
    size_t dataSize = inputArr.ElementLength();

    return QueueAsyncWork<std::shared_ptr<std::vector<int32_t>>>(
        env,
        [dataPtr, dataSize, middle](std::shared_ptr<std::vector<int32_t>>& res, std::string &err){
            try{
                auto fut = hpx_partial_sort(dataPtr, dataSize, middle);
                res = fut.get();
            }catch(const std::exception &e){ err = e.what();}
        },
        [](Napi::Env env, Napi::Promise::Deferred& def, std::shared_ptr<std::vector<int32_t>>& res, const std::string &err){
            if(!err.empty()) def.Reject(Napi::String::New(env,err));
            else {
                Napi::Int32Array arr = Napi::Int32Array::New(env, res->size());
                memcpy(arr.Data(), res->data(), res->size()*sizeof(int32_t));
                def.Resolve(arr);
            }
        }
    );
}

/**
 * @brief Copies the first 'count' elements of the array into a new array.
 *
 * Uses hpx_copy_n. If 'count' > size, it is truncated to size. Returns a Promise with the copied subset.
 *
 */
Napi::Value CopyN(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    auto inputArr = info[0].As<Napi::Int32Array>();
    size_t count = info[1].As<Napi::Number>().Uint32Value();
    int32_t* dataPtr = inputArr.Data();
    size_t dataSize = inputArr.ElementLength();
    if (count > dataSize) count = dataSize;

    return QueueAsyncWork<std::shared_ptr<std::vector<int32_t>>>(
        env,
        [dataPtr, count](std::shared_ptr<std::vector<int32_t>>& res, std::string &err){
            try {
                auto fut = hpx_copy_n(dataPtr, count);
                res = fut.get();
            } catch(const std::exception &e) {
                err = e.what();
            }
        },
        [](Napi::Env env, Napi::Promise::Deferred& def, std::shared_ptr<std::vector<int32_t>>& res, const std::string &err) {
            if(!err.empty()) {
                def.Reject(Napi::String::New(env, err));
            } else {
                // Final single copy to return the result as Int32Array
                Napi::Int32Array arr = Napi::Int32Array::New(env, res->size());
                memcpy(arr.Data(), res->data(), res->size()*sizeof(int32_t));
                def.Resolve(arr);
            }
        }
    );
}


/**
 * @brief Returns a new Int32Array filled with a specified value.
 *
 * Uses hpx_fill. The length is taken from the input array argument (just for sizing). Returns a Promise with the filled array.
 *
 */
Napi::Value Fill(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    // We only use the length from input array, not copying it
    auto inputArr = info[0].As<Napi::Int32Array>();
    size_t dataSize = inputArr.ElementLength();
    int32_t value = info[1].As<Napi::Number>().Int32Value();

    return QueueAsyncWork<std::shared_ptr<std::vector<int32_t>>>(
        env,
        [dataSize,value](std::shared_ptr<std::vector<int32_t>>& res, std::string &err){
            try {
                auto fut = hpx_fill(value, dataSize);
                res = fut.get();
            } catch(const std::exception& e){ err = e.what(); }
        },
        [](Napi::Env env, Napi::Promise::Deferred& def, std::shared_ptr<std::vector<int32_t>>& res, const std::string &err){
            if(!err.empty()) {
                def.Reject(Napi::String::New(env, err));
            } else {
                // Final single copy to return the result as Int32Array
                Napi::Int32Array arr = Napi::Int32Array::New(env, res->size());
                memcpy(arr.Data(), res->data(), res->size()*sizeof(int32_t));
                def.Resolve(arr);
            }
        }
    );
}

/**
 * @brief Counts how many elements satisfy a given JavaScript predicate.
 *
 * Uses a ThreadSafeFunction to call the JS predicate in batch mode.
 * 1 = element satisfies predicate, 0 = does not.
 * Returns a Promise with the count as a Number.
 *
 */
Napi::Value CountIf(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    auto inputArr = GetInt32ArrayArgument(info,0);
    Napi::Function fn = info[1].As<Napi::Function>();

    const int32_t* dataPtr = inputArr.Data();
    size_t dataSize = inputArr.ElementLength();

    auto tsfn = Napi::ThreadSafeFunction::New(env, fn, "BatchPredicate", 0, 1);
    auto tsfnPtr = std::make_shared<Napi::ThreadSafeFunction>(std::move(tsfn));

    return QueueAsyncWork<int64_t>(
        env,
        [dataPtr, dataSize, tsfnPtr](int64_t &res, std::string &err) {
            try {
                // Get mask from JS in one batch call
                auto mask = GetPredicateMaskBatchUsingTSFN(*tsfnPtr, dataPtr, dataSize); //std::shared_ptr<std::vector<uint8_t>>

                // Hold the mask in a shared_ptr to ensure lifetime
                auto maskHolder = std::make_shared<std::vector<uint8_t>>(*mask);

                // We need a predicate that reads from mask and increments index
                struct MaskData {
                    std::atomic<size_t> currentIndex{0};
                    std::shared_ptr<std::vector<uint8_t>> maskHolder;
                };
                auto maskData = std::make_shared<MaskData>();
                maskData->maskHolder = maskHolder;

                struct MaskPredicate {
                    std::shared_ptr<MaskData> data;
                    bool operator()(int32_t /*val*/) const {
                        size_t idx = data->currentIndex.fetch_add(1, std::memory_order_relaxed);
                        return (*data->maskHolder)[idx] == 1;
                    }
                };

                MaskPredicate pred{maskData};

                auto fut = hpx_count_if(dataPtr, dataSize, pred);
                res = fut.get();

            } catch(const std::exception& e){
                err = e.what();
            }
        },
        [tsfnPtr](Napi::Env env, Napi::Promise::Deferred& def, int64_t &res, const std::string &err) {
            tsfnPtr->Abort();
            if(!err.empty()) {
                def.Reject(Napi::String::New(env,err));
            } else {
                def.Resolve(Napi::Number::New(env,(double)res));
            }
        }
    );
}


/**
 * @brief Copies all elements that satisfy a given JavaScript predicate into a new array.
 *
 * Similar to CountIf, but returns all elements for which predicate is true.
 * Uses mask-based approach (GetPredicateMaskBatchUsingTSFN) and returns a Promise with the filtered array.
 *
 */
Napi::Value CopyIf(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    auto inputArr = GetInt32ArrayArgument(info,0);
    Napi::Function fn = info[1].As<Napi::Function>();
    const int32_t* dataPtr = inputArr.Data();
    size_t dataSize = inputArr.ElementLength();

    auto tsfn = Napi::ThreadSafeFunction::New(env, fn, "BatchPredicate", 0, 1);
    auto tsfnPtr = std::make_shared<Napi::ThreadSafeFunction>(std::move(tsfn));

    return QueueAsyncWork<std::shared_ptr<std::vector<int32_t>>>(
        env,
        [dataPtr, dataSize, tsfnPtr](std::shared_ptr<std::vector<int32_t>>& res, std::string &err) {
            try {
                // Get mask from JS in one batch call
                auto mask = GetPredicateMaskBatchUsingTSFN(*tsfnPtr, dataPtr, dataSize); //std::shared_ptr<std::vector<uint8_t>>

                // Store mask and index in a separate struct
                struct MaskData {
                    const uint8_t* maskData;
                    std::atomic<size_t> currentIndex{0};
                };

                // Allocate mask data on the heap so we can share it
                auto maskData = std::make_shared<MaskData>();
                auto maskVec = *mask; // copy the vector of uint8_t locally
                // Store maskVec in a shared_ptr so its lifetime is guaranteed
                auto maskHolder = std::make_shared<std::vector<uint8_t>>(std::move(maskVec));
                maskData->maskData = maskHolder->data();

                // Now define the predicate that references maskData
                struct MaskPredicate {
                    std::shared_ptr<MaskData> data;
                    std::shared_ptr<std::vector<uint8_t>> maskHolder; 
                    bool operator()(int32_t /*val*/) const {
                        size_t idx = data->currentIndex.fetch_add(1, std::memory_order_relaxed);
                        return (*maskHolder)[idx] == 1;
                    }
                };

                MaskPredicate pred{maskData, maskHolder};

                // Now we can use hpx_copy_if which uses run_with_policy + hpx::copy_if
                auto fut = hpx_copy_if(dataPtr, dataSize, pred);
                res = fut.get();

            } catch(const std::exception &e){
                err = e.what();
            }
        },
        [tsfnPtr](Napi::Env env, Napi::Promise::Deferred& def, std::shared_ptr<std::vector<int32_t>>& res, const std::string &err) {
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
                // Extract keys from JS
                auto keys = GetKeyArrayBatchUsingTSFN(*tsfnPtr, dataPtr, dataSize);

                // Build the initial data vector
                auto inputVec = std::make_shared<std::vector<int32_t>>(dataPtr, dataPtr + dataSize);

                // Build an index array to sort by keys
                std::vector<int32_t> idx(dataSize);
                for (int32_t i = 0; i < (int32_t)dataSize; i++) idx[i] = i;

                // Define a comparator for indexes that uses the keys vector
                auto comp = [keys](int32_t a, int32_t b) {
                    return (*keys)[(size_t)a] < (*keys)[(size_t)b];
                };

                // Sort indices by keys using hpx_sort_comp
                // hpx_sort_comp sorts an array of int32_t using a custom comparator
                auto fut = hpx_sort_comp(idx.data(), dataSize, comp);
                auto sortedIdx = fut.get(); // sortedIdx is the final sorted index array

                // Now rearrange the original data according to sortedIdx
                std::vector<int32_t> sortedData(dataSize);
                for (size_t i=0; i<dataSize; i++) {
                    sortedData[i] = (*inputVec)[(*sortedIdx)[i]];
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


/**
 * @brief Partially sorts the array using a JavaScript key extractor. Only the smallest 'middle' elements are guaranteed sorted.
 *
 * Similar approach to SortComp: one JS call to get keys, then hpx logic to partial sort.
 * Returns a Promise with partially sorted array.
 *
 */
Napi::Value PartialSortComp(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    auto inputArr = GetInt32ArrayArgument(info,0);
    uint32_t middle = info[1].As<Napi::Number>().Uint32Value();
    Napi::Function fn = info[2].As<Napi::Function>();
    const int32_t* dataPtr = inputArr.Data();
    size_t dataSize = inputArr.ElementLength();
    if (middle > dataSize) middle = dataSize;

    auto tsfn = Napi::ThreadSafeFunction::New(env, fn, "BatchKeyExtractor", 0, 1);
    auto tsfnPtr = std::make_shared<Napi::ThreadSafeFunction>(std::move(tsfn));

    return QueueAsyncWork<std::shared_ptr<std::vector<int32_t>>>(
        env,
        [dataPtr, dataSize, middle, tsfnPtr](std::shared_ptr<std::vector<int32_t>>& result, std::string &err){
            try {
                // Extract keys
                auto keys = GetKeyArrayBatchUsingTSFN(*tsfnPtr, dataPtr, dataSize);

                // Create input data vector
                auto inputVec = std::make_shared<std::vector<int32_t>>(dataPtr, dataPtr + dataSize);

                // Create index array
                std::vector<int32_t> idx(dataSize);
                for (int32_t i = 0; i < (int32_t)dataSize; i++) idx[i] = i;

                // Comparator based on keys
                auto comp = [keys](int32_t lhs, int32_t rhs) {
                    return (*keys)[(size_t)lhs] < (*keys)[(size_t)rhs];
                };

                // Use hpx_partial_sort_comp on the idx array using the comp
                auto fut = hpx_partial_sort_comp(idx.data(), dataSize, middle, comp);
                auto partiallySortedIdx = fut.get();

                // Reorder input data according to partiallySortedIdx
                std::vector<int32_t> outVec(dataSize);
                for (size_t i=0; i<dataSize; i++) {
                    outVec[i] = (*inputVec)[(*partiallySortedIdx)[i]];
                }
                result = std::make_shared<std::vector<int32_t>>(std::move(outVec));

            } catch(const std::exception &e){ err = e.what(); }
        },
        [tsfnPtr](Napi::Env env, Napi::Promise::Deferred& def, std::shared_ptr<std::vector<int32_t>>& out, const std::string &err){
            tsfnPtr->Abort();
            if(!err.empty()) def.Reject(Napi::String::New(env, err));
            else {
                Napi::Int32Array arr = Napi::Int32Array::New(env, out->size());
                memcpy(arr.Data(), out->data(), out->size()*sizeof(int32_t));
                def.Resolve(arr);
            }
        }
    );
}

Napi::Object InitAddon(Napi::Env env, Napi::Object exports) {
    exports.Set("initHPX", Napi::Function::New(env, InitHPX));
    exports.Set("finalizeHPX", Napi::Function::New(env, FinalizeHPX));
    exports.Set("sort", Napi::Function::New(env, Sort));
    exports.Set("count", Napi::Function::New(env, Count));
    exports.Set("copy", Napi::Function::New(env, Copy));
    exports.Set("endsWith", Napi::Function::New(env, EndsWith));
    exports.Set("equal", Napi::Function::New(env, Equal));
    exports.Set("find", Napi::Function::New(env, Find));
    exports.Set("merge", Napi::Function::New(env, Merge));
    exports.Set("partialSort", Napi::Function::New(env, PartialSort));
    exports.Set("copyN", Napi::Function::New(env, CopyN));
    exports.Set("fill", Napi::Function::New(env, Fill));
    exports.Set("countIf", Napi::Function::New(env, CountIf));
    exports.Set("copyIf", Napi::Function::New(env, CopyIf));
    exports.Set("sortComp", Napi::Function::New(env, SortComp));
    exports.Set("partialSortComp", Napi::Function::New(env, PartialSortComp));
    return exports;
}

NODE_API_MODULE(hpxaddon, InitAddon)
