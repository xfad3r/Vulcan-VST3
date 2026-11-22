#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>
#include <vector>
#include <memory>

//==============================================================================
// Automotive Metallic Blue & 3D CNC Machined LookAndFeel
//==============================================================================
class VulcanAutomotiveLookAndFeel : public juce::LookAndFeel_V4
{
public:
    VulcanAutomotiveLookAndFeel();
    ~VulcanAutomotiveLookAndFeel() override = default;

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override;

    juce::Label* createSliderTextBox (juce::Slider& slider) override;

    void drawLabel (juce::Graphics& g, juce::Label& label) override;

    void drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox& box) override;

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override;

    juce::Font getComboBoxFont (juce::ComboBox& box) override;
    juce::Font getPopupMenuFont() override;

    juce::Rectangle<int> getTooltipBounds (const juce::String& tipText,
                                           juce::Point<int> screenPos,
                                           juce::Rectangle<int> parentArea) override;

    void drawTooltip (juce::Graphics& g, const juce::String& text, int width, int height) override;
};

//==============================================================================
// Vulcan Editor (Automotive Metallic Sapphire Blue + 3D Billet Hardware Edition)
//==============================================================================
class VulcanEditor : public juce::AudioProcessorEditor,
                     public juce::TooltipClient,
                     private juce::Timer
{
public:
    explicit VulcanEditor (VulcanProcessor&);
    ~VulcanEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& event) override;

    // Provides quick-manual hover descriptions for meters, telemetry & hardware accents
    juce::String getTooltip() override;

private:
    void timerCallback() override;
    void syncPresetDisplay();
    void rebuildMetallicChassisCache();

    enum class ValueDisplayMode
    {
        Decibels,
        FrequencyHz,
        Percent100From1,
        Percent100From2
    };

    struct KnobControl
    {
        juce::Slider slider;
        juce::String title;
        juce::String subLabel;
        juce::Colour arcColour;
        ValueDisplayMode displayMode = ValueDisplayMode::Decibels;
        bool isBipolar = false;
        bool isHero = false;
        juce::Rectangle<int> sliderBounds;
        juce::Rectangle<int> dialBounds;
    };

    struct MeterBallistics
    {
        float smoothL = 0.0f;
        float smoothR = 0.0f;
        float holdL   = 0.0f;
        float holdR   = 0.0f;
        int   timerL  = 0;
        int   timerR  = 0;

        void update (float rawL, float rawR)
        {
            smoothL = (rawL > smoothL) ? rawL : (smoothL * 0.85f);
            smoothR = (rawR > smoothR) ? rawR : (smoothR * 0.85f);

            if (rawL >= holdL) { holdL = rawL; timerL = 42; }
            else if (--timerL <= 0) { holdL *= 0.94f; }

            if (rawR >= holdR) { holdR = rawR; timerR = 42; }
            else if (--timerR <= 0) { holdR *= 0.94f; }
        }
    };

    void setupKnob (KnobControl& kc, const juce::String& paramID,
                    const juce::String& titleText, const juce::String& subText,
                    const juce::String& manualTooltip,
                    juce::Colour arcCol, ValueDisplayMode mode,
                    bool bipolar, bool hero, double defaultVal);

    void drawFaceplate (juce::Graphics& g);
    void drawChromeScrew (juce::Graphics& g, juce::Point<float> center, float radius, float angleRad);
    void drawSapphireJewel (juce::Graphics& g, juce::Point<float> center, float radius);
    void drawMetallicEmblem (juce::Graphics& g, juce::Rectangle<float> area);
    void drawKnobLabels (juce::Graphics& g, const KnobControl& kc);
    void drawStereoMeterBridge (juce::Graphics& g, juce::Rectangle<int> rect,
                                const juce::String& headerLabel, const MeterBallistics& m);

    VulcanProcessor& audioProcessor;
    VulcanAutomotiveLookAndFeel automotiveLnf;

    // Embedded Quick-Manual HUD Tooltip Engine (2.5s hover delay)
    juce::TooltipWindow tooltipWindow { this, 2500 };

    // Cached automotive metallic clearcoat texture
    juce::Image chassisCache;

    // 9 Mastering Controls with full Keyboard Editing & Hover Manual
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

    // Interactive CNC Corner Screws
    std::array<juce::Point<float>, 4> screwCenters {};
    std::array<float, 4> screwAngles { { 0.40f, 1.25f, 2.10f, 0.75f } };
    std::array<float, 4> screwVelocities { { 0.0f, 0.0f, 0.0f, 0.0f } };

    // Dual Stereo Meter Ballistics (Input + Output)
    MeterBallistics inMeter;
    MeterBallistics outMeter;

    // Layout Regions
    juce::Rectangle<int> outerPlateRect;
    juce::Rectangle<int> headerRect;
    juce::Rectangle<int> telemetryRect;
    juce::Rectangle<int> inMeterRect;
    juce::Rectangle<int> outMeterRect;
    juce::Rectangle<int> topBankRect;
    juce::Rectangle<int> botBankRect;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VulcanEditor)
};
