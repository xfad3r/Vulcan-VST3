#include "PluginProcessor.h"
#include "PluginEditor.h"

VulcanProcessor::VulcanProcessor()
    : AudioProcessor (BusesProperties()
                      .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    apvts.state = juce::ValueTree("VulcanState");

    presets = {
        { "01. Default / Clean",      0.0f,  0.0f,  0.0f,  0.0f, 250.0f, 0.46f, 0.0f, 0.0f,  0.0f },
        { "02. Glue & Punch",         0.0f,  0.5f,  0.0f,  0.5f, 250.0f, 0.46f, 0.3f, 1.5f, -0.5f },
        { "03. 808 Sub Enhancer",     0.0f,  1.5f, -0.5f,  0.5f, 150.0f, 0.46f, 0.6f, 2.5f, -1.0f },
        { "04. Drill Aggression",     0.0f,  2.0f,  0.5f,  1.5f, 300.0f, 0.85f, 1.0f, 4.0f, -1.5f },
        { "05. Vocal Silk (Air)",     0.0f, -0.5f,  1.0f,  2.5f, 200.0f, 0.60f, 0.2f, 1.0f, -0.5f },
        { "06. Analog Tape Warmth",   0.0f,  1.0f,  0.5f, -1.0f, 250.0f, 0.46f, 0.8f, 2.0f, -1.0f },
        { "07. Master Polish",        0.0f,  0.5f,  0.5f,  1.0f, 250.0f, 0.46f, 0.15f,0.8f, -0.3f },
        { "08. Lo-Fi Vibe",           0.0f,  2.0f, -1.0f, -3.0f, 400.0f, 1.0f,  1.5f, 3.0f, -1.5f }
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
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("mono_freq", 1), "Mono Lock Freq", 80.0f, 800.0f, 250.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("haas_amount", 1), "Stereo Width", 0.0f, 1.0f, 0.46f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("amount", 1), "Character Amount", 0.0f, 2.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("limiter_drive", 1), "Limiter Drive", 0.0f, 18.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("output_gain", 1), "Output Gain", -24.0f, 24.0f, 0.0f));
    return { params.begin(), params.end() };
}

int VulcanProcessor::getNumPrograms() { return static_cast<int>(presets.size()); }
int VulcanProcessor::getCurrentProgram() { return currentPreset; }

void VulcanProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= static_cast<int>(presets.size())) return;
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
    setParam("mono_freq", p.monoFreq);
    setParam("haas_amount", p.haasWidth);
    setParam("amount", p.amount);
    setParam("limiter_drive", p.limDrive);
    setParam("output_gain", p.outGain);
}

const juce::String VulcanProcessor::getProgramName (int index)
{
    if (index >= 0 && index < static_cast<int>(presets.size())) return presets[index].name;
    return {};
}

void VulcanProcessor::changeProgramName (int index, const juce::String& newName)
{
    if (index >= 0 && index < static_cast<int>(presets.size())) presets[index].name = newName;
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
    currentMonoFreq = -1.0f; 
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32> (getMainBusNumInputChannels());

    updatePreEq(); 
    
    std::fill(midDelayBufferR.begin(), midDelayBufferR.end(), 0.0f);
    midDelayWriteIdxR = 0;

    std::fill(highDelayBufferL.begin(), highDelayBufferL.end(), 0.0f);
    highDelayWriteIdxL = 0;
    
    noiseHpfState = 0.0f;
    rng.setSeedRandomly();
    
    peakL.store(0.0f);
    peakR.store(0.0f);

    float initMonoFreq = juce::jlimit(80.0f, 800.0f, apvts.getRawParameterValue("mono_freq")->load());
    currentMonoFreq = initMonoFreq;
    auto lpMono = juce::dsp::IIR::Coefficients<float>::makeLowPass(currentSampleRate, initMonoFreq, 0.7071f);
    auto hpMono = juce::dsp::IIR::Coefficients<float>::makeHighPass(currentSampleRate, initMonoFreq, 0.7071f);
    lpLowMid.template get<0>().state = lpMono;
    lpLowMid.template get<1>().state = lpMono;
    hpLowMid.template get<0>().state = hpMono;
    hpLowMid.template get<1>().state = hpMono;

    auto lpHaas = juce::dsp::IIR::Coefficients<float>::makeLowPass(currentSampleRate, 1000.0f, 0.7071f);
    auto hpHaas = juce::dsp::IIR::Coefficients<float>::makeHighPass(currentSampleRate, 1000.0f, 0.7071f);
    lpMidHigh.template get<0>().state = lpHaas;
    lpMidHigh.template get<1>().state = lpHaas;
    hpMidHigh.template get<0>().state = hpHaas;
    hpMidHigh.template get<1>().state = hpHaas;

    inputGainModule.prepare(spec);
    preEqChain.prepare(spec);
    lpLowMid.prepare(spec);
    hpLowMid.prepare(spec);
    lpMidHigh.prepare(spec);
    hpMidHigh.prepare(spec);

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

    preEqChain.template get<0>().state = juce::dsp::IIR::Coefficients<float>::makeLowShelf(currentSampleRate, 100.0f, 0.707f, lowGain);
    preEqChain.template get<1>().state = juce::dsp::IIR::Coefficients<float>::makePeakFilter(currentSampleRate, 2500.0f, 1.0f, midGain);
    preEqChain.template get<2>().state = juce::dsp::IIR::Coefficients<float>::makeHighShelf(currentSampleRate, 10000.0f, 0.707f, highGain);
}

void VulcanProcessor::updateCrossover()
{
    if (currentSampleRate <= 0.0) return;
    
    float targetFreq = juce::jlimit(80.0f, 800.0f, apvts.getRawParameterValue("mono_freq")->load());
    if (std::abs(targetFreq - currentMonoFreq) > 0.5f) {
        currentMonoFreq = targetFreq;
        auto lp = juce::dsp::IIR::Coefficients<float>::makeLowPass(currentSampleRate, targetFreq, 0.7071f);
        auto hp = juce::dsp::IIR::Coefficients<float>::makeHighPass(currentSampleRate, targetFreq, 0.7071f);
        lpLowMid.template get<0>().state = lp;
        lpLowMid.template get<1>().state = lp;
        hpLowMid.template get<0>().state = hp;
        hpLowMid.template get<1>().state = hp;
    }
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
    updateCrossover();
    preEqChain.process(context);

    float amount = apvts.getRawParameterValue("amount")->load();
    float thresholdDb = (amount <= 0.001f) ? 0.0f : (-6.0f - (amount * 10.0f));
    float ratio = 1.0f + (amount * 1.75f);
    
    float rateMultiplier = static_cast<float>(currentSampleRate) / 44100.0f;
    float haasMultiplier = apvts.getRawParameterValue("haas_amount")->load();
    
    int delaySamplesMid = static_cast<int>(std::round(23.0f * rateMultiplier * haasMultiplier));
    int delaySamplesHigh = static_cast<int>(std::round(41.0f * rateMultiplier * haasMultiplier));

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
    lpLowMid.process(lowContext);

    juce::dsp::AudioBlock<float> splitBlock(splitBuffer);
    juce::dsp::ProcessContextReplacing<float> splitContext(splitBlock);
    hpLowMid.process(splitContext);

    for (int ch = 0; ch < numChannels; ++ch) {
        bandBuffers[1].copyFrom(ch, 0, splitBuffer, ch, 0, numSamples);
        bandBuffers[2].copyFrom(ch, 0, splitBuffer, ch, 0, numSamples);
    }

    juce::dsp::AudioBlock<float> midBlock(bandBuffers[1]);
    juce::dsp::ProcessContextReplacing<float> midContext(midBlock);
    lpMidHigh.process(midContext);

    juce::dsp::AudioBlock<float> highBlock(bandBuffers[2]);
    juce::dsp::ProcessContextReplacing<float> highContext(highBlock);
    hpMidHigh.process(highContext);

    buffer.clear();
    
    for (int i = 0; i < 3; ++i)
    {
        juce::dsp::AudioBlock<float> bandBlock(bandBuffers[i]);
        juce::dsp::ProcessContextReplacing<float> bandContext(bandBlock);
        
        if (i == 0 && bandBlock.getNumChannels() > 1) {
            float* left = bandBlock.getChannelPointer(0);
            float* right = bandBlock.getChannelPointer(1);
            for (size_t s = 0; s < bandBlock.getNumSamples(); ++s) {
                float mono = 0.5f * (left[s] + right[s]);
                left[s] = mono;
                right[s] = mono;
            }
        }

        if (i == 1 && bandBlock.getNumChannels() > 1 && currentSampleRate > 0.0) {
            float* right = bandBlock.getChannelPointer(1);
            for (size_t s = 0; s < bandBlock.getNumSamples(); ++s) {
                float inR = right[s];
                if (delaySamplesMid > 0) {
                    int readPos = (midDelayWriteIdxR - delaySamplesMid) & 8191;
                    right[s] = midDelayBufferR[readPos];
                    midDelayBufferR[midDelayWriteIdxR] = inR;
                    midDelayWriteIdxR = (midDelayWriteIdxR + 1) & 8191;
                }
            }
        }

        if (i == 2 && bandBlock.getNumChannels() > 1 && currentSampleRate > 0.0) {
            float* left = bandBlock.getChannelPointer(0);
            float* right = bandBlock.getChannelPointer(1);
            for (size_t s = 0; s < bandBlock.getNumSamples(); ++s) {
                float inL = left[s];
                if (delaySamplesHigh > 0) {
                    int readPos = (highDelayWriteIdxL - delaySamplesHigh) & 8191;
                    left[s] = highDelayBufferL[readPos];
                    highDelayBufferL[highDelayWriteIdxL] = inL;
                    highDelayWriteIdxL = (highDelayWriteIdxL + 1) & 8191;
                }
                
                float mid = 0.5f * (left[s] + right[s]);
                float side = 0.5f * (left[s] - right[s]);
                float raw_noise = ((rng.nextFloat() * 2.0f) - 1.0f);
                noiseHpfState = 0.85f * noiseHpfState + 0.15f * raw_noise;
                float vhf_noise = (raw_noise - noiseHpfState) * 0.00005f; 
                side += vhf_noise;

                left[s] = mid + side;
                right[s] = mid - side;
            }
        }

        bandCompressors[i].process(bandContext);
        
        auto oversampledBlock = oversamplers[i]->processSamplesUp(bandBlock);
        
        const float driveFactor = 1.0f + (amount * 1.5f);
        const float drive = juce::jmax(0.1f, driveFactor);
        const float tanhDrive = juce::dsp::FastMathApproximations::tanh(drive);
        const float norm = tanhDrive + 0.032f * (tanhDrive * tanhDrive);

        for (size_t ch = 0; ch < oversampledBlock.getNumChannels(); ++ch)
        {
            float* ptr = oversampledBlock.getChannelPointer(ch);
            for (size_t s = 0; s < oversampledBlock.getNumSamples(); ++s)
            {
                float sat = juce::dsp::FastMathApproximations::tanh(ptr[s] * drive);
                float asymmetric = sat + 0.032f * (sat * sat) * (sat >= 0.0f ? 1.0f : -1.0f);
                ptr[s] = asymmetric / norm;
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

    float maxL = 0.0f;
    float maxR = 0.0f;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* channelData = buffer.getWritePointer(ch);
        for (int s = 0; s < numSamples; ++s)
        {
            float r1 = rng.nextFloat();
            float r2 = rng.nextFloat();
            float dither = (r1 - r2) * (1.0f / 32767.0f); 
            
            float sample = juce::jlimit(-1.0f, 1.0f, channelData[s] + dither);
            int pcm16 = static_cast<int>(sample * 32767.0f);
            float finalSample = static_cast<float>(pcm16) / 32767.0f;
            channelData[s] = finalSample;
            
            if (ch == 0) maxL = juce::jmax(maxL, std::abs(finalSample));
            if (ch == 1) maxR = juce::jmax(maxR, std::abs(finalSample));
        }
    }
    
    if (numChannels >= 2) {
        peakL.store(maxL);
        peakR.store(maxR);
    } else if (numChannels == 1) {
        peakL.store(maxL);
        peakR.store(maxL);
    }
}

void VulcanProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty("currentPreset", currentPreset, nullptr);

    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    if (xml != nullptr)
        copyXmlToBinary (*xml, destData);
}

void VulcanProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr)
    {
        if (xmlState->hasTagName (apvts.state.getType()))
        {
            auto loadedState = juce::ValueTree::fromXml (*xmlState);
            apvts.replaceState (loadedState);
            
            if (loadedState.hasProperty("currentPreset")) {
                currentPreset = loadedState.getProperty("currentPreset");
            }
        }
    }
}

juce::AudioProcessorEditor* VulcanProcessor::createEditor()
{
    return new VulcanEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VulcanProcessor();
}
