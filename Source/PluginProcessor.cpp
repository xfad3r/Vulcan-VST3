#include "PluginProcessor.h"

VulcanAudioProcessor::VulcanAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
        .withInput("Sidechain", juce::AudioChannelSet::stereo(), false)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    inputGainParam  = apvts.getRawParameterValue("input_gain");
    eqLowParam      = apvts.getRawParameterValue("eq_low");
    eqMidParam      = apvts.getRawParameterValue("eq_mid");
    eqHighParam     = apvts.getRawParameterValue("eq_high");
    amountParam     = apvts.getRawParameterValue("amount");
    extScParam      = apvts.getRawParameterValue("ext_sidechain");
    limitInParam    = apvts.getRawParameterValue("limiter_drive");
    outputGainParam = apvts.getRawParameterValue("output_gain");
}

VulcanAudioProcessor::~VulcanAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout VulcanAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"input_gain", 1}, "Input Gain", 
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"eq_low", 1}, "Pre EQ Low (100Hz)", 
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"eq_mid", 1}, "Pre EQ Mid (2.5kHz)", 
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));

    params.push_back(juce::ParameterID{"eq_high", 1}, "Pre EQ High (10kHz)", 
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));

    // Default calibrated to 0.00 (Transparent / Clean)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"amount", 1}, "Character Amount", 
        juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"ext_sidechain", 1}, "External Sidechain", false));

    // Default calibrated to 0.0 dB
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"limiter_drive", 1}, "Limiter Drive", 
        juce::NormalisableRange<float>(0.0f, 18.0f, 0.1f), 0.0f));

    params.push_back(std::make_unique<jujuce::ParameterID{"output_gain", 1}, "Output Gain", 
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f));

    return { params.begin(), params.end() };
}

bool VulcanAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void VulcanAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getMainBusNumInputChannels());

    // Stage 1: Input & EQ
    inputGainModule.prepare(spec);
    preEqChain.prepare(spec);
    updatePreEq();

    // Stage 2: Crossovers (24dB/oct Linkwitz-Riley)
    lowMidCrossoverLP.prepare(spec);
    lowMidCrossoverHP.prepare(spec);
    midHighCrossoverLP.prepare(spec);
    midHighCrossoverHP.prepare(spec);

    lowMidCrossoverLP.setCutoffFrequency(250.0f);
    lowMidCrossoverLP.setType(juce::dsp::LinklowMidCrossoverHP.setCutoffFrequency(250.0f);
    lowMidCrossoverHP.setType(juce::dsp::LinkwitzRileyFilterType::highpass);

    midHighCrossoverLP.setCutoffFrequency(4000.0f);
    midHighCrossoverLP.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
    midHighCrossoverHP.setCutoffFrequency(4000.0f);
    midHighCrossoverHP.setType(juce::dsp::LinkwitzRileyFilterType::highpass);

    for (int i = 0; i < numBands; ++i)
    {
        bandCompressors[i].prepare(spec);
        bandCompressors[i].setAttack(20.0f);   // 20ms attack
        bandCompressors[i].setRelease(120.0f); // 120ms release
        bandBuffers[i].setSize(getMainBusNumInputChannels(), samplesPerBlock);
    }

    sidechainBuffer.setSize(getMainBusNumInputChannels(), samplesPerBlock);

    // Stage 3: Brickwall Limiter (0.0 dBFS Ceiling)
    limiter.prepare(spec);
    limiter.setThreshold(0.0f);
    limiter.setRelease(50.0f);

    // Stage 4: Output Gain
    outputGainModule.prepare(spec);
}

void VulcanAudioProcessor::releaseResources() {}

void VulcanAudioProcessor::updatePreEq()
{
    auto lowCoeff = juce::dsp::IIR::Coefficients<float>::makeLowShelf(
        currentSampleRate, 100.0f, 0.707f, juce::Decibels::decibelsToGain(eqLowParam->load()));

    auto midCoeff = juce::dsp::IcurrentSampleRate, 2500.0f, 1.0f, juce::Decibels::decibelsToGain(eqMidParam->load()));

    auto highCoeff = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
        currentSampleRate, 10000.0f, 0.707f, juce::Decibels::decibelsToGain(eqHighParam->load()));

    *preEqChain.get<0>().coefficients = *lowCoeff;
    *preEqChain.get<1>().coefficients = *midCoeff;
    *preEqChain.get<2>().coefficients = *highCoeff;
}

// Normalized Tanh Curve to avoid volume jumps and maintain unity peak output
void VulcanAudioProcessor::applySaturation(juce::AudioBuffer<float>& buffer, float driveGainFactor)
{
    const float drive = juce::jmax(0.1f, driveGainFactor);
    const float norm = std::tanh(drive);
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    for (int channel = 0; channel < numChannels; ++channel)
    {
        float* samples = buffer.getWritePointer(channel);
        for (int i = 0; i < numSamples; ++i)
        {
            float x = samples[i];
            samples[i] = std::tanh(x * drive) / norm;
        }
    }
}

void VulcanAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int totalNumInputChannels = getMainBusNumInputChannels();
    const int numSamples = buffer.getNumSamples();

    // -------------------------------------------------------------------
    // STAGE 1: INPUT STAGING & PRE-EQ
    // -------------------------------------------------------------------
    inputGainModule.setGainDecibels(inputGainParam->load());
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    inputGainModule.process(context);
    updatePreEq();
    preEqChain.process(context);

    // Peak & RMS Metering Calculation (Input)
    inputPeak.store(buffer.getMagnitude(0, numSamples));
    inputRMS.store(buffer.getRMSLevel(0, 0, numSamples));

    // Handle External Sidechain Input
    const bool useExtSidechain = extScParam->load() > 0.5f;
    auto sidechainBus = getBusBuffer(buffer, true, 1);
    
    if (useExtSidechain && sidechainBus.getNumChannels() > 0)
    {
        sidechainBuffer.copyFrom(0, 0, sidechainBus, 0, 0, numSamples);
        if (sidechainBus.getNumChannels() > 1 && sidechainBuffer.getNumChannels() > 1)
            sidechainBuffer.copyFrom(1, 0, sidechainBus, 1, 0, numSamples);
    }
    else
    {
        for (int ch = 0; ch < totalNumInputChannels; ++ch)
            sidechainBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
    }

    // -------------------------------------------------------------------
    // STAGE 2: "CHARACTER" MULTIBAND PROCESSOR (Amount Scaling)
    // -------------------------------------------------------------------
    float amount = amountParam->load();

    // Amount Macro:
    // Amount = 0.0 -> Drive = 1.0, Thresh = 0 dB, Ratio = 1.0:1 (Linear/Transparent)
    // Amount = 1.0 -> Drive = 2.5, Thresh = -16 dB, Ratio = 2.75:1
    // Amount = 2.0 -> Drive = 4.0, Thresh = -26 dB, Ratio = 4.5:1
    float driveFactor = 1.0f + (amount * 1.5f);
    float thresholdDb = (amount <= 0.001f) ? 0.0f : (-6.0f - (amount * 10.0f));
    float ratio = 1.0f + (amount * 1.75f);

    for (int i = 0; i < numBands; ++i)
    {
        bandCompressors[i].setThreshold(thresholdDb);
        bandCompressors[i].setRatio(ratio);
        bandBuffers[i].setSize(totalNumInputChannels, numSamples, false, false, true);
        bandBuffers[i].clear();
    }

    // 3-Band Linkwitz-Riley Crossover Split
    for (int ch = 0; ch < totalNumInputChannels; ++ch)
    {
        const float* src = buffer.getReadPointer(ch);
        float* lowPtr  = bandBuffers[0].getWritePointer(ch);
        float* midPtr  = bandBuffers[1].getWritePointer(ch);
        float* highPtr = bandBuffers[2].getWritePointer(ch);

        for (int s = 0; s < numSamples; ++s)
        {
            float inputSample = src[s];

            // 250Hz Crossover
            float lowPart  = lowMidCrossoverLP.processSample(ch, inputSample);
            float highPart = lowMidCrossoverHP.processSample(ch, inputSample);

            // 4kHz Crossover
            float midPart  = midHighCrossoverLP.processSample(ch, highPart);
            float highFinal= midHighCrossoverHP.processSample(ch, highPart);

            lowPtr[s]   = lowPart;
            midPtr[s]   = midPart;
            highPtr[s]  = highFinal;
        }
    }

    // Compress & Saturate per Band, then Sum Back
    buffer.clear();

    for (int i = 0; i < numBands; ++i)
    {
        juce::dsp::AudioBlock<float> bandBlock(bandBuffers[i]);
        juce::dsp::ProcessContextReplacing<float> bandContext(bandBlock);

        bandCompressors[i].process(bandContext);
        applySaturation(bandBuffers[i], driveFactor);

        for (int ch = 0; ch < totalNumInputChannels; ++ch)
        {
            buffer.addFrom(ch, 0, bandBuffers[i], ch, 0, numSamples);
        }
    }

    // -------------------------------------------------------------------
    // STAGE 3: ADAPTIVE LOOKAHEAD LIMITER
    // -------------------------------------------------------------------
    float limiterDrive = juce::Decibels::decibelsToGain(limitInParam->load());
    buffer.applyGain(limiterDrive);

    limiter.process(context);

    // -------------------------------------------------------------------
    // STAGE 4: OUTPUT STAGING & METERING
    // -------------------------------------------------------------------
    outputGainModule.setGainDecibels(outputGainParam->load());
    outputGainModule.process(context);

    // Peak & RMS Metering Calculation (Output)
    outputPeak.store(buffer.getMagnitude(0, numSamples));
    outputRMS.store(buffer.getRMSLevel(0, 0, numSamples));
}

void VulcanAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void VulcanAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<xmlState(getXmlFromBinary(data, sizeInBytes));State.get() != nullptr && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VulcanAudioProcessor();
}
