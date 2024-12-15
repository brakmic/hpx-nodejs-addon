#include "hpx_manager.hpp"
#include "hpx_config.hpp" 
#include "log_macros.hpp"
#include <hpx/hpx.hpp>
#include <hpx/hpx_start.hpp>
#include <iostream>
#include <mutex>
#include <memory>
#include <utility>

// Define the singleton instance and mutex at file scope (not sure if this is the best approach, but it works)
static std::unique_ptr<HPXManager> g_hpx_manager_instance = nullptr;
static std::mutex g_hpx_manager_mutex;

// Singleton accessor for HPXManager
HPXManager& getHPXManager() {
    std::lock_guard<std::mutex> lock(g_hpx_manager_mutex);
    if (!g_hpx_manager_instance) {
        g_hpx_manager_instance = std::make_unique<HPXManager>();
    }
    return *g_hpx_manager_instance;
}

// Function to reset the HPXManager singleton
void resetHPXManager() {
    std::lock_guard<std::mutex> lock(g_hpx_manager_mutex);
    g_hpx_manager_instance.reset();
    LOG_DEBUG("[HPXManager] resetHPXManager: Singleton instance has been reset.");
}


HPXManager::HPXManager() : running_(false), finalize_signal_future_(finalize_signal_promise_.get_future()) {
    LOG_DEBUG("[HPXManager] Constructor called.");
}

HPXManager::~HPXManager() {
    LOG_DEBUG("[HPXManager] Destructor called.");
    if (hpx_thread_.joinable()) {
        LOG_DEBUG("[HPXManager] Destructor: Finalizing HPX.");
        // Only finalize HPX if it's running
        if (IsRunning()) {
            LOG_DEBUG("[HPXManager] Destructor: HPX is running. Finalizing HPX.");
            try
            {
                FinalizeHPX().get();
            }
            catch(...)
            {
                LOG_ERROR("[HPXManager] Destructor: Exception during FinalizeHPX.");
            }
        } else {
            LOG_DEBUG("[HPXManager] Destructor: HPX is not running. Skipping finalization.");
        }
    }
}

// Initialize HPX asynchronously
std::future<int> HPXManager::InitHPX(int argc, const std::vector<std::string>& argv, const std::vector<std::string>& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_DEBUG("[HPXManager] InitHPX called with argc=" << argc);

    if (running_.load()) {
        LOG_ERROR("[HPXManager] InitHPX: HPX is already running.");
        std::promise<int> promise;
        promise.set_value(-1);
        return promise.get_future();
    }

    // Create a new promise for initialization
    init_promise_ = std::make_shared<std::promise<int>>();
    LOG_DEBUG("[HPXManager] InitHPX: Initialization promise created.");

    // Start HPX in a separate thread
    hpx_thread_ = std::thread(&HPXManager::RunHPX, this, argc, std::cref(argv), std::cref(config));
    LOG_DEBUG("[HPXManager] InitHPX: HPX thread started.");

    // Return the future associated with the initialization result
    return init_promise_->get_future();
}


// Finalize HPX asynchronously
std::future<int> HPXManager::FinalizeHPX() {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_DEBUG("[HPXManager] FinalizeHPX called.");
    if (!running_.load()) {
        LOG_ERROR("[HPXManager] FinalizeHPX: HPX is not running.");
        std::promise<int> promise;
        promise.set_value(-1);
        return promise.get_future();
    }

    if (finalize_promise_) {
        LOG_ERROR("[HPXManager] FinalizeHPX: Finalization already in progress.");
        return finalize_promise_->get_future();
    }

    try
    {
        finalize_promise_ = std::make_shared<std::promise<int>>();
        LOG_DEBUG("[HPXManager] FinalizeHPX: Finalization promise created.");

        // Signal the HPX thread to finalize via the separate promise
        finalize_signal_promise_.set_value();
        LOG_DEBUG("[HPXManager] FinalizeHPX: finalize_signal_promise_ set.");

        // Wait for the finalize_promise_ to be set by the HPX thread
        LOG_DEBUG("[HPXManager] FinalizeHPX: Waiting for finalize_promise_.");
        std::future<int> fut = finalize_promise_->get_future();
        fut.get(); // wait for the HPX thread to set finalize_promise_
    }
    catch(...)
    {
        if (finalize_promise_) {
            finalize_promise_->set_value(-1);
        }
        throw;
    }

    if (hpx_thread_.joinable()) {
        LOG_DEBUG("[HPXManager] FinalizeHPX: Joining HPX thread.");
        hpx_thread_.join();
        LOG_DEBUG("[HPXManager] FinalizeHPX: HPX thread joined successfully.");
    } else {
        LOG_ERROR("[HPXManager] FinalizeHPX: hpx_thread_ is not joinable.");
    }

    LOG_DEBUG("[HPXManager] FinalizeHPX: Finalize promise set to 0.");

    // Return the result
    std::promise<int> result_promise;
    result_promise.set_value(0);
    return result_promise.get_future();
}

// Check if HPX is running
bool HPXManager::IsRunning() const {
    return running_.load();
}

// Method to set the running flag
void HPXManager::SetRunning(bool running) {
    running_.store(running);
    LOG_DEBUG("[HPXManager] SetRunning: " << (running ? "true" : "false"));
}

// Method to resolve the initialization promise
void HPXManager::ResolveInitPromise(int value) {
    if (init_promise_) {
        init_promise_->set_value(value);
        LOG_DEBUG("[HPXManager] ResolveInitPromise: " << value);
    } else {
        LOG_ERROR("[HPXManager] ResolveInitPromise: init_promise_ is null.");
    }
}

// Wait for finalize signal
void HPXManager::WaitForFinalizeHPX() {
    LOG_DEBUG("[HPXManager] WaitForFinalizeHPX: Waiting for finalize signal.");
    finalize_signal_future_.get();
    LOG_DEBUG("[HPXManager] WaitForFinalizeHPX: Received finalize signal.");

    // Call hpx::finalize to stop the HPX runtime
    try {
        LOG_DEBUG("[HPXManager] WaitForFinalizeHPX: Calling hpx::finalize().");
        hpx::finalize();
        LOG_DEBUG("[HPXManager] WaitForFinalizeHPX: hpx::finalize() called successfully.");
    } catch (const std::exception& e) {
        LOG_ERROR("[HPXManager] WaitForFinalizeHPX: Exception during hpx::finalize(): " << e.what());
    } catch (...) {
        LOG_ERROR("[HPXManager] WaitForFinalizeHPX: Unknown exception during hpx::finalize().");
    }

    // Set the finalize promise
    if (finalize_promise_) {
        finalize_promise_->set_value(0);
        LOG_DEBUG("[HPXManager] WaitForFinalizeHPX: finalize_promise_ set to 0.");
    } else {
        LOG_ERROR("[HPXManager] WaitForFinalizeHPX: finalize_promise_ is null.");
    }
}

// Internal function to run HPX
void HPXManager::RunHPX(int argc, const std::vector<std::string>& argv, const std::vector<std::string>& config) {
    try {
        LOG_DEBUG("[HPXManager] RunHPX: Starting HPX runtime with argc=" << argc);
        LOG_DEBUG("[HPXManager] RunHPX: Configuration options:");
        for (const auto& opt : config) {
            LOG_DEBUG("  " << opt);
        }

        hpx::init_params params;
        params.cfg = config;

        // Make a copy of argv strings to ensure they remain valid
        argv_copies_.reserve(argc);
        for (int i = 0; i < argc; ++i) {
            argv_copies_.emplace_back(argv[i]);
            LOG_DEBUG("[HPXManager] RunHPX: argv_copies[" << i << "] = " << argv_copies_[i]);
        }

        // Create char* vector with copies
        std::vector<char*> argv_copy_ptrs(argc + 1, nullptr);
        for (int i = 0; i < argc; ++i) {
            argv_copy_ptrs[i] = const_cast<char*>(argv_copies_[i].c_str());
            LOG_DEBUG("[HPXManager] RunHPX: argv_copy_ptrs[" << i << "] points to '" << argv_copy_ptrs[i] << "'");
        }
        argv_copy_ptrs[argc] = nullptr;

        LOG_DEBUG("[HPXManager] RunHPX: Calling hpx::start.");
        // Start HPX runtime
        bool start_success = hpx::start(hpx_main_handler, argc, argv_copy_ptrs.data(), params);
        if (!start_success) {
            LOG_ERROR("[HPXManager] RunHPX: HPX failed to start.");
            if (init_promise_) {
                init_promise_->set_value(-1);
            }
            return;
        }

        LOG_DEBUG("[HPXManager] RunHPX: HPX started successfully.");
        LOG_DEBUG("[HPXManager] RunHPX: hpx::start is blocking.");
    } catch (const hpx::exception& e) {
        LOG_ERROR("[HPXManager] RunHPX: HPX exception during runtime: " << e.what());
        if (init_promise_) {
            init_promise_->set_value(-1);
        }
    } catch (const std::exception& e) {
        LOG_ERROR("[HPXManager] RunHPX: Standard exception during runtime: " << e.what());
        if (init_promise_) {
            init_promise_->set_value(-1);
        }
    } catch (...) {
        LOG_ERROR("[HPXManager] RunHPX: Unknown exception during runtime.");
        if (init_promise_) {
            init_promise_->set_value(-1);
        }
    }
}

// hpx_main_handler implementation
int hpx_main_handler(int argc, char** argv) {
    LOG_DEBUG("[HPX] hpx_main_handler: Invoked.");

    // Log argv contents
    for (int i = 0; i < argc; ++i) {
        LOG_DEBUG("[HPX] hpx_main_handler: argv[" << i << "] = " << argv[i]);
    }

    try {
        LOG_DEBUG("[HPX] hpx_main_handler: HPX main handler started.");

        HPXManager& manager = getHPXManager();
        
        manager.SetRunning(true);
        manager.ResolveInitPromise(0);
        LOG_DEBUG("[HPX] hpx_main_handler: Running flag set to true.");

        LOG_DEBUG("[HPX] hpx_main_handler: Waiting for finalize signal.");
        // Wait for the finalize signal from FinalizeHPX
        manager.WaitForFinalizeHPX();
        LOG_DEBUG("[HPX] hpx_main_handler: Received finalize signal. Finalizing HPX.");

        manager.SetRunning(false);
        LOG_DEBUG("[HPX] hpx_main_handler: Exiting hpx_main_handler.");

        return 0;
    } catch (const std::exception& e) {
        LOG_ERROR("[HPX] hpx_main_handler: Exception: " << e.what());
        return 1;
    } catch (...) {
        LOG_ERROR("[HPX] hpx_main_handler: Unknown exception.");
        return 1;
    }
}
