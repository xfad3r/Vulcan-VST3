#pragma once

#include <JuceHeader.h>

class VulcanAudioProcessor : public juce::AudioProcessor
{
public:
    VulcanAudioProcessor();
    ~VulcanAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }

    const juce::String getName() const override { return "VULCAN by DJSmartie"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    // Metering API for Editor/GUI
    float getInputPeak() const noexcept { return inputPeak.load(); }
    float getInputRMS() const noexcept { return inputRMS.load(); }
    float getOutputPeak() const noexcept { return outputPeak.load(); }
    float getOutputRMS() const noexcept { return outputRMS.load(); }

private:
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // DSP Components: Stage 1 - Input Staging & Pre-EQ
    juce::dsp::Gain<float> inputGainModule;
    juce::dsp::ProcessorChain<juce::dsp::IIR::Filter<float>, 
juce::dsp::IIR::Filter<float>> preEqChain;

    // DSP Components: Stage 2 - Multiband Character Engine (3 Bands Linkwitz-Riley)
    static constexpr int numBands = 3;
    juce::dsp::LinkwitzRiley<float> lowMidCrossoverLP;
    juce::dsp::LinkwitzRiley<float> lowMidCrossoverHP;
    juce::dsp::LinkwitzRiley<float> midHighCrossoverLP;
    juce::dsp::LinkwitzRiley<float> midHighCrossoverHP;

    std::array<juce::dsp::Compressor<float>, numBands> bandCompressors;

    // Internal Multiband Audio Buffers
    juce::AudioBuffer<float> bandBuffers[numBands];
    juce::AudioBuffer<float> sidechainBuffer;

    // DSP Components: Stage 3 & 4 - Limiter & Output Staging
    juce::dsp::Limiter<float> limiter;
    juce::dsp::Gain<float> outputGainModule;

    // Parameter Caches
    std::atomic<float>* inputGainParam  = nullptr;
    std::atomic<float>* eqLowParam      = nullptr;
    std::atomic<float>* eqMidParam      = nullptr;
    std::atomic<float>* eqHighParam     = nullptr;
    std::atomic<float>* amountParam     = nullptr;
    std::atomic<float>* extScParam      = nullptr;
    std::atomic<float>* limitInParam    = nullptr;
    std::atomic<float>* outputGainParam = nullptr;

    // Metering Atomics
    std::atomic<float> inputPeak  { 0.0f };
    std::atomic<float> inputRMS   { 0.0f };
    std::atomic<float> outputPeak { 0.0f };
    std::atomic<float> outputRMS  { 0.0f };

    double currentSampleRate = 44100.0;

    void updatePreEq();
    void applySaturation(juce::AudioBuffer<float>& buffer, float driveGainFactor);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VulcanAudioProcessor)
};
