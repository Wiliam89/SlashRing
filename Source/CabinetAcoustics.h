#pragma once
#include <JuceHeader.h>
#include <array>
#include <vector>

class CabinetAcoustics final
{
public:
    CabinetAcoustics() = default;
    ~CabinetAcoustics() = default;

    void prepare (const juce::dsp::ProcessSpec& spec) noexcept;
    void reset() noexcept;

    void setCabinetSize (float size) noexcept;
    void setResonanceDepth (float depth) noexcept;

    void process (juce::dsp::ProcessContextReplacing<float>& context) noexcept;

private:
    void updateCoefficients() noexcept;

    double sampleRate = 44100.0;
    float cabinetSize = 1.0f;
    float resonanceDepth = 0.5f;

    // Estrutura matemática para suportar o CabinetAcoustics.cpp original
    struct BiquadCoeffs {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
        float a1 = 0.0f, a2 = 0.0f;
    };

    static constexpr int numFilters = 3;
    std::array<float, numFilters> baseFrequencies { { 80.0f, 210.0f, 450.0f } };
    std::array<float, numFilters> baseQFactors { { 2.0f, 1.5f, 1.2f } };
    
    std::array<BiquadCoeffs, numFilters> activeCoefficients;
    
    // Estados dos filtros [Canal][Filtro][Histórico x1, x2, y1, y2]
    std::vector<std::array<std::array<float, 4>, numFilters>> filterDelays;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CabinetAcoustics)
};