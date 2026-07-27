#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

//============================================================

class SlashRingAudioProcessorEditor final
    : public juce::AudioProcessorEditor
{
public:
    SlashRingAudioProcessorEditor(SlashRingAudioProcessor&);
    ~SlashRingAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    //========================================================
    // REFERENCE
    //========================================================

    SlashRingAudioProcessor& processor;

    //========================================================
    // UI COMPONENTS
    //========================================================

    juce::Slider inputGainKnob;
    juce::Slider outputGainKnob;

    juce::Slider ampGainKnob;
    juce::Slider bassKnob;
    juce::Slider middleKnob;
    juce::Slider trebleKnob;
    juce::Slider presenceKnob;
    juce::Slider masterKnob;

    juce::Slider delayTimeKnob;
    juce::Slider delayFeedbackKnob;
    juce::Slider delayMixKnob;

    juce::ToggleButton overdriveButton;
    juce::ToggleButton cabinetButton;
    juce::ToggleButton reverbButton;
    juce::ToggleButton delayButton;

    //========================================================
    // ATTACHMENTS
    //========================================================

    using SliderAttachment =
        juce::AudioProcessorValueTreeState::SliderAttachment;

    using ButtonAttachment =
        juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> inputGainAttachment;
    std::unique_ptr<SliderAttachment> outputGainAttachment;

    std::unique_ptr<SliderAttachment> ampGainAttachment;
    std::unique_ptr<SliderAttachment> bassAttachment;
    std::unique_ptr<SliderAttachment> middleAttachment;
    std::unique_ptr<SliderAttachment> trebleAttachment;
    std::unique_ptr<SliderAttachment> presenceAttachment;
    std::unique_ptr<SliderAttachment> masterAttachment;

    std::unique_ptr<SliderAttachment> delayTimeAttachment;
    std::unique_ptr<SliderAttachment> delayFeedbackAttachment;
    std::unique_ptr<SliderAttachment> delayMixAttachment;

    std::unique_ptr<ButtonAttachment> overdriveAttachment;
    std::unique_ptr<ButtonAttachment> cabinetAttachment;
    std::unique_ptr<ButtonAttachment> reverbAttachment;
    std::unique_ptr<ButtonAttachment> delayAttachment;

    //========================================================
    // HELPERS
    //========================================================

    void configureKnob(juce::Slider& slider);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        SlashRingAudioProcessorEditor)
};