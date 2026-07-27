#include "IRCache.h"

IRCache& IRCache::getInstance() noexcept
{
    static IRCache instance;
    return instance;
}

CachedIRData::Ptr IRCache::get (const juce::String& hash) noexcept
{
    const juce::ScopedLock sl (lock);
    return cacheMap[hash];
}

void IRCache::store (const juce::String& hash, CachedIRData::Ptr data) noexcept
{
    const juce::ScopedLock sl (lock);
    if (data != nullptr)
        cacheMap.set (hash, data);
}

void IRCache::clear() noexcept
{
    const juce::ScopedLock sl (lock);
    cacheMap.clear();
}