#pragma once
#include <JuceHeader.h>

class AmpModule
{
public:
 AmpModule();
 ~AmpModule();

 
 void prepare (double sampleRate, int samplesPerBlock, int numChannels);
 void process (juce::dsp::AudioBlock<float>& block);
 void reset();
 void release();
 void setGain (float value);
 void setBass (float value);
 void setMiddle (float value);
 void setTreble (float value);
 void setPresence (float value);
 void setMaster (float value);
 float getCurrentGainValue() const noexcept { return gainSmoothed.getCurrentValue(); }


private:
 juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> inputHPF;
 juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> bassFilter;
 juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> midFilter;
 juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> trebleFilter;
 juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> resonanceShelf; // NOVO
 juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> presenceShelf;
 juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> outputLPF;
 juce::SmoothedValue<float> gainSmoothed;
 juce::SmoothedValue<float> masterSmoothed;


  // SAG (power amp) — NOVO
    float sagEnvelope = 0.0f;
   float sagAttackCoeff = 0.0f;
   float sagReleaseCoeff = 0.0f;

 static constexpr float sagAmount = 0.25f;
   float gain = 5.0f, bass = 5.0f, middle = 5.0f, treble = 5.0f, presence = 5.0f, master = 6.0f;
   double currentSampleRate = 44100.0;
   float currentDriveGain = 1.0f;
   float currentMasterGain = 1.0f;
  void updateToneStack();


 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AmpModule)
};