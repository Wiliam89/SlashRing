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

    speakerNonLinearity.prepare (monoSpec);
    cabinetPhysics.prepare (monoSpec);
    cabinetAcoustics.prepare (monoSpec);
    cabinetConvolution.prepare (monoSpec);

    lowCutFilter.prepare (spec);
    highCutFilter.prepare (spec);
    microDelayLine.prepare (spec);
    microDelayLine.setMaximumDelayInSamples (4800);

    reset();
}

void CabinetVoice::reset() noexcept
{
    speakerNonLinearity.reset();
    cabinetPhysics.reset();
    cabinetAcoustics.reset();
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
    gainLinear = juce::Decibels::decibelsToGain (gainDb);
    delaySamplesTarget = static_cast<float> ((delayMs / 1000.0) * sampleRate);

    float adjustedSize = juce::jlimit (0.3f, 1.7f, 1.0f + (micDist * 0.2f));
    cabinetAcoustics.setCabinetSize (adjustedSize);
    cabinetAcoustics.setResonanceDepth (juce::jlimit (0.0f, 1.0f, 1.0f - (micAngle / 90.0f)));

    float clampedLow = juce::jlimit (20.0f, 600.0f, lowCut);
    float clampedHigh = juce::jlimit (800.0f, 20000.0f, highCut);
    *lowCutFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, clampedLow);
    *highCutFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, clampedHigh);
}

void CabinetVoice::setPhysics (float cabSize, float openBack, float drive, float breakup) noexcept
{
    cabinetPhysics.setCabinetParams (cabSize, openBack);
    
    // Vinculo direto com as funções originais do seu alto-falante
   // Correção absoluta: mapeando para a função real do seu SpeakerNonLinearity
    speakerNonLinearity.setPhysicsParams (drive, breakup);
}

void CabinetVoice::process (juce::dsp::ProcessContextReplacing<float>& context) noexcept
{
    if (context.isBypassed)
        return;

    auto inBlock = context.getInputBlock();
    auto outBlock = context.getOutputBlock();
    size_t numSamples = inBlock.getNumSamples();

    float* monoPtr = internalMonoBuffer.getWritePointer (0);
    const float* srcL = inBlock.getChannelPointer (0);
    const float* srcR = inBlock.getNumChannels() > 1 ? inBlock.getChannelPointer (1) : srcL;

    for (size_t i = 0; i < numSamples; ++i)
    {
        monoPtr[i] = (srcL[i] + srcR[i]) * 0.5f;
    }

    // CORREÇÃO C3536: Captura o sub-bloco dinamicamente garantindo que ele está inicializado para o monoContext
    auto activeMonoBlock = monoBlock.getSubBlock (0, numSamples);
    juce::dsp::ProcessContextReplacing<float> monoContext (activeMonoBlock);

    speakerNonLinearity.process (monoContext);
    cabinetPhysics.process (activeMonoBlock);
    cabinetAcoustics.process (monoContext);
    cabinetConvolution.process (monoContext);

    size_t outChannels = outBlock.getNumChannels();
    for (size_t ch = 0; ch < outChannels; ++ch)
    {
        float* dst = outBlock.getChannelPointer (ch);
        juce::FloatVectorOperations::copy (dst, monoPtr, static_cast<int> (numSamples));
    }

    lowCutFilter.process (context);
    highCutFilter.process (context);

    if (delaySamplesTarget > 0.5f)
    {
        microDelayLine.setDelay (delaySamplesTarget);

        for (size_t ch = 0; ch < outChannels; ++ch)
        {
            float* dst = outBlock.getChannelPointer (ch);
            for (size_t s = 0; s < numSamples; ++s)
            {
                dst[s] = microDelayLine.popSample (static_cast<int> (ch)) * gainLinear;
                microDelayLine.pushSample (static_cast<int> (ch), dst[s]);
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