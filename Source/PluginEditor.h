#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
//====================================================================
// LOOK & FEEL — desenho central de todos os controles
//====================================================================
class SlashRingLookAndFeel : public juce::LookAndFeel_V4
{

public:
 SlashRingLookAndFeel();
 void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
 float sliderPos, float startAngle, float endAngle,

 juce::Slider&) override;
 void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
 bool shouldDrawButtonAsHighlighted,
 bool shouldDrawButtonAsDown) override;
 const juce::Colour accentColour { 0xffE0A24A }; // dourado
 const juce::Colour trackColour { 0xff3A352F };
 const juce::Colour knobColour { 0xff262220 };
 const juce::Colour textColour { 0xffEDE7DD };
 const juce::Colour ledColour { 0xffE0492E }; // vermelho quente
};
//====================================================================
// KNOB — slider rotativo + rótulo, num unico componente
//====================================================================
class Knob : public juce::Component
{
public:
 explicit Knob (const juce::String& caption);
 void resized() override;
 juce::Slider slider;
 juce::Label label;
};
//====================================================================
// EDITOR
//====================================================================
class SlashRingAudioProcessorEditor final : public juce::AudioProcessorEditor
{

public:
 explicit SlashRingAudioProcessorEditor (SlashRingAudioProcessor&);
 ~SlashRingAudioProcessorEditor() override;
 void paint (juce::Graphics&) override;
 void resized() override;
private:
 SlashRingAudioProcessor& processor;
 SlashRingLookAndFeel lnf;
 // ---- Controles ----
 juce::ComboBox inputTypeBox;
 Knob inputGain { "INPUT" };
 Knob outputGain { "OUTPUT" };

 juce::ToggleButton overdriveButton { "OVERDRIVE" };
 Knob odDrive { "DRIVE" };
 Knob odLevel { "LEVEL" };
 Knob odTone  { "TONE" };
 Knob ampGain { "GAIN" };
 Knob bass { "BASS" };
 Knob mid { "MIDDLE" };
 Knob treble { "TREBLE" };
 Knob presence { "PRESENCE" };
 Knob master { "MASTER" };

 juce::ToggleButton cabinetButton { "CABINET" };
 juce::ComboBox cabinetModelBox;          // NOVO: menu de cabinets
 Knob cabLowCut { "LOW CUT" };
 Knob cabHighCut { "HIGH CUT" };
 Knob cabLevel { "LEVEL" };
 Knob cabMix { "MIX" };

 juce::ToggleButton delayButton { "DELAY" };
 Knob delayTime { "TIME" };
 Knob delayFb { "FEEDBACK" };
 Knob delayMix { "MIX" };

 juce::ToggleButton reverbButton { "REVERB" };
 Knob reverbMix { "MIX" };

 // ---- Attachments (ligam controle <-> parametro) ----
 using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
 using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
 using ComboAtt = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

 std::unique_ptr<ComboAtt> inputTypeAtt;
 std::unique_ptr<ComboAtt> cabinetModelAtt;   // NOVO: menu de cabinets
 std::unique_ptr<SliderAtt> inputGainAtt, outputGainAtt;
 std::unique_ptr<SliderAtt> odDriveAtt, odLevelAtt, odToneAtt;
 std::unique_ptr<SliderAtt> ampGainAtt, bassAtt, midAtt, trebleAtt, presenceAtt, masterAtt;
 std::unique_ptr<SliderAtt> delayTimeAtt, delayFbAtt, delayMixAtt, reverbMixAtt;
 std::unique_ptr<SliderAtt> cabLowCutAtt, cabHighCutAtt, cabLevelAtt, cabMixAtt;
 std::unique_ptr<ButtonAtt> overdriveAtt, cabinetAtt, delayAtt, reverbAtt;

 // ---- Paineis das secoes (desenhados no paint) ----
 struct Section { juce::Rectangle<int> bounds; juce::String title; };
 std::vector<Section> sections;

 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SlashRingAudioProcessorEditor)
};
