#include "CabinetModule.h"

// ========================================================
// CAB-002: MODULE ISOLATION TEST
// ========================================================
// Temporary isolation flag for debugging.
// When true, enables selective bypass of pipeline stages.
// Set to false for production.
constexpr bool kCabIsolation = true;
// ========================================================

CabinetModule::CabinetModule()
{
    irLoader = std::make_unique<IRLoader> (this);
}

CabinetModule::~CabinetModule() {}

void CabinetModule::prepare (double sampleRate, int samplesPerBlock, int numChannels)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32> (numChannels);
    prepare (spec);
}

void CabinetModule::prepare (const juce::dsp::ProcessSpec& spec)
{
    irLoader->prepare (spec.sampleRate);
    
    voiceA.prepare (spec);
    voiceB.prepare (spec);
    mixer.prepare (spec);

    localVoiceBufferA.setSize (2, (int)spec.maximumBlockSize);
    localVoiceBufferB.setSize (2, (int)spec.maximumBlockSize);
    
    audioBlockA = juce::dsp::AudioBlock<float> (localVoiceBufferA);
    audioBlockB = juce::dsp::AudioBlock<float> (localVoiceBufferB);
}

void CabinetModule::process (juce::AudioBuffer<float>& buffer) noexcept
{
    if (!isEnabled)
        return;

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);
    process (ctx);
}

void CabinetModule::process (juce::dsp::ProcessContextReplacing<float>& context) noexcept
{
    if (!isEnabled || context.isBypassed)
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

    // ========================================================
    // CAB-002: ISOLATION TESTS
    // ========================================================
    // TESTE 1: Only CabinetConvolution
    // Bypass: SpeakerNonLinearity, CabinetAcoustics, CabinetMixer
    if (kCabIsolation)
    {
        // TESTE 1 - ONLY CONVOLUTION
        // voiceA.process (ctxA);
        // voiceB.process (ctxB);
        // outBlock.copyFrom (activeBlockA);
        // return;

        // TESTE 2 - CONVOLUTION + SPEAKER
        voiceA.process (ctxA);
        voiceB.process (ctxB);
        // outBlock.copyFrom (activeBlockA);
        // return;

        // TESTE 3 - CONVOLUTION + SPEAKER + ACOUSTICS
        // voiceA.process (ctxA);
        // voiceB.process (ctxB);
        // outBlock.copyFrom (activeBlockA);
        // return;

        // TESTE 4 - FULL PIPELINE
        mixer.process (activeBlockA, activeBlockB, outBlock);
    }
    else
    {
        voiceA.process (ctxA);
        voiceB.process (ctxB);
        mixer.process (activeBlockA, activeBlockB, outBlock);
    }
}

void CabinetModule::triggerAsyncFactoryIRLoad (int voiceIndex, const void* data, size_t size, const juce::String& uniqueHash)
{
    juce::String id = (voiceIndex == 0) ? "A" : "B";
    // Correção dos parâmetros passados para o IRLoader do seu projeto
    irLoader->submitLoadTask (id, data, size, uniqueHash); 
}

void CabinetModule::triggerAsyncUserIRLoad (int voiceIndex, const juce::File& file)
{
    juce::String id = (voiceIndex == 0) ? "A" : "B";
    irLoader->submitLoadTask (id, file);
}

void CabinetModule::configureVoice (int voiceIndex, float gainDb, float lowCut, float highCut, float delayMs, float micDist, float micAngle) noexcept
{
    if (voiceIndex == 0)
        voiceA.setVoiceParameters (gainDb, lowCut, highCut, delayMs, micDist, micAngle);
    else
        voiceB.setVoiceParameters (gainDb, lowCut, highCut, delayMs, micDist, micAngle);
}

void CabinetModule::configurePhysics (int voiceIndex, float cabSize, float openBack, float drive, float breakup) noexcept
{
    if (voiceIndex == 0)
        voiceA.setPhysics (cabSize, openBack, drive, breakup);
    else
        voiceB.setPhysics (cabSize, openBack, drive, breakup);
}

void CabinetModule::configureMixer (float blend, float pan, float width, float outputDb) noexcept
{
    mixer.setMixerParams (blend, pan, width, outputDb);
}

void CabinetModule::irLoadingFinished (const juce::String& voiceId, CachedIRData::Ptr irData)
{
    if (voiceId == "A")
        voiceA.updateIRBuffer (irData);
    else if (voiceId == "B")
        voiceB.updateIRBuffer (irData);
}

void CabinetModule::irLoadingFailed (const juce::String& voiceId, const juce::String& reason)
{
    juce::Logger::writeToLog ("--- SLASHRING DSP ERROR --- Voice ID: " + voiceId + " -> " + reason);
}

void CabinetModule::reset()
{
    voiceA.reset();
    voiceB.reset();
    mixer.reset();
}