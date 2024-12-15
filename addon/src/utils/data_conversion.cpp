#include "data_conversion.hpp"
#include <napi.h>
#include <vector>
#include <atomic>
#include <thread>
#include <memory>
#include <algorithm>
#include <cstring>

/**
 * @brief Converts a given string to uppercase characters.
 *
 * This is a simple utility function used, for example, when normalizing logging levels.
 *
 * @param str The input string.
 * @return A new string with all characters converted to uppercase.
 */
std::string ToUpperCase(const std::string& str) {
    std::string upper_str = str;
    std::transform(
        upper_str.begin(), upper_str.end(),
        upper_str.begin(),
        [](unsigned char c) { return std::toupper(c); }
    );
    return upper_str;
}

/**
 * @brief Extracts and validates an Int32Array argument from a JavaScript call.
 *
 * When the addon’s exposed functions expect an Int32Array (for performance reasons),
 * this function checks if the argument at the specified index is indeed an Int32Array.
 * If not, it throws a JavaScript TypeError. If successful, it returns a Napi::Int32Array
 * that can be used to access data pointers directly.
 *
 * @param info Napi callback info, providing access to arguments.
 * @param index The zero-based index of the argument to extract.
 * @return A Napi::Int32Array representing the extracted argument.
 * @throws If the argument is missing or not an Int32Array, a JS TypeError is thrown.
 */
Napi::Int32Array GetInt32ArrayArgument(const Napi::CallbackInfo& info, size_t index) {
    if (info.Length() <= index || !info[index].IsTypedArray()) {
        Napi::TypeError::New(info.Env(), "Expected an Int32Array at argument " + std::to_string(index)).ThrowAsJavaScriptException();
        return Napi::Int32Array();
    }
    auto ta = info[index].As<Napi::TypedArray>();
    if (ta.TypedArrayType() != napi_int32_array) {
        Napi::TypeError::New(info.Env(), "Expected Int32Array").ThrowAsJavaScriptException();
        return Napi::Int32Array();
    }
    return ta.As<Napi::Int32Array>();
}

/**
 * @brief Retrieves a predicate mask array from a JavaScript callback using a ThreadSafeFunction.
 *
 * Some operations (like countIf, copyIf) need to evaluate a user-provided JS predicate against an entire array.
 * Instead of calling the predicate per element, we do a "batch" call: we send the entire array once to JS, and expect
 * a mask (Uint8Array) of the same length back, where each element is either 1 (predicate true) or 0 (predicate false).
 *
 * Steps:
 * - We copy the input C++ array into a std::vector<int32_t> (dataCopy).
 * - We invoke the JS function once via NonBlockingCall, passing the entire array as an Int32Array.
 * - The JS predicate must return a Uint8Array of the same length.
 * - We then copy that Uint8Array into a std::vector<uint8_t> 'mask', which is returned to C++ code.
 *
 * This function blocks until the NonBlockingCall completes by busy-waiting on an atomic<bool>. Although not ideal,
 * this is simple and acceptable for this scenario, as it's done in a separate worker thread anyway.
 *
 * @param tsfn A Napi::ThreadSafeFunction representing the JS predicate callback.
 * @param data Pointer to the input int32_t array.
 * @param length Number of elements in 'data'.
 * @return A shared_ptr to a std::vector<uint8_t> containing the mask. Each element corresponds to the predicate result for that index.
 * @throws std::runtime_error if the JS callback returns something invalid or if NonBlockingCall fails.
 */
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
