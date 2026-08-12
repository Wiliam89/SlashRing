#include "InputStage.h"

void InputStage::prepare(double sampleRate,
    int samplesPerBlock,
    int numChannels)
{
    currentSampleRate = sampleRate;

    calibrationGain =
        getPickupCalibration();

    //========================================================
    // SMOOTHING
    //========================================================

    gainSmoothed.reset(sampleRate, 0.02); // 20ms smoothing
    gainSmoothed.setCurrentAndTargetValue(targetGain);

    //========================================================
    // FILTER SETUP (HPF ~ 20Hz)
    //========================================================

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(numChannels);

    highPassFilter.reset();
    highPassFilter.prepare(spec);

    *highPassFilter.state =
        *juce::dsp::IIR::Coefficients<float>::makeHighPass(
            sampleRate,
            20.0f
        );

        voicingFilter.reset();
        voicingFilter.prepare(spec);
        updateVoicingFilter();

    reset();
}

void InputStage::reset()
{
    highPassFilter.reset();

    gainSmoothed.setCurrentAndTargetValue(
        targetGain);
}

void InputStage::release()
{
    // Método intencionalmente vazio.
    // Recursos desta classe são gerenciados por RAII.
}

void InputStage::process(juce::AudioBuffer<float>& buffer)
{
    juce::ScopedNoDenormals noDenormals;

    //========================================================
    // APPLY HPF FIRST
    //========================================================

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    highPassFilter.process(context);
    voicingFilter.process(context);

    //========================================================
    // GAIN + VOICING
    //========================================================

    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    for (int sample = 0;
        sample < numSamples;
        ++sample)
    {
        const float currentGain =
            gainSmoothed.getNextValue();

        for (int channel = 0;
            channel < numChannels;
            ++channel)
        {
            auto* samples =
                buffer.getWritePointer(channel);

            float x =
                samples[sample];

            x *= currentGain;

            x = applyInputVoicing(x);

            samples[sample] = x;
        }
    }
}

void InputStage::setInputGain(float newGain)
{
    targetGain = juce::jlimit(0.0f, 2.0f, newGain);
    gainSmoothed.setTargetValue(targetGain);
}

void InputStage::setInputType(int newType)
{
    const int clampedType =
        juce::jlimit(0, 4, newType);

    inputType =
        static_cast<PickupType>(
            clampedType);

    calibrationGain =
        getPickupCalibration();

    updateVoicingFilter();
}

float InputStage::applyInputVoicing(float x) noexcept
{
    x *= calibrationGain;

    switch (inputType)
    {
    case PickupType::SingleCoil:
        x *= 1.05f;
        break;

    case PickupType::P90:
        x *= 1.02f;
        break;

    case PickupType::HumbuckerVintage:
        break;

    case PickupType::HumbuckerModern:
        x *= 0.98f;
        break;

    case PickupType::Active:
        x *= 0.95f;
        break;

    default:
        break;
    }

    return x;
}

float InputStage::getPickupCalibration() const noexcept
{
    switch (inputType)
    {
    case PickupType::SingleCoil:
        return 1.15f;

    case PickupType::P90:
        return 1.08f;

    case PickupType::HumbuckerVintage:
        return 1.00f;

    case PickupType::HumbuckerModern:
        return 0.92f;

    case PickupType::Active:
        return 0.85f;

    default:
        return 1.0f;
    }
}

void InputStage::updateVoicingFilter()
{

float freq = 3000.0f, gainDb = 0.0f, q = 0.7f;
switch (inputType)
{
  case PickupType::SingleCoil: freq = 4500.0f; gainDb = +2.5f; break;
  case PickupType::P90: freq = 1200.0f; gainDb = +2.0f; break;
  case PickupType::HumbuckerVintage: freq = 3000.0f; gainDb = -1.0f; break;
  case PickupType::HumbuckerModern: freq = 2000.0f; gainDb = +2.0f; break;
  case PickupType::Active: freq = 5000.0f; gainDb = +1.0f; break;
  default: break;
}

*voicingFilter.state =
    *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
         currentSampleRate, freq, q,
         juce::Decibels::decibelsToGain(gainDb));
}