#ifndef HPX_MANAGER_HPP
#define HPX_MANAGER_HPP

#include <hpx/hpx.hpp>
#include <thread>
#include <future>
#include <mutex>
#include <vector>
#include <string>
#include <atomic>
#include <memory>
#include <napi.h>

// Forward declaration of hpx_main_handler
int hpx_main_handler(int argc, char** argv);

class HPXManager {
public:
    HPXManager();
    ~HPXManager();

    // Initialize HPX asynchronously
    std::future<int> InitHPX(int argc, const std::vector<std::string>& argv, const std::vector<std::string>& config = {});

    // Finalize HPX asynchronously
    std::future<int> FinalizeHPX();

    // Check if HPX is running
    bool IsRunning() const;

    // Method to set the running flag
    void SetRunning(bool running);

    // Method to resolve the initialization promise
    void ResolveInitPromise(int value);

    void WaitForFinalizeHPX();

private:
    // Thread to run HPX runtime
    std::thread hpx_thread_;

    // Promises for initialization and finalization
    std::shared_ptr<std::promise<int>> init_promise_;
    std::shared_ptr<std::promise<int>> finalize_promise_;

    // Mutex to protect shared resources
    std::mutex mutex_;

    // Atomic flag to indicate HPX state
    std::atomic<bool> running_;

    // Internal function to run HPX
    void RunHPX(int argc, const std::vector<std::string>& argv, const std::vector<std::string>& config);

    // Store argv copies to ensure their lifetime
    std::vector<std::string> argv_copies_;

    // Synchronization primitive to signal hpx_main_handler to finalize
    std::promise<void> finalize_signal_promise_;
    std::future<void> finalize_signal_future_;

    // Delete copy constructor and assignment operator
    HPXManager(const HPXManager&) = delete;
    HPXManager& operator=(const HPXManager&) = delete;
};

// Singleton accessor for HPXManager
HPXManager& getHPXManager();

// Function to reset the HPXManager singleton
void resetHPXManager();

#endif // HPX_MANAGER_HPP
