#include "PluginProcessor.h"
#include "PluginEditor.h"

VulcanEditor::VulcanEditor (VulcanProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Ακόμα μεγαλύτερο, άνετο μέγεθος παραθύρου
    setSize (850, 500);
    
    // Εφαρμόζουμε το δικό μας L&F σε όλο το παράθυρο
    setLookAndFeel(&customLookAndFeel);

    // Σύγχρονες, flat, sans-serif γραμματοσειρές (Βιομηχανικό Look)
    defaultFont = juce::Font (juce::Font::getDefaultSansSerifFontName(), 14.0f, juce::Font::bold);
    titleFont = juce::Font (juce::Font::getDefaultSansSerifFontName(), 52.0f, juce::Font::bold);
    subtitleFont = juce::Font (juce::Font::getDefaultSansSerifFontName(), 16.0f, juce::Font::plain);

    addAndMakeVisible(presetMenu);
    for (int i = 0; i < audioProcessor.getNumPrograms(); ++i)
    {
        presetMenu.addItem(audioProcessor.getProgramName(i), i + 1);
    }
    presetMenu.setSelectedId(audioProcessor.getCurrentProgram() + 1, juce::dontSendNotification);
    presetMenu.addListener(this);

    // Ορίζουμε ποια knobs είναι τα "Μεγάλα" (Character / Drive)
    setupSlider (inGainSlider, inGainAtt, "input_gain", false);
    setupSlider (eqLowSlider, eqLowAtt, "eq_low", false);
    setupSlider (eqMidSlider, eqMidAtt, "eq_mid", false);
    setupSlider (eqHighSlider, eqHighAtt, "eq_high", false);
    
    setupSlider (amountSlider, amountAtt, "amount", true); // Main Knob (Cyan)
    setupSlider (limDriveSlider, limDriveAtt, "limiter_drive", true); // Main Knob (Cyan)
    
    setupSlider (outGainSlider, outGainAtt, "output_gain", false);
}

VulcanEditor::~VulcanEditor() 
{
    setLookAndFeel(nullptr); // Καθαρισμός μνήμης L&F
}

void VulcanEditor::setupSlider (juce::Slider& slider, std::unique_ptr<SliderAttachment>& att, const juce::String& paramID, bool isMainKnob)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    
    // Κρύβουμε τα textboxes των sliders (θα ζωγραφίσουμε μόνοι μας τα νούμερα ή δεν θα τα δείχνουμε, για πιο καθαρό look)
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0); 
    
    // Ονομάζουμε τα sliders για να ξέρει η drawRotarySlider τι χρώμα να βάλει
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
    // Βαθύ γκρι/μαύρο (Dark Theme - όπως το LoudBody)
    g.fillAll (juce::Colour(0xff181818)); 

    // Τίτλος (Λευκό, Sans-Serif, Bold)
    g.setColour (juce::Colours::white);
    g.setFont (titleFont);
    g.drawFittedText ("VULCAN", 0, 30, getWidth(), 60, juce::Justification::centred, 1);
    
    // Υπότιτλος "by DJ Smartie"
    g.setFont (subtitleFont);
    g.setColour (juce::Colour(0xff888888)); // Γκρι
    g.drawFittedText ("by DJ Smartie", 0, 85, getWidth(), 20, juce::Justification::centred, 1);

    // Labels των Knobs
    g.setFont (defaultFont);
    g.setColour (juce::Colour(0xffaaaaaa)); // Γκρι ανοιχτό
    
    // Συντεταγμένες Labels
    int smallW = 100;
    int yBottom = 430; // Ύψος για τα κάτω labels
    int yTop = 270;    // Ύψος για τα κεντρικά μεγάλα labels

    g.drawFittedText ("IN", 50, yBottom, smallW, 20, juce::Justification::centred, 1);
    g.drawFittedText ("LOW", 160, yBottom, smallW, 20, juce::Justification::centred, 1);
    g.drawFittedText ("MID", 270, yBottom, smallW, 20, juce::Justification::centred, 1);
    
    // Τα κεντρικά μεγάλα (Character & Drive)
    g.drawFittedText ("CHARACTER", getWidth()/2 - 140, yTop, 120, 20, juce::Justification::centred, 1);
    g.drawFittedText ("DRIVE", getWidth()/2 + 20, yTop, 120, 20, juce::Justification::centred, 1);

    g.drawFittedText ("HIGH", 480, yBottom, smallW, 20, juce::Justification::centred, 1);
    g.drawFittedText ("OUT", 590, yBottom, smallW, 20, juce::Justification::centred, 1);
    
    // Σχεδίαση ενός διαχωριστικού (γραμμής) κάτω από το menu
    g.setColour(juce::Colour(0xff2a2a2a));
    g.drawLine(0.0f, 170.0f, (float)getWidth(), 170.0f, 2.0f);
}

void VulcanEditor::resized()
{
    // Το Menu κάτω από τον υπότιτλο, κεντραρισμένο
    presetMenu.setBounds (getWidth() / 2 - 125, 120, 250, 26);

    int smallSize = 75; // Μέγεθος μικρών knobs
    int largeSize = 130; // Μέγεθος μεγάλων knobs
    
    int yBottomKnobs = 350;
    int yTopKnobs = 190;

    // Τοποθέτηση μικρών Knobs (Κάτω σειρά)
    inGainSlider.setBounds   (50, yBottomKnobs, smallSize, smallSize);
    eqLowSlider.setBounds    (160, yBottomKnobs, smallSize, smallSize);
    eqMidSlider.setBounds    (270, yBottomKnobs, smallSize, smallSize);
    
    eqHighSlider.setBounds   (480, yBottomKnobs, smallSize, smallSize);
    outGainSlider.setBounds  (590, yBottomKnobs, smallSize, smallSize);

    // Τοποθέτηση Μεγάλων Knobs (Κέντρο)
    amountSlider.setBounds   (getWidth()/2 - 145, yTopKnobs - 50, largeSize, largeSize);
    limDriveSlider.setBounds (getWidth()/2 + 15, yTopKnobs - 50, largeSize, largeSize);
}
