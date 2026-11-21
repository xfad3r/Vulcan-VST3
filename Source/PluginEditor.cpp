#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
    // Boutique Cobalt / Cerulean Blue + Warm Cream Silkscreen Palette
    const juce::Colour kChassisDark     (0xff0b1926);
    const juce::Colour kFaceplateTop    (0xff1e5682);
    const juce::Colour kFaceplateBot    (0xff143c5c);
    const juce::Colour kPanelRecess     (0xff11324d);
    const juce::Colour kSilkCream       (0xfff4f0e6);
    const juce::Colour kSilkMuted       (0xffb8d4e8);
    const juce::Colour kSkyCyan         (0xff5ce1e6);
    const juce::Colour kWarmAmber       (0xffffc857);
    const juce::Colour kCoralAlert      (0xffff6b6b);
    const juce::Colour kKnobBodyDark    (0xff172029);
    const juce::Colour kKnobCapMid      (0xff263442);

    juce::Font makeBoutiqueFont (float height, bool bold = false, bool mono = false)
    {
       #if JUCE_MAJOR_VERSION >= 8
        auto opts = juce::FontOptions (height);
        if (mono)
            opts = opts.withName (juce::Font::getDefaultMonospacedFontName());
        if (bold)
            opts = opts.withStyle ("Bold");
        return juce::Font (opts);
       #else
        if (mono)
            return juce::Font (juce::Font::getDefaultMonospacedFontName(), height, bold ? juce::Font::bold : juce::Font::plain);
        return juce::Font (height, bold ? juce::Font::bold : juce::Font::plain);
       #endif
    }
}

//==============================================================================
// VulcanBoutiqueLookAndFeel Implementation
//==============================================================================
VulcanBoutiqueLookAndFeel::VulcanBoutiqueLookAndFeel()
{
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff123450));
    setColour (juce::PopupMenu::textColourId, kSilkCream);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff24689c));
    setColour (juce::PopupMenu::highlightedTextColourId, kSkyCyan);
}

void VulcanBoutiqueLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                                  float sliderPos, float rotaryStartAngle,
                                                  float rotaryEndAngle, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                                static_cast<float> (width), static_cast<float> (height)).reduced (4.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const float cx = bounds.getCentreX();
    const float cy = bounds.getCentreY();
    const float toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    const juce::Colour arcAccent = slider.findColour (juce::Slider::rotarySliderFillColourId);
    const bool isBipolar = static_cast<bool> (slider.getProperties().getWithDefault ("bipolar", false));

    // 1. Boutique Silkscreen Dial Ticks & Dots (13 crisp markers)
    const int numTicks = 13;
    for (int i = 0; i < numTicks; ++i)
    {
        const float prop = static_cast<float> (i) / static_cast<float> (numTicks - 1);
        const float ang = rotaryStartAngle + prop * (rotaryEndAngle - rotaryStartAngle);
        const bool isMajor = (i == 0 || i == 6 || i == 12);

        const float sinA = std::sin (ang);
        const float cosA = -std::cos (ang);

        if (isMajor)
        {
            const float rIn  = radius * 0.86f;
            const float rOut = radius * 0.99f;
            g.setColour (kSilkCream.withAlpha (0.92f));
            g.drawLine (cx + sinA * rIn, cy + cosA * rIn,
                        cx + sinA * rOut, cy + cosA * rOut, 2.0f);
        }
        else
        {
            const float rDot = radius * 0.92f;
            const float dotR = 1.6f;
            g.setColour (kSilkCream.withAlpha (0.55f));
            g.fillEllipse (cx + sinA * rDot - dotR, cy + cosA * rDot - dotR, dotR * 2.0f, dotR * 2.0f);
        }
    }

    // 2. Clean Value Arc Ring
    const float arcR = radius * 0.78f;
    juce::Path bgTrack;
    bgTrack.addCentredArc (cx, cy, arcR, arcR, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (kChassisDark.withAlpha (0.55f));
    g.strokePath (bgTrack, juce::PathStrokeType (3.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float startAng = isBipolar ? (rotaryStartAngle + rotaryEndAngle) * 0.5f : rotaryStartAngle;
    if (std::abs (toAngle - startAng) > 0.01f)
    {
        juce::Path activeArc;
        activeArc.addCentredArc (cx, cy, arcR, arcR, 0.0f,
                                 juce::jmin (startAng, toAngle),
                                 juce::jmax (startAng, toAngle), true);
        g.setColour (arcAccent);
        g.strokePath (activeArc, juce::PathStrokeType (3.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // 3. Stompbox Knob Physical Cast Shadow
    const float knobR = radius * 0.67f;
    g.setColour (juce::Colour (0xff07121c).withAlpha (0.55f));
    g.fillEllipse (cx - knobR + 2.0f, cy - knobR + 4.5f, knobR * 2.0f, knobR * 2.0f);

    // 4. Fluted Boutique Pedal Skirt (12 scalloped lobes rotating with the knob)
    const int numFlutes = 12;
    juce::Path flutedSkirt;
    const int resolution = 96;
    for (int step = 0; step <= resolution; ++step)
    {
        const float theta = (static_cast<float> (step) / static_cast<float> (resolution)) * juce::MathConstants<float>::twoPi;
        const float fluteWave = std::cos (static_cast<float> (numFlutes) * (theta - toAngle));
        const float rMod = knobR * (0.955f + 0.045f * fluteWave);
        const float px = cx + std::sin (theta) * rMod;
        const float py = cy - std::cos (theta) * rMod;

        if (step == 0)
            flutedSkirt.startNewSubPath (px, py);
        else
            flutedSkirt.lineTo (px, py);
    }
    flutedSkirt.closeSubPath();

    juce::ColourGradient skirtGrad (juce::Colour (0xff2d3d4d), cx - knobR * 0.4f, cy - knobR * 0.7f,
                                    kKnobBodyDark, cx + knobR * 0.5f, cy + knobR * 0.8f, false);
    g.setGradientFill (skirtGrad);
    g.fillPath (flutedSkirt);

    g.setColour (kChassisDark.withAlpha (0.85f));
    g.strokePath (flutedSkirt, juce::PathStrokeType (1.3f));

    // 5. Matte Anodized Top Cap
    const float capR = knobR * 0.78f;
    juce::ColourGradient capGrad (kKnobCapMid, cx - capR * 0.35f, cy - capR * 0.55f,
                                  kKnobBodyDark, cx + capR * 0.45f, cy + capR * 0.65f, false);
    g.setGradientFill (capGrad);
    g.fillEllipse (cx - capR, cy - capR, capR * 2.0f, capR * 2.0f);

    // Crisp rim highlight
    g.setColour (kSilkCream.withAlpha (0.14f));
    g.drawEllipse (cx - capR + 0.6f, cy - capR + 0.6f, (capR - 0.6f) * 2.0f, (capR - 0.6f) * 2.0f, 1.0f);

    // 6. Bold Warm-Cream Boutique Pointer Bar
    const float pSin = std::sin (toAngle);
    const float pCos = -std::cos (toAngle);
    const float pStart = capR * 0.22f;
    const float pEnd   = capR * 0.92f;

    juce::Path pointerBar;
    pointerBar.startNewSubPath (cx + pSin * pStart, cy + pCos * pStart);
    pointerBar.lineTo (cx + pSin * pEnd, cy + pCos * pEnd);

    g.setColour (kSilkCream);
    g.strokePath (pointerBar, juce::PathStrokeType (3.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Subtle center dimple
    g.setColour (kChassisDark.withAlpha (0.65f));
    g.fillEllipse (cx - 2.5f, cy - 2.5f, 5.0f, 5.0f);
}

void VulcanBoutiqueLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                              int, int, int, int, juce::ComboBox&)
{
    const auto box = juce::Rectangle<float> (0.0f, 0.0f, static_cast<float> (width), static_cast<float> (height));

    // Warm Cream Pill Selector (Signature Canvas Audio contrast accent)
    g.setColour (kChassisDark.withAlpha (0.35f));
    g.fillRoundedRectangle (box.translated (0.0f, 2.0f), 6.0f);

    g.setColour (kSilkCream);
    g.fillRoundedRectangle (box, 6.0f);

    g.setColour (kChassisDark.withAlpha (0.4f));
    g.drawRoundedRectangle (box.reduced (0.5f), 6.0f, 1.2f);

    // Dark Blue Dropdown Chevron
    juce::Path chevron;
    const float ax = static_cast<float> (width) - 16.0f;
    const float ay = static_cast<float> (height) * 0.5f;
    chevron.startNewSubPath (ax - 4.5f, ay - 2.0f);
    chevron.lineTo (ax, ay + 2.5f);
    chevron.lineTo (ax + 4.5f, ay - 2.0f);

    g.setColour (kFaceplateBot);
    g.strokePath (chevron, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void VulcanBoutiqueLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (10, 1, box.getWidth() - 28, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
    label.setColour (juce::Label::textColourId, kFaceplateBot);
}

juce::Font VulcanBoutiqueLookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return makeBoutiqueFont (12.0f, true, false);
}

juce::Font VulcanBoutiqueLookAndFeel::getPopupMenuFont()
{
    return makeBoutiqueFont (12.0f, true, false);
}

//==============================================================================
// VulcanEditor Implementation
//==============================================================================
VulcanEditor::VulcanEditor (VulcanProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&boutiqueLnf);

    // Top Row: 5 Core Controls (Hero size for Stereo Width)
    setupKnob (inGainKnob,      "input_gain",  "INPUT",        "TRIM",      "dB", kWarmAmber, true,  false);
    setupKnob (monoFreqKnob,    "mono_freq",   "MONO LOCK",    "X-OVER",    "Hz", kSilkCream, false, false);
    setupKnob (stereoWidthKnob, "haas_amount", "STEREO WIDTH", "HAAS L/R",  "%",  kSkyCyan,   false, true);
    setupKnob (characterKnob,   "amount",      "HARMONICS",    "SATURATE",  "%",  kWarmAmber, false, false);
    setupKnob (outGainKnob,     "output_gain", "OUTPUT",       "LEVEL",     "dB", kSilkCream, true,  false);

    // Bottom Row: 4 Tone & Ceiling Controls
    setupKnob (eqLowKnob,        "eq_low",        "LOW",         "100 Hz",  "dB", kSkyCyan,   true,  false);
    setupKnob (eqMidKnob,        "eq_mid",        "MID",         "2.5 kHz", "dB", kSkyCyan,   true,  false);
    setupKnob (eqHighKnob,       "eq_high",       "HIGH",        "10 kHz",  "dB", kSkyCyan,   true,  false);
    setupKnob (limiterDriveKnob, "limiter_drive", "CEILING DRV", "LIMITER", "dB", kWarmAmber, false, false);

    // Preset Selector
    addAndMakeVisible (presetSelector);
    for (int i = 0; i < audioProcessor.getNumPrograms(); ++i)
        presetSelector.addItem (audioProcessor.getProgramName (i), i + 1);

    presetSelector.setSelectedId (audioProcessor.getCurrentProgram() + 1, juce::dontSendNotification);
    presetSelector.onChange = [this]
    {
        const int idx = presetSelector.getSelectedId() - 1;
        if (idx >= 0)
            audioProcessor.setCurrentProgram (idx);
    };

    setSize (860, 440);
    startTimerHz (30);
}

VulcanEditor::~VulcanEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void VulcanEditor::setupKnob (KnobControl& kc, const juce::String& paramID,
                              const juce::String& titleText, const juce::String& subText,
                              const juce::String& unitText, juce::Colour arcCol,
                              bool bipolar, bool hero)
{
    kc.title = titleText;
    kc.subLabel = subText;
    kc.unit = unitText;
    kc.arcColour = arcCol;
    kc.isBipolar = bipolar;
    kc.isHero = hero;

    kc.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    kc.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    kc.slider.setColour (juce::Slider::rotarySliderFillColourId, arcCol);
    kc.slider.getProperties().set ("bipolar", bipolar);
    kc.slider.onValueChange = [this] { repaint(); };

    addAndMakeVisible (kc.slider);
    attachments.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, paramID, kc.slider));
}

void VulcanEditor::mouseDown (const juce::MouseEvent& event)
{
    // Easter egg inspired by Canvas Audio Prickly Clip: clicking corner screws spins them!
    const auto pos = event.position;
    for (size_t i = 0; i < screwCenters.size(); ++i)
    {
        if (pos.getDistanceFrom (screwCenters[i]) <= 16.0f)
        {
            screwVelocities[i] += 0.55f;
            repaint();
            return;
        }
    }
}

void VulcanEditor::timerCallback()
{
    const int expectedId = audioProcessor.getCurrentProgram() + 1;
    if (presetSelector.getSelectedId() != expectedId)
        presetSelector.setSelectedId (expectedId, juce::dontSendNotification);

    // Animate any spinning corner screws
    bool screwMoving = false;
    for (size_t i = 0; i < screwCenters.size(); ++i)
    {
        if (std::abs (screwVelocities[i]) > 0.002f)
        {
            screwAngles[i] += screwVelocities[i];
            screwVelocities[i] *= 0.90f;
            screwMoving = true;
        }
    }

    // Ballistic stereo meter smoothing
    const float rawL = audioProcessor.peakL.load();
    const float rawR = audioProcessor.peakR.load();

    smoothPeakL = (rawL > smoothPeakL) ? rawL : (smoothPeakL * 0.85f);
    smoothPeakR = (rawR > smoothPeakR) ? rawR : (smoothPeakR * 0.85f);

    if (rawL >= peakHoldL) { peakHoldL = rawL; holdTimerL = 42; }
    else if (--holdTimerL <= 0) { peakHoldL *= 0.94f; }

    if (rawR >= peakHoldR) { peakHoldR = rawR; holdTimerR = 42; }
    else if (--holdTimerR <= 0) { peakHoldR *= 0.94f; }

    if (screwMoving)
        repaint();
    else
    {
        repaint (meterRect);
        repaint (telemetryPillRect);
    }
}

void VulcanEditor::rebuildFaceplateTexture()
{
    const int w = getWidth();
    const int h = getHeight();
    if (w <= 0 || h <= 0)
        return;

    faceplateCache = juce::Image (juce::Image::ARGB, w, h, true);
    juce::Graphics g (faceplateCache);

    // 1. Deep Navy Outer Bevel Chassis
    g.fillAll (kChassisDark);

    const auto plate = outerFrameRect.toFloat();

    // Soft cast shadow behind blue faceplate
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (plate.translated (0.0f, 3.0f), 14.0f);

    // 2. Cobalt / Cerulean Matte Powder-Coated Blue Faceplate
    juce::ColourGradient blueGrad (kFaceplateTop, 0.0f, plate.getY(),
                                   kFaceplateBot, 0.0f, plate.getBottom(), false);
    g.setGradientFill (blueGrad);
    g.fillRoundedRectangle (plate, 14.0f);

    // Top edge metallic highlight & bottom bevel shadow
    g.setColour (juce::Colour (0xff4ea8de).withAlpha (0.45f));
    g.drawRoundedRectangle (plate.reduced (0.8f), 13.5f, 1.5f);

    // 3. Deterministic Powder-Coat Mineral Micro-Texture
    juce::Random texRng (20261121);
    for (int i = 0; i < 4200; ++i)
    {
        const float rx = plate.getX() + texRng.nextFloat() * plate.getWidth();
        const float ry = plate.getY() + texRng.nextFloat() * plate.getHeight();
        const bool lightSpeck = (i % 2 == 0);
        g.setColour (lightSpeck ? juce::Colours::white.withAlpha (0.022f)
                                : kChassisDark.withAlpha (0.045f));
        g.fillRect (rx, ry, 1.5f, 1.5f);
    }

    // 4. Classic Cream Boutique Silkscreen Inner Border Frame
    const auto silkFrame = plate.reduced (18.0f);
    g.setColour (kSilkCream.withAlpha (0.78f));
    g.drawRoundedRectangle (silkFrame, 10.0f, 1.8f);

    // Subtle secondary inner hairline
    g.setColour (kSilkCream.withAlpha (0.22f));
    g.drawRoundedRectangle (silkFrame.reduced (4.0f), 7.5f, 1.0f);

    // 5. Recessed Sub-Zones for Top & Bottom Control Banks
    const auto topBankRect = juce::Rectangle<float> (silkFrame.getX() + 14.0f, 76.0f, 712.0f, 166.0f);
    const auto botBankRect = juce::Rectangle<float> (silkFrame.getX() + 14.0f, 252.0f, 712.0f, 146.0f);

    g.setColour (kPanelRecess.withAlpha (0.42f));
    g.fillRoundedRectangle (topBankRect, 9.0f);
    g.fillRoundedRectangle (botBankRect, 9.0f);

    g.setColour (kSilkCream.withAlpha (0.18f));
    g.drawRoundedRectangle (topBankRect, 9.0f, 1.0f);
    g.drawRoundedRectangle (botBankRect, 9.0f, 1.0f);

    // Subtle vertical dividers separating EQ trio from Limiter Drive in bottom bank
    g.setColour (kSilkCream.withAlpha (0.20f));
    g.drawLine (568.0f, botBankRect.getY() + 14.0f, 568.0f, botBankRect.getBottom() - 14.0f, 1.2f);
}

void VulcanEditor::resized()
{
    outerFrameRect    = { 8, 8, getWidth() - 16, getHeight() - 16 };
    headerRect        = { 34, 24, getWidth() - 68, 46 };
    telemetryPillRect = { 346, 33, 276, 28 };
    presetSelector.setBounds (638, 32, 184, 30);

    meterRect = { 758, 76, 66, 322 };

    // 4 Corner Screw Positions
    screwCenters[0] = { 21.0f, 21.0f };
    screwCenters[1] = { static_cast<float> (getWidth()) - 21.0f, 21.0f };
    screwCenters[2] = { 21.0f, static_cast<float> (getHeight()) - 21.0f };
    screwCenters[3] = { static_cast<float> (getWidth()) - 21.0f, static_cast<float> (getHeight()) - 21.0f };

    auto placeKnob = [] (KnobControl& kc, int cx, int cy, int diameter)
    {
        kc.bounds = { cx - diameter / 2, cy - diameter / 2, diameter, diameter };
        kc.slider.setBounds (kc.bounds);
        kc.readoutBounds = { cx - 36, cy + diameter / 2 + 2, 72, 18 };
    };

    // Top Row (5 Controls — centered at y = 160)
    const int topY = 160;
    placeKnob (inGainKnob,      102, topY, 84);
    placeKnob (monoFreqKnob,    244, topY, 84);
    placeKnob (stereoWidthKnob, 396, topY - 2, 102); // Center Hero Knob
    placeKnob (characterKnob,   548, topY, 84);
    placeKnob (outGainKnob,     684, topY, 84);

    // Bottom Row (3 Pre-EQ Knobs + Limiter Drive — centered at y = 326)
    const int botY = 326;
    placeKnob (eqLowKnob,        124, botY, 74);
    placeKnob (eqMidKnob,        296, botY, 74);
    placeKnob (eqHighKnob,       468, botY, 74);
    placeKnob (limiterDriveKnob, 654, botY, 78);

    rebuildFaceplateTexture();
}

juce::String VulcanEditor::formatKnobValue (const KnobControl& kc, const juce::String& id) const
{
    const float val = static_cast<float> (kc.slider.getValue());

    if (id == "mono_freq")
        return juce::String (static_cast<int> (std::round (val))) + " Hz";

    if (id == "haas_amount")
        return juce::String (static_cast<int> (std::round (val * 100.0f))) + "%";

    if (id == "amount")
        return juce::String (static_cast<int> (std::round (val * 50.0f))) + "%";

    const juce::String prefix = (kc.isBipolar && val > 0.05f) ? "+" : "";
    return prefix + juce::String (val, 1) + " " + kc.unit;
}

void VulcanEditor::drawBoutiqueScrew (juce::Graphics& g, juce::Point<float> c, float r, float angleRad)
{
    // Cast shadow
    g.setColour (kChassisDark.withAlpha (0.65f));
    g.fillEllipse (c.x - r + 1.0f, c.y - r + 1.5f, r * 2.0f, r * 2.0f);

    // Brushed stainless screw head
    juce::ColourGradient metalGrad (juce::Colour (0xffd8e2ec), c.x - r * 0.5f, c.y - r * 0.5f,
                                    juce::Colour (0xff6c7a89), c.x + r * 0.6f, c.y + r * 0.6f, false);
    g.setGradientFill (metalGrad);
    g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);

    g.setColour (kChassisDark.withAlpha (0.8f));
    g.drawEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f, 1.0f);

    // Rotatable Phillips cross-slot
    const float s = std::sin (angleRad) * (r * 0.58f);
    const float co = std::cos (angleRad) * (r * 0.58f);

    g.setColour (juce::Colour (0xff2b3642));
    g.drawLine (c.x - co, c.y - s, c.x + co, c.y + s, 1.6f);
    g.drawLine (c.x + s, c.y - co, c.x - s, c.y + co, 1.6f);
}

void VulcanEditor::drawJewelLight (juce::Graphics& g, juce::Point<float> c, float r)
{
    // Outer ambient cyan glow
    juce::ColourGradient glow (kSkyCyan.withAlpha (0.45f), c.x, c.y,
                               kSkyCyan.withAlpha (0.0f), c.x + r * 2.4f, c.y, true);
    g.setGradientFill (glow);
    g.fillEllipse (c.x - r * 2.4f, c.y - r * 2.4f, r * 4.8f, r * 4.8f);

    // Chrome bezel ring
    g.setColour (juce::Colour (0xffcbd5e1));
    g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
    g.setColour (kChassisDark);
    g.drawEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f, 1.2f);

    // Faceted Glowing Blue/Cyan Jewel Lens
    const float lensR = r * 0.74f;
    juce::ColourGradient jewelGrad (juce::Colour (0xffe0fbfc), c.x - lensR * 0.3f, c.y - lensR * 0.3f,
                                    juce::Colour (0xff0077b6), c.x + lensR * 0.5f, c.y + lensR * 0.5f, true);
    g.setGradientFill (jewelGrad);
    g.fillEllipse (c.x - lensR, c.y - lensR, lensR * 2.0f, lensR * 2.0f);

    // Specular pinpoint sparkle
    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.fillEllipse (c.x - lensR * 0.45f, c.y - lensR * 0.45f, 2.5f, 2.5f);
}

void VulcanEditor::drawVulcanEmblem (juce::Graphics& g, juce::Rectangle<float> area)
{
    // Minimalist custom geometric V-wave crest in warm cream
    g.setColour (kSilkCream);
    g.drawRoundedRectangle (area, 6.0f, 1.6f);

    juce::Path bolt;
    const float cx = area.getCentreX();
    const float cy = area.getCentreY();
    bolt.startNewSubPath (cx - 7.0f, cy - 6.0f);
    bolt.lineTo (cx, cy + 7.0f);
    bolt.lineTo (cx + 7.0f, cy - 6.0f);

    g.setColour (kSkyCyan);
    g.strokePath (bolt, juce::PathStrokeType (2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void VulcanEditor::drawKnobGraphics (juce::Graphics& g, const KnobControl& kc, const juce::String& formattedText)
{
    const int labelW = kc.bounds.getWidth() + 26;
    const int labelX = kc.bounds.getCentreX() - labelW / 2;

    // Main Silkscreen Parameter Name above knob
    g.setFont (makeBoutiqueFont (kc.isHero ? 12.0f : 11.0f, true, false));
    g.setColour (kSilkCream);
    g.drawText (kc.title, labelX, kc.bounds.getY() - 28, labelW, 15, juce::Justification::centred);

    // Subtle secondary descriptor
    g.setFont (makeBoutiqueFont (9.0f, true, true));
    g.setColour (kc.isHero ? kSkyCyan : kSilkMuted.withAlpha (0.8f));
    g.drawText (kc.subLabel, labelX, kc.bounds.getY() - 14, labelW, 12, juce::Justification::centred);

    // Clean Boutique Value Pill underneath knob
    const auto pill = kc.readoutBounds.toFloat();
    g.setColour (kChassisDark.withAlpha (0.62f));
    g.fillRoundedRectangle (pill, 4.0f);

    g.setColour (kSilkCream.withAlpha (0.28f));
    g.drawRoundedRectangle (pill, 4.0f, 1.0f);

    g.setFont (makeBoutiqueFont (11.0f, true, true));
    g.setColour (kc.isHero ? kSkyCyan : kSilkCream);
    g.drawText (formattedText, kc.readoutBounds, juce::Justification::centred);
}

void VulcanEditor::drawStereoMeter (juce::Graphics& g, juce::Rectangle<int> rect)
{
    const auto fRect = rect.toFloat();

    // Recessed Navy Well matching the control banks
    g.setColour (kPanelRecess.withAlpha (0.68f));
    g.fillRoundedRectangle (fRect, 9.0f);
    g.setColour (kSilkCream.withAlpha (0.35f));
    g.drawRoundedRectangle (fRect, 9.0f, 1.2f);

    // Top Header
    g.setFont (makeBoutiqueFont (10.0f, true, false));
    g.setColour (kSilkCream);
    g.drawText ("OUT", rect.getX(), rect.getY() + 7, rect.getWidth(), 14, juce::Justification::centred);

    const float barW = 10.0f;
    const float barTop = fRect.getY() + 34.0f;
    const float barBot = fRect.getBottom() - 36.0f;
    const float barH = barBot - barTop;

    const float leftBarX  = fRect.getX() + 12.0f;
    const float rightBarX = fRect.getRight() - 12.0f - barW;

    // Clip LEDs
    const bool clipL = (peakHoldL >= 0.985f);
    const bool clipR = (peakHoldR >= 0.985f);
    g.setColour (clipL ? kCoralAlert : kChassisDark.withAlpha (0.7f));
    g.fillRoundedRectangle (leftBarX, fRect.getY() + 24.0f, barW, 5.0f, 2.0f);
    g.setColour (clipR ? kCoralAlert : kChassisDark.withAlpha (0.7f));
    g.fillRoundedRectangle (rightBarX, fRect.getY() + 24.0f, barW, 5.0f, 2.0f);

    // Bar backgrounds
    g.setColour (kChassisDark.withAlpha (0.75f));
    g.fillRoundedRectangle (leftBarX, barTop, barW, barH, 3.0f);
    g.fillRoundedRectangle (rightBarX, barTop, barW, barH, 3.0f);

    const float dbFloor = -48.0f;
    const float dbL = juce::Decibels::gainToDecibels (smoothPeakL, dbFloor);
    const float dbR = juce::Decibels::gainToDecibels (smoothPeakR, dbFloor);
    const float holdDbL = juce::Decibels::gainToDecibels (peakHoldL, dbFloor);
    const float holdDbR = juce::Decibels::gainToDecibels (peakHoldR, dbFloor);

    const float normL = juce::jlimit (0.0f, 1.0f, (dbL - dbFloor) / (-dbFloor));
    const float normR = juce::jlimit (0.0f, 1.0f, (dbR - dbFloor) / (-dbFloor));
    const float normHoldL = juce::jlimit (0.0f, 1.0f, (holdDbL - dbFloor) / (-dbFloor));
    const float normHoldR = juce::jlimit (0.0f, 1.0f, (holdDbR - dbFloor) / (-dbFloor));

    auto drawChannelFill = [&] (float bx, float normVal, float holdVal)
    {
        if (normVal > 0.005f)
        {
            const float fillH = barH * normVal;
            const float fillY = barBot - fillH;

            juce::ColourGradient meterGrad (kCoralAlert, bx, barTop,
                                            kSkyCyan, bx, barBot, false);
            meterGrad.addColour (0.18, kWarmAmber);
            meterGrad.addColour (0.45, kSkyCyan);

            g.setGradientFill (meterGrad);
            g.fillRoundedRectangle (bx + 1.0f, fillY, barW - 2.0f, fillH, 2.0f);
        }

        if (holdVal > 0.01f)
        {
            const float holdY = barBot - barH * holdVal;
            g.setColour (kSilkCream);
            g.fillRect (bx, holdY, barW, 2.0f);
        }
    };

    drawChannelFill (leftBarX, normL, normHoldL);
    drawChannelFill (rightBarX, normR, normHoldR);

    // Center dB Ticks
    g.setFont (makeBoutiqueFont (8.5f, true, true));
    g.setColour (kSilkMuted.withAlpha (0.85f));
    const int marks[] = { 0, -6, -12, -24, -36 };
    for (int m : marks)
    {
        const float n = (static_cast<float> (m) - dbFloor) / (-dbFloor);
        const float yM = barBot - n * barH - 5.0f;
        g.drawText (juce::String (std::abs (m)),
                    static_cast<int> (leftBarX + barW), static_cast<int> (yM),
                    static_cast<int> (rightBarX - (leftBarX + barW)), 10, juce::Justification::centred);
    }

    // L / R labels & Peak numeric readout
    g.setFont (makeBoutiqueFont (9.5f, true, true));
    g.setColour (kSilkCream);
    g.drawText ("L", static_cast<int> (leftBarX), static_cast<int> (barBot + 3.0f), static_cast<int> (barW), 12, juce::Justification::centred);
    g.drawText ("R", static_cast<int> (rightBarX), static_cast<int> (barBot + 3.0f), static_cast<int> (barW), 12, juce::Justification::centred);

    const float maxPeak = juce::jmax (peakHoldL, peakHoldR);
    const juce::String peakTxt = (maxPeak <= 0.0005f) ? "-INF"
                                                      : juce::String (juce::Decibels::gainToDecibels (maxPeak), 1);

    g.setFont (makeBoutiqueFont (10.0f, true, true));
    g.setColour (clipL || clipR ? kCoralAlert : kSkyCyan);
    g.drawText (peakTxt, rect.getX() + 4, rect.getBottom() - 20, rect.getWidth() - 8, 15, juce::Justification::centred);
}

void VulcanEditor::drawFaceplate (juce::Graphics& g)
{
    if (faceplateCache.isValid())
        g.drawImageAt (faceplateCache, 0, 0);

    // 4 Interactive Corner Screws
    for (size_t i = 0; i < screwCenters.size(); ++i)
        drawBoutiqueScrew (g, screwCenters[i], 6.5f, screwAngles[i]);

    // Top-Left Jewel Pilot Lamp + Brand Emblem + Title
    drawJewelLight (g, { 46.0f, 47.0f }, 7.5f);
    drawVulcanEmblem (g, { 64.0f, 35.0f, 26.0f, 24.0f });

    g.setFont (makeBoutiqueFont (23.0f, true, false));
    g.setColour (kSilkCream);
    g.drawText ("VULCAN", 98, 29, 120, 26, juce::Justification::centredLeft);

    g.setFont (makeBoutiqueFont (9.5f, true, true));
    g.setColour (kSilkMuted);
    g.drawText ("BOUTIQUE PHASE & TRANSIENT ENGINE", 99, 51, 235, 13, juce::Justification::centredLeft);

    // Center Live Haas Telemetry Pill
    const auto pill = telemetryPillRect.toFloat();
    g.setColour (kChassisDark.withAlpha (0.65f));
    g.fillRoundedRectangle (pill, 6.0f);
    g.setColour (kSilkCream.withAlpha (0.30f));
    g.drawRoundedRectangle (pill, 6.0f, 1.0f);

    const double sr = audioProcessor.getSampleRate() > 0.0 ? audioProcessor.getSampleRate() : 44100.0;
    const float rateMult = static_cast<float> (sr / 44100.0);
    const float widthVal = static_cast<float> (stereoWidthKnob.slider.getValue());
    const int dMid  = static_cast<int> (std::round (23.0f * rateMult * widthVal));
    const int dHigh = static_cast<int> (std::round (41.0f * rateMult * widthVal));

    const juce::String teleStr = "HAAS: " + juce::String (dMid) + " / " + juce::String (dHigh)
                               + " SMP  |  " + juce::String (sr / 1000.0, 1) + " kHz";

    g.setFont (makeBoutiqueFont (11.0f, true, true));
    g.setColour (kSkyCyan);
    g.drawText (teleStr, telemetryPillRect, juce::Justification::centred);
}

void VulcanEditor::paint (juce::Graphics& g)
{
    drawFaceplate (g);
    drawStereoMeter (g, meterRect);

    // Render all 9 Silkscreen Labels & Digital Value Pills
    drawKnobGraphics (g, inGainKnob,       formatKnobValue (inGainKnob,       "input_gain"));
    drawKnobGraphics (g, monoFreqKnob,     formatKnobValue (monoFreqKnob,     "mono_freq"));
    drawKnobGraphics (g, stereoWidthKnob,  formatKnobValue (stereoWidthKnob,  "haas_amount"));
    drawKnobGraphics (g, characterKnob,    formatKnobValue (characterKnob,    "amount"));
    drawKnobGraphics (g, outGainKnob,      formatKnobValue (outGainKnob,      "output_gain"));

    drawKnobGraphics (g, eqLowKnob,        formatKnobValue (eqLowKnob,        "eq_low"));
    drawKnobGraphics (g, eqMidKnob,        formatKnobValue (eqMidKnob,        "eq_mid"));
    drawKnobGraphics (g, eqHighKnob,       formatKnobValue (eqHighKnob,       "eq_high"));
    drawKnobGraphics (g, limiterDriveKnob, formatKnobValue (limiterDriveKnob, "limiter_drive"));
}
