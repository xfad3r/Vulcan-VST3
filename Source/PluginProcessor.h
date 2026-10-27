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
    using Biquad = juce::dsp::IIR::Filter<float>;
    juce::dsp::ProcessorChain<Biquad, Biquad, Biquad> preEqChain;

    // WebAudio Emulation: Cascaded Biquads instead of Linkwitz-Riley
    juce::dsp::ProcessorChain<Biquad, Biquad> lp250Chain;
    juce::dsp::ProcessorChain<Biquad, Biquad> hp250Chain;
    juce::dsp::ProcessorChain<Biquad, Biquad> lp4000Chain;
    juce::dsp::ProcessorChain<Biquad, Biquad> hp4000Chain;

    juce::AudioBuffer<float> splitBuffer; // Temp buffer for the WebAudio routing simulation

    std::array<juce::dsp::Compressor<float>, 3> bandCompressors;
    juce::AudioBuffer<float> bandBuffers[3];

    juce::dsp::Limiter<float> limiter;
    juce::dsp::Gain<float> outputGainModule;

    double currentSampleRate = 44100.0;
    void updatePreEq();
    void applySaturation(juce::AudioBuffer<float>& buffer, float amount);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VulcanProcessor)
};
