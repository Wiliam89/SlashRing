#pragma once

#include <JuceHeader.h>

//================================================================
// OverdriveModule
//
// Single-stage boost/overdrive pedal, designed to push AmpModule
// (JCM800-inspired) rather than act as a self-contained distortion
// source. AmpModule remains the primary source of amp-style
// saturation in the SlashRing signal chain.
//
// TD-010 REVISION (gain-staging root-cause fix):
//   - Removed redundant double application of the drive value as
//     gain (previously applied once in an input-boost stage and
//     again as a raw multiply before clipping).
//   - Removed two of the three cascaded independent tanh stages;
//     the module now uses exactly ONE fixed-shape nonlinearity fed
//     by exactly ONE drive-controlled pre-gain stage.
//   - Fixed a stereo-unsafe smoothing bug where driveSmoothed /
//     levelSmoothed were advanced once per (channel, sample) pair
//     instead of once per sample, causing channel L and channel R
//     to consume different points along the same smoothing ramp.
//
// See TECHNICAL_DEBT.md (TD-010) for full audit history.
//================================================================

class OverdriveModule
{
public:
    OverdriveModule();
    ~OverdriveModule();

    void prepare(
        double sampleRate,
        int samplesPerBlock,
        int numChannels);

    void process(
        juce::dsp::AudioBlock<float>& block);

    //========================================================
    // PARAMETERS
    //========================================================

    void setDrive(float newDrive);
    void setLevel(float newLevelDb);
    void setTone(float newTone);

    //========================================================
    // LIFECYCLE (TD-004)
   //========================================================
    void reset();
    void release();

    //========================================================
    // TD-010 GAIN MAP B — TEMPORARY DIAGNOSTIC
    //
    // Read-only, observational only. Returns the CURRENT
    // (non-advancing) value of the internal drive smoother
    // at the moment of query, via getCurrentValue().
    //
    // Under the TD-010 drive-law revision, this now reflects
    // the single-stage dB-mapped drive gain (see setDrive())
    // rather than the previous two-stage linear-gain law. The
    // getter's contract is otherwise unchanged: it does NOT
    // represent every sample-by-sample value consumed during
    // the block being measured — only the smoother's state at
    // the instant this is called. It does NOT call
    // getNextValue() and does NOT advance or otherwise modify
    // smoother state.
    //
    // Diagnostic-only. Remove after TD-010 audit concludes.
    //========================================================
    float getCurrentDriveValue() const noexcept
    {
        return driveSmoothed.getCurrentValue();
    }

private:

    //========================================================
    // FILTERS
    //========================================================

    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> inputHPF;

    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> outputLPF;

    //========================================================
    // SMOOTHING
    //
    // TD-010: advanced exactly once per sample (not once per
    // channel) in process(). Both channels consume the same
    // trajectory. See process() implementation.
    //========================================================

    juce::SmoothedValue<float> driveSmoothed;
    juce::SmoothedValue<float> levelSmoothed;

    //========================================================
    // STATE
    //========================================================

    float drive = 35.0f;

    float driveGain = 1.0f;

    float outputLevelDb = 0.0f;

    float outputGainLinear = 1.0f;

    double currentSampleRate = 44100.0;

    // Tone (LPF de saida controlavel) — NOVO
    float toneHz = 5600.0f;

    //========================================================
    // FIXED NONLINEARITY SHAPE (TD-010)
    //
    // Deliberately NOT drive-dependent. The drive parameter
    // controls only the pre-gain feeding this fixed curve, so
    // increasing drive pushes more signal into an unchanging,
    // well-behaved shape rather than changing the shape itself
    // at multiple cascaded points. Mild asymmetry gives a
    // tube-like even-harmonic bias without a hard fuzz edge.
    //========================================================

    static constexpr float positiveShapeCoefficient = 1.35f;
    static constexpr float negativeShapeCoefficient = 1.05f;

    //========================================================
    // INTERNAL
    //========================================================

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        OverdriveModule)
};