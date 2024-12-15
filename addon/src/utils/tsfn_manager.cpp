#include "tsfn_manager.hpp"
#include "log_macros.hpp"
#include <iostream>
#include <thread>

TSFNManager& TSFNManager::GetInstance() {
    static TSFNManager instance;
    return instance;
}

void TSFNManager::RegisterTSFN(const std::shared_ptr<Napi::ThreadSafeFunction>& tsfn) {
    std::lock_guard<std::mutex> lock(tsfn_mutex_);
    if (releasing_) {
        LOG_DEBUG("[HPX] Cannot register TSFN; releasing is in progress.");
        return;
    }
    LOG_DEBUG("[HPX] Registering TSFN. Current count: " << tsfn_list_.size());
    tsfn_list_.emplace_back(tsfn);
}

void TSFNManager::ReleaseAllTSFNs() {
    {
        std::lock_guard<std::mutex> lock(tsfn_mutex_);
        if (releasing_) {
            LOG_DEBUG("[HPX] TSFNs are already being released.");
            return;
        }
        releasing_ = true;
        LOG_DEBUG("[HPX] Initiating release of all TSFNs. Count: " << tsfn_list_.size());
    }

    // Launch a dedicated thread to handle TSFN releases
    release_thread_ = std::thread([this]() {
        std::vector<std::shared_ptr<Napi::ThreadSafeFunction>> tsfn_copy;

        {
            std::lock_guard<std::mutex> lock(tsfn_mutex_);
            tsfn_copy = tsfn_list_;
            tsfn_list_.clear();
        }

        for(auto& tsfn : tsfn_copy) {
            if(tsfn) {
                tsfn->Release();
                LOG_DEBUG("[HPX] TSFN release initiated.");
            }
        }

        LOG_DEBUG("[HPX] All TSFNs have been instructed to release.");

        // Signal that release is complete
        {
            std::lock_guard<std::mutex> lock(release_mutex_);
            release_completed_ = true;
        }
        release_cv_.notify_all();
    });
}

bool TSFNManager::IsReleasing() {
    std::lock_guard<std::mutex> lock(tsfn_mutex_);
    return releasing_;
}

void TSFNManager::WaitForReleaseCompletion() {
    std::unique_lock<std::mutex> lock(release_mutex_);
    release_cv_.wait(lock, [this]() { return release_completed_.load(); });
}

TSFNManager::~TSFNManager() {
    if (release_thread_.joinable()) {
        release_thread_.join();
    }
}
