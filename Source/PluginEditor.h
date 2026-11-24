#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <optional>
#include <vector>

//==============================================================================
// VulcanEditor — JUCE 8 Hardware-Accelerated WebView2 / WKWebView Edition
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
    void pushFullSyncToWeb();
    std::optional<juce::WebBrowserComponent::Resource> handleResourceRequest (const juce::String& url);

    VulcanProcessor& audioProcessor;

    int lastSyncedPreset = -1;
    bool lastSyncedModified = false;

    // =========================================================================
    // STRICT JUCE 8 LIFECYCLE ORDER (DO NOT REORDER THESE 3 BLOCKS):
    // 1. Relays (constructed first, destroyed last)
    // 2. WebBrowserComponent (constructed after relays, destroyed before relays)
    // 3. ParameterAttachments (constructed last, destroyed first)
    // =========================================================================

    // 1. Parameter Relays
    juce::WebSliderRelay inGainRelay       { "input_gain" };
    juce::WebSliderRelay eqLowRelay        { "eq_low" };
    juce::WebSliderRelay eqMidRelay        { "eq_mid" };
    juce::WebSliderRelay eqHighRelay       { "eq_high" };
    juce::WebSliderRelay monoFreqRelay     { "mono_freq" };
    juce::WebSliderRelay stereoWidthRelay  { "haas_amount" };
    juce::WebSliderRelay characterRelay    { "amount" };
    juce::WebSliderRelay limiterDriveRelay { "limiter_drive" };
    juce::WebSliderRelay outGainRelay      { "output_gain" };

    // 2. Embedded Browser Instance
    juce::WebBrowserComponent webView;

    // 3. APVTS <-> WebSliderRelay Attachments
    juce::WebSliderParameterAttachment inGainAttachment;
    juce::WebSliderParameterAttachment eqLowAttachment;
    juce::WebSliderParameterAttachment eqMidAttachment;
    juce::WebSliderParameterAttachment eqHighAttachment;
    juce::WebSliderParameterAttachment monoFreqAttachment;
    juce::WebSliderParameterAttachment stereoWidthAttachment;
    juce::WebSliderParameterAttachment characterAttachment;
    juce::WebSliderParameterAttachment limiterDriveAttachment;
    juce::WebSliderParameterAttachment outGainAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VulcanEditor)
};
