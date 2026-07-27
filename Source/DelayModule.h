#pragma once

#include <JuceHeader.h>

class DelayModule
{
public:
    DelayModule();
    ~DelayModule();

    //========================================================
    // LIFECYCLE
    //========================================================

    void prepare(
        double sampleRate,
        int samplesPerBlock,
        int numChannels);

    void reset();

    //========================================================
    // DSP
    //========================================================

    void process(juce::AudioBuffer<float>& buffer);

    //========================================================
    // PARAMETERS
    //========================================================

    void setDelayTime(float newDelayMs);
    void setFeedback(float newFeedback);
    void setMix(float newMix);

private:
    //========================================================
    // DELAY LINES (STEREO)
    //========================================================

    juce::dsp::DelayLine<
        float,
        juce::dsp::DelayLineInterpolationTypes::Linear
    > leftDelay{ 96000 };

    juce::dsp::DelayLine<
        float,
        juce::dsp::DelayLineInterpolationTypes::Linear
    > rightDelay{ 96000 };

    //========================================================
    // SMOOTHED PARAMETERS
    //========================================================

    juce::SmoothedValue<float,
        juce::ValueSmoothingTypes::Linear>
        delayTimeSmoothed;

    juce::SmoothedValue<float,
        juce::ValueSmoothingTypes::Linear>
        feedbackSmoothed;

    juce::SmoothedValue<float,
        juce::ValueSmoothingTypes::Linear>
        mixSmoothed;

    //========================================================
    // STATE
    //========================================================

    float targetDelayMs = 420.0f;
    float targetFeedback = 0.35f;
    float targetMix = 0.22f;

    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DelayModule)
};