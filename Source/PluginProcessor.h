#pragma once
#include <JuceHeader.h>

struct Preset {
    juce::String name;
    float inGain, eqLow, eqMid, eqHigh, amount, limDrive, outGain;
};

class VulcanProcessor : public juce::AudioProcessor
{
public:
    VulcanProcessor();
    ~VulcanProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override { return new juce::GenericAudioProcessorEditor (*this); }
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "VULCAN by DJSmartie"; }
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

private:
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    std::vector<Preset> presets;
    int currentPreset = 0;

    juce::dsp::Gain<float> inputGainModule;
    
    using FilterMono = juce::dsp::IIR::Filter<float>;
    using FilterCoef = juce::dsp::IIR::Coefficients<float>;
    using StereoBiquad = juce::dsp::ProcessorDuplicator<FilterMono, FilterCoef>;

    juce::dsp::ProcessorChain<StereoBiquad, StereoBiquad, StereoBiquad> preEqChain;

    juce::dsp::ProcessorChain<StereoBiquad, StereoBiquad> lp250Chain;
    juce::dsp::ProcessorChain<StereoBiquad, StereoBiquad> hp250Chain;
    juce::dsp::ProcessorChain<StereoBiquad, StereoBiquad> lp4000Chain;
    juce::dsp::ProcessorChain<StereoBiquad, StereoBiquad> hp4000Chain;

    juce::AudioBuffer<float> splitBuffer;

    std::array<juce::dsp::Compressor<float>, 3> bandCompressors;
    juce::AudioBuffer<float> bandBuffers[3];

    // Array of 4x Oversamplers for the 3 bands
    std::array<std::unique_ptr<juce::dsp::Oversampling<float>>, 3> oversamplers;

    juce::dsp::Compressor<float> limiter;
    juce::dsp::Gain<float> outputGainModule;

    double currentSampleRate = 44100.0;
    void updatePreEq();
    
    // Saturation now directly processes an AudioBlock (needed for Oversampling API)
    void applySaturation(juce::dsp::AudioBlock<float>& block, float amount);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VulcanProcessor)
};
