#include "PluginProcessor.h"
#include "PluginEditor.h"

VulcanEditor::VulcanEditor (VulcanProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (850, 500);
    setLookAndFeel(&customLookAndFeel);

    defaultFont = juce::Font (juce::FontOptions(juce::Font::getDefaultSansSerifFontName(), 14.0f, juce::Font::bold));
    titleFont = juce::Font (juce::FontOptions(juce::Font::getDefaultSansSerifFontName(), 52.0f, juce::Font::bold));
    subtitleFont = juce::Font (juce::FontOptions(juce::Font::getDefaultSansSerifFontName(), 16.0f, juce::Font::plain));

    addAndMakeVisible(presetMenu);
    for (int i = 0; i < audioProcessor.getNumPrograms(); ++i)
    {
        presetMenu.addItem(audioProcessor.getProgramName(i), i + 1);
    }
    presetMenu.setSelectedId(audioProcessor.getCurrentProgram() + 1, juce::dontSendNotification);
    presetMenu.addListener(this);

    // 3 αριστερά μικρά knobs (IN, LOW, MONO HZ)
    setupSlider (inGainSlider, inGainAtt, "input_gain", false);
    setupSlider (eqLowSlider, eqLowAtt, "eq_low", false);
    setupSlider (monoFreqSlider, monoFreqAtt, "mono_freq", false); // Το νέο ρυθμιστικό
    
    // 2 κεντρικά μεγάλα knobs
    setupSlider (amountSlider, amountAtt, "amount", true); 
    setupSlider (limDriveSlider, limDriveAtt, "limiter_drive", true); 
    
    // 3 δεξιά μικρά knobs (MID, HIGH, OUT)
    setupSlider (eqMidSlider, eqMidAtt, "eq_mid", false);
    setupSlider (eqHighSlider, eqHighAtt, "eq_high", false);
    setupSlider (outGainSlider, outGainAtt, "output_gain", false);
}

VulcanEditor::~VulcanEditor() 
{
    setLookAndFeel(nullptr); 
}

void VulcanEditor::setupSlider (juce::Slider& slider, std::unique_ptr<SliderAttachment>& att, const juce::String& paramID, bool isMainKnob)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0); 
    
    if (isMainKnob) slider.setName("main_knob"); 
    else slider.setName("small_knob");

    addAndMakeVisible(slider);
    att = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), paramID, slider);
}

void VulcanEditor::comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &presetMenu)
    {
        audioProcessor.setCurrentProgram(presetMenu.getSelectedId() - 1);
    }
}

void VulcanEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour(0xff181818)); 

    g.setColour (juce::Colours::white);
    g.setFont (titleFont);
    g.drawFittedText ("VULCAN", 0, 30, getWidth(), 60, juce::Justification::centred, 1);
    
    g.setFont (subtitleFont);
    g.setColour (juce::Colour(0xff888888)); 
    g.drawFittedText ("by DJ Smartie", 0, 85, getWidth(), 20, juce::Justification::centred, 1);

    g.setFont (defaultFont);
    g.setColour (juce::Colour(0xffaaaaaa)); 
    
    int smallW = 100;
    int yBottom = 430; 
    int yTop = 270;    

    // Αριστερά Labels
    g.drawFittedText ("IN", 40, yBottom, smallW, 20, juce::Justification::centred, 1);
    g.drawFittedText ("LOW", 150, yBottom, smallW, 20, juce::Justification::centred, 1);
    g.drawFittedText ("MONO HZ", 260, yBottom, smallW, 20, juce::Justification::centred, 1); // Label ρυθμιστικού
    
    // Κεντρικά Μεγάλα Labels
    g.drawFittedText ("CHARACTER", getWidth()/2 - 140, yTop, 120, 20, juce::Justification::centred, 1);
    g.drawFittedText ("DRIVE", getWidth()/2 + 20, yTop, 120, 20, juce::Justification::centred, 1);

    // Δεξιά Labels
    g.drawFittedText ("MID", 500, yBottom, smallW, 20, juce::Justification::centred, 1);
    g.drawFittedText ("HIGH", 610, yBottom, smallW, 20, juce::Justification::centred, 1);
    g.drawFittedText ("OUT", 720, yBottom, smallW, 20, juce::Justification::centred, 1);
    
    g.setColour(juce::Colour(0xff2a2a2a));
    g.drawLine(0.0f, 170.0f, (float)getWidth(), 170.0f, 2.0f);
}

void VulcanEditor::resized()
{
    presetMenu.setBounds (getWidth() / 2 - 125, 120, 250, 26);

    int smallSize = 75; 
    int largeSize = 130; 
    
    int yBottomKnobs = 350;
    int yTopKnobs = 190;

    // Τοποθέτηση μικρών Knobs (Αριστερά)
    inGainSlider.setBounds   (40, yBottomKnobs, smallSize, smallSize);
    eqLowSlider.setBounds    (150, yBottomKnobs, smallSize, smallSize);
    monoFreqSlider.setBounds (260, yBottomKnobs, smallSize, smallSize);

    // Τοποθέτηση μικρών Knobs (Δεξιά)
    eqMidSlider.setBounds    (500, yBottomKnobs, smallSize, smallSize);
    eqHighSlider.setBounds   (610, yBottomKnobs, smallSize, smallSize);
    outGainSlider.setBounds  (720, yBottomKnobs, smallSize, smallSize);

    // Τοποθέτηση Μεγάλων Knobs (Κέντρο)
    amountSlider.setBounds   (getWidth()/2 - 145, yTopKnobs - 50, largeSize, largeSize);
    limDriveSlider.setBounds (getWidth()/2 + 15, yTopKnobs - 50, largeSize, largeSize);
}
