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

    // Preset / Program Management API
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
    using Filter = juce::dsp::IIR::Filter<float>;
    juce::dsp::ProcessorChain<Filter, Filter, Filter> preEqChain;

    juce::dsp::LinkwitzRileyFilter<float> lowMidCrossoverLP;
    juce::dsp::LinkwitzRileyFilter<float> lowMidCrossoverHP;
    juce::dsp::LinkwitzRileyFilter<float> midHighCrossoverLP;
    juce::dsp::LinkwitzRileyFilter<float> midHighCrossoverHP;

    std::array<juce::dsp::Compressor<float>, 3> bandCompressors;
    juce::AudioBuffer<float> bandBuffers[3];

    juce::dsp::Limiter<float> limiter;
    juce::dsp::Gain<float> outputGainModule;

    double currentSampleRate = 44100.0;
    void updatePreEq();
    
    // Fixed DSP Signature
    void applySaturation(juce::AudioBuffer<float>& buffer, float amount);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VulcanProcessor)
};
