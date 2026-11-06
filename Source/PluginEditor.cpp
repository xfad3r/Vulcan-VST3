#include "PluginProcessor.h"
#include "PluginEditor.h"

VulcanEditor::VulcanEditor (VulcanProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), vuMeter(p)
{
    juce::LookAndFeel::setDefaultLookAndFeel(&customLookAndFeel);

    titleLabel.setText("VULCAN", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(48.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(titleLabel);

    subTitleLabel.setText("by DJ Smartie", juce::dontSendNotification);
    subTitleLabel.setFont(juce::Font(18.0f, juce::Font::plain));
    subTitleLabel.setJustificationType(juce::Justification::centred);
    subTitleLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible(subTitleLabel);

    addAndMakeVisible(presetBox);
    for (int i = 0; i < audioProcessor.getNumPrograms(); ++i)
        presetBox.addItem(audioProcessor.getProgramName(i), i + 1);
    
    presetBox.setSelectedId(audioProcessor.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this]() {
        audioProcessor.setCurrentProgram(presetBox.getSelectedId() - 1);
    };

    addAndMakeVisible(vuMeter);

    setupSlider(inputGainSlider, inLabel, "IN", "input_gain");
    setupSlider(eqLowSlider, lowLabel, "LOW", "eq_low");
    setupSlider(eqMidSlider, midLabel, "MID", "eq_mid");
    setupSlider(eqHighSlider, highLabel, "HIGH", "eq_high");
    setupSlider(monoFreqSlider, monoLabel, "MONO HZ", "mono_freq");
    setupSlider(haasSlider, haasLabel, "WIDTH", "haas_amount");
    setupSlider(amountSlider, amountLabel, "CHARACTER", "amount");
    setupSlider(limiterDriveSlider, driveLabel, "DRIVE", "limiter_drive");
    setupSlider(outputGainSlider, outLabel, "OUT", "output_gain");
    
    amountSlider.setName("Character");
    limiterDriveSlider.setName("Drive");
    haasSlider.setName("Width");

    inAttach = std::make_unique<Attachment>(audioProcessor.getAPVTS(), "input_gain", inputGainSlider);
    lowAttach = std::make_unique<Attachment>(audioProcessor.getAPVTS(), "eq_low", eqLowSlider);
    midAttach = std::make_unique<Attachment>(audioProcessor.getAPVTS(), "eq_mid", eqMidSlider);
    highAttach = std::make_unique<Attachment>(audioProcessor.getAPVTS(), "eq_high", eqHighSlider);
    monoAttach = std::make_unique<Attachment>(audioProcessor.getAPVTS(), "mono_freq", monoFreqSlider);
    haasAttach = std::make_unique<Attachment>(audioProcessor.getAPVTS(), "haas_amount", haasSlider);
    amountAttach = std::make_unique<Attachment>(audioProcessor.getAPVTS(), "amount", amountSlider);
    driveAttach = std::make_unique<Attachment>(audioProcessor.getAPVTS(), "limiter_drive", limiterDriveSlider);
    outAttach = std::make_unique<Attachment>(audioProcessor.getAPVTS(), "output_gain", outputGainSlider);

    // Έκανα το παράθυρο ελαφρώς πιο πλατύ για να χωρέσει το 8ο μικρό knob άνετα (800x480)
    setSize (800, 480);
}

VulcanEditor::~VulcanEditor()
{
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
}

void VulcanEditor::setupSlider (juce::Slider& slider, juce::Label& label, const juce::String& text, const juce::String& /*paramID*/)
{
    addAndMakeVisible(slider);
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    
    addAndMakeVisible(label);
    label.setText(text, juce::dontSendNotification);
    label.setFont(juce::Font(14.0f, juce::Font::bold));
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colour(0xffaaaaaa));
}

void VulcanEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour(0xff181a1f)); 
    
    g.setColour(juce::Colour(0xff22252a));
    g.fillRect(0, 110, getWidth(), 2);
}

void VulcanEditor::resized()
{
    auto area = getLocalBounds();
    
    titleLabel.setBounds(0, 15, getWidth(), 50);
    subTitleLabel.setBounds(0, 55, getWidth(), 20);
    
    presetBox.setBounds(getWidth() / 2 - 125, 90, 250, 30);

    int largeKnobSize = 130;
    int smallKnobSize = 80;
    int labelHeight = 25;

    amountSlider.setBounds(getWidth() / 2 - 160, 150, largeKnobSize, largeKnobSize);
    amountLabel.setBounds(amountSlider.getX(), amountSlider.getBottom(), largeKnobSize, labelHeight);

    limiterDriveSlider.setBounds(getWidth() / 2 + 30, 150, largeKnobSize, largeKnobSize);
    driveLabel.setBounds(limiterDriveSlider.getX(), limiterDriveSlider.getBottom(), largeKnobSize, labelHeight);

    int bottomRowY = 340;
    
    // Αριστερή Πλευρά: 4 Knobs (EQ + Input)
    inputGainSlider.setBounds(25, bottomRowY, smallKnobSize, smallKnobSize);
    inLabel.setBounds(inputGainSlider.getX(), inputGainSlider.getBottom(), smallKnobSize, labelHeight);

    eqLowSlider.setBounds(110, bottomRowY, smallKnobSize, smallKnobSize);
    lowLabel.setBounds(eqLowSlider.getX(), eqLowSlider.getBottom(), smallKnobSize, labelHeight);

    eqMidSlider.setBounds(195, bottomRowY, smallKnobSize, smallKnobSize);
    midLabel.setBounds(eqMidSlider.getX(), eqMidSlider.getBottom(), smallKnobSize, labelHeight);

    eqHighSlider.setBounds(280, bottomRowY, smallKnobSize, smallKnobSize);
    highLabel.setBounds(eqHighSlider.getX(), eqHighSlider.getBottom(), smallKnobSize, labelHeight);

    // Κέντρο: VU Meter
    vuMeter.setBounds(375, 335, 50, 95);

    // Δεξιά Πλευρά: 3 Knobs (Stereo Width + Output)
    monoFreqSlider.setBounds(470, bottomRowY, smallKnobSize, smallKnobSize);
    monoLabel.setBounds(monoFreqSlider.getX(), monoFreqSlider.getBottom(), smallKnobSize, labelHeight);

    haasSlider.setBounds(570, bottomRowY, smallKnobSize, smallKnobSize);
    haasLabel.setBounds(haasSlider.getX(), haasSlider.getBottom(), smallKnobSize, labelHeight);

    outputGainSlider.setBounds(670, bottomRowY, smallKnobSize, smallKnobSize);
    outLabel.setBounds(outputGainSlider.getX(), outputGainSlider.getBottom(), smallKnobSize, labelHeight);
}
