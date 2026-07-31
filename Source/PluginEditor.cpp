#include "PluginEditor.h"

//====================================================================
// LOOK & FEEL
//====================================================================
SlashRingLookAndFeel::SlashRingLookAndFeel()
{
 setColour (juce::Slider::textBoxTextColourId, textColour);
 setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
 setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff17140F));
 setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff2A2622));
 setColour (juce::ComboBox::textColourId, textColour);
 setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff4A443C));
 setColour (juce::ComboBox::arrowColourId, accentColour);
 setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff201C18));
 setColour (juce::PopupMenu::textColourId, textColour);
 setColour (juce::PopupMenu::highlightedBackgroundColourId, accentColour.withAlpha (0.30f));
 setColour (juce::Label::textColourId, textColour);
}

void SlashRingLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
 float sliderPos, float startAngle, float endAngle,
 juce::Slider&)
{
 auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (6.0f);
 auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
 auto centre = bounds.getCentre();
 auto toAngle = startAngle + sliderPos * (endAngle - startAngle);
 auto lineW = radius * 0.14f;
 auto arcR = radius - lineW * 0.5f;

 // trilho de fundo (arco completo)
 juce::Path track;
 track.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
 g.setColour (trackColour);
 g.strokePath (track, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

 // arco de valor (do inicio ate a posicao atual)
 juce::Path value;
 value.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, toAngle, true);
 g.setColour (accentColour);
 g.strokePath (value, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
 // corpo do knob
 auto knobR = radius * 0.66f;
 juce::Rectangle<float> knob (0.0f, 0.0f, knobR * 2.0f, knobR * 2.0f);
 knob.setCentre (centre);
 g.setColour (knobColour);
 g.fillEllipse (knob);
 g.setColour (knobColour.brighter (0.18f));
 g.drawEllipse (knob, 1.0f);

 // ponteiro
 juce::Path pointer;
 const float thickness = 2.6f;
 pointer.addRoundedRectangle (-thickness * 0.5f, -knobR + 3.0f, thickness, knobR * 0.6f, 1.0f);
 pointer.applyTransform (juce::AffineTransform::rotation (toAngle).translated (centre.x, centre.y));
 g.setColour (accentColour.brighter (0.25f));
 g.fillPath (pointer);
}


void SlashRingLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b,
 bool shouldDrawButtonAsHighlighted, bool)
{
 auto bounds = b.getLocalBounds().toFloat().reduced (1.0f);
 const bool on = b.getToggleState();
 g.setColour (on ? accentColour.withAlpha (0.16f) : juce::Colour (0xff2A2622));
 g.fillRoundedRectangle (bounds, 5.0f);
 g.setColour (on ? accentColour : juce::Colour (0xff4A443C));
 g.drawRoundedRectangle (bounds, 5.0f, shouldDrawButtonAsHighlighted ? 1.6f : 1.0f);

 // LED
 auto led = juce::Rectangle<float> (0.0f, 0.0f, 9.0f, 9.0f)
 .withCentre ({ bounds.getX() + 15.0f, bounds.getCentreY() });
 if (on)
 {
 g.setColour (ledColour.withAlpha (0.35f));
 g.fillEllipse (led.expanded (3.5f));
 }
 g.setColour (on ? ledColour : juce::Colour (0xff3A352F));
 g.fillEllipse (led);
 // texto
 juce::Font f (13.0f); f.setBold (true);
 g.setFont (f);
 g.setColour (on ? textColour : textColour.withAlpha (0.55f));
 g.drawText (b.getButtonText(), bounds.withTrimmedLeft (28.0f), juce::Justification::centredLeft, true);
}

//====================================================================
// KNOB
//====================================================================
Knob::Knob (const juce::String& caption)
{
 slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
 slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 62, 16);
 addAndMakeVisible (slider);
 label.setText (caption, juce::dontSendNotification);
 label.setJustificationType (juce::Justification::centred);
 label.setInterceptsMouseClicks (false, false);
 juce::Font lf (11.0f); lf.setBold (true);
 label.setFont (lf);
 addAndMakeVisible (label);
}

void Knob::resized()
{
 auto r = getLocalBounds();
 label.setBounds (r.removeFromTop (16));
 slider.setBounds (r);
}
//====================================================================
// EDITOR — construtor
//====================================================================
SlashRingAudioProcessorEditor::SlashRingAudioProcessorEditor (SlashRingAudioProcessor& p)
 : AudioProcessorEditor (&p), processor (p)
{

 setLookAndFeel (&lnf);
 // ComboBox de tipo de captador (IDs 1..5 = indices 0..4 do parametro)
 inputTypeBox.addItem ("Single Coil", 1);
 inputTypeBox.addItem ("P90", 2);
 inputTypeBox.addItem ("Humbucker Vintage", 3);
 inputTypeBox.addItem ("Humbucker Modern", 4);
 inputTypeBox.addItem ("Active", 5);
 addAndMakeVisible (inputTypeBox);
 cabinetInfo.setText ("IR-BASED", juce::dontSendNotification);
 cabinetInfo.setJustificationType (juce::Justification::centred);
 cabinetInfo.setColour (juce::Label::textColourId, juce::Colour (0xff9A9188));
 addAndMakeVisible (cabinetInfo);
 for (auto* k : { &inputGain, &outputGain, &odDrive, &odLevel, &ampGain, &bass,
 &mid, &treble, &presence, &master, &delayTime, &delayFb, &delayMix, &reverbMix })
 addAndMakeVisible (*k);
 for (auto* btn : { &overdriveButton, &cabinetButton, &delayButton, &reverbButton })
 addAndMakeVisible (*btn);

 // Attachments
 auto& s = processor.getAPVTS();
 inputTypeAtt = std::make_unique<ComboAtt> (s, ParameterID::inputType, inputTypeBox);
 inputGainAtt = std::make_unique<SliderAtt> (s, ParameterID::inputGain, inputGain.slider);
 outputGainAtt = std::make_unique<SliderAtt> (s, ParameterID::outputGain, outputGain.slider);
 odDriveAtt = std::make_unique<SliderAtt> (s, ParameterID::overdriveDrive, odDrive.slider);
 odLevelAtt = std::make_unique<SliderAtt> (s, ParameterID::overdriveLevel, odLevel.slider);
 ampGainAtt = std::make_unique<SliderAtt> (s, ParameterID::ampGain, ampGain.slider);
 bassAtt = std::make_unique<SliderAtt> (s, ParameterID::bass, bass.slider);
 midAtt = std::make_unique<SliderAtt> (s, ParameterID::middle, mid.slider);
 trebleAtt = std::make_unique<SliderAtt> (s, ParameterID::treble, treble.slider);
 presenceAtt = std::make_unique<SliderAtt> (s, ParameterID::presence, presence.slider);
 masterAtt = std::make_unique<SliderAtt> (s, ParameterID::master, master.slider);
 delayTimeAtt = std::make_unique<SliderAtt> (s, ParameterID::delayTime, delayTime.slider);
 delayFbAtt = std::make_unique<SliderAtt> (s, ParameterID::delayFeedback, delayFb.slider);
 delayMixAtt = std::make_unique<SliderAtt> (s, ParameterID::delayMix, delayMix.slider);
 reverbMixAtt = std::make_unique<SliderAtt> (s, ParameterID::reverbMix, reverbMix.slider);
 overdriveAtt = std::make_unique<ButtonAtt> (s, ParameterID::overdriveOn, overdriveButton);
 cabinetAtt = std::make_unique<ButtonAtt> (s, ParameterID::cabinetOn, cabinetButton);
 delayAtt = std::make_unique<ButtonAtt> (s, ParameterID::delayOn, delayButton);
 reverbAtt = std::make_unique<ButtonAtt> (s, ParameterID::reverbOn, reverbButton);
 setSize (1080, 600);
}


SlashRingAudioProcessorEditor::~SlashRingAudioProcessorEditor()
{
 setLookAndFeel (nullptr);
}
//====================================================================
// PAINT — fundo, cabecalho e paineis das secoes
//====================================================================
void SlashRingAudioProcessorEditor::paint (juce::Graphics& g)
{
 juce::ColourGradient grad (juce::Colour (0xff231E19), 0.0f, 0.0f,
 juce::Colour (0xff141210), 0.0f, (float) getHeight(), false);
 g.setGradientFill (grad);
 g.fillAll();

 // cabecalho
 auto header = getLocalBounds().removeFromTop (74);
 g.setColour (juce::Colour (0xffE0A24A));
 juce::Font title (34.0f); title.setBold (true);
 g.setFont (title);
 g.drawText ("SLASHRING", header.reduced (22, 0).withTrimmedBottom (12),
 juce::Justification::centredLeft, false);
 g.setColour (juce::Colour (0xffEDE7DD).withAlpha (0.55f));
 g.setFont (juce::Font (12.5f));
 g.drawText ("Slash-voiced guitar suite", header.reduced (24, 0).removeFromBottom (20),
 juce::Justification::centredLeft, false);
 g.setColour (juce::Colour (0xff4A443C));
 g.fillRect (14, 72, getWidth() - 28, 1);


 // paineis das secoes
 for (auto& sec : sections)
 {
 auto b = sec.bounds.toFloat();
 g.setColour (juce::Colour (0xff231F1B));
 g.fillRoundedRectangle (b, 8.0f);
 g.setColour (juce::Colour (0xff3A352F));
 g.drawRoundedRectangle (b, 8.0f, 1.0f);
 auto head = sec.bounds.removeFromTop (24); // copia local via valor
 g.setColour (juce::Colour (0xffE0A24A));
 juce::Font hf (12.0f); hf.setBold (true);
 g.setFont (hf);
 g.drawText (sec.title, head.reduced (12, 0), juce::Justification::centredLeft, false);
 }
}

//====================================================================
// RESIZED — layout por retangulos
//====================================================================
void SlashRingAudioProcessorEditor::resized()

{
 sections.clear();
 auto area = getLocalBounds();
 area.removeFromTop (74); // cabecalho (desenhado no paint)
 area.reduce (14, 10); // margem externa
 const int gap = 12;

 // ---------- LINHA A ----------
 auto rowA = area.removeFromTop (250);
 area.removeFromTop (gap);
 auto rowB = area; // resto
 auto inputArea = rowA.removeFromLeft (180); rowA.removeFromLeft (gap);
 auto odArea = rowA.removeFromLeft (220); rowA.removeFromLeft (gap);
 auto ampArea = rowA; // resto (o mais largo)

 // ---------- LINHA B ----------
 auto cabArea = rowB.removeFromLeft (180); rowB.removeFromLeft (gap);
 auto dlyArea = rowB.removeFromLeft (360); rowB.removeFromLeft (gap);
 auto revArea = rowB.removeFromLeft (200); rowB.removeFromLeft (gap);
 auto outArea = rowB; // resto

 // registra os paineis (para o paint desenhar)
 sections.clear();
 sections.push_back ({ inputArea, "INPUT" });
 sections.push_back ({ odArea, "OVERDRIVE" });
 sections.push_back ({ ampArea, "AMPLIFIER" });
 sections.push_back ({ cabArea, "CABINET" });
 sections.push_back ({ dlyArea, "DELAY" });
 sections.push_back ({ revArea, "REVERB" });
 sections.push_back ({ outArea, "OUTPUT" });
 const int headH = 24;
 auto inner = [&] (juce::Rectangle<int> s) { s.removeFromTop (headH); return s.reduced (10); };

 // INPUT: combo + knob
 {
 auto r = inner (inputArea);
 inputTypeBox.setBounds (r.removeFromTop (26));
 r.removeFromTop (8);
 inputGain.setBounds (r.withSizeKeepingCentre (juce::jmin (r.getWidth(), 96),
 juce::jmin (r.getHeight(), 104)));
 }

 // OVERDRIVE: toggle + 2 knobs
 {
 auto r = inner (odArea);
 overdriveButton.setBounds (r.removeFromTop (28));
 r.removeFromTop (6);
 int kw = r.getWidth() / 2;
 odDrive.setBounds (r.removeFromLeft (kw).reduced (2));
 odLevel.setBounds (r.reduced (2));
 }

 // AMPLIFIER: 6 knobs
 {
 auto r = inner (ampArea);
 juce::Component* ks[] = { &ampGain, &bass, &mid, &treble, &presence, &master };
 const int n = 6;
 int kw = r.getWidth() / n;
 for (int i = 0; i < n; ++i)
 ks[i]->setBounds (r.removeFromLeft (kw).reduced (3));
 }

 // CABINET: toggle + info
 {
 auto r = inner (cabArea);
 cabinetButton.setBounds (r.removeFromTop (28));
 r.removeFromTop (8);
 cabinetInfo.setBounds (r.removeFromTop (24));
 }

 // DELAY: toggle + 3 knobs
 {
 auto r = inner (dlyArea);
 delayButton.setBounds (r.removeFromTop (28));
 r.removeFromTop (6);
 int kw = r.getWidth() / 3;
 delayTime.setBounds (r.removeFromLeft (kw).reduced (2));
 delayFb.setBounds (r.removeFromLeft (kw).reduced (2));
 delayMix.setBounds (r.reduced (2));
 }


 // REVERB: toggle + 1 knob
 {
 auto r = inner (revArea);
 reverbButton.setBounds (r.removeFromTop (28));
 r.removeFromTop (6);
 reverbMix.setBounds (r.withSizeKeepingCentre (juce::jmin (r.getWidth(), 96),
 juce::jmin (r.getHeight(), 104)));
 }

 // OUTPUT: 1 knob
 {
 auto r = inner (outArea);
 outputGain.setBounds (r.withSizeKeepingCentre (juce::jmin (r.getWidth(), 96),
 juce::jmin (r.getHeight(), 104)));
 }

 repaint();

}