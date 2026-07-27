#pragma once

#include <JuceHeader.h>

class OutputStage
{
public:
    OutputStage();
    ~OutputStage();

    void prepare(
        double sampleRate,
        int samplesPerBlock,
        int numChannels);

    void process(juce::AudioBuffer<float>& buffer);

    void setOutputGain(float newGain);
     //========================================================
     // TD-004 LIFECYCLE
    //========================================================
    void reset();
    void release();

private:
    //========================================================
    // OUTPUT GAIN (SMOOTHED)
    //========================================================

    juce::SmoothedValue<float,
        juce::ValueSmoothingTypes::Linear> outputGainSmoothed;

    float targetOutputGain = 1.0f;

    //========================================================
    // SAFETY LIMITER
    //========================================================

    juce::dsp::Limiter<float> limiter;

    //========================================================
    // STATE
    //========================================================

    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutputStage)
};