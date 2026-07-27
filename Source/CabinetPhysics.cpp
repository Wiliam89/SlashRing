/*
  ==============================================================================

    CabinetPhysics.cpp
    Created: 18 Jul 2026 6:27:30pm
    Author:  wilia

  ==============================================================================
*/

#include "CabinetPhysics.h"

CabinetPhysics::CabinetPhysics() noexcept {}

void CabinetPhysics::prepare (const juce::dsp::ProcessSpec& spec) noexcept
{
    sampleRate = spec.sampleRate;
    woodFilter.prepare (spec);
    
    maxDelaySamples = (size_t)juce::roundToInt (0.04f * sampleRate); // Max 40ms reflexões internas
    internalDelayBuffer.assign (maxDelaySamples, 0.0f);
    delayIndex = 0;

    woodResonanceGain.reset (sampleRate, 0.05);
    airCompressionFeedback.reset (sampleRate, 0.05);
}

void CabinetPhysics::setCabinetParams (float sizeNorm, float openBackAmount) noexcept
{
    // Determina o comportamento de ressonância do compensador de tamanho da caixa
    float centerFreq = juce::jmap (sizeNorm, 0.0f, 1.0f, 180.0f, 75.0f);
    *woodFilter.coefficients = *juce::dsp::IIR::Coefficients<float>::makePeakFilter (sampleRate, centerFreq, 4.0f, juce::Decibels::decibelsToGain (4.0f));
    
    woodResonanceGain.setValue (0.15f * (1.0f - openBackAmount));
    airCompressionFeedback.setValue (0.22f * (1.0f - openBackAmount));
}

void CabinetPhysics::process (juce::dsp::AudioBlock<float>& block) noexcept
{
    float* ptr = block.getChannelPointer (0);
    size_t numSamples = block.getNumSamples();

    for (size_t i = 0; i < numSamples; ++i)
    {
        float x = ptr[i];
        float resWood = woodFilter.processSample (x);
        
        float delayedSample = internalDelayBuffer[(delayIndex - 1 + maxDelaySamples) % maxDelaySamples];
        float feedbackAmount = airCompressionFeedback.getNextValue();
        
        float currentX = x + (delayedSample * feedbackAmount);
        internalDelayBuffer[delayIndex] = currentX;
        delayIndex = (delayIndex + 1) % maxDelaySamples;

        ptr[i] = currentX + (resWood * woodResonanceGain.getNextValue());
    }
}

void CabinetPhysics::reset() noexcept
{
    woodFilter.reset();
    std::fill (internalDelayBuffer.begin(), internalDelayBuffer.end(), 0.0f);
}
