#include "DelayModule.h"

//============================================================
// CONSTRUCTOR
//============================================================

DelayModule::DelayModule()
{
}

DelayModule::~DelayModule()
{
}

//============================================================
// PREPARE
//============================================================

void DelayModule::prepare(
    double sampleRate,
    int samplesPerBlock,
    int numChannels)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate =
        sampleRate;

    spec.maximumBlockSize =
        static_cast<juce::uint32>(
            samplesPerBlock);

    spec.numChannels =
        static_cast<juce::uint32>(
            numChannels);

    leftDelay.prepare(spec);
    rightDelay.prepare(spec);

    leftDelay.setMaximumDelayInSamples(
        static_cast<int>(
            sampleRate * 2.5));

    rightDelay.setMaximumDelayInSamples(
        static_cast<int>(
            sampleRate * 2.5));

    delayTimeSmoothed.reset(
        sampleRate,
        0.02);

    feedbackSmoothed.reset(
        sampleRate,
        0.02);

    mixSmoothed.reset(
        sampleRate,
        0.02);

    delayTimeSmoothed
        .setCurrentAndTargetValue(
            targetDelayMs);

    feedbackSmoothed
        .setCurrentAndTargetValue(
            targetFeedback);

    mixSmoothed
        .setCurrentAndTargetValue(
            targetMix);

    reset();
}

//============================================================
// RESET
//============================================================

void DelayModule::reset()
{
    leftDelay.reset();
    rightDelay.reset();
}

//============================================================
// PROCESS
//============================================================

void DelayModule::process(
    juce::AudioBuffer<float>& buffer)
{
    juce::ScopedNoDenormals noDenormals;

    const int numChannels =
        buffer.getNumChannels();

    const int numSamples =
        buffer.getNumSamples();

    for (int sample = 0;
        sample < numSamples;
        ++sample)
    {
        const float delayMs =
            delayTimeSmoothed.getNextValue();

        const float feedback =
            feedbackSmoothed.getNextValue();

        const float mix =
            mixSmoothed.getNextValue();

        const float delaySamples =
            (delayMs / 1000.0f)
            * static_cast<float>(
                currentSampleRate);

        leftDelay.setDelay(delaySamples);
        rightDelay.setDelay(delaySamples);

        for (int channel = 0;
            channel < numChannels;
            ++channel)
        {
            auto* samples =
                buffer.getWritePointer(channel);

            const float input =
                samples[sample];

            const float delayed =
                (channel == 0)
                ? leftDelay.popSample(0)
                : rightDelay.popSample(0);

            const float feedbackInput =
                input + (delayed * feedback);

            if (channel == 0)
                leftDelay.pushSample(
                    0,
                    feedbackInput);
            else
                rightDelay.pushSample(
                    0,
                    feedbackInput);

            samples[sample] =
                (input * (1.0f - mix))
                + (delayed * mix);
        }
    }
}

//============================================================
// PARAMETERS
//============================================================

void DelayModule::setDelayTime(
    float newDelayMs)
{
    targetDelayMs =
        juce::jlimit(
            1.0f,
            2000.0f,
            newDelayMs);

    delayTimeSmoothed
        .setTargetValue(
            targetDelayMs);
}

void DelayModule::setFeedback(
    float newFeedback)
{
    targetFeedback =
        juce::jlimit(
            0.0f,
            0.95f,
            newFeedback);

    feedbackSmoothed
        .setTargetValue(
            targetFeedback);
}

void DelayModule::setMix(
    float newMix)
{
    targetMix =
        juce::jlimit(
            0.0f,
            1.0f,
            newMix);

    mixSmoothed
        .setTargetValue(
            targetMix);
}