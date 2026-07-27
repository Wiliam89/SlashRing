#pragma once
#include <JuceHeader.h>

class CabinetMixer final
{
public:
    CabinetMixer();
    ~CabinetMixer() = default;

    void prepare (const juce::dsp::ProcessSpec& spec);
    void setMixerParams (float blend, float pan, float width, float outputDb) noexcept;
    void process (const juce::dsp::AudioBlock<float>& blockA, 
                  const juce::dsp::AudioBlock<float>& blockB, 
                  juce::dsp::AudioBlock<float>& outputBlock) noexcept;
    void reset();

private:
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> blendLinear;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> panLeft;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> panRight;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> gainLinear;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CabinetMixer)
};