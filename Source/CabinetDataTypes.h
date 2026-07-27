#pragma once
#include <JuceHeader.h>

/**
   @class CachedIRData
   @brief Objeto com contagem de referência que armazena os buffers de áudio processados das IRs.
*/
class CachedIRData final : public juce::ReferenceCountedObject
{
public:
    using Ptr = juce::ReferenceCountedObjectPtr<CachedIRData>;

    CachedIRData (juce::AudioBuffer<float>&& bufferToUse, double sampleRateToUse, const juce::String& hashToUse)
        : audioBuffer (std::move (bufferToUse)), 
          sampleRate (sampleRateToUse), 
          hash (hashToUse)
    {
    }

    ~CachedIRData() override = default;

    const juce::AudioBuffer<float>& getBuffer() const noexcept { return audioBuffer; }
    double getSampleRate() const noexcept { return sampleRate; }
    const juce::String& getHash() const noexcept { return hash; }

private:
    juce::AudioBuffer<float> audioBuffer;
    double sampleRate;
    juce::String hash;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CachedIRData)
};