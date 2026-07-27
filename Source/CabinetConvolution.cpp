/*
  ==============================================================================

    CabinetConvolution.cpp
    Created: 18 Jul 2026 6:11:23pm
    Author:  wilia

  ==============================================================================
*/

#include "CabinetConvolution.h"

void CabinetConvolution::prepare (const juce::dsp::ProcessSpec& spec) noexcept
{
    const juce::ScopedLock sl (engineLock);
    convolutionEngine.prepare (spec);
}

void CabinetConvolution::reset() noexcept
{
    const juce::ScopedLock sl (engineLock);
    convolutionEngine.reset();
}

void CabinetConvolution::updateIR (CachedIRData::Ptr newIRData) noexcept
{
    const juce::ScopedLock sl (engineLock);
    
    if (newIRData == nullptr || newIRData->getBuffer().getNumSamples() == 0)
    {
        isLoaded = false;
        return;
    }

    // Faz uma cópia temporária do buffer
    juce::AudioBuffer<float> tempBuffer (newIRData->getBuffer());

      convolutionEngine.loadImpulseResponse(
       std::move(tempBuffer),
       newIRData->getSampleRate(),
       juce::dsp::Convolution::Stereo::no,
       juce::dsp::Convolution::Trim::no,
       juce::dsp::Convolution::Normalise::yes
    );
    
      isLoaded = true;
}

void CabinetConvolution::process (juce::dsp::ProcessContextReplacing<float>& context) noexcept
{
    const juce::ScopedLock sl (engineLock);
    
    if (! isLoaded)
    {
        if (context.isBypassed)
            return;
            
        auto& block = context.getOutputBlock();
        block.copyFrom (context.getInputBlock());
        return;
    }

    convolutionEngine.process (context);
}