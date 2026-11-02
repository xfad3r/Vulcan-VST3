#include "PluginProcessor.h"
#include "PluginEditor.h"

VulcanEditor::VulcanEditor (VulcanProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // 1. Μεγαλύτερο, πιο άνετο μέγεθος παραθύρου
    setSize (720, 450);

    // 2. Οργανικές/Serif γραμματοσειρές αντί για default sans-serif
    customFont = juce::Font (juce::Font::getDefaultSerifFontName(), 17.0f, juce::Font::italic);
    titleFont = juce::Font (juce::Font::getDefaultSerifFontName(), 46.0f, juce::Font::bold | juce::Font::italic);

    // Custom χρώματα για τα Rotary Sliders (Ταιριάζουν με το Navy Blue)
    getLookAndFeel().setColour (juce::Slider::thumbColourId, juce::Colour(0xff8c9eff));
    getLookAndFeel().setColour (juce::Slider::rotarySliderFillColourId, juce::Colour(0xff4a5c88));
    getLookAndFeel().setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff12182b));

    addAndMakeVisible(presetMenu);
    for (int i = 0; i < audioProcessor.getNumPrograms(); ++i)
    {
        presetMenu.addItem(audioProcessor.getProgramName(i), i + 1);
    }
    presetMenu.setSelectedId(audioProcessor.getCurrentProgram() + 1, juce::dontSendNotification);
    presetMenu.addListener(this);
    presetMenu.setLookAndFeel(&getLookAndFeel());

    setupSlider (inGainSlider, inGainAtt, "input_gain");
    setupSlider (eqLowSlider, eqLowAtt, "eq_low");
    setupSlider (eqMidSlider, eqMidAtt, "eq_mid");
    setupSlider (eqHighSlider, eqHighAtt, "eq_high");
    setupSlider (amountSlider, amountAtt, "amount");
    setupSlider (limDriveSlider, limDriveAtt, "limiter_drive");
    setupSlider (outGainSlider, outGainAtt, "output_gain");
}

VulcanEditor::~VulcanEditor() {}

void VulcanEditor::setupSlider (juce::Slider& slider, std::unique_ptr<SliderAttachment>& att, const juce::String& paramID)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 65, 22);
    
    // Αλλαγή γραμματοσειράς στο textbox του slider
    slider.setLookAndFeel(&getLookAndFeel());
    
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
    // 3. Βαθύ Μπλε Background Gradient (Midnight Blue to Navy)
    juce::Colour colour1 = juce::Colour(0xff091121); // Σκούρο πάνω
    juce::Colour colour2 = juce::Colour(0xff1a2c4c); // Ελαφρώς πιο ανοιχτό κάτω

    juce::ColourGradient bgGradient (colour1, 0, 0, colour2, 0, (float)getHeight(), false);
    g.setGradientFill (bgGradient);
    g.fillAll();

    // Τίτλος
    g.setColour (juce::Colours::white.withAlpha(0.9f));
    g.setFont (titleFont);
    g.drawFittedText ("VULCAN", getLocalBounds().removeFromTop(90), juce::Justification::centred, 1);
    
    // Υπότιτλος 
    g.setFont (juce::Font(juce::Font::getDefaultSerifFontName(), 14.0f, juce::Font::plain));
    g.setColour (juce::Colours::lightgrey.withAlpha(0.6f));
    g.drawFittedText ("STEALTH MASTERING EDITION", getLocalBounds().removeFromTop(140), juce::Justification::centredBottom, 1);

    // Labels των Knobs
    g.setFont (customFont);
    g.setColour (juce::Colour(0xffaab6cb)); // Απαλό γαλάζιο/γκρι για τα γράμματα
    
    int w = getWidth() / 7;
    int y = 330; // Ύψος των labels
    
    g.drawFittedText ("In Gain",  0 * w, y, w, 30, juce::Justification::centred, 1);
    g.drawFittedText ("Low EQ",   1 * w, y, w, 30, juce::Justification::centred, 1);
    g.drawFittedText ("Mid EQ",   2 * w, y, w, 30, juce::Justification::centred, 1);
    g.drawFittedText ("High EQ",  3 * w, y, w, 30, juce::Justification::centred, 1);
    g.drawFittedText ("Character",4 * w, y, w, 30, juce::Justification::centred, 1);
    g.drawFittedText ("Drive",    5 * w, y, w, 30, juce::Justification::centred, 1);
    g.drawFittedText ("Out Gain", 6 * w, y, w, 30, juce::Justification::centred, 1);
}

void VulcanEditor::resized()
{
    presetMenu.setBounds (getWidth() / 2 - 140, 105, 280, 28);

    int w = getWidth() / 7;
    int size = juce::jmin(105, w - 10);
    int startY = 210; // Ύψος των knobs
    int yOffset = (w - size) / 2;

    inGainSlider.setBounds   (0 * w + yOffset, startY, size, size);
    eqLowSlider.setBounds    (1 * w + yOffset, startY, size, size);
    eqMidSlider.setBounds    (2 * w + yOffset, startY, size, size);
    eqHighSlider.setBounds   (3 * w + yOffset, startY, size, size);
    amountSlider.setBounds   (4 * w + yOffset, startY, size, size);
    limDriveSlider.setBounds (5 * w + yOffset, startY, size, size);
    outGainSlider.setBounds  (6 * w + yOffset, startY, size, size);
}
