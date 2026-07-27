#pragma once

#include "CabinetDataTypes.h"

class IRCache final
{
public:
    static IRCache& getInstance() noexcept;

    CachedIRData::Ptr get (const juce::String& hash) noexcept;
    void store (const juce::String& hash, CachedIRData::Ptr data) noexcept;
    void clear() noexcept;

private:
    IRCache() = default;
    ~IRCache() = default;

    juce::CriticalSection lock;
    juce::HashMap<juce::String, CachedIRData::Ptr> cacheMap;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IRCache)
};