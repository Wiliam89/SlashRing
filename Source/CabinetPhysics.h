/*
  ==============================================================================

    CabinetPhysics.h
    Created: 18 Jul 2026 6:27:48pm
    Author:  wilia

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class CabinetPhysics
{
public:
    CabinetPhysics() noexcept;
    void prepare (const juce::dsp::ProcessSpec& spec) noexcept;
    void setCabinetParams (float sizeNorm, float openBackAmount) noexcept;
    void process (juce::dsp::AudioBlock<float>& block) noexcept;
    void reset() noexcept;

private:
    double sampleRate = 44100.0;
    juce::LinearSmoothedValue<float> woodResonanceGain { 0.0f };
    juce::LinearSmoothedValue<float> airCompressionFeedback { 0.0f };
    
    std::vector<float> internalDelayBuffer;
    size_t delayIndex = 0;
    size_t maxDelaySamples = 0;

    juce::dsp::IIR::Filter<float> woodFilter;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CabinetPhysics)
};
