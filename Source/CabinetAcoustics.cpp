#include "CabinetAcoustics.h"

void CabinetAcoustics::prepare (const juce::dsp::ProcessSpec& spec) noexcept
{
    sampleRate = spec.sampleRate;
    
    // Redimensiona o histórico de delays para bater com o número dinâmico de canais
    filterDelays.resize (spec.numChannels);
    reset();
    updateCoefficients();
}

void CabinetAcoustics::reset() noexcept
{
    for (auto& channelDelays : filterDelays)
        for (auto& delays : channelDelays)
            delays.fill (0.0f);
}

void CabinetAcoustics::setCabinetSize (float size) noexcept
{
    if (cabinetSize != size)
    {
        cabinetSize = size;
        updateCoefficients();
    }
}

void CabinetAcoustics::setResonanceDepth (float depth) noexcept
{
    if (resonanceDepth != depth)
    {
        resonanceDepth = depth;
        updateCoefficients();
    }
}

void CabinetAcoustics::updateCoefficients() noexcept
{
    if (sampleRate <= 0.0) return;

    for (int i = 0; i < numFilters; ++i)
    {
        // Escala a frequência base do biquad proporcionalmente ao tamanho do gabinete
        float scaledFreq = baseFrequencies[static_cast<size_t>(i)] / juce::jmax (0.1f, cabinetSize);
        scaledFreq = juce::jlimit (20.0f, static_cast<float> (sampleRate * 0.49), scaledFreq);
        
        float q = baseQFactors[static_cast<size_t>(i)] * (0.5f + resonanceDepth);

        float omega = 2.0f * juce::MathConstants<float>::pi * scaledFreq / static_cast<float> (sampleRate);
        float alpha = std::sin (omega) / (2.0f * q);
        float cosOmega = std::cos (omega);

        float a0 = 1.0f + alpha;
        
        activeCoefficients[static_cast<size_t>(i)].b0 = alpha / a0;
        activeCoefficients[static_cast<size_t>(i)].b1 = 0.0f;
        activeCoefficients[static_cast<size_t>(i)].b2 = -alpha / a0;
        activeCoefficients[static_cast<size_t>(i)].a1 = -2.0f * cosOmega / a0;
        activeCoefficients[static_cast<size_t>(i)].a2 = (1.0f - alpha) / a0;
    }
}

void CabinetAcoustics::process (juce::dsp::ProcessContextReplacing<float>& context) noexcept
{
    if (context.isBypassed) return;

    auto block = context.getOutputBlock();
    size_t numChannels = block.getNumChannels();
    size_t numSamples = block.getNumSamples();

    for (size_t ch = 0; ch < numChannels; ++ch)
    {
        float* data = block.getChannelPointer (ch);
        auto& channelDelays = filterDelays[ch];

        for (size_t i = 0; i < numSamples; ++i)
        {
            float input = data[i];

            // Cascata de filtros biquad lineares
            for (int f = 0; f < numFilters; ++f)
            {
                const auto& c = activeCoefficients[static_cast<size_t>(f)];
                auto& d = channelDelays[static_cast<size_t>(f)];

                float output = c.b0 * input + c.b1 * d[0] + c.b2 * d[1] - c.a1 * d[2] - c.a2 * d[3];

                d[1] = d[0];
                d[0] = input;
                d[3] = d[2];
                d[2] = output;

                input = output; // O output vira entrada para o próximo filtro na cascata
            }

            data[i] = input;
        }
    }
}