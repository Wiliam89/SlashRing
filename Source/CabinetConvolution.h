/*
  ==============================================================================

    CabinetConvolution.h
    Created: 18 Jul 2026 6:11:37pm
    Author:  wilia

  ==============================================================================
*/

#pragma once

#include "CabinetDataTypes.h"
#include <juce_dsp/juce_dsp.h>

/**
   @class CabinetConvolution
   @brief Encapsulador matemático de alta performance para convolução de latência zero.
   
   Utiliza a engine estruturada de FFT particionada (juce::dsp::Convolution) para processamento 
   no domínio da frequência sem adicionar overhead ou atrasos ao vetor de áudio principal.
*/
class CabinetConvolution final
{
public:
    CabinetConvolution() = default;
    ~CabinetConvolution() = default;

    void prepare (const juce::dsp::ProcessSpec& spec) noexcept;
    void reset() noexcept;

    /** Atualiza o bloco convolutivo de forma síncrona com os dados limpos e preparados */
    void updateIR (CachedIRData::Ptr newIRData) noexcept;

    void process (juce::dsp::ProcessContextReplacing<float>& context) noexcept;

private:
    juce::dsp::Convolution convolutionEngine { juce::dsp::Convolution::NonUniform { 128 } };
    bool isLoaded = false;
    juce::CriticalSection engineLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CabinetConvolution)
};
