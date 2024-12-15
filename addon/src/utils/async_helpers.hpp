#ifndef ASYNC_HELPERS_HPP
#define ASYNC_HELPERS_HPP

#include <napi.h>
#include <functional>
#include <memory>
#include <utility>
#include <string>

/**
 * @brief General template for AsyncWorkData.
 * 
 * Manages data for asynchronous work, including execution and completion callbacks.
 */
template <typename ResultType>
struct AsyncWorkData {
    Napi::Env env;
    Napi::Promise::Deferred deferred;
    std::function<void(ResultType&, std::string&)> executeFunc;
    std::function<void(Napi::Env, Napi::Promise::Deferred&, ResultType&, const std::string&)> completeFunc;
    ResultType result;
    std::string errorMsg;
    napi_async_work work;
};

// Specialization for void ResultType
template <>
struct AsyncWorkData<void> {
    Napi::Env env;
    Napi::Promise::Deferred deferred;
    // For void, no result parameter in executeFunc and completeFunc
    std::function<void(std::string&)> executeFunc;
    std::function<void(Napi::Env, Napi::Promise::Deferred&, const std::string&)> completeFunc;
    std::string errorMsg;
    napi_async_work work;
};

/**
 * @brief Queues asynchronous work with a result.
 * 
 * @tparam ResultType The type of the result produced by the async work.
 * @param env The Node-API environment.
 * @param execute The function to execute asynchronously.
 * @param complete The function to call upon completion.
 * @return Napi::Promise The promise representing the asynchronous operation.
 */
template <typename ResultType>
Napi::Promise QueueAsyncWork(
    Napi::Env env,
    std::function<void(ResultType&, std::string&)> execute,
    std::function<void(Napi::Env, Napi::Promise::Deferred&, ResultType&, const std::string&)> complete)
{
    // Create a unique_ptr to manage AsyncWorkData
    auto data = std::make_unique<AsyncWorkData<ResultType>>(
         AsyncWorkData<ResultType>{
            env,
            Napi::Promise::Deferred::New(env),
            execute,
            complete,
            ResultType(),
            std::string(),
            nullptr
         }
    );

    Napi::String resourceName = Napi::String::New(env, "HPXAsyncWork");

    napi_async_work work_handle;
    napi_status status = napi_create_async_work(
        env,
        nullptr,
        resourceName,
        // Execute callback: runs on a separate thread
        [](napi_env env, void* rawData) {
            auto* d = reinterpret_cast<AsyncWorkData<ResultType>*>(rawData);
            try {
                d->executeFunc(d->result, d->errorMsg);
            } catch (const std::exception& e) {
                d->errorMsg = e.what();
            } catch (...) {
                d->errorMsg = "Unknown exception in execute callback.";
            }
        },
        // Complete callback: runs on the main thread
        [](napi_env env, napi_status status, void* rawData) {
            Napi::Env napiEnv = Napi::Env(env);
            Napi::HandleScope scope(napiEnv);

            // Wrap the raw pointer back into a unique_ptr for automatic deletion
            std::unique_ptr<AsyncWorkData<ResultType>> d(reinterpret_cast<AsyncWorkData<ResultType>*>(rawData));

            if (d->errorMsg.empty()) {
                d->completeFunc(napiEnv, d->deferred, d->result, d->errorMsg);
            } else {
                d->deferred.Reject(Napi::String::New(napiEnv, d->errorMsg));
            }

            // Clean up the async work handle
            napi_delete_async_work(env, d->work);
            // unique_ptr automatically deletes 'd' when it goes out of scope
        },
        data.get(),    // Pass the raw pointer to the execute callback
        &work_handle   // Store the work handle
    );

    if (status != napi_ok) {
        // If creating async work fails, unique_ptr will delete 'data' when it goes out of scope
        throw Napi::Error::New(env, "Failed to create async work.");
    }

    // Assign the work handle to data
    data->work = work_handle;

    // Now queue the async work
    status = napi_queue_async_work(env, work_handle);
    if (status != napi_ok) {
        // If queuing fails, unique_ptr will delete 'data' when it goes out of scope
        napi_delete_async_work(env, work_handle);
        throw Napi::Error::New(env, "Failed to queue async work.");
    }

    // Release ownership of the unique_ptr as N-API now owns the data via rawData
    AsyncWorkData<ResultType>* rawData = data.release();

    // Return the promise to the JavaScript side
    return rawData->deferred.Promise();
}

/**
 * @brief Queues asynchronous work with no result (void).
 * 
 * @param env The N-API environment.
 * @param execute The function to execute asynchronously.
 * @param complete The function to call upon completion.
 * @return Napi::Promise The promise representing the asynchronous operation.
 */
inline Napi::Promise QueueAsyncWork(
    Napi::Env env,
    std::function<void(std::string&)> execute,
    std::function<void(Napi::Env, Napi::Promise::Deferred&, const std::string&)> complete)
{
    // Create a unique_ptr to manage AsyncWorkData<void>
    auto data = std::make_unique<AsyncWorkData<void>>(
        AsyncWorkData<void>{
            env,
            Napi::Promise::Deferred::New(env),
            execute,
            complete,
            std::string(),
            nullptr
        }
    );

    Napi::String resourceName = Napi::String::New(env, "HPXAsyncWork");

    napi_async_work work_handle;
    napi_status status = napi_create_async_work(
        env,
        nullptr,
        resourceName,
        // Execute callback: runs on a separate thread
        [](napi_env env, void* rawData) {
            auto* d = reinterpret_cast<AsyncWorkData<void>*>(rawData);
            try {
                d->executeFunc(d->errorMsg);
            } catch (const std::exception& e) {
                d->errorMsg = e.what();
            } catch (...) {
                d->errorMsg = "Unknown exception in execute callback.";
            }
        },
        // Complete callback: runs on the main thread
        [](napi_env env, napi_status status, void* rawData) {
            Napi::Env napiEnv = Napi::Env(env);
            Napi::HandleScope scope(napiEnv);

            // Wrap the raw pointer back into a unique_ptr for automatic deletion
            std::unique_ptr<AsyncWorkData<void>> d(reinterpret_cast<AsyncWorkData<void>*>(rawData));

            if (d->errorMsg.empty()) {
                d->completeFunc(napiEnv, d->deferred, d->errorMsg);
            } else {
                d->deferred.Reject(Napi::String::New(napiEnv, d->errorMsg));
            }

            // Clean up the async work handle
            napi_delete_async_work(env, d->work);
            // unique_ptr automatically deletes 'd' when it goes out of scope
        },
        data.get(),    // Pass the raw pointer to the execute callback
        &work_handle   // Store the work handle
    );

    if (status != napi_ok) {
        // If creating async work fails, unique_ptr will delete 'data' when it goes out of scope
        throw Napi::Error::New(env, "Failed to create async work.");
    }

    // Assign the work handle to data
    data->work = work_handle;

    // Now queue the async work
    status = napi_queue_async_work(env, work_handle);
    if (status != napi_ok) {
        // If queuing fails, unique_ptr will delete 'data' when it goes out of scope
        napi_delete_async_work(env, work_handle);
        throw Napi::Error::New(env, "Failed to queue async work.");
    }

    // Release ownership of the unique_ptr as N-API now owns the data via rawData
    AsyncWorkData<void>* rawData = data.release();

    // Return the promise to the JavaScript side
    return rawData->deferred.Promise();
}

#endif // ASYNC_HELPERS_HPP
