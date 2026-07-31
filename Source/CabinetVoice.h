#pragma once
#include <JuceHeader.h>
#include "CabinetDataTypes.h"
#include "CabinetConvolution.h"
class CabinetVoice final
{
public:
 CabinetVoice() = default;
 ~CabinetVoice() = default;

 void prepare (const juce::dsp::ProcessSpec& spec) noexcept;
 void reset() noexcept;
 void updateIRBuffer (CachedIRData::Ptr irData) noexcept;
 void setVoiceParameters (float gainDb, float lowCut, float highCut, float delayMs, float micDist, float micAngle) noexcept;
 void setPhysics (float cabSize, float openBack, float drive, float breakup) noexcept; // no-op (IR-only)
 void process (juce::dsp::ProcessContextReplacing<float>& context) noexcept;

private:
 double sampleRate = 44100.0;
 float gainLinear = 1.0f;
 float delaySamplesTarget = 0.0f;
 juce::dsp::AudioBlock<float> monoBlock;
 juce::AudioBuffer<float> internalMonoBuffer;
 
 CabinetConvolution cabinetConvolution;
 juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> lowCutFilter;
 juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> highCutFilter;
 juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> microDelayLine { 4800 };
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CabinetVoice)

};