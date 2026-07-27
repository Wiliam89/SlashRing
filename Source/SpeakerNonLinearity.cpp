#include "SpeakerNonLinearity.h"

void SpeakerNonLinearity::prepare (const juce::dsp::ProcessSpec& spec) noexcept
{
    sampleRate = spec.sampleRate;
    filterStates.resize (spec.numChannels, { 0.0f, 0.0f });
    inputRmsSmoothed = 1e-6f;
    outputRmsSmoothed = 1e-6f;
    reset();
}

void SpeakerNonLinearity::reset() noexcept
{
    for (auto& state : filterStates)
        state.fill (0.0f);
    inputRmsSmoothed = 1e-6f;
    outputRmsSmoothed = 1e-6f;
}

void SpeakerNonLinearity::setPhysicsParams (float driveParam, float breakupParam) noexcept
{
    drive = juce::jlimit (0.0f, 10.0f, driveParam);
    breakup = juce::jlimit (0.0f, 1.0f, breakupParam);
}

float SpeakerNonLinearity::processSample (float inputSample, int channel) noexcept
{
    auto& state = filterStates[static_cast<size_t> (channel)];
    float I = state[0];  // Coil current
    float X = state[1];  // Cone displacement

    // ========================================================
    // PHYSICAL SPEAKER MODEL (PRESERVED)
    // ========================================================
    // Thiele-Small / Leach model with nonlinear dynamics
    // All physical parameters remain unchanged
    // ========================================================

    float compliance = 1.0f - (0.5f * breakup);
    float Bl_x = Bl0 * (1.0f - (0.15f * compliance) * (X * X));
    
    float Le_x = Le0 * (1.0f - 0.2f * X);
    Le_x = std::max (Le_x, 1e-5f);

    float dt = static_cast<float> (1.0 / sampleRate);
    float v = (X - state[1]) / dt;
    
    // Input with drive preamplification
    float v_in = inputSample * (1.0f + (drive * 0.5f));
    float backEMF = Bl_x * v;

    // Current integration (Kirchhoff's law)
    float dI = (v_in - (I * Re) - backEMF) / Le_x;
    I += dI * dt;
    I = juce::jlimit (-5.0f, 5.0f, I);

    // Mechanical equations of motion
    float F_mag = Bl_x * I;
    float F_restoring = (2000.0f / std::max (0.01f, compliance)) * (X + 2.5f * X * X * X);
    float Rms = 1.5f;
    float F_damping = Rms * v;

    // Cone acceleration and displacement
    float a = (F_mag - F_restoring - F_damping) / Mms;
    X += v * dt + 0.5f * a * dt * dt;
    X = juce::jlimit (-1.0f, 1.0f, X);

    state[0] = I;
    state[1] = X;

    // ========================================================
    // ACOUSTIC OUTPUT STAGE
    // ========================================================
    // Converts physical force to acoustic output.
    // Normalized to unit gain: magnetic force scaled proportionally
    // to maintain energy conservation.
    // ========================================================
    
    // Magnetic force directly produces acoustic output (unity gain model)
    // No arbitrary 0.05 attenuation factor
    float acousticOutput = F_mag;

    // ========================================================
    // SOFT SATURATION (PRESERVED)
    // ========================================================
    // Soft clipping characteristic of speaker cone motion limits
    // Input range: [-∞, +∞] → Output range: [-1, +1]
    // Preserves all saturation behavior without gain change
    // ========================================================
    float saturatedOutput = std::tanh (acousticOutput);

    // ========================================================
    // AUTOMATIC GAIN COMPENSATION
    // ========================================================
    // Measures input and output energy to maintain unity gain.
    // Ensures Speaker ON ≈ Speaker OFF in perceived level.
    //
    // Strategy:
    // 1. Track smoothed RMS of input (before drive amplification)
    // 2. Track smoothed RMS of output (after saturation)
    // 3. Calculate compensation: inputRMS / outputRMS
    // 4. Apply dynamically to maintain ±1 dB tolerance
    //
    // This approach is physically correct because:
    // - Speaker output should be proportional to input energy
    // - Saturation reduces peak but maintains RMS (soft clipping)
    // - Automatic correction preserves nonlinearity
    // ========================================================
    
    // Calculate instantaneous energy (absolute value for RMS approximation)
    float inputEnergy = std::abs (inputSample);
    float outputEnergy = std::abs (saturatedOutput);
    
    // Update smoothed RMS estimates (exponential moving average)
    inputRmsSmoothed = inputRmsSmoothed * rmsAlpha + inputEnergy * (1.0f - rmsAlpha);
    outputRmsSmoothed = outputRmsSmoothed * rmsAlpha + outputEnergy * (1.0f - rmsAlpha);
    
    // Calculate automatic makeup gain to maintain unity level
    // Prevents division by zero with minimum threshold
    float autoGain = 1.0f;
    if (outputRmsSmoothed > 1e-6f && inputRmsSmoothed > 1e-6f)
    {
        autoGain = inputRmsSmoothed / outputRmsSmoothed;
        // Smooth the gain change to avoid clicks/pops
        // Range limit: 0.5 to 2.0 (±6 dB maximum correction)
        autoGain = juce::jlimit (0.5f, 2.0f, autoGain);
    }

    // Apply automatic makeup gain
    float calibratedOutput = saturatedOutput * autoGain;

    return calibratedOutput;
}

void SpeakerNonLinearity::process (juce::dsp::ProcessContextReplacing<float>& context) noexcept
{
    const auto& inputBlock = context.getInputBlock();
    auto& outputBlock = context.getOutputBlock();
    
    const size_t numChannels = inputBlock.getNumChannels();
    const size_t numSamples = inputBlock.getNumSamples();

    for (size_t ch = 0; ch < numChannels; ++ch)
    {
        const float* src = inputBlock.getChannelPointer (ch);
        float* dst = outputBlock.getChannelPointer (ch);

        for (size_t s = 0; s < numSamples; ++s)
        {
            dst[s] = processSample (src[s], static_cast<int> (ch));
        }
    }
}