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
// MAIN PROCESSOR
//============================================================

class SlashRingAudioProcessor final : public juce::AudioProcessor
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
    //========================================================

    juce::dsp::Oversampling<float> oversampling
    {
        2,
        2,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR
    };

    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;

    //========================================================
    // TD-010 GAIN MAP B — TEMPORARY DIAGNOSTIC
    //
    // Log-throttle accumulator only. Not read by, or
    // exposed to, any DSP module. Used exclusively to
    // limit diagnostic log writes to ~2 lines/sec so the
    // log file stays readable during manual test-matrix
    // execution. Remove after TD-010 audit concludes.
    //========================================================
    double td010DiagAccumSamples = 0.0;

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