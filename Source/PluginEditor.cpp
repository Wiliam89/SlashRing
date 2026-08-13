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

 // Menu de cabinets (IDs 1..6 = indices 0..5 do parametro cabinet_model).
 // A ORDEM E OS NOMES precisam bater com a lista em createParameterLayout().
 cabinetModelBox.addItem ("Metal1", 1);
 cabinetModelBox.addItem ("Metal2", 2);
 cabinetModelBox.addItem ("Marshall1960", 3);
 cabinetModelBox.addItem ("Marshall21960", 4);
 cabinetModelBox.addItem ("Marshall34x12", 5);
 cabinetModelBox.addItem ("Marshall44x12", 6);
 addAndMakeVisible (cabinetModelBox);

 for (auto* k : { &inputGain, &outputGain, &odDrive, &odLevel, &odTone, &ampGain, &bass,
 &mid, &treble, &presence, &master, &cabLowCut, &cabHighCut, &cabLevel, &cabMix, &delayTime, &delayFb, &delayMix, &reverbMix })
 addAndMakeVisible (*k);
 for (auto* btn : { &overdriveButton, &cabinetButton, &delayButton, &reverbButton })
 addAndMakeVisible (*btn);



 // Attachments
 auto& s = processor.getAPVTS();
 inputTypeAtt = std::make_unique<ComboAtt> (s, ParameterID::inputType, inputTypeBox);
 cabinetModelAtt = std::make_unique<ComboAtt> (s, ParameterID::cabinetModel, cabinetModelBox);
 inputGainAtt = std::make_unique<SliderAtt> (s, ParameterID::inputGain, inputGain.slider);
 outputGainAtt = std::make_unique<SliderAtt> (s, ParameterID::outputGain, outputGain.slider);
 odDriveAtt = std::make_unique<SliderAtt> (s, ParameterID::overdriveDrive, odDrive.slider);
 odLevelAtt = std::make_unique<SliderAtt> (s, ParameterID::overdriveLevel, odLevel.slider);
 odToneAtt = std::make_unique<SliderAtt> (s, ParameterID::overdriveTone, odTone.slider);
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
 cabLowCutAtt = std::make_unique<SliderAtt> ( s, ParameterID::cabinetLowCut, cabLowCut.slider);
 cabHighCutAtt = std::make_unique<SliderAtt> (s, ParameterID::cabinetHighCut, cabHighCut.slider);
 cabLevelAtt = std::make_unique<SliderAtt> (s, ParameterID::cabinetLevel, cabLevel.slider);
 cabMixAtt = std::make_unique<SliderAtt> (s, ParameterID::cabinetMix, cabMix.slider);
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

// titulo de secao sempre dentro do retangulo do cabecalho da secao:
auto head = sec.bounds.removeFromTop (24).reduced (12, 0);
g.setColour (juce::Colour (0xffE0A24A));
g.setFont (juce::Font (12.0f, juce::Font::bold));
g.drawText (sec.title, head, juce::Justification::centredLeft, false);

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

auto b = ampArea.reduced (12).toFloat(); // area do amp com uma folga
juce::ColourGradient plate (juce::Colour (0xffD8B264), b.getX(), b.getY(),
 juce::Colour (0xff7C5C28), b.getX(), b.getBottom(),
 false);
g.setGradientFill (plate);
g.fillRoundedRectangle (b, 6.0f);

 // faixa logo abaixo do cabecalho, com 24 px de margem nas laterais
drawSignalChain (g, juce::Rectangle<int> (24, 88, getWidth() - 48, 24));

}

// dentro do paint(), usando a area da secao (ex.: odArea):
auto pedal = odArea.reduced (14).withTrimmedTop (24).toFloat();
g.setColour (juce::Colour (0xff241f18));
g.fillRoundedRectangle (pedal, 8.0f);
g.setColour (juce::Colour (0xff4A4038));
g.drawRoundedRectangle (pedal, 8.0f, 1.0f);
// footswitch (circulo) na base do pedal:
auto cx = pedal.getCentreX();
auto cy = pedal.getBottom() - 20.0f;
g.setColour (juce::Colour (0xff15110d));
g.fillEllipse (cx - 12, cy - 12, 24, 24);
g.setColour (juce::Colour (0xffE0A24A));
g.drawEllipse (cx - 12, cy - 12, 24, 24, 2.0f);

void SlashRingAudioProcessorEditor::drawSignalChain (juce::Graphics& g,
 juce::Rectangle<int> area)
{
 const juce::StringArray names { "INPUT","OVERDRIVE","AMPLIFIER",
 "CABINET","DELAY","REVERB","OUTPUT" };
 const int n = names.size(); // 7 caixinhas
 const int gap = 12; // espaco entre elas
 const int h = 24;
 // >>> o segredo: largura total, menos os espacos, dividida por 7
 const int w = (area.getWidth() - gap * (n - 1)) / n;
 int x = area.getX();
 const int y = area.getCentreY() - h / 2;
 for (int i = 0; i < n; ++i)
 {
 auto chip = juce::Rectangle<int> (x, y, w, h).toFloat();
 g.setColour (juce::Colour (0xff211c16));
 g.fillRoundedRectangle (chip, 12.0f);
 g.setColour (juce::Colour (0xffE0A24A));
 g.drawRoundedRectangle (chip, 12.0f, 1.2f);
 g.setColour (juce::Colour (0xffEDE7DD));
 g.setFont (juce::Font (11.0f, juce::Font::bold));
 g.drawText (names[i], chip.toNearestInt(),
 juce::Justification::centred, false);
 x += w + gap; // anda para a proxima
 }

auto grille = juce::Rectangle<float> (gx, gy, 196, 150); // ajuste gx, gy
g.setColour (juce::Colour (0xff0f0c09));
g.fillRoundedRectangle (grille, 6.0f);
g.setColour (juce::Colour (0xff9C6F28));
g.drawRoundedRectangle (grille, 6.0f, 1.0f);
for (int r = 0; r < 2; ++r)
 for (int cN = 0; cN < 2; ++cN)
 {
 float scx = grille.getX() + grille.getWidth() * (0.28f + 0.44f*cN);
 float scy = grille.getY() + grille.getHeight() * (0.28f + 0.44f*r);
 g.setColour (juce::Colour (0xff0a0806));
 g.fillEllipse (scx - 30, scy - 30, 60, 60);
 g.setColour (juce::Colour (0xff2a241d));
 g.drawEllipse (scx - 30, scy - 30, 60, 60, 2.0f);
 }

}

void SlashRingAudioProcessorEditor::setParam (const juce::String& id, float value)
{
 if (auto* p = processor.getAPVTS().getParameter (id))
 p->setValueNotifyingHost (p->convertTo0to1 (value));
}

void SlashRingAudioProcessorEditor::applyPreset
 (const std::map<juce::String, float>& values)
{
 for (const auto& kv : values) // kv.first = nome, kv.second = valor
 setParam (kv.first, kv.second);
}

struct Preset { juce::String name; std::map<juce::String, float> values; };
const std::vector<Preset> factoryPresets =
{
 { "Appetite Lead", {
 { ParameterID::ampGain, 8.0f }, { ParameterID::master, 6.0f },
 { ParameterID::bass, 6.0f }, { ParameterID::middle, 5.5f },
 { ParameterID::treble, 6.5f }, { ParameterID::presence, 5.0f },
 { ParameterID::overdriveOn, 1.0f }, { ParameterID::cabinetModel, 0.0f },
 { ParameterID::delayOn, 0.0f }, { ParameterID::reverbMix, 0.20f } } },
 { "Rhythm Crunch", {
 { ParameterID::ampGain, 5.5f }, { ParameterID::master, 6.0f },
 { ParameterID::bass, 6.5f }, { ParameterID::middle, 6.0f },
 { ParameterID::treble, 5.5f }, { ParameterID::cabinetModel, 4.0f },
 { ParameterID::delayOn, 0.0f }, { ParameterID::reverbMix, 0.10f } } },
 { "Clean Warm", {
 { ParameterID::ampGain, 2.0f }, { ParameterID::master, 6.5f },
 { ParameterID::overdriveOn, 0.0f }, { ParameterID::cabinetModel, 0.0f },
 { ParameterID::reverbMix, 0.30f } } },
};

// 1) preencher o menu com os nomes (IDs comecam em 1):
for (int i = 0; i < (int) factoryPresets.size(); ++i)
 presetBox.addItem (factoryPresets[i].name, i + 1);
addAndMakeVisible (presetBox);
// 2) quando o usuario escolher, aplicar aquele preset:
presetBox.onChange = [this]
{
 const int idx = presetBox.getSelectedId() - 1; // volta para 0,1,2...
 if (idx >= 0 && idx < (int) factoryPresets.size())
 applyPreset (factoryPresets[idx].values);
};

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
 auto odArea = rowA.removeFromLeft (300); rowA.removeFromLeft (gap);
 auto ampArea = rowA; // resto (o mais largo)

 // ---------- LINHA B ----------
 auto cabArea = rowB.removeFromLeft (240); rowB.removeFromLeft (gap);
 auto dlyArea = rowB.removeFromLeft (300); rowB.removeFromLeft (gap);
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
 int kw = r.getWidth() / 3;
 odDrive.setBounds (r.removeFromLeft (kw).reduced (2));
 odLevel.setBounds (r.removeFromLeft (kw).reduced (2));
 odTone .setBounds (r.reduced (2));
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

 // CABINET: toggle + menu + 4 knobs (2x2)
 {
    auto r = inner (cabArea);
    cabinetButton.setBounds (r.removeFromTop (28));
    r.removeFromTop (6);
    cabinetModelBox.setBounds (r.removeFromTop (24));   // NOVO: menu de cabinets
    r.removeFromTop (6);
    auto top = r.removeFromTop (r.getHeight() / 2);
    auto bot = r;
    int kwT = top.getWidth() / 2;
    cabLowCut .setBounds (top.removeFromLeft (kwT).reduced (2));
    cabHighCut.setBounds (top.reduced (2));
    int kwB = bot.getWidth() / 2;
    cabLevel .setBounds (bot.removeFromLeft (kwB).reduced (2));
    cabMix .setBounds (bot.reduced (2));
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
