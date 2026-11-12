#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <vector>
#include <memory>

class VulcanProcessor : public juce::AudioProcessor
{
public:
    VulcanProcessor();
    ~VulcanProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    // Public state for Editor attachments & lock-free metering
    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> peakL { 0.0f };
    std::atomic<float> peakR { 0.0f };

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void updatePreEq();
    void updateCrossover();

    struct Preset
    {
        juce::String name;
        float inGain;
        float eqLow;
        float eqMid;
        float eqHigh;
        float monoFreq;
        float haasWidth;
        float amount;
        float limDrive;
        float outGain;
    };

    std::vector<Preset> presets;
    int currentPreset = 0;

    double currentSampleRate = 44100.0;
    float currentMonoFreq = -1.0f;

    using FilterBand = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                                      juce::dsp::IIR::Coefficients<float>>;

    juce::dsp::Gain<float> inputGainModule;
    juce::dsp::ProcessorChain<FilterBand, FilterBand, FilterBand> preEqChain;

    juce::dsp::ProcessorChain<FilterBand, FilterBand> lpLowMid, hpLowMid;
    juce::dsp::ProcessorChain<FilterBand, FilterBand> lpMidHigh, hpMidHigh;

    juce::AudioBuffer<float> splitBuffer;
    juce::AudioBuffer<float> bandBuffers[3];
    juce::dsp::Compressor<float> bandCompressors[3];
    std::unique_ptr<juce::dsp::Oversampling<float>> oversamplers[3];

    std::array<float, 8192> midDelayBufferR {};
    int midDelayWriteIdxR = 0;

    std::array<float, 8192> highDelayBufferL {};
    int highDelayWriteIdxL = 0;

    float noiseHpfState = 0.0f;
    juce::Random rng;

    juce::dsp::Compressor<float> limiter;
    juce::dsp::Gain<float> outputGainModule;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VulcanProcessor)
};
