#ifndef HPX_RUN_POLICY_HPP
#define HPX_RUN_POLICY_HPP

#include "hpx_config.hpp"
#include <hpx/hpx.hpp>
#include <utility>
#include <cstddef>

template <typename F>
auto run_with_policy(F&& f, size_t size) {
    const auto& cfg = GetUserConfig();
    bool useParallel = (size >= cfg.threshold);

    if (!useParallel) {
        return std::forward<F>(f)(hpx::execution::seq);
    }

    if (cfg.executionPolicy == "seq") {
        return std::forward<F>(f)(hpx::execution::seq);
    } else if (cfg.executionPolicy == "par_unseq") {
        return std::forward<F>(f)(hpx::execution::par_unseq);
    } else {
        return std::forward<F>(f)(hpx::execution::par);
    }
}

#endif // HPX_RUN_POLICY_HPP
