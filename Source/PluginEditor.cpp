#include "PluginProcessor.h"
#include "PluginEditor.h"

VulcanEditor::VulcanEditor (VulcanProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Ρύθμιση παραθύρου: 700x350
    setSize (700, 350);

    setupRotarySlider(inputGainSlider, inputGainAttach, "input_gain");
    setupRotarySlider(eqLowSlider, eqLowAttach, "eq_low");
    setupRotarySlider(eqMidSlider, eqMidAttach, "eq_mid");
    setupRotarySlider(eqHighSlider, eqHighAttach, "eq_high");
    setupRotarySlider(amountSlider, amountAttach, "amount");
    setupRotarySlider(limitDriveSlider, limitDriveAttach, "limiter_drive");
    setupRotarySlider(outputGainSlider, outputGainAttach, "output_gain");
}

VulcanEditor::~VulcanEditor()
{
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
}

void VulcanEditor::setupRotarySlider(juce::Slider& slider, std::unique_ptr<Attachment>& attach, const juce::String& paramID)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    slider.setLookAndFeel(&customLookAndFeel);
    slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xFFAAAAAA));
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(slider);
    
    // Binding: Συνδέει γραφικά το UI με τις τιμές APVTS του VULCAN Processor
    attach = std::make_unique<Attachment>(audioProcessor.getAPVTS(), paramID, slider);
}

void VulcanEditor::paint (juce::Graphics& g)
{
    // Φόντο "Vulcan Magma" (Black/Deep Amber tint)
    g.fillAll (juce::Colour(0xFF0D0705)); 
    
    g.setColour (juce::Colour(0xFFF59E0B));
    g.setFont (24.0f);
    g.setFont (g.getFont().withStyle(juce::Font::bold));
    g.drawFittedText ("VULCAN MASTERING CONSOLE", getLocalBounds().withTrimmedTop(20), juce::Justification::centredTop, 1);
    
    g.setColour (juce::Colour(0xFF555555));
    g.setFont (12.0f);
    
    // Ετικέτες περιοχών
    g.drawText("INPUT GAIN", 45, 120, 100, 20, juce::Justification::centred);
    g.drawText("EQ LOW", 145, 120, 100, 20, juce::Justification::centred);
    g.drawText("EQ MID", 245, 120, 100, 20, juce::Justification::centred);
    g.drawText("EQ HIGH", 345, 120, 100, 20, juce::Justification::centred);
    
    g.setColour (juce::Colour(0xFFF59E0B)); // Amber Highlight
    g.drawText("CHARACTER", 445, 120, 100, 20, juce::Justification::centred);
    
    g.setColour (juce::Colour(0xFF555555));
    g.drawText("DRIVE", 545, 120, 100, 20, juce::Justification::centred);
    g.drawText("OUT GAIN", 645, 120, 100, 20, juce::Justification::centred);
}

void VulcanEditor::resized()
{
    // Τοποθέτηση των Knobs στον καμβά
    const int knobSize = 80;
    const int startY = 150;
    const int spacing = 100;
    
    inputGainSlider.setBounds(50, startY, knobSize, knobSize);
    eqLowSlider.setBounds(150, startY, knobSize, knobSize);
    eqMidSlider.setBounds(250, startY, knobSize, knobSize);
    eqHighSlider.setBounds(350, startY, knobSize, knobSize);
    
    // Το κεντρικό Amount Knob γίνεται ελαφρώς μεγαλύτερο (100px)
    amountSlider.setBounds(445, startY - 10, 100, 100);
    
    limitDriveSlider.setBounds(550, startY, knobSize, knobSize);
    outputGainSlider.setBounds(650, startY, knobSize, knobSize);
}
