#include "ReverbModule.h"


ReverbModule::ReverbModule() {}
ReverbModule::~ReverbModule() {}

void ReverbModule::prepare (double sampleRate, int samplesPerBlock, int numChannels)
{
 currentSampleRate = sampleRate;
 juce::dsp::ProcessSpec spec;
 spec.sampleRate = sampleRate;
 spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
 spec.numChannels = static_cast<juce::uint32> (numChannels);
 reverb.reset();
 reverb.prepare (spec);


 juce::dsp::Reverb::Parameters params;
 params.roomSize = 0.5f;
 params.damping = 0.5f;
 params.wetLevel = 1.0f; // 100% wet — o balanco e feito manualmente em process()
 params.dryLevel = 0.0f;
 params.width = 1.0f;
 params.freezeMode = 0.0f;
 reverb.setParameters (params);
 sendHighPass.prepare (spec);
 *sendHighPass.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, lowCutHz);
 sendHighPass.reset();
 preDelay.prepare (spec);
 preDelay.setMaximumDelayInSamples (static_cast<int> (sampleRate * 0.2) + 1);
 preDelay.reset();
 preDelay.setDelay (static_cast<float> ((preDelayMs / 1000.0) * sampleRate));
 wetBuffer.setSize (numChannels, samplesPerBlock);
}

void ReverbModule::setMix (float newMix)
{
 mix = juce::jlimit (0.0f, 1.0f, newMix);
}


void ReverbModule::process (juce::AudioBuffer<float>& buffer)
{
 juce::ScopedNoDenormals noDenormals;
 const int numChannels = buffer.getNumChannels();
 const int numSamples = buffer.getNumSamples();

 // Copia seca -> envio (wet)
 for (int ch = 0; ch < numChannels; ++ch)
 wetBuffer.copyFrom (ch, 0, buffer, ch, 0, numSamples);
 juce::dsp::AudioBlock<float> wetBlock (wetBuffer);
 auto wetSub = wetBlock.getSubBlock (0, static_cast<size_t> (numSamples));

 // Low-cut no envio (mantem o grave seco)
 { juce::dsp::ProcessContextReplacing<float> ctx (wetSub); sendHighPass.process (ctx); }

 // Pre-delay
 for (int ch = 0; ch < numChannels; ++ch)
 {

 float* w = wetBuffer.getWritePointer (ch);
 for (int s = 0; s < numSamples; ++s)
 {
 preDelay.pushSample (ch, w[s]);
 w[s] = preDelay.popSample (ch);
 }

 }

 // Reverb 100% wet
 { juce::dsp::ProcessContextReplacing<float> ctx (wetSub); reverb.process (ctx); }
 // Mix dry / wet

 for (int ch = 0; ch < numChannels; ++ch)
 {
     float* dry = buffer.getWritePointer (ch);
     const float* wet = wetBuffer.getReadPointer (ch);
     for (int s = 0; s < numSamples; ++s)
     dry[s] = dry[s] * (1.0f - mix) + wet[s] * mix;
   }
}