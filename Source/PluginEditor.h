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
    void handleParamGestureFromWeb (const juce::String& paramId, bool isStarting);
    void handleSetParamFromWeb (const juce::String& paramId, float normValue);
    std::optional<juce::WebBrowserComponent::Resource> handleResourceRequest (const juce::String& url);

    VulcanProcessor& audioProcessor;

    int lastSyncedPreset = -1;
    bool lastSyncedModified = false;

    juce::WebBrowserComponent webView;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VulcanEditor)
};
