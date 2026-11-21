#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
    // Automotive Metallic Sapphire / Tanzanite Blue + Billet Chrome Palette
    const juce::Colour kOuterBezelDark    (0xff040911);
    const juce::Colour kPaintDeep         (0xff081b36);
    const juce::Colour kPaintMid          (0xff0f3c78);
    const juce::Colour kPaintHighlight    (0xff2372d4);
    const juce::Colour kGlassRecess       (0xff061224);

    const juce::Colour kChromeBright      (0xffe8f1fa);
    const juce::Colour kChromeMid         (0xff8fa8c4);
    const juce::Colour kChromeDark        (0xff2c3d52);

    const juce::Colour kElectricCyan      (0xff4ce8ff);
    const juce::Colour kWarmAmber         (0xffffbe3d);
    const juce::Colour kCrimsonRed        (0xffff4d5e);
    const juce::Colour kEmeraldSafe       (0xff2ce68a);

    juce::Font makeAutoFont (float height, bool bold = false, bool mono = false)
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

    juce::TextLayout buildTooltipLayout (const juce::String& text, float maxTextWidth)
    {
        juce::AttributedString attr;
        attr.setJustification (juce::Justification::topLeft);
        attr.setWordWrap (juce::AttributedString::WordWrap::byWord);

        const int newlineIdx = text.indexOfChar ('\n');
        if (newlineIdx >= 0)
        {
            const juce::String header = text.substring (0, newlineIdx);
            const juce::String body   = text.substring (newlineIdx + 1);

            attr.append (header + "\n", makeAutoFont (11.5f, true, true), kElectricCyan);
            attr.append (body, makeAutoFont (11.0f, false, false), kChromeBright);
            attr.setLineSpacing (2.2f);
        }
        else
        {
            attr.append (text, makeAutoFont (11.0f, false, false), kChromeBright);
            attr.setLineSpacing (2.0f);
        }

        juce::TextLayout layout;
        layout.createLayout (attr, maxTextWidth);
        return layout;
    }
}

//==============================================================================
// VulcanAutomotiveLookAndFeel Implementation
//==============================================================================
VulcanAutomotiveLookAndFeel::VulcanAutomotiveLookAndFeel()
{
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff0a1f3d));
    setColour (juce::PopupMenu::textColourId, kChromeBright);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff1c5cb3));
    setColour (juce::PopupMenu::highlightedTextColourId, kElectricCyan);
    setColour (juce::ComboBox::textColourId, juce::Colour (0xffffd670));

    setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff040c18));
    setColour (juce::TextEditor::textColourId, kElectricCyan);
    setColour (juce::TextEditor::highlightColourId, juce::Colour (0xff1d63b8));
    setColour (juce::TextEditor::highlightedTextColourId, juce::Colours::white);
    setColour (juce::TextEditor::outlineColourId, kElectricCyan);
    setColour (juce::TextEditor::focusedOutlineColourId, kElectricCyan);
    setColour (juce::CaretComponent::caretColourId, kElectricCyan);
}

void VulcanAutomotiveLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                                    float sliderPos, float rotaryStartAngle,
                                                    float rotaryEndAngle, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                                static_cast<float> (width), static_cast<float> (height)).reduced (4.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const float cx = bounds.getCentreX();
    const float cy = bounds.getCentreY() - 2.0f;
    const float toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    const juce::Colour arcAccent = slider.findColour (juce::Slider::rotarySliderFillColourId);
    const bool isBipolar = static_cast<bool> (slider.getProperties().getWithDefault ("bipolar", false));

    // 1. Engraved Chrome Scale Graduations (21 precision ticks)
    const int numTicks = 21;
    for (int i = 0; i < numTicks; ++i)
    {
        const float prop = static_cast<float> (i) / static_cast<float> (numTicks - 1);
        const float ang = rotaryStartAngle + prop * (rotaryEndAngle - rotaryStartAngle);
        const bool isMajor = (i == 0 || i == 5 || i == 10 || i == 15 || i == 20);

        const float sinA = std::sin (ang);
        const float cosA = -std::cos (ang);
        const float rIn  = radius * (isMajor ? 0.87f : 0.91f);
        const float rOut = radius * 1.00f;

        g.setColour (isMajor ? kChromeBright.withAlpha (0.85f) : kChromeMid.withAlpha (0.38f));
        g.drawLine (cx + sinA * rIn, cy + cosA * rIn,
                    cx + sinA * rOut, cy + cosA * rOut,
                    isMajor ? 1.6f : 1.0f);
    }

    // 2. Recessed Luminous Value Track & Active Arc
    const float arcR = radius * 0.79f;
    juce::Path bgArc;
    bgArc.addCentredArc (cx, cy, arcR, arcR, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (juce::Colour (0xff030811).withAlpha (0.85f));
    g.strokePath (bgArc, juce::PathStrokeType (3.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float startAng = isBipolar ? (rotaryStartAngle + rotaryEndAngle) * 0.5f : rotaryStartAngle;
    if (std::abs (toAngle - startAng) > 0.01f)
    {
        juce::Path valArc;
        valArc.addCentredArc (cx, cy, arcR, arcR, 0.0f,
                              juce::jmin (startAng, toAngle),
                              juce::jmax (startAng, toAngle), true);

        // Soft outer LED halo
        g.setColour (arcAccent.withAlpha (0.28f));
        g.strokePath (valArc, juce::PathStrokeType (6.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Crisp core LED arc
        g.setColour (arcAccent);
        g.strokePath (valArc, juce::PathStrokeType (3.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // 3. Multi-Layered 3D Cast Shadow & Contact Ambient Occlusion
    const float knobR = radius * 0.68f;
    g.setColour (juce::Colours::black.withAlpha (0.48f));
    g.fillEllipse (cx - knobR + 2.5f, cy - knobR + 7.5f, knobR * 2.0f, knobR * 2.0f);

    g.setColour (juce::Colours::black.withAlpha (0.72f));
    g.fillEllipse (cx - knobR - 0.5f, cy - knobR + 3.5f, (knobR + 0.5f) * 2.0f, (knobR + 0.5f) * 2.0f);

    // 4. 3D Extruded Cylindrical Skirt Wall (visible physical height below the knob)
    const float skirtExtrudeY = 4.2f;
    juce::ColourGradient cylinderWallGrad (juce::Colour (0xff0c1420), cx - knobR, cy + skirtExtrudeY,
                                           juce::Colour (0xff364c66), cx, cy + skirtExtrudeY, true);
    g.setGradientFill (cylinderWallGrad);
    g.fillEllipse (cx - knobR, cy - knobR + skirtExtrudeY, knobR * 2.0f, knobR * 2.0f);

    g.setColour (juce::Colour (0xff04080f));
    g.drawEllipse (cx - knobR, cy - knobR + skirtExtrudeY, knobR * 2.0f, knobR * 2.0f, 1.2f);

    // 5. 3D CNC Diamond-Knurled Outer Grip Ring (24 sculpted metallic teeth)
    const int numTeeth = 24;
    const float toothOuterR = knobR;
    const float toothInnerR = knobR * 0.86f;

    juce::ColourGradient skirtBaseGrad (juce::Colour (0xff496280), cx - knobR * 0.5f, cy - knobR * 0.7f,
                                        juce::Colour (0xff101926), cx + knobR * 0.5f, cy + knobR * 0.8f, false);
    g.setGradientFill (skirtBaseGrad);
    g.fillEllipse (cx - toothOuterR, cy - toothOuterR, toothOuterR * 2.0f, toothOuterR * 2.0f);

    for (int t = 0; t < numTeeth; ++t)
    {
        const float tAng = toAngle + (static_cast<float> (t) / static_cast<float> (numTeeth)) * juce::MathConstants<float>::twoPi;
        const float sA = std::sin (tAng);
        const float cA = -std::cos (tAng);

        // Specular highlight ridge on each knurl tooth
        g.setColour (kChromeBright.withAlpha (0.24f));
        g.drawLine (cx + sA * toothInnerR - 0.5f, cy + cA * toothInnerR - 0.5f,
                    cx + sA * toothOuterR - 0.5f, cy + cA * toothOuterR - 0.5f, 1.3f);

        // Deep shadow groove next to ridge
        g.setColour (juce::Colour (0xff050911).withAlpha (0.85f));
        g.drawLine (cx + sA * toothInnerR + 0.7f, cy + cA * toothInnerR + 0.7f,
                    cx + sA * toothOuterR + 0.7f, cy + cA * toothOuterR + 0.7f, 1.4f);
    }

    // 6. 45-Degree Machined Chrome Chamfer Ring
    const float bevelR = knobR * 0.84f;
    juce::ColourGradient bevelGrad (kChromeBright, cx - bevelR * 0.65f, cy - bevelR * 0.65f,
                                    juce::Colour (0xff121c2b), cx + bevelR * 0.65f, cy + bevelR * 0.65f, false);
    g.setGradientFill (bevelGrad);
    g.fillEllipse (cx - bevelR, cy - bevelR, bevelR * 2.0f, bevelR * 2.0f);

    // 7. Anisotropic Lathe-Turned Metal Top Cap (Conical 3D Sheen)
    const float capR = knobR * 0.75f;
    juce::ColourGradient capBaseGrad (juce::Colour (0xff3a506b), cx - capR * 0.45f, cy - capR * 0.65f,
                                      juce::Colour (0xff131e2e), cx + capR * 0.55f, cy + capR * 0.70f, false);
    g.setGradientFill (capBaseGrad);
    g.fillEllipse (cx - capR, cy - capR, capR * 2.0f, capR * 2.0f);

    // Conical specular light wedges (gives authentic turned-aluminum 3D reflection)
    {
        juce::Graphics::ScopedSaveState state (g);
        juce::Path clipCap;
        clipCap.addEllipse (cx - capR, cy - capR, capR * 2.0f, capR * 2.0f);
        g.reduceClipRegion (clipCap);

        const float wedgeSpan = 0.52f;
        const float sheenAng1 = -0.65f + (toAngle * 0.18f);
        const float sheenAng2 = sheenAng1 + juce::MathConstants<float>::pi;

        for (float baseA : { sheenAng1, sheenAng2 })
        {
            juce::Path wedge;
            wedge.startNewSubPath (cx, cy);
            wedge.lineTo (cx + std::sin (baseA - wedgeSpan) * capR * 1.3f, cy - std::cos (baseA - wedgeSpan) * capR * 1.3f);
            wedge.lineTo (cx + std::sin (baseA + wedgeSpan) * capR * 1.3f, cy - std::cos (baseA + wedgeSpan) * capR * 1.3f);
            wedge.closeSubPath();

            g.setColour (kChromeBright.withAlpha (0.14f));
            g.fillPath (wedge);
        }

        // Subtle concentric lathe micro-rings
        g.setColour (juce::Colours::white.withAlpha (0.045f));
        g.drawEllipse (cx - capR * 0.62f, cy - capR * 0.62f, capR * 1.24f, capR * 1.24f, 0.8f);
        g.drawEllipse (cx - capR * 0.36f, cy - capR * 0.36f, capR * 0.72f, capR * 0.72f, 0.8f);
    }

    // Crisp rim highlight on top cap
    g.setColour (kChromeBright.withAlpha (0.35f));
    g.drawEllipse (cx - capR + 0.5f, cy - capR + 0.5f, (capR - 0.5f) * 2.0f, (capR - 0.5f) * 2.0f, 1.0f);

    // 8. Milled 3D Indicator Groove & Luminous Pointer Bar
    const float pSin = std::sin (toAngle);
    const float pCos = -std::cos (toAngle);
    const float pIn  = capR * 0.24f;
    const float pOut = capR * 0.90f;

    // Dark recessed groove shadow
    g.setColour (juce::Colour (0xff04080e));
    g.drawLine (cx + pSin * pIn + 0.9f, cy + pCos * pIn + 1.1f,
                cx + pSin * pOut + 0.9f, cy + pCos * pOut + 1.1f, 3.8f);

    // Glowing indicator core
    juce::Path pointerLine;
    pointerLine.startNewSubPath (cx + pSin * pIn, cy + pCos * pIn);
    pointerLine.lineTo (cx + pSin * pOut, cy + pCos * pOut);

    g.setColour (arcAccent.withAlpha (0.45f));
    g.strokePath (pointerLine, juce::PathStrokeType (4.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour (juce::Colours::white);
    g.strokePath (pointerLine, juce::PathStrokeType (2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

juce::Label* VulcanAutomotiveLookAndFeel::createSliderTextBox (juce::Slider& slider)
{
    auto* l = juce::LookAndFeel_V4::createSliderTextBox (slider);
    l->setFont (makeAutoFont (11.5f, true, true));
    l->setJustificationType (juce::Justification::centred);
    // Single-click OR double-click opens immediate keyboard numeric editing
    l->setEditable (true, true, false);
    l->setColour (juce::Label::textColourId, kChromeBright);
    l->setColour (juce::Label::textWhenEditingColourId, kElectricCyan);
    l->setColour (juce::Label::backgroundWhenEditingColourId, juce::Colour (0xff030914));
    l->setColour (juce::Label::outlineWhenEditingColourId, kElectricCyan);
    return l;
}

void VulcanAutomotiveLookAndFeel::drawLabel (juce::Graphics& g, juce::Label& label)
{
    if (label.isBeingEdited())
        return;

    const auto bounds = label.getLocalBounds().toFloat().reduced (1.0f);

    // Smoked-Glass & Chrome Digital Readout Pill
    g.setColour (juce::Colour (0xff040b16).withAlpha (0.88f));
    g.fillRoundedRectangle (bounds, 4.0f);

    g.setColour (kChromeMid.withAlpha (0.38f));
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);

    // Subtle top glass reflection
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.fillRoundedRectangle (bounds.getX() + 1.5f, bounds.getY() + 1.0f, bounds.getWidth() - 3.0f, bounds.getHeight() * 0.42f, 2.5f);

    g.setFont (label.getFont());
    g.setColour (label.findColour (juce::Label::textColourId));
    g.drawText (label.getText(), label.getLocalBounds(), label.getJustificationType(), false);
}

void VulcanAutomotiveLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                                int, int, int, int, juce::ComboBox&)
{
    const auto box = juce::Rectangle<float> (0.0f, 0.0f, static_cast<float> (width), static_cast<float> (height));

    // Machined Dark Chrome & Glass Selector Bezel
    g.setColour (juce::Colour (0xff040b16));
    g.fillRoundedRectangle (box, 5.0f);

    juce::ColourGradient rimGrad (kChromeBright.withAlpha (0.65f), 0.0f, 0.0f,
                                  kChromeDark, 0.0f, static_cast<float> (height), false);
    g.setGradientFill (rimGrad);
    g.drawRoundedRectangle (box.reduced (0.5f), 5.0f, 1.2f);

    // Chrome Dropdown Chevron
    juce::Path chevron;
    const float ax = static_cast<float> (width) - 16.0f;
    const float ay = static_cast<float> (height) * 0.5f;
    chevron.startNewSubPath (ax - 4.5f, ay - 2.0f);
    chevron.lineTo (ax, ay + 2.5f);
    chevron.lineTo (ax + 4.5f, ay - 2.0f);

    g.setColour (kElectricCyan);
    g.strokePath (chevron, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void VulcanAutomotiveLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (10, 1, box.getWidth() - 28, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
    label.setColour (juce::Label::textColourId, kChromeBright);
}

juce::Font VulcanAutomotiveLookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return makeAutoFont (12.0f, true, false);
}

juce::Font VulcanAutomotiveLookAndFeel::getPopupMenuFont()
{
    return makeAutoFont (12.0f, true, false);
}

juce::Rectangle<int> VulcanAutomotiveLookAndFeel::getTooltipBounds (const juce::String& tipText,
                                                                    juce::Point<int> screenPos,
                                                                    juce::Rectangle<int> parentArea)
{
    const float maxTextW = 296.0f;
    const auto layout = buildTooltipLayout (tipText, maxTextW);

    const int boxW = static_cast<int> (std::ceil (layout.getWidth())) + 26;
    const int boxH = static_cast<int> (std::ceil (layout.getHeight())) + 18;

    int x = screenPos.x + 14;
    int y = screenPos.y + 18;

    if (x + boxW > parentArea.getRight() - 10)
        x = screenPos.x - boxW - 10;

    if (y + boxH > parentArea.getBottom() - 10)
        y = screenPos.y - boxH - 12;

    x = juce::jlimit (parentArea.getX() + 10, juce::jmax (parentArea.getX() + 10, parentArea.getRight() - boxW - 10), x);
    y = juce::jlimit (parentArea.getY() + 10, juce::jmax (parentArea.getY() + 10, parentArea.getBottom() - boxH - 10), y);

    return { x, y, boxW, boxH };
}

void VulcanAutomotiveLookAndFeel::drawTooltip (juce::Graphics& g, const juce::String& text, int width, int height)
{
    const auto bounds = juce::Rectangle<float> (0.0f, 0.0f, static_cast<float> (width), static_cast<float> (height));

    // Smoked-Sapphire & Obsidian HUD Card
    g.setColour (juce::Colour (0xf4050e1c));
    g.fillRoundedRectangle (bounds, 6.0f);

    // Subtle top specular sheen
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.fillRoundedRectangle (bounds.getX() + 1.0f, bounds.getY() + 1.0f, bounds.getWidth() - 2.0f, bounds.getHeight() * 0.32f, 5.0f);

    // Left Electric-Cyan Accent Stripe
    g.setColour (kElectricCyan);
    g.fillRoundedRectangle (bounds.getX() + 2.0f, bounds.getY() + 7.0f, 3.0f, bounds.getHeight() - 14.0f, 1.5f);

    // Chrome & Cyan Frame Border
    g.setColour (kElectricCyan.withAlpha (0.58f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.1f);

    const float maxTextW = juce::jmax (60.0f, static_cast<float> (width) - 26.0f);
    const auto layout = buildTooltipLayout (text, maxTextW);
    layout.draw (g, juce::Rectangle<float> (14.0f, 9.0f, maxTextW, static_cast<float> (height) - 18.0f));
}

//==============================================================================
// VulcanEditor Implementation
//==============================================================================
VulcanEditor::VulcanEditor (VulcanProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&automotiveLnf);

    // Top Bank: 5 Core Controls (with Keyboard Entry + Quick-Manual Tooltips)
    setupKnob (inGainKnob, "input_gain", "INPUT", "TRIM",
               "INPUT TRIM (-24 to +24 dB)\nAdjusts incoming gain before Pre-EQ and saturation. Aim for peaks around -6 dBFS on the Input Meter for optimal headroom.\n[Click readout to type | Double-click knob to reset]",
               kWarmAmber, ValueDisplayMode::Decibels, true, false, 0.0);

    setupKnob (monoFreqKnob, "mono_freq", "MONO LOCK", "X-OVER",
               "MONO LOCK CROSSOVER (80 - 800 Hz)\nSums frequencies below this cutoff to pure mono while keeping upper bands wide. Tightens sub-bass & 808 punch with zero phase smear. Sweet spot: 150 - 250 Hz.",
               kChromeBright, ValueDisplayMode::FrequencyHz, false, false, 250.0);

    setupKnob (stereoWidthKnob, "haas_amount", "STEREO WIDTH", "HAAS L/R",
               "STEREO WIDTH // HAAS PHASE MATRIX (0 - 100%)\nIntroduces sample-rate-compensated L/R micro-delays in Mid & High bands to expand width and break static phase symmetry. Default: 46% (11 / 19 smp at 44.1 kHz).",
               kElectricCyan, ValueDisplayMode::Percent100From1, false, true, 0.46);

    setupKnob (characterKnob, "amount", "HARMONICS", "SATURATE",
               "HARMONICS // 4X OVERSAMPLED SATURATION (0 - 100%)\nEngages 3-band glue compression and asymmetric tanh tube/tape saturation with 4x polyphase oversampling. Adds analog density and harmonic warmth.",
               kWarmAmber, ValueDisplayMode::Percent100From2, false, false, 0.0);

    setupKnob (outGainKnob, "output_gain", "OUTPUT", "LEVEL",
               "OUTPUT TRIM (-24 to +24 dB)\nFinal post-limiter gain stage prior to 16-bit TPDF dithering. Use to level-match processed vs. bypassed audio or set streaming headroom.",
               kChromeBright, ValueDisplayMode::Decibels, true, false, 0.0);

    // Bottom Bank: 4 Pre-EQ & Limiter Controls
    setupKnob (eqLowKnob, "eq_low", "LOW", "100 Hz",
               "PRE-EQ LOW SHELF // 100 Hz (-12 to +12 dB)\nSmooth 100 Hz low-shelf filter feeding the crossover and saturation stage. Shapes sub weight and low-end punch before compression.",
               kElectricCyan, ValueDisplayMode::Decibels, true, false, 0.0);

    setupKnob (eqMidKnob, "eq_mid", "MID", "2.5 kHz",
               "PRE-EQ MID BELL // 2.5 kHz (-12 to +12 dB)\nMusical presence bell (Q = 1.0) positioned pre-saturation. Enhances vocal articulation, synth bite, or tames upper-mid harshness.",
               kElectricCyan, ValueDisplayMode::Decibels, true, false, 0.0);

    setupKnob (eqHighKnob, "eq_high", "HIGH", "10 kHz",
               "PRE-EQ HIGH SHELF // 10 kHz (-12 to +12 dB)\n10 kHz air shelf ahead of the harmonic engine. Boosts high-frequency sheen or softens brittle digital transients.",
               kElectricCyan, ValueDisplayMode::Decibels, true, false, 0.0);

    setupKnob (limiterDriveKnob, "limiter_drive", "CEILING DRV", "LIMITER",
               "CEILING DRIVE // BRICKWALL LIMITER (0 to +18 dB)\nDrives the mix into a fast transparent brickwall limiter (-0.1 dBFS ceiling, 1 ms attack / 50 ms release) for commercial loudness.",
               kWarmAmber, ValueDisplayMode::Decibels, false, false, 0.0);

    // Preset Selector
    addAndMakeVisible (presetSelector);
    presetSelector.setTooltip ("PRESET BANK\nSelect a factory mastering curve. An asterisk (*) indicates manual edits - re-select the same preset anytime to restore its original settings.");
    for (int i = 0; i < audioProcessor.getNumPrograms(); ++i)
        presetSelector.addItem (audioProcessor.getProgramName (i), i + 1);

    syncPresetDisplay();
    presetSelector.onChange = [this]
    {
        const int idx = presetSelector.getSelectedId() - 1;
        if (idx >= 0)
        {
            audioProcessor.setCurrentProgram (idx);
            syncPresetDisplay();
        }
    };

    setSize (940, 470);
    startTimerHz (30);
}

VulcanEditor::~VulcanEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

juce::String VulcanEditor::getTooltip()
{
    const auto mousePos = getMouseXYRelative();

    if (inMeterRect.contains (mousePos))
        return "INPUT PEAK METER (dBFS)\nDisplays post-Input-Trim stereo peak levels with 32-segment resolution, peak hold, and 0 dBFS clip indicators.";

    if (outMeterRect.contains (mousePos))
        return "OUTPUT MASTER METER (dBFS)\nDisplays final post-limiter, post-16-bit TPDF dithered stereo output levels and peak hold ceiling.";

    if (telemetryRect.contains (mousePos))
        return "LIVE HAAS TELEMETRY\nReal-time readout of active L/R sample offsets (Mid / High bands), millisecond delay, and DAW sample rate.";

    for (const auto& screwPos : screwCenters)
    {
        if (mousePos.toFloat().getDistanceFrom (screwPos) <= 16.0f)
            return "CNC CHASSIS FASTENER\nClick to spin the billet stainless screw.";
    }

    return {};
}

void VulcanEditor::setupKnob (KnobControl& kc, const juce::String& paramID,
                              const juce::String& titleText, const juce::String& subText,
                              const juce::String& manualTooltip,
                              juce::Colour arcCol, ValueDisplayMode mode,
                              bool bipolar, bool hero, double defaultVal)
{
    kc.title = titleText;
    kc.subLabel = subText;
    kc.arcColour = arcCol;
    kc.displayMode = mode;
    kc.isBipolar = bipolar;
    kc.isHero = hero;

    kc.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    kc.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 78, 20);
    kc.slider.setWantsKeyboardFocus (true);
    kc.slider.setDoubleClickReturnValue (true, defaultVal);
    kc.slider.setTooltip (manualTooltip);
    kc.slider.setColour (juce::Slider::rotarySliderFillColourId, arcCol);
    kc.slider.setColour (juce::Slider::textBoxTextColourId, hero ? kElectricCyan : kChromeBright);
    kc.slider.getProperties().set ("bipolar", bipolar);

    // Custom Bidirectional Keyboard String <-> Parameter Conversion
    kc.slider.textFromValueFunction = [mode, bipolar] (double val) -> juce::String
    {
        switch (mode)
        {
            case ValueDisplayMode::FrequencyHz:
                return juce::String (static_cast<int> (std::round (val))) + " Hz";

            case ValueDisplayMode::Percent100From1:
                return juce::String (static_cast<int> (std::round (val * 100.0))) + " %";

            case ValueDisplayMode::Percent100From2:
                return juce::String (static_cast<int> (std::round (val * 50.0))) + " %";

            case ValueDisplayMode::Decibels:
            default:
            {
                const juce::String prefix = (bipolar && val > 0.05) ? "+" : "";
                return prefix + juce::String (val, 1) + " dB";
            }
        }
    };

    kc.slider.valueFromTextFunction = [mode] (const juce::String& text) -> double
    {
        const juce::String cleaned = text.retainCharacters ("0123456789.-");
        const double num = cleaned.getDoubleValue();

        switch (mode)
        {
            case ValueDisplayMode::Percent100From1:
                return juce::jlimit (0.0, 100.0, num) / 100.0;

            case ValueDisplayMode::Percent100From2:
                return juce::jlimit (0.0, 100.0, num) / 50.0;

            case ValueDisplayMode::FrequencyHz:
            case ValueDisplayMode::Decibels:
            default:
                return num;
        }
    };

    kc.slider.onValueChange = [this]
    {
        syncPresetDisplay();
        repaint (telemetryRect);
    };

    addAndMakeVisible (kc.slider);

    // Propagate quick-manual tooltip to the child numeric readout label as well
    for (int i = 0; i < kc.slider.getNumChildComponents(); ++i)
    {
        if (auto* tipClient = dynamic_cast<juce::SettableTooltipClient*> (kc.slider.getChildComponent (i)))
            tipClient->setTooltip (manualTooltip);
    }

    attachments.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, paramID, kc.slider));
}

void VulcanEditor::mouseDown (const juce::MouseEvent& event)
{
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

void VulcanEditor::syncPresetDisplay()
{
    const int progIdx = audioProcessor.getCurrentProgram();
    const juce::String progName = audioProcessor.getProgramName (progIdx);

    if (audioProcessor.isPresetModified())
    {
        const juce::String dirtyName = progName + " *";
        if (presetSelector.getSelectedId() != 0 || presetSelector.getTextWhenNothingSelected() != dirtyName)
        {
            presetSelector.setTextWhenNothingSelected (dirtyName);
            presetSelector.setSelectedId (0, juce::dontSendNotification);
        }
    }
    else
    {
        const int expectedId = progIdx + 1;
        if (presetSelector.getSelectedId() != expectedId)
            presetSelector.setSelectedId (expectedId, juce::dontSendNotification);
    }
}

void VulcanEditor::timerCallback()
{
    syncPresetDisplay();

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

    inMeter.update (audioProcessor.inPeakL.load(), audioProcessor.inPeakR.load());
    outMeter.update (audioProcessor.peakL.load(),  audioProcessor.peakR.load());

    if (screwMoving)
        repaint();
    else
    {
        repaint (inMeterRect);
        repaint (outMeterRect);
        repaint (telemetryRect);
    }
}

void VulcanEditor::rebuildMetallicChassisCache()
{
    const int w = getWidth();
    const int h = getHeight();
    if (w <= 0 || h <= 0)
        return;

    chassisCache = juce::Image (juce::Image::ARGB, w, h, true);
    juce::Graphics g (chassisCache);

    // 1. Deep Obsidian Outer Chassis Frame
    g.fillAll (kOuterBezelDark);

    const auto plate = outerPlateRect.toFloat();

    // 2. Automotive Sapphire / Tanzanite Metallic Multi-Stop Base Coat
    juce::ColourGradient autoBaseGrad (kPaintDeep, 0.0f, plate.getY(),
                                       kPaintMid, plate.getWidth() * 0.45f, plate.getHeight() * 0.42f, false);
    autoBaseGrad.addColour (0.35, kPaintHighlight);
    autoBaseGrad.addColour (0.68, kPaintMid);
    autoBaseGrad.addColour (1.00, kPaintDeep);

    g.setGradientFill (autoBaseGrad);
    g.fillRoundedRectangle (plate, 12.0f);

    // 3. Curved Clearcoat Specular Sweep (Automotive Body Reflection)
    {
        juce::Graphics::ScopedSaveState state (g);
        juce::Path plateClip;
        plateClip.addRoundedRectangle (plate, 12.0f);
        g.reduceClipRegion (plateClip);

        juce::ColourGradient clearcoatSheen (juce::Colour (0xff6bb8ff).withAlpha (0.20f), plate.getX(), plate.getY() + 35.0f,
                                             juce::Colours::transparentBlack, plate.getRight(), plate.getBottom() - 60.0f, false);
        clearcoatSheen.addColour (0.28, juce::Colour (0xff8fd0ff).withAlpha (0.24f));
        clearcoatSheen.addColour (0.52, juce::Colours::transparentBlack);
        g.setGradientFill (clearcoatSheen);
        g.fillRect (plate);

        // 4. High-Density Metallic Aluminum & Mica Flakes (Directional Light-Modulated Sparkle)
        juce::Random flakeRng (20261121);
        const float lightCenterX = plate.getWidth() * 0.42f;
        const float lightCenterY = plate.getHeight() * 0.34f;

        for (int i = 0; i < 14000; ++i)
        {
            const float fx = plate.getX() + flakeRng.nextFloat() * plate.getWidth();
            const float fy = plate.getY() + flakeRng.nextFloat() * plate.getHeight();

            const float dx = (fx - lightCenterX) / plate.getWidth();
            const float dy = (fy - lightCenterY) / plate.getHeight();
            const float dist = std::sqrt (dx * dx + dy * dy);
            const float lightCatch = juce::jlimit (0.0f, 1.0f, 1.0f - dist * 1.35f);

            const float roll = flakeRng.nextFloat();
            if (roll > 0.42f)
            {
                // Specular silver-blue metallic flake
                const float alpha = (0.04f + 0.16f * lightCatch) * flakeRng.nextFloat();
                g.setColour (juce::Colour (0xffa8dcff).withAlpha (alpha));
            }
            else
            {
                // Deep sapphire micro-grain shadow
                g.setColour (juce::Colour (0xff030c1c).withAlpha (0.07f * flakeRng.nextFloat()));
            }

            const float flakeSize = (roll > 0.88f) ? 1.6f : 1.1f;
            g.fillRect (fx, fy, flakeSize, flakeSize);
        }
    }

    // 5. CNC-Machined Chrome Outer Bevel & Specular Edge Trim
    juce::ColourGradient chromeTrimGrad (kChromeBright.withAlpha (0.85f), 0.0f, plate.getY(),
                                         kChromeDark, 0.0f, plate.getBottom(), false);
    chromeTrimGrad.addColour (0.50, kChromeMid.withAlpha (0.55f));
    g.setGradientFill (chromeTrimGrad);
    g.drawRoundedRectangle (plate.reduced (0.75f), 11.5f, 1.8f);

    // 6. Smoked-Glass & Chrome Control Sub-Panels
    auto drawRecessedBay = [&] (juce::Rectangle<int> r)
    {
        const auto rf = r.toFloat();
        g.setColour (kGlassRecess.withAlpha (0.74f));
        g.fillRoundedRectangle (rf, 8.0f);

        // Inner top shadow
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.drawRoundedRectangle (rf, 8.0f, 1.4f);

        // Bottom chrome catch-light
        g.setColour (kChromeBright.withAlpha (0.18f));
        g.drawRoundedRectangle (rf.reduced (1.0f), 7.5f, 0.9f);
    };

    drawRecessedBay (topBankRect);
    drawRecessedBay (botBankRect);

    // Machined Chrome Divider between EQ Trio and Ceiling Limiter in Bottom Bank
    const float divX = 632.0f;
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawLine (divX - 1.0f, static_cast<float> (botBankRect.getY() + 14),
                divX - 1.0f, static_cast<float> (botBankRect.getBottom() - 14), 1.2f);
    g.setColour (kChromeBright.withAlpha (0.22f));
    g.drawLine (divX + 0.5f, static_cast<float> (botBankRect.getY() + 14),
                divX + 0.5f, static_cast<float> (botBankRect.getBottom() - 14), 1.0f);
}

void VulcanEditor::resized()
{
    outerPlateRect = { 6, 6, getWidth() - 12, getHeight() - 12 };
    headerRect     = { 24, 18, getWidth() - 48, 50 };
    telemetryRect  = { 386, 28, 316, 30 };
    presetSelector.setBounds (718, 28, 198, 30);

    inMeterRect    = { 24,  80, 72, 366 };
    outMeterRect   = { 844, 80, 72, 366 };

    topBankRect    = { 108, 80,  724, 188 };
    botBankRect    = { 108, 280, 724, 166 };

    // 4 Corner CNC Screws
    screwCenters[0] = { 17.0f, 17.0f };
    screwCenters[1] = { static_cast<float> (getWidth()) - 17.0f, 17.0f };
    screwCenters[2] = { 17.0f, static_cast<float> (getHeight()) - 17.0f };
    screwCenters[3] = { static_cast<float> (getWidth()) - 17.0f, static_cast<float> (getHeight()) - 17.0f };

    auto placeKnob = [] (KnobControl& kc, int cx, int cy, int dialDiam)
    {
        const int textBoxH = 20;
        const int totalW = juce::jmax (dialDiam + 8, 84);
        const int totalH = dialDiam + textBoxH + 4;

        kc.dialBounds   = { cx - dialDiam / 2, cy - dialDiam / 2, dialDiam, dialDiam };
        kc.sliderBounds = { cx - totalW / 2,   cy - dialDiam / 2, totalW,   totalH };
        kc.slider.setBounds (kc.sliderBounds);
    };

    // Top Bank: 5 Controls (centered at y = 172)
    const int topY = 172;
    placeKnob (inGainKnob,      178, topY,     84);
    placeKnob (monoFreqKnob,    322, topY,     84);
    placeKnob (stereoWidthKnob, 470, topY - 2, 102); // Hero 3D Dial
    placeKnob (characterKnob,   618, topY,     84);
    placeKnob (outGainKnob,     762, topY,     84);

    // Bottom Bank: 3 Pre-EQ Knobs + Limiter Drive (centered at y = 362)
    const int botY = 362;
    placeKnob (eqLowKnob,        198, botY, 74);
    placeKnob (eqMidKnob,        364, botY, 74);
    placeKnob (eqHighKnob,       530, botY, 74);
    placeKnob (limiterDriveKnob, 732, botY, 78);

    rebuildMetallicChassisCache();
}

void VulcanEditor::drawChromeScrew (juce::Graphics& g, juce::Point<float> c, float r, float angleRad)
{
    g.setColour (juce::Colours::black.withAlpha (0.75f));
    g.fillEllipse (c.x - r + 1.0f, c.y - r + 1.5f, r * 2.0f, r * 2.0f);

    juce::ColourGradient metalGrad (kChromeBright, c.x - r * 0.55f, c.y - r * 0.55f,
                                    kChromeDark,   c.x + r * 0.65f, c.y + r * 0.65f, false);
    g.setGradientFill (metalGrad);
    g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);

    g.setColour (juce::Colour (0xff050911));
    g.drawEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f, 1.0f);

    const float s  = std::sin (angleRad) * (r * 0.58f);
    const float co = std::cos (angleRad) * (r * 0.58f);

    g.setColour (juce::Colour (0xff152233));
    g.drawLine (c.x - co, c.y - s, c.x + co, c.y + s, 1.6f);
    g.drawLine (c.x + s, c.y - co, c.x - s, c.y + co, 1.6f);
}

void VulcanEditor::drawSapphireJewel (juce::Graphics& g, juce::Point<float> c, float r)
{
    juce::ColourGradient glow (kElectricCyan.withAlpha (0.48f), c.x, c.y,
                               kElectricCyan.withAlpha (0.0f),  c.x + r * 2.6f, c.y, true);
    g.setGradientFill (glow);
    g.fillEllipse (c.x - r * 2.6f, c.y - r * 2.6f, r * 5.2f, r * 5.2f);

    g.setColour (kChromeBright);
    g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
    g.setColour (kOuterBezelDark);
    g.drawEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f, 1.2f);

    const float lensR = r * 0.74f;
    juce::ColourGradient jewelGrad (juce::Colour (0xffe6fbff), c.x - lensR * 0.3f, c.y - lensR * 0.3f,
                                    juce::Colour (0xff0066cc), c.x + lensR * 0.5f, c.y + lensR * 0.5f, true);
    g.setGradientFill (jewelGrad);
    g.fillEllipse (c.x - lensR, c.y - lensR, lensR * 2.0f, lensR * 2.0f);

    g.setColour (juce::Colours::white.withAlpha (0.92f));
    g.fillEllipse (c.x - lensR * 0.45f, c.y - lensR * 0.45f, 2.5f, 2.5f);
}

void VulcanEditor::drawMetallicEmblem (juce::Graphics& g, juce::Rectangle<float> area)
{
    // 3D Chrome Automotive Badge
    juce::ColourGradient badgeGrad (kChromeBright, area.getX(), area.getY(),
                                    kChromeDark,   area.getX(), area.getBottom(), false);
    g.setGradientFill (badgeGrad);
    g.drawRoundedRectangle (area, 6.0f, 1.8f);

    juce::Path vGlyph;
    const float cx = area.getCentreX();
    const float cy = area.getCentreY();
    vGlyph.startNewSubPath (cx - 7.5f, cy - 6.0f);
    vGlyph.lineTo (cx, cy + 7.5f);
    vGlyph.lineTo (cx + 7.5f, cy - 6.0f);

    g.setColour (kElectricCyan);
    g.strokePath (vGlyph, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void VulcanEditor::drawKnobLabels (juce::Graphics& g, const KnobControl& kc)
{
    const int labelW = kc.dialBounds.getWidth() + 30;
    const int labelX = kc.dialBounds.getCentreX() - labelW / 2;

    g.setFont (makeAutoFont (kc.isHero ? 12.0f : 11.0f, true, false));
    g.setColour (kChromeBright);
    g.drawText (kc.title, labelX, kc.dialBounds.getY() - 29, labelW, 15, juce::Justification::centred);

    g.setFont (makeAutoFont (9.0f, true, true));
    g.setColour (kc.isHero ? kElectricCyan : kChromeMid);
    g.drawText (kc.subLabel, labelX, kc.dialBounds.getY() - 15, labelW, 12, juce::Justification::centred);
}

void VulcanEditor::drawStereoMeterBridge (juce::Graphics& g, juce::Rectangle<int> rect,
                                          const juce::String& headerLabel, const MeterBallistics& m)
{
    const auto fRect = rect.toFloat();

    // Smoked-Glass & Chrome Meter Housing
    g.setColour (kGlassRecess.withAlpha (0.85f));
    g.fillRoundedRectangle (fRect, 8.0f);

    g.setColour (juce::Colours::black.withAlpha (0.65f));
    g.drawRoundedRectangle (fRect, 8.0f, 1.4f);
    g.setColour (kChromeBright.withAlpha (0.22f));
    g.drawRoundedRectangle (fRect.reduced (1.0f), 7.5f, 0.9f);

    // Header Title (INPUT / OUTPUT)
    g.setFont (makeAutoFont (10.5f, true, false));
    g.setColour (kChromeBright);
    g.drawText (headerLabel, rect.getX(), rect.getY() + 7, rect.getWidth(), 14, juce::Justification::centred);

    const float barW = 11.0f;
    const float barTop = fRect.getY() + 36.0f;
    const float barBot = fRect.getBottom() - 38.0f;
    const float barH = barBot - barTop;

    const float leftBarX  = fRect.getX() + 12.0f;
    const float rightBarX = fRect.getRight() - 12.0f - barW;

    // Clip LEDs
    const bool clipL = (m.holdL >= 0.985f);
    const bool clipR = (m.holdR >= 0.985f);
    g.setColour (clipL ? kCrimsonRed : juce::Colour (0xff1a0a10));
    g.fillRoundedRectangle (leftBarX, fRect.getY() + 25.0f, barW, 5.5f, 2.0f);
    g.setColour (clipR ? kCrimsonRed : juce::Colour (0xff1a0a10));
    g.fillRoundedRectangle (rightBarX, fRect.getY() + 25.0f, barW, 5.5f, 2.0f);

    // Recessed Ladder Wells
    g.setColour (juce::Colour (0xff03070e));
    g.fillRoundedRectangle (leftBarX, barTop, barW, barH, 2.5f);
    g.fillRoundedRectangle (rightBarX, barTop, barW, barH, 2.5f);

    // 32-Segment Precision LED Ladder
    const int numSegs = 32;
    const float segStep = barH / static_cast<float> (numSegs);
    const float segH = segStep - 1.6f;

    const float dbFloor = -48.0f;
    const float dbL = juce::Decibels::gainToDecibels (m.smoothL, dbFloor);
    const float dbR = juce::Decibels::gainToDecibels (m.smoothR, dbFloor);
    const float holdDbL = juce::Decibels::gainToDecibels (m.holdL, dbFloor);
    const float holdDbR = juce::Decibels::gainToDecibels (m.holdR, dbFloor);

    const float normL = juce::jlimit (0.0f, 1.0f, (dbL - dbFloor) / (-dbFloor));
    const float normR = juce::jlimit (0.0f, 1.0f, (dbR - dbFloor) / (-dbFloor));
    const int holdSegL = static_cast<int> (std::round (juce::jlimit (0.0f, 1.0f, (holdDbL - dbFloor) / (-dbFloor)) * (numSegs - 1)));
    const int holdSegR = static_cast<int> (std::round (juce::jlimit (0.0f, 1.0f, (holdDbR - dbFloor) / (-dbFloor)) * (numSegs - 1)));

    for (int i = 0; i < numSegs; ++i)
    {
        const int segFromBot = (numSegs - 1) - i;
        const float segNorm = static_cast<float> (segFromBot) / static_cast<float> (numSegs - 1);
        const float yPos = barTop + static_cast<float> (i) * segStep;

        juce::Colour segCol = kElectricCyan;
        if (segNorm > 0.88f)      segCol = kCrimsonRed;
        else if (segNorm > 0.72f) segCol = kWarmAmber;
        else if (segNorm > 0.45f) segCol = kEmeraldSafe;

        const bool litL = (normL >= segNorm && m.smoothL > 0.0001f) || (segFromBot == holdSegL && m.holdL > 0.001f);
        const bool litR = (normR >= segNorm && m.smoothR > 0.0001f) || (segFromBot == holdSegR && m.holdR > 0.001f);

        g.setColour (litL ? segCol : segCol.withAlpha (0.11f));
        g.fillRect (leftBarX + 1.0f, yPos, barW - 2.0f, segH);

        g.setColour (litR ? segCol : segCol.withAlpha (0.11f));
        g.fillRect (rightBarX + 1.0f, yPos, barW - 2.0f, segH);
    }

    // Center dBFS Scale Markings
    g.setFont (makeAutoFont (8.5f, true, true));
    g.setColour (kChromeMid.withAlpha (0.85f));
    const int marks[] = { 0, -6, -12, -24, -36 };
    for (int mark : marks)
    {
        const float n = (static_cast<float> (mark) - dbFloor) / (-dbFloor);
        const float yM = barBot - n * barH - 5.0f;
        g.drawText (juce::String (std::abs (mark)),
                    static_cast<int> (leftBarX + barW), static_cast<int> (yM),
                    static_cast<int> (rightBarX - (leftBarX + barW)), 10, juce::Justification::centred);
    }

    // L / R Channel Labels & Peak Hold Readout
    g.setFont (makeAutoFont (9.5f, true, true));
    g.setColour (kChromeBright);
    g.drawText ("L", static_cast<int> (leftBarX),  static_cast<int> (barBot + 3.0f), static_cast<int> (barW), 12, juce::Justification::centred);
    g.drawText ("R", static_cast<int> (rightBarX), static_cast<int> (barBot + 3.0f), static_cast<int> (barW), 12, juce::Justification::centred);

    const float maxPeak = juce::jmax (m.holdL, m.holdR);
    const juce::String peakTxt = (maxPeak <= 0.0005f) ? "-INF"
                                                      : juce::String (juce::Decibels::gainToDecibels (maxPeak), 1);

    g.setFont (makeAutoFont (10.5f, true, true));
    g.setColour (clipL || clipR ? kCrimsonRed : kElectricCyan);
    g.drawText (peakTxt, rect.getX() + 4, rect.getBottom() - 21, rect.getWidth() - 8, 16, juce::Justification::centred);
}

void VulcanEditor::drawFaceplate (juce::Graphics& g)
{
    if (chassisCache.isValid())
        g.drawImageAt (chassisCache, 0, 0);

    // 4 Interactive CNC Corner Screws
    for (size_t i = 0; i < screwCenters.size(); ++i)
        drawChromeScrew (g, screwCenters[i], 6.0f, screwAngles[i]);

    // Header Sapphire Jewel + Metallic Badge + Chrome Title
    drawSapphireJewel (g, { 42.0f, 43.0f }, 7.5f);
    drawMetallicEmblem (g, { 60.0f, 30.0f, 28.0f, 26.0f });

    g.setFont (makeAutoFont (24.0f, true, false));
    g.setColour (kChromeBright);
    g.drawText ("VULCAN", 96, 23, 125, 26, juce::Justification::centredLeft);

    g.setFont (makeAutoFont (9.5f, true, true));
    g.setColour (kElectricCyan.withAlpha (0.88f));
    g.drawText ("PHASE & TRANSIENT MASTERING ENGINE", 97, 47, 275, 13, juce::Justification::centredLeft);

    // Center Smoked-Glass Telemetry Display
    const auto pill = telemetryRect.toFloat();
    g.setColour (juce::Colour (0xff040b16).withAlpha (0.88f));
    g.fillRoundedRectangle (pill, 5.0f);
    g.setColour (kChromeMid.withAlpha (0.38f));
    g.drawRoundedRectangle (pill, 5.0f, 1.0f);

    const double sr = audioProcessor.getSampleRate() > 0.0 ? audioProcessor.getSampleRate() : 44100.0;
    const float rateMult = static_cast<float> (sr / 44100.0);
    const float widthVal = static_cast<float> (stereoWidthKnob.slider.getValue());
    const int dMid  = static_cast<int> (std::round (23.0f * rateMult * widthVal));
    const int dHigh = static_cast<int> (std::round (41.0f * rateMult * widthVal));
    const float msHigh = (static_cast<float> (dHigh) / static_cast<float> (sr)) * 1000.0f;

    const juce::String teleStr = "HAAS: " + juce::String (dMid) + "/" + juce::String (dHigh)
                               + " SMP (" + juce::String (msHigh, 2) + " ms)  |  "
                               + juce::String (sr / 1000.0, 1) + " kHz";

    g.setFont (makeAutoFont (11.0f, true, true));
    g.setColour (kElectricCyan);
    g.drawText (teleStr, telemetryRect, juce::Justification::centred);
}

void VulcanEditor::paint (juce::Graphics& g)
{
    drawFaceplate (g);

    // Dual Stereo VU/Peak Meter Bridges (Input on Left, Output on Right)
    drawStereoMeterBridge (g, inMeterRect,  "INPUT",  inMeter);
    drawStereoMeterBridge (g, outMeterRect, "OUTPUT", outMeter);

    // Parameter Titles above all 9 3D Knobs
    drawKnobLabels (g, inGainKnob);
    drawKnobLabels (g, monoFreqKnob);
    drawKnobLabels (g, stereoWidthKnob);
    drawKnobLabels (g, characterKnob);
    drawKnobLabels (g, outGainKnob);

    drawKnobLabels (g, eqLowKnob);
    drawKnobLabels (g, eqMidKnob);
    drawKnobLabels (g, eqHighKnob);
    drawKnobLabels (g, limiterDriveKnob);
}
