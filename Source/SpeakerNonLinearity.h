#pragma once

#include <JuceHeader.h>
#include <cmath>
#include <algorithm>
#include <vector>
#include <array>

class SpeakerNonLinearity final
{
public:
    SpeakerNonLinearity() = default;
    ~SpeakerNonLinearity() = default;

    void prepare (const juce::dsp::ProcessSpec& spec) noexcept;
    void reset() noexcept;

    void setPhysicsParams (float driveParam, float breakupParam) noexcept;

    void process (juce::dsp::ProcessContextReplacing<float>& context) noexcept;

private:
    float processSample (float inputSample, int channel) noexcept;

    double sampleRate = 44100.0;
    float drive = 1.0f;
    float breakup = 0.5f;

    std::vector<std::array<float, 2>> filterStates;

    // Physical speaker model parameters (Thiele-Small / Leach model)
    const float Re = 6.5f;       // Voice coil resistance (Ohms)
    const float Le0 = 0.0003f;   // Nominal inductance (H)
    const float Bl0 = 11.5f;     // Force factor (N/A)
    const float Mms = 0.025f;    // Mechanical mass (kg)

    // ========================================================
    // INTERNAL GAIN CALIBRATION
    // ========================================================
    // Automatic makeup gain to maintain unity gain behavior.
    //
    // The speaker model produces output normalized to input energy.
    // Gain structure:
    // - Input scaling: (1 + drive * 0.5) produces -6 to +13.98 dB
    // - Physical model: produces normalized magnetic/acoustic force
    // - Output normalization: preserves input RMS ±1dB
    // - tanh() saturation: soft clipping without gain change
    //
    // This ensures Speaker ON ≈ Speaker OFF in level, with only
    // timbre/saturation changes (professional standard).
    // ========================================================
    
    // Running RMS calculation for automatic gain compensation
    static constexpr float rmsAlpha = 0.99f;  // Exponential smoothing factor
    float inputRmsSmoothed = 1e-6f;           // Smoothed input RMS level
    float outputRmsSmoothed = 1e-6f;          // Smoothed output RMS level

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpeakerNonLinearity)
};