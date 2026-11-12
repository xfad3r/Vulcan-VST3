#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    juce::Font makeStudioFont (float height, bool bold = false, bool mono = false)
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
// VulcanHardwareLookAndFeel Implementation
//==============================================================================
VulcanHardwareLookAndFeel::VulcanHardwareLookAndFeel()
{
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff121518));
    setColour (juce::PopupMenu::textColourId, juce::Colour (0xffd0d7de));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff26313a));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colour (0xffffb444));
}

void VulcanHardwareLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                                  float sliderPos, float rotaryStartAngle,
                                                  float rotaryEndAngle, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                                static_cast<float> (width), static_cast<float> (height)).reduced (4.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const float cx = bounds.getCentreX();
    const float cy = bounds.getCentreY();
    const float toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    const juce::Colour accent = slider.findColour (juce::Slider::rotarySliderFillColourId);
    const bool isBipolar = slider.getProperties().getWithDefault ("bipolar", false);

    // 1. Engraved Precision Scale Ticks (21 steps)
    const int numTicks = 21;
    const float tickOuterR = radius * 0.98f;
    for (int i = 0; i < numTicks; ++i)
    {
        const float prop = static_cast<float> (i) / static_cast<float> (numTicks - 1);
        const float ang = rotaryStartAngle + prop * (rotaryEndAngle - rotaryStartAngle);
        const bool isMajor = (i == 0 || i == 5 || i == 10 || i == 15 || i == 20);
        const float tickInnerR = radius * (isMajor ? 0.84f : 0.89f);

        const float sinA = std::sin (ang);
        const float cosA = -std::cos (ang);

        g.setColour (isMajor ? juce::Colour (0xff6e7781) : juce::Colour (0xff3b424a));
        g.drawLine (cx + sinA * tickInnerR, cy + cosA * tickInnerR,
                    cx + sinA * tickOuterR, cy + cosA * tickOuterR,
                    isMajor ? 1.5f : 1.0f);
    }

    // 2. Recessed LED Collar Track
    const float arcRadius = radius * 0.77f;
    juce::Path backgroundArc;
    backgroundArc.addCentredArc (cx, cy, arcRadius, arcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (juce::Colour (0xff0d0f12));
    g.strokePath (backgroundArc, juce::PathStrokeType (3.2f, juce::PathStrokeType::curved, juce::PathStrokeType::butt));

    // Active LED Collar Arc (Bipolar from 12 o'clock or Unipolar from start)
    juce::Path valueArc;
    const float startAng = isBipolar ? (rotaryStartAngle + rotaryEndAngle) * 0.5f : rotaryStartAngle;
    if (std::abs (toAngle - startAng) > 0.01f)
    {
        valueArc.addCentredArc (cx, cy, arcRadius, arcRadius, 0.0f,
                                juce::jmin (startAng, toAngle),
                                juce::jmax (startAng, toAngle), true);
        g.setColour (accent.withAlpha (0.9f));
        g.strokePath (valueArc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::butt));
    }

    // 3. Hardware Drop Shadow
    const float bodyRadius = radius * 0.67f;
    g.setColour (juce::Colours::black.withAlpha (0.65f));
    g.fillEllipse (cx - bodyRadius + 1.5f, cy - bodyRadius + 3.5f, bodyRadius * 2.0f, bodyRadius * 2.0f);

    // 4. Machined Outer Knurled Skirt
    juce::ColourGradient skirtGrad (juce::Colour (0xff3a4048), cx, cy - bodyRadius,
                                    juce::Colour (0xff14171a), cx, cy + bodyRadius, false);
    g.setGradientFill (skirtGrad);
    g.fillEllipse (cx - bodyRadius, cy - bodyRadius, bodyRadius * 2.0f, bodyRadius * 2.0f);

    g.setColour (juce::Colour (0xff0a0c0e));
    g.drawEllipse (cx - bodyRadius, cy - bodyRadius, bodyRadius * 2.0f, bodyRadius * 2.0f, 1.2f);

    // Subtle knurl serrations around knob skirt
    const int knurlCount = 36;
    g.setColour (juce::Colour (0xff101215).withAlpha (0.7f));
    for (int i = 0; i < knurlCount; ++i)
    {
        const float kAng = (static_cast<float> (i) / static_cast<float> (knurlCount)) * juce::MathConstants<float>::twoPi;
        const float sA = std::sin (kAng);
        const float cA = -std::cos (kAng);
        g.drawLine (cx + sA * (bodyRadius * 0.85f), cy + cA * (bodyRadius * 0.85f),
                    cx + sA * (bodyRadius * 0.98f), cy + cA * (bodyRadius * 0.98f), 1.0f);
    }

    // 5. Anodized Aluminum Cap
    const float capRadius = bodyRadius * 0.80f;
    juce::ColourGradient capGrad (juce::Colour (0xff2e343b), cx - capRadius * 0.4f, cy - capRadius * 0.6f,
                                  juce::Colour (0xff181b20), cx + capRadius * 0.4f, cy + capRadius * 0.6f, false);
    g.setGradientFill (capGrad);
    g.fillEllipse (cx - capRadius, cy - capRadius, capRadius * 2.0f, capRadius * 2.0f);

    // Specular metallic rim highlight
    g.setColour (juce::Colours::white.withAlpha (0.09f));
    g.drawEllipse (cx - capRadius + 0.5f, cy - capRadius + 0.5f, (capRadius - 0.5f) * 2.0f, (capRadius - 0.5f) * 2.0f, 1.0f);

    // 6. Precision Engraved Pointer Line
    const float pSin = std::sin (toAngle);
    const float pCos = -std::cos (toAngle);
    const float pInner = capRadius * 0.28f;
    const float pOuter = capRadius * 0.90f;

    g.setColour (juce::Colour (0xff0c0e10));
    g.drawLine (cx + pSin * pInner + 0.8f, cy + pCos * pInner + 0.8f,
                cx + pSin * pOuter + 0.8f, cy + pCos * pOuter + 0.8f, 3.0f);

    g.setColour (juce::Colour (0xfff0f4f8));
    g.drawLine (cx + pSin * pInner, cy + pCos * pInner,
                cx + pSin * pOuter, cy + pCos * pOuter, 2.2f);

    // Center cap rivet
    g.setColour (juce::Colour (0xff121519));
    g.fillEllipse (cx - 3.0f, cy - 3.0f, 6.0f, 6.0f);
}

void VulcanHardwareLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                              int, int, int, int, juce::ComboBox&)
{
    auto boxBounds = juce::Rectangle<float> (0.0f, 0.0f, static_cast<float> (width), static_cast<float> (height));

    // Recessed OLED Bezel
    g.setColour (juce::Colour (0xff0b0d10));
    g.fillRoundedRectangle (boxBounds, 3.0f);

    g.setColour (juce::Colour (0xff2b3138));
    g.drawRoundedRectangle (boxBounds.reduced (0.5f), 3.0f, 1.0f);

    // Dropdown arrow
    juce::Path arrow;
    const float ax = static_cast<float> (width) - 15.0f;
    const float ay = static_cast<float> (height) * 0.5f;
    arrow.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
    g.setColour (juce::Colour (0xffd49a36));
    g.fillPath (arrow);
}

void VulcanHardwareLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (8, 1, box.getWidth() - 26, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
    label.setColour (juce::Label::textColourId, juce::Colour (0xffe2e8f0));
}

juce::Font VulcanHardwareLookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return makeStudioFont (12.0f, true, true);
}

juce::Font VulcanHardwareLookAndFeel::getPopupMenuFont()
{
    return makeStudioFont (12.0f, false, true);
}

//==============================================================================
// VulcanEditor Implementation
//==============================================================================
VulcanEditor::VulcanEditor (VulcanProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&hardwareLnf);

    // Studio Hardware Accent Palette (restrained anodized tones, zero AI neon)
    const juce::Colour colAmber   (0xffd49a36); // Pre-Tone & Input
    const juce::Colour colCopper  (0xffc87d46); // Mono Lock Crossover
    const juce::Colour colSteel   (0xff4a90a4); // Stereo Haas Matrix
    const juce::Colour colCrimson (0xffd15b47); // Harmonic Saturation & Limiter
    const juce::Colour colSilver  (0xff9aa5b1); // Final Output Trim

    // Section 1: Input & Pre-EQ
    setupKnob (inGainKnob,       "input_gain",    "INPUT TRIM",   "dB", colAmber,   true);
    setupKnob (eqLowKnob,        "eq_low",        "LF 100 Hz",    "dB", colAmber,   true);
    setupKnob (eqMidKnob,        "eq_mid",        "MF 2.5 kHz",   "dB", colAmber,   true);
    setupKnob (eqHighKnob,       "eq_high",       "HF 10 kHz",    "dB", colAmber,   true);

    // Section 2: Spatial Phase Matrix & Harmonic Core
    setupKnob (monoFreqKnob,     "mono_freq",     "MONO LOCK",    "Hz", colCopper,  false);
    setupKnob (stereoWidthKnob,  "haas_amount",   "STEREO WIDTH", "%",  colSteel,   false);
    setupKnob (characterKnob,    "amount",        "HARMONICS",    "%",  colCrimson, false);

    // Section 3: Limiter & Output Stage
    setupKnob (limiterDriveKnob, "limiter_drive", "CEILING DRV",  "dB", colCrimson, false);
    setupKnob (outGainKnob,      "output_gain",   "OUTPUT TRIM",  "dB", colSilver,  true);

    // Preset Selector Setup
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

    setSize (920, 460);
    startTimerHz (30);
}

VulcanEditor::~VulcanEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void VulcanEditor::setupKnob (KnobControl& kc, const juce::String& paramID,
                              const juce::String& titleText, const juce::String& unitText,
                              juce::Colour accent, bool bipolar)
{
    kc.title = titleText;
    kc.unit = unitText;
    kc.accentColour = accent;
    kc.isBipolar = bipolar;

    kc.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    kc.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    kc.slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
    kc.slider.getProperties().set ("bipolar", bipolar);
    kc.slider.onValueChange = [this] { repaint(); };

    addAndMakeVisible (kc.slider);
    attachments.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, paramID, kc.slider));
}

void VulcanEditor::timerCallback()
{
    // Sync preset selector if host restored state
    const int expectedId = audioProcessor.getCurrentProgram() + 1;
    if (presetSelector.getSelectedId() != expectedId)
        presetSelector.setSelectedId (expectedId, juce::dontSendNotification);

    const float rawL = audioProcessor.peakL.load();
    const float rawR = audioProcessor.peakR.load();

    // Smooth Ballistic Attack / Decay
    smoothPeakL = (rawL > smoothPeakL) ? rawL : (smoothPeakL * 0.86f);
    smoothPeakR = (rawR > smoothPeakR) ? rawR : (smoothPeakR * 0.86f);

    if (rawL >= peakHoldL) { peakHoldL = rawL; holdTimerL = 45; }
    else if (--holdTimerL <= 0) { peakHoldL *= 0.94f; }

    if (rawR >= peakHoldR) { peakHoldR = rawR; holdTimerR = 45; }
    else if (--holdTimerR <= 0) { peakHoldR *= 0.94f; }

    repaint (modMeterRect);
    repaint (telemetryRect);
}

void VulcanEditor::resized()
{
    headerRect     = { 18, 12, 884, 46 };
    telemetryRect  = { 300, 19, 395, 32 };

    presetSelector.setBounds (712, 21, 176, 28);

    modPreRect     = { 18,  68, 256, 376 };
    modSpatialRect = { 284, 68, 336, 376 };
    modFinalRect   = { 630, 68, 162, 376 };
    modMeterRect   = { 802, 68, 100, 376 };

    auto placeKnob = [] (KnobControl& kc, int cx, int cy, int diameter)
    {
        kc.bounds = { cx - diameter / 2, cy - diameter / 2, diameter, diameter };
        kc.slider.setBounds (kc.bounds);
        kc.readoutBounds = { cx - 40, cy + diameter / 2 + 2, 80, 18 };
    };

    // Module 1: Pre-Emphasis & Input
    placeKnob (inGainKnob, modPreRect.getCentreX(), 156, 92);
    placeKnob (eqLowKnob,  modPreRect.getX() + 48,  328, 74);
    placeKnob (eqMidKnob,  modPreRect.getCentreX(), 328, 74);
    placeKnob (eqHighKnob, modPreRect.getRight() - 48, 328, 74);

    // Module 2: Spatial Phase Matrix & Harmonic Core
    placeKnob (monoFreqKnob,    modSpatialRect.getX() + 92,     176, 108);
    placeKnob (stereoWidthKnob, modSpatialRect.getRight() - 92, 176, 108);
    placeKnob (characterKnob,   modSpatialRect.getCentreX(),    332, 98);

    // Module 3: Limiter & Output Trim
    placeKnob (limiterDriveKnob, modFinalRect.getCentreX(), 172, 96);
    placeKnob (outGainKnob,      modFinalRect.getCentreX(), 332, 96);
}

juce::String VulcanEditor::formatKnobValue (const KnobControl& kc, const juce::String& id) const
{
    const float val = static_cast<float> (kc.slider.getValue());

    if (id == "mono_freq")
        return juce::String ( static_cast<int> (std::round (val))) + " Hz";

    if (id == "haas_amount")
        return juce::String (static_cast<int> (std::round (val * 100.0f))) + " %";

    if (id == "amount")
        return juce::String (static_cast<int> (std::round (val * 50.0f))) + " %";

    const juce::String prefix = (kc.isBipolar && val > 0.0f) ? "+" : "";
    return prefix + juce::String (val, 1) + " " + kc.unit;
}

void VulcanEditor::drawRackChassis (juce::Graphics& g, juce::Rectangle<int> area)
{
    // Base Dark Gunmetal Brushed Chassis
    juce::ColourGradient chassisGrad (juce::Colour (0xff1c2026), 0.0f, 0.0f,
                                      juce::Colour (0xff121519), 0.0f, static_cast<float> (area.getHeight()), false);
    g.setGradientFill (chassisGrad);
    g.fillRect (area);

    // Subtle horizontal brushed-anodized grain
    g.setColour (juce::Colours::white.withAlpha (0.012f));
    for (int y = 0; y < area.getHeight(); y += 3)
        g.drawHorizontalLine (y, 0.0f, static_cast<float> (area.getWidth()));

    // Top & Bottom Milled Rack Rails
    g.setColour (juce::Colour (0xff2b3038));
    g.fillRect (0, 0, area.getWidth(), 3);
    g.setColour (juce::Colour (0xff090b0d));
    g.fillRect (0, area.getHeight() - 3, area.getWidth(), 3);

    // 4 Corner Rack Mounting Hex Screws
    drawScrew (g, 10.0f, 10.0f, 4.5f);
    drawScrew (g, static_cast<float> (area.getWidth()) - 10.0f, 10.0f, 4.5f);
    drawScrew (g, 10.0f, static_cast<float> (area.getHeight()) - 10.0f, 4.5f);
    drawScrew (g, static_cast<float> (area.getWidth()) - 10.0f, static_cast<float> (area.getHeight()) - 10.0f, 4.5f);
}

void VulcanEditor::drawScrew (juce::Graphics& g, float cx, float cy, float r)
{
    g.setColour (juce::Colour (0xff0b0d10));
    g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);

    g.setColour (juce::Colour (0xff2c323b));
    g.fillEllipse (cx - r * 0.75f, cy - r * 0.75f, r * 1.5f, r * 1.5f);

    g.setColour (juce::Colour (0xff111418));
    g.drawLine (cx - r * 0.45f, cy - r * 0.45f, cx + r * 0.45f, cy + r * 0.45f, 1.3f);
    g.drawLine (cx - r * 0.45f, cy + r * 0.45f, cx + r * 0.45f, cy - r * 0.45f, 1.3f);
}

void VulcanEditor::drawModulePanel (juce::Graphics& g, juce::Rectangle<int> rect,
                                    const juce::String& headerTitle, const juce::String& subCode)
{
    auto fRect = rect.toFloat();

    // Recessed Sub-Panel Well
    g.setColour (juce::Colour (0xff14171c));
    g.fillRoundedRectangle (fRect, 5.0f);

    // Inner Bevel Border
    g.setColour (juce::Colour (0xff0a0c0e));
    g.drawRoundedRectangle (fRect, 5.0f, 1.5f);
    g.setColour (juce::Colours::white.withAlpha (0.04f));
    g.drawRoundedRectangle (fRect.reduced (1.5f), 4.0f, 1.0f);

    // Module Top Header Strip
    auto topStrip = juce::Rectangle<float> (fRect.getX() + 1.5f, fRect.getY() + 1.5f, fRect.getWidth() - 3.0f, 26.0f);
    g.setColour (juce::Colour (0xff1b2026));
    g.fillRoundedRectangle (topStrip, 4.0f);

    g.setColour (juce::Colour (0xff2b313a));
    g.drawLine (fRect.getX() + 2.0f, fRect.getY() + 27.5f, fRect.getRight() - 2.0f, fRect.getY() + 27.5f, 1.0f);

    // Engraved Section Title & Subcode
    g.setFont (makeStudioFont (11.0f, true, false));
    g.setColour (juce::Colour (0xffc9d1d9));
    g.drawText (headerTitle, rect.getX() + 12, rect.getY() + 5, rect.getWidth() - 70, 18, juce::Justification::centredLeft);

    g.setFont (makeStudioFont (10.0f, false, true));
    g.setColour (juce::Colour (0xff6e7681));
    g.drawText (subCode, rect.getRight() - 68, rect.getY() + 5, 56, 18, juce::Justification::centredRight);

    // Corner panel hex screws
    drawScrew (g, fRect.getX() + 10.0f, fRect.getBottom() - 10.0f, 3.0f);
    drawScrew (g, fRect.getRight() - 10.0f, fRect.getBottom() - 10.0f, 3.0f);
}

void VulcanEditor::drawKnobReadout (juce::Graphics& g, const KnobControl& kc, const juce::String& formattedText)
{
    // Silkscreen Title above knob
    g.setFont (makeStudioFont (10.5f, true, false));
    g.setColour (juce::Colour (0xff9aa5b1));
    g.drawText (kc.title, kc.bounds.getX() - 10, kc.bounds.getY() - 17, kc.bounds.getWidth() + 20, 15, juce::Justification::centred);

    // Recessed OLED Digital Readout Pill below knob
    auto rBox = kc.readoutBounds.toFloat();
    g.setColour (juce::Colour (0xff0b0d10));
    g.fillRoundedRectangle (rBox, 3.0f);

    g.setColour (juce::Colour (0xff242930));
    g.drawRoundedRectangle (rBox, 3.0f, 1.0f);

    g.setFont (makeStudioFont (11.0f, true, true));
    g.setColour (kc.accentColour.brighter (0.25f));
    g.drawText (formattedText, kc.readoutBounds, juce::Justification::centred);
}

void VulcanEditor::drawStereoMeterBridge (juce::Graphics& g, juce::Rectangle<int> rect)
{
    drawModulePanel (g, rect, "OUTPUT", "dBFS");

    auto bezel = juce::Rectangle<float> (static_cast<float> (rect.getX() + 12),
                                         static_cast<float> (rect.getY() + 38),
                                         static_cast<float> (rect.getWidth() - 24),
                                         static_cast<float> (rect.getHeight() - 82));

    g.setColour (juce::Colour (0xff090b0d));
    g.fillRoundedRectangle (bezel, 3.0f);
    g.setColour (juce::Colour (0xff21262d));
    g.drawRoundedRectangle (bezel, 3.0f, 1.0f);

    // Clip Indicators at top of meter
    const bool clipL = (peakHoldL >= 0.985f);
    const bool clipR = (peakHoldR >= 0.985f);

    auto clipRectL = juce::Rectangle<float> (bezel.getX() + 6.0f, bezel.getY() + 6.0f, 18.0f, 7.0f);
    auto clipRectR = juce::Rectangle<float> (bezel.getRight() - 24.0f, bezel.getY() + 6.0f, 18.0f, 7.0f);

    g.setColour (clipL ? juce::Colour (0xffff3b30) : juce::Colour (0xff2a1215));
    g.fillRoundedRectangle (clipRectL, 1.5f);
    g.setColour (clipR ? juce::Colour (0xffff3b30) : juce::Colour (0xff2a1215));
    g.fillRoundedRectangle (clipRectR, 1.5f);

    // 32-Segment Precision Ladder
    const int numSegs = 32;
    const float ladderTop = bezel.getY() + 18.0f;
    const float ladderBot = bezel.getBottom() - 20.0f;
    const float totalH = ladderBot - ladderTop;
    const float segStep = totalH / static_cast<float> (numSegs);
    const float segH = segStep - 1.8f;

    const float dbFloor = -48.0f;
    const float dbL = juce::Decibels::gainToDecibels (smoothPeakL, dbFloor);
    const float dbR = juce::Decibels::gainToDecibels (smoothPeakR, dbFloor);
    const float holdDbL = juce::Decibels::gainToDecibels (peakHoldL, dbFloor);
    const float holdDbR = juce::Decibels::gainToDecibels (peakHoldR, dbFloor);

    const float normL = juce::jlimit (0.0f, 1.0f, (dbL - dbFloor) / (-dbFloor));
    const float normR = juce::jlimit (0.0f, 1.0f, (dbR - dbFloor) / (-dbFloor));
    const int holdSegL = static_cast<int> ( std::round (juce::jlimit (0.0f, 1.0f, (holdDbL - dbFloor) / (-dbFloor)) * (numSegs - 1)));
    const int holdSegR = static_cast<int> ( std::round (juce::jlimit (0.0f, 1.0f, (holdDbR - dbFloor) / (-dbFloor)) * (numSegs - 1)));

    for (int i = 0; i < numSegs; ++i)
    {
        const int segIdxFromBottom = (numSegs - 1) - i;
        const float segNorm = static_cast<float> (segIdxFromBottom) / static_cast<float> (numSegs - 1);
        const float yPos = ladderTop + static_cast<float> (i) * segStep;

        juce::Colour segCol = juce::Colour (0xff3fb950); // Safe Green
        if (segNorm > 0.88f)      segCol = juce::Colour (0xfff85149); // Red (-3 to 0 dB)
        else if (segNorm > 0.72f) segCol = juce::Colour (0xffd29922); // Amber (-12 to -3 dB)

        const bool litL = (normL >= segNorm && smoothPeakL > 0.0001f) || (segIdxFromBottom == holdSegL && peakHoldL > 0.001f);
        const bool litR = (normR >= segNorm && smoothPeakR > 0.0001f) || (segIdxFromBottom == holdSegR && peakHoldR > 0.001f);

        g.setColour (litL ? segCol : segCol.withAlpha (0.12f));
        g.fillRect (bezel.getX() + 6.0f, yPos, 18.0f, segH);

        g.setColour (litR ? segCol : segCol.withAlpha (0.12f));
        g.fillRect (bezel.getRight() - 24.0f, yPos, 18.0f, segH);
    }

    // Center dBFS Scale Markings
    g.setFont (makeStudioFont (9.0f, false, true));
    g.setColour (juce::Colour (0xff6e7681));
    const int marks[] = { 0, -6, -12, -24, -36, -48 };
    for (int dbMark : marks)
    {
        const float n = (static_cast<float> (dbMark) - dbFloor) / (-dbFloor);
        const float yM = ladderBot - n * totalH - 5.0f;
        g.drawText (juce::String (std::abs (dbMark)),
                    static_cast<int> (bezel.getX() + 24), static_cast<int> (yM),
                    static_cast<int> (bezel.getWidth() - 48), 10, juce::Justification::centred);
    }

    // Channel Labels
    g.setFont (makeStudioFont (10.0f, true, true));
    g.setColour (juce::Colour (0xff8b949e));
    g.drawText ("L", static_cast<int> (bezel.getX() + 6), static_cast<int> (bezel.getBottom() - 16), 18, 14, juce::Justification::centred);
    g.drawText ("R", static_cast<int> (bezel.getRight() - 24), static_cast<int> (bezel.getBottom() - 16), 18, 14, juce::Justification::centred);

    // Bottom Digital Peak Readout
    const float maxPeak = juce::jmax (peakHoldL, peakHoldR);
    const juce::String peakStr = (maxPeak <= 0.0005f) ? "-INF dB"
                                                      : juce::String (juce::Decibels::gainToDecibels (maxPeak), 1) + " dB";
    auto peakReadout = juce::Rectangle<int> (rect.getX() + 12, rect.getBottom() - 36, rect.getWidth() - 24, 20);
    g.setColour (juce::Colour (0xff090b0d));
    g.fillRoundedRectangle (peakReadout.toFloat(), 3.0f);
    g.setFont (makeStudioFont (10.5f, true, true));
    g.setColour (clipL || clipR ? juce::Colour (0xffff3b30) : juce::Colour (0xffd49a36));
    g.drawText (peakStr, peakReadout, juce::Justification::centred);
}

void VulcanEditor::paint (juce::Graphics& g)
{
    drawRackChassis (g, getLocalBounds());

    //==========================================================================
    // Top Header Bar
    //==========================================================================
    g.setColour (juce::Colour (0xff13161b));
    g.fillRoundedRectangle (headerRect.toFloat(), 4.0f);
    g.setColour (juce::Colour (0xff282e37));
    g.drawRoundedRectangle (headerRect.toFloat(), 4.0f, 1.0f);

    // Status Power Lamp + Brand Plate
    g.setColour (juce::Colour (0xff2ea043));
    g.fillEllipse (32.0f, 30.0f, 8.0f, 8.0f);

    g.setFont (makeStudioFont (21.0f, true, false));
    g.setColour (juce::Colour (0xfff0f6fc));
    g.drawText ("VULCAN", 48, 15, 115, 24, juce::Justification::centredLeft);

    g.setFont (makeStudioFont (9.5f, true, true));
    g.setColour (juce::Colour (0xff8b949e));
    g.drawText ("MODEL V-1 // PHASE & TRANSIENT MATRIX", 48, 36, 245, 14, juce::Justification::centredLeft);

    // Center Telemetry OLED Screen (Live Sample Rate & Haas Sample Delay)
    g.setColour (juce::Colour (0xff090b0e));
    g.fillRoundedRectangle (telemetryRect.toFloat(), 3.0f);
    g.setColour (juce::Colour (0xff242b35));
    g.drawRoundedRectangle (telemetryRect.toFloat(), 3.0f, 1.0f);

    const double sr = audioProcessor.getSampleRate() > 0.0 ? audioProcessor.getSampleRate() : 44100.0;
    const float rateMult = static_cast<float> (sr / 44100.0);
    const float widthVal = static_cast<float> (stereoWidthKnob.slider.getValue());
    const int dMid  = static_cast<int> (std::round (23.0f * rateMult * widthVal));
    const int dHigh = static_cast<int> (std::round (41.0f * rateMult * widthVal));
    const float msHigh = (static_cast<float> (dHigh) / static_cast<float> (sr)) * 1000.0f;

    const juce::String teleText = "SR: " + juce::String (sr / 1000.0, 1) + "kHz  |  HAAS L/R: "
                                + juce::String (dMid) + "/" + juce::String (dHigh) + " SMP ("
                                + juce::String (msHigh, 2) + " ms)  |  16-BIT TPDF";

    g.setFont (makeStudioFont (11.0f, true, true));
    g.setColour (juce::Colour (0xff58a6ff));
    g.drawText (teleText, telemetryRect, juce::Justification::centred);

    //==========================================================================
    // 4 Hardware Functional Modules
    //==========================================================================
    drawModulePanel (g, modPreRect,     "PRE-EMPHASIS & INPUT", "SEC-01");
    drawModulePanel (g, modSpatialRect, "SPATIAL PHASE MATRIX", "SEC-02");
    drawModulePanel (g, modFinalRect,   "DYNAMICS & OUTPUT",    "SEC-03");
    drawStereoMeterBridge (g, modMeterRect);

    // Subtle divider lines inside modules
    g.setColour (juce::Colour (0xff21262d));
    g.drawLine (static_cast<float> (modPreRect.getX() + 20), 248.0f,
                static_cast<float> (modPreRect.getRight() - 20), 248.0f, 1.0f);
    g.drawLine (static_cast<float> (modSpatialRect.getX() + 24), 262.0f,
                static_cast<float> (modSpatialRect.getRight() - 24), 262.0f, 1.0f);
    g.drawLine (static_cast<float> (modFinalRect.getX() + 20), 258.0f,
                static_cast<float> (modFinalRect.getRight() - 20), 258.0f, 1.0f);

    //==========================================================================
    // Knob Readouts & Titles
    //==========================================================================
    drawKnobReadout (g, inGainKnob,       formatKnobValue (inGainKnob,       "input_gain"));
    drawKnobReadout (g, eqLowKnob,        formatKnobValue (eqLowKnob,        "eq_low"));
    drawKnobReadout (g, eqMidKnob,        formatKnobValue (eqMidKnob,        "eq_mid"));
    drawKnobReadout (g, eqHighKnob,       formatKnobValue (eqHighKnob,       "eq_high"));

    drawKnobReadout (g, monoFreqKnob,     formatKnobValue (monoFreqKnob,     "mono_freq"));
    drawKnobReadout (g, stereoWidthKnob,  formatKnobValue (stereoWidthKnob,  "haas_amount"));
    drawKnobReadout (g, characterKnob,    formatKnobValue (characterKnob,    "amount"));

    drawKnobReadout (g, limiterDriveKnob, formatKnobValue (limiterDriveKnob, "limiter_drive"));
    drawKnobReadout (g, outGainKnob,      formatKnobValue (outGainKnob,      "output_gain"));
}
