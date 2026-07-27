#include "IRLoader.h"
#include "IRCache.h"

IRLoader::IRLoader (Listener* l) 
    : juce::ThreadPoolJob ("SlashRing_AsyncIRWorker"), 
      listener (l)
{
    formatManager.registerBasicFormats();
}

IRLoader::~IRLoader()
{
    threadPool.removeAllJobs (true, 4000);
}

void IRLoader::prepare (double sampleRate)
{
    targetSampleRate = sampleRate;
}

void IRLoader::submitLoadTask (const juce::String& voiceId, const juce::File& file)
{
    const juce::ScopedLock sl (queueLock);
    Task t;
    t.voiceId = voiceId;
    t.file = file;
    t.isFactory = false;
    taskQueue.add (t);
    threadPool.addJob (this, false);
}

void IRLoader::submitLoadTask (const juce::String& voiceId, const void* binaryData, size_t dataSize, const juce::String& customHash)
{
    const juce::ScopedLock sl (queueLock);
    Task t;
    t.voiceId = voiceId;
    t.binaryData = binaryData;
    t.binaryDataSize = dataSize;
    t.explicitHash = customHash;
    t.isFactory = true;
    taskQueue.add (t);
    threadPool.addJob (this, false);
}

juce::String IRLoader::generateSecureFileHash (const juce::File& f)
{
    int64_t size = f.getSize();
    int64_t time = f.getLastModificationTime().toMilliseconds();
    
    uint64_t hash = 0xcbf29ce484222325ULL;
    auto updateHash = [&hash](uint64_t value) {
        hash ^= value;
        hash *= 0x00000100000001B3ULL;
    };
    
    updateHash (static_cast<uint64_t> (size));
    updateHash (static_cast<uint64_t> (time));
    return juce::String::toHexString (hash);
}

void IRLoader::executeMinimumPhaseEngine (juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    int fftSize = 1;
    while (fftSize < numSamples * 2) { fftSize <<= 1; }

    std::unique_ptr<juce::dsp::FFT> fft = std::make_unique<juce::dsp::FFT> (juce::roundToInt (std::log2 (fftSize)));
    
    std::vector<std::complex<float>> timeData (static_cast<size_t> (fftSize), 0.0f);
    std::vector<std::complex<float>> freqData (static_cast<size_t> (fftSize), 0.0f);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        std::fill (timeData.begin(), timeData.end(), 0.0f);
        const float* readPtr = buffer.getReadPointer (ch);
        float* writePtr = buffer.getWritePointer (ch);
        
        for (int i = 0; i < numSamples; ++i)
            timeData[static_cast<size_t> (i)] = readPtr[i];

        // CORREÇÃO INTEGRAL: performFrequencyOnlyForwardTransform substitui performAbsoluteFFT.
        fft->performFrequencyOnlyForwardTransform (reinterpret_cast<float*> (timeData.data()));

        for (size_t i = 0; i < static_cast<size_t> (fftSize); ++i)
        {
            float mag = std::abs (timeData[i]);
            freqData[i] = std::log (std::max (mag, 1e-8f));
        }

        // CORREÇÃO INTEGRAL: performRealOnlyInverseTransform substitui performRealInverseFFT.
        fft->performRealOnlyInverseTransform (reinterpret_cast<float*> (freqData.data()));

        for (size_t i = 1; i < static_cast<size_t> (fftSize) / 2; ++i)
            freqData[i] *= 2.0f;
        for (size_t i = static_cast<size_t> (fftSize) / 2 + 1; i < static_cast<size_t> (fftSize); ++i)
            freqData[i] = 0.0f;

        fft->performFrequencyOnlyForwardTransform (reinterpret_cast<float*> (freqData.data()));

        for (size_t i = 0; i < static_cast<size_t> (fftSize); ++i)
            timeData[i] = std::exp (freqData[i]);

        fft->performRealOnlyInverseTransform (reinterpret_cast<float*> (timeData.data()));

        for (int i = 0; i < numSamples; ++i)
            writePtr[i] = reinterpret_cast<float*> (timeData.data())[i];
    }
}

juce::ThreadPoolJob::JobStatus IRLoader::runJob()
{
    Task task;
    {
        const juce::ScopedLock sl (queueLock);
        if (taskQueue.isEmpty())
            return jobHasFinished;
        task = taskQueue.removeAndReturn (0);
    }

    juce::String hash = task.isFactory ? task.explicitHash : generateSecureFileHash (task.file);
    
    if (auto cached = IRCache::getInstance().get (hash))
    {
        if (listener != nullptr) 
            listener->irLoadingFinished (task.voiceId, cached);
        return jobHasFinished;
    }

    std::unique_ptr<juce::AudioFormatReader> reader;
    if (task.isFactory)
    {
        reader.reset (formatManager.createReaderFor (std::make_unique<juce::MemoryInputStream> (task.binaryData, task.binaryDataSize, false)));
    }
    else
    {
        if (!task.file.existsAsFile())
        {
            if (listener != nullptr) listener->irLoadingFailed (task.voiceId, "File target does not exist");
            return jobHasFinished;
        }
        reader.reset (formatManager.createReaderFor (task.file));
    }

    if (!reader)
    {
        if (listener != nullptr) listener->irLoadingFailed (task.voiceId, "Codec compression mismatch");
        return jobHasFinished;
    }

    int sourceSamples = static_cast<int> (reader->lengthInSamples);
    juce::AudioBuffer<float> rawBuffer (static_cast<int> (reader->numChannels), sourceSamples);
    reader->read (&rawBuffer, 0, sourceSamples, 0, true, true);

    if (reader->sampleRate != targetSampleRate)
    {
        double ratio = reader->sampleRate / targetSampleRate;
        int targetSamples = juce::roundToInt (static_cast<double> (sourceSamples) / ratio);
        juce::AudioBuffer<float> resampledBuffer (rawBuffer.getNumChannels(), targetSamples);
        
        // CORREÇÃO INTEGRAL: juce::LagrangeInterpolator substitui o namespace inexistente juce::Interpolatables.
        juce::LagrangeInterpolator resampler;
        for (int c = 0; c < rawBuffer.getNumChannels(); ++c)
        {
            resampler.reset();
            resampler.process (ratio, rawBuffer.getReadPointer (c), resampledBuffer.getWritePointer (c), targetSamples);
        }
        rawBuffer = std::move (resampledBuffer);
    }

    executeMinimumPhaseEngine (rawBuffer);

    float maxMag = rawBuffer.getMagnitude (0, rawBuffer.getNumSamples());
    if (maxMag > 0.0f)
        rawBuffer.applyGain (1.0f / maxMag * juce::Decibels::decibelsToGain (-18.0f));

    auto finalCachedObj = juce::ReferenceCountedObjectPtr<CachedIRData> (new CachedIRData (std::move (rawBuffer), targetSampleRate, hash));
    IRCache::getInstance().store (hash, finalCachedObj);

    if (listener != nullptr)
        listener->irLoadingFinished (task.voiceId, finalCachedObj);

    return jobHasFinished;
}