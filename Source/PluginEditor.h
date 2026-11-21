#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>
#include <vector>
#include <memory>

//==============================================================================
// Boutique Blue Stompbox / Faceplate LookAndFeel (Canvas-Inspired Aesthetic)
//==============================================================================
class VulcanBoutiqueLookAndFeel : public juce::LookAndFeel_V4
{
public:
    VulcanBoutiqueLookAndFeel();
    ~VulcanBoutiqueLookAndFeel() override = default;

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override;

    void drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox& box) override;

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override;

    juce::Font getComboBoxFont (juce::ComboBox& box) override;
    juce::Font getPopupMenuFont() override;
};

//==============================================================================
// Vulcan Editor (Boutique Cobalt Blue Faceplate Edition)
//==============================================================================
class VulcanEditor : public juce::AudioProcessorEditor,
                     private juce::Timer
{
public:
    explicit VulcanEditor (VulcanProcessor&);
    ~VulcanEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& event) override;

private:
    void timerCallback() override;
    void rebuildFaceplateTexture();

    struct KnobControl
    {
        juce::Slider slider;
        juce::String title;
        juce::String subLabel;
        juce::String unit;
        juce::Colour arcColour;
        bool isBipolar = false;
        bool isHero = false;
        juce::Rectangle<int> bounds;
        juce::Rectangle<int> readoutBounds;
    };

    void setupKnob (KnobControl& kc, const juce::String& paramID,
                    const juce::String& titleText, const juce::String& subText,
                    const juce::String& unitText, juce::Colour arcCol,
                    bool bipolar, bool hero);

    void drawFaceplate (juce::Graphics& g);
    void drawBoutiqueScrew (juce::Graphics& g, juce::Point<float> center, float radius, float angleRad);
    void drawJewelLight (juce::Graphics& g, juce::Point<float> center, float radius);
    void drawVulcanEmblem (juce::Graphics& g, juce::Rectangle<float> area);
    void drawKnobGraphics (juce::Graphics& g, const KnobControl& kc, const juce::String& formattedText);
    void drawStereoMeter (juce::Graphics& g, juce::Rectangle<int> rect);

    juce::String formatKnobValue (const KnobControl& kc, const juce::String& id) const;

    VulcanProcessor& audioProcessor;
    VulcanBoutiqueLookAndFeel boutiqueLnf;

    // Cached powder-coat texture image for zero-overhead 60fps painting
    juce::Image faceplateCache;

    // 9 Mastering Controls
    KnobControl inGainKnob;
    KnobControl monoFreqKnob;
    KnobControl stereoWidthKnob;
    KnobControl characterKnob;
    KnobControl outGainKnob;

    KnobControl eqLowKnob;
    KnobControl eqMidKnob;
    KnobControl eqHighKnob;
    KnobControl limiterDriveKnob;

    std::vector< std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> > attachments;
    juce::ComboBox presetSelector;

    // Interactive Corner Screws (Canvas-style tactile detail)
    std::array<juce::Point<float>, 4> screwCenters {};
    std::array<float, 4> screwAngles { { 0.35f, 1.15f, 2.20f, 0.80f } };
    std::array<float, 4> screwVelocities { { 0.0f, 0.0f, 0.0f, 0.0f } };

    // Ballistic Metering State
    float smoothPeakL = 0.0f;
    float smoothPeakR = 0.0f;
    float peakHoldL   = 0.0f;
    float peakHoldR   = 0.0f;
    int   holdTimerL  = 0;
    int   holdTimerR  = 0;

    // Layout Regions
    juce::Rectangle<int> outerFrameRect;
    juce::Rectangle<int> headerRect;
    juce::Rectangle<int> telemetryPillRect;
    juce::Rectangle<int> meterRect;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VulcanEditor)
};
