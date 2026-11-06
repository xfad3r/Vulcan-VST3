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

    // Ορισμός των suffixes (μονάδες μέτρησης) για κάθε knob
    setupSlider(inputGainSlider, inLabel, "IN", "input_gain", " dB");
    setupSlider(eqLowSlider, lowLabel, "LOW", "eq_low", " dB");
    setupSlider(eqMidSlider, midLabel, "MID", "eq_mid", " dB");
    setupSlider(eqHighSlider, highLabel, "HIGH", "eq_high", " dB");
    setupSlider(monoFreqSlider, monoLabel, "MONO HZ", "mono_freq", " Hz");
    setupSlider(haasSlider, haasLabel, "WIDTH", "haas_amount", "");
    setupSlider(amountSlider, amountLabel, "CHARACTER", "amount", "");
    setupSlider(limiterDriveSlider, driveLabel, "DRIVE", "limiter_drive", " dB");
    setupSlider(outputGainSlider, outLabel, "OUT", "output_gain", " dB");
    
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

    setSize (800, 480);
}

VulcanEditor::~VulcanEditor()
{
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
}

void VulcanEditor::setupSlider (juce::Slider& slider, juce::Label& label, const juce::String& text, const juce::String& /*paramID*/, const juce::String& suffix)
{
    addAndMakeVisible(slider);
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    
    // Ενεργοποίηση και παραμετροποίηση του TextBox (της τιμής) κάτω από το knob
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffcccccc)); // Απαλό γκρι κείμενο
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0x00000000)); // Διαφανές περίγραμμα
    slider.setTextValueSuffix(suffix);
    slider.setNumDecimalPlacesToDisplay(1);
    
    addAndMakeVisible(label);
    label.setText(text, juce::dontSendNotification);
    label.setFont(juce::Font(14.0f, juce::Font::bold));
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colour(0xff888888));
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

    // Μεγεθύνθηκαν τα κουτιά των knobs (Y axis) για να χωρέσει το TextBox χωρίς να μικρύνει η γραφική ροδέλα
    int largeSliderW = 130;
    int largeSliderH = 155; 
    int smallSliderW = 80;
    int smallSliderH = 105; 
    int labelHeight = 20;

    amountSlider.setBounds(getWidth() / 2 - 160, 140, largeSliderW, largeSliderH);
    amountLabel.setBounds(amountSlider.getX(), amountSlider.getBottom(), largeSliderW, labelHeight);

    limiterDriveSlider.setBounds(getWidth() / 2 + 30, 140, largeSliderW, largeSliderH);
    driveLabel.setBounds(limiterDriveSlider.getX(), limiterDriveSlider.getBottom(), largeSliderW, labelHeight);

    int bottomRowY = 325;
    
    // Αριστερή Πλευρά: 4 Knobs (EQ + Input)
    inputGainSlider.setBounds(25, bottomRowY, smallSliderW, smallSliderH);
    inLabel.setBounds(inputGainSlider.getX(), inputGainSlider.getBottom(), smallSliderW, labelHeight);

    eqLowSlider.setBounds(110, bottomRowY, smallSliderW, smallSliderH);
    lowLabel.setBounds(eqLowSlider.getX(), eqLowSlider.getBottom(), smallSliderW, labelHeight);

    eqMidSlider.setBounds(195, bottomRowY, smallSliderW, smallSliderH);
    midLabel.setBounds(eqMidSlider.getX(), eqMidSlider.getBottom(), smallSliderW, labelHeight);

    eqHighSlider.setBounds(280, bottomRowY, smallSliderW, smallSliderH);
    highLabel.setBounds(eqHighSlider.getX(), eqHighSlider.getBottom(), smallSliderW, labelHeight);

    // Κέντρο: VU Meter
    vuMeter.setBounds(375, 330, 50, 100);

    // Δεξιά Πλευρά: 3 Knobs (Stereo Width + Output)
    monoFreqSlider.setBounds(470, bottomRowY, smallSliderW, smallSliderH);
    monoLabel.setBounds(monoFreqSlider.getX(), monoFreqSlider.getBottom(), smallSliderW, labelHeight);

    haasSlider.setBounds(570, bottomRowY, smallSliderW, smallSliderH);
    haasLabel.setBounds(haasSlider.getX(), haasSlider.getBottom(), smallSliderW, labelHeight);

    outputGainSlider.setBounds(670, bottomRowY, smallSliderW, smallSliderH);
    outLabel.setBounds(outputGainSlider.getX(), outputGainSlider.getBottom(), smallSliderW, labelHeight);
}
