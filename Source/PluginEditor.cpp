#include "PluginProcessor.h"
#include "PluginEditor.h"

VulcanEditor::VulcanEditor (VulcanProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (820, 350);

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
    
    attach = std::make_unique<Attachment>(audioProcessor.getAPVTS(), paramID, slider);
}

void VulcanEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour(0xFF0D0705)); 
    
    g.setColour (juce::Colour(0xFFF59E0B));
    g.setFont (24.0f);
    g.setFont (g.getCurrentFont().withStyle(juce::Font::bold));
    g.drawFittedText ("VULCAN MASTERING CONSOLE", getLocalBounds().withTrimmedTop(20), juce::Justification::centredTop, 1);
    
    g.setColour (juce::Colour(0xFF555555));
    g.setFont (12.0f);
    
    g.drawText("INPUT GAIN", 50,  120, 100, 20, juce::Justification::centred);
    g.drawText("EQ LOW",     150, 120, 100, 20, juce::Justification::centred);
    g.drawText("EQ MID",     250, 120, 100, 20, juce::Justification::centred);
    g.drawText("EQ HIGH",    350, 120, 100, 20, juce::Justification::centred);
    
    g.setColour (juce::Colour(0xFFF59E0B)); 
    g.drawText("CHARACTER",  465, 120, 100, 20, juce::Justification::centred);
    
    g.setColour (juce::Colour(0xFF555555));
    g.drawText("DRIVE",      570, 120, 100, 20, juce::Justification::centred);
    g.drawText("OUT GAIN",   670, 120, 100, 20, juce::Justification::centred);
}

void VulcanEditor::resized()
{
    const int knobSize = 80;
    const int startY = 150;
    
    inputGainSlider.setBounds(60,  startY, knobSize, knobSize);
    eqLowSlider.setBounds    (160, startY, knobSize, knobSize);
    eqMidSlider.setBounds    (260, startY, knobSize, knobSize);
    eqHighSlider.setBounds   (360, startY, knobSize, knobSize);
    
    amountSlider.setBounds   (465, startY - 10, 100, 100);
    
    limitDriveSlider.setBounds(580, startY, knobSize, knobSize);
    outputGainSlider.setBounds(680, startY, knobSize, knobSize);
}
