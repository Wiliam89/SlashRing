#pragma once

#include <JuceHeader.h>

class AmpModule
{
public:
    AmpModule();
    ~AmpModule();

    void prepare(
        double sampleRate,
        int samplesPerBlock,
        int numChannels);

    void process(juce::dsp::AudioBlock<float>& block);

    //========================================================
   // LIFECYCLE (TD-004)
   //========================================================

    void reset();
    void release();

    //========================================================
    // PARAMETERS
    //========================================================

    void setGain(float value);
    void setBass(float value);
    void setMiddle(float value);
    void setTreble(float value);
    void setPresence(float value);
    void setMaster(float value);

    //========================================================
    // TD-010 GAIN MAP B — TEMPORARY DIAGNOSTIC
    //
    // Read-only, observational only. Returns the CURRENT
    // (non-advancing) value of the internal preamp gain
    // smoother at the moment of query, via getCurrentValue().
    //
    // This does NOT represent every sample-by-sample value
    // consumed during the block being measured — only the
    // smoother's state at the instant this is called. It
    // does NOT call getNextValue() and does NOT advance or
    // otherwise modify smoother state.
    //
    // Diagnostic-only. Remove after TD-010 audit concludes.
    //========================================================
    float getCurrentGainValue() const noexcept
    {
        return gainSmoothed.getCurrentValue();
    }

private:
    //========================================================
    // FILTERS
    //========================================================

    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> inputHPF;

    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> bassFilter;

    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> midFilter;

    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> trebleFilter;

    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> presenceShelf;

    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> outputLPF;

    //========================================================
    // SMOOTHING
    //========================================================

    juce::SmoothedValue<float> gainSmoothed;
    juce::SmoothedValue<float> masterSmoothed;

    //========================================================
    // STATE
    //========================================================

    float gain = 5.0f;
    float bass = 5.0f;
    float middle = 5.0f;
    float treble = 5.0f;
    float presence = 5.0f;
    float master = 6.0f;

    double currentSampleRate = 44100.0;

    float currentDriveGain = 1.0f;
    float currentMasterGain = 1.0f;

    //========================================================
    // INTERNAL
    //========================================================

    void updateToneStack();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AmpModule)
};