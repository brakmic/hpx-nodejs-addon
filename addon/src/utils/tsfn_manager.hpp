#ifndef TSFN_MANAGER_HPP
#define TSFN_MANAGER_HPP

#include <napi.h>
#include <vector>
#include <memory>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>

/**
 * @brief Singleton class to manage all ThreadSafeFunction instances.
 */
class TSFNManager {
public:
    // Singleton access
    static TSFNManager& GetInstance();

    // Register a new ThreadSafeFunction
    void RegisterTSFN(const std::shared_ptr<Napi::ThreadSafeFunction>& tsfn);

    // Release all registered ThreadSafeFunctions
    void ReleaseAllTSFNs();

    // Check if the manager is currently releasing TSFNs
    bool IsReleasing();

    // Wait until all TSFNs have been released
    void WaitForReleaseCompletion();

    // Destructor
    ~TSFNManager();

private:
    // Private constructor for Singleton pattern
    TSFNManager() : releasing_(false), release_completed_(false) {}

    // Disable copy and assignment
    TSFNManager(const TSFNManager&) = delete;
    TSFNManager& operator=(const TSFNManager&) = delete;

    // Mutex to protect access to tsfn_list_
    std::mutex tsfn_mutex_;

    // List of registered ThreadSafeFunctions
    std::vector<std::shared_ptr<Napi::ThreadSafeFunction>> tsfn_list_;

    // Flag indicating whether TSFNs are being released
    bool releasing_;

    // Thread and synchronization for release
    std::thread release_thread_;
    std::mutex release_mutex_;
    std::condition_variable release_cv_;
    std::atomic<bool> release_completed_;
};

#endif // TSFN_MANAGER_HPP
