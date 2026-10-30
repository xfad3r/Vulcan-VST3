#include "PluginProcessor.h"
#include "PluginEditor.h"

VulcanProcessor::VulcanProcessor()
    : AudioProcessor (BusesProperties()
                      .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    presets = {
        { "01. Default / Clean",   0.0f,  0.0f,  0.0f,  0.0f, 0.0f, 0.0f,  0.0f },
        { "02. Glue & Punch",      0.0f,  0.5f,  0.0f,  0.5f, 0.3f, 1.5f, -0.5f },
        { "03. 808 Sub Enhancer",  0.0f,  1.5f, -0.5f,  0.5f, 0.6f, 2.5f, -1.0f },
        { "04. Drill Aggression",  0.0f,  2.0f,  0.5f,  1.5f, 1.0f, 4.0f, -1.5f }
    };

    for (int i = 0; i < 3; ++i) {
        oversamplers[i] = std::make_unique<juce::dsp::Oversampling<float>>(
            2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true
        );
    }
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

int VulcanProcessor::getNumPrograms() { return static_cast<int>(presets.size()); }
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

    updatePreEq(); 
    
    // Αρχικοποίηση Buffer Καθυστέρησης (Haas Effect)
    std::fill(highBandDelayBuffer.begin(), highBandDelayBuffer.end(), 0.0f);
    delayWriteIndex = 0;

    auto lp250 = juce::dsp::IIR::Coefficients<float>::makeLowPass(currentSampleRate, 250.0f, 0.7071f);
    auto hp250 = juce::dsp::IIR::Coefficients<float>::makeHighPass(currentSampleRate, 250.0f, 0.7071f);
    auto lp4000 = juce::dsp::IIR::Coefficients<float>::makeLowPass(currentSampleRate, 4000.0f, 0.7071f);
    auto hp4000 = juce::dsp::IIR::Coefficients<float>::makeHighPass(currentSampleRate, 4000.0f, 0.7071f);

    lp250Chain.get<0>().state = lp250;
    lp250Chain.get<1>().state = lp250;
    hp250Chain.get<0>().state = hp250;
    hp250Chain.get<1>().state = hp250;
    lp4000Chain.get<0>().state = lp4000;
    lp4000Chain.get<1>().state = lp4000;
    hp4000Chain.get<0>().state = hp4000;
    hp4000Chain.get<1>().state = hp4000;

    inputGainModule.prepare(spec);
    preEqChain.prepare(spec);
    lp250Chain.prepare(spec);
    hp250Chain.prepare(spec);
    lp4000Chain.prepare(spec);
    hp4000Chain.prepare(spec);

    splitBuffer.setSize(getMainBusNumInputChannels(), samplesPerBlock);

    for (int i = 0; i < 3; ++i)
    {
        bandCompressors[i].prepare(spec);
        bandCompressors[i].setAttack(20.0f);
        bandCompressors[i].setRelease(120.0f);
        bandBuffers[i].setSize(getMainBusNumInputChannels(), samplesPerBlock);
        
        oversamplers[i]->initProcessing(samplesPerBlock);
        oversamplers[i]->reset();
    }

    limiter.prepare(spec);
    limiter.setThreshold(-0.1f);
    limiter.setRatio(20.0f);
    limiter.setAttack(1.0f); 
    limiter.setRelease(50.0f);
    
    outputGainModule.prepare(spec);
}

void VulcanProcessor::releaseResources()
{
    for (int i = 0; i < 3; ++i)
        oversamplers[i]->reset();
}

void VulcanProcessor::updatePreEq()
{
    auto lowGain = juce::Decibels::decibelsToGain(apvts.getRawParameterValue("eq_low")->load());
    auto midGain = juce::Decibels::decibelsToGain(apvts.getRawParameterValue("eq_mid")->load());
    auto highGain = juce::Decibels::decibelsToGain(apvts.getRawParameterValue("eq_high")->load());

    preEqChain.get<0>().state = juce::dsp::IIR::Coefficients<float>::makeLowShelf(currentSampleRate, 100.0f, 0.707f, lowGain);
    preEqChain.get<1>().state = juce::dsp::IIR::Coefficients<float>::makePeakFilter(currentSampleRate, 2500.0f, 1.0f, midGain);
    preEqChain.get<2>().state = juce::dsp::IIR::Coefficients<float>::makeHighShelf(currentSampleRate, 10000.0f, 0.707f, highGain);
}

void VulcanProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    splitBuffer.setSize(numChannels, numSamples, false, false, true);

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

    for (int ch = 0; ch < numChannels; ++ch) {
        bandBuffers[0].copyFrom(ch, 0, buffer, ch, 0, numSamples);
        splitBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
    }

    juce::dsp::AudioBlock<float> lowBlock(bandBuffers[0]);
    juce::dsp::ProcessContextReplacing<float> lowContext(lowBlock);
    lp250Chain.process(lowContext);

    juce::dsp::AudioBlock<float> splitBlock(splitBuffer);
    juce::dsp::ProcessContextReplacing<float> splitContext(splitBlock);
    hp250Chain.process(splitContext);

    for (int ch = 0; ch < numChannels; ++ch) {
        bandBuffers[1].copyFrom(ch, 0, splitBuffer, ch, 0, numSamples);
        bandBuffers[2].copyFrom(ch, 0, splitBuffer, ch, 0, numSamples);
    }

    juce::dsp::AudioBlock<float> midBlock(bandBuffers[1]);
    juce::dsp::ProcessContextReplacing<float> midContext(midBlock);
    lp4000Chain.process(midContext);

    juce::dsp::AudioBlock<float> highBlock(bandBuffers[2]);
    juce::dsp::ProcessContextReplacing<float> highContext(highBlock);
    hp4000Chain.process(highContext);

    buffer.clear();
    
    for (int i = 0; i < 3; ++i)
    {
        juce::dsp::AudioBlock<float> bandBlock(bandBuffers[i]);
        juce::dsp::ProcessContextReplacing<float> bandContext(bandBlock);
        
        // --- High-Band Stereo Decorrelation (Haas Effect) ---
        if (i == 2 && bandBlock.getNumChannels() > 1 && currentSampleRate > 0.0) {
            int delaySamples = static_cast<int>(currentSampleRate * 0.0015f); // 1.5ms
            float* rightChannel = bandBlock.getChannelPointer(1);
            
            for (size_t s = 0; s < bandBlock.getNumSamples(); ++s) {
                float input = rightChannel[s];
                int readPos = delayWriteIndex - delaySamples;
                if (readPos < 0) readPos += 8192;
                
                rightChannel[s] = highBandDelayBuffer[readPos];
                highBandDelayBuffer[delayWriteIndex] = input;
                delayWriteIndex = (delayWriteIndex + 1) % 8192;
            }
        }

        bandCompressors[i].process(bandContext);
        
        auto oversampledBlock = oversamplers[i]->processSamplesUp(bandBlock);
        
        const float driveFactor = 1.0f + (amount * 1.5f);
        const float drive = juce::jmax(0.1f, driveFactor);
        const float norm = std::tanh(drive);

        for (size_t ch = 0; ch < oversampledBlock.getNumChannels(); ++ch)
        {
            float* ptr = oversampledBlock.getChannelPointer(ch);
            for (size_t s = 0; s < oversampledBlock.getNumSamples(); ++s)
            {
                ptr[s] = std::tanh(ptr[s] * drive) / norm;
            }
        }
        
        oversamplers[i]->processSamplesDown(bandBlock);

        for (int ch = 0; ch < numChannels; ++ch)
            buffer.addFrom(ch, 0, bandBuffers[i], ch, 0, numSamples);
    }

    float limiterDrive = juce::Decibels::decibelsToGain(apvts.getRawParameterValue("limiter_drive")->load());
    buffer.applyGain(limiterDrive);
    limiter.process(context);

    outputGainModule.setGainDecibels(apvts.getRawParameterValue("output_gain")->load());
    outputGainModule.process(context);

    // 16-Bit TPDF Dither & PCM Truncation (Ακριβές Noise Floor στα -96dB)
    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* channelData = buffer.getWritePointer(ch);
        for (int s = 0; s < numSamples; ++s)
        {
            float r1 = juce::Random::getSystemRandom().nextFloat();
            float r2 = juce::Random::getSystemRandom().nextFloat();
            float dither = (r1 - r2) * (1.0f / 32767.0f); 
            
            float sample = juce::jlimit(-1.0f, 1.0f, channelData[s] + dither);
            int pcm16 = static_cast<int>(sample * 32767.0f);
            channelData[s] = static_cast<float>(pcm16) / 32767.0f;
        }
    }
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

juce::AudioProcessorEditor* VulcanProcessor::createEditor()
{
    return new VulcanEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VulcanProcessor();
}
