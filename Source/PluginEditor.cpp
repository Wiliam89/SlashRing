#pragma once
#include "PluginProcessor.h"
#include "PluginEditor.h"

//============================================================
// CONSTRUCTOR
//============================================================

SlashRingAudioProcessorEditor::SlashRingAudioProcessorEditor(
    SlashRingAudioProcessor& p)
    : AudioProcessorEditor(&p),
    processor(p)
{
    setSize(1100, 650);

    //========================================================
    // CONFIGURE KNOBS
    //========================================================

    configureKnob(inputGainKnob);
    configureKnob(outputGainKnob);

    configureKnob(ampGainKnob);
    configureKnob(bassKnob);
    configureKnob(middleKnob);
    configureKnob(trebleKnob);
    configureKnob(presenceKnob);
    configureKnob(masterKnob);

    configureKnob(delayTimeKnob);
    configureKnob(delayFeedbackKnob);
    configureKnob(delayMixKnob);

    //========================================================
    // ADD COMPONENTS
    //========================================================

    addAndMakeVisible(inputGainKnob);
    addAndMakeVisible(outputGainKnob);

    addAndMakeVisible(ampGainKnob);
    addAndMakeVisible(bassKnob);
    addAndMakeVisible(middleKnob);
    addAndMakeVisible(trebleKnob);
    addAndMakeVisible(presenceKnob);
    addAndMakeVisible(masterKnob);

    addAndMakeVisible(delayTimeKnob);
    addAndMakeVisible(delayFeedbackKnob);
    addAndMakeVisible(delayMixKnob);

    addAndMakeVisible(overdriveButton);
    addAndMakeVisible(cabinetButton);
    addAndMakeVisible(reverbButton);
    addAndMakeVisible(delayButton);

    //========================================================
    // BUTTON TEXT
    //========================================================

    overdriveButton.setButtonText("OVERDRIVE");
    cabinetButton.setButtonText("CABINET");
    reverbButton.setButtonText("REVERB");
    delayButton.setButtonText("DELAY");

    //========================================================
    // ATTACHMENTS
    //========================================================

    auto& state = processor.getAPVTS();

    inputGainAttachment =
        std::make_unique<SliderAttachment>(
            state,
            ParameterID::inputGain,
            inputGainKnob);

    outputGainAttachment =
        std::make_unique<SliderAttachment>(
            state,
            ParameterID::outputGain,
            outputGainKnob);

    ampGainAttachment =
        std::make_unique<SliderAttachment>(
            state,
            ParameterID::ampGain,
            ampGainKnob);

    bassAttachment =
        std::make_unique<SliderAttachment>(
            state,
            ParameterID::bass,
            bassKnob);

    middleAttachment =
        std::make_unique<SliderAttachment>(
            state,
            ParameterID::middle,
            middleKnob);

    trebleAttachment =
        std::make_unique<SliderAttachment>(
            state,
            ParameterID::treble,
            trebleKnob);

    presenceAttachment =
        std::make_unique<SliderAttachment>(
            state,
            ParameterID::presence,
            presenceKnob);

    masterAttachment =
        std::make_unique<SliderAttachment>(
            state,
            ParameterID::master,
            masterKnob);

    overdriveAttachment =
        std::make_unique<ButtonAttachment>(
            state,
            ParameterID::overdriveOn,
            overdriveButton);

    cabinetAttachment =
        std::make_unique<ButtonAttachment>(
            state,
            ParameterID::cabinetOn,
            cabinetButton);

    reverbAttachment =
        std::make_unique<ButtonAttachment>(
            state,
            ParameterID::reverbOn,
            reverbButton);

    delayTimeAttachment =
        std::make_unique<SliderAttachment>(
            state,
            ParameterID::delayTime,
            delayTimeKnob);

    delayFeedbackAttachment =
        std::make_unique<SliderAttachment>(
            state,
            ParameterID::delayFeedback,
            delayFeedbackKnob);

    delayMixAttachment =
        std::make_unique<SliderAttachment>(
            state,
            ParameterID::delayMix,
            delayMixKnob);

    delayAttachment =
        std::make_unique<ButtonAttachment>(
            state,
            ParameterID::delayOn,
            delayButton);
}



//============================================================
// DESTRUCTOR
//============================================================

SlashRingAudioProcessorEditor::~SlashRingAudioProcessorEditor() = default;

//============================================================
// KNOB CONFIG
//============================================================

void SlashRingAudioProcessorEditor::configureKnob(
    juce::Slider& slider)
{
    slider.setSliderStyle(
        juce::Slider::RotaryHorizontalVerticalDrag);

    slider.setTextBoxStyle(
        juce::Slider::TextBoxBelow,
        false,
        70,
        20);
}

//============================================================
// PAINT
//============================================================

void SlashRingAudioProcessorEditor::paint(
    juce::Graphics& g)
{
    g.fillAll(juce::Colour(20, 20, 20));

    g.setColour(juce::Colours::white);
    g.setFont(32.0f);

    g.drawFittedText(
        "SlashRing",
        0,
        20,
        getWidth(),
        40,
        juce::Justification::centred,
        1);
}

//============================================================
// LAYOUT
//============================================================

void SlashRingAudioProcessorEditor::resized()
{
    const int knobSize = 110;
    const int topY = 120;
    const int secondRowY = 320;

    inputGainKnob.setBounds(60, topY, knobSize, knobSize);
    outputGainKnob.setBounds(190, topY, knobSize, knobSize);

    ampGainKnob.setBounds(360, topY, knobSize, knobSize);
    bassKnob.setBounds(490, topY, knobSize, knobSize);
    middleKnob.setBounds(620, topY, knobSize, knobSize);
    trebleKnob.setBounds(750, topY, knobSize, knobSize);
    presenceKnob.setBounds(880, topY, knobSize, knobSize);

    masterKnob.setBounds(490, secondRowY, knobSize, knobSize);

    overdriveButton.setBounds(120, 520, 140, 30);
    cabinetButton.setBounds(430, 520, 140, 30);
    reverbButton.setBounds(740, 520, 140, 30);

    delayTimeKnob.setBounds(120, 320, knobSize, knobSize);
    delayFeedbackKnob.setBounds(250, 320, knobSize, knobSize);
    delayMixKnob.setBounds(380, 320, knobSize, knobSize);

    delayButton.setBounds(900, 520, 140, 30);
}