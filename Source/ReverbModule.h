#pragma once

#include <JuceHeader.h>

class ReverbModule
{
public:
    ReverbModule();
    ~ReverbModule();

    void prepare(
        double sampleRate,
        int samplesPerBlock,
        int numChannels);

    void setMix(float newMix);

    void process(juce::AudioBuffer<float>& buffer);

private:
    float mix = 0.18f;

    juce::dsp::Reverb reverb;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReverbModule)
};