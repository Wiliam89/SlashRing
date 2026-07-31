#include "OverdriveModule.h"

//================================================================
// TD-010 REVISION — see OverdriveModule.h for full rationale.
// Summary of what changed vs. the prior implementation:
//   1. Drive law: single dB-linear map (0 dB..+30 dB), replacing
//      the previous linear-gain map (1x..40x) that was applied
//      TWICE per sample.
//   2. Topology: one pre-gain stage feeding one fixed-shape
//      asymmetric tanh nonlinearity. The prior three cascaded,
//      independently-driven tanh stages have been removed.
//   3. Smoothing: driveSmoothed / levelSmoothed are now advanced
//      exactly once per sample, identical trajectory applied to
//      every channel (previously advanced once per (channel,
//      sample) pair, desynchronizing L/R).
// AmpModule is unmodified by this change.
//================================================================

OverdriveModule::OverdriveModule()
{
}

OverdriveModule::~OverdriveModule()
{
}

void OverdriveModule::prepare(
    double sampleRate,
    int samplesPerBlock,
    int numChannels)
{
    currentSampleRate = sampleRate;

    driveSmoothed.reset(
        sampleRate,
        0.02);

    levelSmoothed.reset(
        sampleRate,
        0.02);

    driveSmoothed.setCurrentAndTargetValue(
        driveGain);

    levelSmoothed.setCurrentAndTargetValue(
        outputGainLinear);

    juce::dsp::ProcessSpec spec;

    spec.sampleRate =
        sampleRate;

    spec.maximumBlockSize =
        static_cast<juce::uint32>(
            samplesPerBlock);

    spec.numChannels =
        static_cast<juce::uint32>(
            numChannels);

    inputHPF.reset();
    outputLPF.reset();

    inputHPF.prepare(spec);
    outputLPF.prepare(spec);


    *inputHPF.state =
      *juce::dsp::IIR::Coefficients<float>::makeHighPass(
           sampleRate,
           250.0f);

    *outputLPF.state =
       *juce::dsp::IIR::Coefficients<float>::makeLowPass(
            sampleRate,
            toneHz);

     reset();
}

void OverdriveModule::reset()
{
    inputHPF.reset();
    outputLPF.reset();

    driveSmoothed.setCurrentAndTargetValue(
        driveGain);

    levelSmoothed.setCurrentAndTargetValue(
        outputGainLinear);
}

void OverdriveModule::release()
{
}

void OverdriveModule::setDrive(
    float newDrive)
{
    drive =
        juce::jlimit(
            0.0f,
            100.0f,
            newDrive);

    //========================================================
    // DRIVE LAW (TD-010 REVISION)
    //
    // Single dB-linear mapping. This is the ONLY gain stage
    // feeding the nonlinearity in process() — no secondary
    // multiplication by drive occurs anywhere else in this
    // module.
    //
    //   drive =   0  -> 0.0 dB  (1.00x)  unity, minimal color
    //   drive = 100  -> +30.0 dB (~31.6x) singly-staged drive
    //
    // Replaces the previous two-stage linear-gain law (1x-40x
    // applied twice per sample), which produced uncontrolled
    // cumulative pre-clip gain and forced the clipping stage
    // deep into saturation even at moderate drive settings.
    //
    // +30 dB ceiling keeps this module functioning as a boost
    // /overdrive pedal that pushes AmpModule, consistent with
    // product intent (AmpModule remains the primary amp-style
    // distortion source; see ARCHITECTURE.md / PRODUCT_SPEC).
    //========================================================

    constexpr float minDriveDb = 0.0f;
    constexpr float maxDriveDb = 30.0f;

    const float driveDb =
        juce::jmap(
            drive,
            0.0f,
            100.0f,
            minDriveDb,
            maxDriveDb);

    driveGain =
        juce::Decibels::decibelsToGain(
            driveDb);

    driveSmoothed.setTargetValue(
        driveGain);
}

void OverdriveModule::setLevel(
    float newLevelDb)
{
    outputLevelDb =
        juce::jlimit(
            -60.0f,
            12.0f,
            newLevelDb);

    outputGainLinear =
        juce::Decibels::decibelsToGain(
            outputLevelDb);

    levelSmoothed.setTargetValue(
        outputGainLinear);
}

void OverdriveModule::setTone(
 float newTone)
{
     const float t =
    juce::jlimit(0.0f, 10.0f, newTone);

    // 0..10 -> 2 kHz .. 8 kHz
    const float hz =
    juce::jmap(t, 0.0f, 10.0f, 2000.0f, 8000.0f);

   // So recomputa se mudou (evita alocacao por bloco)
   if (std::abs(hz - toneHz) < 0.5f)
    return;
    toneHz = hz;
    *outputLPF.state =
    *juce::dsp::IIR::Coefficients<float>::makeLowPass(
    currentSampleRate,
    toneHz);
}

void OverdriveModule::process(
    juce::dsp::AudioBlock<float>& block)
{
    juce::ScopedNoDenormals noDenormals;

    {
        juce::dsp::ProcessContextReplacing<float> context(block);

        inputHPF.process(context);
    }

    const auto numChannels =
        block.getNumChannels();

    const auto numSamples =
        block.getNumSamples();

    //========================================================
    // TD-010 STEREO-SAFE SMOOTHING
    //
    // Sample-outer, channel-inner loop order. Each smoothed
    // value is advanced exactly ONCE per sample (not once per
    // channel), and the same value is applied identically to
    // every channel for that sample. This replaces the prior
    // channel-outer loop, which advanced the ramp once per
    // (channel, sample) pair and caused channel R to consume a
    // different segment of the ramp than channel L.
    //========================================================

    for (size_t sample = 0;
        sample < numSamples;
        ++sample)
    {
        const float currentDriveGain =
            driveSmoothed.getNextValue();

        const float currentLevel =
            levelSmoothed.getNextValue();

        for (size_t channel = 0;
            channel < numChannels;
            ++channel)
        {
            auto* samples =
                block.getChannelPointer(channel);

            float x =
                samples[sample];

            //====================================================
            // STAGE 1
            // SINGLE GAIN-INTO-SATURATION STAGE
            //
            // The mapped drive gain is applied exactly once. No
            // secondary multiplication by drive occurs anywhere
            // else in this function.
            //====================================================

            x *= currentDriveGain;

            //====================================================
            // STAGE 2
            // SINGLE PROGRESSIVE NONLINEARITY
            //
            // Fixed-shape asymmetric soft clip. Curve steepness
            // and asymmetry are FIXED constants, independent of
            // drive, so increasing drive pushes more signal into
            // an unchanging, well-behaved curve rather than
            // reshaping the curve itself. This replaces the prior
            // three cascaded, independently-driven tanh stages.
            //====================================================

            x =
                (x >= 0.0f)
                ? std::tanh(x * positiveShapeCoefficient)
                : std::tanh(x * negativeShapeCoefficient);

            //====================================================
            // STAGE 3
            // OUTPUT LEVEL (user trim, unchanged semantics)
            //====================================================

            x *= currentLevel;

            samples[sample] = x;
        }
    }

    {
        juce::dsp::ProcessContextReplacing<float> context(block);

        outputLPF.process(context);
    }
}