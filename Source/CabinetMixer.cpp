#include "CabinetMixer.h"

CabinetMixer::CabinetMixer() {}

void CabinetMixer::prepare (const juce::dsp::ProcessSpec& spec)
{
    blendLinear.setTargetValue (0.5f);
    panLeft.setTargetValue (0.5f);
    panRight.setTargetValue (0.5f);
    gainLinear.setTargetValue (1.0f);
    
    blendLinear.reset (spec.sampleRate, 0.02);
    panLeft.reset (spec.sampleRate, 0.02);
    panRight.reset (spec.sampleRate, 0.02);
    gainLinear.reset (spec.sampleRate, 0.02);
}

void CabinetMixer::setMixerParams (float blend, float pan, float width, float outputDb) noexcept
{
    blendLinear.setTargetValue (blend);
    gainLinear.setTargetValue (juce::Decibels::decibelsToGain (outputDb));
    
    float pL = juce::jlimit (0.0f, 1.0f, 0.5f - (pan * 0.5f) - (width * 0.1f));
    float pR = juce::jlimit (0.0f, 1.0f, 0.5f + (pan * 0.5f) + (width * 0.1f));
    panLeft.setTargetValue (pL);
    panRight.setTargetValue (pR);
}

void CabinetMixer::process (const juce::dsp::AudioBlock<float>& blockA, 
                            const juce::dsp::AudioBlock<float>& blockB, 
                            juce::dsp::AudioBlock<float>& outputBlock) noexcept
{
    size_t numSamples = outputBlock.getNumSamples();
    size_t numChannels = outputBlock.getNumChannels();

    const float* ptrA = blockA.getChannelPointer (0);
    const float* ptrB = blockB.getChannelPointer (0);

    float* outL = outputBlock.getChannelPointer (0);
    float* outR = numChannels > 1 ? outputBlock.getChannelPointer (1) : outL;

    for (size_t i = 0; i < numSamples; ++i)
    {
        float b = blendLinear.getNextValue();
        float pL = panLeft.getNextValue();
        float pR = panRight.getNextValue();
        float g = gainLinear.getNextValue();

        float mixedA = ptrA[i] * (1.0f - b);
        float mixedB = ptrB[i] * b;
        float monoSum = mixedA + mixedB;

        outL[i] = monoSum * pL * g;
        outR[i] = monoSum * pR * g;
    }
}

void CabinetMixer::reset()
{
    blendLinear.setCurrentAndTargetValue (blendLinear.getCurrentValue());
    panLeft.setCurrentAndTargetValue (panLeft.getCurrentValue());
    panRight.setCurrentAndTargetValue (panRight.getCurrentValue());
    gainLinear.setCurrentAndTargetValue (gainLinear.getCurrentValue());
}