#pragma once

#include <JuceHeader.h>

class InputStage
{
public:
    InputStage() = default;
    ~InputStage() = default;

    void prepare(double sampleRate,
        int samplesPerBlock,
        int numChannels);

    void process(juce::AudioBuffer<float>& buffer);

    void setInputGain(float newGain);
    void setInputType(int newType);

    //========================================================
    // LIFECYCLE (TD-004)
    //========================================================
    void reset();
    void release();

private:
    //========================================================
    // PARAMETERS (SMOOTHED)
    //========================================================

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> gainSmoothed;

    //========================================================
    // FILTER (DC / RUMBLE CONTROL)
    //========================================================

    juce::dsp::ProcessorDuplicator<
        juce::dsp::IIR::Filter<float>,
        juce::dsp::IIR::Coefficients<float>
    > highPassFilter;

    juce::dsp::ProcessorDuplicator<
        juce::dsp::IIR::Filter<float>,
        juce::dsp::IIR::Coefficients<float>
    > voicingFilter;

    //========================================================
    // STATE
    //========================================================

    float targetGain = 1.0f;

    enum class PickupType : int
    {
        SingleCoil = 0,
        P90,
        HumbuckerVintage,
        HumbuckerModern,
        Active
    };

    PickupType inputType =
        PickupType::HumbuckerVintage;

    double currentSampleRate = 44100.0;

    float calibrationGain = 1.0f;

    //========================================================
    // INTERNAL
    //========================================================

    float applyInputVoicing(float x) noexcept;
    float getPickupCalibration() const noexcept;
    void updateVoicingFilter();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InputStage)
};