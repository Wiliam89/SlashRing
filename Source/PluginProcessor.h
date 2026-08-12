#pragma once
#include <JuceHeader.h>

//============================================================
// FORWARD DECLARATIONS
//============================================================

class InputStage;
class AmpModule;
class CabinetModule;
class OverdriveModule;
class ReverbModule;
class DelayModule;
class OutputStage;

//============================================================
// PARAMETER IDS
//============================================================

namespace ParameterID
{
    static constexpr auto inputGain = "input_gain";
    static constexpr auto outputGain = "output_gain";

    static constexpr auto ampGain = "amp_gain";
    static constexpr auto bass = "bass";
    static constexpr auto middle = "middle";
    static constexpr auto treble = "treble";
    static constexpr auto presence = "presence";
    static constexpr auto master = "master";

    static constexpr auto overdriveOn = "overdrive_on";
    static constexpr auto overdriveDrive = "overdrive_drive";
    static constexpr auto overdriveLevel = "overdrive_level";
    static constexpr auto overdriveTone = "overdrive_tone";

    static constexpr auto cabinetOn = "cabinet_on";
    static constexpr auto cabinetModel = "cabinet_model";   // NOVO: menu de cabinets
    static constexpr auto cabinetLowCut = "cabinet_low_cut";
    static constexpr auto cabinetHighCut = "cabinet_high_cut";
    static constexpr auto cabinetLevel = "cabinet_level";
    static constexpr auto cabinetMix = "cabinet_mix";

    static constexpr auto reverbOn = "reverb_on";
    static constexpr auto reverbMix = "reverb_mix";

    static constexpr auto delayOn = "delay_on";
    static constexpr auto delayTime = "delay_time";
    static constexpr auto delayFeedback = "delay_feedback";
    static constexpr auto delayMix = "delay_mix";

    static constexpr auto inputType = "input_type";
}

//============================================================
// NUMERO DE CABINETS NO MENU
// Mude aqui se quiser mais/menos slots. Precisa bater com a
// lista de nomes (createParameterLayout), com a tabela de IRs
// (loadCabinetIR) e com o ComboBox do editor.
//============================================================
static constexpr int kNumCabinets = 6;

//============================================================
// MAIN PROCESSOR
//============================================================

class SlashRingAudioProcessor final : public juce::AudioProcessor,
                                      private juce::AudioProcessorValueTreeState::Listener,
                                      private juce::AsyncUpdater
{
public:
    //========================================================
    // LIFECYCLE
    //========================================================

    SlashRingAudioProcessor();
    ~SlashRingAudioProcessor() override;

    //========================================================
    // AUDIO ENGINE
    //========================================================

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    //========================================================
    // EDITOR
    //========================================================

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //========================================================
    // PLUGIN INFO
    //========================================================

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //========================================================
    // PROGRAMS
    //========================================================

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //========================================================
    // STATE SAVE / LOAD
    //========================================================

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //========================================================
    // PUBLIC ACCESS
    //========================================================

    juce::AudioProcessorValueTreeState& getAPVTS() noexcept
    {
        return apvts;
    }

private:
    //========================================================
    // PARAMETER SYSTEM
    //========================================================

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts;

    //========================================================
    // MENU DE CABINETS (troca de IR)
    //
    // A troca NUNCA e feita na thread de audio. Um listener de
    // parametro anota o cabinet desejado e acorda o AsyncUpdater;
    // o carregamento real acontece em handleAsyncUpdate() (thread
    // de mensagens), que e o unico lugar seguro para chamar o
    // carregador de IR (ele usa lock + ThreadPool).
    //========================================================

    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void handleAsyncUpdate() override;
    void loadCabinetIR(int modelIndex);

    std::atomic<int> desiredCabinetModel { 0 };
    int loadedCabinetModel = -1;

    //========================================================
    // DSP MODULE OWNERSHIP
    //========================================================

    std::unique_ptr<InputStage>      inputStage;
    std::unique_ptr<AmpModule>       ampModule;
    std::unique_ptr<CabinetModule>   cabinetModule;
    std::unique_ptr<OverdriveModule> overdriveModule;
    std::unique_ptr<DelayModule>     delayModule;
    std::unique_ptr<ReverbModule>    reverbModule;
    std::unique_ptr<OutputStage>     outputStage;

    //========================================================
    // GLOBAL ENGINE SYSTEMS
    //
    // Oversampling JUCE: construtor (numCanais, factor, tipo).
    //   1o valor = 2  -> 2 canais (estereo)
    //   2o valor = 2  -> factor 2 => taxa 2^2 = 4x
    // Ou seja, o plugin JA roda em 4x oversampling. Para 8x
    // (menos aliasing, mais CPU) troque o 2o valor por 3.
    //========================================================

    juce::dsp::Oversampling<float> oversampling
    {
        2,
        3,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR
    };

    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;

    //========================================================
    // INTERNAL HELPERS
    //========================================================

    void prepareDSP(double sampleRate, int samplesPerBlock);

    // TD-002: base-rate and oversampled domains are prepared
    // with independent ProcessSpec configurations, per
    // ARCHITECTURE.md "Oversampling Architecture".
    void prepareBaseRateModules(const juce::dsp::ProcessSpec& baseSpec);
    void prepareOversampledModules(const juce::dsp::ProcessSpec& oversampledSpec);

    void updateParameterState();

    //========================================================
    // SAFETY
    //========================================================

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SlashRingAudioProcessor)
};
