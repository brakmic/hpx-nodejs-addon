#include "hpx_wrapper.hpp"
#include "hpx_config.hpp"
#include "hpx_run_policy.hpp"
#include <hpx/hpx.hpp>
#include <algorithm>
#include <vector>
#include <memory>
#include <stdexcept>
#include <functional>

// https://hpx-docs.stellar-group.org/latest/html/libs/core/algorithms/api/sort.html
hpx::future<std::shared_ptr<std::vector<int32_t>>> hpx_sort(const int32_t* src, size_t size) {
    auto input = std::make_shared<std::vector<int32_t>>(src, src + size);
    return hpx::async([input]() {
        return run_with_policy([&](auto policy) {
            hpx::sort(policy, input->begin(), input->end());
            return input;
        }, input->size());
    });
}
// https://hpx-docs.stellar-group.org/latest/html/libs/core/algorithms/api/count.html
hpx::future<int64_t> hpx_count(const int32_t* src, size_t size, int32_t value) {
    auto input = std::make_shared<std::vector<int32_t>>(src, src + size);
    return hpx::async([input, value]() {
        return run_with_policy([&](auto policy) {
            return static_cast<int64_t>(hpx::count(policy, input->begin(), input->end(), value));
        }, input->size());
    });
}
// https://hpx-docs.stellar-group.org/latest/html/libs/core/algorithms/api/copy.html
hpx::future<std::shared_ptr<std::vector<int32_t>>> hpx_copy(const int32_t* src, size_t size) {
    auto input = std::make_shared<std::vector<int32_t>>(src, src + size);
    return hpx::async([input]() {
        return run_with_policy([&](auto policy) {
            auto output = std::make_shared<std::vector<int32_t>>(input->size());
            hpx::copy(policy, input->begin(), input->end(), output->begin());
            return output;
        }, input->size());
    });
}
// https://hpx-docs.stellar-group.org/latest/html/libs/core/algorithms/api/ends_with.html
hpx::future<bool> hpx_ends_with(const int32_t* src, size_t src_size, const int32_t* suffix, size_t suffix_size) {
    auto s1 = std::make_shared<std::vector<int32_t>>(src, src + src_size);
    auto s2 = std::make_shared<std::vector<int32_t>>(suffix, suffix + suffix_size);
    return hpx::async([s1, s2]() {
        return run_with_policy([&](auto policy) {
            return hpx::ends_with(policy, s1->begin(), s1->end(), s2->begin(), s2->end());
        }, s1->size());
    });
}
// https://hpx-docs.stellar-group.org/latest/html/libs/core/algorithms/api/equal.html
hpx::future<bool> hpx_equal(const int32_t* arr1, size_t size1, const int32_t* arr2, size_t size2) {
    auto v1 = std::make_shared<std::vector<int32_t>>(arr1, arr1 + size1);
    auto v2 = std::make_shared<std::vector<int32_t>>(arr2, arr2 + size2);
    size_t effective_size = std::min(v1->size(), v2->size());
    return hpx::async([v1, v2, effective_size]() {
        return run_with_policy([&](auto policy) {
            return hpx::equal(policy, v1->begin(), v1->end(), v2->begin(), v2->end());
        }, effective_size);
    });
}
// https://hpx-docs.stellar-group.org/latest/html/libs/core/algorithms/api/find.html
hpx::future<int64_t> hpx_find(const int32_t* src, size_t size, int32_t value) {
    auto input = std::make_shared<std::vector<int32_t>>(src, src + size);
    return hpx::async([input, value]() {
        return run_with_policy([&](auto policy) {
            auto it = hpx::find(policy, input->begin(), input->end(), value);
            if (it == input->end()) return static_cast<int64_t>(-1);
            return static_cast<int64_t>(std::distance(input->begin(), it));
        }, input->size());
    });
}
// https://hpx-docs.stellar-group.org/latest/html/libs/core/algorithms/api/merge.html
hpx::future<std::shared_ptr<std::vector<int32_t>>> hpx_merge(const int32_t* src1, size_t size1, const int32_t* src2, size_t size2) {
    auto v1 = std::make_shared<std::vector<int32_t>>(src1, src1 + size1);
    auto v2 = std::make_shared<std::vector<int32_t>>(src2, src2 + size2);

    size_t effective_size = v1->size() + v2->size();
    return hpx::async([v1, v2, effective_size]() {
        return run_with_policy([&](auto policy) {
            auto out = std::make_shared<std::vector<int32_t>>(v1->size() + v2->size());
            hpx::merge(policy, v1->begin(), v1->end(), v2->begin(), v2->end(), out->begin());
            return out;
        }, effective_size);
    });
}
// https://hpx-docs.stellar-group.org/latest/html/libs/core/algorithms/api/partial_sort.html
hpx::future<std::shared_ptr<std::vector<int32_t>>> hpx_partial_sort(const int32_t* src, size_t size, size_t middle) {
    if (middle > size) {
        return hpx::make_exceptional_future<std::shared_ptr<std::vector<int32_t>>>(std::runtime_error("'middle' index out of bounds"));
    }

    auto input = std::make_shared<std::vector<int32_t>>(src, src + size);
    return hpx::async([input, middle]() {
        return run_with_policy([&](auto policy) {
            hpx::partial_sort(policy, input->begin(), input->begin() + middle, input->end());
            return input;
        }, input->size());
    });
}
// https://hpx-docs.stellar-group.org/latest/html/libs/core/algorithms/api/copy.html
hpx::future<std::shared_ptr<std::vector<int32_t>>> hpx_copy_n(const int32_t* src, size_t count) {
    auto input = std::make_shared<std::vector<int32_t>>(src, src + count);
    return hpx::async([input,count]() {
        return run_with_policy([&](auto policy) {
            auto output = std::make_shared<std::vector<int32_t>>(count);
            hpx::copy_n(policy, input->begin(), count, output->begin());
            return output;
        }, count);
    });
}
// https://hpx-docs.stellar-group.org/latest/html/libs/core/algorithms/api/fill.html
hpx::future<std::shared_ptr<std::vector<int32_t>>> hpx_fill(int32_t value, size_t size) {
    return hpx::async([value,size]() {
        return run_with_policy([&](auto policy) {
            auto output = std::make_shared<std::vector<int32_t>>(size);
            hpx::fill(policy, output->begin(), output->end(), value);
            return output;
        }, size);
    });
}
// https://hpx-docs.stellar-group.org/latest/html/libs/core/algorithms/api/count.html
hpx::future<int64_t> hpx_count_if(const int32_t* src, size_t size, std::function<bool(int32_t)> pred) {
    auto input = std::make_shared<std::vector<int32_t>>(src, src + size);
    return hpx::async([input, pred, size]() {
        return run_with_policy([&](auto policy) {
            return (int64_t)hpx::count_if(policy, input->begin(), input->end(), pred);
        }, size);
    });
}
// https://hpx-docs.stellar-group.org/latest/html/libs/core/algorithms/api/copy.html
hpx::future<std::shared_ptr<std::vector<int32_t>>> hpx_copy_if(const int32_t* src, size_t size, std::function<bool(int32_t)> pred) {
    auto input = std::make_shared<std::vector<int32_t>>(src, src + size);
    return hpx::async([input, pred, size]() {
        return run_with_policy([&](auto policy) {
            std::vector<int32_t> temp(input->size());
            auto end_it = hpx::copy_if(policy, input->begin(), input->end(), temp.begin(), pred);
            temp.resize(std::distance(temp.begin(), end_it));
            return std::make_shared<std::vector<int32_t>>(std::move(temp));
        }, size);
    });
}
// https://hpx-docs.stellar-group.org/latest/html/libs/core/algorithms/api/sort.html
hpx::future<std::shared_ptr<std::vector<int32_t>>> hpx_sort_comp(const int32_t* src, size_t size, std::function<bool(int32_t,int32_t)> comp) {
    auto input = std::make_shared<std::vector<int32_t>>(src, src + size);
    return hpx::async([input, comp, size]() {
        return run_with_policy([&](auto policy) {
            hpx::sort(policy, input->begin(), input->end(), comp);
            return input;
        }, size);
    });
}
// https://hpx-docs.stellar-group.org/latest/html/libs/core/algorithms/api/partial_sort.html
hpx::future<std::shared_ptr<std::vector<int32_t>>> hpx_partial_sort_comp(const int32_t* src, size_t size, size_t middle, std::function<bool(int32_t,int32_t)> comp) {
    if (middle > size) middle = size;
    auto input = std::make_shared<std::vector<int32_t>>(src, src + size);
    return hpx::async([input, comp, size, middle]() {
        return run_with_policy([&](auto policy) {
            hpx::partial_sort(policy, input->begin(), input->begin() + middle, input->end(), comp);
            return input;
        }, size);
    });
}
