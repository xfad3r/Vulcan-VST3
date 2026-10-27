#include "PluginProcessor.h"

VulcanProcessor::VulcanProcessor()
    : AudioProcessor (BusesProperties()
                      .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    // Calibrated Presets for ALMORPH (Hip-Hop / Electronic)
    presets = {
        { "01. Default / Clean",   0.0f,  0.0f,  0.0f,  0.0f, 0.0f, 0.0f,  0.0f },
        { "02. Glue & Punch",      0.0f,  0.5f,  0.0f,  0.5f, 0.3f, 1.5f, -0.5f },
        { "03. 808 Sub Enhancer",  0.0f,  1.5f, -0.5f,  0.5f, 0.6f, 2.5f, -1.0f },
        { "04. Drill Aggression",  0.0f,  2.0f,  0.5f,  1.5f, 1.0f, 4.0f, -1.5f }
    };
}

VulcanProcessor::~VulcanProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout VulcanProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("input_gain", 1), "Input Gain", -24.0f, 24.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("eq_low", 1), "Pre EQ Low (100Hz)", -12.0f, 12.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("eq_mid", 1), "Pre EQ Mid (2.5kHz)", -12.0f, 12.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("eq_high", 1), "Pre EQ High (10kHz)", -12.0f, 12.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("amount", 1), "Character Amount", 0.0f, 2.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("limiter_drive", 1), "Limiter Drive", 0.0f, 18.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("output_gain", 1), "Output Gain", -24.0f, 24.0f, 0.0f));
    return { params.begin(), params.end() };
}

// -------------------------------------------------------------------------
// Preset Functions Implementation (Fixes LNK1120 & LNK2001)
// -------------------------------------------------------------------------
int VulcanProcessor::getNumPrograms() { return presets.size(); }
int VulcanProcessor::getCurrentProgram() { return currentPreset; }

void VulcanProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= presets.size()) return;
    currentPreset = index;
    auto& p = presets[index];
    
    auto setParam = [&](const juce::String& id, float val) {
        auto* param = apvts.getParameter(id);
        if (param != nullptr)
            param->setValueNotifyingHost(param->convertTo0to1(val));
    };

    setParam("input_gain", p.inGain);
    setParam("eq_low", p.eqLow);
    setParam("eq_mid", p.eqMid);
    setParam("eq_high", p.eqHigh);
    setParam("amount", p.amount);
    setParam("limiter_drive", p.limDrive);
    setParam("output_gain", p.outGain);
}

const juce::String VulcanProcessor::getProgramName (int index)
{
    if (index >= 0 && index < presets.size()) return presets[index].name;
    return {};
}

void VulcanProcessor::changeProgramName (int index, const juce::String& newName)
{
    if (index >= 0 && index < presets.size()) presets[index].name = newName;
}

// -------------------------------------------------------------------------
// Core DSP Implementation
// -------------------------------------------------------------------------
bool VulcanProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    return true;
}

void VulcanProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32> (getMainBusNumInputChannels());

    inputGainModule.prepare(spec);
    preEqChain.prepare(spec);
    updatePreEq();

    lowMidCrossoverLP.prepare(spec);
    lowMidCrossoverHP.prepare(spec);
    midHighCrossoverLP.prepare(spec);
    midHighCrossoverHP.prepare(spec);

    lowMidCrossoverLP.setCutoffFrequency(250.0f);
    lowMidCrossoverLP.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
    lowMidCrossoverHP.setCutoffFrequency(250.0f);
    lowMidCrossoverHP.setType(juce::dsp::LinkwitzRileyFilterType::highpass);

    midHighCrossoverLP.setCutoffFrequency(4000.0f);
    midHighCrossoverLP.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
    midHighCrossoverHP.setCutoffFrequency(4000.0f);
    midHighCrossoverHP.setType(juce::dsp::LinkwitzRileyFilterType::highpass);

    // 0: Low Band (<250Hz) - Slow attack to let kicks/808s punch through
    bandCompressors[0].prepare(spec);
    bandCompressors[0].setAttack(50.0f); 
    bandCompressors[0].setRelease(150.0f);
    bandBuffers[0].setSize(getMainBusNumInputChannels(), samplesPerBlock);

    // 1: Mid Band (250Hz - 4kHz) - Fast attack for vocal/snare consistency
    bandCompressors[1].prepare(spec);
    bandCompressors[1].setAttack(15.0f);
    bandCompressors[1].setRelease(80.0f);
    bandBuffers[1].setSize(getMainBusNumInputChannels(), samplesPerBlock);

    // 2: High Band (>4kHz) - Very fast attack to catch harsh transients/hi-hats
    bandCompressors[2].prepare(spec);
    bandCompressors[2].setAttack(5.0f);
    bandCompressors[2].setRelease(50.0f);
    bandBuffers[2].setSize(getMainBusNumInputChannels(), samplesPerBlock);

    limiter.prepare(spec);
    limiter.setThreshold(0.0f);
    limiter.setRelease(50.0f);
    outputGainModule.prepare(spec);
}

void VulcanProcessor::releaseResources() {}

void VulcanProcessor::updatePreEq()
{
    auto lowGain = juce::Decibels::decibelsToGain(apvts.getRawParameterValue("eq_low")->load());
    auto midGain = juce::Decibels::decibelsToGain(apvts.getRawParameterValue("eq_mid")->load());
    auto highGain = juce::Decibels::decibelsToGain(apvts.getRawParameterValue("eq_high")->load());

    preEqChain.get<0>().coefficients = juce::dsp::IIR::Coefficients<float>::makeLowShelf(currentSampleRate, 100.0f, 0.707f, lowGain);
    preEqChain.get<1>().coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(currentSampleRate, 2500.0f, 1.0f, midGain);
    preEqChain.get<2>().coefficients = juce::dsp::IIR::Coefficients<float>::makeHighShelf(currentSampleRate, 10000.0f, 0.707f, highGain);
}

void VulcanProcessor::applySaturation(juce::AudioBuffer<float>& buffer, float amount)
{
    // Restoring the Web App's exact non-linear normalization logic
    float driveFactor = 1.0f + (amount * 1.5f);
    float drive = juce::jmax(0.1f, driveFactor);
    float norm = std::tanh(drive);
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    for (int channel = 0; channel < numChannels; ++channel)
    {
        float* samples = buffer.getWritePointer(channel);
        for (int i = 0; i < numSamples; ++i)
        {
            samples[i] = std::tanh(samples[i] * drive) / norm;
        }
    }
}

void VulcanProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    inputGainModule.setGainDecibels(apvts.getRawParameterValue("input_gain")->load());
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    inputGainModule.process(context);
    updatePreEq();
    preEqChain.process(context);

    float amount = apvts.getRawParameterValue("amount")->load();
    float thresholdDb = (amount <= 0.001f) ? 0.0f : (-6.0f - (amount * 10.0f));
    float ratio = 1.0f + (amount * 1.75f);

    for (int i = 0; i < 3; ++i)
    {
        bandCompressors[i].setThreshold(thresholdDb);
        bandCompressors[i].setRatio(ratio);
        bandBuffers[i].setSize(numChannels, numSamples, false, false, true);
        bandBuffers[i].clear();
    }

    for (int ch = 0; ch < numChannels; ++ch)
    {
        const float* src = buffer.getReadPointer(ch);
        float* lowPtr = bandBuffers[0].getWritePointer(ch);
        float* midPtr = bandBuffers[1].getWritePointer(ch);
        float* highPtr = bandBuffers[2].getWritePointer(ch);

        for (int s = 0; s < numSamples; ++s)
        {
            float inputSample = src[s];
            float lowPart = lowMidCrossoverLP.processSample(ch, inputSample);
            float highPart = lowMidCrossoverHP.processSample(ch, inputSample);
            float midPart = midHighCrossoverLP.processSample(ch, highPart);
            float highFinal = midHighCrossoverHP.processSample(ch, highPart);

            lowPtr[s] = lowPart;
            midPtr[s] = midPart;
            highPtr[s] = highFinal;
        }
    }

    buffer.clear();
    for (int i = 0; i < 3; ++i)
    {
        juce::dsp::AudioBlock<float> bandBlock(bandBuffers[i]);
        juce::dsp::ProcessContextReplacing<float> bandContext(bandBlock);
        bandCompressors[i].process(bandContext);
        
        applySaturation(bandBuffers[i], amount);

        for (int ch = 0; ch < numChannels; ++ch)
            buffer.addFrom(ch, 0, bandBuffers[i], ch, 0, numSamples);
    }

    float limiterDrive = juce::Decibels::decibelsToGain(apvts.getRawParameterValue("limiter_drive")->load());
    buffer.applyGain(limiterDrive);
    limiter.process(context);

    outputGainModule.setGainDecibels(apvts.getRawParameterValue("output_gain")->load());
    outputGainModule.process(context);
}

void VulcanProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void VulcanProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VulcanProcessor();
}
