#include "AmpModule.h"


AmpModule::AmpModule() {}
AmpModule::~AmpModule() {}


void AmpModule::reset()

{
   inputHPF.reset();
   bassFilter.reset();
   midFilter.reset();
   trebleFilter.reset();
   resonanceShelf.reset();
   presenceShelf.reset();
   outputLPF.reset();
   sagEnvelope = 0.0f;
}

void AmpModule::release() {}

void AmpModule::prepare (double sampleRate, int samplesPerBlock, int numChannels)

{
 currentSampleRate = sampleRate;
 gainSmoothed.reset (sampleRate, 0.02);
 masterSmoothed.reset (sampleRate, 0.02);
 gainSmoothed.setCurrentAndTargetValue (currentDriveGain);
 masterSmoothed.setCurrentAndTargetValue (currentMasterGain);

 // SAG: constantes de tempo (attack ~5 ms, release ~60 ms)
 sagAttackCoeff = 1.0f - std::exp (-1.0f / (0.005f * (float) sampleRate));
 sagReleaseCoeff = 1.0f - std::exp (-1.0f / (0.060f * (float) sampleRate));

 juce::dsp::ProcessSpec spec;
 spec.sampleRate = sampleRate;
 spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
 spec.numChannels = static_cast<juce::uint32> (numChannels);
 inputHPF.prepare (spec);
 bassFilter.prepare (spec);
 midFilter.prepare (spec);
 trebleFilter.prepare (spec);
 resonanceShelf.prepare (spec);
 presenceShelf.prepare (spec);
 outputLPF.prepare (spec);
 *inputHPF.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, 80.0f);
 *outputLPF.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, 9000.0f);
 
 // Resonance: voicing fixo do grave do power amp. Pode virar parametro depois.
 *resonanceShelf.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf (sampleRate, 100.0f, 0.707f, juce::Decibels::decibelsToGain(1.3f)); 
  updateToneStack();
   reset();

}


void AmpModule::updateToneStack()
{
 const float bassGain = juce::jmap (bass, 0.0f, 10.0f, 0.5f, 2.0f);
 const float midGain = juce::jmap (middle, 0.0f, 10.0f, 0.5f, 2.0f);
 const float trebleGain = juce::jmap (treble, 0.0f, 10.0f, 0.5f, 2.0f);
 const float presenceGain = juce::jmap (presence, 0.0f, 10.0f, 0.5f, 2.0f);
 *bassFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf (currentSampleRate, 120.0f, 0.707f, bassGain);
 *midFilter.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter (currentSampleRate, 750.0f, 0.8f, midGain);
 *trebleFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf (currentSampleRate, 3500.0f, 0.707f, trebleGain);
 *presenceShelf.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf (currentSampleRate, 5000.0f, 0.707f, presenceGain);
}


void AmpModule::process (juce::dsp::AudioBlock<float>& block)
{

  juce::ScopedNoDenormals noDenormals;
  // INPUT HPF
  { juce::dsp::ProcessContextReplacing<float> ctx (block); inputHPF.process (ctx); }
  const auto numChannels = block.getNumChannels();
  const auto numSamples = block.getNumSamples();


 // PREAMP — 2 estagios, sample-outer / channel-inner (TD-003)
 for (size_t i = 0; i < numSamples; ++i)
 {
 const float drive = gainSmoothed.getNextValue();

 for (size_t ch = 0; ch < numChannels; ++ch)
    {
      auto* s = block.getChannelPointer (ch);
      float x = s[i] * drive;
      x = (x >= 0.0f) ? std::tanh (x * 1.75f) : std::tanh (x * 1.25f); // estagio 1
      x = (x >= 0.0f) ? std::tanh (x * 1.10f) : std::tanh (x * 0.90f); // estagio 2
      s[i] = x;
    }

 }


 // TONE STACK
 { juce::dsp::ProcessContextReplacing<float> ctx (block);
 bassFilter.process (ctx); midFilter.process (ctx); trebleFilter.process (ctx); }
 // RESONANCE (grave do power amp)
 { juce::dsp::ProcessContextReplacing<float> ctx (block); resonanceShelf.process (ctx); }
 // POWER AMP — sag + tanh assimetrico + master, sample-outer / channel-inner
 for (size_t i = 0; i < numSamples; ++i)
 {
 const float masterGain = masterSmoothed.getNextValue();
 float lvl = 0.0f;


 for (size_t ch = 0; ch < numChannels; ++ch)
 lvl = juce::jmax (lvl, std::abs (block.getChannelPointer (ch)[i]));

 const float coeff = (lvl > sagEnvelope) ? sagAttackCoeff : sagReleaseCoeff;
 sagEnvelope += coeff * (lvl - sagEnvelope);
 const float sagGain = 1.0f / (1.0f + sagAmount * sagEnvelope);

 for (size_t ch = 0; ch < numChannels; ++ch)
 {
 auto* s = block.getChannelPointer (ch);
 float x = s[i];
 x = (x >= 0.0f) ? std::tanh (x * 1.40f) : std::tanh (x * 1.30f);
 x *= sagGain;
 x *= masterGain;
 s[i] = x;
 }


 }
 // PRESENCE (agudo do power amp)
 { juce::dsp::ProcessContextReplacing<float> ctx (block); presenceShelf.process (ctx); }
 // OUTPUT LPF
 { juce::dsp::ProcessContextReplacing<float> ctx (block); outputLPF.process (ctx); }
}  


void AmpModule::setGain (float value)
{
   gain = juce::jlimit (0.0f, 10.0f, value);
   const float normalized = gain / 10.0f;
   currentDriveGain = juce::jmap (normalized, 1.0f, 35.0f);
   gainSmoothed.setTargetValue (currentDriveGain);
}


void AmpModule::setBass (float value) { bass = value; updateToneStack(); }
void AmpModule::setMiddle (float value) { middle = value; updateToneStack(); }
void AmpModule::setTreble (float value) { treble = value; updateToneStack(); }
void AmpModule::setPresence (float value) { presence = value; updateToneStack(); }
void AmpModule::setMaster (float value)


{
 master = juce::jlimit (0.0f, 10.0f, value);
 const float normalized = master / 10.0f;
 currentMasterGain = juce::Decibels::decibelsToGain (juce::jmap (normalized, -60.0f, 0.0f));
 masterSmoothed.setTargetValue (currentMasterGain);
}