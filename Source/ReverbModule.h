#pragma once
#include <JuceHeader.h>

class ReverbModule

{
public:
 ReverbModule();
 ~ReverbModule();
 void prepare (double sampleRate, int samplesPerBlock, int numChannels);
 void setMix (float newMix);
 void process (juce::AudioBuffer<float>& buffer);


private:
 float mix = 0.15f;
 double currentSampleRate = 44100.0;
 juce::dsp::Reverb reverb;

 // Pre-delay + low-cut no envio — NOVO
 juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> preDelay { 96000 };
 juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> sendHighPass;
 juce::AudioBuffer<float> wetBuffer;
 static constexpr float preDelayMs = 25.0f;
 static constexpr float lowCutHz = 120.0f;

 
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReverbModule)
};