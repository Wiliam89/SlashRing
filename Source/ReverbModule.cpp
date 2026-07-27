#include "ReverbModule.h"

ReverbModule::ReverbModule()
{
}

ReverbModule::~ReverbModule()
{
}

void ReverbModule::prepare(
    double sampleRate,
    int samplesPerBlock,
    int numChannels)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize =
        static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels =
        static_cast<juce::uint32>(numChannels);

    reverb.reset();
    reverb.prepare(spec);

    juce::dsp::Reverb::Parameters params;
    params.roomSize = 0.45f;
    params.damping = 0.50f;
    params.wetLevel = mix;
    params.dryLevel = 1.0f;
    params.width = 1.0f;
    params.freezeMode = 0.0f;

    reverb.setParameters(params);
}

void ReverbModule::setMix(float newMix)
{
    mix = juce::jlimit(0.0f, 1.0f, newMix);

    auto params = reverb.getParameters();
    params.wetLevel = mix;

    reverb.setParameters(params);
}

void ReverbModule::process(
    juce::AudioBuffer<float>& buffer)
{
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    reverb.process(context);
}