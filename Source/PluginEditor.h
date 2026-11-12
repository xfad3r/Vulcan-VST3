#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
// Custom Boutique Mastering Hardware LookAndFeel
//==============================================================================
class VulcanHardwareLookAndFeel : public juce::LookAndFeel_V4
{
public:
    VulcanHardwareLookAndFeel();
    ~VulcanHardwareLookAndFeel() override = default;

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
// Vulcan Editor (2U Mastering Rack Interface)
//==============================================================================
class VulcanEditor : public juce::AudioProcessorEditor,
                     private juce::Timer
{
public:
    explicit VulcanEditor (VulcanProcessor&);
    ~VulcanEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    struct KnobControl
    {
        juce::Slider slider;
        juce::String title;
        juce::String unit;
        juce::Colour accentColour;
        bool isBipolar = false;
        juce::Rectangle<int> bounds;
        juce::Rectangle<int> readoutBounds;
    };

    void setupKnob (KnobControl& kc, const juce::String& paramID,
                    const juce::String& titleText, const juce::String& unitText,
                    juce::Colour accent, bool bipolar);

    void drawRackChassis (juce::Graphics& g, juce::Rectangle<int> area);
    void drawScrew (juce::Graphics& g, float cx, float cy, float radius);
    void drawModulePanel (juce::Graphics& g, juce::Rectangle<int> rect, const juce::String& headerTitle, const juce::String& subCode);
    void drawKnobReadout (juce::Graphics& g, const KnobControl& kc, const juce::String& formattedText);
    void drawStereoMeterBridge (juce::Graphics& g, juce::Rectangle<int> rect);

    juce::String formatKnobValue (const KnobControl& kc, const juce::String& id) const;

    VulcanProcessor& audioProcessor;
    VulcanHardwareLookAndFeel hardwareLnf;

    // 9 Mastering Controls
    KnobControl inGainKnob;
    KnobControl eqLowKnob;
    KnobControl eqMidKnob;
    KnobControl eqHighKnob;

    KnobControl monoFreqKnob;
    KnobControl stereoWidthKnob;
    KnobControl characterKnob;

    KnobControl limiterDriveKnob;
    KnobControl outGainKnob;

    std::vector< std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> > attachments;

    juce::ComboBox presetSelector;

    // Ballistic Metering State
    float smoothPeakL = 0.0f;
    float smoothPeakR = 0.0f;
    float peakHoldL   = 0.0f;
    float peakHoldR   = 0.0f;
    int   holdTimerL  = 0;
    int   holdTimerR  = 0;

    // Cached layout areas
    juce::Rectangle<int> headerRect;
    juce::Rectangle<int> telemetryRect;
    juce::Rectangle<int> modPreRect;
    juce::Rectangle<int> modSpatialRect;
    juce::Rectangle<int> modFinalRect;
    juce::Rectangle<int> modMeterRect;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VulcanEditor)
};
