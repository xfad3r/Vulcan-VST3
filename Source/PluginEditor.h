#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class VulcanLookAndFeel : public juce::LookAndFeel_V4
{
public:
    VulcanLookAndFeel()
    {
        setColour (juce::ComboBox::backgroundColourId, juce::Colour(0xff1e1e1e));
        setColour (juce::ComboBox::outlineColourId, juce::Colour(0xff333333));
        setColour (juce::ComboBox::textColourId, juce::Colours::white);
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour(0xff1e1e1e));
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override
    {
        auto radius = (float) juce::jmin (width / 2, height / 2) - 4.0f;
        auto centreX = (float) x + (float) width  * 0.5f;
        auto centreY = (float) y + (float) height * 0.5f;
        auto rx = centreX - radius;
        auto ry = centreY - radius;
        auto rw = radius * 2.0f;
        auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        g.setColour (juce::Colour (0xff121212)); 
        g.fillEllipse (rx, ry, rw, rw);

        g.setColour (juce::Colour (0xff2a2a2a));
        g.drawEllipse (rx, ry, rw, rw, 2.0f);

        juce::Path backgroundArc;
        backgroundArc.addCentredArc (centreX, centreY, radius, radius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (juce::Colour (0xff333333));
        g.strokePath (backgroundArc, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        juce::Colour fillColour = slider.getName() == "main_knob" ? juce::Colour(0xff00ffcc) : juce::Colour(0xffff9900);
        
        juce::Path valueArc;
        valueArc.addCentredArc (centreX, centreY, radius, radius, 0.0f, rotaryStartAngle, angle, true);
        g.setColour (fillColour);
        g.strokePath (valueArc, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        juce::Path p;
        auto pointerLength = radius * 0.33f;
        auto pointerThickness = 3.0f;
        p.addRectangle (-pointerThickness * 0.5f, -radius + 6.0f, pointerThickness, pointerLength);
        p.applyTransform (juce::AffineTransform::rotation (angle).translated (centreX, centreY));
        g.setColour (juce::Colours::white);
        g.fillPath (p);
    }
};

class VulcanEditor : public juce::AudioProcessorEditor, private juce::ComboBox::Listener
{
public:
    VulcanEditor (VulcanProcessor&);
    ~VulcanEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged) override;

private:
    VulcanProcessor& audioProcessor;
    VulcanLookAndFeel customLookAndFeel; 

    juce::ComboBox presetMenu;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    juce::Slider inGainSlider, eqLowSlider, monoFreqSlider, eqMidSlider, eqHighSlider, amountSlider, limDriveSlider, outGainSlider;
    std::unique_ptr<SliderAttachment> inGainAtt, eqLowAtt, monoFreqAtt, eqMidAtt, eqHighAtt, amountAtt, limDriveAtt, outGainAtt;

    void setupSlider (juce::Slider& slider, std::unique_ptr<SliderAttachment>& att, const juce::String& paramID, bool isMainKnob);
    
    juce::Font defaultFont;
    juce::Font titleFont;
    juce::Font subtitleFont;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VulcanEditor)
};
