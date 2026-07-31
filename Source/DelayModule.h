#pragma once
#include <JuceHeader.h>
class DelayModule
{
public:
 DelayModule();
 ~DelayModule();

 void prepare (double sampleRate, int samplesPerBlock, int numChannels);
 void reset();
 void process (juce::AudioBuffer<float>& buffer);
 void setDelayTime (float newDelayMs);
 void setFeedback (float newFeedback);
 void setMix (float newMix);


private:
 juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> leftDelay { 96000 };
 juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> rightDelay { 96000 };
 juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> delayTimeSmoothed;
 juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> feedbackSmoothed;
 juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmoothed;

 // High-cut no caminho de feedback (repeticoes analogicas) — NOVO
 float fbLpfCoeff = 1.0f;
 float fbStateL = 0.0f;
 float fbStateR = 0.0f;
 
 static constexpr float feedbackToneHz = 4000.0f;
 float targetDelayMs = 420.0f;
 float targetFeedback = 0.35f;
 float targetMix = 0.22f;
 double currentSampleRate = 44100.0;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DelayModule)
};