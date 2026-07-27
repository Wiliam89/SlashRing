#include "AmpModule.h"

AmpModule::AmpModule()
{
}

AmpModule::~AmpModule()
{
}

void AmpModule::reset()
{
    inputHPF.reset();
    bassFilter.reset();
    midFilter.reset();
    trebleFilter.reset();
    presenceShelf.reset();
    outputLPF.reset();
}

void AmpModule::release()
{
}

void AmpModule::prepare(
    double sampleRate,
    int samplesPerBlock,
    int numChannels)
{
    currentSampleRate = sampleRate;

    //========================================================
    // SMOOTHING
    //========================================================

    gainSmoothed.reset(sampleRate, 0.02);
    masterSmoothed.reset(sampleRate, 0.02);

    gainSmoothed.setCurrentAndTargetValue(gain);
    masterSmoothed.setCurrentAndTargetValue(master);

    //========================================================
    // DSP SPEC
    //========================================================

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize =
        static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels =
        static_cast<juce::uint32>(numChannels);

    inputHPF.reset();
    bassFilter.reset();
    midFilter.reset();
    trebleFilter.reset();
    presenceShelf.reset();
    outputLPF.reset();

    inputHPF.prepare(spec);
    bassFilter.prepare(spec);
    midFilter.prepare(spec);
    trebleFilter.prepare(spec);
    presenceShelf.prepare(spec);
    outputLPF.prepare(spec);

    *inputHPF.state =
        *juce::dsp::IIR::Coefficients<float>::makeHighPass(
            sampleRate,
            80.0f);

    *outputLPF.state =
        *juce::dsp::IIR::Coefficients<float>::makeLowPass(
            sampleRate,
            9000.0f);

    updateToneStack();

    reset();
}

void AmpModule::updateToneStack()
{
    const float bassGain =
        juce::jmap(bass, 0.0f, 10.0f, 0.5f, 2.0f);

    const float midGain =
        juce::jmap(middle, 0.0f, 10.0f, 0.5f, 2.0f);

    const float trebleGain =
        juce::jmap(treble, 0.0f, 10.0f, 0.5f, 2.0f);

    const float presenceGain =
        juce::jmap(presence, 0.0f, 10.0f, 0.5f, 2.0f);

    *bassFilter.state =
        *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
            currentSampleRate,
            120.0f,
            0.707f,
            bassGain);

    *midFilter.state =
        *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
            currentSampleRate,
            750.0f,
            0.8f,
            midGain);

    *trebleFilter.state =
        *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
            currentSampleRate,
            3500.0f,
            0.707f,
            trebleGain);

    *presenceShelf.state =
        *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
            currentSampleRate,
            5000.0f,
            0.707f,
            presenceGain);
}

void AmpModule::process(
    juce::dsp::AudioBlock<float>& block)
{
    juce::ScopedNoDenormals noDenormals;

    //========================================================
    // INPUT HPF
    //========================================================

    {
        juce::dsp::ProcessContextReplacing<float> context(block);
        inputHPF.process(context);
    }

    //========================================================
    // PREAMP SATURATION (Sample-Outer, Channel-Inner)
    //========================================================

    const auto numChannels = block.getNumChannels();
    const auto numSamples = block.getNumSamples();

    for (size_t i = 0; i < numSamples; ++i)
    {
        const float drive = gainSmoothed.getNextValue();

        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            auto* samples = block.getChannelPointer(ch);
            float x = samples[i];

            x *= drive;

            const float positive = std::tanh(x * 1.75f);
            const float negative = std::tanh(x * 1.25f);

            x = (x >= 0.0f) ? positive : negative;

            samples[i] = x;
        }
    }

    //========================================================
    // TONE STACK
    //========================================================

    {
        juce::dsp::ProcessContextReplacing<float> context(block);

        bassFilter.process(context);
        midFilter.process(context);
        trebleFilter.process(context);
        presenceShelf.process(context);
    }

    //========================================================
    // POWER AMP + MASTER (Sample-Outer, Channel-Inner)
    //========================================================

    for (size_t i = 0; i < numSamples; ++i)
    {
        const float masterGain = masterSmoothed.getNextValue();

        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            auto* samples = block.getChannelPointer(ch);
            float x = samples[i];

            x = std::tanh(x * 1.4f);
            x *= masterGain;

            samples[i] = x;
        }
    }

    //========================================================
    // OUTPUT LPF
    //========================================================

    {
        juce::dsp::ProcessContextReplacing<float> context(block);
        outputLPF.process(context);
    }
}

void AmpModule::setGain(float value)
{
    gain = juce::jlimit(0.0f, 10.0f, value);

    const float normalized = gain / 10.0f;

    const float driveGain =
        juce::jmap(
            normalized,
            1.0f,
            35.0f);

    currentDriveGain = driveGain;

    gainSmoothed.setTargetValue(
        currentDriveGain);
}

void AmpModule::setBass(float value)
{
    bass = value;
    updateToneStack();
}

void AmpModule::setMiddle(float value)
{
    middle = value;
    updateToneStack();
}

void AmpModule::setTreble(float value)
{
    treble = value;
    updateToneStack();
}

void AmpModule::setPresence(float value)
{
    presence = value;
    updateToneStack();
}

void AmpModule::setMaster(float value)
{
    master = juce::jlimit(0.0f, 10.0f, value);

    const float normalized = master / 10.0f;

    currentMasterGain =
        juce::Decibels::decibelsToGain(
            juce::jmap(
                normalized,
                -60.0f,
                0.0f));

    masterSmoothed.setTargetValue(
        currentMasterGain);
}