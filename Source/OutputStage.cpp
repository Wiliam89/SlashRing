#include "OutputStage.h"

//============================================================
// CONSTRUCTOR
//============================================================

OutputStage::OutputStage()
{
}

OutputStage::~OutputStage()
{
}

//============================================================
// PREPARE
//============================================================

void OutputStage::prepare(
    double sampleRate,
    int samplesPerBlock,
    int numChannels)
{
    currentSampleRate = sampleRate;

    //========================================================
    // OUTPUT GAIN SMOOTHING
    //========================================================

    outputGainSmoothed.reset(sampleRate, 0.02);
    outputGainSmoothed.setCurrentAndTargetValue(
        targetOutputGain);

    //========================================================
    // LIMITER
    //========================================================

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize =
        static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels =
        static_cast<juce::uint32>(numChannels);

    limiter.reset();
    limiter.prepare(spec);

    limiter.setThreshold(-0.5f);
    limiter.setRelease(50.0f);

    //========================================================
    // TD-004 LIFECYCLE COMPLETION INITIALIZATION
    //========================================================
    reset();
}

//============================================================
// LIFECYCLE IMPLEMENTATION (TD-004)
//============================================================

void OutputStage::reset()
{
    limiter.reset();

    outputGainSmoothed.setCurrentAndTargetValue(
        targetOutputGain);
}

void OutputStage::release()
{
    // No resources currently require explicit release.
}

//============================================================
// PROCESS
//============================================================

void OutputStage::process(
    juce::AudioBuffer<float>& buffer)
{
    juce::ScopedNoDenormals noDenormals;

    const int numChannels =
        buffer.getNumChannels();

    const int numSamples =
        buffer.getNumSamples();

    //========================================================
    // APPLY OUTPUT GAIN
    //========================================================

    for (int sample = 0;
        sample < numSamples;
        ++sample)
    {
        const float currentGain =
            outputGainSmoothed.getNextValue();

        for (int channel = 0;
            channel < numChannels;
            ++channel)
        {
            auto* samples =
                buffer.getWritePointer(channel);

            samples[sample] *= currentGain;
        }
    }

    //========================================================
    // LIMITER
    //========================================================

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float>
        context(block);

    limiter.process(context);
}

//============================================================
// PARAMETER
//============================================================

void OutputStage::setOutputGain(float newGain)
{
    targetOutputGain =
        juce::jlimit(0.0f, 2.0f, newGain);

    outputGainSmoothed.setTargetValue(
        targetOutputGain);
}