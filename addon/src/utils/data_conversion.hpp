#ifndef DATA_CONVERSION_HPP
#define DATA_CONVERSION_HPP

#include <napi.h>
#include <memory>
#include <vector>

std::string ToUpperCase(const std::string& str);
Napi::Int32Array GetInt32ArrayArgument(const Napi::CallbackInfo& info, size_t index);

// Functions that use an already-created TSFN
std::shared_ptr<std::vector<uint8_t>> GetPredicateMaskBatchUsingTSFN(const Napi::ThreadSafeFunction& tsfn, const int32_t* data, size_t length);
std::shared_ptr<std::vector<int32_t>> GetKeyArrayBatchUsingTSFN(const Napi::ThreadSafeFunction& tsfn, const int32_t* data, size_t length);

#endif // DATA_CONVERSION_HPP
