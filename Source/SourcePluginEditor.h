#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// Custom LookAndFeel για το "Vulcan Magma" Theme
class VulcanLookAndFeel : public juce::LookAndFeel_V4
{
public:
    VulcanLookAndFeel()
    {
        setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFFF59E0B)); // Amber
        setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xFF180E0A)); // Dark Panel
        setColour(juce::Slider::thumbColourId, juce::Colour(0xFFF59E0B));
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                           const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider& slider) override
    {
        auto radius = (float) juce::jmin (width / 2, height / 2) - 4.0f;
        auto centreX = (float) x + (float) width  * 0.5f;
        auto centreY = (float) y + (float) height * 0.5f;
        auto rx = centreX - radius;
        auto ry = centreY - radius;
        auto rw = radius * 2.0f;
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // Σώμα του Knob
        g.setColour (juce::Colour(0xFF111111));
        g.fillEllipse (rx, ry, rw, rw);
        g.setColour (juce::Colour(0xFF333333));
        g.drawEllipse (rx, ry, rw, rw, 1.5f);

        // Τόξο τιμής (Amber Glow)
        juce::Path filledArc;
        filledArc.addPieSegment (rx, ry, rw, rw, rotaryStartAngle, angle, 0.6f);
        g.setColour (findColour (juce::Slider::rotarySliderFillColourId));
        g.fillPath (filledArc);

        // Δείκτης (Pointer)
        juce::Path p;
        auto pointerLength = radius * 0.33f;
        auto pointerThickness = 3.0f;
        p.addRectangle (-pointerThickness * 0.5f, -radius, pointerThickness, pointerLength);
        p.applyTransform (juce::AffineTransform::rotation (angle).translated (centreX, centreY));
        g.setColour (juce::Colours::white);
        g.fillPath (p);
    }
};

class VulcanEditor : public juce::AudioProcessorEditor
{
public:
    VulcanEditor (VulcanProcessor&);
    ~VulcanEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    VulcanProcessor& audioProcessor;
    VulcanLookAndFeel customLookAndFeel;

    juce::Slider inputGainSlider, eqLowSlider, eqMidSlider, eqHighSlider;
    juce::Slider amountSlider, limitDriveSlider, outputGainSlider;

    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Attachment> inputGainAttach, eqLowAttach, eqMidAttach, eqHighAttach;
    std::unique_ptr<Attachment> amountAttach, limitDriveAttach, outputGainAttach;

    void setupRotarySlider(juce::Slider& slider, std::unique_ptr<Attachment>& attach, const juce::String& paramID);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VulcanEditor)
};