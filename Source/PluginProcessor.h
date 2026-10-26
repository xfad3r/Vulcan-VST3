#pragma once
#include <JuceHeader.h>

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
    bool hasEditor() const override { return true; } // Generic UI

    const juce::String getName() const override { return "VULCAN by DJSmartie"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Stage 1: Input & EQ
    juce::dsp::Gain<float> inputGainModule;
    using Filter = juce::dsp::IIR::Filter<float>;
    juce::dsp::ProcessorChain<Filter, Filter, Filter> preEqChain;

    // Stage 2: Multiband Crossovers & Compressors
    juce::dsp::LinkwitzRileyFilter<float> lowMidCrossoverLP;
    juce::dsp::LinkwitzRileyFilter<float> lowMidCrossoverHP;
    juce::dsp::LinkwitzRileyFilter<float> midHighCrossoverLP;
    juce::dsp::LinkwitzRileyFilter<float> midHighCrossoverHP;

    std::array<juce::dsp::Compressor<float>, 3> bandCompressors;
    juce::AudioBuffer<float> bandBuffers[3];

    // Stage 3 & 4: Limiter & Output
    juce::dsp::Limiter<float> limiter;
    juce::dsp::Gain<float> outputGainModule;

    double currentSampleRate = 44100.0;
    void updatePreEq();
    void applySaturation(juce::AudioBuffer<float>& buffer, float driveGainFactor);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VulcanProcessor)
};
