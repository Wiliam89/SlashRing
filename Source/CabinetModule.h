#pragma once
#include "CabinetVoice.h"
#include "CabinetMixer.h"
#include "IRLoader.h"

class CabinetModule : public IRLoader::Listener
{
public:
    CabinetModule();
    ~CabinetModule() override;

    // CORREÇÃO: Método sobrecarregado clássico para aceitar os parâmetros passados pelo PluginProcessor original
    void prepare (double sampleRate, int samplesPerBlock, int numChannels);
    void prepare (const juce::dsp::ProcessSpec& spec);
    
    // CORREÇÃO: Aceita o AudioBuffer diretamente vindo do processBlock do PluginProcessor
    void process (juce::AudioBuffer<float>& buffer) noexcept;
    void process (juce::dsp::ProcessContextReplacing<float>& context) noexcept;
    
    void reset();

    // Controle de Ativação exigido pelo PluginProcessor
    void setEnabled (bool shouldBeEnabled) noexcept { isEnabled = shouldBeEnabled; }

    void triggerAsyncFactoryIRLoad (int voiceIndex, const void* data, size_t size, const juce::String& uniqueHash);
    void triggerAsyncUserIRLoad (int voiceIndex, const juce::File& file);
    
    void configureVoice (int voiceIndex, float gainDb, float lowCut, float highCut, float delayMs, float micDist, float micAngle) noexcept;
    void configurePhysics (int voiceIndex, float cabSize, float openBack, float drive, float breakup) noexcept;
    void configureMixer (float blend, float pan, float width, float outputDb) noexcept;

    void irLoadingFinished (const juce::String& voiceId, CachedIRData::Ptr irData) override;
    void irLoadingFailed (const juce::String& voiceId, const juce::String& reason) override;

private:
    bool isEnabled = true;

    CabinetVoice voiceA;
    CabinetVoice voiceB;
    CabinetMixer mixer;
    std::unique_ptr<IRLoader> irLoader;

    juce::AudioBuffer<float> localVoiceBufferA;
    juce::AudioBuffer<float> localVoiceBufferB;
    juce::dsp::AudioBlock<float> audioBlockA;
    juce::dsp::AudioBlock<float> audioBlockB;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CabinetModule)
};