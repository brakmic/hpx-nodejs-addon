#ifndef ADDON_HPP
#define ADDON_HPP

#include <napi.h>

// Exported HPX-related functions
Napi::Value InitHPX(const Napi::CallbackInfo& info);
Napi::Value FinalizeHPX(const Napi::CallbackInfo& info);

// Algorithmic functions
Napi::Value Sort(const Napi::CallbackInfo& info);
Napi::Value Count(const Napi::CallbackInfo& info);
Napi::Value Copy(const Napi::CallbackInfo& info);
Napi::Value EndsWith(const Napi::CallbackInfo& info);
Napi::Value Equal(const Napi::CallbackInfo& info);
Napi::Value Find(const Napi::CallbackInfo& info);
Napi::Value Merge(const Napi::CallbackInfo& info);
Napi::Value PartialSort(const Napi::CallbackInfo& info);
Napi::Value CopyN(const Napi::CallbackInfo& info);
Napi::Value Fill(const Napi::CallbackInfo& info);
Napi::Value CountIf(const Napi::CallbackInfo& info);
Napi::Value CopyIf(const Napi::CallbackInfo& info);
Napi::Value SortComp(const Napi::CallbackInfo& info);
Napi::Value PartialSortComp(const Napi::CallbackInfo& info);

// Initialization of the addon
Napi::Object InitAddon(Napi::Env env, Napi::Object exports);

#endif // ADDON_HPP
