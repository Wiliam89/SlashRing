#include "CabinetModule.h"


CabinetModule::CabinetModule()
{
 irLoader = std::make_unique<IRLoader> (this);

}

CabinetModule::~CabinetModule() {}
void CabinetModule::prepare (double sampleRate,
 int samplesPerBlock,
 int numChannels)
{
 juce::dsp::ProcessSpec spec;
 spec.sampleRate = sampleRate;
 spec.maximumBlockSize =
 static_cast<juce::uint32> (samplesPerBlock);
 spec.numChannels =
 static_cast<juce::uint32> (numChannels);
 prepare (spec);
}

void CabinetModule::prepare (const juce::dsp::ProcessSpec& spec)
{
 irLoader->prepare (spec.sampleRate);
 voiceA.prepare (spec);
 voiceB.prepare (spec);
 mixer.prepare (spec);
 localVoiceBufferA.setSize (2, (int) spec.maximumBlockSize);
 localVoiceBufferB.setSize (2, (int) spec.maximumBlockSize);
 audioBlockA = juce::dsp::AudioBlock<float> (localVoiceBufferA);
 audioBlockB = juce::dsp::AudioBlock<float> (localVoiceBufferB);
 // Dry/wet
 dryBuffer.setSize ((int) spec.numChannels,
 (int) spec.maximumBlockSize);
 mixSmoothed.reset (spec.sampleRate, 0.02);
 mixSmoothed.setCurrentAndTargetValue (1.0f);
}

void CabinetModule::process
 (juce::AudioBuffer<float>& buffer) noexcept
{

 if (! isEnabled)
 return;
 juce::ScopedNoDenormals noDenormals;
 const int numChannels = buffer.getNumChannels();
 const int numSamples = buffer.getNumSamples();

 // 1) Copia do sinal seco (antes do cabinet)
 for (int ch = 0; ch < numChannels; ++ch)
 dryBuffer.copyFrom (ch, 0, buffer, ch, 0, numSamples);

 // 2) Processa o molhado (IR) no proprio buffer
 juce::dsp::AudioBlock<float> block (buffer);
 juce::dsp::ProcessContextReplacing<float> ctx (block);
 process (ctx);

 // 3) Mistura dry/wet
 for (int s = 0; s < numSamples; ++s)
 {
 const float m = mixSmoothed.getNextValue();
 for (int ch = 0; ch < numChannels; ++ch)
 {
     float* wet = buffer.getWritePointer (ch);
    const float* dry = dryBuffer.getReadPointer (ch);
    wet[s] = dry[s] * (1.0f - m) + wet[s] * m;

    }
  }
}


void CabinetModule::process
 (juce::dsp::ProcessContextReplacing<float>& context) noexcept
{

 if (! isEnabled || context.isBypassed)
 return;
 auto inBlock = context.getInputBlock();
 auto outBlock = context.getOutputBlock();
 size_t samples = inBlock.getNumSamples();
 auto activeBlockA = audioBlockA.getSubBlock (0, samples);
 auto activeBlockB = audioBlockB.getSubBlock (0, samples);
 activeBlockA.copyFrom (inBlock);
 activeBlockB.copyFrom (inBlock);
 juce::dsp::ProcessContextReplacing<float> ctxA (activeBlockA);
 juce::dsp::ProcessContextReplacing<float> ctxB (activeBlockB);
 voiceA.process (ctxA);
 voiceB.process (ctxB);
 mixer.process (activeBlockA, activeBlockB, outBlock);
}

void CabinetModule::setMix (float newMix) noexcept
{
 mixSmoothed.setTargetValue (juce::jlimit (0.0f, 1.0f, newMix));
}


void CabinetModule::triggerAsyncFactoryIRLoad
 (int voiceIndex, const void* data, size_t size,
 const juce::String& uniqueHash)
{
 juce::String id = (voiceIndex == 0) ? "A" : "B";
 irLoader->submitLoadTask (id, data, size, uniqueHash);
}


void CabinetModule::triggerAsyncUserIRLoad
 (int voiceIndex, const juce::File& file)
{
 juce::String id = (voiceIndex == 0) ? "A" : "B";
 irLoader->submitLoadTask (id, file);
}


void CabinetModule::configureVoice
 (int voiceIndex, float gainDb, float lowCut, float highCut,
 float delayMs, float micDist, float micAngle) noexcept
{
 if (voiceIndex == 0)
 voiceA.setVoiceParameters (gainDb, lowCut, highCut,
 delayMs, micDist, micAngle);
 else
 voiceB.setVoiceParameters (gainDb, lowCut, highCut,
 delayMs, micDist, micAngle);
}


void CabinetModule::configurePhysics
 (int voiceIndex, float cabSize, float openBack,
 float drive, float breakup) noexcept
{
 if (voiceIndex == 0)
 voiceA.setPhysics (cabSize, openBack, drive, breakup);

  else
 voiceB.setPhysics (cabSize, openBack, drive, breakup);
}


void CabinetModule::configureMixer
 (float blend, float pan, float width, float outputDb) noexcept
{
 mixer.setMixerParams (blend, pan, width, outputDb);
}


void CabinetModule::irLoadingFinished
 (const juce::String& voiceId, CachedIRData::Ptr irData)
{
 if (voiceId == "A")
 voiceA.updateIRBuffer (irData);
 else if (voiceId == "B")
 voiceB.updateIRBuffer (irData);
}


void CabinetModule::irLoadingFailed
 (const juce::String& voiceId, const juce::String& reason)
{
 juce::Logger::writeToLog (
 "--- SLASHRING DSP ERROR --- Voice ID: "
 + voiceId + " -> " + reason);
}


void CabinetModule::reset()
{
 voiceA.reset();
 voiceB.reset();
 mixer.reset();
 dryBuffer.clear();
 mixSmoothed.setCurrentAndTargetValue (
 mixSmoothed.getTargetValue());
}