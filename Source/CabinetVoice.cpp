#include "CabinetVoice.h"
void CabinetVoice::prepare (const juce::dsp::ProcessSpec& spec) noexcept
{
 sampleRate = spec.sampleRate;

 internalMonoBuffer.setSize (1, static_cast<int> (spec.maximumBlockSize));
 monoBlock = juce::dsp::AudioBlock<float> (internalMonoBuffer);

 juce::dsp::ProcessSpec monoSpec;
 monoSpec.sampleRate = spec.sampleRate;
 monoSpec.maximumBlockSize = spec.maximumBlockSize;
 monoSpec.numChannels = 1;


 cabinetConvolution.prepare (monoSpec);


 lowCutFilter.prepare (spec);
 highCutFilter.prepare (spec);
 microDelayLine.prepare (spec);
 microDelayLine.setMaximumDelayInSamples (4800);

 // Defaults seguros ate setVoiceParameters ser chamado
 *lowCutFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, 70.0f);
 *highCutFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, 9000.0f);

 reset();

}

void CabinetVoice::reset() noexcept
{
    cabinetConvolution.reset();
    lowCutFilter.reset();
    highCutFilter.reset();
    microDelayLine.reset();
    internalMonoBuffer.clear();
}


void CabinetVoice::updateIRBuffer (CachedIRData::Ptr irData) noexcept
{
 cabinetConvolution.updateIR (irData);
}


void CabinetVoice::setVoiceParameters (float gainDb, float lowCut, float highCut, float delayMs, float micDist, float micAngle) noexcept
{
    juce::ignoreUnused (micDist, micAngle);

    gainLinear = juce::Decibels::decibelsToGain (gainDb);
    delaySamplesTarget = static_cast<float> ((delayMs / 1000.0) * sampleRate);

    const float clampedLow = juce::jlimit (20.0f, 600.0f, lowCut);
    const float clampedHigh = juce::jlimit (800.0f, 20000.0f, highCut);

    *lowCutFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, clampedLow);
    *highCutFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, clampedHigh);
}


void CabinetVoice::setPhysics (float, float, float, float) noexcept
{
 // IR-only: a fisica do alto-falante ja esta capturada na IR. No-op por compatibilidade.
}


void CabinetVoice::process (juce::dsp::ProcessContextReplacing<float>& context) noexcept
{
    if (context.isBypassed)
      return;


    auto inBlock = context.getInputBlock();
    auto outBlock = context.getOutputBlock();
    const size_t numSamples = inBlock.getNumSamples();


    // 1) Soma para mono
    float* monoPtr = internalMonoBuffer.getWritePointer (0);
    const float* srcL = inBlock.getChannelPointer (0);
    const float* srcR = inBlock.getNumChannels() > 1 ? inBlock.getChannelPointer (1) : srcL;
    for (size_t i = 0; i < numSamples; ++i)
        monoPtr[i] = (srcL[i] + srcR[i]) * 0.5f;
    // 2) IR (convolucao) no dominio mono
    auto activeMonoBlock = monoBlock.getSubBlock (0, numSamples); 


juce::dsp::ProcessContextReplacing<float> monoContext (activeMonoBlock);
 cabinetConvolution.process (monoContext);


    // 3) Copia mono -> todos os canais de saida
    const size_t outChannels = outBlock.getNumChannels();
    for (size_t ch = 0; ch < outChannels; ++ch)
        juce::FloatVectorOperations::copy (outBlock.getChannelPointer (ch), monoPtr, static_cast<int> (numSamples));
   // 4) Filtros de trim (HPF / LPF)
    lowCutFilter.process (context);
    highCutFilter.process (context);


   // 5) Micro-delay (distancia de mic) + ganho de saida
   if (delaySamplesTarget > 0.5f)
  {

       microDelayLine.setDelay (delaySamplesTarget);
      for (size_t ch = 0; ch < outChannels; ++ch)
    {
         float* dst = outBlock.getChannelPointer (ch);
         for (size_t s = 0; s < numSamples; ++s)
       {
            const float in = dst[s];
            microDelayLine.pushSample (static_cast<int> (ch), in);
            dst[s] = microDelayLine.popSample (static_cast<int> (ch)) * gainLinear;
        }
     }
  }
 else
  {
      for (size_t ch = 0; ch < outChannels; ++ch)
     {
       float* dst = outBlock.getChannelPointer (ch);
       for (size_t s = 0; s < numSamples; ++s)
           dst[s] *= gainLinear;
    }
  }
}