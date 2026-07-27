#pragma once

#include "CabinetDataTypes.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <vector>
#include <complex>

class IRLoader final : private juce::ThreadPoolJob
{
public:
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void irLoadingFinished (const juce::String& voiceId, CachedIRData::Ptr irData) = 0;
        virtual void irLoadingFailed (const juce::String& voiceId, const juce::String& reason) = 0;
    };

    explicit IRLoader (Listener* l);
    ~IRLoader() override;

    void prepare (double sampleRate);
    void submitLoadTask (const juce::String& voiceId, const juce::File& file);
    void submitLoadTask (const juce::String& voiceId, const void* binaryData, size_t dataSize, const juce::String& customHash);
    
private:
    juce::ThreadPoolJob::JobStatus runJob() override;

    struct Task
    {
        juce::String voiceId;
        juce::File file;
        const void* binaryData = nullptr;
        size_t binaryDataSize = 0;
        juce::String explicitHash;
        bool isFactory = false;
    };

    juce::String generateSecureFileHash (const juce::File& f);
    void executeMinimumPhaseEngine (juce::AudioBuffer<float>& buffer);

    Listener* listener;
    double targetSampleRate = 44100.0;
    juce::AudioFormatManager formatManager;
    juce::ThreadPool threadPool { 1 };
    
    juce::CriticalSection queueLock;
    juce::Array<Task> taskQueue;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IRLoader)
};