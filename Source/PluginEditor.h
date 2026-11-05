#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// --- CUSTOM LOOK AND FEEL ---
class VulcanLookAndFeel : public juce::LookAndFeel_V4
{
public:
    VulcanLookAndFeel()
    {
        setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff00d084));
        setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff151515));
        setColour(juce::Slider::thumbColourId, juce::Colours::white);
        setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff1a1d21));
        setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff333333));
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

        // Solid shade inside the knob so it pops from the background
        g.setColour (juce::Colour(0xff22252a)); 
        g.fillEllipse (rx, ry, rw, rw);

        // Knob border
        g.setColour (juce::Colour(0xff0d0e11));
        g.drawEllipse (rx, ry, rw, rw, 2.0f);

        // Colored Value Arc
        juce::Path filledArc;
        filledArc.addCentredArc (centreX, centreY, radius, radius, 0.0f, rotaryStartAngle, angle, true);
        
        if (slider.getName() == "Character" || slider.getName() == "Drive")
            g.setColour (juce::Colour(0xff00e699)); // Cyan/Green
        else
            g.setColour (juce::Colour(0xffff9900)); // Orange

        g.strokePath (filledArc, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Pointer Tick
        juce::Path pointer;
        auto pointerLength = radius * 0.33f;
        auto pointerThickness = 3.0f;
        pointer.addRectangle (-pointerThickness * 0.5f, -radius, pointerThickness, pointerLength);
        pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centreX, centreY));
        g.setColour (juce::Colours::white);
        g.fillPath (pointer);
    }
};

// --- VU METER COMPONENT ---
class VuMeter : public juce::Component, public juce::Timer
{
public:
    VuMeter(VulcanProcessor& p) : processor(p) { startTimerHz(60); }
    ~VuMeter() override { stopTimer(); }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        g.setColour(juce::Colour(0xff0d0e11)); // Meter background
        g.fillRoundedRectangle(bounds, 4.0f);

        float w = bounds.getWidth() / 2.0f - 2.0f; // Width per channel
        
        auto drawBar = [&](float x, float level) {
            float db = juce::Decibels::gainToDecibels(level, -60.0f);
            float normalized = juce::jmap(db, -60.0f, 6.0f, 0.0f, 1.0f);
            normalized = juce::jlimit(0.0f, 1.0f, normalized);
            
            float h = bounds.getHeight() * normalized;
            float yPos = bounds.getHeight() - h;
            
            juce::Rectangle<float> bar(x, yPos, w, h);
            juce::ColourGradient grad(juce::Colours::red, 0, 0, juce::Colour(0xff00e699), 0, bounds.getHeight(), false);
            grad.addColour(0.3, juce::Colours::yellow);
            g.setGradientFill(grad);
            g.fillRoundedRectangle(bar, 2.0f);
        };

        drawBar(1.0f, levelL);
        drawBar(bounds.getWidth() / 2.0f + 1.0f, levelR);
    }

    void timerCallback() override
    {
        float newL = processor.peakL.load();
        float newR = processor.peakR.load();
        
        // Smooth decay physics
        levelL = newL >= levelL ? newL : levelL * 0.85f;
        levelR = newR >= levelR ? newR : levelR * 0.85f;
        
        repaint();
    }

private:
    VulcanProcessor& processor;
    float levelL = 0.0f, levelR = 0.0f;
};

// --- EDITOR CLASS ---
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
    VuMeter vuMeter;

    juce::ComboBox presetBox;

    juce::Slider inputGainSlider, eqLowSlider, eqMidSlider, eqHighSlider;
    juce::Slider monoFreqSlider, amountSlider, limiterDriveSlider, outputGainSlider;
    
    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Attachment> inAttach, lowAttach, midAttach, highAttach;
    std::unique_ptr<Attachment> monoAttach, amountAttach, driveAttach, outAttach;

    juce::Label titleLabel, subTitleLabel;
    juce::Label inLabel, lowLabel, midLabel, highLabel;
    juce::Label monoLabel, amountLabel, driveLabel, outLabel;

    void setupSlider (juce::Slider&, juce::Label&, const juce::String&, const juce::String&);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VulcanEditor)
};
