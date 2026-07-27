/*
  ==============================================================================

    CabinetEngine.h
    Created: 18 Jul 2026 6:09:36pm
    Author:  wilia

  ==============================================================================
*/

#pragma once

#include "CabinetVoice.h"

/**
   @class CabinetEngine
   @brief Fachada controladora principal (Master) exposta para o core do SlashRing.
   
   Suporta arquitetura Dual-Cab nativa criando e misturando duas instâncias de CabinetVoice independentes.
*/
class CabinetEngine final
{
public:
    CabinetEngine();
    ~CabinetEngine() = default;

    void prepare (const juce::dsp::ProcessSpec& spec) noexcept;
    void reset() noexcept;

    CabinetVoice& getVoice (int voiceIndex) noexcept { return (voiceIndex == 0) ? voiceA : voiceB; }

    void setDualCabMode (bool enabled) noexcept { dualCabEnabled = enabled; }
    void setMasterMix (float mix0to1) noexcept   { masterMix = juce::jlimit (0.0f, 1.0f, mix0to1); }

    void process (juce::dsp::ProcessContextReplacing<float>& context) noexcept;

private:
    // CORREÇÃO INTEGRAL: Inicialização das vozes atualizada para o construtor padrão compatível.
    CabinetVoice voiceA;
    CabinetVoice voiceB;

    bool dualCabEnabled = false;
    float masterMix = 0.5f;

    juce::AudioBuffer<float> parallelBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CabinetEngine)
};