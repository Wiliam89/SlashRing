#pragma once
#include "CabinetVoice.h"
#include "CabinetMixer.h"
#include "IRLoader.h"
class CabinetModule : public IRLoader::Listener
{
public:
 CabinetModule();
 ~CabinetModule() override;

void prepare (double sampleRate, int samplesPerBlock,
 int numChannels);
 void prepare (const juce::dsp::ProcessSpec& spec);
 void process (juce::AudioBuffer<float>& buffer) noexcept;
 void process (juce::dsp::ProcessContextReplacing<float>&
 context) noexcept;
 void reset();
 
 void setEnabled (bool shouldBeEnabled) noexcept
 { isEnabled = shouldBeEnabled; }


 // Dry/wet do cabinet (0 = seco, 1 = 100% com IR)
 void setMix (float newMix) noexcept;
 void triggerAsyncFactoryIRLoad (int voiceIndex,
 const void* data,
 size_t size,
 const juce::String& uniqueHash);
 void triggerAsyncUserIRLoad (int voiceIndex,
 const juce::File& file);

 void configureVoice (int voiceIndex, float gainDb,
 float lowCut, float highCut,
 float delayMs, float micDist,
 float micAngle) noexcept;

 void configurePhysics (int voiceIndex, float cabSize,
 float openBack, float drive,
 float breakup) noexcept;
 void configureMixer (float blend, float pan,

 float width, float outputDb) noexcept;
 void irLoadingFinished (const juce::String& voiceId,
 CachedIRData::Ptr irData) override;
 void irLoadingFailed (const juce::String& voiceId,
 const juce::String& reason) override;


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


 // Dry/wet — NOVO
 juce::AudioBuffer<float> dryBuffer;
 juce::SmoothedValue<float,
 juce::ValueSmoothingTypes::Linear> mixSmoothed;


 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CabinetModule)
};