#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ReverbModule.h"
#include "OverdriveModule.h"
#include "CabinetModule.h"
#include "AmpModule.h"
#include "InputStage.h"
#include "DelayModule.h"
#include "OutputStage.h"
#include "BinaryData.h"

//============================================================
// TD-010 GAIN MAP B — TEMPORARY DIAGNOSTIC HELPERS
//
// Observational only. Never write to, resample, or
// otherwise modify the buffers/blocks they read. INPUT is
// read at base sample rate; OD and AMP are read in the
// oversampled domain, exactly as they exist at their
// respective tap points — no domain equalization is
// performed.
//
// Remove this entire anonymous namespace after the TD-010
// audit concludes.
//============================================================

namespace
{
    void td010ComputeRmsPeakDb(
        const juce::AudioBuffer<float>& buffer,
        float& rmsDb,
        float& peakDb)
    {
        float peak = 0.0f;
        double sumSquares = 0.0;
        juce::int64 total = 0;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const auto* data = buffer.getReadPointer(ch);

            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                const float s = data[i];
                peak = juce::jmax(peak, std::abs(s));
                sumSquares += static_cast<double>(s) * static_cast<double>(s);
                ++total;
            }
        }

        const float rms =
            (total > 0)
            ? static_cast<float>(std::sqrt(sumSquares / static_cast<double>(total)))
            : 0.0f;

        rmsDb = juce::Decibels::gainToDecibels(rms, -120.0f);
        peakDb = juce::Decibels::gainToDecibels(peak, -120.0f);
    }

    void td010ComputeRmsPeakDb(
        const juce::dsp::AudioBlock<float>& block,
        float& rmsDb,
        float& peakDb)
    {
        float peak = 0.0f;
        double sumSquares = 0.0;
        juce::int64 total = 0;

        const auto numChannels = block.getNumChannels();
        const auto numSamples = block.getNumSamples();

        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            const auto* data = block.getChannelPointer(ch);

            for (size_t i = 0; i < numSamples; ++i)
            {
                const float s = data[i];
                peak = juce::jmax(peak, std::abs(s));
                sumSquares += static_cast<double>(s) * static_cast<double>(s);
                ++total;
            }
        }

        const float rms =
            (total > 0)
            ? static_cast<float>(std::sqrt(sumSquares / static_cast<double>(total)))
            : 0.0f;

        rmsDb = juce::Decibels::gainToDecibels(rms, -120.0f);
        peakDb = juce::Decibels::gainToDecibels(peak, -120.0f);
    }

    void td010AppendDiagLogLine(const juce::String& line)
    {
        auto file =
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
            .getChildFile("SlashRing_TD010_GainMapB.log");

        file.appendText(line + "\n", false, false, "\n");
    }
}

//============================================================
// CONSTRUCTOR
//============================================================

SlashRingAudioProcessor::SlashRingAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(
        BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    )
    , apvts(*this, nullptr, "PLUGIN_STATE", createParameterLayout())
#endif
{
    inputStage = std::make_unique<InputStage>();
    overdriveModule = std::make_unique<OverdriveModule>();
    ampModule = std::make_unique<AmpModule>();
    cabinetModule = std::make_unique<CabinetModule>();
    delayModule = std::make_unique<DelayModule>();
    reverbModule = std::make_unique<ReverbModule>();
    outputStage = std::make_unique<OutputStage>();
}

SlashRingAudioProcessor::~SlashRingAudioProcessor() = default;

//============================================================
// PREPARE
//============================================================

void SlashRingAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;

    prepareDSP(sampleRate, samplesPerBlock);
}

void SlashRingAudioProcessor::releaseResources()
{
}

//============================================================
// DSP PREPARE
//============================================================

void SlashRingAudioProcessor::prepareDSP(double sampleRate, int samplesPerBlock)
{
    oversampling.reset();
    oversampling.initProcessing(static_cast<size_t>(samplesPerBlock));

    const auto oversamplingFactor =
        oversampling.getOversamplingFactor();

    const auto oversampledRate =
        sampleRate * oversamplingFactor;

    //========================================================
    // TD-002: BASE-RATE DOMAIN
    //
    // InputStage, CabinetModule, DelayModule, ReverbModule
    // and OutputStage run outside the oversampling region
    // per ARCHITECTURE.md and must be prepared with the
    // host's actual sample rate and block size.
    //========================================================

    juce::dsp::ProcessSpec baseSpec;
    baseSpec.sampleRate = sampleRate;
    baseSpec.maximumBlockSize =
        static_cast<juce::uint32>(samplesPerBlock);
    baseSpec.numChannels =
        static_cast<juce::uint32>(getTotalNumOutputChannels());

    //========================================================
    // TD-002: OVERSAMPLED DOMAIN
    //
    // OverdriveModule and AmpModule run inside the
    // oversampling region and must be prepared with the
    // oversampled rate and block size.
    //========================================================

    juce::dsp::ProcessSpec oversampledSpec;
    oversampledSpec.sampleRate = oversampledRate;
    oversampledSpec.maximumBlockSize =
        static_cast<juce::uint32>(
            samplesPerBlock * oversamplingFactor
            );
    oversampledSpec.numChannels =
        static_cast<juce::uint32>(getTotalNumOutputChannels());

    prepareBaseRateModules(baseSpec);
    prepareOversampledModules(oversampledSpec);
}

void SlashRingAudioProcessor::prepareBaseRateModules(
    const juce::dsp::ProcessSpec& baseSpec)
{
    inputStage->prepare(
        baseSpec.sampleRate,
        static_cast<int>(baseSpec.maximumBlockSize),
        static_cast<int>(baseSpec.numChannels));

    cabinetModule->prepare(
        baseSpec.sampleRate,
        static_cast<int>(baseSpec.maximumBlockSize),
        static_cast<int>(baseSpec.numChannels));

    delayModule->prepare(
        baseSpec.sampleRate,
        static_cast<int>(baseSpec.maximumBlockSize),
        static_cast<int>(baseSpec.numChannels));

    reverbModule->prepare(
        baseSpec.sampleRate,
        static_cast<int>(baseSpec.maximumBlockSize),
        static_cast<int>(baseSpec.numChannels));

    outputStage->prepare(
        baseSpec.sampleRate,
        static_cast<int>(baseSpec.maximumBlockSize),
        static_cast<int>(baseSpec.numChannels));

    //==========================================
    // Factory IR
    //==========================================

    cabinetModule->triggerAsyncFactoryIRLoad(
       0,
       BinaryData::Marshall_Creamback_wav,
       BinaryData::Marshall_Creamback_wavSize,
       "marshall_creamback_factory"
    );
}

void SlashRingAudioProcessor::prepareOversampledModules(
    const juce::dsp::ProcessSpec& oversampledSpec)
{
    overdriveModule->prepare(
        oversampledSpec.sampleRate,
        static_cast<int>(oversampledSpec.maximumBlockSize),
        static_cast<int>(oversampledSpec.numChannels));

    ampModule->prepare(
        oversampledSpec.sampleRate,
        static_cast<int>(oversampledSpec.maximumBlockSize),
        static_cast<int>(oversampledSpec.numChannels));
}

//============================================================
// PROCESS BLOCK
//============================================================

void SlashRingAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const auto totalInputChannels = getTotalNumInputChannels();
    const auto totalOutputChannels = getTotalNumOutputChannels();

    for (int channel = totalInputChannels;
        channel < totalOutputChannels;
        ++channel)
    {
        buffer.clear(channel, 0, buffer.getNumSamples());
    }

    updateParameterState();

    //========================================================
   // DSP CHAIN (TD-001)
  //========================================================
     //
     // Official signal flow per ARCHITECTURE.md:
     //
     // Input -> InputStage -> Oversampling Engine -> Overdrive
     // -> Amp -> Downsampling -> Cabinet -> Delay -> Reverb
     // -> OutputStage -> Host Output
     //
     // InputStage runs at base rate BEFORE upsampling.
     // OverdriveModule and AmpModule run on the actual
     // oversampled AudioBlock produced by processSamplesUp().
     // Cabinet, Delay, Reverb and OutputStage run at base
     // rate AFTER downsampling.
     //========================================================

     // INPUT STAGE (base rate)
    if (inputStage != nullptr)
    {
        inputStage->process(buffer);
    }

    // TD-010 GAIN MAP B — TEMPORARY DIAGNOSTIC — INPUT TAP — BEGIN
    // Base-rate tap, immediately after InputStage, before upsampling.
    float td010InputRmsDb = -120.0f;
    float td010InputPeakDb = -120.0f;
    td010ComputeRmsPeakDb(buffer, td010InputRmsDb, td010InputPeakDb);
    // TD-010 GAIN MAP B — TEMPORARY DIAGNOSTIC — INPUT TAP — END

    juce::dsp::AudioBlock<float> block(buffer);

    auto oversampledBlock =
        oversampling.processSamplesUp(block);

    // OVERDRIVE (oversampled domain)
    if (overdriveModule != nullptr)
    {
        overdriveModule->process(oversampledBlock);
    }

    // TD-010 GAIN MAP B — TEMPORARY DIAGNOSTIC — OD TAP — BEGIN
    // Oversampled-domain tap, immediately after OverdriveModule,
    // before AmpModule. No domain equalization performed.
    float td010OdRmsDb = -120.0f;
    float td010OdPeakDb = -120.0f;
    td010ComputeRmsPeakDb(oversampledBlock, td010OdRmsDb, td010OdPeakDb);
    // TD-010 GAIN MAP B — TEMPORARY DIAGNOSTIC — OD TAP — END

    // AMP (oversampled domain)
    if (ampModule != nullptr)
    {
        ampModule->process(oversampledBlock);
    }

    // TD-010 GAIN MAP B — TEMPORARY DIAGNOSTIC — AMP TAP — BEGIN
    // Oversampled-domain tap, immediately after AmpModule,
    // before downsampling. No domain equalization performed.
    float td010AmpRmsDb = -120.0f;
    float td010AmpPeakDb = -120.0f;
    td010ComputeRmsPeakDb(oversampledBlock, td010AmpRmsDb, td010AmpPeakDb);
    // TD-010 GAIN MAP B — TEMPORARY DIAGNOSTIC — AMP TAP — END

    // TD-010 GAIN MAP B — TEMPORARY DIAGNOSTIC — PARAMETER-STATE + LOG — BEGIN
    //
    // AMP_GAIN_STATE / OD_DRIVE_STATE: current (non-advancing)
    // SmoothedValue state read via the diagnostic getters added
    // to AmpModule / OverdriveModule. Represents the smoother's
    // state at this query point in this block only — not every
    // sample-by-sample value consumed during the block.
    //
    // OD_REQUESTED_ON: mirrors the exact condition used in
    // updateParameterState() to decide OverdriveModule routing
    // (overdriveOn > 0.5f). This is the Processor's REQUESTED
    // routing decision, NOT an internal OverdriveModule bypass
    // state — OverdriveModule has no real bypass state (TD-005,
    // open, unaddressed by this diagnostic).
    //
    // Throttled to ~2 log lines/sec so manual test-matrix
    // capture stays readable.
    //========================================================
    td010DiagAccumSamples += static_cast<double>(buffer.getNumSamples());

    const double td010LogIntervalSamples = currentSampleRate * 0.5;

    if (td010DiagAccumSamples >= td010LogIntervalSamples)
    {
        td010DiagAccumSamples = 0.0;

        const float td010AmpGainState =
            (ampModule != nullptr)
            ? ampModule->getCurrentGainValue()
            : 0.0f;

        const float td010OdDriveState =
            (overdriveModule != nullptr)
            ? overdriveModule->getCurrentDriveValue()
            : 0.0f;

        const bool td010OdRequestedOn =
            apvts.getRawParameterValue(ParameterID::overdriveOn)->load() > 0.5f;

        const juce::String td010LogLine =
            juce::String("TD-010 GAIN MAP B | ")
            + "AMP_GAIN_STATE: " + juce::String(td010AmpGainState, 3) + " | "
            + "OD_REQUESTED_ON: " + juce::String(td010OdRequestedOn ? 1 : 0) + " | "
            + "OD_DRIVE_STATE: " + juce::String(td010OdDriveState, 3) + " | "
            + "INPUT RMS: " + juce::String(td010InputRmsDb, 2) + " dBFS | "
            + "PEAK: " + juce::String(td010InputPeakDb, 2) + " dBFS || "
            + "OD RMS: " + juce::String(td010OdRmsDb, 2) + " dBFS | "
            + "PEAK: " + juce::String(td010OdPeakDb, 2) + " dBFS || "
            + "AMP RMS: " + juce::String(td010AmpRmsDb, 2) + " dBFS | "
            + "PEAK: " + juce::String(td010AmpPeakDb, 2) + " dBFS";

        td010AppendDiagLogLine(td010LogLine);
    }
    // TD-010 GAIN MAP B — TEMPORARY DIAGNOSTIC — PARAMETER-STATE + LOG — END

    oversampling.processSamplesDown(block);

    // CABINET (base rate)
    if (cabinetModule != nullptr)
    {
        cabinetModule->process(buffer);
    }

    // DELAY (base rate)
    if (delayModule != nullptr)
    {
        delayModule->process(buffer);
    }

    // REVERB (base rate)
    if (reverbModule != nullptr)
    {
        reverbModule->process(buffer);
    }

    // OUTPUT STAGE (base rate)
    if (outputStage != nullptr)
    {
        outputStage->process(buffer);
    }
}

//============================================================
// PARAMETER SYNC
//============================================================

void SlashRingAudioProcessor::updateParameterState()
{

    //========================================================
   // INPUT STAGE
  //========================================================

    if (inputStage != nullptr)
    {
        const auto inputGain =
            apvts.getRawParameterValue(
                ParameterID::inputGain)->load();

        const auto inputType =
            apvts.getRawParameterValue(
                ParameterID::inputType)->load();

        inputStage->setInputGain(inputGain);
        inputStage->setInputType(static_cast<int>(inputType));
    }

    //========================================================
    // OVERDRIVE
    //========================================================

    if (overdriveModule != nullptr)
    {
        const auto overdriveEnabled =
            apvts.getRawParameterValue(
                ParameterID::overdriveOn)->load();

        const auto overdriveDrive =
            apvts.getRawParameterValue(
                ParameterID::overdriveDrive)->load();

        const auto overdriveLevel =
            apvts.getRawParameterValue(
                ParameterID::overdriveLevel)->load();
        
        const auto overdriveTone =
            apvts.getRawParameterValue(
                ParameterID::overdriveTone)->load();

        if (overdriveEnabled > 0.5f)
        {
            overdriveModule->setDrive(
                overdriveDrive);

            overdriveModule->setLevel(
                overdriveLevel);
        }
        else
        {
            overdriveModule->setDrive(0.0f);
            overdriveModule->setLevel(0.0f);
            overdriveModule->setTone(overdriveTone);
        }
    }

    //========================================================
   // AMP
  //========================================================

    if (ampModule != nullptr)
    {
        const auto gain =
            apvts.getRawParameterValue(
                ParameterID::ampGain)->load();

        const auto bass =
            apvts.getRawParameterValue(
                ParameterID::bass)->load();

        const auto middle =
            apvts.getRawParameterValue(
                ParameterID::middle)->load();

        const auto treble =
            apvts.getRawParameterValue(
                ParameterID::treble)->load();

        const auto presence =
            apvts.getRawParameterValue(
                ParameterID::presence)->load();

        const auto master =
            apvts.getRawParameterValue(
                ParameterID::master)->load();

        ampModule->setGain(gain);
        ampModule->setBass(bass);
        ampModule->setMiddle(middle);
        ampModule->setTreble(treble);
        ampModule->setPresence(presence);
        ampModule->setMaster(master);
    }

    //========================================================
   // CABINET
  //========================================================

   if (cabinetModule != nullptr)
{
    const auto enabled =
    apvts.getRawParameterValue(
    ParameterID::cabinetOn)->load();


    const auto lowCut =
    apvts.getRawParameterValue(
    ParameterID::cabinetLowCut)->load();

    const auto highCut =
    apvts.getRawParameterValue(
    ParameterID::cabinetHighCut)->load();

    const auto levelDb =
    apvts.getRawParameterValue(
    ParameterID::cabinetLevel)->load();

    const auto mix =
    apvts.getRawParameterValue(
    ParameterID::cabinetMix)->load();
    cabinetModule->setEnabled(enabled > 0.5f);
    // Voz A carrega a IR de fabrica; voz B fica muda
    // (blend = 0). Low/High cut ajustam o timbre.

    cabinetModule->configureVoice(
    0, 0.0f, lowCut, highCut, 0.0f, 0.5f, 45.0f);
    cabinetModule->configureVoice(
    1, 0.0f, lowCut, highCut, 0.0f, 0.5f, 45.0f);
    // blend 0 = 100% voz A. O +6 dB compensa a lei de
    // pan (~0.5) do CabinetMixer, devolvendo o nivel a
    // unidade. levelDb e o makeup do usuario.

    cabinetModule->configureMixer(
    0.0f, 0.0f, 0.0f, levelDb + 6.0f);
    // Dry/wet do cabinet
    cabinetModule->setMix(mix);
}

    //========================================================
    // DELAY
   //========================================================

    if (delayModule != nullptr)
    {
        const auto delayEnabled =
            apvts.getRawParameterValue(
                ParameterID::delayOn)->load();

        const auto delayTime =
            apvts.getRawParameterValue(
                ParameterID::delayTime)->load();

        const auto delayFeedback =
            apvts.getRawParameterValue(
                ParameterID::delayFeedback)->load();

        const auto delayMix =
            apvts.getRawParameterValue(
                ParameterID::delayMix)->load();

        if (delayEnabled > 0.5f)
        {
            delayModule->setDelayTime(delayTime);
            delayModule->setFeedback(delayFeedback);
            delayModule->setMix(delayMix);
        }
        else
        {
            delayModule->setMix(0.0f);
        }
    }

    //========================================================
    // REVERB
    //========================================================

    if (reverbModule != nullptr)
    {
        const auto reverbEnabled =
            apvts.getRawParameterValue(
                ParameterID::reverbOn)->load();

        const auto reverbMix =
            apvts.getRawParameterValue(
                ParameterID::reverbMix)->load();

        if (reverbEnabled > 0.5f)
            reverbModule->setMix(reverbMix);
        else
            reverbModule->setMix(0.0f);
    }

    //========================================================
    // OUTPUT
    //========================================================

    if (outputStage != nullptr)
    {
        const auto outputGain =
            apvts.getRawParameterValue(
                ParameterID::outputGain)->load();

        outputStage->setOutputGain(outputGain);
    }
}

//============================================================
// LAYOUT SUPPORT
//============================================================

#ifndef JucePlugin_PreferredChannelConfigurations
bool SlashRingAudioProcessor::isBusesLayoutSupported(
    const BusesLayout& layouts) const
{
    const auto& mainOutput = layouts.getMainOutputChannelSet();

    if (mainOutput != juce::AudioChannelSet::mono()
        && mainOutput != juce::AudioChannelSet::stereo())
        return false;

    return true;
}
#endif

//============================================================
// EDITOR
//============================================================

bool SlashRingAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* SlashRingAudioProcessor::createEditor()
{
    return new SlashRingAudioProcessorEditor(*this);
}

//============================================================
// PLUGIN INFO
//============================================================

const juce::String SlashRingAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SlashRingAudioProcessor::acceptsMidi() const
{
    return false;
}

bool SlashRingAudioProcessor::producesMidi() const
{
    return false;
}

bool SlashRingAudioProcessor::isMidiEffect() const
{
    return false;
}

double SlashRingAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

//============================================================
// PROGRAMS
//============================================================

int SlashRingAudioProcessor::getNumPrograms()
{
    return 1;
}

int SlashRingAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SlashRingAudioProcessor::setCurrentProgram(int)
{
}

const juce::String SlashRingAudioProcessor::getProgramName(int)
{
    return {};
}

void SlashRingAudioProcessor::changeProgramName(
    int,
    const juce::String&)
{
}

//============================================================
// STATE SAVE
//============================================================

void SlashRingAudioProcessor::getStateInformation(
    juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());

    copyXmlToBinary(*xml, destData);
}

void SlashRingAudioProcessor::setStateInformation(
    const void* data,
    int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(
        getXmlFromBinary(data, sizeInBytes));

    if (xml != nullptr)
    {
        if (xml->hasTagName(apvts.state.getType()))
        {
            apvts.replaceState(
                juce::ValueTree::fromXml(*xml)
            );
        }
    }
}

//============================================================
// PARAMETERS
//============================================================

juce::AudioProcessorValueTreeState::ParameterLayout
SlashRingAudioProcessor::createParameterLayout()
{
    using Parameter = juce::AudioParameterFloat;
    using Toggle = juce::AudioParameterBool;
    using Choice = juce::AudioParameterChoice;

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

    //========================================================
    // INPUT / OUTPUT
    //========================================================

    parameters.push_back(std::make_unique<Parameter>(
        ParameterID::inputGain,
        "Input Gain",
        0.0f,
        2.0f,
        1.0f));

    parameters.push_back(std::make_unique<Parameter>(
        ParameterID::outputGain,
        "Output Gain",
        0.0f,
        2.0f,
        1.0f));

    //========================================================
    // AMP
    //========================================================

    parameters.push_back(std::make_unique<Parameter>(
        ParameterID::ampGain,
        "Amp Gain",
        0.0f,
        10.0f,
        5.0f));

    parameters.push_back(std::make_unique<Parameter>(
        ParameterID::bass,
        "Bass",
        0.0f,
        10.0f,
        5.0f));

    parameters.push_back(std::make_unique<Parameter>(
        ParameterID::middle,
        "Middle",
        0.0f,
        10.0f,
        6.0f));

    parameters.push_back(std::make_unique<Parameter>(
        ParameterID::treble,
        "Treble",
        0.0f,
        10.0f,
        5.0f));

    parameters.push_back(std::make_unique<Parameter>(
        ParameterID::presence,
        "Presence",
        0.0f,
        10.0f,
        5.0f));

    parameters.push_back(std::make_unique<Parameter>(
        ParameterID::master,
        "Master",
        0.0f,
        10.0f,
        6.0f));

    //========================================================
    // OVERDRIVE
    //========================================================

    parameters.push_back(std::make_unique<Toggle>(
        ParameterID::overdriveOn,
        "Overdrive On",
        false));

    parameters.push_back(std::make_unique<Parameter>(
        ParameterID::overdriveDrive,
        "Overdrive Drive",
        juce::NormalisableRange<float>(
            0.0f,
            100.0f,
            0.01f),
        35.0f));

    parameters.push_back(std::make_unique<Parameter>(
        ParameterID::overdriveLevel,
        "Overdrive Level",
        juce::NormalisableRange<float>(
            -60.0f,
            12.0f,
            0.01f),
        0.0f));

    parameters.push_back(std::make_unique<Parameter>(
        ParameterID::overdriveTone,
        "Overdrive Tone",
        juce::NormalisableRange<float>(
             0.0f, 10.0f, 0.01f),
             6.0f));

    //========================================================
    // CAB / REVERB
    //========================================================

    parameters.push_back(std::make_unique<Toggle>(
        ParameterID::cabinetOn,
        "Cabinet On",
        true));

    parameters.push_back(std::make_unique<Parameter>(
         ParameterID::cabinetLowCut,
         "Cabinet Low Cut",
    juce::NormalisableRange<float>(
      20.0f, 300.0f, 1.0f, 0.5f),
      80.0f));

    parameters.push_back(std::make_unique<Parameter>(
         ParameterID::cabinetHighCut,
         "Cabinet High Cut",
    juce::NormalisableRange<float>(
       2000.0f, 20000.0f, 1.0f, 0.5f),
       12000.0f));

    parameters.push_back(std::make_unique<Parameter>(
         ParameterID::cabinetLevel,
         "Cabinet Level",
    juce::NormalisableRange<float>(
       -24.0f, 24.0f, 0.1f),
        0.0f));

    parameters.push_back(std::make_unique<Parameter>(
         ParameterID::cabinetMix,
         "Cabinet Mix",
    juce::NormalisableRange<float>(
        0.0f, 1.0f, 0.01f),
        1.0f));

    parameters.push_back(std::make_unique<Toggle>(
        ParameterID::reverbOn,
        "Reverb On",
        false));

    parameters.push_back(std::make_unique<Parameter>(
        ParameterID::reverbMix,
        "Reverb Mix",
        0.0f,
        1.0f,
        0.15f));

    //========================================================
    // DELAY
    //========================================================

    parameters.push_back(std::make_unique<Toggle>(
        ParameterID::delayOn,
        "Delay On",
        false));

    parameters.push_back(std::make_unique<Parameter>(
        ParameterID::delayTime,
        "Delay Time",
        1.0f,
        2000.0f,
        380.0f));

    parameters.push_back(std::make_unique<Parameter>(
        ParameterID::delayFeedback,
        "Delay Feedback",
        0.0f,
        0.95f,
        0.35f));

    parameters.push_back(std::make_unique<Parameter>(
        ParameterID::delayMix,
        "Delay Mix",
        0.0f,
        1.0f,
        0.20f));

    //========================================================
    // INPUT TYPE
    //========================================================

    parameters.push_back(std::make_unique<Choice>(
        ParameterID::inputType,
        "Input Type",
        juce::StringArray
        {
            "Single Coil",
            "P90",
            "Humbucker Vintage",
            "Humbucker Modern",
            "Active"
        },

        2));

    return { parameters.begin(), parameters.end() };
}

//============================================================
// FACTORY
//============================================================

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SlashRingAudioProcessor();
}