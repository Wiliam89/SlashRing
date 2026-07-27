/*
  ==============================================================================

    CabinetEngine.cpp
    Created: 18 Jul 2026 6:09:54pm
    Author:  wilia

  ==============================================================================
*/

#include "CabinetEngine.h"

// CORREÇÃO INTEGRAL: Construtor atualizado para a lista de inicialização padrão, respeitando a nova API do CabinetVoice.
CabinetEngine::CabinetEngine()
    : voiceA(),
      voiceB()
{
}

void CabinetEngine::prepare (const juce::dsp::ProcessSpec& spec) noexcept
{
    voiceA.prepare (spec);
    voiceB.prepare (spec);
    
    // CORREÇÃO INTEGRAL: Ortografia corrigida de maxBlockSize para maximumBlockSize e assinatura de setSize ajustada.
    parallelBuffer.setSize (static_cast<int> (spec.numChannels), static_cast<int> (spec.maximumBlockSize));
    reset();
}

void CabinetEngine::reset() noexcept
{
    voiceA.reset();
    voiceB.reset();
    parallelBuffer.clear();
}

void CabinetEngine::process (juce::dsp::ProcessContextReplacing<float>& context) noexcept
{
    if (context.isBypassed)
        return;

    auto& inputBlock  = context.getInputBlock();
    auto& outputBlock = context.getOutputBlock();

    if (! dualCabEnabled)
    {
        voiceA.process (context);
        return;
    }

    const int numChannels = static_cast<int> (inputBlock.getNumChannels());
    const int numSamples  = static_cast<int> (inputBlock.getNumSamples());

    for (int ch = 0; ch < numChannels; ++ch)
    {
        parallelBuffer.copyFrom (ch, 0, inputBlock.getChannelPointer (static_cast<size_t> (ch)), numSamples);
    }

    voiceA.process (context);

    juce::dsp::AudioBlock<float> parallelBlock (parallelBuffer);
    juce::dsp::AudioBlock<float> parallelSubBlock = parallelBlock.getSubBlock (0, static_cast<size_t> (numSamples));
    juce::dsp::ProcessContextReplacing<float> parallelContext (parallelSubBlock);
    
    voiceB.process (parallelContext);

    float weightA = 1.0f - masterMix;
    float weightB = masterMix;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* dst = outputBlock.getChannelPointer (static_cast<size_t> (ch));
        const float* srcB = parallelSubBlock.getChannelPointer (static_cast<size_t> (ch));

        for (int s = 0; s < numSamples; ++s)
        {
            dst[s] = (dst[s] * weightA) + (srcB[s] * weightB);
        }
    }
}