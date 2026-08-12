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

    // Menu de cabinets: ouvir mudancas do parametro cabinet_model.
    // Quando o usuario troca de cabinet, agendamos o carregamento
    // do IR fora da thread de audio (ver handleAsyncUpdate()).
    apvts.addParameterListener(ParameterID::cabinetModel, this);
}

SlashRingAudioProcessor::~SlashRingAudioProcessor()
{
    apvts.removeParameterListener(ParameterID::cabinetModel, this);
    cancelPendingUpdate();
}

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
    //========================================================

    juce::dsp::ProcessSpec baseSpec;
    baseSpec.sampleRate = sampleRate;
    baseSpec.maximumBlockSize =
        static_cast<juce::uint32>(samplesPerBlock);
    baseSpec.numChannels =
        static_cast<juce::uint32>(getTotalNumOutputChannels());

    //========================================================
    // TD-002: OVERSAMPLED DOMAIN
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
    // Cabinet inicial (menu): carrega o IR do cabinet
    // atualmente selecionado no parametro.
    //==========================================

    const int startModel =
        static_cast<int>(
            apvts.getRawParameterValue(ParameterID::cabinetModel)->load());

    desiredCabinetModel.store(startModel);
    loadedCabinetModel = startModel;
    loadCabinetIR(startModel);
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
// MENU DE CABINETS - CARREGAMENTO DE IR (thread de mensagens)
//============================================================

void SlashRingAudioProcessor::parameterChanged(
    const juce::String& parameterID,
    float newValue)
{
    // Pode ser chamado ate pela thread de audio (automacao do host),
    // entao aqui fazemos SO o minimo e seguro: anotar o desejo e
    // acordar o AsyncUpdater. O trabalho pesado vai para a thread
    // de mensagens em handleAsyncUpdate().
    if (parameterID == ParameterID::cabinetModel)
    {
        desiredCabinetModel.store(static_cast<int>(newValue));
        triggerAsyncUpdate();
    }
}

void SlashRingAudioProcessor::handleAsyncUpdate()
{
    // Roda na thread de mensagens. Aqui e seguro chamar o
    // carregador de IR (ele usa lock + ThreadPool internamente).
    const int want = desiredCabinetModel.load();

    if (want != loadedCabinetModel)
    {
        loadCabinetIR(want);
        loadedCabinetModel = want;
    }
}

void SlashRingAudioProcessor::loadCabinetIR(int modelIndex)
{
    if (cabinetModule == nullptr)
        return;

    //========================================================
    // TABELA DE CABINETS
    //
    // >>> COMO ADICIONAR SEUS IRs (ver PDF): <<<
    // 1) Adicione o WAV no Projucer como "Binary Resource".
    // 2) O Projucer cria um simbolo em BinaryData, ex.:
    //    BinaryData::MeuIR_wav e BinaryData::MeuIR_wavSize.
    // 3) Troque, no slot desejado abaixo, os dois BinaryData::...
    //    pelo seu IR e mude o texto do hash (precisa ser UNICO).
    //
    // Hoje os slots 3..6 apontam para o Creamback so para o
    // projeto COMPILAR de imediato. Troque cada um pelo seu IR.
    //========================================================

    struct CabIR { const char* data; int size; const char* hash; };

    static const CabIR table[kNumCabinets] =
    {
        { BinaryData::BD_CL_Telocastme_wav, BinaryData::BD_CL_Telocastme_wavSize, "cab0_BD_CL_Telocastme"    }, // Slot 1
        { BinaryData::marshall_cab_wav,       BinaryData::marshall_cab_wavSize,        "cab1_marshallcab" }, // Slot 2
        { BinaryData::BD_HV_Creamback3_mixed_wav, BinaryData::BD_HV_Creamback3_mixed_wavSize,  "cab2_BD_HV_Creamback3"}, // Slot 3
        { BinaryData::BD_LD_HairApparently_wav, BinaryData::BD_LD_HairApparently_wavSize, "cab3_BD_LD_HairApparently"}, // Slot 4  <- troque
        { BinaryData::BD_RH_GatesOfHell_wav, BinaryData::BD_RH_GatesOfHell_wavSize, "cab4_BD_RH_GatesOfHell"       }, // Slot 5  <- troque
        { BinaryData::Marshall_Creamback_wav, BinaryData::Marshall_Creamback_wavSize, "cab5_troque"       }, // Slot 6  <- troque
    };

    const int idx = juce::jlimit(0, kNumCabinets - 1, modelIndex);

    cabinetModule->triggerAsyncFactoryIRLoad(
        0,
        table[idx].data,
        static_cast<size_t>(table[idx].size),
        table[idx].hash);
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
    //
    // Input -> InputStage -> Oversampling -> Overdrive -> Amp
    // -> Downsampling -> Cabinet -> Delay -> Reverb -> Output
    //========================================================

    // INPUT STAGE (base rate)
    if (inputStage != nullptr)
    {
        inputStage->process(buffer);
    }

    juce::dsp::AudioBlock<float> block(buffer);

    auto oversampledBlock =
        oversampling.processSamplesUp(block);

    // OVERDRIVE (oversampled domain)
    if (overdriveModule != nullptr)
    {
        overdriveModule->process(oversampledBlock);
    }

    // AMP (oversampled domain)
    if (ampModule != nullptr)
    {
        ampModule->process(oversampledBlock);
    }

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
    //
    // CORRECAO: o Tone agora e SEMPRE aplicado, ligado ou
    // desligado. Antes o setTone() so era chamado com o
    // overdrive DESLIGADO, entao o knob nao fazia nada
    // com o overdrive LIGADO.
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

        // Tone SEMPRE aplicado -> o knob sempre funciona.
        overdriveModule->setTone(overdriveTone);

        if (overdriveEnabled > 0.5f)
        {
            overdriveModule->setDrive(overdriveDrive);
            overdriveModule->setLevel(overdriveLevel);
        }
        else
        {
            overdriveModule->setDrive(0.0f);
            overdriveModule->setLevel(0.0f);
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

        // Voz A carrega a IR selecionada; voz B fica muda
        // (blend = 0). Low/High cut ajustam o timbre.
        cabinetModule->configureVoice(
            0, 0.0f, lowCut, highCut, 0.0f, 0.5f, 45.0f);
        cabinetModule->configureVoice(
            1, 0.0f, lowCut, highCut, 0.0f, 0.5f, 45.0f);

        // blend 0 = 100% voz A. O +6 dB compensa a lei de
        // pan (~0.5) do CabinetMixer. levelDb e o makeup do usuario.
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
        ParameterID::inputGain, "Input Gain", 0.0f, 2.0f, 1.0f));

    parameters.push_back(std::make_unique<Parameter>(
        ParameterID::outputGain, "Output Gain", 0.0f, 2.0f, 1.0f));

    //========================================================
    // AMP
    //========================================================

    parameters.push_back(std::make_unique<Parameter>(
        ParameterID::ampGain, "Amp Gain", 0.0f, 10.0f, 5.0f));

    parameters.push_back(std::make_unique<Parameter>(
        ParameterID::bass, "Bass", 0.0f, 10.0f, 5.0f));

    parameters.push_back(std::make_unique<Parameter>(
        ParameterID::middle, "Middle", 0.0f, 10.0f, 6.0f));

    parameters.push_back(std::make_unique<Parameter>(
        ParameterID::treble, "Treble", 0.0f, 10.0f, 5.0f));

    parameters.push_back(std::make_unique<Parameter>(
        ParameterID::presence, "Presence", 0.0f, 10.0f, 5.0f));

    parameters.push_back(std::make_unique<Parameter>(
        ParameterID::master, "Master", 0.0f, 10.0f, 6.0f));

    //========================================================
    // OVERDRIVE
    //========================================================

    parameters.push_back(std::make_unique<Toggle>(
        ParameterID::overdriveOn, "Overdrive On", false));

    parameters.push_back(std::make_unique<Parameter>(
        ParameterID::overdriveDrive, "Overdrive Drive",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.01f), 35.0f));

    parameters.push_back(std::make_unique<Parameter>(
        ParameterID::overdriveLevel, "Overdrive Level",
        juce::NormalisableRange<float>(-60.0f, 12.0f, 0.01f), 0.0f));

    parameters.push_back(std::make_unique<Parameter>(
        ParameterID::overdriveTone, "Overdrive Tone",
        juce::NormalisableRange<float>(0.0f, 10.0f, 0.01f), 6.0f));

    //========================================================
    // CABINET
    //========================================================

    parameters.push_back(std::make_unique<Toggle>(
        ParameterID::cabinetOn, "Cabinet On", true));

    // MENU DE CABINETS (troca de IR).
    // A ORDEM E O NUMERO de nomes precisa bater com a tabela em
    // loadCabinetIR() e com os itens do ComboBox no editor.
    parameters.push_back(std::make_unique<Choice>(
        ParameterID::cabinetModel,
        "Cabinet",
        juce::StringArray
        {
            "BD_CL_Telocastme",   // Slot 1
            "Marshall Cab",     // Slot 2
            "BD_HV_Creamback3_mixed",  // Slot 3 (troque pelo seu IR)
            "BD_LD_HairApparently",  // Slot 4 (troque pelo seu IR)
            "BD_RH_GatesOfHell",            // Slot 5 (troque pelo seu IR)
            "Cab 6"             // Slot 6 (troque pelo seu IR)
        },
        0));

    parameters.push_back(std::make_unique<Parameter>(
        ParameterID::cabinetLowCut, "Cabinet Low Cut",
        juce::NormalisableRange<float>(20.0f, 300.0f, 1.0f, 0.5f), 80.0f));

    parameters.push_back(std::make_unique<Parameter>(
        ParameterID::cabinetHighCut, "Cabinet High Cut",
        juce::NormalisableRange<float>(2000.0f, 20000.0f, 1.0f, 0.5f), 12000.0f));

    parameters.push_back(std::make_unique<Parameter>(
        ParameterID::cabinetLevel, "Cabinet Level",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f));

    parameters.push_back(std::make_unique<Parameter>(
        ParameterID::cabinetMix, "Cabinet Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f));

    //========================================================
    // REVERB
    //========================================================

    parameters.push_back(std::make_unique<Toggle>(
        ParameterID::reverbOn, "Reverb On", false));

    parameters.push_back(std::make_unique<Parameter>(
        ParameterID::reverbMix, "Reverb Mix", 0.0f, 1.0f, 0.15f));

    //========================================================
    // DELAY
    //========================================================

    parameters.push_back(std::make_unique<Toggle>(
        ParameterID::delayOn, "Delay On", false));

    parameters.push_back(std::make_unique<Parameter>(
        ParameterID::delayTime, "Delay Time", 1.0f, 2000.0f, 380.0f));

    parameters.push_back(std::make_unique<Parameter>(
        ParameterID::delayFeedback, "Delay Feedback", 0.0f, 0.95f, 0.35f));

    parameters.push_back(std::make_unique<Parameter>(
        ParameterID::delayMix, "Delay Mix", 0.0f, 1.0f, 0.20f));

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
